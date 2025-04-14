#include "DisplaySettings.hpp"
#include "HardwareFactory.hpp"
#include "Label.hpp"
#include "Slider.hpp"

using namespace UI::Page;

DisplaySettings::DisplaySettings(std::shared_ptr<DisplayAbstract> aDisplay)
    : Base(UI::ID::Pages::DisplaySettings), mDisplay(aDisplay),
      mLcdDayLabel(AddNewElement<Widget::Label>("LCD Day Brightness")),
      mLcdNightLabel(AddNewElement<Widget::Label>("LCD Night Brightness")),
      mKbdDayLabel(AddNewElement<Widget::Label>("Keypad Day Brightness")),
      mKbdNightLabel(AddNewElement<Widget::Label>("Keypad Night Brightness")),
      mLcdDaySlider(AddNewElement<Widget::Slider>(
          [this](auto aNewBrightness) {
            mDisplay->setLcdDayBrightness(aNewBrightness);
          }, 0, 255)),
      mLcdNightSlider(AddNewElement<Widget::Slider>(
          [this](auto aNewBrightness) {
            mDisplay->setLcdNightBrightness(aNewBrightness);
          },0, 255)),
      mKbdDaySlider(AddNewElement<Widget::Slider>(
          [this](auto aNewBrightness) {
            mDisplay->setKbdDayBrightness(aNewBrightness);
          }, 0, 255)),
      mKbdNightSlider(AddNewElement<Widget::Slider>(
          [this](auto aNewBrightness) {
            mDisplay->setKbdNightBrightness(aNewBrightness);
          }, 0, 255)) {
  SetBgColor(Color::GREY);

  auto labelHeight = 12;
  auto sliderHeight = 55 * 0.60f - labelHeight;
  mLcdDayLabel->SetHeight(labelHeight);
  mLcdDaySlider->SetHeight(sliderHeight);
  mLcdNightLabel->SetHeight(labelHeight);
  mLcdNightSlider->SetHeight(sliderHeight);
  mKbdDayLabel->SetHeight(labelHeight);
  mKbdDaySlider->SetHeight(sliderHeight);
  mKbdNightLabel->SetHeight(labelHeight);
  mKbdNightSlider->SetHeight(sliderHeight);

  mLcdDayLabel->AlignTo(this, LV_ALIGN_TOP_MID);
  mLcdDaySlider->AlignTo(mLcdDayLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, mLcdDayLabel->GetContentHeight());

  mLcdNightLabel->AlignTo(mLcdDaySlider, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
  mLcdNightSlider->AlignTo(mLcdNightLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, mLcdNightLabel->GetContentHeight());

  mKbdDayLabel->AlignTo(mLcdNightSlider, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
  mKbdDaySlider->AlignTo(mKbdDayLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, mKbdDayLabel->GetContentHeight());

  mKbdNightLabel->AlignTo(mKbdDaySlider, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
  mKbdNightSlider->AlignTo(mKbdNightLabel, LV_ALIGN_OUT_BOTTOM_MID, 0, mKbdNightLabel->GetContentHeight());

  mLcdDaySlider->SetValue(mDisplay->getLcdDayBrightness());
  mLcdNightSlider->SetValue(mDisplay->getLcdNightBrightness());
  mKbdDaySlider->SetValue(mDisplay->getKbdDayBrightness());
  mKbdNightSlider->SetValue(mDisplay->getKbdNightBrightness());

  #ifndef OMOTE_KEYBRD_3661
    mLcdNightLabel->SetVisiblity(false);
    mLcdNightSlider->SetVisiblity(false);
    mKbdNightLabel->SetVisiblity(false);
    mKbdNightSlider->SetVisiblity(false);  
  #ifndef OMOTE_HARDWARE_REV5
    mKbdDayLabel->SetVisiblity(false);
    mKbdDaySlider->SetVisiblity(false);
  #endif
  #endif
}

DisplaySettings::~DisplaySettings() {
  HardwareFactory::getAbstract().saveSettings();
}
