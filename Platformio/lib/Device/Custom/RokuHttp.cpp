#include "Custom/RokuHttp.hpp"
#include "DeviceFactory.hpp"
#include "HardwareFactory.hpp"
#include <sstream>

auto RokuRegistered = DeviceFactory::RegisterDevice(
    DeviceId::Roku, []() -> IDevice::Ptr {
      static constexpr char defaultName[] = "Roku";
      // TODO: Get IP Address from config or discovery?
      std::string ipAddress = "192.168.86.104";
      auto wifi = HardwareFactory().getAbstract().wifi();

      return !wifi ? nullptr : std::make_shared<RokuHttp>(defaultName, ipAddress, *wifi);
    });

RokuHttp::RokuHttp(const std::string &name, const std::string &ipAddress,
                   wifiHandlerInterface &wifiHandler)
    : mName(name), mIpAddress(ipAddress), mWifiHandler(wifiHandler) {
  // Get the HTTP client from the wifi handler
  mHttpClient = mWifiHandler.getHttpClient();
}

std::string RokuHttp::GetName() const {
  return mName;
}

lv_color_t RokuHttp::GetDisplayColor() const {
  // Roku purple color
  return lv_color_make(198, 89, 17);
}

bool RokuHttp::IsDeleteAble() const {
  return true;
}

DeviceType RokuHttp::GetType() const {
  return DeviceType::CompileTime;
}

DeviceId RokuHttp::GetId() const {
  return DeviceId::Roku;
}

rapidjson::Document RokuHttp::GetExtraConfig() const {
  rapidjson::Document extraInfo;
  auto &alloc = extraInfo.GetAllocator();
  extraInfo.SetObject();

  rapidjson::Value name;
  name.SetString(mName.c_str(), alloc);
  extraInfo.AddMember("name", name, alloc);

  rapidjson::Value ipAddress;
  ipAddress.SetString(mIpAddress.c_str(), alloc);
  extraInfo.AddMember("ip_address", ipAddress, alloc);

  return extraInfo;
}

void RokuHttp::SetExtraConfig(const rapidjson::Document &config) {
  if (config.HasMember("name") && config["name"].IsString()) {
    mName = config["name"].GetString();
  }
  if (config.HasMember("ip_address") && config["ip_address"].IsString()) {
    mIpAddress = config["ip_address"].GetString();
  }
}

bool RokuHttp::HandleKeyEvent(KeyPressAbstract::KeyEvent event) {
  // Only handle key presses (not repeats or releases)
  if (!event.isPress()) {
    return false;
  }

  switch (event.mId) {
  case KeyPressAbstract::KeyId::Power:
    power();
    return true;
  case KeyPressAbstract::KeyId::VolUp:
    volumeUp();
    return true;
  case KeyPressAbstract::KeyId::VolDown:
    volumeDown();
    return true;
  default:
    return false;
  }
}

std::unique_ptr<UI::Page::Base> RokuHttp::GetControlPage() {
  // TODO: Implement control page for Roku
  return nullptr;
}

std::string RokuHttp::buildRokuUrl(const std::string &endpoint) const {
  std::stringstream ss;
  ss << "http://" << mIpAddress << ":8060" << endpoint;
  return ss.str();
}

void RokuHttp::sendKeyPress(const std::string &keyName) {
  if (!mHttpClient || !mHttpClient->isReady()) {
    return;
  }

  std::string url = buildRokuUrl("/keypress/" + keyName);
  mHttpClient->postAsync(url, "")->onError([](const HttpResponse &err) {
    // TODO: Handle Error Somehow
    // Handle error silently for now
  });
}

void RokuHttp::launchApp(const std::string &appId) {
  if (!mHttpClient || !mHttpClient->isReady()) {
    return;
  }

  std::string url = buildRokuUrl("/launch/app");
  std::string body = appId;
  mHttpClient->postAsync(url, body)->onError([](const HttpResponse &err) {
    // Handle error silently for now
  });
}

void RokuHttp::power() {
  sendKeyPress("Power");
}

void RokuHttp::volumeUp() {
  sendKeyPress("VolumeUp");
}

void RokuHttp::volumeDown() {
  sendKeyPress("VolumeDown");
}
