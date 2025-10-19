## JSON Configuration
The firmware supports complete confuguration from JSON files stored in a LittleFS filesystem within the ESP32 flash memory.  While it should also be possible to store this on the SD card that feature is yet to be developed.  Similarly browser access to the file system directly from the OMOTE via WiFi is a planned future development.

For the time being the simulator is the quickest route to testing JSON defined UIs.  Any JSON files present in the /data subdirectory will be parsed by the OMOTE simulator and used to generate the UI.  MQTT is also supported in the simulator.  Unfortunately there is no way to test the sending of IR commands in the simulator, they must be tested on the real hardware.

The UI is built using Scenes, Pages and Commands. 

All the JSON files live within the /data subdirectory of the PlatformIO project.  Once created they can be copied to the OMOTE by using the PlatformIO Upload Filesytem Image tool.

**Note:** The JSON parser used by OMOTE is configured to accept files containing comments.  These are not handled well in many editors so if JSON files with comments are used then editing within PlatformIO is recommended as this handles the comment lines correctly.

### Scenes
Scenes are the top level, they serve a number of functions.  They allow a subset of pages to be grouped together.  For example a scene for watching a BluRay disc on a TV might include a page to control the BluRay player, one for the TV, one for an AV amp/sound bar, etc.  The scene for watching from a Roku stick on the TV would replace the BluRay player with a Roku page and so on.

Scenes also allow physical key overrides from the pages they contain.  For example the volume button could be mapped to the soundbar or the play button always mapped to the Roku box for every page within a scene.

Scenes also allow macros to be defined that are used on entry and exit to the scene to configure the environment.  For example the above scene might turn the BluRay, TV and AV amp on when entering the scene and off when exiting.

More details can be found here [Scenes](Scenes.md)

### Pages
Pages normally have a one to one mapping with physical devices to be controlled, for example you might have a page for the TV.  The page defines the LCD GUI contents and what screen button presses do.  It also maps commands to physical buttons on the keypad and defines whether they respond to a Long, Short or Repeated press.

Each page can also reference a single Command file which contains details on how to send commands to the device.  The Commands are held separately to the Page so that a generic TV page could support use with multiple different TVs(Sony, LG, Panasonic, etc) just by changing the command file used.

More details can be found here [Pages](Pages.md)

### Commands
Command files contain the commands that are actually used to control the physical devices.  Currently both IR and MQTT commands are supported.

Although it is recommended to have a command file for each physical device (for example a file might contain the codes for a Yamaha AV amp) to simplify portability there is no reason why different protocols and sub-protocols can't be mixed in a single install specific command file.

More details can be found here [Commands](Commands.md)
