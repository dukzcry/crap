#!/usr/bin/env nix-shell
#!nix-shell -i python3 -p gobjectIntrospection "[ libnotify libappindicator-gtk3 python3Packages.pykde4 ]" "python3.withPackages (ps: with ps; [ pygobject3 dbus-python sqlalchemy pillow python-fontconfig ])"

import gi
gi.require_version('Gtk', '3.0')
gi.require_version('Notify', '0.7')
from gi.repository import Notify
from gi.repository.Gtk import IconTheme

import dbus
import time
import os
import socket
from sys import argv
from urllib.parse import urlparse
from dbus.mainloop.glib import DBusGMainLoop
from gi.repository import GLib

from sqlalchemy import *
from sqlalchemy.orm import *

from PIL import Image,ImageFont,ImageDraw
from fontconfig import query

### configuration ###
progname = "notifier"

dir = "/tmp/" + progname + "/"

fonts = query(family='DejaVu Sans')
for i in range(0,len(fonts)):
    if fonts[i].fontformat == 'TrueType' and [item for item in fonts[i].style if 'Bold' in item] != []:
        #print(fonts[i].family)
        fontpath = fonts[i].file
        break

limit = 30

size = (128, 128)

#gi.require_version('AppIndicator3', '0.1')
#from gi.repository import AppIndicator3
#from gi.repository import Gtk
#notifierClass = lambda: GNotifier()

from PyQt4.Qt import QApplication, QIcon
import PyKDE4.kdeui as kdeui
notifierClass = lambda: KNotifier()
### end of configuration ###

class GNotifier():
    def __init__(self):
        img = Image.new("RGBA", size, (0,0,0,0))
        path = dir + '_blank_.png'
        img.save(path)

    def create_indicator(self, name):
        indicator = AppIndicator3.Indicator.new_with_path(progname + name, '_blank_', AppIndicator3.IndicatorCategory.APPLICATION_STATUS, dir)
        self.show_icon(indicator, True)
        self.create_menu(indicator, name)
        return indicator

    def create_menu(self, indicator, name):
        # hackish, we don't reuse menu
        menu = Gtk.Menu()
        item = Gtk.MenuItem(label='Clear ' + name)
        separator = Gtk.SeparatorMenuItem()
        item.connect('activate', lambda self: clear(name))
        menu.append(item)
        menu.append(separator)
        menu.show_all()
        indicator.set_menu(menu)

    def show_icon(self, indicator, switch):
        if switch:
            indicator.set_status(AppIndicator3.IndicatorStatus.ACTIVE)
        else:
            indicator.set_status(AppIndicator3.IndicatorStatus.PASSIVE)

    def fill_menu(self, message, indicator, string):
        parent = indicator.get_menu()
        summary = Gtk.MenuItem(label=string)
        if message.body:
            submenu = Gtk.Menu()
            body = Gtk.MenuItem(label=message.body)
            submenu.append(body)
            body.show()
            summary.set_submenu(submenu)
        parent.append(summary)
        summary.show()

    def set_icon(self, indicator, name):
        # delay is needed for icon by name
        # icon by path works much better, but is not in spec
        GLib.timeout_add_seconds(3, lambda: (
          # force icon refresh
          indicator.set_icon_full("", ""),
          indicator.set_icon_full(name, "")
        ))

class KNotifier():
    def __init__(self):
        # don't pull gtk2 from gtk+ plugin
        self.app = QApplication([argv[0], "--style=motif"])
        img = Image.new("RGBA", size, (0,0,0,0))
        path = dir + '_blank_.png'
        img.save(path)

    def create_indicator(self, name):
        indicator = kdeui.KStatusNotifierItem(progname + name, None)
        self.show_icon(indicator, True)
        self.create_menu(indicator, name)
        return indicator

    def create_menu(self, indicator, name):
        menu = indicator.contextMenu()
        menu.clear()
        menu.addAction('Clear ' + name, lambda: clear(name))
        menu.addSeparator()

    def show_icon(self, indicator, switch):
        if switch:
            indicator.setStatus(kdeui.KStatusNotifierItem.Active)
        else:
            # doesn't hide the icon, just gives the hint to tray
            indicator.setStatus(kdeui.KStatusNotifierItem.Passive)
            self.set_icon(indicator, '_blank_')

    def fill_menu(self, message, indicator, string):
        parent = indicator.contextMenu()
        if message.body:
            submenu = parent.addMenu(string)
            submenu.addAction(message.body)
        else:
            parent.addAction(string)

    def set_icon(self, indicator, name):
        indicator.setIconByPixmap(QIcon(dir + name + '.png'))

class Message_(object):
    def __init__(self, args, key, time):
        self.key = key
        self.time = time
        # https://developer.gnome.org/notification-spec/
        self.app_name = args[0]
        self.replaces_id = args[1]
        self.app_icon = args[2]
        self.summary = args[3]
        self.body = args[4]
        self.actions = args[5]
        self.hints = args[6]
        self.expire_timeout = args[7]

        if self.app_name == "" or not has_icon(self.app_icon):
            self.name = "generic"
        else:
            self.name = self.app_name

def create_metadata(name, metadata):
    return Table(name, metadata,
        Column('key',Integer,primary_key=True),
        Column('time',Integer),
        Column('app_name',String),
        Column('replaces_id',Integer),
        Column('app_icon',String),
        Column('summary',String),
        Column('body',String),
        Column('expire_timeout',Integer),
        Column('name',String)
    )

def notifications(bus, message):
    try:
        args = message.get_args_list()
        if message.get_member() == "Notify" and message.get_interface() == "org.freedesktop.Notifications" and len(args) == 8:
            name = Message_(args, 0, 0).name
            (Message,indicator) = get_mapping(name, Message_)
            excess(name, Message, indicator)
            notifier.show_icon(indicator, True)
            if not engine.dialect.has_table(engine, name):
                table = create_metadata(name, metadata)
                table.create()
                mapper(Message, table)
            timetime = time.time()
            message = Message(args, int(str(timetime).replace('.','')), int(timetime))
            menu_item(message, indicator)
            session = Session()
            try:
                session.add(message)
                session.commit()
                draw_badge(session.query(Message).count(), message, indicator)
            except Exception as msg:
                session.rollback()
                print(("exc notifications",msg,args))
            finally:
                session.close()
    except Exception as msg:
        print(msg)

def excess(name, message, indicator):
    if limit == 0:
        return
    session = Session()
    real_limit = limit-1
    try:
        items = session.query(message).order_by(desc(message.key)).all()
        session.commit()
        if len(items) > real_limit:
            clear(name, [item.key for item in items[real_limit:]])
            for item in reversed(items[:real_limit]):
                menu_item(item, indicator)
    except Exception as msg:
        session.rollback()
        print(("exc excess",msg))
    finally:
        session.close()

def menu_item(message, indicator):
    string = time.asctime(time.localtime(message.time)) + " " + message.app_name  + " " + message.summary
    notifier.fill_menu(message, indicator, string)

def clear(name, items=None):
    (message,indicator) = mappings.get(name)
    if items is None:
        notifier.show_icon(indicator, False)
    notifier.create_menu(indicator, name)
    session = Session()
    try:
        if items is not None:
            session.query(message).filter(message.key.in_(items)).delete(synchronize_session='fetch')
        else:
            session.query(message).delete()
        session.commit()
    except Exception as msg:
        session.rollback()
        print(("exc clear",msg))
    finally:
        session.close()

# stupid
def get_mapping(name, cls):
    mapping = mappings.get(name)
    if not mapping:
        value = type(name, (cls,), {})
        indicator = notifier.create_indicator(name)
        mappings.update({name: (value,indicator)})
        mapping = (value,indicator)
    return mapping

def has_icon(icon):
    if icon == "":
        return False

    sizes = icon_theme.get_icon_sizes(icon)
    if sizes != []:
        # svg icon only
        if size == [-1]:
            return False
        else:
            return True

    o = urlparse(icon)
    if o.scheme == 'file':
        try:
            Image.open(o.path).verify()
            return True
        except Exception:
            pass

    return False

# google chrome uses temporary icons
def save_icon(message):
    icon = message.app_icon
    sizes = icon_theme.get_icon_sizes(icon)
    if sizes != []:
        s = min(sizes, key=lambda x: abs(x-size[0]))
        path = icon_theme.lookup_icon(icon, s, 0).get_filename()
    else:
        path = urlparse(icon).path

    try:
        img = Image.open(path)
        img = img.resize(size)
    except Exception:
        img = Image.new("RGBA", size, (0,0,0,0))

    img.save(dir + message.name + '.png')

def draw_badge(num, message, indicator):
    name = message.name
    path = dir + name + '.png'
    if not os.path.exists(path):
        save_icon(message)

    text = str(num)
    img = Image.open(path)
    width, height = img.size

    draw = ImageDraw.Draw(img)

    draw.rectangle([0, 0, width/2, height/2], fill='red', outline='red')

    fontsize=1
    font = ImageFont.truetype(font=fontpath, size=fontsize)
    while font.getsize(text)[0] < width/2 and font.getsize(text)[1] < height/2:
        fontsize += 1
        font = ImageFont.truetype(font=fontpath, size=fontsize)

    #fontsize -= 1
    #font = ImageFont.truetype(font=fontpath, size=fontsize)

    draw.text((0,0), text, fill='white', font=font)

    path = dir + name + '_counter.png'
    img.save(path)
    notifier.set_icon(indicator, name + '_counter')

sock = socket.socket(socket.AF_UNIX, socket.SOCK_DGRAM)
sock.bind('\0' + progname)

if not os.path.exists(dir):
    os.makedirs(dir)

notifier = notifierClass()

# bring notification daemon in case it's not running
Notify.init(progname)
Notify.Notification.new("started " + progname).show()
Notify.uninit()

DBusGMainLoop(set_as_default=True)
bus = dbus.SessionBus()
string = "eavesdrop=true, interface='org.freedesktop.Notifications', member='Notify'"
bus.add_match_string_non_blocking(string)
bus.add_message_filter(notifications)

engine = create_engine('sqlite:///' + dir + progname + '.db', echo=True)
metadata = MetaData(engine)
metadata.reflect(bind=engine)
Session = sessionmaker(bind=engine)

#icon_theme = IconTheme.get_default()
# wayland compat
icon_theme = IconTheme()
icon_theme.set_custom_theme("hicolor")

mappings = {}

session_= Session()
try:
    for t in metadata.tables.keys():
        table = metadata.tables[t]
        (message,indicator) = get_mapping(t, Message_)
        mapper(message, table)
        messages = session_.query(message)
        i = 0
        for m in messages:
            menu_item(m, indicator)
            i += 1
        if i == 0:
            notifier.show_icon(indicator, False)
        else:
            draw_badge(i, m, indicator)
except Exception as msg:
    session_.rollback()
    print(("exc start",msg))
finally:
    session_.close()

mainloop = GLib.MainLoop()
if len(argv) == 2 and argv[1] == "-f":
    mainloop.run()
else:
    pid = os.fork()
    if pid == 0:
        mainloop.run()
