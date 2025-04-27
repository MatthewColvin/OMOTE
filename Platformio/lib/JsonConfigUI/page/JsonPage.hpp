#pragma once
#include "Command.hpp"
#include "PageBase.hpp"
#include "RapidJsonUtilty.hpp"

namespace UI::Widget {
class Label;
class Button;
class Image;
class ColorButtons;
} // namespace UI::Widget

namespace UI::Page {

class JsonPage : public Base {

public:
  JsonPage(std::string aFileName, std::string aPageName, std::string aCommandPrefix);
  virtual ~JsonPage();

  bool OnKeyEvent(KeyPressAbstract::KeyEvent aKeyEvent) override;

  void getKeyOverrides(const MemConciousValue &value, std::multimap<Command::KeyIds, Command::KeyStruct> &aKeyHandlers);

private:
  void addLabel(const std::string &aCommandPrefix, const MemConciousValue &value);
  void addButton(const std::string &aCommandPrefix, const MemConciousValue &value);
  void addImage(const MemConciousValue &value);
  void addColorButtons(const std::string &aCommandPrefix, const MemConciousValue &value);
  void addNumberPad(const std::string &aCommandPrefix, const MemConciousValue &value);

  std::vector<UIElement *> mWidgets;
  std::vector<std::unique_ptr<char[]>> mFormatStrings;
  // std::vector<std::string[2]> mActionStrings;
  std::vector<uint32_t> mSubscriptions;
  static constexpr auto distBetweenWidgets = 5;
  std::string mCommandFile;
  std::multimap<Command::KeyIds, Command::KeyStruct> mKeyHandlers;
};

} // namespace UI::Page
