# Devices 

# Introduction

1) `Devices` are a concept that were intended to represent a physical device such as a TV or Speaker. 
2) `ActiveDevices` is basically a priority list of devices that can be interacted with. (typically via key events). 
   1) The `ActiveDeviceConfig` class is responsible for saving and restoring the order of these devices.


# Defining a Device via JSON

1) The device json works in conjunction with the ActionsFactory to help define what the device does see the JsonDevices Roku.json file to see an example of creating a device that can control a Roku via IR. 