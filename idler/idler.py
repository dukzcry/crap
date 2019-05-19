#!/usr/bin/env nix-shell
#!nix-shell -i python3 -p gobjectIntrospection "python3.withPackages(ps: with ps; [ evdev pygobject3 dbus-python ])"

# bug: while stucked waiting for an input event we can't account new device if such was added

from time import sleep
from datetime import datetime
from threading import Timer
from select import select
from sys import argv
from glob import glob
import threading
import evdev.ecodes as ecodes
import evdev,logging,socket,os

import dbus
import dbus.service
from dbus.mainloop.glib import DBusGMainLoop
from gi.repository import GLib

alarm_interval = 600
throttle_interval = 60
inhibitors_interval = 3600
jitter = 10
no_battery = False
low_battery_level = range(0, 6)
battery_path = "/sys/class/power_supply/BAT?/"
adapter_path = "/sys/class/power_supply/ADP?/"
progname = "idler"
log_file = "/tmp/"+progname+".log"
ac_command = "sudo chvt 20"
#ac_command = "sudo physlock -d"
wakeup_command = ac_command
battery_command = wakeup_command + "; sudo systemctl suspend"
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

    if inhibitors != {}:
        logging.warning("sleep prevented by dbus %s", inhibitors)
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
        logging.warning("adapter %s -> %s", ctx.adapterPresent, newAdapterPresent)
        ctx.adapterPresent = newAdapterPresent
    ctx.battery_timer.new()

def batteryCapacity():
    [path] = glob(battery_path)
    with open(path + "capacity", "r") as myfile:
        return int(myfile.read().replace("\n", ""))

def adapterPresent():
    if no_battery:
        return 1
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

service_names = [ "org.freedesktop.ScreenSaver"
                  ,"org.freedesktop.PowerManagement.Inhibit"
                  ,"org.mate.SessionManager"
                  ,"org.gnome.SessionManager" ]
object_paths = [ "/ScreenSaver"
                 ,"/org/freedesktop/PowerManagement"
                 ,"/org/mate/SessionManager"
                 ,"/org/gnome/SessionManager" ]
inhibitors = {}

def inhibitorsHandler(ctx):
        ctx.inhibitors_timer.new()
        if wakeup(ctx, False):
            return

        global inhibitors
        if inhibitors != {}:
            logging.warning("cleaning stale dbus inhibitors %s", inhibitors)
            inhibitors = {}
            ctx.idle_timer.reset()

class Main:
    def main(self):
        self.adapterPresent = adapterPresent()

        logging.basicConfig(format="%(asctime)s %(message)s", filename=log_file)
        logging.getLogger().addHandler(logging.StreamHandler())

        if not no_battery:
            self.battery_timer = Watchdog(throttle_interval, batteryHandler, self)
        self.idle_timer = Watchdog(alarm_interval, idleHandler, self, idleStartHandler)
        self.inhibitors_timer = Watchdog(inhibitors_interval, inhibitorsHandler, self)

        myT = threading.Thread(target=self.myThread)
        myT.start()

        DBusGMainLoop(set_as_default=True)

        self.session_bus = dbus.SessionBus()
        services = zip(service_names, object_paths)
        objects = list(map(lambda xy: self.service_init(xy[0], xy[1], self), services))
        logging.warning("started")

        mainloop = GLib.MainLoop()
        mainloop.run()

    def myThread(self):
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

    def service_init(self, service_name, object_path, ctx):
        class Service(dbus.service.Object):
            def __init__(self, service_name, object_path):
                name = dbus.service.BusName(service_name, bus=ctx.session_bus)
                super().__init__(name, object_path)
            @dbus.service.method(dbus_interface=service_name)
            def Inhibit(self, app, *args):
                key = 1
                while inhibitors.get(key) != None:
                    key += 1
                inhibitors.update({key: app})
                ctx.inhibitors_timer.reset()
                logging.warning("dbus inhibit %s", inhibitors)
                return dbus.UInt32(key)
            @dbus.service.method(dbus_interface=service_name)
            def UnInhibit(self, key):
                inhibitors.pop(key)
                logging.warning("dbus uninhibit %s", inhibitors)
            @dbus.service.method(dbus_interface=service_name)
            def Uninhibit(self, key):
                inhibitors.pop(key)
                logging.warning("dbus uninhibit %s", inhibitors)
                logging.warning(inhibitors)

        return Service(service_name, object_path)


sock = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
sock.bind('\0' + progname)

program = Main()
if len(argv) == 2 and argv[1] == "-f":
    program.main()
else:
    pid = os.fork()
    if pid == 0:
        program.main()
