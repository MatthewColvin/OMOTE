#pragma once
#include <functional>
#include <memory>
#include <string>

#include "Notification.hpp"

class wifiHandlerInterface {
public:
  wifiHandlerInterface() = default;
  virtual ~wifiHandlerInterface() = default;

  struct WifiInfo {
    WifiInfo() = default;
    WifiInfo(std::string aSsid, int aRssi, bool aConn)
        : ssid(aSsid), rssi(aRssi), isConnected(aConn) {}

    std::string ssid = "";
    int rssi = 0;
    bool isConnected = false;
  };

  struct wifiStatus {
    wifiStatus() = default;
    wifiStatus(bool aConnected, std::string aIp, std::string aSsid)
        : isConnected(aConnected), IP(aIp), ssid(aSsid) {};

    bool isConnected = false;
    std::string IP = "";
    std::string ssid = "";
  };

  typedef std::vector<WifiInfo> ScanDoneDataTy;
  typedef Notification<ScanDoneDataTy> ScanNotificationTy;

  /// @brief Initialize the wifi handler
  virtual void begin() = 0;
  /// @brief Trigger a scan scan for wifi networks
  virtual void scan() = 0;
  /// @brief Attempt a connection to the wifi using the provided credentials
  virtual void connect(std::string ssid, std::string password) = 0;
  /// @brief Store credentials if connection succeeds
  virtual void saveCredentialsOnConnect() = 0;
  /// @brief Get the status of the current wifi connection
  virtual wifiStatus GetStatus() = 0;

  // Register for Scan Notification to handle when scans are completed
  std::shared_ptr<ScanNotificationTy> ScanCompleteNotification() {
    return mScanNotification;
  };

  // Register for Status notifications to handle changes in status
  std::shared_ptr<Notification<wifiStatus>> WifiStatusNotification() {
    return mStatusUpdate;
  };

  virtual void setupMqttBroker() = 0;
  virtual void mqttSend(std::string aTopic, std::string aMessage) = 0;
  virtual void mqttSync() = 0;
  virtual void mqttSetBroker(std::string broker) = 0;
  virtual void mqttSetPort(std::string port) = 0;
  virtual void mqttSetUser(std::string user) = 0;
  virtual void mqttSetPassword(std::string pword) = 0;
  virtual std::string mqttGetBroker() = 0;
  virtual std::string mqttGetPort() = 0;
  virtual std::string mqttGetUser() = 0;
  virtual std::string mqttGetPassword() = 0;
  virtual void mqttSaveCredentialsOnConnect() = 0;
  virtual void mqttBindTextEvent(uint32_t bindId, std::string topic, std::string field) = 0;
  virtual void mqttUnBindTextEvent(uint32_t unBindId) = 0;
  virtual void enableMqtt(bool enabled) = 0;
  virtual bool isMqttEnabled(void) = 0;

protected:
  std::shared_ptr<ScanNotificationTy> mScanNotification =
      std::make_shared<ScanNotificationTy>();
  std::shared_ptr<Notification<wifiStatus>> mStatusUpdate =
      std::make_shared<Notification<wifiStatus>>();
};