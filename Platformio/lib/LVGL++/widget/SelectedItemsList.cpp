#include "SelectedItemsList.hpp"

namespace UI::Widget {

SelectedItemsList::SelectedItemsList(size_t aMaxSelectedItems)
    : List(), mMaxSelectedItems(aMaxSelectedItems) {
  lv_style_init(&mItemSelectedStyle);
  lv_style_set_bg_color(&mItemSelectedStyle, Color::GREY);
}

size_t SelectedItemsList::GetSelectedCount() const {
  return mSelectedItems.size();
}

ListItem *SelectedItemsList::AddItem(std::string aTitle, const char *aSymbol,
                                     std::function<void()> aItemHeldCallback, lv_coord_t aHeight) {
  // Add Item to the base list and then update its behavior to handle being selected
  // and also allow support for long press still
  auto *item = List::AddItem(std::move(aTitle), aSymbol, nullptr, aHeight);
  item->AddStyle(&mItemSelectedStyle, LV_STATE_CHECKED);
  item->OnClick([this, item]() {
        item->ToggleCheck();
        if (item->IsChecked()) {
          if (mSelectedItems.size() >= mMaxSelectedItems) {
            auto &front = mSelectedItems.front();
            front->UnCheck();
            mSelectedItems.pop_front();
          }
          mSelectedItems.push_back(item);
        } else {
          auto it = std::find(mSelectedItems.begin(), mSelectedItems.end(), item);
          if (it != mSelectedItems.end()) {
            mSelectedItems.erase(it);
          }
        }
        // Notify owner that we had a change in selected items if they cared
        if (mSelectionChangedCallback) {
          mSelectionChangedCallback(*item, mSelectedItems);
        }
      })
      .OnLongPress([this, aItemHeldCallback] {
        if (aItemHeldCallback) {
          aItemHeldCallback();
        }
      });
  return item;
}

void SelectedItemsList::SetMaxSelectedItems(size_t aMax) {
  mMaxSelectedItems = aMax;
}

size_t SelectedItemsList::GetMaxSelectedItems() const {
  return mMaxSelectedItems;
}

SelectedItemsList &SelectedItemsList::OnSelectionChanged(SelectionChangedCallback aSelectionChangedCallback) {
  mSelectionChangedCallback = std::move(aSelectionChangedCallback);
  return *this;
}

} // namespace UI::Widget
