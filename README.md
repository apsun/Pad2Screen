# Pad2Screen

A Windows kernel driver that converts any Windows precision touchpad into a
touchscreen (i.e. gives you absolute cursor positioning).

This effectively supersedes [AbsoluteTouch](https://github.com/apsun/AbsoluteTouch),
a program that (poorly) emulated the functionality of this driver in
userspace. That program suffered from slow performance and only worked on
Synaptics touchpads.

WARNING: This is my first time writing a Windows kernel driver. As such,
there is a very high chance that this driver will cause BSODs or other issues.
If your computer is used for anything mission critical, DO NOT INSTALL THIS
DRIVER. PLEASE.

## Why should I use Pad2Screen?

- It's significantly faster than AbsoluteTouch
- It works (or at least it should) with any Windows precision touchpad
- You get true touchscreen functionality on programs that support them

## Why should I NOT use Pad2Screen?

- It's a kernel driver, so there's a risk it will brick your system
- It's (currently) unsigned, so you'll have to disable driver signature verification
- It does not work with UMDF (user-mode driver framework) touchpad drivers

## How does it work?

Touchpads send their their input data over HID (human interface device).
In the Windows kernel, there exists a generic driver which takes a HID report
(essentially a packet) and a HID report descriptor (similar to a struct
definition in C) from any input device, in our case a touchpad, and
translates the data to an event that gets delivered to your application.

How does Windows know that the HID reports are coming from a touchpad?
That's easy - just look at the HID report descriptor. It contains a "usage"
field that tells the HID driver what the source of the data is - mouse,
keyboard, touchpad, touchscreen, or something else.

Where does Pad2Screen come in? In a nutshell, it sits between the HID and
touchpad drivers and hooks the report descriptor to change the usage field
from "touchpad" to "touchscreen". Everything else remains the same, and it
just so happens that the report formats are similar enough to "just work" out
of the box, on current Windows versions at least.
