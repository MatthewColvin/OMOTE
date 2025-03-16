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
  DeviceList(HomeAssist::WebSocket::Api& aApi, ActiveDevices& aActiveDevices);
  virtual ~DeviceList() = default;

  // Override from Base
  std::string GetTitle() override { return "Devices"; }

 protected:
  bool OnKeyEvent(KeyPressAbstract::KeyEvent aKeyEvent) override;

  void AddEntity(const std::string& aEntity);

 private:
  Widget::List* mDeviceList;
  Widget::Arc* mLoadingArc;
  HomeAssist::WebSocket::Api& mApi;
  ActiveDevices& mActiveDevices;
  std::shared_ptr<UI::DevicesQueryProcessor> mDeviceQueryProcessor;
  int mDevicesAdded = 0;
};

}  // namespace UI::Page
