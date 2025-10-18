#include "ActiveDeviceConfig.hpp"
#include "DeviceFactory.hpp"
#include "HardwareFactory.hpp"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include <fstream>

ActiveDeviceConfig::ActiveDeviceConfig(DeviceFactory &factory)
    : mFactory(factory),
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

  std::vector<MemConsciousDocument> configDocs;
  for (const auto &device : devices) {
    MemConciousValue deviceObj(rapidjson::kObjectType);
    deviceObj.AddMember("type", static_cast<int>(device->GetType()), allocator);
    deviceObj.AddMember("id", static_cast<int>(device->GetId()), allocator);

    MemConsciousDocument extraConfig = device->GetExtraConfig();
    if (!extraConfig.IsNull()) {
      configDocs.push_back(std::move(extraConfig));
      deviceObj.AddMember("config", configDocs.back(), allocator);
    }

    doc.PushBack(deviceObj, allocator);
  }

  std::string jsonStr = ToString(doc);
  std::ofstream file(FS_PATH + std::string(ACTIVE_DEVICES_CONFIG_FILE), std::ios::out | std::ios::trunc);
  if (!file)
    return false;

  HardwareFactory::getAbstract().debugPrint("DeviceSaveJSON:  %s", jsonStr.c_str());

  file << jsonStr;

  file.close();

  return true;
}

std::vector<IDevice::Ptr> ActiveDeviceConfig::loadDevices() {
  std::vector<IDevice::Ptr> devices;

  std::ifstream file(FS_PATH + std::string(ACTIVE_DEVICES_CONFIG_FILE), std::ios::in);
  if (!file)
    return devices;

  // Todo consider max read size api
  std::stringstream buffer;
  buffer << file.rdbuf();
  file.close();
  std::string content(buffer.str());
  HardwareFactory::getAbstract().debugPrint("Loaded JSON %s", content.c_str());

  if (content.empty())
    return devices;

  MemConsciousDocument doc;
  doc.Parse(content.c_str());

  if (!doc.IsArray())
    return devices;

  HardwareFactory::getAbstract().debugPrint("Restoring Devices");

  for (const auto &deviceObj : doc.GetArray()) {
    DeviceType type = static_cast<DeviceType>(deviceObj["type"].GetInt());
    DeviceId id = static_cast<DeviceId>(deviceObj["id"].GetInt());

    HardwareFactory::getAbstract().debugPrint("Type:%d id:%d", type, id);

    std::shared_ptr<IDevice> device;
    if (type == DeviceType::CompileTime) {
      device = mFactory.Create(id);
    }
    if (deviceObj.HasMember("config")) {
      auto &configObj = deviceObj["config"];
      HardwareFactory::getAbstract().debugPrint("Found Config");
      switch (type) {
      case DeviceType::HomeAssist:
        device = createHomeAssistDevice(configObj);
        break;
      case DeviceType::JSON:
        device = createJsonDevice(configObj);
        break;
      default:
        break;
      }

      if (device) {
        MemConsciousDocument configDoc;
        configDoc.CopyFrom(configObj, configDoc.GetAllocator());
        device->SetExtraConfig(configDoc);
      }
    }

    if (device) {
      devices.push_back(std::move(device));
    }
  }

  return devices;
}

std::shared_ptr<IDevice> ActiveDeviceConfig::createHomeAssistDevice(const MemConciousValue &aActiveDeviceJsonConfigMember) {
  MemConsciousDocument configDoc;
  configDoc.CopyFrom(aActiveDeviceJsonConfigMember, configDoc.GetAllocator());
  // Check Config for entityId to build HA Device
  if (configDoc.HasMember("entityId") && configDoc["entityId"].IsString()) {
    HardwareFactory::getAbstract().debugPrint("found");
    if (std::string entityIdFromConfig = configDoc["entityId"].GetString(); !entityIdFromConfig.empty()) {
      return mFactory.CreateHomeAssistDevice(entityIdFromConfig);
    }
  }
  return nullptr;
}

std::shared_ptr<IDevice> ActiveDeviceConfig::createJsonDevice(const MemConciousValue &aActiveDeviceJsonConfigMember) {
  if (!aActiveDeviceJsonConfigMember.HasMember("file_path") ||
      !aActiveDeviceJsonConfigMember["file_path"].IsString()) {
    return nullptr;
  }
  auto filePath = aActiveDeviceJsonConfigMember["file_path"].GetString();
  return mFactory.CreateJsonDevice(filePath);
}
