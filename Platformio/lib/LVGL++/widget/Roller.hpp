#pragma once
#include <functional>
#include <string>
#include <vector>

#include "BackgroundScreen.hpp"
#include "WidgetBase.hpp"

namespace UI::Widget {

template <typename T>
class Roller : public Base {
public:
  Roller(std::function<void(T)> aOnItemSelected)
      : Base(lv_roller_create(UI::Screen::BackgroundScreen::getLvInstance()),
             ID::Widgets::Roller),
        mSelectionHandler(aOnItemSelected) {
    lv_roller_set_options(LvglSelf(), "", LV_ROLLER_MODE_NORMAL);
  }

  void AddItem(std::string aOptionTitle, T aOptionData) {
    if (mOptions.empty()) {
      mOptions = aOptionTitle;
    } else {
      mOptions += "\n" + aOptionTitle;
    }
    mOptionsData.push_back(aOptionData);
    lv_roller_set_options(LvglSelf(), mOptions.c_str(), LV_ROLLER_MODE_NORMAL);
  }

  void SetSelected(T aOptionData) {
    for (size_t i = 0; i < mOptionsData.size(); i++) {
      if (mOptionsData[i] == aOptionData) {
        lv_roller_set_selected(LvglSelf(), i, LV_ANIM_ON);
        break;
      }
    }
  }

  T GetSelectedData() {
    uint16_t idx = lv_roller_get_selected(LvglSelf());
    return mOptionsData[idx];
  }

protected:
  void OnLvglEvent(lv_event_t *anEvent) override {
    if (lv_event_get_code(anEvent) == LV_EVENT_VALUE_CHANGED) {
      uint16_t idx = lv_roller_get_selected(LvglSelf());
      if (mSelectionHandler) {
        mSelectionHandler(mOptionsData[idx]);
      }
    }
  }

private:
  std::function<void(T)> mSelectionHandler;
  std::vector<T> mOptionsData;
  std::string mOptions;
};

} // namespace UI::Widget
