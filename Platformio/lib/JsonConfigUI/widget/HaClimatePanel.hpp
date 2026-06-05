#pragma once

#include "HaAttrs.hpp"
#include "UIElement.hpp"
#include <lvgl.h>
#include <string>

namespace UI::Widget {

/** Home Assistant climate thermostat (arc + mode + setpoint controls). */
class HaClimatePanel : public UIElement {
public:
  HaClimatePanel(lv_obj_t *parent, const std::string &entityId);
  ~HaClimatePanel() override;

  const std::string &entityId() const { return mEntityId; }
  void refreshFromCache();
  void requestFetchIfNeeded();

protected:
  bool OnKeyEvent(KeyPressAbstract::KeyEvent aKeyEvent) override { return false; }

private:
  struct Ui;

  void buildUi();
  void applyAttrs(const HaClimate::Attrs &ca);
  void closeModeMenu();
  void showStatus(const char *msg);
  void callSetTemperature(float target);
  void callSetRange(float low, float high);
  void callSetMode(const char *mode);
  void onAdjust(int which);
  static void onModeLabelClick(lv_event_t *e);
  static void onAdjustBtn(lv_event_t *e);
  static void onModeBtn(lv_event_t *e);

  std::string mEntityId;
  Ui *mUi = nullptr;
  HaClimate::Attrs mLastAttrs;
  bool mHasAttrs = false;
};

} // namespace UI::Widget
