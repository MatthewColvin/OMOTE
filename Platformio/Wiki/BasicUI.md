## Basic UI Configuration
The Basic UI is used when you would like to program the user interface directly in C++.

To enable it uncomment this [line](https://github.com/OMOTE-Community/OMOTE-Firmware-object-oriented/blob/a8268516d815e16a7e3459c7bb56e2b30a0ab436/Platformio/src/OmoteSetup.hpp#L13) in the createUI() function in the file OmoteSetup.hpp.

```c_cpp
ui = std::make_unique<UI::BasicUI>();
```
	
The neighbouring JsonUI and HomeAssistUI lines should be commented out.

The [HomeScreen class](https://github.com/OMOTE-Community/OMOTE-Firmware-object-oriented/blob/dev/Platformio/lib/BasicUI/screen/HomeScreen.cpp) is then responsible for creation of the main OMOTE screen. The standard file loads three demo pages into a single Tab View based page, these are:

* **IRLearner** - (commented out in github repo) demonstrates how to add and configure various screen widgets and how to interface to the physical keys.  This is the same screen used within the OMOTE setup menus.
* **Demo** - This demonstrates how it is possible to dynamically add and remove widgets to a screen programmatically.  The Up button adds a slider widget/s while the Down button removes them.
* **Heating** - This demonstrates how to interface buttons and labels to the MQTT interface.

All the screens from the settings menus are also good examples of how different widgets can be used.

## Simulation
The PlatformIO project includes a x64_sim environment which allows most OMOTE functionality to be simulated on a PC with vastly reduced compile times and full source level debug.  Its use is strongly recommended for BasicUI development.  Physical key presses are mapped to the PC keyboard and the mappings can be found in the [KeyPressSim.hpp](https://github.com/OMOTE-Community/OMOTE-Firmware-object-oriented/blob/dev/Platformio/lib/SimulatorHalImpl/KeyPressSim.hpp) file.

On Windows use `pio run -e x64_sim_windows` then run `.pio/build/x64_sim_windows/program.exe` from the `Platformio` folder (SDL2 on PATH).

- **Config editor (sim):** Connect tab → **`http://127.0.0.1:9080`** (same API as hardware on port 80).
- **WiFi setup (sim only):** if `sim_data/wifi_settings.json` is missing, **http://127.0.0.1:8080** — enter any SSID/password (not a URL); saves to `wifi_settings.json`.
- Run the editor itself on another port (e.g. `python -m http.server 8765`) so it does not clash with 8080/9080.