#pragma once
#include "ActiveDevices.hpp"
#include "Arc.hpp"
#include "PageBase.hpp"
#include "SessionProcessors/DevicesQueryProcessor.hpp"
#include "WebSocket/Api.hpp"
#include "WebSocket/Message/MessageHandler.hpp"

namespace Json {
class IChunkProcessor;
}

namespace UI::Widget {
class List;
}

namespace UI::Page {

class DeviceList : public Base {
public:
  enum class EntityType : uint8_t {
    Light,
    Button,
    Sensor,
    BinarySensor,
    Automation,
    Switch,
    Other
  };

  DeviceList(HomeAssist::WebSocket::Api &aApi, ActiveDevices &aActiveDevices);
  virtual ~DeviceList() = default;

  // Override from Base
  std::string GetTitle() override { return "Devices"; }

protected:
  bool OnKeyEvent(KeyPressAbstract::KeyEvent aKeyEvent) override;

private:
  // Save Entity in the Entity map to build UI later
  void StoreEntity(const std::string &aEntity);
  // Handle pushing sub list when device list is processed
  void HandleDevicesQueryComplete(
      const UI::DevicesQueryProcessor::resultType &aResult);

  Widget::List *mEntityTypeList;
  Widget::Arc *mLoadingArc;
  HomeAssist::WebSocket::Api &mApi;
  ActiveDevices &mActiveDevices;

  std::shared_ptr<UI::DevicesQueryProcessor> mDeviceQueryProcessor;
  std::map<EntityType, std::vector<std::string>> mEntityMap;
  void AddEntityTypeListItem(EntityType aEntityType,
                             const std::vector<std::string> &aEntities) const;

  int mDevicesAdded = 0;
};

} // namespace UI::Page
