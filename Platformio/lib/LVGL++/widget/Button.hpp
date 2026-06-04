#pragma once
#include <functional>

#include "Label.hpp"
#include "WidgetBase.hpp"

namespace UI::Widget {
class Button : public Base {
public:
  Button();

  Button(std::function<void()> aOnShortClickHandler);

  Button(std::function<void()> aOnPressHandler,
         std::function<void()> aOnReleaseHandler);
  virtual ~Button() = default;

  void SetText(std::string aText);

  void SetHeight(lv_coord_t aHeight) override;
  void SetSize(lv_coord_t aWidth, lv_coord_t aHeight) override;

  // Override in order to pass styling to label
  void SetTextStyle(TextStyle aNewStyle,
                    lv_part_t aStyle = LV_PART_MAIN) override;

  Button &OnPress(std::function<void()> aOnPressHandler);
  Button &OnRelease(std::function<void()> aOnReleaseHandler);
  Button &OnShortClick(std::function<void()> aOnShortClickHandler);
  Button &OnLongHold(std::function<void()> aOnLongClickHandler);

protected:
  void OnLvglEvent(lv_event_t *anEvent) override;

private:
  static void applyLabelReadabilityStyle(lv_obj_t *label);
  void layoutLabelText();

  Label *mText = nullptr;
  std::function<void()> mOnPress = nullptr;
  std::function<void()> mOnRelease = nullptr;
  std::function<void()> mOnShortClick = nullptr;
  std::function<void()> mOnLongHold = nullptr;
};

} // namespace UI::Widget
