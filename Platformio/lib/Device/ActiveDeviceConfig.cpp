#include "ActiveDeviceConfig.hpp"
#include "DeviceFactory.hpp"
#include "HardwareFactory.hpp"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include <fstream>
#include <string>

ActiveDeviceConfig::ActiveDeviceConfig(DeviceFactory &factory)
    : mFactory(factory),
      mSaveOnChangeHandler(mFactory.getActiveDevices().getListUpdateNotification()) {
  mSaveOnChangeHandler = [this](auto) {
    auto devices = mFactory.getActiveDevices().getDevices();
    saveDevices(devices);
  };
}

bool ActiveDeviceConfig::saveDevices(const std::deque<IDevice::Ptr> &devices) {
  rapidjson::Document doc;
  doc.SetArray();
  auto &allocator = doc.GetAllocator();

  std::vector<rapidjson::Document> configDocs;
  for (const auto &device : devices) {
    rapidjson::Value deviceObj(rapidjson::kObjectType);
    deviceObj.AddMember("type", static_cast<int>(device->GetType()), allocator);
    deviceObj.AddMember("id", static_cast<int>(device->GetId()), allocator);

    rapidjson::Document extraConfig = device->GetExtraConfig();
    if (!extraConfig.IsNull()) {
      configDocs.push_back(std::move(extraConfig));
      deviceObj.AddMember("config", configDocs.back(), allocator);
    }

    doc.PushBack(deviceObj, allocator);
  }
  std::filesystem::path pathToConfig(ACTIVE_DEVICES_CONFIG_FILE);
  const auto writeResult = OMOTE::JSON::WriteDocumentToFile(doc, pathToConfig);

  // TODO OMOTE-Community/OMOTE-Firmware-object-oriented#72
  // HardwareFactory::getAbstract().debugPrint("DeviceSaveJSON:  %s", jsonStr.c_str());

  return writeResult == OMOTE::JSON::DocumentFileWriteResult::Success;
}

std::vector<IDevice::Ptr> ActiveDeviceConfig::loadDevices() {
  std::vector<IDevice::Ptr> devices;

  // #TODO OMOTE-Community/OMOTE-Firmware-object-oriented#72
  // HardwareFactory::getAbstract().debugPrint("Loaded JSON %s", content.c_str());
  auto configPath = std::filesystem::path(ACTIVE_DEVICES_CONFIG_FILE);
  rapidjson::Document doc = OMOTE::JSON::GetDocument(configPath);

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
        rapidjson::Document configDoc;
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

std::shared_ptr<IDevice> ActiveDeviceConfig::createHomeAssistDevice(const rapidjson::Value &aActiveDeviceJsonConfigMember) {
  rapidjson::Document configDoc;
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

std::shared_ptr<IDevice> ActiveDeviceConfig::createJsonDevice(const rapidjson::Value &aActiveDeviceJsonConfigMember) {
  if (!aActiveDeviceJsonConfigMember.HasMember("file_path") ||
      !aActiveDeviceJsonConfigMember["file_path"].IsString()) {
    return nullptr;
  }
  auto filePath = aActiveDeviceJsonConfigMember["file_path"].GetString();
  return mFactory.CreateJsonDevice(filePath);
}
