#include "DeviceList.hpp"

#include "HardwareFactory.hpp"
#include "HomeAssistDevices/Light.hpp"
#include "List.hpp"
#include "UIElementIds.hpp"
#include "WebSocket/Request.hpp"
#include "WebSocket/RequestBuilder.hpp"
#include "WebSocket/Session/Session.hpp"

namespace UI::Page {

DeviceList::DeviceList(HomeAssist::WebSocket::Api& aApi,
                       ActiveDevices& aActiveDevices)
    : Base(ID::Pages::HomeAssistDeviceList),
      mDeviceList(AddNewElement<Widget::List>()),
      mLoadingArc(AddNewElement<Widget::Arc>()),
      mApi(aApi),
      mActiveDevices(aActiveDevices),
      mDeviceQueryProcessor(std::make_shared<UI::DevicesQueryProcessor>(
          [this](const auto& aEntity) { AddEntity(aEntity); })) {
  // Initially hide device list
  mDeviceList->SetVisiblity(false);

  mLoadingArc->SetRange(0, 100);
  mLoadingArc->SetWidth(GetContentWidth() / 2);
  mLoadingArc->SetHeight(GetContentHeight() / 4);
  mLoadingArc->AlignTo(this, LV_ALIGN_TOP_MID);
  mDeviceList->AlignTo(mDeviceList, LV_ALIGN_OUT_BOTTOM_MID);

  mDeviceQueryProcessor->setRequestProcessCompleteCallback([this](
                                                               auto aResult) {
    bool ranSuccessfully = aResult;
    LvglResourceManager::GetInstance().QueueForLater([this, ranSuccessfully]() {
      if (ranSuccessfully) {
        mLoadingArc->SetVisiblity(false);
        mDeviceList->AlignTo(this, LV_ALIGN_TOP_MID);
        mDeviceList->SetVisiblity(true);
      } else {
        // Todo throw an error up?
      }
    });
  });

  mDeviceQueryProcessor->setPercentCompleteCallback(
      [this](const auto& aPercentComplete) {
        LvglResourceManager::GetInstance().QueueForLater(
            [this, aPercentComplete]() {
              mLoadingArc->SetValue(aPercentComplete);
            });
      });

  {
    using namespace HomeAssist::WebSocket;
    auto request =
        RequestBuilder()
            .SetType(RequestTypes::CONFIG_ENTITY_REGISTRY_LIST_DISPLAY)
            .BuildUnique();

    mApi.AddSession(std::make_unique<Session>(std::move(request), nullptr,
                                              nullptr, mDeviceQueryProcessor));
  }
}

bool DeviceList::OnKeyEvent(KeyPressAbstract::KeyEvent aKeyEvent) {
  return false;
}

void DeviceList::AddEntity(const std::string& aEntity) {
  std::string entityDeepCopy = aEntity;
  LvglResourceManager::GetInstance().AttemptNow([this, entityDeepCopy]() {
    constexpr auto lightStr = "light";
    // MatthewColvin/OMOTE#19 may still need a max here but should probably
    // break device list into 2 parts 1 for the device type and other for the
    // actual entities
    // Maybe the device list can hold a map<devictype, entitiyids>()?
    // Might be a pretty big structure depening on HA instance?
    if (entityDeepCopy.rfind(lightStr, 0) == 0 && mDevicesAdded < 10) {
      mDeviceList->AddItem(
          entityDeepCopy, LV_SYMBOL_OK, [this, entityDeepCopy]() {
            auto light = std::make_shared<HomeAssist::Device::Light>(
                entityDeepCopy, mApi);
            mActiveDevices.addDevice(light);
          });
      mDevicesAdded++;
    }
  });
}

}  // namespace UI::Page
