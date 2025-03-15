#include "HomeAssistDevices/Light.hpp"

#include "WebSocket/Message/Entity.hpp"
#include "WebSocket/Message/MessageHandler.hpp"
#include "WebSocket/RequestBuilder.hpp"
#include "WebSocket/Session/Session.hpp"

namespace HomeAssist::Device {

Light::Light(const std::string& entityId, std::shared_ptr<WebSocket::Api> api)
    : mEntityId(entityId), mApi(api) {
  SetupStateSubscription();
}

bool Light::HandleKeyEvent(KeyPressAbstract::KeyEvent event) { return false; }

std::unique_ptr<UI::Page::Base> Light::GetControlPage() {
  // TODO: Implement control page
  return nullptr;
}

void Light::TurnOn() { SendLightCommand("turn_on"); }

void Light::TurnOff() { SendLightCommand("turn_off"); }

void Light::Toggle() { SendLightCommand("toggle"); }

void Light::SetupStateSubscription() {
  if (!mApi) return;

  auto request = WebSocket::RequestBuilder::CreateTriggerSubscription(
      mEntityId,
      "",  // from any state
      ""   // to any state
  );

  auto messageHandler = std::make_shared<WebSocket::MessageHandler>(
      [this](const WebSocket::Message& message) {
        HandleStateChange(message);
        return true;
      });

  auto session = std::make_unique<WebSocket::Session>(std::move(request),
                                                      nullptr, messageHandler);

  mApi->AddSession(std::move(session));
}

void Light::HandleStateChange(const WebSocket::Message& message) {
  if (auto* state = message.BorrowToState(); state != nullptr) {
    mIsOn = state->GetState() == "on";
  }
}

void Light::SendLightCommand(const std::string& service) {
  if (!mApi) return;

  auto request = WebSocket::RequestBuilder()
                     .SetType(WebSocket::RequestTypes::CALL_SERVICE)
                     .SetId(0)  // ID will be set by Api
                     .AddField("domain", "light")
                     .AddField("service", service)
                     .AddField("target", mEntityId)
                     .BuildUnique();

  auto session = std::make_unique<WebSocket::Session>(std::move(request));
  mApi->AddSession(std::move(session));
}

}  // namespace HomeAssist::Device
