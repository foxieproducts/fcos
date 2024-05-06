1.11
----
- Add additional time on boot to wait for RTC to respond
- Now saves settings immediately on wifi connection

1.10
----
- Add functional test to first power-up (requires using 3d printed cover (in black) to isolate light sensor)
- Add support for CardClock 2.0
- Add support for a few more UTC timezones (UTC-3 to +3)

1.09
----
- Minor improvements, now using latest espressif SDK for better wireless reliability.

1.08
----
- Add new animation mode 5 with configurable colors per column. Use short left/right presses to select active digit, then up/down to change the color of that column. 
- Also now briefly shows animation mode # when clicking center button in white.

1.07
----
- Fix bug with not displaying the updated timezone selection

1.06
----
- Add partial support for BME680 sensor -- displays temperature in F and C
  if one is plugged in before powering up. _Don't hot plug_!
  See output by selecting INFO, then use the up button to go past
  version, light sensor val, IP addr, and uptime, to reach Temp F and C.
  Zero is displayed when no sensor is detected.
- Improve WiFi configuration behavior so that "2" always means "config"
- Improve PXL mode brightness behavior
- Hold joystick left then plug in the USB cable to go "back" to default
  settings. The colon will light up orange until you let go of the joystick,
  then will reboot with all settings cleared.

1.05
----
- Another attempt to kill the rare green flicker, this time
  using bitbanging instead of the RMT peripheral

1.04
----
- Another attempt to reduce rare green flicker

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
It only supports the ESP32-C3-based Foxie Clock 2.0, 
but will also support the CardClock 1.0 and future clocks... soon.
Note: There is no Bluetooth support in the 1.0 firmware. It
      could potentially be used to control other BT devices. Maybe
      even original Foxie Clocks with the SparkFun Artemis Nano :)

The first 4 characters in this file are read over the internet
by the firmware when an update check is performed. Currently, 
the firmware only checks for updates when the user selects UPDT
Eventually, a check could be performed once per day, and then the
update LED could gently pulsate orange?
