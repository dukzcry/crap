#!/usr/bin/env nix-shell
#!nix-shell -i python3 -p "python3.withPackages(ps: with ps; [ evdev ])"

# bug: while stucked waiting for an input event we can't account new device if such was added

from time import sleep
from datetime import datetime
from threading import Timer
from select import select
from sys import argv
from glob import glob
import evdev.ecodes as ecodes
import evdev,logging,socket,os

alarm_interval = 600
throttle_interval = 60
jitter = 10
low_battery_level = range(0, 6)
battery_path = "/sys/class/power_supply/BAT?/"
adapter_path = "/sys/class/power_supply/ADP?/"
progname = "idler"
log_file = "/tmp/"+progname+".log"
ac_command = "sudo pkill physlock; sudo physlock -d"
wakeup_command = ac_command
battery_command = ac_command + "; sudo systemctl suspend"
low_battery_command = battery_command

keyDown = evdev.events.KeyEvent.key_down
relX = next((k for k, v in ecodes.REL.items() if v == "REL_X"), None)

class Watchdog:
    def __init__(self, timeout, handler, context, startHandler=None):
        self.__timeout = timeout
        self.__handler = lambda: handler(context)
        self.__startHandler = lambda: startHandler(context) if startHandler is not None else ()
        self.new()

    def new(self):
        self.__timer = Timer(self.__timeout, self.__handler)
        self.__startHandler()
        self.__timer.start()

    def reset(self):
        self.stop()
        self.new()

    def stop(self):
        self.__timer.cancel()

def wakeup(ctx, update):
    # insecure but simple and has no dependence on acpid/logind/...
    newTime = datetime.now()
    delta = newTime - ctx.time
    if update:
        ctx.time = newTime
    return delta.total_seconds() > alarm_interval + jitter

def idleHandler(ctx):
    if wakeup(ctx, True):
        logging.warning("wakeup action")
        os.system(wakeup_command)
        return

    if ctx.adapterPresent == 0:
        logging.warning("battery action")
        os.system(battery_command)
    else:
        logging.warning("ac action")
        os.system(ac_command)

def idleStartHandler(ctx):
    ctx.time = datetime.now()

def batteryHandler(ctx):
    newAdapterPresent = adapterPresent()
    # not all batteries send acpi events
    if newAdapterPresent == 0 and batteryCapacity() in low_battery_level:
        ctx.idle_timer.stop()
        logging.warning("low battery action")
        os.system(low_battery_command)
    elif ctx.adapterPresent != newAdapterPresent:
        if not wakeup(ctx, False):
            ctx.idle_timer.reset()
        logging.warning(("adapter", ctx.adapterPresent, newAdapterPresent))
        ctx.adapterPresent = newAdapterPresent
    ctx.battery_timer.new()

def batteryCapacity():
    [path] = glob(battery_path)
    with open(path + "capacity", "r") as myfile:
        return int(myfile.read().replace("\n", ""))

def adapterPresent():
    [path] = glob(adapter_path)
    with open(path + "online", "r") as myfile:
        return int(myfile.read().replace("\n", ""))

def devices():
    devices = []
    for fn in evdev.list_devices():
        try:
            dev = evdev.InputDevice(fn)
            cap = dev.capabilities()
            #logging.warning(dev)
            if ecodes.EV_KEY in cap:
                if ecodes.BTN_MOUSE in cap[ecodes.EV_KEY] or ecodes.KEY_ENTER in cap[ecodes.EV_KEY]:
                    devices.append(dev)
        except Exception as dev:
            #logging.warning(dev)
            pass
    return {dev.fd: dev for dev in devices}

class Main:
    def main(self):
        self.adapterPresent = adapterPresent()

        logging.basicConfig(format="%(asctime)s %(message)s", filename=log_file)
        logging.getLogger().addHandler(logging.StreamHandler())

        self.battery_timer = Watchdog(throttle_interval, batteryHandler, self)
        self.idle_timer = Watchdog(alarm_interval, idleHandler, self, idleStartHandler)

        logging.warning("started")

        while True:
            reset = False
            devs = devices()

            r, w, x = select(devs, [], [])
            for fd in r:
                try:
                    for event in devs[fd].read():
                        if event.type == ecodes.EV_KEY and event.value == keyDown or event.type == ecodes.EV_REL and event.code == relX:
                            reset = True
                            #logging.warning(evdev.categorize(event))
                            break
                except Exception as msg:
                    logging.warning(msg)
                    break
                if reset:
                    break
            if reset:
                self.idle_timer.reset()
            sleep(throttle_interval)

sock = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
sock.bind('\0' + progname)

program = Main()
if len(argv) == 2 and argv[1] == "-f":
    program.main()
else:
    pid = os.fork()
    if pid == 0:
        program.main()
