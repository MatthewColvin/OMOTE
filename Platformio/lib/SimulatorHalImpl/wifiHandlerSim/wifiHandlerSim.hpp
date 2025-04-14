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
  void mqttSetBroker(std::string broker) override { mMqttBroker = broker; };
  void mqttSetPort(std::string port) override { mMqttPort = port; };
  void mqttSetUser(std::string user) override { mMqttUser = user; };
  void mqttSetPassword(std::string pword) override { mMqttPassword = pword; };
  std::string mqttGetBroker() override { return mMqttBroker; };
  std::string mqttGetPort() override { return mMqttPort; };
  std::string mqttGetUser() override { return mMqttUser; };
  std::string mqttGetPassword() override { return mMqttPassword; };
  void mqttSaveCredentialsOnConnect() override { mMqttSaveOnConnect = true; };
  void mqttBindTextEvent(uint32_t bindId, std::string topic, std::string field) override;
  void mqttUnBindTextEvent(uint32_t unBindId) override;
  void mqttSaveCredentials();
  virtual void enableMqtt(bool enabled) override { mMqttEnabled = enabled; };
  virtual bool isMqttEnabled(void) override { return mMqttEnabled; };

  bool mMqttConnected = false;

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
  std::string mMqttClientName = "OMOTE";
  bool mMqttSaveOnConnect = false;
  bool mMqttEnabled = false;
};