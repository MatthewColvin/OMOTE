#include "SystemSettings.hpp"

#include "DropDown.hpp"
#include "HardwareFactory.hpp"
#include "Keyboard.hpp"
#include "Label.hpp"
#include "Button.hpp"
#include "Slider.hpp"
#include "Switch.hpp"
#include "device_settings.hpp"
#include "device_settings_schema.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

using namespace UI::Page;

namespace {

int32_t readJsonInt(const rapidjson::Value &obj, const char *key, int32_t fallback) {
  if (!obj.IsObject() || !key || !obj.HasMember(key))
    return fallback;
  const auto &v = obj[key];
  if (v.IsInt())
    return v.GetInt();
  if (v.IsUint())
    return (int32_t)v.GetUint();
  if (v.IsNumber())
    return (int32_t)v.GetDouble();
  return fallback;
}

bool readJsonBool(const rapidjson::Value &values, const char *key, bool fallback) {
  if (!values.IsObject() || !values.HasMember(key) || !values[key].IsBool())
    return fallback;
  return values[key].GetBool();
}

std::string readJsonString(const rapidjson::Value &values, const char *key,
                           const std::string &fallback) {
  if (!values.IsObject() || !values.HasMember(key) || !values[key].IsString())
    return fallback;
  return values[key].GetString();
}

int32_t readFieldDefault(const rapidjson::Value &field) {
  if (field.HasMember("default")) {
    const auto &d = field["default"];
    if (d.IsInt())
      return d.GetInt();
    if (d.IsUint())
      return (int32_t)d.GetUint();
    if (d.IsBool())
      return d.GetBool() ? 1 : 0;
    if (d.IsNumber())
      return (int32_t)d.GetDouble();
  }
  return 0;
}

std::string readJsonStringOpt(const rapidjson::Value &opt, const char *key,
                              const std::string &fallback) {
  if (!opt.IsObject() || !opt.HasMember(key))
    return fallback;
  const auto &v = opt[key];
  if (v.IsString())
    return v.GetString();
  if (v.IsInt())
    return std::to_string(v.GetInt());
  if (v.IsUint())
    return std::to_string(v.GetUint());
  return fallback;
}

std::string matchStringOption(const std::string &current, const rapidjson::Value &options) {
  if (!options.IsArray())
    return current;
  for (rapidjson::SizeType i = 0; i < options.Size(); ++i) {
    const auto &opt = options[i];
    if (!opt.IsObject())
      continue;
    const std::string val = readJsonStringOpt(opt, "value", "");
    if (val == current)
      return current;
  }
  return current;
}

int32_t nearestChoiceValue(int32_t current, const rapidjson::Value &options) {
  if (!options.IsArray() || options.Empty())
    return current;
  int32_t best = readJsonInt(options[0], "value", 0);
  int32_t bestDist = abs(current - best);
  for (rapidjson::SizeType i = 1; i < options.Size(); ++i) {
    const int32_t v = readJsonInt(options[i], "value", best);
    const int32_t dist = abs(current - v);
    if (dist < bestDist) {
      bestDist = dist;
      best = v;
    }
  }
  return best;
}

} // namespace

SystemSettings::SystemSettings(const std::string &sectionFilterId,
                               const std::string &forcedTitle)
    : Base(ID::Pages::SystemSettings), mSectionFilterId(sectionFilterId),
      mForcedTitle(forcedTitle) {
  lv_obj_add_flag(LvglSelf(), LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(LvglSelf(), LV_DIR_VER);
  lv_obj_set_scrollbar_mode(LvglSelf(), LV_SCROLLBAR_MODE_AUTO);

  device_settings_schema::loadFromLittleFS();
  device_settings::loadFromLittleFS();
  buildFromSchema();
}

SystemSettings::~SystemSettings() {
  if (mSaveReqrd) {
    device_settings::applyToHardware();
    device_settings::saveToLittleFS();
    HardwareFactory::getAbstract().saveSettings();
  }
}

std::string SystemSettings::GetTitle() {
  if (!mForcedTitle.empty())
    return mForcedTitle;
  const auto &schema = device_settings_schema::document();
  if (!mSectionFilterId.empty() && schema.IsObject() && schema.HasMember("sections") &&
      schema["sections"].IsArray()) {
    const auto &sections = schema["sections"];
    for (rapidjson::SizeType i = 0; i < sections.Size(); ++i) {
      const auto &section = sections[i];
      if (!section.IsObject() || !section.HasMember("id") || !section["id"].IsString())
        continue;
      if (mSectionFilterId == section["id"].GetString() && section.HasMember("title") &&
          section["title"].IsString()) {
        return section["title"].GetString();
      }
    }
  }
  if (schema.IsObject() && schema.HasMember("title") && schema["title"].IsString())
    return schema["title"].GetString();
  return "Device settings";
}

void SystemSettings::patchBool(const char *key, bool value) {
  rapidjson::Document patch;
  patch.SetObject();
  auto &a = patch.GetAllocator();
  patch.AddMember(rapidjson::StringRef(key), value, a);
  device_settings::mergeFromJson(patch);
  device_settings::applyToHardware();
  mSaveReqrd = true;
}

void SystemSettings::patchInt(const char *key, int32_t value) {
  rapidjson::Document patch;
  patch.SetObject();
  auto &a = patch.GetAllocator();
  patch.AddMember(rapidjson::StringRef(key), rapidjson::Value(static_cast<int>(value)), a);
  device_settings::mergeFromJson(patch);
  device_settings::applyToHardware();
  mSaveReqrd = true;
}

void SystemSettings::patchString(const char *key, const std::string &value) {
  rapidjson::Document patch;
  patch.SetObject();
  auto &a = patch.GetAllocator();
  patch.AddMember(rapidjson::StringRef(key), rapidjson::Value(value.c_str(), a), a);
  device_settings::mergeFromJson(patch);
  device_settings::applyToHardware();
  mSaveReqrd = true;
}

int32_t SystemSettings::readIntField(const char *key, int32_t fallback) const {
  const auto values = device_settings::toJsonDocument();
  return readJsonInt(values, key, fallback);
}

bool SystemSettings::readBoolField(const char *key, bool fallback) const {
  const auto values = device_settings::toJsonDocument();
  return readJsonBool(values, key, fallback);
}

std::string SystemSettings::readStringField(const char *key, const std::string &fallback) const {
  const auto values = device_settings::toJsonDocument();
  return readJsonString(values, key, fallback);
}

void SystemSettings::openStringKeyboard(const char *key, const std::string &currentValue,
                                        UI::Widget::Button *button) {
  if (mKeyboard)
    return;
  auto keyboard = std::make_unique<UI::Widget::Keyboard>(
      [this, key, button](const std::string entered) {
        if (!entered.empty()) {
          patchString(key, entered);
          if (button)
            button->SetText(entered);
        }
        if (mKeyboard)
          mKeyboard->AnimateOut();
      },
      currentValue);
  keyboard->OnKeyboardAnimatedOut([this] {
    if (!mKeyboard)
      return;
    RemoveElement(mKeyboard);
    mKeyboard = nullptr;
  });
  mKeyboard = AddElement(std::move(keyboard));
}

void SystemSettings::buildFromSchema() {
  const auto values = device_settings::toJsonDocument();
  const auto &schema = device_settings_schema::document();

  mAnchor = nullptr;
  const lv_coord_t width = GetContentWidth();
  static constexpr lv_coord_t kTopGap = 4;
  static constexpr lv_coord_t kRowGap = 8;
  static constexpr lv_coord_t kLabelToControlGap = 4;
  static constexpr lv_coord_t kSectionGap = 10;
  static constexpr lv_coord_t kSectionLabelH = 16;
  static constexpr lv_coord_t kFieldLabelH = 14;
  static constexpr lv_coord_t kHintLabelH = 12;
  static constexpr lv_coord_t kControlH = 30;

  auto prepareFieldLabel = [&](UI::Widget::Label *label) {
    label->SetWidth(width);
    label->SetHeight(kFieldLabelH);
    label->SetLongMode(LV_LABEL_LONG_DOT);
  };

  auto placeBelow = [&](UI::Widget::Label *label, UIElement *control, bool switchRow) {
    if (!mAnchor) {
      label->AlignTo(this, LV_ALIGN_TOP_LEFT, 0, kTopGap);
    } else {
      label->AlignTo(mAnchor, LV_ALIGN_OUT_BOTTOM_LEFT, 0, kRowGap);
    }
    if (switchRow) {
      auto *sw = static_cast<UI::Widget::Switch *>(control);
      sw->SetSize(lv_pct(20), 18);
      sw->AlignTo(label, LV_ALIGN_OUT_RIGHT_MID);
      // Anchor following rows to the label (full width), not the switch on the right.
      mAnchor = label;
    } else {
      control->SetWidth(width);
      control->AlignTo(label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, kLabelToControlGap);
      mAnchor = control;
    }
  };

  if (!schema.IsObject() || !schema.HasMember("sections") || !schema["sections"].IsArray()) {
    auto *label = AddNewElement<UI::Widget::Label>("No DeviceSettings.schema.json");
    label->AlignTo(this, LV_ALIGN_TOP_LEFT, 0, 10);
    return;
  }

  const auto &sections = schema["sections"];
  for (rapidjson::SizeType si = 0; si < sections.Size(); ++si) {
    const auto &section = sections[si];
    if (!section.IsObject())
      continue;

    const char *sectionId = section.HasMember("id") && section["id"].IsString()
                                ? section["id"].GetString()
                                : "";
    const char *placement =
        section.HasMember("placement") && section["placement"].IsString()
            ? section["placement"].GetString()
            : "submenu";
    const bool isMenuSection = strcmp(placement, "menu") == 0;
    if (!mSectionFilterId.empty()) {
      if (mSectionFilterId != sectionId)
        continue;
    } else if (isMenuSection) {
      continue;
    }

    // Dedicated menu pages already show the section title in the popup header.
    if (mSectionFilterId.empty()) {
      const char *sectionTitle = section.HasMember("title") && section["title"].IsString()
                                     ? section["title"].GetString()
                                     : "Settings";
      auto *sectionLabel = AddNewElement<UI::Widget::Label>(sectionTitle);
      sectionLabel->SetWidth(width);
      sectionLabel->SetHeight(kSectionLabelH);
      if (!mAnchor)
        sectionLabel->AlignTo(this, LV_ALIGN_TOP_LEFT, 0, si == 0 ? kTopGap : kSectionGap);
      else
        sectionLabel->AlignTo(mAnchor, LV_ALIGN_OUT_BOTTOM_LEFT, 0, kSectionGap);
      mAnchor = sectionLabel;
    }

    if (section.HasMember("hint") && section["hint"].IsString()) {
      auto *hint = AddNewElement<UI::Widget::Label>(section["hint"].GetString());
      hint->SetWidth(width);
      hint->SetHeight(kHintLabelH);
      hint->AlignTo(mAnchor, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 1);
      mAnchor = hint;
    }

    if (!section.HasMember("fields") || !section["fields"].IsArray())
      continue;

    const auto &fields = section["fields"];
    for (rapidjson::SizeType fi = 0; fi < fields.Size(); ++fi) {
      const auto &field = fields[fi];
      if (!field.IsObject() || !field.HasMember("key") || !field.HasMember("type"))
        continue;

      const char *key = field["key"].GetString();
      const char *type = field["type"].GetString();
      const char *labelText =
          field.HasMember("label") && field["label"].IsString() ? field["label"].GetString() : key;
      const int32_t fallback = readFieldDefault(field);
      auto *fieldLabel = AddNewElement<UI::Widget::Label>(labelText);
      prepareFieldLabel(fieldLabel);
      if (strcmp(type, "boolean") == 0) {
        const bool current = readJsonBool(values, key, fallback != 0);
        auto *sw = AddNewElement<UI::Widget::Switch>(
            [this, key](bool state) { patchBool(key, state); }, current);
        placeBelow(fieldLabel, sw, true);
      } else if (strcmp(type, "choice") == 0 && field.HasMember("options") &&
                 field["options"].IsArray()) {
        const int32_t current =
            nearestChoiceValue(readJsonInt(values, key, fallback), field["options"]);
        auto *dd = AddNewElement<Widget::DropDown<int>>(
            [this, key](int value) { patchInt(key, value); });
        dd->SetHeight(kControlH);
        const auto &options = field["options"];
        for (rapidjson::SizeType oi = 0; oi < options.Size(); ++oi) {
          const auto &opt = options[oi];
          if (!opt.IsObject())
            continue;
          const char *optLabel =
              opt.HasMember("label") && opt["label"].IsString() ? opt["label"].GetString() : "?";
          const int32_t optVal = readJsonInt(opt, "value", 0);
          dd->AddItem(optLabel, (int)optVal);
        }
        dd->SetSelected((int)current);
        placeBelow(fieldLabel, dd, false);
      } else if (strcmp(type, "slider") == 0) {
        const int32_t minV = field.HasMember("min") ? readJsonInt(field, "min", 0) : 0;
        const int32_t maxV = field.HasMember("max") ? readJsonInt(field, "max", 255) : 255;
        const int32_t current = readJsonInt(values, key, fallback);
        auto *slider = AddNewElement<Widget::Slider>(
            [this, key](int32_t v) { patchInt(key, v); }, minV, maxV);
        slider->SetHeight(kControlH);
        slider->SetValue(std::clamp(current, minV, maxV), LV_ANIM_OFF);
        placeBelow(fieldLabel, slider, false);
      } else if (strcmp(type, "string") == 0 && field.HasMember("options") &&
                 field["options"].IsArray() && !field["options"].Empty()) {
        const std::string fallback =
            field.HasMember("default") && field["default"].IsString()
                ? field["default"].GetString()
                : "";
        std::string current = readJsonString(values, key, fallback);
        current = matchStringOption(current, field["options"]);
        auto *dd = AddNewElement<Widget::DropDown<std::string>>(
            [this, key](const std::string &value) { patchString(key, value); });
        dd->SetHeight(kControlH);
        bool hasCurrent = false;
        const auto &options = field["options"];
        for (rapidjson::SizeType oi = 0; oi < options.Size(); ++oi) {
          const auto &opt = options[oi];
          if (!opt.IsObject())
            continue;
          const std::string val = readJsonStringOpt(opt, "value", "");
          if (val.empty())
            continue;
          const char *optLabel =
              opt.HasMember("label") && opt["label"].IsString() ? opt["label"].GetString() : val.c_str();
          dd->AddItem(optLabel, val);
          if (val == current)
            hasCurrent = true;
        }
        if (!hasCurrent && !current.empty())
          dd->AddItem(current.c_str(), current);
        dd->SetSelected(current);
        placeBelow(fieldLabel, dd, false);
      } else if (strcmp(type, "string") == 0) {
        const std::string current = readJsonString(values, key, "");
        auto *btn = AddNewElement<Widget::Button>();
        btn->SetHeight(kControlH);
        btn->SetText(current.empty() ? "<empty>" : current);
        placeBelow(fieldLabel, btn, false);
        btn->OnShortClick([this, key, btn] {
          const std::string latest = readStringField(key, "");
          openStringKeyboard(key, latest, btn);
        });
      }
    }
  }
}
