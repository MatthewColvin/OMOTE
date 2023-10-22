#pragma once
#include "Notification.hpp"
#include "memory.h"
#include "wifiHandlerInterface.h"
#include <WiFi.h>

class wifiHandler : public wifiHandlerInterface {
public:
  wifiHandler() = default;
  virtual ~wifiHandler();

  // wifiHandlerInterface Implementation
  void begin() override;
  void scan() override;
  void connect(std::string ssid, std::string password) override;
  wifiStatus GetStatus() override { return mCurrentStatus; };
  //

protected:
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
  /// @brief Keep Track of event registered to WiFi Arduino lib
  wifi_event_id_t mCallbackRegistration = -1;

  /**
   * @brief Update Internal status and send out a notification
   */
  void UpdateStatus();
  wifiStatus mCurrentStatus;

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
};