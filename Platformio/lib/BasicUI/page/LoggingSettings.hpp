#pragma once
#include "DropDown.hpp"
#include "Hardware/LoggingInterface.hpp"
#include "PageBase.hpp"
#include <vector>

namespace UI::Widget {
class Label;
} // namespace UI::Widget

namespace UI::Page {

class LoggingSettings : public Base {
public:
  LoggingSettings();
  std::string GetTitle() override { return "Logging Settings"; }

private:
  void AddRow(LogModule aLogLevel, bool aIsFirst);

  std::vector<Widget::Label *> mModuleLabels;
  std::vector<Widget::DropDown<LogLevel> *> mLogLevelDropDowns;
};

} // namespace UI::Page
