#include "DeviceList.hpp"

#include "HardwareFactory.hpp"
#include "HomeAssistDevices/Light.hpp"
#include "List.hpp"
#include "ScreenManager.hpp"
#include "UIElementIds.hpp"
#include "WebSocket/Request.hpp"
#include "WebSocket/RequestBuilder.hpp"
#include "WebSocket/Session/Session.hpp"

namespace {
using Type = UI::Page::DeviceList::EntityType;
static const std::map<const std::string, Type> PrefixToType{
    {"automation", Type::Automation},
    {"binary_sensor", Type::BinarySensor},
    {"light", Type::Light},
    {"sensor", Type::Sensor},
    {"switch", Type::Switch}};
}  // namespace

namespace UI::Page {

DeviceList::DeviceList(HomeAssist::WebSocket::Api& aApi,
                       ActiveDevices& aActiveDevices)
    : Base(ID::Pages::HomeAssistDeviceList),
      mEntityTypeList(AddNewElement<Widget::List>()),
      mLoadingArc(AddNewElement<Widget::Arc>()),
      mApi(aApi),
      mActiveDevices(aActiveDevices),
      mDeviceQueryProcessor(std::make_shared<UI::DevicesQueryProcessor>(
          [this](const auto& aEntity) { StoreEntity(aEntity); })) {
  // Initially hide device list
  mEntityTypeList->SetVisiblity(false);

  mLoadingArc->SetRange(0, 100);
  mLoadingArc->SetWidth(GetContentWidth() / 2);
  mLoadingArc->SetHeight(GetContentHeight() / 4);
  mLoadingArc->AlignTo(this, LV_ALIGN_TOP_MID);
  mEntityTypeList->AlignTo(mEntityTypeList, LV_ALIGN_OUT_BOTTOM_MID);

  mDeviceQueryProcessor->setRequestProcessCompleteCallback(std::bind(
      &DeviceList::HandleDevicesQueryComplete, this, std::placeholders::_1));

  mDeviceQueryProcessor->setPercentCompleteCallback(
      [this](const auto& aPercentComplete) {
        LvglResourceManager::GetInstance().QueueForLater(
            [this, aPercentComplete]() {
              mLoadingArc->SetValue(aPercentComplete);
            });
      });

  {
    using namespace HomeAssist::WebSocket;
    auto request =
        RequestBuilder()
            .SetType(RequestTypes::CONFIG_ENTITY_REGISTRY_LIST_DISPLAY)
            .BuildUnique();

    mApi.AddSession(std::make_unique<Session>(std::move(request), nullptr,
                                              nullptr, mDeviceQueryProcessor));
  }
}

bool DeviceList::OnKeyEvent(KeyPressAbstract::KeyEvent aKeyEvent) {
  return false;
}

void DeviceList::StoreEntity(const std::string& aEntity) {
  constexpr auto lightStr = "light";
  size_t dotPos = aEntity.find('.');
  std::string prefix =
      (dotPos != std::string::npos) ? aEntity.substr(0, dotPos) : aEntity;

  auto knownTypeIt = PrefixToType.find(prefix);
  auto isKnownType = knownTypeIt != PrefixToType.end();
  if (isKnownType) {
    const auto entityType = knownTypeIt->second;
    mEntityMap[entityType].push_back(aEntity);
  } else {
    mEntityMap[EntityType::Other].push_back(aEntity);
  }
}

void DeviceList::HandleDevicesQueryComplete(
    const UI::DevicesQueryProcessor::resultType& aResult) {
  bool ranSuccessfully = aResult;
  LvglResourceManager::GetInstance().QueueForLater([this, ranSuccessfully]() {
    if (ranSuccessfully) {
      mLoadingArc->SetVisiblity(false);
      mEntityTypeList->AlignTo(this, LV_ALIGN_TOP_MID);
      mEntityTypeList->SetVisiblity(true);
      for (auto& [type, list] : mEntityMap) {
        AddEntityTypeListItem(type, list);
      }
    } else {
      // Todo throw an error up?
    }
  });
}

void DeviceList::AddEntityTypeListItem(
    EntityType aEntityType, const std::vector<std::string>& aEntities) const {
  auto typeToPrefixMatch =
      std::find_if(PrefixToType.begin(), PrefixToType.end(),
                   [aEntityType](auto& prefixToEntity) {
                     return aEntityType == prefixToEntity.second;
                   });

  auto addEntityItem = [this](auto* aList, auto aEntityType,
                              const auto& aEntityName) {
    aList->AddItem(
        aEntityName, LV_SYMBOL_HOME, [this, aEntityType, aEntityName]() {
          if (aEntityType == EntityType::Light) {
            mActiveDevices.addDevice(
                std::make_unique<HomeAssist::Device::Light>(aEntityName, mApi));
          }
        });
  };

  auto handleEntityTypeSelected = [&aEntities, aEntityType, addEntityItem,
                                   this]() {
    auto entityListPage =
        std::make_unique<Page::Base>(UI::ID::Pages::HomeAssistDeviceList);
    auto* list = entityListPage->AddNewElement<Widget::List>();

    for (int i = 0; i < aEntities.size() && i < 9; i++) {
      addEntityItem(list, aEntityType, aEntities[i]);
    }
    UI::Screen::Manager::getInstance().pushPopUp(std::move(entityListPage));
  };

  auto foundTypeString = typeToPrefixMatch != PrefixToType.end();
  if (foundTypeString) {
    mEntityTypeList->AddItem(typeToPrefixMatch->first, LV_SYMBOL_HOME,
                             handleEntityTypeSelected);
  }
}

}  // namespace UI::Page
