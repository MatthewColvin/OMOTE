## OMOTE Status Bar
The STatus Bar is positioned at the top of most screens.

![](images/StatusBar.png)

The right hand side provides details of the WiFi status and battery level.  The final field can either be the battery State Of Charge (SOC) in % or the current time.  Tapping on the right hand side of the status bar opens the Settings Menu.

The left hand side use is dependant on which UI is being used.  For the Home Assist UI it is used to display/access the active device selection menus.  For the JSON UI it is used to display/select the current scene.

## OMOTE Settings Menu
<div align="center">
  <img src="images/SettingsMenu.png">
</div>

The settings menu allows the OMOTE to be configured, there are sub-menus for each area:

### Backlight Settings
<div align="center">
  <img src="images/BacklightMenu.png">
</div

This menu will vary dependant on the OMOTE hardware revision the code has been built. For Rev1 hardware it will just show a single slider that allows the LCD backlight brightness to be set.  For Rev5 hardware there will be two sliders which allow the LCD and keypad brightness to be independently set.  For Rev5_3661 hardware there will be an additional two sliders so that the brightness can be set for both day and night modes (OMOTE then automatically switches between these based on ambient light levels).

### Sleep Settings
<div align="center">
  <img src="images/SleepMenu.png">
</div

This menu allows the Sleep settings to be configured. The OMOTE can use both light and deep sleep modes.  Deep sleep provides lower power consumption but takes longer to wake, light sleep allows almost instant wake but at the cost of increased power draw.  

If light sleep is disabled the OMOTE will always go directly into deep sleep to maximise battery life.  If light sleep is enabled the OMOTE will go into light sleep first and will stay in that mode for up to the duration set.  If woken within that time it will power up instantly.  If not woken in that time it will automatically revert to deep sleep.  

For reference the power consumed for each of teh following are roughly equivalent (based on Rev5 hardware):

	1day of deep sleep = 60min of light sleep = 30sec of normal operation

Typical settings would be a Timeout of 10-20sec and a light sleep duration of 1hour.  This will give fast wake while still dropping back into deep sleep when not used.

**Note:** For the JSON UI light sleep is only used when within a scene.  If not in a scene the OMOTE will go directly to deep sleep even if light sleep is enabled.  The same effect can be achieved when programming in the Basic UI by calling:

	HardwareFactory::getAbstract().setInScene(true/false);

**Note:** Light sleep has, to date, only been tested on Rev5_3661 hardware.

### WiFi Settings
<div align="center">
  <img src="images/WiFiMenu.png">
</div

The WiFi menu displays the currently available WiFi networks available.  If the OMOTE is currently connected to one of these then the WiFi symbol will be shown next to its name.  Any line can be tapped on to connect to that network.  An on-screen keyboard will then be displayed to allow the network password to be entered.  To accept the password entered and connect tap the tick, to cancel tap the keyboard icon.  If the OMOTE successfully connects it will save the details to the preferences store and they will be used in future. 


### MQTT Settings
<div align="center">
  <img src="images/MqttMenu.png">
</div

The OMOTE can connect to a MQTT server to allow it to send commands or receive data.  This screen allows the server details to be configured.  Tapping on any line will pop up the on-screen keyboard to allow the details to be entered.  Once all details have been entered click Connect/Save to connect to the server.

### NTP Settings
<div align="center">
  <img src="images/NtpMenu.png">
</div

The OMOTE can subscribe to a network time server to synchronise it's real time clock.  This menu allows this feature to be enabled or disabled and the server details to be set.  It is also possible to select how the time is displayed, the options are:
* Don't display the time (Disabled)
* Display the time constantly (will replace the battery SOC% display in the status bar).
* Alternate the status bar display between the time and the SOC%
* Display time for first 5sec following power up.

Time zone strings can be found [here](https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv)

**Note:** The ESP32 RTC has some drift in light sleep (measured at 1min per hour on ESP32-S3 based hardware).  If light sleep is regularly used then some error is to be expected.  An NTP request is performed on every deep sleep wake so if light sleep is disabled, and a short timeout is used, then it is possible to spam the NTP server.

### FTP Settings
<div align="center">
  <img src="images/FtpMenu.png">
</div

The OMOTE includes a basic FTP server that allows the contents of the SPIFFS filesystem to be remotely accessed.  This menu allows the server to be enabled or disabled and configured.  

To connect either the OMOTE's IP address, which is shown at the top of the screen, or the mDNS name can be used (when the FTP server is enabled mDNS is also enabled).  The default mDNS name is 'omote' which can be connected to using omote.local on some operating systems (linux, Mac, iOS).  

The server uses the default FTP port of 21.

Both the default FTP user name and password are set to OMOTE.  It is recommended that these are changed before enabling the server.

Once set up the FTP server provides and easy way of transferring or editing scene, page and command files or configuration data.  It is worth noting that some file changes will take effect within the OMOTE following a wake from deep sleep.

**Note:** The FTP server only supports a single connection.  Any FTP client that allows this to be configured can be used.  For example on FileZilla go to 'Transfer Settings' and tick 'Limit number of simultaneous connections' and set the 'Maximum number of connections' to 1. The FTP client within the Linux Nautilis file manager does not and so can not be used.

**Note:** The FTP server uses the ESP32's real time clock for file time stamping.  If NTP isn't enabled the dates on all files will be 1st January 1970.

**Note:** The FTP server does NOT support encryption, names and passwords are transferred in plain text.

### Logging Settings
<div align="center">
  <img src="images/LoggingMenu.png">
</div

It is possible to select what logging output is sent by the OMOTE.  This is sent out the virtual serial port on the USB connection at 115200baud (can be viewed using any terminal emulator or in the Monitor within the PlatformIO IDE).

The logging level for each peripheral can be set to varying levels between None to Critical.  Only log entries with a severity greater than or equal to the set level will be output.

### Battery Settings
<div align="center">
  <img src="images/BatteryMenu.png">
</div

**WARNING - Experimental (especially on hardware < Rev5)**

The battery settings allow the calibration mode for the percentage SOC display to be set.  Options are:
* **MAX17048 SOC** - directly use the level reported by the fuelgauge IC (or ESP32 ADC on hardware < Rev4)
* **Charge** - take measurements during a full charge cycle and use these to create a calibration curve.
* **Discharge** (recommended) - take measurements during a full discharge and use these to create a calibration curve.

To use select the appropriate option and in the case of Charge or Discharge perform a Calibration.  The calibration needs to start with either a fully charged (in the case of Discharge mode) or fully discharged battery (in the case of Charge mode).  It will then do either a full charge or full discharge while taking battery measurements every 10min.  Light sleep is used in the Discharge mode to minimise extra load on the charger and in the Charge mode to relax the battery voltage before taking each measurement.  Expect the OMOTE to repeatedly enter and leave light sleep for the duration of the calibration.  

The time taken to do a calibration can be considerable, especially with larger battery packs, the OMOTE can not be used during this time.

The calibration can be cancelled at any time by closing the screen.  No data will be saved if cancelled.

The bottom half of the screen shows either text which comprises of instructions for performing the test and measurement results or a graphical display.  While calibrating the graph will build up to show the calibration curve as each 10min measurement takes place.  The graph window will also show the calibration curve (if present) when the mode is changed.  If Charge or Discharge is selected when no calibration data is available the OMOTE will revert to using Direct mode.

### IR Receiver Screen
<div align="center">
  <img src="images/IrMenu.png">
</div

While not a settings menu the IR Receiver screen is also available under settings.  It allows the OMOTE to receive, and decode, IR commands sent by other remotes.  This is typically used to learn IR commands which can then be used by the OMOTE.  To use click "ENABLE RX" to power up the IR receiver.  Any received IR commands should then be displayed in the lower half of the screen.  At the moment it is necessary to manually copy these commands from the OMOTE screen, or the serial log output, to either the firmware code or the JSON command file.

This screen also has some hardware key mappings:
* **Aux1 (Red)** - send Samsung36 0x400E00FF code.
* **Aux2 (Green)** - Enable IR receiver.
* **Aux3 (Yellow)** - Disable IR Receiver.
* **Aux4 (Blue) or Center/OK** - Run IR library timing calibration.

The calibration measures the timing value needed by the ESP32 to provide perfect IR timing.  This can vary slightly between processor variants and compiler optimisations.  If the value reported on the serial log output does not match the default (-2 for the ESP32) then the code should be modified to change it (uncomment the calibrateTx(); line in [IRTransceiver.cpp](https://github.com/OMOTE-Community/OMOTE-Firmware-object-oriented/blob/dev/Platformio/lib/esp32HalImpl/ir/IRTransceiver.cpp), Note: this will slow boot times by 65ms);