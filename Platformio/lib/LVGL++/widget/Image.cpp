#include "Image.hpp"

#include "BackgroundScreen.hpp"

using namespace UI::Widget;

Image::Image(const char *aSymbol)
    : Base(lv_image_create(UI::Screen::BackgroundScreen::getLvInstance()),
           ID::Widgets::Image) {
  lv_image_set_src(LvglSelf(), aSymbol);
}

void Image::SetRotation(int16_t aAngle) {
  lv_image_set_rotation(LvglSelf(), aAngle);
}

void Image::SetZoom(uint16_t aZoom) { lv_image_set_scale(LvglSelf(), aZoom); }

void Image::SetAntialias(bool aEnable) {
  lv_image_set_antialias(LvglSelf(), aEnable);
}

void Image::SetOffset(lv_coord_t aX, lv_coord_t aY) {
  lv_image_set_offset_x(LvglSelf(), aX);
  lv_image_set_offset_y(LvglSelf(), aY);
}
