#include "WidgetBase.hpp"

namespace UI::Widget {
class Bar : public Base {
public:
  Bar(int32_t aMinVal = 0, int32_t aMaxVal = 100);

  int32_t GetValue();
  void SetValue(int32_t aValue, lv_anim_enable_t aIsAnimate = LV_ANIM_ON);

private:
  std::function<void(int32_t)> mOnSliderChange;
  bool mOnlyProccessOnRelease = false;
};

} // namespace UI::Widget
