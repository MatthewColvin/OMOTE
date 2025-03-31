#include "HomeAssistUI.hpp"

#include "Button.hpp"
#include "DeviceList.hpp"
#include "HardwareFactory.hpp"
#include "ScreenManager.hpp"
#include "WebSocket/Message/Attributes/Light.hpp"
#include "WebSocket/Message/Entity.hpp"
#include "WebSocket/Message/Message.hpp"
#include "WebSocket/Message/MessageHandler.hpp"
#include "WebSocket/Session/Session.hpp"

using namespace UI;

using namespace HomeAssist::WebSocket;

HomeAssistUI::HomeAssistUI() : BasicUI() {
  auto &hardware = HardwareFactory::getAbstract();
  auto socket = hardware.webSocket();
  if (!socket) {
    hardware.debugPrint("Unable To Get WebSocket Total Failure Condition!");
    return;
  }
  mHomeAssistApi = std::make_unique<Api>(socket);
  mConnectionStatusHandler.SetNotification(
      mHomeAssistApi->GetConnectionNotification());
  mConnectionStatusHandler = [this](auto aStatus) {
    HandleConnectionStatusChange(aStatus);
  };

  auto tmpAddDeviceButton = std::make_unique<UI::Widget::Button>([this]() {
    auto deviceList =
        std::make_unique<UI::Page::DeviceList>(*mHomeAssistApi, mDeviceFactory.getActiveDevices());
    UI::Screen::Manager::getInstance().pushPopUp(std::move(deviceList));
  });
  tmpAddDeviceButton->SetWidth(SCREEN_WIDTH);
  tmpAddDeviceButton->SetHeight(SCREEN_HEIGHT / 8);
  tmpAddDeviceButton->AddNewElement<UI::Widget::Label>("Add HomeAssist Device");
  auto page = std::make_unique<UI::Page::Base>(ID::Pages::INVALID_PAGE_ID);
  page->AddElement(std::move(tmpAddDeviceButton));

  AddPageToHomeScreen(std::move(page));
};

void HomeAssistUI::HandleConnectionStatusChange(
    Api::ConnectionStatus aNewStatus) {
  if (aNewStatus == Api::ConnectionStatus::Disconnected) {
    // MatthewColvin/OMOTE#7
  }
}

void HomeAssistUI::loopHandler() {
  BasicUI::loopHandler();
  if (mHomeAssistApi) {
    mHomeAssistApi->Process();
  }
}
