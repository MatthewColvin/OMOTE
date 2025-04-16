#include "WidgetBase.hpp"

namespace UI::Widget {
class Switch : public Base {
public:
  Switch(std::function<void(bool)> OnSwitchValueChange, bool aState = false);

  bool GetValue();
  void SetValue(bool aState = false);
  
protected:
  void OnLvglEvent(lv_event_t *anEvent) override;

private:
  std::function<void(bool)> mOnSwitchChange;
};

} // namespace UI::Widget
