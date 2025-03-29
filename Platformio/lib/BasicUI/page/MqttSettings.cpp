#include "MqttSettings.hpp"
#include "Keyboard.hpp"
#include "Label.hpp"
#include "List.hpp"
#include "LvglResourceManager.hpp"

using namespace UI;
using namespace UI::Page;

MqttSettings::MqttSettings(std::shared_ptr<wifiHandlerInterface> aWifi)
    : Base(ID::Pages::MqttSettings), mWifi(aWifi),
      mList(AddNewElement<Widget::List>()), mPasswordGetter(nullptr) {

  mList->AddItem("Broker", NULL, [this] {OpenPasswordKeyboard(broker, "broker");});
  mList->AddItem("Port", NULL, [this] {OpenPasswordKeyboard(port, "1883");});
  mList->AddItem("User", NULL, [this] {OpenPasswordKeyboard(user, "user");});
  mList->AddItem("Password", NULL, [this] {OpenPasswordKeyboard(password, "password");});

}

void MqttSettings::OpenPasswordKeyboard(field aField, std::string aText) {
  // We already have a Keyboard don't launch another one.
  if (mPasswordGetter) {
    return;
  }
  auto keyboard = std::make_unique<Widget::Keyboard>(
      [this](auto aField) {
        mPasswordGetter->AnimateOut();
        }, aText);
  keyboard->OnKeyboardAnimatedOut([this] {
    // Keyboard is done animating out remove it and null the ref
    RemoveElement(mPasswordGetter);
    mPasswordGetter = nullptr;
  });
  mPasswordGetter = AddElement(std::move(keyboard));
}

void MqttSettings::SetHeight(lv_coord_t aHeight) {
  Base::SetHeight(aHeight);
  
};
