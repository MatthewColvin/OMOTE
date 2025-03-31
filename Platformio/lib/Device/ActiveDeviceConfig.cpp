#include "ActiveDeviceConfig.hpp"
#include "DeviceFactory.hpp"
#include "HardwareFactory.hpp"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

ActiveDeviceConfig::ActiveDeviceConfig(std::shared_ptr<LittleFsInterface> fs, DeviceFactory &factory)
    : mFs(fs),
      mFactory(factory),
      mSaveOnChangeHandler(mFactory.getActiveDevices().getListUpdateNotification()) {
  mSaveOnChangeHandler = [this](auto) {
    auto devices = mFactory.getActiveDevices().getDevices();
    saveDevices(devices);
  };
}

bool ActiveDeviceConfig::saveDevices(const std::deque<IDevice::Ptr> &devices) {
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

  HardwareFactory::getAbstract().debugPrint("DeviceSaveJSON:  %s", jsonStr.c_str());

  file.write(jsonStr);

  file ? file.truncate(jsonStr.length()) : []() { return -1; }();

  return file;
}

std::vector<IDevice::Ptr> ActiveDeviceConfig::loadDevices() {
  std::vector<IDevice::Ptr> devices;

  auto file = mFs->open(ACTIVE_DEVICES_CONFIG_FILE, LFS_O_RDONLY);
  if (!file)
    return devices;

  // Todo consider max read size api
  std::string content = file.read(1000);
  HardwareFactory::getAbstract().debugPrint("Loaded JSON %s", content.c_str());

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
          device = mFactory.CreateHomeAssistDevice(entityIdFromConfig);
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
