#include "LoggingSettings.hpp"
#include "Label.hpp"
#include <magic_enum.hpp>

using namespace UI::Page;

namespace {
static constexpr auto ITEM_HEIGHT = 40;
static constexpr auto LABEL_WIDTH = 100;
static constexpr auto SPACING = 10;
} // namespace

LoggingSettings::LoggingSettings() : Base(ID::Pages::LoggingSettings) {
  bool first = true;
  for (auto module : magic_enum::enum_values<LogModule>()) {
    AddRow(module, first);
    first = false;
  }
}

void LoggingSettings::AddRow(LogModule aModule, bool aIsFirst) {
  // Create label for this module
  auto moduleName = std::string(magic_enum::enum_name(aModule));
  auto label = AddNewElement<Widget::Label>(moduleName);
  label->SetWidth(LABEL_WIDTH);
  label->SetHeight(30);
  if (aIsFirst) {
    label->AlignTo(this, LV_ALIGN_TOP_LEFT, SPACING, SPACING);
  } else {
    label->AlignTo(mModuleLabels.back(), LV_ALIGN_OUT_BOTTOM_MID, 0, SPACING);
  }
  mModuleLabels.push_back(label);

  // Create DropDown with the verbosity levels
  auto dropdown = AddNewElement<Widget::DropDown<LogLevel>>(
      [aModule](LogLevel level) {
        LoggingInterface::setLogLevel(aModule, level);
      });

  for (auto level : magic_enum::enum_values<LogLevel>()) {
    auto levelName = std::string(magic_enum::enum_name(level));
    dropdown->AddItem(levelName, level);
  }

  dropdown->SetWidth(GetContentWidth() - LABEL_WIDTH - (3 * SPACING));
  dropdown->SetHeight(30);
  dropdown->AlignTo(label, LV_ALIGN_OUT_RIGHT_MID, SPACING, 0);
  dropdown->SetSelected(LoggingInterface::getLogLevel(aModule));

  mLogLevelDropDowns.push_back(dropdown);
}