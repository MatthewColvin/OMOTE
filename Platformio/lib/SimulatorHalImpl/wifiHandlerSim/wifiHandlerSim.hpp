#pragma once
#include <atomic>
#include <memory>
#include <thread>

#include "Hardware/wifiHandlerInterface.h"
#include "Notification.hpp"

class wifiHandlerSim : public wifiHandlerInterface {
public:
  wifiHandlerSim();
  ~wifiHandlerSim() override = default;

  void begin() override;
  void scan() override;
  void connect(std::string ssid, std::string password) override;
  void saveCredentialsOnConnect() override {};
  wifiStatus GetStatus() override { return mCurrentStatus; };

  void setupMqttBroker() override;
  void mqttSend(std::string aTopic, std::string aMessage) override;
  void mqttSync() override;
  void mqttForceReconnect() override {};
  void mqttSetBroker(std::string broker) override { mMqttBroker = broker; };
  void mqttSetPort(std::string port) override { mMqttPort = port; };
  void mqttSetUser(std::string user) override { mMqttUser = user; };
  void mqttSetPassword(std::string pword) override { mMqttPassword = pword; };
  void mqttSetClientID(std::string clientid) override { mMqttClientName = clientid; };
  std::string mqttGetBroker() override { return mMqttBroker; };
  std::string mqttGetPort() override { return mMqttPort; };
  std::string mqttGetUser() override { return mMqttUser; };
  std::string mqttGetPassword() override { return mMqttPassword; };
  std::string mqttGetClientID() override { return mMqttClientName; };
  void mqttSaveCredentialsOnConnect() override { mMqttSaveOnConnect = true; };
  void mqttBindTextEvent(uint32_t bindId, std::string topic, std::string field) override;
  void mqttUnBindTextEvent(uint32_t unBindId) override;
  void mqttSaveCredentials();
  void enableMqtt(bool enabled) override { mMqttEnabled = enabled; };
  bool isMqttEnabled(void) override { return mMqttEnabled; };

  void enableFtp(bool enabled) override { mFtpEnabled = enabled; };
  bool isFtpEnabled(void) override { return mFtpEnabled; };
  void ftpSetUser(std::string user) override { mFtpUser = user; };
  void ftpSetPassword(std::string password) override { mFtpPassword = password; };
  void mDNSSetName(std::string name) override { mmDNSName = name; };
  std::string ftpGetUser() override { return mFtpUser; };
  std::string ftpGetPassword() override { return mFtpPassword; };
  std::string mDNSGetName() override { return mmDNSName; };
  void ftpSaveCredentials() override {};

  bool mMqttConnected = false;

  void enableNtp(bool enabled) override { mNtpEnabled = enabled; };
  bool isNtpEnabled(void) override { return mNtpEnabled; };
  std::string ntpGetServer() override { return mNtpServer; };
  std::string ntpGetTimeZone() override { return mNtpTimeZone; };
  int ntpGetDisplayMode() override { return mNtpDisplayMode; };
  void ntpSetServer(std::string server) override { mNtpServer = server; };
  void ntpSetTimeZone(std::string timezone) override { mNtpTimeZone = timezone; };
  void ntpSetDisplayMode(int mode) override { mNtpDisplayMode = mode; };
  void ntpSaveCredentials() override {};
  void setupNtp() override {};

private:
  void init_mqtt();
  void stop_mqtt();
  void restoreCredentials();

  std::thread mFakeScanThread = std::thread([] {});
  std::thread mFakeStatusThread = std::thread([] {});
  wifiStatus mCurrentStatus = wifiStatus(true, "172.0.0.1", "FakeNet");

  std::string mMqttBroker = "broker";
  std::string mMqttPort = "port";
  std::string mMqttUser = "user";
  std::string mMqttPassword = "password";
  std::string mMqttClientName = "OMOTESIM";
  bool mMqttSaveOnConnect = false;
  bool mMqttEnabled = false;

  bool mNtpEnabled = false;
  std::string mNtpServer = "pool.ntp.org";
  std::string mNtpTimeZone = "GMT0BST,M3.5.0/1,M10.5.0";
  int mNtpDisplayMode = ntpDisplayMode::constant;

  std::string mFtpUser = "OMOTE";
  std::string mFtpPassword = "OMOTE";
  std::string mmDNSName = "omote";
  bool mFtpEnabled = false;
};