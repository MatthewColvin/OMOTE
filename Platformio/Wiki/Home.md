Welcome to the Object Oriented code that drives OMOTE!  
We have plenty of features and are very open to contributions! 

# End User Features:
* Defining UI in JSON including Scenes
* Defining Devices in JOSN to map button behavior globally 
* Defining UI in JSON that allows control and view of MQTT devices 
* Interacting with Home assist devices (Basic light control)

# Development Features:
* Rich logging framework with configurable levels that can by dynamically set and persisted 
* Extensible UI for easy custom building 
* Home Assist Web Socket framework waiting for device integration for features. 
* LVGL9 support 
* IDF 5 / Arduino Core V3 support 
* Filesystem support for easy persistence 

# Getting Started

Three main architectures are supported for the development of OMOTE applications:

1. Direct coding in C++ ([Basic UI](./BasicUI.md "Create UI directly from c++ code"))
2. Home Assistant Web Socket Framework (TBA)
3. Configuration via JSON files ([JSON configuration](./JsonConfig.md "OMOTE GUI and command configuration via JSON files"))

There is also a status bar along the top of the screen that displays current OMOTE status and allows access to the OMOTE settings menu ([Status Bar](./StatusBar.md "OMOTE Status Bar and Setup Menu"))

Note: Most testing to date has been on Rev1 and Rev5_3661 hardware.  To use on other hardware revisions some firmware changes will likely be required.  In particular a PlatformIO build will need to be created, source code changes may also be required.  Assistance with this process should be available on the OMOTE discord.
