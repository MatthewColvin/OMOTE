#pragma once
#include "WidgetBase.hpp"
#include <string>

namespace UI::Widget {

class ListItem : public UIElement {
public:
  ListItem(lv_obj_t *aListItem, std::function<void()> onItemSelected);

  ListItem &OnPress(std::function<void()> aPressHandler);
  ListItem &OnRelease(std::function<void()> aReleaseHandler);
  ListItem &OnClick(std::function<void()> aClickHandler);
  ListItem &OnLongPress(std::function<void()> aLongPressHandler);

  bool IsChecked();
  void Check();
  void UnCheck();
  void ToggleCheck();

  void MoveToIndex(int aIndex);

protected:
  void OnLvglEvent(lv_event_t *anEvent) override;
  bool OnKeyEvent(KeyPressAbstract::KeyEvent anEvent) override {
    return false;
  };

private:
  std::function<void()> mPressHandler = nullptr;
  std::function<void()> mReleaseHandler = nullptr;

  std::function<void()> mClickHandler = nullptr;
  std::function<void()> mLongPressHandler = nullptr;
};

class List : public Base {
public:
  List();
  virtual ListItem *AddItem(std::string aTitle, const char *aSymbol,
                            std::function<void()> onItemSelected, lv_coord_t aHeight = lv_pct(20));

  void ForEachItem(std::function<void(ListItem &)> aFunction);

  /** Remove all list buttons (e.g. before rebuilding from Scenes.json). */
  void ClearItems();

protected:
  ListItem *AddItem(std::unique_ptr<ListItem> aItemToAdd);

private:
  std::vector<UIElement::Ptr> mListItems;
};

} // namespace UI::Widget