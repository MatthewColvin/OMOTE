#pragma once
#include "List.hpp"
#include <deque>

namespace UI::Widget {

class SelectedItemsList : public List {
public:
  SelectedItemsList(size_t aMaxSelectedItems = 1);

  size_t GetSelectedCount() const;
  void SetMaxSelectedItems(size_t aMax);
  size_t GetMaxSelectedItems() const;

  ListItem *AddItem(std::string aTitle, const char *aSymbol,
                    std::function<void()> OnItemHeldCallback = nullptr, lv_coord_t aHeight = lv_pct(20)) override;

  using SelectionChangedCallback = std::function<void(const ListItem &aChangedItem, const std::deque<ListItem *> &aSelectedItems)>;
  SelectedItemsList &OnSelectionChanged(SelectionChangedCallback aCallback);

private:
  size_t mMaxSelectedItems = 1;
  std::deque<ListItem *> mSelectedItems;
  SelectionChangedCallback mSelectionChangedCallback = nullptr;

  lv_style_t mItemSelectedStyle;
};

} // namespace UI::Widget
