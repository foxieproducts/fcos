1.03
----
- Reduce rare green flicker

1.02
----
- Improve blinker and dark mode behavior

1.01
----
This is a simple revision to be confirm the entire end to end update process.

1.00
----
This is the initial release of the FCOS firmware. 
It primarily supports the ESP32-C3-based Foxie Clock 2.0, 
but will also support the CardClock 1.0 and future clocks. 
Note: There is no Bluetooth support in the 1.0 firmware. It
      could potentially be used to control over BT devices.

The first 4 characters in this file are read over the internet
by the firmware when an update check is performed. Currently, 
the firmware only checks for firmware updates when the user
asks it to update. Perhaps eventually, a check could be performed
once per day, and then the update LED could be lit up in orange?
