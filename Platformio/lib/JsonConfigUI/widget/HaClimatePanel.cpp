#include "HaClimatePanel.hpp"

#include "HaRuntime.hpp"
#include "LvglResourceManager.hpp"
#include "RapidJsonUtilty.hpp"

#include <Arduino.h>
#include <cmath>
#include <cstring>

#ifndef SCREEN_WIDTH
#define SCREEN_WIDTH 240
#endif

namespace UI::Widget {

namespace {

void stripContainerStyle(lv_obj_t *obj) {
  lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN);
}

std::string capitalizeMode(const std::string &m) {
  if (m.empty())
    return "—";
  std::string s = m;
  for (char &c : s) {
    if (c == '_')
      c = ' ';
  }
  if (!s.empty() && s[0] >= 'a' && s[0] <= 'z')
    s[0] = static_cast<char>(toupper(s[0]));
  return s;
}

uint16_t tempToArcAngle(float temp, float minT, float maxT) {
  constexpr uint16_t rot = 135;
  constexpr uint16_t span = 270;
  if (maxT <= minT)
    return rot;
  float t = (temp - minT) / (maxT - minT);
  if (t < 0)
    t = 0;
  if (t > 1)
    t = 1;
  return rot + static_cast<uint16_t>(t * span);
}

void positionArcTick(lv_obj_t *tick, float temp, float minT, float maxT) {
  if (!tick)
    return;
  const int cx = SCREEN_WIDTH / 2;
  const int cy = 6 + 95;
  const int r = 89;
  if (maxT <= minT)
    return;
  float t = (temp - minT) / (maxT - minT);
  if (t < 0)
    t = 0;
  if (t > 1)
    t = 1;
  const float angleDeg = 135.f + t * 270.f;
  const float rad = angleDeg * 3.14159265f / 180.f;
  const int x = cx + static_cast<int>(r * cosf(rad)) - 7;
  const int y = cy + static_cast<int>(r * sinf(rad)) - 7;
  lv_obj_set_pos(tick, x, y);
}

lv_obj_t *makeArcTick(lv_obj_t *panel, lv_color_t color) {
  lv_obj_t *tick = lv_obj_create(panel);
  lv_obj_set_size(tick, 14, 14);
  lv_obj_set_style_radius(tick, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(tick, color, 0);
  lv_obj_set_style_border_width(tick, 2, 0);
  lv_obj_set_style_border_color(tick, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_pad_all(tick, 0, 0);
  lv_obj_remove_flag(tick, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(tick, LV_OBJ_FLAG_GESTURE_BUBBLE);
  return tick;
}

} // namespace

struct HaClimatePanel::Ui {
  lv_obj_t *modeLbl = nullptr;
  lv_obj_t *lowLbl = nullptr;
  lv_obj_t *highLbl = nullptr;
  lv_obj_t *roomLbl = nullptr;
  lv_obj_t *heatArc = nullptr;
  lv_obj_t *coolArc = nullptr;
  lv_obj_t *heatTick = nullptr;
  lv_obj_t *coolTick = nullptr;
  lv_obj_t *roomTick = nullptr;
  lv_obj_t *modeMenu = nullptr;
  bool menuOpen = false;
};

HaClimatePanel::~HaClimatePanel() { delete mUi; }

HaClimatePanel::HaClimatePanel(lv_obj_t *parent, const std::string &entityId)
    : UIElement(lv_obj_create(parent), ID()), mEntityId(entityId) {
  auto lock = LvglResourceManager::GetInstance().scopeLock();
  mUi = new Ui();
  lv_obj_set_style_bg_color(LvglSelf(), lv_color_hex(0x2a1a3a), LV_PART_MAIN);
  stripContainerStyle(LvglSelf());
  lv_obj_remove_flag(LvglSelf(), LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(LvglSelf(), LV_OBJ_FLAG_GESTURE_BUBBLE);
  buildUi();
  refreshFromCache();
  requestFetchIfNeeded();
}

void HaClimatePanel::buildUi() {
  auto lock = LvglResourceManager::GetInstance().scopeLock();
  lv_obj_t *panel = LvglSelf();

  lv_obj_t *arc = lv_arc_create(panel);
  lv_obj_set_size(arc, 190, 190);
  lv_obj_align(arc, LV_ALIGN_TOP_MID, 0, 6);
  lv_arc_set_rotation(arc, 0);
  lv_arc_set_bg_angles(arc, 135, 405);
  lv_arc_set_range(arc, 0, 100);
  lv_arc_set_angles(arc, 135, 270);
  lv_obj_set_style_arc_width(arc, 12, LV_PART_MAIN);
  lv_obj_set_style_arc_width(arc, 0, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(arc, lv_color_hex(0x443322), LV_PART_MAIN);
  lv_obj_remove_style(arc, nullptr, LV_PART_KNOB);
  lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);

  auto mkIndicatorArc = [&](lv_color_t color) {
    lv_obj_t *a = lv_arc_create(panel);
    lv_obj_set_size(a, 190, 190);
    lv_obj_align(a, LV_ALIGN_TOP_MID, 0, 6);
    lv_arc_set_rotation(a, 0);
    lv_arc_set_bg_angles(a, 135, 405);
    lv_arc_set_range(a, 0, 100);
    lv_arc_set_angles(a, 135, 135);
    lv_obj_set_style_arc_width(a, 0, LV_PART_MAIN);
    lv_obj_set_style_arc_width(a, 12, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(a, color, LV_PART_INDICATOR);
    lv_obj_remove_style(a, nullptr, LV_PART_KNOB);
    lv_obj_remove_flag(a, LV_OBJ_FLAG_CLICKABLE);
    return a;
  };

  mUi->heatArc = mkIndicatorArc(lv_color_hex(0xff8844));
  mUi->coolArc = mkIndicatorArc(lv_color_hex(0x66aaff));
  mUi->heatTick = makeArcTick(panel, lv_color_hex(0xff8844));
  mUi->coolTick = makeArcTick(panel, lv_color_hex(0x4488ff));
  mUi->roomTick = makeArcTick(panel, lv_color_hex(0xe6e6e6));
  lv_obj_move_foreground(mUi->heatArc);
  lv_obj_move_foreground(mUi->coolArc);
  lv_obj_move_foreground(mUi->heatTick);
  lv_obj_move_foreground(mUi->coolTick);
  lv_obj_move_foreground(mUi->roomTick);

  mUi->modeLbl = lv_label_create(panel);
  lv_label_set_text(mUi->modeLbl, "…");
  lv_obj_align(mUi->modeLbl, LV_ALIGN_TOP_MID, 0, 68);
  lv_obj_set_style_text_font(mUi->modeLbl, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(mUi->modeLbl, lv_color_hex(0xdddddd), 0);
  lv_obj_add_flag(mUi->modeLbl, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(mUi->modeLbl, onModeLabelClick, LV_EVENT_CLICKED, this);

  mUi->lowLbl = lv_label_create(panel);
  mUi->highLbl = lv_label_create(panel);
  lv_label_set_text(mUi->lowLbl, "--°");
  lv_label_set_text(mUi->highLbl, "--°");
  lv_obj_set_style_text_font(mUi->lowLbl, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_font(mUi->highLbl, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(mUi->lowLbl, lv_color_hex(0xffaa66), 0);
  lv_obj_set_style_text_color(mUi->highLbl, lv_color_hex(0x66aaff), 0);
  lv_obj_align(mUi->lowLbl, LV_ALIGN_TOP_MID, -54, 92);
  lv_obj_align(mUi->highLbl, LV_ALIGN_TOP_MID, 54, 92);

  mUi->roomLbl = lv_label_create(panel);
  lv_label_set_text(mUi->roomLbl, "Room --°");
  lv_obj_align(mUi->roomLbl, LV_ALIGN_TOP_MID, 0, 118);
  lv_obj_set_style_text_color(mUi->roomLbl, lv_color_hex(0xbbbbbb), 0);
  lv_obj_set_style_text_font(mUi->roomLbl, &lv_font_montserrat_12, 0);

  auto mkRoundBtn = [&](const char *sym, int xofs, int which) {
    lv_obj_t *b = lv_btn_create(panel);
    lv_obj_set_size(b, 40, 40);
    lv_obj_align(b, LV_ALIGN_BOTTOM_MID, xofs, -10);
    lv_obj_set_style_radius(b, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(b, 2, 0);
    lv_obj_set_style_border_color(b, lv_color_hex(0xff8844), 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(0x3a2848), 0);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, sym);
    lv_obj_center(l);
    lv_obj_add_event_cb(b, onAdjustBtn, LV_EVENT_CLICKED, this);
    lv_obj_set_user_data(b, reinterpret_cast<void *>(static_cast<intptr_t>(which)));
    lv_obj_add_flag(b, LV_OBJ_FLAG_GESTURE_BUBBLE);
  };
  mkRoundBtn("-", -88, 0);
  mkRoundBtn("+", -44, 1);
  mkRoundBtn("-", 44, 2);
  mkRoundBtn("+", 88, 3);

  mUi->modeMenu = lv_obj_create(panel);
  lv_obj_set_size(mUi->modeMenu, SCREEN_WIDTH - 20, 36);
  lv_obj_align(mUi->modeMenu, LV_ALIGN_BOTTOM_MID, 0, -58);
  lv_obj_set_style_bg_color(mUi->modeMenu, lv_color_hex(0x1a1228), 0);
  lv_obj_set_style_border_color(mUi->modeMenu, lv_color_hex(0x555555), 0);
  lv_obj_set_style_border_width(mUi->modeMenu, 1, 0);
  lv_obj_set_style_radius(mUi->modeMenu, 8, 0);
  lv_obj_set_flex_flow(mUi->modeMenu, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(mUi->modeMenu, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  stripContainerStyle(mUi->modeMenu);
  lv_obj_add_flag(mUi->modeMenu, LV_OBJ_FLAG_HIDDEN);

  const char *labels[] = {"Off", "Heat", "Cool", "Auto"};
  static const char *modes[] = {"off", "heat", "cool", "auto"};
  for (int m = 0; m < 4; m++) {
    lv_obj_t *b = lv_btn_create(mUi->modeMenu);
    lv_obj_set_size(b, 50, 28);
    lv_obj_t *l = lv_label_create(b);
    lv_label_set_text(l, labels[m]);
    lv_obj_center(l);
    lv_obj_set_user_data(b, reinterpret_cast<void *>(static_cast<intptr_t>(m)));
    lv_obj_add_event_cb(b, onModeBtn, LV_EVENT_CLICKED, this);
    lv_obj_add_flag(b, LV_OBJ_FLAG_GESTURE_BUBBLE);
    (void)modes;
  }
}

void HaClimatePanel::closeModeMenu() {
  if (!mUi || !mUi->modeMenu)
    return;
  auto lock = LvglResourceManager::GetInstance().scopeLock();
  lv_obj_add_flag(mUi->modeMenu, LV_OBJ_FLAG_HIDDEN);
  mUi->menuOpen = false;
}

void HaClimatePanel::showStatus(const char *msg) {
  if (mUi && mUi->modeLbl) {
    auto lock = LvglResourceManager::GetInstance().scopeLock();
    lv_label_set_text(mUi->modeLbl, msg);
  }
}

void HaClimatePanel::applyAttrs(const HaClimate::Attrs &ca) {
  if (!mUi)
    return;
  auto lock = LvglResourceManager::GetInstance().scopeLock();
  mLastAttrs = ca;
  mHasAttrs = true;

  char buf[24];
  if (mUi->modeLbl)
    lv_label_set_text(mUi->modeLbl, capitalizeMode(ca.mode).c_str());
  if (mUi->lowLbl) {
    snprintf(buf, sizeof(buf), "%.0f°", ca.low);
    lv_label_set_text(mUi->lowLbl, buf);
  }
  if (mUi->highLbl) {
    snprintf(buf, sizeof(buf), "%.0f°", ca.high);
    lv_label_set_text(mUi->highLbl, buf);
  }
  if (mUi->roomLbl) {
    snprintf(buf, sizeof(buf), "Room %.0f°", ca.current);
    lv_label_set_text(mUi->roomLbl, buf);
  }

  if (ca.hasRange) {
    uint16_t aLow = tempToArcAngle(ca.low, ca.minT, ca.maxT);
    uint16_t aHigh = tempToArcAngle(ca.high, ca.minT, ca.maxT);
    if (aHigh < aLow) {
      const uint16_t t = aLow;
      aLow = aHigh;
      aHigh = t;
    }
    if (mUi->heatArc) {
      lv_obj_remove_flag(mUi->heatArc, LV_OBJ_FLAG_HIDDEN);
      lv_arc_set_angles(mUi->heatArc, 135, aLow);
    }
    if (mUi->coolArc) {
      lv_obj_remove_flag(mUi->coolArc, LV_OBJ_FLAG_HIDDEN);
      lv_arc_set_angles(mUi->coolArc, aHigh, 405);
    }
    if (mUi->heatTick) {
      lv_obj_remove_flag(mUi->heatTick, LV_OBJ_FLAG_HIDDEN);
      positionArcTick(mUi->heatTick, ca.low, ca.minT, ca.maxT);
    }
    if (mUi->coolTick) {
      lv_obj_remove_flag(mUi->coolTick, LV_OBJ_FLAG_HIDDEN);
      positionArcTick(mUi->coolTick, ca.high, ca.minT, ca.maxT);
    }
  } else {
    const float t = ca.target > 0 ? ca.target : ca.current;
    const uint16_t a = tempToArcAngle(t, ca.minT, ca.maxT);
    if (mUi->heatArc) {
      lv_obj_remove_flag(mUi->heatArc, LV_OBJ_FLAG_HIDDEN);
      lv_arc_set_angles(mUi->heatArc, 135, a);
    }
    if (mUi->coolArc)
      lv_obj_add_flag(mUi->coolArc, LV_OBJ_FLAG_HIDDEN);
    if (mUi->heatTick)
      lv_obj_add_flag(mUi->heatTick, LV_OBJ_FLAG_HIDDEN);
    if (mUi->coolTick)
      lv_obj_add_flag(mUi->coolTick, LV_OBJ_FLAG_HIDDEN);
  }

  if (mUi->roomTick) {
    if (ca.current > 0) {
      lv_obj_remove_flag(mUi->roomTick, LV_OBJ_FLAG_HIDDEN);
      positionArcTick(mUi->roomTick, ca.current, ca.minT, ca.maxT);
    } else {
      lv_obj_add_flag(mUi->roomTick, LV_OBJ_FLAG_HIDDEN);
    }
  }
}

void HaClimatePanel::refreshFromCache() {
  if (mEntityId.empty())
    return;
  std::string state;
  std::string attrsJson;
  if (!HaRuntime::getCachedState(mEntityId, state))
    return;
  if (!HaRuntime::getCachedAttributes(mEntityId, attrsJson))
    return;
  HaClimate::Attrs ca;
  if (!HaClimate::parseClimateAttrsJson(attrsJson, state, ca))
    return;
  applyAttrs(ca);
}

void HaClimatePanel::requestFetchIfNeeded() {
  if (mEntityId.empty())
    return;
  std::string attrs;
  if (!HaRuntime::getCachedAttributes(mEntityId, attrs))
    HaRuntime::fetchEntityState(mEntityId);
}

void HaClimatePanel::callSetTemperature(float target) {
  rapidjson::Document doc;
  doc.SetObject();
  auto &a = doc.GetAllocator();
  doc.AddMember("temperature", rapidjson::Value(target), a);
  HaRuntime::callServiceWithData("climate", "set_temperature", mEntityId, OMOTE::JSON::ToString(doc));
  if (mHasAttrs) {
    mLastAttrs.target = target;
    if (!mLastAttrs.hasRange) {
      mLastAttrs.low = target;
      mLastAttrs.high = target;
    }
    applyAttrs(mLastAttrs);
  }
}

void HaClimatePanel::callSetRange(float low, float high) {
  rapidjson::Document doc;
  doc.SetObject();
  auto &a = doc.GetAllocator();
  doc.AddMember("target_temp_low", rapidjson::Value(low), a);
  doc.AddMember("target_temp_high", rapidjson::Value(high), a);
  HaRuntime::callServiceWithData("climate", "set_temperature", mEntityId, OMOTE::JSON::ToString(doc));
  if (mHasAttrs) {
    mLastAttrs.low = low;
    mLastAttrs.high = high;
    mLastAttrs.hasRange = true;
    applyAttrs(mLastAttrs);
  }
}

void HaClimatePanel::callSetMode(const char *mode) {
  rapidjson::Document doc;
  doc.SetObject();
  auto &a = doc.GetAllocator();
  doc.AddMember("hvac_mode", rapidjson::Value(mode, a), a);
  HaRuntime::callServiceWithData("climate", "set_hvac_mode", mEntityId, OMOTE::JSON::ToString(doc));
  if (mHasAttrs) {
    mLastAttrs.mode = mode;
    applyAttrs(mLastAttrs);
  }
}

void HaClimatePanel::onAdjust(int which) {
  if (!mHasAttrs) {
    showStatus("Syncing");
    requestFetchIfNeeded();
    return;
  }
  HaClimate::Attrs ca = mLastAttrs;
  if (ca.hasRange) {
    float low = ca.low;
    float high = ca.high;
    if (which == 0)
      low -= 1.f;
    else if (which == 1)
      low += 1.f;
    else if (which == 2)
      high -= 1.f;
    else
      high += 1.f;
    if (low < ca.minT)
      low = ca.minT;
    if (high > ca.maxT)
      high = ca.maxT;
    if (low > high - 1.f)
      low = high - 1.f;
    if (high < low + 1.f)
      high = low + 1.f;
    callSetRange(low, high);
    return;
  }
  float t = ca.target > 0 ? ca.target : ca.current;
  if (which == 0 || which == 2)
    t -= 1.f;
  else
    t += 1.f;
  if (t < ca.minT)
    t = ca.minT;
  if (t > ca.maxT)
    t = ca.maxT;
  callSetTemperature(t);
}

void HaClimatePanel::onModeLabelClick(lv_event_t *e) {
  auto *self = static_cast<HaClimatePanel *>(lv_event_get_user_data(e));
  if (!self || !self->mUi)
    return;
  auto lock = LvglResourceManager::GetInstance().scopeLock();
  if (self->mUi->menuOpen)
    self->closeModeMenu();
  else {
    lv_obj_remove_flag(self->mUi->modeMenu, LV_OBJ_FLAG_HIDDEN);
    self->mUi->menuOpen = true;
  }
}

void HaClimatePanel::onAdjustBtn(lv_event_t *e) {
  auto *self = static_cast<HaClimatePanel *>(lv_event_get_user_data(e));
  if (!self)
    return;
  lv_obj_t *btn = static_cast<lv_obj_t *>(lv_event_get_target(e));
  const int which = static_cast<int>(reinterpret_cast<intptr_t>(lv_obj_get_user_data(btn)));
  self->onAdjust(which);
}

void HaClimatePanel::onModeBtn(lv_event_t *e) {
  auto *self = static_cast<HaClimatePanel *>(lv_event_get_user_data(e));
  if (!self)
    return;
  lv_obj_t *btn = static_cast<lv_obj_t *>(lv_event_get_target(e));
  const int modeIdx = static_cast<int>(reinterpret_cast<intptr_t>(lv_obj_get_user_data(btn)));
  static const char *modes[] = {"off", "heat", "cool", "auto"};
  if (modeIdx >= 0 && modeIdx <= 3)
    self->callSetMode(modes[modeIdx]);
  self->closeModeMenu();
}

} // namespace UI::Widget
