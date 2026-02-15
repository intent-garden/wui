# WUI

## Window User Interface Library

WUI is a cross-platform modern C++ library for creating graphical user interfaces.
Now it support only Windows and Linux*.

WUI is an attempt to make as easy to use cross-platform library as possible for creating a modern graphical user interface in C++. The library uses C++17 and has a minimalistic API.

## Tasks for the UI framework

- Run on Windows (At least 7, but works on XP as well)
- Run on Linux (Starting with Ubuntu 16 / CentOS6)
- Run on macOS
- Open windows and display controls on them. 
- Provide a common interface to the drawing subsystem that hides platform-dependent methods. This will allow you to write a control once, on any platform, and it will look and behave the same on all.
- Provide a common interface to events. Any control or user can subscribe to any message group, including custom ones, with the ability to send/receive messages asynchronously.
- Receive system messages, respond to mouse, keyboard and other events.
- Have the ability to change the color scheme / style / icons / images of all controls / windows from one place. Store all visual settings of the application in json, including having the ability to store images in it as well.
- Provide a system of text constants for titles and labels depending on the language selected.
- Have the ability to detach / attach windows from each other.
- Provide an implementation of basic UI controls and have a clear and accessible way for third party developers to add new controls for their applications.
- Have a user-friendly interface for working with application configs. Windows registry and ini files are supported. Naturally, with the possibility of modification.

## How it works

![UI framework principals](https://ud84.github.io/wui/doc/en/img/system.png)

Everything is based on two entities - Window and Control. A window can contain controls, and the window itself is a control.

Control is any visual element for user interaction - button, input field, list, menu, etc. Control knows how to handle events coming from Window, stores its states, and draws itself on the graphical context provided by the window containing it.

Window - receives system events and provides their distribution to subscribers. The window also commands its controllers to redraw and provides them with their own graphic. In addition, the window controls the input focus, can do modality and send an event to the subscribed user or to the system.

Graphic is the third base entity that provides an interface to the system's drawing methods. Currently, drawing is implemented on Windows GDI/GDI+ and Linux xcb/cairo. Of course, there is no obstacle to implement drawing on vulcan/bare metal/etc.

The library also has auxiliary tools for work - structures common (containing such basic types as rect, color), event (mouse, keyboard, internal and system events), graphic (for physical rendering on the system graphical context) and Theme (a system of constants for convenient support of visual themes).

## Start

[Onboarding article](doc/en/article/onboarding.md)

Docs: https://libwui.org/doc

Web site: https://libwui.org

Reddit community: https://www.reddit.com/r/wui/

Telegram: https://t.me/libwui_chat

Email: info@libwui.org

## Example screenshot (Video call application)

![WUI screen 1](https://libwui.org/main/img/screenshoot-1.png)
![WUI screen 2](https://libwui.org/main/img/screenshoot-2.png)

## SAST Tools

[PVS-Studio](https://pvs-studio.com/pvs-studio/?utm_source=website&utm_medium=github&utm_campaign=open_source) - static analyzer for C, C++, C#, and Java code.

## Maintainer

Intent Garden Org
