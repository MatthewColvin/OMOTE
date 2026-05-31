#include "Custom/RokuHttp.hpp"
#include "ActiveDeviceConfig.hpp"
#include "DeviceFactory.hpp"
#include "HardwareFactory.hpp"
#include "ObjectSchemaBuilder.hpp"
#include <sstream>

using Ids = KeyPressAbstract::KeyId;
std::map<Ids, std::string_view> mKeyIdToRokuKeyName{
    {Ids::Up, "Up"},
    {Ids::Down, "Down"},
    {Ids::Left, "Left"},
    {Ids::Right, "Right"},
    {Ids::Center, "Select"},
    {Ids::Back, "Back"},
    {Ids::Home, "Home"},
    {Ids::Play, "Play"}};

constexpr auto const RokuSchemaArray = OMOTE::JSON::ObjectSchema()
                                           .Require(RokuHttp::IpAddressKey, "string")
                                           .Require(RokuHttp::NameKey, "string")
                                           .Build();

static constexpr std::string_view BuiltRokuSchema(RokuSchemaArray.data(), RokuSchemaArray.size() - 1);

auto RokuConfigSchemaRegistered = ActiveDeviceConfig::ConfigJsonValidator::Register(DeviceId::Roku, BuiltRokuSchema);

auto RokuCreationRegistered = DeviceFactory::RegisterDevice(
    DeviceId::Roku, []() -> IDevice::Ptr {
      static constexpr char defaultName[] = "Roku";
      // TODO: Get IP Address from config or discovery?
      std::string ipAddress = "192.168.86.41";
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

  auto it = mKeyIdToRokuKeyName.find(event.mId);
  if (it != mKeyIdToRokuKeyName.end()) {
    sendKeyPress(std::string(it->second));
    return true;
  }
  return false;
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

  HttpRequest request(HttpRequest::Method::Post, url);
  // TODO Try and add the other headers from postman here
  // request.headers["Content-Type"] = "application/x-www-form-urlencoded";
  request.headers["Expect"] = "";
  request.headers["Content-Type"] = "application/x-www-form-urlencoded";
  request.headers["Accept"] = "*/*";
  request.timeout_ms = 2000;

  mHttpClient->executeAsync(request)
      ->onReturnCode(200, [](const HttpResponse &sSuccess) {});
}

void RokuHttp::launchApp(const std::string &appId) {
  if (!mHttpClient || !mHttpClient->isReady()) {
    return;
  }

  std::string url = buildRokuUrl("/launch/app");
  std::string body = appId;
  mHttpClient->postAsync(url, body)->onReturnCode(200, [](const HttpResponse &sSuccess) {});
}
