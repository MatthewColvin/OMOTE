#include "MqttSettings.hpp"
#include "HardwareFactory.hpp"
#include "Keyboard.hpp"
#include "Label.hpp"
#include "Switch.hpp"
#include "List.hpp"
#include "LvglResourceManager.hpp"

using namespace UI;
using namespace UI::Page;

MqttSettings::MqttSettings(std::shared_ptr<wifiHandlerInterface> aWifi)
    : Base(ID::Pages::MqttSettings), mWifi(aWifi),
      mEnLabel(AddNewElement<Widget::Label>("Enable")),
      mEnSwitch(AddNewElement<Widget::Switch>([this](auto aNewState) {
        HardwareFactory::getAbstract().wifi()->enableMqtt(aNewState);}, 
        HardwareFactory::getAbstract().wifi()->isMqttEnabled())),
      mList(AddNewElement<Widget::List>()), mPasswordGetter(nullptr),
      mButton(AddNewElement<Widget::Button>([this] { Reconnect(); })) {

  mEnLabel->SetSize(lv_pct(80), 15);
  mEnLabel->AlignTo(this, LV_ALIGN_TOP_LEFT, 0, 15);

  mEnSwitch->SetSize(lv_pct(20), 15);
  mEnSwitch->AlignTo(mEnLabel, LV_ALIGN_OUT_RIGHT_MID);

  mList->AddItem("Broker", NULL, [this] { OpenPasswordKeyboard(broker, mWifi->mqttGetBroker()); });
  mList->AddItem("Port", NULL, [this] { OpenPasswordKeyboard(port, mWifi->mqttGetPort()); });
  mList->AddItem("User", NULL, [this] { OpenPasswordKeyboard(user, mWifi->mqttGetUser()); });
  mList->AddItem("Password", NULL, [this] { OpenPasswordKeyboard(password, mWifi->mqttGetPassword()); });
  mList->SetHeight(lv_pct(50));
  mList->AlignTo(mEnLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 25);
  mButton->SetText("Connect/Save");
  mButton->SetHeight(lv_pct(10));
  mButton->SetWidth(lv_pct(80));
  mButton->AlignTo(mList, LV_ALIGN_OUT_BOTTOM_MID);
}

void MqttSettings::OpenPasswordKeyboard(field aField, std::string aText) {
  // We already have a Keyboard don't launch another one.
  if (mPasswordGetter) {
    return;
  }
  auto keyboard = std::make_unique<Widget::Keyboard>(
      [this, aField](auto aEnteredText) {
        if (aEnteredText != "") {
          switch (aField) {
          case broker:
            mWifi->mqttSetBroker(aEnteredText);
            break;
          case port:
            mWifi->mqttSetPort(aEnteredText);
            break;
          case user:
            mWifi->mqttSetUser(aEnteredText);
            break;
          case password:
            mWifi->mqttSetPassword(aEnteredText);
            break;
          default:
            break;
          }
        }
        mPasswordGetter->AnimateOut();
      },
      aText);
  keyboard->OnKeyboardAnimatedOut([this] {
    // Keyboard is done animating out remove it and null the ref
    RemoveElement(mPasswordGetter);
    mPasswordGetter = nullptr;
  });
  mPasswordGetter = AddElement(std::move(keyboard));

  // HardwareFactory::getAbstract().wifi()->mqttSend("Test", "Test2");
}

void MqttSettings::SetHeight(lv_coord_t aHeight) {
  Base::SetHeight(aHeight);
};

void MqttSettings::Reconnect() {
  mWifi->mqttSaveCredentialsOnConnect(); // mark to persist if connects
  mWifi->setupMqttBroker();
}
