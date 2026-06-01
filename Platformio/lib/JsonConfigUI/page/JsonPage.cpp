#include "JsonPage.hpp"
#include "Button.hpp"
#include "Colors.hpp"
#include "ColorButtons.hpp"
#include "HaAttrs.hpp"
#include "HaClimatePanel.hpp"
#include "HaRuntime.hpp"
#include "Slider.hpp"
#include "Switch.hpp"
#include "HardwareFactory.hpp"
#include "Image.hpp"
#include "Label.hpp"
#include "LvglResourceManager.hpp"
#include "NumberPad.hpp"
#include "magic_enum.hpp"
#include "observerHandles.hpp"
#include <Arduino.h>
#include <fstream>

using namespace UI::Page;
using namespace Command;

namespace {

bool sHaSyncInProgress = false;

bool readJsonUint(const rapidjson::Value &obj, const char *key, unsigned int &out) {
  if (!obj.HasMember(key))
    return false;
  const auto &v = obj[key];
  if (v.IsUint()) {
    out = v.GetUint();
    return true;
  }
  if (v.IsInt() && v.GetInt() >= 0) {
    out = static_cast<unsigned int>(v.GetInt());
    return true;
  }
  return false;
}

std::string readEntityId(const rapidjson::Value &value) {
  if (value.HasMember("EntityId") && value["EntityId"].IsString())
    return value["EntityId"].GetString();
  return {};
}

std::filesystem::path resolvePageJsonPath(const std::string &fileName) {
  if (fileName.empty())
    return std::filesystem::path(FS_PATH);
  const std::filesystem::path direct(FS_PATH + fileName);
  {
    std::ifstream probe(direct);
    if (probe.good())
      return direct;
  }
  if (fileName.rfind("Pages/", 0) != 0) {
    const std::filesystem::path underPages(FS_PATH "Pages/" + fileName);
    std::ifstream probe(underPages);
    if (probe.good())
      return underPages;
  }
  return direct;
}

void configureJsonPage(lv_obj_t *pageObj, bool verticalScroll) {
  if (!pageObj)
    return;
  auto lock = LvglResourceManager::GetInstance().scopeLock();
  lv_obj_remove_flag(pageObj, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
  lv_obj_remove_flag(pageObj, LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
  lv_obj_clear_flag(pageObj, LV_OBJ_FLAG_SCROLL_ELASTIC);
  lv_obj_set_scrollbar_mode(pageObj, LV_SCROLLBAR_MODE_AUTO);
  if (verticalScroll) {
    lv_obj_add_flag(pageObj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(pageObj, LV_DIR_VER);
  } else {
    lv_obj_remove_flag(pageObj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(pageObj, LV_DIR_NONE);
  }
}

void clampWidgetHorizontal(UI::UIElement *widget) {
  if (!widget)
    return;
  auto lock = LvglResourceManager::GetInstance().scopeLock();
  lv_obj_set_style_max_width(widget->LvglSelf(), LV_PCT(100), LV_PART_MAIN);
}

} // namespace

JsonPage::JsonPage(std::string aFileName, std::string aPageName, std::string aCommandPrefix)
    : Base(ID::Pages::JsonPage) {

  const std::filesystem::path aPageJsonPath = resolvePageJsonPath(aFileName);

  rapidjson::Document d = OMOTE::JSON::GetDocument(aPageJsonPath);
  if (d.HasParseError() || d.IsNull() || !d.IsObject()) {
    Serial.printf("JsonPage: failed to load \"%s\" (scene FileName=\"%s\")\n", aPageJsonPath.c_str(),
                  aFileName.c_str());
    return;
  }

  if (d.HasMember("CommandFile") && d["CommandFile"].IsString()) {
    mCommandFile = d["CommandFile"].GetString();
  }

  bool pageNeedsScroll = false;
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
        } else if (type == "HaToggle") {
          addHaToggle(d["Widgets"][i].GetObject());
        } else if (type == "HaLabel") {
          addHaLabel(d["Widgets"][i].GetObject());
        } else if (type == "HaSwitch") {
          addHaSwitch(d["Widgets"][i].GetObject());
        } else if (type == "HaSlider") {
          addHaSlider(d["Widgets"][i].GetObject());
        } else if (type == "HaMomentary") {
          addHaMomentary(d["Widgets"][i].GetObject());
        } else if (type == "HaClimate") {
          addHaClimate(d["Widgets"][i].GetObject());
        } else {
          pageNeedsScroll = true;
          Serial.printf("JsonPage: unknown widget Type \"%s\" in %s\n", type.c_str(), aFileName.c_str());
        }
      }
    }
  }

  configureJsonPage(LvglSelf(), pageNeedsScroll || mWidgets.size() > 6);
  {
    auto lock = LvglResourceManager::GetInstance().scopeLock();
    lv_obj_update_layout(LvglSelf());
    lv_obj_scroll_to_x(LvglSelf(), 0, LV_ANIM_OFF);
  }
  Serial.printf("JsonPage %s: %u widgets (%u HA)\n", aFileName.c_str(), static_cast<unsigned>(mWidgets.size()),
                static_cast<unsigned>(mHaBindings.size()));

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
  HaRuntime::setActivePage(nullptr, {});
}

void JsonPage::OnShow() {
  Base::OnShow();
  HaRuntime::setActivePage(this, mHaEntityIds);
  applyHaStates();
  HaRuntime::requestRefresh();
}

void JsonPage::OnHide() {
  HaRuntime::setActivePage(nullptr, {});
  Base::OnHide();
}

void JsonPage::applyHaStates() {
  for (const auto &b : mHaBindings) {
    std::string state;
    if (!HaRuntime::getCachedState(b.entityId, state))
      continue;
    if (b.stateLabel)
      b.stateLabel->SetText(state);
    if (b.toggle) {
      const bool on = HaRuntime::stateIsOn(b.entityId, state);
      b.toggle->SetBgColor(on ? UI::Color::BTN_ACTIVE : UI::Color::BTN_PRIMARY);
      b.toggle->SetBgOpacity(LV_OPA_COVER);
    }
    if (b.sw) {
      const bool on = HaRuntime::stateIsOn(b.entityId, state);
      sHaSyncInProgress = true;
      b.sw->SetValue(on);
      sHaSyncInProgress = false;
    }
    if (b.slider) {
      std::string attrsJson;
      if (HaRuntime::getCachedAttributes(b.entityId, attrsJson)) {
        rapidjson::Document doc;
        if (!doc.Parse(attrsJson.c_str()).HasParseError() && doc.IsObject()) {
          const int val = HaClimate::readSliderValue(b.sliderDomain, doc, b.sliderAttribute, 0);
          sHaSyncInProgress = true;
          b.slider->SetValue(val, LV_ANIM_OFF);
          sHaSyncInProgress = false;
        }
      }
    }
    if (b.climate)
      b.climate->refreshFromCache();
  }
}

void JsonPage::applyWidgetLayout(UIElement *widget, const rapidjson::Value &value, unsigned int defaultHeightPct) {
  unsigned int heightPct = defaultHeightPct;
  if (readJsonUint(value, "HeightPct", heightPct) || defaultHeightPct > 0) {
    if (heightPct > 0 && heightPct < 10)
      heightPct = 10;
    widget->SetHeight(lv_pct(heightPct > 0 ? heightPct : defaultHeightPct));
  }

  if (value.HasMember("SizeXY") && value["SizeXY"].IsArray() && value["SizeXY"].Size() == 2) {
    unsigned int sx = 0, sy = 0;
    const auto &arr = value["SizeXY"];
    if (arr[0].IsUint())
      sx = arr[0].GetUint();
    else if (arr[0].IsInt() && arr[0].GetInt() >= 0)
      sx = static_cast<unsigned int>(arr[0].GetInt());
    if (arr[1].IsUint())
      sy = arr[1].GetUint();
    else if (arr[1].IsInt() && arr[1].GetInt() >= 0)
      sy = static_cast<unsigned int>(arr[1].GetInt());
    if (sx > 0 && sy > 0)
      widget->SetSize(lv_pct(sx), lv_pct(sy));
    else if (sx > 0)
      widget->SetWidth(lv_pct(sx));
    else if (sy > 0)
      widget->SetHeight(lv_pct(sy));
  } else if (!value.HasMember("HeightPct") && defaultHeightPct > 0) {
    widget->SetWidth(lv_pct(90));
  }

  unsigned int posX = 0, posY = 0;
  const bool hasPosX = readJsonUint(value, "PosX", posX);
  const bool hasPosY = readJsonUint(value, "PosY", posY);
  const bool useExplicitPos = hasPosX || hasPosY;

  if (!useExplicitPos) {
    bool aligned = false;
    unsigned int index = 0;
    if (readJsonUint(value, "AlignTo", index)) {
      if (index == 0) {
        widget->AlignTo(this, LV_ALIGN_TOP_MID, 0, distBetweenWidgets);
        aligned = true;
      } else if (index <= mWidgets.size()) {
        widget->AlignTo(mWidgets[index - 1], LV_ALIGN_OUT_BOTTOM_MID, 0, distBetweenWidgets);
        aligned = true;
      }
    }
    if (!aligned) {
      if (mWidgets.empty())
        widget->AlignTo(this, LV_ALIGN_TOP_MID, 0, distBetweenWidgets);
      else
        widget->AlignTo(mWidgets.back(), LV_ALIGN_OUT_BOTTOM_MID, 0, distBetweenWidgets);
    }
  }

  if (hasPosX)
    widget->SetX(lv_pct(posX));
  if (hasPosY)
    widget->SetY(lv_pct(posY));

  clampWidgetHorizontal(widget);
}

void JsonPage::addHaToggle(const rapidjson::Value &value) {
  const std::string entityId = readEntityId(value);

  std::string domain;
  if (value.HasMember("Domain") && value["Domain"].IsString())
    domain = value["Domain"].GetString();
  else if (!entityId.empty()) {
    const auto dot = entityId.find('.');
    domain = dot != std::string::npos ? entityId.substr(0, dot) : "light";
  } else {
    domain = "light";
  }

  std::string service = "toggle";
  if (value.HasMember("Service") && value["Service"].IsString())
    service = value["Service"].GetString();

  std::string label;
  if (value.HasMember("Text") && value["Text"].IsString())
    label = value["Text"].GetString();
  if (label.empty())
    label = entityId.empty() ? "HA Toggle" : entityId;

  auto button = std::make_unique<Widget::Button>();
  if (!entityId.empty())
    button->OnShortClick([domain, service, entityId]() { HaRuntime::callService(domain, service, entityId); });
  button->SetBgColor(UI::Color::BTN_PRIMARY);
  button->SetBgOpacity(LV_OPA_COVER);
  applyWidgetLayout(button.get(), value, 12);
  button->SetText(label);

  if (!entityId.empty()) {
    HaBinding binding;
    binding.entityId = entityId;
    binding.toggle = button.get();
    mHaBindings.push_back(binding);
    mHaEntityIds.push_back(entityId);
    Serial.printf("JsonPage: HaToggle %s\n", entityId.c_str());
  } else {
    Serial.println("JsonPage: HaToggle has no EntityId — shown but not bound to HA");
  }

  mWidgets.push_back(AddElement(std::move(button)));
}

void JsonPage::addHaLabel(const rapidjson::Value &value) {
  const std::string entityId = readEntityId(value);

  std::string text = "—";
  if (value.HasMember("Text") && value["Text"].IsString())
    text = value["Text"].GetString();
  if (text == "—" && !entityId.empty())
    text = entityId;

  auto label = std::make_unique<Widget::Label>(text);
  applyWidgetLayout(label.get(), value, 8);

  if (!entityId.empty()) {
    HaBinding binding;
    binding.entityId = entityId;
    binding.stateLabel = label.get();
    mHaBindings.push_back(binding);
    mHaEntityIds.push_back(entityId);
    Serial.printf("JsonPage: HaLabel %s\n", entityId.c_str());
  } else {
    Serial.println("JsonPage: HaLabel has no EntityId — shown but not bound to HA");
  }

  mWidgets.push_back(AddElement(std::move(label)));
}

void JsonPage::addHaSwitch(const rapidjson::Value &value) {
  const std::string entityId = readEntityId(value);

  std::string domain;
  if (value.HasMember("Domain") && value["Domain"].IsString())
    domain = value["Domain"].GetString();
  else if (!entityId.empty()) {
    const auto dot = entityId.find('.');
    domain = dot != std::string::npos ? entityId.substr(0, dot) : "light";
  } else {
    domain = "light";
  }

  std::string serviceOn = "turn_on";
  std::string serviceOff = "turn_off";
  if (value.HasMember("ServiceOn") && value["ServiceOn"].IsString())
    serviceOn = value["ServiceOn"].GetString();
  if (value.HasMember("ServiceOff") && value["ServiceOff"].IsString())
    serviceOff = value["ServiceOff"].GetString();

  auto sw = std::make_unique<Widget::Switch>([domain, serviceOn, serviceOff, entityId](bool on) {
    if (sHaSyncInProgress || entityId.empty())
      return;
    HaRuntime::callService(domain, on ? serviceOn : serviceOff, entityId);
  });

  if (value.HasMember("Text") && value["Text"].IsString()) {
    auto caption = std::make_unique<Widget::Label>(value["Text"].GetString());
    caption->SetWidth(lv_pct(90));
    applyWidgetLayout(caption.get(), value, 8);
    mWidgets.push_back(AddElement(std::move(caption)));
  }

  applyWidgetLayout(sw.get(), value, 10);

  if (!entityId.empty()) {
    HaBinding binding;
    binding.entityId = entityId;
    binding.sw = sw.get();
    mHaBindings.push_back(binding);
    mHaEntityIds.push_back(entityId);
    Serial.printf("JsonPage: HaSwitch %s\n", entityId.c_str());
  }

  mWidgets.push_back(AddElement(std::move(sw)));
}

void JsonPage::addHaSlider(const rapidjson::Value &value) {
  const std::string entityId = readEntityId(value);

  std::string domain = "light";
  if (value.HasMember("Domain") && value["Domain"].IsString())
    domain = value["Domain"].GetString();
  else if (!entityId.empty()) {
    const auto dot = entityId.find('.');
    domain = dot != std::string::npos ? entityId.substr(0, dot) : "light";
  }

  std::string attribute;
  if (value.HasMember("Attribute") && value["Attribute"].IsString())
    attribute = value["Attribute"].GetString();

  std::string service = "turn_on";
  if (value.HasMember("Service") && value["Service"].IsString())
    service = value["Service"].GetString();

  int minVal = 0;
  int maxVal = 255;
  if (value.HasMember("Min") && value["Min"].IsInt())
    minVal = value["Min"].GetInt();
  if (value.HasMember("Max") && value["Max"].IsInt())
    maxVal = value["Max"].GetInt();
  if (domain == "cover" && maxVal == 255)
    maxVal = 100;

  auto slider = std::make_unique<Widget::Slider>(
      [domain, service, entityId, attribute](int32_t val) {
        if (sHaSyncInProgress || entityId.empty())
          return;
        rapidjson::Document doc;
        doc.SetObject();
        auto &a = doc.GetAllocator();
        std::string key = attribute;
        if (key.empty()) {
          if (domain == "light")
            key = "brightness";
          else if (domain == "cover")
            key = "position";
          else if (domain == "fan")
            key = "percentage";
          else
            key = "brightness";
        }
        doc.AddMember(rapidjson::StringRef(key.c_str()), rapidjson::Value(static_cast<int>(val)), a);
        HaRuntime::callServiceWithData(domain, service, entityId, OMOTE::JSON::ToString(doc));
      },
      minVal, maxVal);
  slider->UpdateOnReleaseOnly(true);

  if (value.HasMember("Text") && value["Text"].IsString()) {
    auto caption = std::make_unique<Widget::Label>(value["Text"].GetString());
    caption->SetWidth(lv_pct(90));
    applyWidgetLayout(caption.get(), value, 8);
    mWidgets.push_back(AddElement(std::move(caption)));
  }

  applyWidgetLayout(slider.get(), value, 8);

  if (!entityId.empty()) {
    HaBinding binding;
    binding.entityId = entityId;
    binding.slider = slider.get();
    binding.sliderDomain = domain;
    binding.sliderAttribute = attribute;
    binding.sliderService = service;
    mHaBindings.push_back(binding);
    mHaEntityIds.push_back(entityId);
    HaRuntime::fetchEntityState(entityId);
    Serial.printf("JsonPage: HaSlider %s\n", entityId.c_str());
  }

  mWidgets.push_back(AddElement(std::move(slider)));
}

void JsonPage::addHaMomentary(const rapidjson::Value &value) {
  const std::string entityId = readEntityId(value);

  std::string domain;
  if (value.HasMember("Domain") && value["Domain"].IsString())
    domain = value["Domain"].GetString();
  else if (!entityId.empty()) {
    const auto dot = entityId.find('.');
    domain = dot != std::string::npos ? entityId.substr(0, dot) : "light";
  } else {
    domain = "light";
  }

  std::string serviceOn = "turn_on";
  std::string serviceOff = "turn_off";
  if (value.HasMember("ServiceOn") && value["ServiceOn"].IsString())
    serviceOn = value["ServiceOn"].GetString();
  if (value.HasMember("ServiceOff") && value["ServiceOff"].IsString())
    serviceOff = value["ServiceOff"].GetString();

  std::string label = "Hold";
  if (value.HasMember("Text") && value["Text"].IsString())
    label = value["Text"].GetString();
  if (label.empty())
    label = entityId.empty() ? "Momentary" : entityId;

  auto button = std::make_unique<Widget::Button>();
  if (!entityId.empty()) {
    button->OnPress([domain, serviceOn, entityId]() { HaRuntime::callService(domain, serviceOn, entityId); });
    button->OnRelease([domain, serviceOff, entityId]() { HaRuntime::callService(domain, serviceOff, entityId); });
  }
  button->SetBgColor(UI::Color::BTN_PRIMARY);
  button->SetBgOpacity(LV_OPA_COVER);
  applyWidgetLayout(button.get(), value, 12);
  button->SetText(label);

  if (!entityId.empty()) {
    mHaEntityIds.push_back(entityId);
    Serial.printf("JsonPage: HaMomentary %s\n", entityId.c_str());
  }

  mWidgets.push_back(AddElement(std::move(button)));
}

void JsonPage::addHaClimate(const rapidjson::Value &value) {
  const std::string entityId = readEntityId(value);

  auto panel = std::make_unique<Widget::HaClimatePanel>(LvglSelf(), entityId);
  panel->SetWidth(lv_pct(100));
  unsigned int heightPct = 58;
  readJsonUint(value, "HeightPct", heightPct);
  if (heightPct < 40)
    heightPct = 40;
  panel->SetHeight(lv_pct(heightPct));

  unsigned int posX = 0, posY = 0;
  if (readJsonUint(value, "PosX", posX))
    panel->SetX(lv_pct(posX));
  if (readJsonUint(value, "PosY", posY))
    panel->SetY(lv_pct(posY));
  else
    panel->AlignTo(this, LV_ALIGN_TOP_MID, 0, distBetweenWidgets);

  clampWidgetHorizontal(panel.get());

  if (!entityId.empty()) {
    HaBinding binding;
    binding.entityId = entityId;
    binding.climate = panel.get();
    mHaBindings.push_back(binding);
    mHaEntityIds.push_back(entityId);
    panel->requestFetchIfNeeded();
    Serial.printf("JsonPage: HaClimate %s\n", entityId.c_str());
  } else {
    Serial.println("JsonPage: HaClimate has no EntityId");
  }

  mWidgets.push_back(AddElement(std::move(panel)));
}

void JsonPage::addTitle(const std::string &aCommandPrefix, const rapidjson::Value &value, std::string aPageName) {
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

void JsonPage::addLabel(const std::string &aCommandPrefix, const rapidjson::Value &value) {
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

void JsonPage::addButton(const std::string &aCommandPrefix, const rapidjson::Value &value) {
  if (value.HasMember("Command") && value["Command"].IsString() && !mCommandFile.empty()) {
    Command::CommandStruct commandStruct;
    auto actionProto = Command::Commands::getCommand(mCommandFile, aCommandPrefix, value["Command"].GetString(), commandStruct);
    // only process button if we have an action to associate with it
    if (actionProto != Command::NONE) {
      auto button = std::make_unique<Widget::Button>([this, commandStruct]() { Command::Commands::sendCommand(commandStruct); });
      applyWidgetLayout(button.get(), value, 12);
      if (value.HasMember("Text") && value["Text"].IsString())
        button->SetText(value["Text"].GetString());
      mWidgets.push_back(AddElement(std::move(button)));
    }
  }
}

void JsonPage::addImage(const rapidjson::Value &value) {
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
    clampWidgetHorizontal(image.get());
    mWidgets.push_back(AddElement(std::move(image)));
  }
}

void JsonPage::addColorButtons(const std::string &aCommandPrefix, const rapidjson::Value &value) {
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
    applyWidgetLayout(colorButton.get(), value, 0);
    clampWidgetHorizontal(colorButton.get());
    mWidgets.push_back(AddElement(std::move(colorButton)));
  }
}

void JsonPage::addNumberPad(const std::string &aCommandPrefix, const rapidjson::Value &value) {
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
    applyWidgetLayout(numberPad.get(), value, 0);
    clampWidgetHorizontal(numberPad.get());
    mWidgets.push_back(AddElement(std::move(numberPad)));
  }
}

void JsonPage::getKeyOverrides(const rapidjson::Value &value, std::multimap<Command::KeyIds, Command::KeyStruct> &aKeyHandlers) {
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