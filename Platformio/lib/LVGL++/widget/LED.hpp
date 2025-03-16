#pragma once
#include "WidgetBase.hpp"

namespace UI::Widget {
class LED : public Base {
public:
    LED();
    virtual ~LED() = default;

    void SetBrightness(uint8_t aBrightness);
    void SetColor(lv_color_t aColor);
    void Toggle();
    void On();
    void Off();
    bool IsOn() const;

private:
    bool mIsOn = false;
};
} // namespace UI::Widget
