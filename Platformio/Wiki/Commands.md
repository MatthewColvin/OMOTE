## Commands
Command files contain the actual IR or MQTT commands that can be mapped to physical keys or widgets.  A typical file might look like this:

```json
{
  "Manufacturer":"Roku",
  "DeviceClass":"Streaming",
  "Commands": [
    {"Command":"LEFT",      "Mode":"IR", "Protocol":"NEC", "Data":["0x57437887"]},
    {"Command":"RIGHT",     "Mode":"IR", "Protocol":"NEC", "Data":["0x5743B44B"]},
    {"Command":"UP",        "Mode":"IR", "Protocol":"NEC", "Data":["0x57439867"]},
    {"Command":"DOWN",      "Mode":"IR", "Protocol":"NEC", "Data":["0x5743CC33"]},
    {"Command":"SELECT",    "Mode":"IR", "Protocol":"NEC", "Data":["0x574354AB"]},
    {"Command":"HOME",      "Mode":"IR", "Protocol":"NEC", "Data":["0x5743C03F"]},
    {"Command":"BACK",      "Mode":"IR", "Protocol":"NEC", "Data":["0x57436699"]},
    {"Command":"INFO",      "Mode":"IR", "Protocol":"NEC", "Data":["0x57438679"]},
    {"Command":"MENU",      "Mode":"IR", "Protocol":"NEC", "Data":["0x5743C03F"]},
    {"Command":"PLAY_PAUSE","Mode":"IR", "Protocol":"NEC", "Data":["0x574332CD"]},
    {"Command":"FORWARD",   "Mode":"IR", "Protocol":"NEC", "Data":["0x5743AA55"]},
    {"Command":"REVERSE",   "Mode":"IR", "Protocol":"NEC", "Data":["0x57432CD3"]},
    {"Command":"STORE",     "Mode":"IR", "Protocol":"NEC", "Data":["0x5743E817"]},
    {"Command":"NETFLIX",   "Mode":"IR", "Protocol":"NEC", "Data":["0x5743D22D"]},
    {"Command":"PRIME",     "Mode":"IR", "Protocol":"NEC", "Data":["0x574308F7"]},
    {"Command":"YOUTUBE",   "Mode":"IR", "Protocol":"NEC", "Data":["0x574342BD"]}
  ]
}
```

The top level fields are:
* **Manufacturer** Not currently used
* **DeviceClass** Not currently used
* **Commands** Array of the commands that can be used

The fields for each command vary depending on the mode.

### IR:

```json
{"Command":"LEFT", "Mode":"IR", "Protocol":"NEC", "Data":["0x57437887"]},
```
	
* **Command** - is the name that is to be used to refer to this command in the Page JSON files.  Devices in the same class (for example TVs) should try to use the same names so that they can be swapped easily (i.e. the names for standard commands in a Panasonic TV command file should be the same as those in a LG TV command file)
* **Mode** - IR for the Infra Red protocols
* **Protocol** - IR protocol to be used, see [protocols](https://github.com/OMOTE-Community/OMOTE-Firmware-object-oriented/blob/a8268516d815e16a7e3459c7bb56e2b30a0ab436/Platformio/lib/HAL/Hardware/IRInterface.h#L9-L135)
* **Data** - Actual data to send, depends on protocol, simplest way to find the protocol and data is to use the OMOTE IR receiver to learn from he original remote.

### MQTT:
Commands are used to both send and receive MQTT data.

To Send:

```json
{"Command":"LOUNGE_ADVANCE", "Mode":"MQTT", "Protocol":"PUB", "Data":["openHAB/Heating/set/Lounge", "{\"button\":\"Advance\"}"]},
```
	
* **Mode** - MQTT for the Infra Red protocols
* **Protocol** - Pub to publish a MQTT message
* **Data** - Contains two items, the topic to publish to and the payload to publish.  Currently only fixed payload messages are supported.

To Receive:

```json
{"Command":"LOUNGE_CURRENT", "Mode":"MQTT", "Protocol":"SUB", "Data":["openHAB/Heating/status/Lounge", "roomTemperature", "Current: %s°C", "20"]},
```
	
* **Mode** - MQTT for the Infra Red protocols
* **Protocol** - Sub to subscribe to an MQTT message
* **Data** - array of 4 items:
* **Data (1st element)** - the topic to subscribe to
* **Data (2nd element)** - the JSON field within the payload to extract.
* **Data (3rd element)** - printf like formatting string to be used to format the data.
* **Data (4th element)** - maximum size of displayed string (used to allocate internal buffers)

### DELAY
There is also a DELAY command that can be used within sequences to give millisecond delays.  The format is as follows and is self explanatory:

```json
{"Command":"DELAY_PWR_ON","Mode":"IR", "Protocol":"DELAY","Data":["500"]}
```
