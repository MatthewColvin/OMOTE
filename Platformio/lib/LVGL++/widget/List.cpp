#include "List.hpp"
#include "BackgroundScreen.hpp"
#include "LvglResourceManager.hpp"
using namespace UI;
using namespace UI::Widget;

ListItem::ListItem(lv_obj_t *aListItem, std::function<void()> onItemSelected)
    : UIElement(aListItem, ID()), mClickHandler(std::move(onItemSelected)) {}

void ListItem::OnLvglEvent(lv_event_t *anEvent) {
  auto eventCode = lv_event_get_code(anEvent);
  if (eventCode == LV_EVENT_PRESSED && mPressHandler) {
    mPressHandler();
  } else if (eventCode == LV_EVENT_RELEASED && mReleaseHandler) {
    mReleaseHandler();
  } else if (eventCode == LV_EVENT_CLICKED && mClickHandler) {
    mClickHandler();
  } else if (eventCode == LV_EVENT_LONG_PRESSED && mLongPressHandler) {
    mLongPressHandler();
  }
}

ListItem &ListItem::OnPress(std::function<void()> aPressHandler) {
  mPressHandler = std::move(aPressHandler);
  return *this;
}
ListItem &ListItem::OnRelease(std::function<void()> aReleaseHandler) {
  mReleaseHandler = std::move(aReleaseHandler);
  return *this;
}
ListItem &ListItem::OnClick(std::function<void()> aClickHandler) {
  mClickHandler = std::move(aClickHandler);
  return *this;
}
ListItem &ListItem::OnLongPress(std::function<void()> aLongPressHandler) {
  mLongPressHandler = std::move(aLongPressHandler);
  return *this;
}

bool ListItem::IsChecked() {
  return lv_obj_has_state(LvglSelf(), LV_STATE_CHECKED);
}

void ListItem::Check() {
  lv_obj_add_state(LvglSelf(), LV_STATE_CHECKED);
}

void ListItem::UnCheck() {
  lv_obj_remove_state(LvglSelf(), LV_STATE_CHECKED);
}

void ListItem::ToggleCheck() {
  IsChecked() ? UnCheck() : Check();
}

List::List()
    : Base(lv_list_create(Screen::BackgroundScreen::getLvInstance()),
           ID::Widgets::List) {
  StopLvglEventHandler();
}

ListItem *List::AddItem(std::string aTitle, const char *aSymbol,
                        std::function<void()> onItemSelected,
                        lv_coord_t aHeight) {
  lv_obj_t *lvListItem = nullptr;
  {
    auto lock = LvglResourceManager::GetInstance().scopeLock();
    lvListItem = lv_list_add_btn(LvglSelf(), aSymbol, aTitle.c_str());
  }
  auto item = std::make_unique<ListItem>(lvListItem, std::move(onItemSelected));
  item->SetHeight(aHeight);

  return AddItem(std::move(item));
}

ListItem *List::AddItem(std::unique_ptr<ListItem> aListItem) {
  auto *item = aListItem.get();
  mListItems.push_back(std::move(aListItem));
  return item;
}

void List::ForEachItem(std::function<void(ListItem &)> aFunction) {
  if (!aFunction) {
    return;
  }
  for (auto &item : mListItems) {
    aFunction(*static_cast<ListItem *>(item.get()));
  }
}