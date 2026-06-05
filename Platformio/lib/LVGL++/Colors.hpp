#pragma once
#include <lvgl.h>

namespace UI::Color {
const auto WHITE = lv_color_white();
const auto BLACK = lv_color_black();
const auto RED = lv_color_make(255, 0, 0);
const auto GREEN = lv_color_make(0, 255, 0);
const auto BLUE = lv_color_make(0, 0, 255);

const auto PURPLE = lv_color_make(128, 0, 128);
const auto LILAC = lv_color_make(231, 209, 255);
const auto YELLOW = lv_color_make(255, 255, 0);

const auto GREY = lv_color_make(105, 105, 105);

/** Default JsonUI button (LVGL dark-theme blue). */
const auto BTN_PRIMARY = lv_color_make(51, 122, 183);
/** HA / active toggle on-state. */
const auto BTN_ACTIVE = lv_color_make(76, 175, 80);

/** Touch buttons on 240×320 — larger than status bar, readable on colored fills. */
inline const lv_font_t *buttonFont() { return &lv_font_montserrat_16; }

const auto HULU_GREEN = lv_color_make(61, 187, 61);
const auto NETFLIX_RED = lv_color_make(216, 31, 38);
const auto DISNEY_BLUE = lv_color_make(1, 20, 124);

} // namespace UI::Color