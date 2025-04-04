#pragma once
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClient.h>

#include "Hardware/wifiHandlerInterface.h"
#include "Notification.hpp"
#include "memory.h"

class wifiHandler : public wifiHandlerInterface {
public:
  static std::shared_ptr<wifiHandler> getInstance();

  // wifiHandlerInterface Implementation
  void begin() override;
  void scan() override;
  void connect(std::string ssid, std::string password) override;
  void saveCredentialsOnConnect() override { mIsConnectionAttempt = true; };
  wifiStatus GetStatus() override { return mCurrentStatus; };

  // MQTT Interface
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
  void enableMqtt(bool enabled) override;
  bool isMqttEnabled(void) override {return mMqttEnabled;};
  void mqttSaveCredentials();
  void mqttRestoreCredentials();

protected:
  wifiHandler() = default;
  static std::shared_ptr<wifiHandler> mInstance;

  /**
   * @brief Function to store the credentials when we have had a
   *        successful connection
   */
  void StoreCredentials();

private:
  /**
   * @brief Handler for incoming arduino wifi events
   * @param event - a Wifi event
   */
  void WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t aEventInfo);

  /**
   * @brief Update Internal status and send out a notification
   */
  void UpdateStatus();

  wifiStatus mCurrentStatus;

  WiFiClient mWifiClient;
  PubSubClient mMqttClient;

  /**
   * @brief Variables used to track wifi connection attempts
   */
  bool mIsConnectionAttempt = false;
  std::string mConnectionAttemptPassword;
  std::string mConnectionAttemptSSID;

  /**
   * @brief Verified Working User and Pass to Wifi network
   */
  std::string mPassword;
  std::string mSSID;

  /**
   * @brief MQTT variables
   */
  std::string mMqttBroker = "broker";
  std::string mMqttPort = "port";
  std::string mMqttUser = "user";
  std::string mMqttPassword = "password";
  std::string mMqttClientName = "OMOTE";
  bool mMqttSaveOnConnect = false;
  bool mMqttInitDone = false;
  WiFiClient mEspClient;
  unsigned long mOldTime = 0;
  bool mMqttEnabled = false;
};