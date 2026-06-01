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

  void OnShow() override;
  void OnHide() override;

  /** Called by HaRuntime when entity state cache updates. */
  void applyHaStates();

  void getKeyOverrides(const rapidjson::Value &value, std::multimap<Command::KeyIds, Command::KeyStruct> &aKeyHandlers);

private:
  struct HaBinding {
    std::string entityId;
    Widget::Button *toggle = nullptr;
    Widget::Label *stateLabel = nullptr;
  };

  void addTitle(const std::string &aCommandPrefix, const rapidjson::Value &value, std::string aPageName);
  void addLabel(const std::string &aCommandPrefix, const rapidjson::Value &value);
  void addButton(const std::string &aCommandPrefix, const rapidjson::Value &value);
  void addImage(const rapidjson::Value &value);
  void addColorButtons(const std::string &aCommandPrefix, const rapidjson::Value &value);
  void addNumberPad(const std::string &aCommandPrefix, const rapidjson::Value &value);
  void addHaToggle(const rapidjson::Value &value);
  void addHaLabel(const rapidjson::Value &value);
  void applyWidgetLayout(UIElement *widget, const rapidjson::Value &value, unsigned int defaultHeightPct = 0);

  std::vector<UIElement *> mWidgets;
  std::vector<std::unique_ptr<char[]>> mFormatStrings;
  std::vector<uint32_t> mSubscriptions;
  std::vector<std::string> mHaEntityIds;
  std::vector<HaBinding> mHaBindings;
  static constexpr auto distBetweenWidgets = 3;
  std::string mCommandFile;
  std::multimap<Command::KeyIds, Command::KeyStruct> mKeyHandlers;
};

} // namespace UI::Page
