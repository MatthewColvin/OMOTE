#include "HeatingPage.hpp"
#include "Button.hpp"
#include "HardwareFactory.hpp"
#include "Label.hpp"

using namespace UI::Page;

Heating::Heating(std::shared_ptr<wifiHandlerInterface> aWifi)
    : Base(ID::Pages::Heating), mWifi(aWifi),
      mCurrentTemperature(AddNewElement<Widget::Label>("--")),
      mSetpoint(AddNewElement<Widget::Label>("--")),
      mBoost(AddNewElement<Widget::Button>([this] { Boost(); })),
      mAdvance(AddNewElement<Widget::Button>([this] { Advance(); })),
      mUp(AddNewElement<Widget::Button>([this] { Up(); })),
      mDown(AddNewElement<Widget::Button>([this] { Down(); })) {

  mCurrentTemperature->SetHeight(lv_pct(10));
  mTemperatureId = mCurrentTemperature->RegisterBindTextEvent(20, "Current: %s°C", "--");
  mWifi->mqttBindTextEvent(mTemperatureId, "openHAB/Heating/status/Kitchen", "roomTemperature");

  mSetpoint->SetHeight(lv_pct(10));
  mSetpointId = mSetpoint->RegisterBindTextEvent(25, "Setpoint: %s", "--");
  mWifi->mqttBindTextEvent(mSetpointId, "openHAB/Heating/status/Kitchen", "setpoint");

  for (auto button : {mBoost, mAdvance, mUp, mDown}) {
    button->SetHeight(lv_pct(10));
    button->SetWidth(lv_pct(80));
  }

  mBoost->SetText("Boost (" LV_SYMBOL_LEFT ")");
  mAdvance->SetText("Advance (" LV_SYMBOL_RIGHT ")");
  mUp->SetText("Up (" LV_SYMBOL_UP ")");
  mDown->SetText("Down (" LV_SYMBOL_DOWN ")");

  mCurrentTemperature->AlignTo(this, LV_ALIGN_TOP_MID, 0,
                               distBetweenRxButtons);
  mSetpoint->AlignTo(mCurrentTemperature, LV_ALIGN_OUT_BOTTOM_MID, 0,
                     distBetweenRxButtons);
  mBoost->AlignTo(mSetpoint, LV_ALIGN_OUT_BOTTOM_MID, 0,
                  distBetweenRxButtons);
  mAdvance->AlignTo(mBoost, LV_ALIGN_OUT_BOTTOM_MID, 0,
                    distBetweenRxButtons);
  mUp->AlignTo(mAdvance, LV_ALIGN_OUT_BOTTOM_MID, 0, distBetweenRxButtons);
  mDown->AlignTo(mUp, LV_ALIGN_OUT_BOTTOM_MID, 0, distBetweenRxButtons);
}

Heating::~Heating() {
  mWifi->mqttUnBindTextEvent(mTemperatureId);
  mWifi->mqttUnBindTextEvent(mSetpointId);
}

void Heating::Boost() {
  mWifi->mqttSend("openHAB/Heating/set/Kitchen", "{\"button\":\"Boost\"}");
  // mWifi->mqttUnBindTextEvent(mTemperatureId); //include to test unsubscribe
  // mWifi->mqttUnBindTextEvent(mSetpointId);
}

void Heating::Advance() {
  mWifi->mqttSend("openHAB/Heating/set/Kitchen", "{\"button\":\"Advance\"}");
}

void Heating::Up() {
  mWifi->mqttSend("openHAB/Heating/set/Kitchen", "{\"button\":\"Up\"}");
}

void Heating::Down() {
  mWifi->mqttSend("openHAB/Heating/set/Kitchen", "{\"button\":\"Down\"}");
}

bool Heating::OnKeyEvent(KeyPressAbstract::KeyEvent aKeyEvent) {
  using ty = KeyPressAbstract::KeyEvent::Type;
  using id = KeyPressAbstract::KeyId;
  if (aKeyEvent.mType == ty::Press) {
    switch (aKeyEvent.mId) {
    case id::Up:
      mWifi->mqttSend("openHAB/Heating/set/Kitchen", "{\"button\":\"Up\"}");
      break;
    case id::Down:
      mWifi->mqttSend("openHAB/Heating/set/Kitchen", "{\"button\":\"Down\"}");
      break;
    case id::Left:
      mWifi->mqttSend("openHAB/Heating/set/Kitchen", "{\"button\":\"Boost\"}");
      break;
    case id::Right:
      mWifi->mqttSend("openHAB/Heating/set/Kitchen", "{\"button\":\"Advance\"}");
      break;
    default:
      break;
    }
    return true;
  }
  return true;
}