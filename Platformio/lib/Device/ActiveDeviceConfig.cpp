#include "ActiveDeviceConfig.hpp"
#include "DeviceFactory.hpp"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

ActiveDeviceConfig::ActiveDeviceConfig(std::shared_ptr<LittleFsInterface> fs, DeviceFactory &factory)
    : mFs(fs), mFactory(factory) {}

bool ActiveDeviceConfig::saveDevices(const std::vector<IDevice::Ptr> &devices) {
  MemConsciousDocument doc;
  doc.SetArray();
  auto &allocator = doc.GetAllocator();

  for (const auto &device : devices) {
    MemConciousValue deviceObj(rapidjson::kObjectType);
    deviceObj.AddMember("type", static_cast<int>(device->GetType()), allocator);
    deviceObj.AddMember("id", static_cast<int>(device->GetId()), allocator);

    MemConsciousDocument extraConfig = device->GetExtraConfig();
    if (!extraConfig.IsNull()) {
      deviceObj.AddMember("config", extraConfig, allocator);
    }

    doc.PushBack(deviceObj, allocator);
  }

  std::string jsonStr = ToString(doc);
  auto file = mFs->open(ACTIVE_DEVICES_CONFIG_FILE, LFS_O_WRONLY | LFS_O_CREAT);
  if (!file)
    return false;

  file.write(jsonStr);
  return file;
}

std::vector<IDevice::Ptr> ActiveDeviceConfig::loadDevices() {
  std::vector<IDevice::Ptr> devices;

  auto file = mFs->open(ACTIVE_DEVICES_CONFIG_FILE, LFS_O_RDONLY);
  if (!file)
    return devices;

  // Todo consider max read size api
  std::string content = file.read(1000);
  if (content.empty())
    return devices;

  MemConsciousDocument doc;
  doc.Parse(content.c_str());

  if (!doc.IsArray())
    return devices;

  for (const auto &deviceObj : doc.GetArray()) {
    DeviceType type = static_cast<DeviceType>(deviceObj["type"].GetInt());
    DeviceId id = static_cast<DeviceId>(deviceObj["id"].GetInt());

    std::shared_ptr<IDevice> device;
    if (type == DeviceType::CompileTime) {
      device = mFactory.Create(id);
    }
    if (deviceObj.HasMember("config")) {
      MemConsciousDocument configDoc;
      configDoc.CopyFrom(deviceObj["config"], configDoc.GetAllocator());
      // Check Config for entityId to build HA Device
      if (!device && deviceObj.HasMember("entityId") && deviceObj["entityId"].IsString()) {
        if (std::string entityIdFromConfig = deviceObj["entityId"].GetString(); !entityIdFromConfig.empty()) {
          // TODO: Needs HomeAssist API reference
          // device = mFactory.CreateHomeAssistDevice(entityIdFromConfig, haApi);
        }
      }

      if (device) {
        device->SetExtraConfig(configDoc);
      }
    }

    if (device) {
      devices.push_back(std::move(device));
    }
  }

  return devices;
}
