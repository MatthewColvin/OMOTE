#pragma once

#include "Hardware/wifi/http/HttpClientInterface.hpp"
#include "Hardware/wifi/wifiHandlerInterface.h"
#include "IDevice.hpp"
#include <memory>
#include <string>

/**
 * @brief Roku device implementation using HTTP API
 *
 * Controls a Roku device via HTTP requests. Requires wifi connectivity
 * and uses the wifiHandlerInterface to obtain an HttpClientInterface.
 */
class RokuHttp : public IDevice {
public:
  /**
   * @brief Constructor
   * @param name Display name for the Roku device
   * @param ipAddress IP address of the Roku device (e.g., "192.168.1.100")
   * @param wifiHandler Reference to the wifi handler to get HTTP client
   */
  RokuHttp(const std::string &name, const std::string &ipAddress,
           wifiHandlerInterface &wifiHandler);

  virtual ~RokuHttp() = default;

  // IDevice interface implementation
  std::string GetName() const override;
  lv_color_t GetDisplayColor() const override;
  bool IsDeleteAble() const override;

  DeviceType GetType() const override;
  DeviceId GetId() const override;

  rapidjson::Document GetExtraConfig() const override;
  void SetExtraConfig(const rapidjson::Document &config) override;

  bool HandleKeyEvent(KeyPressAbstract::KeyEvent event) override;
  std::unique_ptr<UI::Page::Base> GetControlPage() override;

private:
  // Roku control methods
  void sendKeyPress(const std::string &keyName);
  void launchApp(const std::string &appId);

  // Helper to build Roku API URL
  std::string buildRokuUrl(const std::string &endpoint) const;

  // Member variables
  std::string mName;
  std::string mIpAddress;
  wifiHandlerInterface &mWifiHandler;
  std::shared_ptr<HttpClientInterface> mHttpClient;
};
