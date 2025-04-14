#include "JsonPage.hpp"
#include "Button.hpp"
#include "HardwareFactory.hpp"
#include "Label.hpp"

using namespace UI::Page;

JsonPage::JsonPage(std::string aFileName, std::string aPageName, std::string aCommandPrefix)
    : Base(ID::Pages::JsonPage) {

  auto title = std::make_unique<Widget::Label>(aPageName);
  title->SetHeight(lv_pct(10));
  title->AlignTo(this, LV_ALIGN_TOP_MID, 0, distBetweenWidgets);
  mWidgets.push_back(AddElement(std::move(title)));

  File fp = HardwareFactory::getAbstract().littleFs()->open(aFileName, LFS_O_RDONLY);
  if (!fp)
    return;
  std::string content = fp.read(10000);

  MemConsciousDocument d;
  d.Parse(content.c_str());

  if (d.HasMember("ActionsFile")) {
    mActionsFile = d["ActionsFile"].GetString();
  }

  if (d.HasMember("Widgets")) {
    for (rapidjson::SizeType i = 0; i < d["Widgets"].Size(); i++) {
      if (d["Widgets"][i].HasMember("Type")) {
        std::string type = d["Widgets"][i]["Type"].GetString();
        if (type == "Label") {
          auto label = std::make_unique<Widget::Label>("");
          if (d["Widgets"][i].HasMember("Text"))
            label->SetText(d["Widgets"][i]["Text"].GetString());
          if (d["Widgets"][i].HasMember("HeightPct"))
            label->SetHeight(lv_pct(d["Widgets"][i]["HeightPct"].GetUint()));
          if (d["Widgets"][i].HasMember("AlignTo")) {
            unsigned int index = d["Widgets"][i]["AlignTo"].GetUint();
            if (index < mWidgets.size())
              label->AlignTo(mWidgets[index], LV_ALIGN_OUT_BOTTOM_MID, 0, distBetweenWidgets);
          }

          if (d["Widgets"][i].HasMember("Action") && !mActionsFile.empty()) {
            std::string action = d["Widgets"][i]["Action"].GetString();
            if (!aCommandPrefix.empty())
              action.insert(0, aCommandPrefix);
            std::vector<std::string> actionStrings;
            if (getAction(mActionsFile, action, actionStrings) == MQTT_SUB)
              if (actionStrings.size() == 4) {
                std::string text("");
                if (d["Widgets"][i].HasMember("Text"))
                  text = d["Widgets"][i]["Text"].GetString();
                // need to ensure that format string does not go out of scope
                mFormatStrings.push_back(std::make_unique<char[]>(actionStrings[2].size() + 1));
                strncpy(mFormatStrings.back().get(), actionStrings[2].c_str(), (actionStrings[2].size() + 1));
                uint32_t id = label->RegisterBindTextEvent(std::stoi(actionStrings[3]), mFormatStrings.back().get(), text.c_str());
                HardwareFactory::getAbstract().wifi()->mqttBindTextEvent(id, actionStrings[0], actionStrings[1]);
                mSubscriptions.push_back(id);
              }
          }

          mWidgets.push_back(AddElement(std::move(label)));
        } else if (type == "Button") {
          if (d["Widgets"][i].HasMember("Action") && !mActionsFile.empty()) {
            std::string action = d["Widgets"][i]["Action"].GetString();
            if (!aCommandPrefix.empty())
              action.insert(0, aCommandPrefix);
            std::vector<std::string> actionStrings;
            auto actionProto = getAction(mActionsFile, action, actionStrings);
            // only process button if we have an action to associate with it
            if ((actionProto == MQTT_PUB) || (actionProto == IR)) {
              std::function<void(void)> cbFunction;
              if (actionProto == MQTT_PUB)
                cbFunction = [this, actionStrings]() { HardwareFactory::getAbstract().wifi()->mqttSend(actionStrings[0].c_str(), actionStrings[1].c_str()); };
              else
                cbFunction = [this, actionStrings]() { HardwareFactory::getAbstract().ir()->send(IRInterface::constInt64SendTypes::Panasonic64, std::stoul(actionStrings[1])); };
              auto button = std::make_unique<Widget::Button>(cbFunction);
              if (d["Widgets"][i].HasMember("Text"))
                button->SetText(d["Widgets"][i]["Text"].GetString());
              if (d["Widgets"][i].HasMember("HeightPct"))
                button->SetHeight(lv_pct(d["Widgets"][i]["HeightPct"].GetUint()));
              if (d["Widgets"][i].HasMember("SizeXY"))
                if (d["Widgets"][i]["SizeXY"].Size() == 2)
                  button->SetSize(lv_pct(d["Widgets"][i]["SizeXY"][0].GetUint()), lv_pct(d["Widgets"][i]["SizeXY"][1].GetUint()));
              if (d["Widgets"][i].HasMember("AlignTo")) {
                unsigned int index = d["Widgets"][i]["AlignTo"].GetUint();
                if (index < mWidgets.size())
                  button->AlignTo(mWidgets[index], LV_ALIGN_OUT_BOTTOM_MID, 0, distBetweenWidgets);
              }
              // set position after align so can adjust
              if (d["Widgets"][i].HasMember("PosX"))
                button->SetX(lv_pct(d["Widgets"][i]["PosX"].GetUint()));
              if (d["Widgets"][i].HasMember("PosY"))
                button->SetY(lv_pct(d["Widgets"][i]["PosY"].GetUint()));
              mWidgets.push_back(AddElement(std::move(button)));
            }
          }
        }
      }
    }
  }
}

JsonPage::~JsonPage() {
  // todo unsubscribe mSubscriptions
}

// todo move into own class as will be used in a few places
ActionProtocols JsonPage::getAction(std::string aActionsFIle, std::string aAction, std::vector<std::string> &actionStrings) {

  File fp = HardwareFactory::getAbstract().littleFs()->open(aActionsFIle, LFS_O_RDONLY);
  if (!fp)
    return NONE;
  std::string content = fp.read(10000);

  MemConsciousDocument d;
  d.Parse(content.c_str());

  std::string protocol;
  if (d.HasMember("Actions")) {
    for (rapidjson::SizeType i = 0; i < d["Actions"].Size(); i++) {
      if (d["Actions"][i].HasMember("Action")) {
        std::string action = d["Actions"][i]["Action"].GetString();
        if (action == aAction) {
          if (d["Actions"][i].HasMember("Protocol")) {
            protocol = d["Actions"][i]["Protocol"].GetString();
            if (d["Actions"][i].HasMember("Command")) {
              for (rapidjson::SizeType j = 0; j < d["Actions"][i]["Command"].Size(); j++) {
                actionStrings.push_back(d["Actions"][i]["Command"][j].GetString());
              }
            }
          }
        }
      }
    }
  }

  ActionProtocols retVal = NONE;
  if (protocol == "MQTT_SUB")
    retVal = MQTT_SUB;
  else if (protocol == "MQTT_PUB")
    retVal = MQTT_PUB;
  else if (protocol == "IR")
    retVal = IR;

  return retVal;
}

bool JsonPage::OnKeyEvent(KeyPressAbstract::KeyEvent aKeyEvent) {
  using ty = KeyPressAbstract::KeyEvent::Type;
  using id = KeyPressAbstract::KeyId;
  if (aKeyEvent.mType == ty::Press) {
    switch (aKeyEvent.mId) {
    case id::Up:
      // mWifi->mqttSend("openHAB/Heating/set/Kitchen", "{\"button\":\"Up\"}");
      break;
    case id::Down:
      // mWifi->mqttSend("openHAB/Heating/set/Kitchen", "{\"button\":\"Down\"}");
      break;
    case id::Left:
      // mWifi->mqttSend("openHAB/Heating/set/Kitchen", "{\"button\":\"Boost\"}");
      break;
    case id::Right:
      // mWifi->mqttSend("openHAB/Heating/set/Kitchen", "{\"button\":\"Advance\"}");
      break;
    default:
      break;
    }
    return true;
  }
  return true;
}