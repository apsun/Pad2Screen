# Pad2Screen

A Windows kernel driver that converts any Windows precision touchpad into a
touchscreen (i.e. gives you absolute cursor positioning).

This project was scrapped due to the huge variety of Windows touchpad drivers,
some of which are worse than others. It was initially intended to be a lower
filter for mshidkmdf, but as it turns out, not all touchpad drivers even use
it. This code has never been tested, do not expect support for it. I am
releasing the code in the hopes that someone will find it useful. For a working
alternative to this project, see
[AbsoluteTouchEx](https://github.com/apsun/AbsoluteTouchEx).

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
