#pragma once
#include "PageBase.hpp"

namespace UI::Widget {
class Label;
class Switch;
class Keyboard;
class Button;
template <typename T>
class DropDown;
class Slider;
} // namespace UI::Widget

namespace UI::Page {

/** Settings page built from DeviceSettings.schema.json + DeviceSettings.json */
class SystemSettings : public Base {
public:
  SystemSettings(const std::string &sectionFilterId = "",
                 const std::string &forcedTitle = "");
  ~SystemSettings();

protected:
  std::string GetTitle() override;

private:
  void buildFromSchema();
  void patchBool(const char *key, bool value);
  void patchInt(const char *key, int32_t value);
  void patchString(const char *key, const std::string &value);
  int32_t readIntField(const char *key, int32_t fallback) const;
  bool readBoolField(const char *key, bool fallback) const;
  std::string readStringField(const char *key, const std::string &fallback) const;
  void openStringKeyboard(const char *key, const std::string &currentValue,
                          UI::Widget::Button *button);

  UI::UIElement *mAnchor = nullptr;
  bool mSaveReqrd = false;
  UI::Widget::Keyboard *mKeyboard = nullptr;
  std::string mSectionFilterId;
  std::string mForcedTitle;
};

} // namespace UI::Page
