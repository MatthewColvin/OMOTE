#include "ActiveDeviceList.hpp"
#include "Button.hpp"

namespace UI::Page {

ActiveDeviceList::ActiveDeviceList(DeviceFactory &aFactory)
    : Base(ID::Pages::ActiveDeviceList),
      mFactory(aFactory),
      mDeviceList(nullptr) {
  RefreshDeviceList();
  mUpdateHandler.SetNotification(mFactory.getActiveDevices().getListUpdateNotification());
  mUpdateHandler = [this](auto event) { RefreshDeviceList(); };

  BuildReorderControlsUI();
}

void ActiveDeviceList::SetHeight(lv_coord_t aHeight) {
  Base::SetHeight(aHeight);
  if (mDeviceList) {
    mDeviceList->SetHeight(GetContentHeight());
  }
  if (mReorderControls) {
    mReorderControls->SetHeight(GetContentHeight());
  }
}

void ActiveDeviceList::RefreshDeviceList() {
  if (mDeviceList) {
    RemoveElement(mDeviceList);
    mDeviceListItemToDevice.clear();
    mSelectedItem = nullptr;
  }
  static constexpr auto MaxSelectedDevices = 1;
  mDeviceList = AddNewElement<UI::Widget::SelectedItemsList>(MaxSelectedDevices);
  mDeviceList->SetWidth(GetContentWidth() * 0.8);

  for (auto device : mFactory.getActiveDevices().getDevices()) {
    auto *listItem = mDeviceList->AddItem(
        device->GetName(),
        nullptr, // no symbol
        [] {}    // empty callback since we're just displaying devices
    );
    mDeviceListItemToDevice[listItem] = device;
  }
  mDeviceList->OnSelectionChanged([this](const auto &, const auto &aSelectedItems) {
    mSelectedItem = aSelectedItems.size() > 0 ? aSelectedItems.back() : nullptr;
  });
}

void ActiveDeviceList::BuildReorderControlsUI() {
  mReorderControls = AddNewElement<UI::Widget::Base>(UI::ID::Widgets::INVALID_WIDGET_ID);
  mReorderControls->SetHeight(mDeviceList->GetHeight());
  mReorderControls->SetWidth(GetContentWidth() - mDeviceList->GetWidth());
  mReorderControls->AlignTo(mDeviceList, LV_ALIGN_OUT_RIGHT_MID);

  lv_obj_set_flex_flow(mReorderControls->LvglSelf(), LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(mReorderControls->LvglSelf(), LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  auto *topButton = mReorderControls->AddNewElement<UI::Widget::Button>();
  auto *upButton = mReorderControls->AddNewElement<UI::Widget::Button>();
  auto *downButton = mReorderControls->AddNewElement<UI::Widget::Button>();
  auto *bottomButton = mReorderControls->AddNewElement<UI::Widget::Button>();
  auto *deleteButton = mReorderControls->AddNewElement<UI::Widget::Button>();

  for (auto *button : {upButton, downButton, topButton, bottomButton, deleteButton}) {
    button->SetHeight(30);
  }

  topButton->SetText("Top");
  upButton->SetText("Up");
  downButton->SetText("Down");
  bottomButton->SetText("Bottom");
  deleteButton->SetText("Delete");

  topButton->OnShortClick([this]() {
    if (mSelectedItem) {
      mFactory.getActiveDevices().setDevicePriority(mDeviceListItemToDevice[mSelectedItem], 0);
    }
  });
  upButton->OnShortClick([this]() {
    if (mSelectedItem) {
      auto &activeDevices = mFactory.getActiveDevices();
      auto currentPriority = activeDevices.getDevicePriority(mDeviceListItemToDevice[mSelectedItem]);
      activeDevices.setDevicePriority(mDeviceListItemToDevice[mSelectedItem], currentPriority - 1);
    }
  });
  downButton->OnShortClick([this]() {
    if (mSelectedItem) {
      auto &activeDevices = mFactory.getActiveDevices();
      auto currentPriority = activeDevices.getDevicePriority(mDeviceListItemToDevice[mSelectedItem]);
      activeDevices.setDevicePriority(mDeviceListItemToDevice[mSelectedItem], currentPriority + 1);
    }
  });
  bottomButton->OnShortClick([this]() {
    if (mSelectedItem) {
      auto &activeDevices = mFactory.getActiveDevices();
      auto numDevices = activeDevices.getDevices().size();
      activeDevices.setDevicePriority(mDeviceListItemToDevice[mSelectedItem], numDevices - 1);
    }
  });
  deleteButton->OnShortClick([this]() {
    if (mSelectedItem) {
      mFactory.getActiveDevices().removeDevice(mDeviceListItemToDevice[mSelectedItem]->GetName());
    }
  });

  // topButton->AlignTo(mReorderControls, LV_ALIGN_TOP_MID);
  // upButton->AlignTo(topButton, LV_ALIGN_OUT_BOTTOM_MID);
  // downButton->AlignTo(upButton, LV_ALIGN_OUT_BOTTOM_MID);
  // bottomButton->AlignTo(downButton, LV_ALIGN_OUT_BOTTOM_MID);
}

} // namespace UI::Page
