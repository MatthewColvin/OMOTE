#pragma once
#include "Hardware/wifiHandlerInterface.h"
#include "PageBase.hpp"
namespace UI::Widget {
class Label;
class Button;
class Image;
} // namespace UI::Widget

namespace UI::Page {

typedef enum {
  NONE = 0,
  MQTT_SUB,
  MQTT_PUB,
  IR
} ActionProtocols;

class JsonPage : public Base {

public:
  JsonPage(std::string aFileName, std::string aPageName, std::string aCommandPrefix);
  virtual ~JsonPage();

  bool OnKeyEvent(KeyPressAbstract::KeyEvent aKeyEvent) override;

private:
  ActionProtocols getAction(std::string aActionsFIle, std::string aAction, std::vector<std::string> &actionStrings);

  std::vector<UIElement *> mWidgets;
  std::vector<std::unique_ptr<char[]>> mFormatStrings;
  // std::vector<std::string[2]> mActionStrings;
  std::vector<uint32_t> mSubscriptions;
  static constexpr auto distBetweenWidgets = 5;
  std::string mActionsFile;
};

} // namespace UI::Page
