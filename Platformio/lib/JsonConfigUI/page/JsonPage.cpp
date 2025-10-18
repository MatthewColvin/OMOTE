#include "JsonPage.hpp"
#include "Button.hpp"
#include "ColorButtons.hpp"
#include "HardwareFactory.hpp"
#include "Image.hpp"
#include "Label.hpp"
#include "NumberPad.hpp"
#include "magic_enum.hpp"
#include "observerHandles.hpp"
#include <fstream>

using namespace UI::Page;
using namespace Command;

JsonPage::JsonPage(std::string aFileName, std::string aPageName, std::string aCommandPrefix)
    : Base(ID::Pages::JsonPage) {

  std::ifstream file(FS_PATH + aFileName, std::ios::in);
  if (!file)
    return;

  std::stringstream buffer;
  buffer << file.rdbuf();
  file.close();
  std::string content(buffer.str());

  MemConsciousDocument d;
  if (d.Parse<rapidjson::ParseFlag::kParseCommentsFlag>(content.c_str()).HasParseError())
    return;

  if (d.HasMember("CommandFile") && d["CommandFile"].IsString()) {
    mCommandFile = d["CommandFile"].GetString();
  }

  if (d.HasMember("Widgets") && d["Widgets"].IsArray()) {
    for (rapidjson::SizeType i = 0; i < d["Widgets"].Size(); i++) {
      if (d["Widgets"][i].HasMember("Type") && d["Widgets"][i]["Type"].IsString()) {
        std::string type = d["Widgets"][i]["Type"].GetString();

        if (type == "Title") {
          addTitle(aCommandPrefix, d["Widgets"][i].GetObject(), aPageName);
        } else if (type == "Label") {
          addLabel(aCommandPrefix, d["Widgets"][i].GetObject());
        } else if (type == "Button") {
          addButton(aCommandPrefix, d["Widgets"][i].GetObject());
        } else if (type == "Image") {
          addImage(d["Widgets"][i].GetObject());
        } else if (type == "ColorButtons") {
          addColorButtons(aCommandPrefix, d["Widgets"][i].GetObject());
        } else if (type == "NumberPad") {
          addNumberPad(aCommandPrefix, d["Widgets"][i].GetObject());
        }
      }
    }
  }

  if (d.HasMember("ButtonMaps") && d["ButtonMaps"].IsObject()) {
    for (Command::KeyIds id = Command::KeyIds::Power; id != Command::KeyIds::INVALID; id = (Command::KeyIds)((int)id + 1)) {
      auto key = magic_enum::enum_name(id);
      if (d["ButtonMaps"].HasMember(key.data())) {
        Command::CommandStruct commandStruct;
        if (d["ButtonMaps"][key.data()].HasMember("Press") && d["ButtonMaps"][key.data()]["Press"].IsString()) {
          if (Command::Commands::getCommand(mCommandFile, aCommandPrefix, d["ButtonMaps"][key.data()]["Press"].GetString(), commandStruct) != Command::NONE)
            mKeyHandlers.insert({id, {Command::KeyPressTypes::Press, commandStruct}});
        }
        if (d["ButtonMaps"][key.data()].HasMember("Release") && d["ButtonMaps"][key.data()]["Release"].IsString()) {
          if (Command::Commands::getCommand(mCommandFile, aCommandPrefix, d["ButtonMaps"][key.data()]["Release"].GetString(), commandStruct) != Command::NONE)
            mKeyHandlers.insert({id, {Command::KeyPressTypes::Release, commandStruct}});
        }
        if (d["ButtonMaps"][key.data()].HasMember("Repeat") && d["ButtonMaps"][key.data()]["Repeat"].IsString()) {
          if (Command::Commands::getCommand(mCommandFile, aCommandPrefix, d["ButtonMaps"][key.data()]["Repeat"].GetString(), commandStruct) != Command::NONE)
            mKeyHandlers.insert({id, {Command::KeyPressTypes::Repeat, commandStruct}});
        }
        if (d["ButtonMaps"][key.data()].HasMember("Long") && d["ButtonMaps"][key.data()]["Long"].IsString()) {
          if (Command::Commands::getCommand(mCommandFile, aCommandPrefix, d["ButtonMaps"][key.data()]["Long"].GetString(), commandStruct) != Command::NONE)
            mKeyHandlers.insert({id, {Command::KeyPressTypes::Long, commandStruct}});
        }
        if (d["ButtonMaps"][key.data()].HasMember("Short") && d["ButtonMaps"][key.data()]["Short"].IsString()) {
          if (Command::Commands::getCommand(mCommandFile, aCommandPrefix, d["ButtonMaps"][key.data()]["Short"].GetString(), commandStruct) != Command::NONE)
            mKeyHandlers.insert({id, {Command::KeyPressTypes::Short, commandStruct}});
        }
      }
    }
  }
}

JsonPage::~JsonPage() {
  // unbind any MQTT subscriptions
  for (auto &id : mSubscriptions) {
    HardwareFactory::getAbstract().wifi()->mqttUnBindTextEvent(id);
    UI::observerHandles::deleteHandle(id);
  }
}

void JsonPage::addTitle(const std::string &aCommandPrefix, const MemConciousValue &value, std::string aPageName) {
  auto title = std::make_unique<Widget::Label>(aPageName);
  if (value.HasMember("HeightPct") && value["HeightPct"].IsUint())
    title->SetHeight(lv_pct(value["HeightPct"].GetUint()));
  if (value.HasMember("AlignTo") && value["AlignTo"].IsUint()) {
    unsigned int index = value["AlignTo"].GetUint();
    if (index == 0)
      title->AlignTo(this, LV_ALIGN_TOP_MID, 0, distBetweenWidgets);
    else if (index < mWidgets.size())
      title->AlignTo(mWidgets[index], LV_ALIGN_OUT_BOTTOM_MID, 0, distBetweenWidgets);
  }

  mWidgets.push_back(AddElement(std::move(title)));
}

void JsonPage::addLabel(const std::string &aCommandPrefix, const MemConciousValue &value) {
  auto label = std::make_unique<Widget::Label>("");
  if (value.HasMember("Text") && value["Text"].IsString())
    label->SetText(value["Text"].GetString());
  if (value.HasMember("HeightPct") && value["HeightPct"].IsUint())
    label->SetHeight(lv_pct(value["HeightPct"].GetUint()));
  if (value.HasMember("AlignTo") && value["AlignTo"].IsUint()) {
    unsigned int index = value["AlignTo"].GetUint();
    if (index == 0)
      label->AlignTo(this, LV_ALIGN_TOP_MID, 0, distBetweenWidgets);
    else if (index <= mWidgets.size())
      label->AlignTo(mWidgets[index - 1], LV_ALIGN_OUT_BOTTOM_MID, 0, distBetweenWidgets);
  }

  if (value.HasMember("Command") && value["Command"].IsString() && !mCommandFile.empty()) {
    Command::CommandStruct commandStruct;
    if (Command::Commands::getCommand(mCommandFile, aCommandPrefix, value["Command"].GetString(), commandStruct) == Command::MQTT) {
      if ((commandStruct.protocol == "SUB") && (commandStruct.data.size() == 4)) {
        std::string text("");
        if (value.HasMember("Text") && value["Text"].IsString())
          text = value["Text"].GetString();
        // need to ensure that format string does not go out of scope
        mFormatStrings.push_back(std::make_unique<char[]>(commandStruct.data[2].size() + 1));
        strncpy(mFormatStrings.back().get(), commandStruct.data[2].c_str(), (commandStruct.data[2].size() + 1));
        uint32_t id = label->RegisterBindTextEvent(std::stoi(commandStruct.data[3]), mFormatStrings.back().get(), text.c_str());
        HardwareFactory::getAbstract().wifi()->mqttBindTextEvent(id, commandStruct.data[0], commandStruct.data[1]);
        mSubscriptions.push_back(id);
      }
    }
  }
  mWidgets.push_back(AddElement(std::move(label)));
}

void JsonPage::addButton(const std::string &aCommandPrefix, const MemConciousValue &value) {
  if (value.HasMember("Command") && value["Command"].IsString() && !mCommandFile.empty()) {
    Command::CommandStruct commandStruct;
    auto actionProto = Command::Commands::getCommand(mCommandFile, aCommandPrefix, value["Command"].GetString(), commandStruct);
    // only process button if we have an action to associate with it
    if (actionProto != Command::NONE) {
      auto button = std::make_unique<Widget::Button>([this, commandStruct]() { Command::Commands::sendCommand(commandStruct); });
      if (value.HasMember("Text") && value["Text"].IsString())
        button->SetText(value["Text"].GetString());
      if (value.HasMember("HeightPct") && value["HeightPct"].IsUint())
        button->SetHeight(lv_pct(value["HeightPct"].GetUint()));
      if (value.HasMember("SizeXY") && value["SizeXY"].IsArray())
        if (value["SizeXY"].Size() == 2 && value["SizeXY"][0].IsUint() && value["SizeXY"][1].IsUint())
          button->SetSize(lv_pct(value["SizeXY"][0].GetUint()), lv_pct(value["SizeXY"][1].GetUint()));
      if (value.HasMember("AlignTo") && value["AlignTo"].IsUint()) {
        unsigned int index = value["AlignTo"].GetUint();
        if (index == 0)
          button->AlignTo(this, LV_ALIGN_TOP_MID, 0, distBetweenWidgets);
        else if (index <= mWidgets.size())
          button->AlignTo(mWidgets[index - 1], LV_ALIGN_OUT_BOTTOM_MID, 0, distBetweenWidgets);
      }
      // set position after align so can adjust
      if (value.HasMember("PosX") && value["PosX"].IsUint())
        button->SetX(lv_pct(value["PosX"].GetUint()));
      if (value.HasMember("PosY") && value["PosY"].IsUint())
        button->SetY(lv_pct(value["PosY"].GetUint()));
      mWidgets.push_back(AddElement(std::move(button)));
    }
  }
}

void JsonPage::addImage(const MemConciousValue &value) {
  if (value.HasMember("FileName") && value["FileName"].IsString()) {
    std::string file = value["FileName"].GetString();
    auto image = std::make_unique<Widget::Image>(file.c_str());
    if (value.HasMember("SizeXYinPixels") && value["SizeXYinPixels"].IsArray())
      if (value["SizeXYinPixels"].Size() == 2 && value["SizeXYinPixels"][0].IsUint() && value["SizeXYinPixels"][1].IsUint())
        image->SetSize(value["SizeXYinPixels"][0].GetUint(), value["SizeXYinPixels"][1].GetUint());
    if (value.HasMember("AlignTo") && value["AlignTo"].IsUint()) {
      unsigned int index = value["AlignTo"].GetUint();
      if (index == 0)
        image->AlignTo(this, LV_ALIGN_TOP_MID, 0, distBetweenWidgets);
      else if (index <= mWidgets.size())
        image->AlignTo(mWidgets[index - 1], LV_ALIGN_OUT_BOTTOM_MID, 0, distBetweenWidgets);
    }
    // set position after align so can adjust
    if (value.HasMember("PosX") && value["PosX"].IsUint())
      image->SetX(lv_pct(value["PosX"].GetUint()));
    if (value.HasMember("PosY") && value["PosY"].IsUint())
      image->SetY(lv_pct(value["PosY"].GetUint()));
    mWidgets.push_back(AddElement(std::move(image)));
  }
}

void JsonPage::addColorButtons(const std::string &aCommandPrefix, const MemConciousValue &value) {
  if (value.HasMember("Command") && value["Command"].IsArray() && !mCommandFile.empty()) {
    std::vector<Command::CommandStruct> commandStructs;
    for (rapidjson::SizeType i = 0; i < value["Command"].Size(); i++) {
      if (value["Command"][i].IsString()) {
        Command::CommandStruct commandStruct;
        auto actionProto = Command::Commands::getCommand(mCommandFile, aCommandPrefix, value["Command"][i].GetString(), commandStruct);
        // only process button if we have an action to associate with it
        if (actionProto != Command::NONE)
          commandStructs.push_back(commandStruct);
      }
    }
    auto colorButton = std::make_unique<Widget::ColorButtons>(commandStructs);
    if (value.HasMember("AlignTo") && value["AlignTo"].IsUint()) {
      unsigned int index = value["AlignTo"].GetUint();
      if (index == 0)
        colorButton->AlignTo(this, LV_ALIGN_TOP_MID, 0, distBetweenWidgets);
      else if (index <= mWidgets.size())
        colorButton->AlignTo(mWidgets[index - 1], LV_ALIGN_OUT_BOTTOM_MID, 0, distBetweenWidgets);
    }
    mWidgets.push_back(AddElement(std::move(colorButton)));
  }
}

void JsonPage::addNumberPad(const std::string &aCommandPrefix, const MemConciousValue &value) {
  if (value.HasMember("Command") && value["Command"].IsArray() && !mCommandFile.empty()) {
    std::vector<Command::CommandStruct> commandStructs;
    for (rapidjson::SizeType i = 0; i < value["Command"].Size(); i++) {
      if (value["Command"][i].IsString()) {
        Command::CommandStruct commandStruct;
        auto actionProto = Command::Commands::getCommand(mCommandFile, aCommandPrefix, value["Command"][i].GetString(), commandStruct);
        // only process button if we have an action to associate with it
        if (actionProto != Command::NONE)
          commandStructs.push_back(commandStruct);
      }
    }
    auto numberPad = std::make_unique<Widget::NumberPad>(commandStructs);
    if (value.HasMember("AlignTo") && value["AlignTo"].IsUint()) {
      unsigned int index = value["AlignTo"].GetUint();
      if (index == 0)
        numberPad->AlignTo(this, LV_ALIGN_TOP_MID, 0, distBetweenWidgets);
      else if (index <= mWidgets.size())
        numberPad->AlignTo(mWidgets[index - 1], LV_ALIGN_OUT_BOTTOM_MID, 0, distBetweenWidgets);
    }
    mWidgets.push_back(AddElement(std::move(numberPad)));
  }
}

void JsonPage::getKeyOverrides(const MemConciousValue &value, std::multimap<Command::KeyIds, Command::KeyStruct> &aKeyHandlers) {
  if (value.IsArray()) {
    for (rapidjson::SizeType i = 0; i < value.Size(); i++) {
      if (value[i].IsString()) {
        auto id = magic_enum::enum_cast<Command::KeyIds>(value[i].GetString());
        if (id.has_value()) {
          auto range = mKeyHandlers.equal_range(id.value());
          if (range.first != mKeyHandlers.end()) {
            for (auto i = range.first; i != range.second; ++i) {
              aKeyHandlers.insert({i->first, i->second});
            }
            mKeyHandlers.erase(id.value()); // being overridden so may as well remove here
          }
        }
      }
    }
  }
}

bool JsonPage::OnKeyEvent(KeyPressAbstract::KeyEvent aKeyEvent) {

  auto range = mKeyHandlers.equal_range(aKeyEvent.mId);
  if (range.first != mKeyHandlers.end()) {
    for (auto i = range.first; i != range.second; ++i) {
      if (i->second.pressType == aKeyEvent.mType) {
        Command::Commands::sendCommand(i->second.command);
        return true;
      }
    }
  }
  return false;
}