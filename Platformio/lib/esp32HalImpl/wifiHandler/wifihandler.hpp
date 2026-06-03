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
  void mqttForceReconnect() override;
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
  void enableMqtt(bool enabled) override;
  bool isMqttEnabled(void) override { return mMqttEnabled; };
  void mqttSaveCredentials();
  void mqttRestoreCredentials();

  // TODO: look into moving this to its own interface
  // NTP Interface
  void enableNtp(bool enabled) override { mNtpEnabled = enabled; };
  void nptSync();
  bool isNtpEnabled(void) override { return mNtpEnabled; };
  std::string ntpGetServer() override { return mNtpServer; };
  std::string ntpGetTimeZone() override { return mNtpTimeZone; };
  int ntpGetDisplayMode() override { return mNtpDisplayMode; };
  void ntpSetServer(std::string server) override { mNtpServer = server; };
  void ntpSetTimeZone(std::string timezone) override { mNtpTimeZone = timezone; };
  void ntpSetDisplayMode(int mode) override { mNtpDisplayMode = mode; };
  void ntpSaveCredentials() override;
  void ntpRestoreCredentials();
  void setupNtp() override;

  // FTP interface
  void enableFtp(bool enabled) override {
    mFtpEnabled = enabled;
    if (enabled)
      mFtpForceConnect = true;
  };
  bool isFtpEnabled(void) override { return mFtpEnabled; };
  void ftpSync();
  void ftpSetUser(std::string user) override { mFtpUser = user; };
  void ftpSetPassword(std::string password) override { mFtpPassword = password; };
  void mDNSSetName(std::string name) override { mmDNSName = name; };
  std::string ftpGetUser() override { return mFtpUser; };
  std::string ftpGetPassword() override { return mFtpPassword; };
  std::string mDNSGetName() override { return mmDNSName; };
  void ftpSaveCredentials() override;
  void ftpRestoreCredentials();

  /** HTTP config API + captive portal / connect polling */
  void networkSync() override;
  bool isPortalActive() const override;
  bool hasStoredCredentials() const override;
  const char *portalStatusText() const override;

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
  bool mForceReconnect = false;

  /**
   * @brief NTP variables
   */
  bool mNtpEnabled = false;
  bool mNtpInitialised = false;
  std::string mNtpServer = "pool.ntp.org";
  // see https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
  std::string mNtpTimeZone = "GMT0BST,M3.5.0/1,M10.5.0";
  int mNtpDisplayMode = ntpDisplayMode::constant;

  std::string mFtpUser = "OMOTE";
  std::string mFtpPassword = "OMOTE";
  std::string mmDNSName = "omote";
  bool mFtpEnabled = false;
  bool mFtpInitialised = false;
  unsigned long mOldFtpTime = 0;
  bool mFtpForceConnect = true;

  bool mConnectPending = false;
  uint32_t mConnectStartMs = 0;
  static constexpr uint32_t kConnectTimeoutMs = 30000;
};