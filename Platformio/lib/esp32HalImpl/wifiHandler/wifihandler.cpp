#include "wifihandler.hpp"

#define RAPIDJSON_HAS_STDSTRING 1
#include <Arduino.h>
#include <Preferences.h>
#include <rapidjson/document.h>

#include "HardwareAbstract.hpp"
#include "HardwareFactory.hpp"
#include "WiFi.h"
#include "ftp.hpp"
#include "observerHandles.hpp"
#include "omoteconfig.h"

#include "esp_netif_sntp.h"
#include "esp_sntp.h"
#include <ESPmDNS.h>
#include <fstream>

#define MQTT_RETRY 60000

std::shared_ptr<wifiHandler> wifiHandler::mInstance = nullptr;
std::unique_ptr<LoggingInterface> mLogger = nullptr;

std::shared_ptr<wifiHandler> wifiHandler::getInstance() {
  if (mInstance) {
    return mInstance;
  }
  mInstance = std::shared_ptr<wifiHandler>(new wifiHandler());
  mLogger = std::make_unique<LoggingInterface>();
  mLogger->setLogModule(LogModule::WiFi);
  return mInstance;
};

void wifiHandler::WiFiEvent(WiFiEvent_t event, WiFiEventInfo_t aEventInfo) {
  int no_networks = 0;
  switch (event) {
  case ARDUINO_EVENT_WIFI_SCAN_DONE: {
    no_networks = WiFi.scanComplete();
    auto info = std::vector<WifiInfo>(no_networks);
    for (int i = 0; i < no_networks; i++) {
      auto ssid = WiFi.SSID(i).c_str() ? std::string(WiFi.SSID(i).c_str())
                                       : "No SSID";
      bool isConnected = (WiFi.isConnected() && (WiFi.SSID() == WiFi.SSID(i))) ? true : false;
      info[i] = WifiInfo(ssid, WiFi.RSSI(i), isConnected);
    }
    mScanNotification->notify(info);
    if (WiFi.isConnected() == false) {
      WiFi.reconnect();
    }
    break;
  }
  case ARDUINO_EVENT_WIFI_STA_CONNECTED:
    StoreCredentials();
    WiFi.setAutoReconnect(true);
    UpdateStatus();
    break;
  case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
  case ARDUINO_EVENT_WIFI_STA_STOP:
  case ARDUINO_EVENT_WIFI_STA_GOT_IP:
  case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
  case ARDUINO_EVENT_WIFI_STA_LOST_IP:
    UpdateStatus();
    break;
  default:
    break;
  }
  if (WiFi.status() == WL_CONNECT_FAILED) {
    mLogger->error("connection failed.");
    // Serial.println("connection failed.");
    WiFi.disconnect();
  }
  if (mLogger->isPrintWanted(LogLevel::Debug)) {
    std::stringstream ss;
    ss << "Status: " << WiFi.status();
    mLogger->log(LogLevel::Debug, ss);
  }
  // Serial.println(WiFi.status());
}

void wifiHandler::UpdateStatus() {
  // mLogger->debug("update_status");
  //  Serial.println("update_status");

  IPAddress ip = WiFi.localIP();
  String ip_str = ip.toString();

  mCurrentStatus.isConnected = WiFi.isConnected();
  mCurrentStatus.IP = std::string(ip_str.c_str());
  mCurrentStatus.ssid = WiFi.SSID().c_str();

  if (mLogger->isPrintWanted(LogLevel::Debug)) {
    std::stringstream ss("Status-Not Connected");
    if (mCurrentStatus.isConnected)
      ss << "Status-Connected to:" << mCurrentStatus.ssid << ", IP:" << mCurrentStatus.IP;
    mLogger->log(LogLevel::Debug, ss);
  }

  mStatusUpdate->notify(mCurrentStatus);
}

void wifiHandler::StoreCredentials() {
  // No connection was attempted so don't try to to save the creds
  if (!mIsConnectionAttempt) {
    return;
  }
  mPassword = mConnectionAttemptPassword;
  mSSID = mConnectionAttemptSSID;

  Preferences preferences;
  preferences.begin("wifiSettings", false);
  preferences.putString("password", mPassword.c_str());
  preferences.putString("SSID", mSSID.c_str());
  preferences.end();

  mConnectionAttemptPassword.clear();
  mConnectionAttemptSSID.clear();
  mIsConnectionAttempt = false;
}

void wifiHandler::scan() {
  mLogger->debug("scan called");
  // Serial.println("scan called");
  WiFi.setAutoReconnect(false);
  WiFi.scanNetworks(true);
}

void wifiHandler::begin() {
  WiFi.setHostname("OMOTE");
  WiFi.mode(WIFI_STA);
  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t aEventInfo) {
    mInstance->WiFiEvent(event, aEventInfo);
  });

  Preferences preferences;
  preferences.begin("wifiSettings", false);
  String ssid = preferences.getString("SSID");
  String password = preferences.getString("password");
  preferences.end();

  // Attempt Connection with stored Credentials
  if (!ssid.isEmpty()) {
    connect(ssid.c_str(), password.c_str());
  } else {
    mLogger->error("no SSID or password stored");
    // Serial.println("no SSID or password stored");
    WiFi.disconnect();
  }

  WiFi.setSleep(true);
}

void wifiHandler::connect(std::string ssid, std::string password) {
  if (mLogger->isPrintWanted(LogLevel::Debug)) {
    std::stringstream ss;
    ss << "Attempting Wifi Connection To " << ssid;
    mLogger->log(LogLevel::Debug, ss);
  }
  // Serial.printf("Attempting Wifi Connection To %s \n", ssid.c_str());

  mConnectionAttemptPassword = password;
  mConnectionAttemptSSID = ssid;
  auto status = WiFi.begin(mConnectionAttemptSSID.c_str(),
                           mConnectionAttemptPassword.c_str());
}

void wifiHandler::mqttSend(std::string aTopic, std::string aMessage) {
  if (!mMqttClient.publish(aTopic.c_str(), aMessage.c_str()))
    mLogger->error("Failed to Send MQTT due to Connection Failure");
  // Serial.println("Failed to Send MQTT due to Connection Failure");
}

struct fieldIdStruct {
  std::string field;
  uint32_t id;
};

std::multimap<std::string, fieldIdStruct> Subscriptions;

void wifiHandler::mqttBindTextEvent(uint32_t bindId, std::string topic, std::string field) {
  mMqttClient.subscribe(topic.c_str());
  Subscriptions.insert({topic, {field, bindId}});
  if (mLogger->isPrintWanted(LogLevel::Debug)) {
    std::stringstream ss;
    ss << "Subscribing to topic: " << topic << ", field: " << field;
    mLogger->log(LogLevel::Debug, ss);
  }
  // Serial.printf("Subscribing to topic: %s, field: %s\r\n", topic.c_str(), field.c_str());
}

void wifiHandler::mqttUnBindTextEvent(uint32_t unBindId) {
  for (std::multimap<std::string, fieldIdStruct>::iterator it = Subscriptions.begin(); it != Subscriptions.end();) {
    if (it->second.id == unBindId) {
      if (Subscriptions.count(it->first) == 1) // only delete if last topic entry
        mMqttClient.unsubscribe(it->first.c_str());
      it = Subscriptions.erase(it);
    } else
      ++it;
  }
}

void publish_cb(char *aTopic, byte *aPayload, unsigned int length) {
  // handle message arrived
  std::string topic(aTopic);
  std::string payload(reinterpret_cast<const char *>(aPayload), length);
  if (mLogger->isPrintWanted(LogLevel::Debug)) {
    std::stringstream ss;
    ss << "MQTT: received topic " << topic << " with payload " << payload;
    mLogger->log(LogLevel::Debug, ss);
  }
  // Serial.printf("MQTT: received topic %s with payload %s\r\n", topic.c_str(), payload.c_str());

  auto range = Subscriptions.equal_range(topic);
  if (range.first != Subscriptions.end()) {
    // Serial.println("Subscription found");
    //  have match so parse json
    rapidjson::Document d;
    if (d.Parse(payload).HasParseError())
      return;

    for (auto i = range.first; i != range.second; ++i) {
      // see if we can find a json field that matches and if so call the observer on the id
      if (d.HasMember(i->second.field)) {
        // Serial.printf("Updating UI: %s\r\n", i->second.field.c_str());
        UI::observerHandles::setText(i->second.id, d[i->second.field].GetString());
      }
    }
  }
}

void wifiHandler::setupMqttBroker() {
  if (mMqttClient.connected())
    mMqttClient.disconnect();

  mMqttClient.setClient(mEspClient);
  mMqttClient.setSocketTimeout(1);
  mMqttClient.setBufferSize(512);

  uint16_t port = 1883; // std::stoi(mMqttPort);
  if (mLogger->isPrintWanted(LogLevel::Debug)) {
    std::stringstream ss;
    ss << "Setting up Mqtt Connection to: " << mMqttBroker << ", port: " << port << ", " << mMqttPort;
    mLogger->log(LogLevel::Debug, ss);
  }
  // Serial.printf("Setting up Mqtt Connection to: %s, port: %d, %s \r\n", mMqttBroker.c_str(), port, mMqttPort.c_str());
  mMqttClient.setServer(mMqttBroker.c_str(), port);
  mMqttClient.setCallback(publish_cb);
  // connect handled by mqqtSync once wifi comes up
  mOldTime = millis() - MQTT_RETRY;
  mMqttInitDone = true;
}

void wifiHandler::mqttForceReconnect() {
  mForceReconnect = true;
}

void wifiHandler::mqttSync() {
  mMqttClient.loop();
  unsigned long time = millis();
  // Serial.printf("MQTT Sync, init:%i, wifi:%i, mqtt:%i, mqtt_en:%i, time:%i, oldTime:%i\r\n", mMqttInitDone, WiFi.isConnected(), mMqttClient.connected(), mMqttEnabled, time, mOldTime);
  //  Note - connect is a blocking call, timeout set to min of 1sec but still
  //      don't retry too often and only when WiFi connected
  if ((((time - mOldTime) > MQTT_RETRY) || mForceReconnect) && mMqttInitDone && WiFi.isConnected() && !mMqttClient.connected() && mMqttEnabled) {
    mForceReconnect = false;
    mOldTime = time;
    if (mLogger->isPrintWanted(LogLevel::Debug)) {
      std::stringstream ss;
      ss << "Attempting Mqtt Connect, client: " << mMqttClientName << ",, user: " << mMqttUser;
      mLogger->log(LogLevel::Debug, ss);
    }
    // Serial.printf("Attempting Mqtt Connect, client: %s, user: %s \r\n", mMqttClientName.c_str(), mMqttUser.c_str());
    if (mMqttClient.connect(mMqttClientName.c_str(), mMqttUser.c_str(), mMqttPassword.c_str())) {
      mLogger->info("MQTT Connected");
      // Serial.println("MQTT Connected");
      if (mMqttSaveOnConnect)
        mqttSaveCredentials();
      // now need to resubmit any subscriptions to make sure they are current
      // may get duplicates but shouldn't matter
      for (std::multimap<std::string, fieldIdStruct>::iterator it = Subscriptions.begin(); it != Subscriptions.end(); it++) {
        // Serial.printf("Subscribing to topic: %s\r\n", it->first.c_str());
        mMqttClient.subscribe(it->first.c_str());
      }
    }
  }
}

void wifiHandler::mqttSaveCredentials() {

  // persist to disk
  rapidjson::Document d;
  d.SetObject();

  // Add data to the JSON document
  d.AddMember("broker", mMqttBroker, d.GetAllocator());
  d.AddMember("port", mMqttPort, d.GetAllocator());
  d.AddMember("user", mMqttUser, d.GetAllocator());
  d.AddMember("password", mMqttPassword, d.GetAllocator());
  d.AddMember("client", mMqttClientName, d.GetAllocator());

  std::ofstream file(FS_PATH "mqtt.json", std::ios::out | std::ios::trunc);
  if (!file) {
    mLogger->error("Could not save MQTT credentials.");
    return;
  }

  std::string jsonStr = OMOTE::JSON::ToString(d);
  file << jsonStr;
  file.close();

  mLogger->info("MQTT credentials saved");

  mMqttSaveOnConnect = false;
}

// need to handle this seperately as otherwise can't be disabled
//      (rest only save on succsfull connection)
void wifiHandler::enableMqtt(bool enabled) {
  mMqttEnabled = enabled;
  Preferences preferences;
  preferences.begin("MqttSettings", false);
  preferences.putBool("enabled", mMqttEnabled);
  preferences.end();
}

void wifiHandler::mqttRestoreCredentials() {
  // restore from disk
  std::filesystem::path aMqttPath(FS_PATH "mqtt.json");
  rapidjson::Document d = OMOTE::JSON::GetDocument(aMqttPath);
  if (d.HasParseError() || d.IsNull()) {
    mLogger->error("Could not load MQTT credentials.");
    return;
  }

  if (d.HasMember("broker") && d["broker"].IsString())
    mMqttBroker = d["broker"].GetString();
  if (d.HasMember("port") && d["port"].IsString())
    mMqttPort = d["port"].GetString();
  if (d.HasMember("user") && d["user"].IsString())
    mMqttUser = d["user"].GetString();
  if (d.HasMember("password") && d["password"].IsString())
    mMqttPassword = d["password"].GetString();
  if (d.HasMember("client") && d["client"].IsString())
    mMqttClientName = d["client"].GetString();

  // enabled kept in preferences as updated separately
  Preferences preferences;
  preferences.begin("MqttSettings", false);
  mMqttEnabled = preferences.getBool("enabled", false);
  preferences.end();
  mLogger->info("MQTT credentials restored");
}

void wifiHandler::ntpSaveCredentials() {
  // persist to disk
  rapidjson::Document d;
  d.SetObject();
  d.AddMember("enabled", mNtpEnabled, d.GetAllocator());
  d.AddMember("displayMode", mNtpDisplayMode, d.GetAllocator());
  d.AddMember("server", mNtpServer, d.GetAllocator());
  d.AddMember("timezone", mNtpTimeZone, d.GetAllocator());

  using namespace OMOTE::JSON;
  std::filesystem::path ntpConfigPath(FS_PATH "ntp.json");
  if (WriteDocumentToFile(d, ntpConfigPath) != DocumentFileWriteResult::Success) {
    mLogger->error("Could not save NTP credentials.");
    return;
  }
  mLogger->info("NTP credentials saved");
}

void wifiHandler::ntpRestoreCredentials() {
  // defaults
  mNtpEnabled = false;
  mNtpServer = "pool.ntp.org";
  // see https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
  mNtpTimeZone = "GMT0BST,M3.5.0/1,M10.5.0";
  mNtpDisplayMode = ntpDisplayMode::constant;

  // restore from disk
  std::filesystem::path ntpJsonConfig(FS_PATH "ntp.json");
  rapidjson::Document d = OMOTE::JSON::GetDocument(ntpJsonConfig);

  if (d.HasParseError() || d.IsNull()) {
    mLogger->error("Could not load NTP credentials.");
    mLogger->info("NTP defaults used");
    return;
  }

  if (d.HasMember("enabled") && d["enabled"].IsBool())
    mNtpEnabled = d["enabled"].GetBool();
  if (d.HasMember("displayMode") && d["displayMode"].IsInt())
    mNtpDisplayMode = d["displayMode"].GetInt();
  if (d.HasMember("server") && d["server"].IsString())
    mNtpServer = d["server"].GetString();
  if (d.HasMember("timezone") && d["timezone"].IsString())
    mNtpTimeZone = d["timezone"].GetString();

  mLogger->info("NTP credentials restored");
  mLogger->debug(mNtpEnabled ? "NTP enabled" : "NTP disabled");
  mLogger->debug(mNtpServer);
  mLogger->debug(mNtpTimeZone);
}

void wifiHandler::setupNtp() {
  // May need to re-init following light sleep, not sure whether the re-init of wifi will bother things
  // Not implemented yet as could spam NTP server on frequent sleeps
  // See what accuracy is like without for now
  setenv("TZ", mNtpTimeZone.c_str(), 1);
  tzset();
  esp_netif_sntp_deinit();
  mNtpInitialised = false;
}

void wifiHandler::nptSync() {
  // All this has to do is start the NTP service once the WiFi is up and running on a cold boot
  // Fairly nasty way of doing it, should really use a wifi connect callback but this is quick and easy to test
  if (!mNtpInitialised && mNtpEnabled && WiFi.isConnected()) {
    mLogger->info("Starting NTP");
    mNtpInitialised = true;

    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG_MULTIPLE(1, {mNtpServer.c_str()});

    esp_netif_sntp_init(&config);
  }
}

void wifiHandler::ftpSync() {
  unsigned long time = millis();
  if (mFtpEnabled && WiFi.isConnected()) {
    if (mFtpInitialised) {
      ftp::sync();
    } else {
      // don't retry too often
      if (((time - mOldFtpTime) > MQTT_RETRY) || mFtpForceConnect) {
        mFtpForceConnect = false;
        mLogger->info("Starting FTP Server");
        MDNS.begin(mmDNSName.c_str());
        ftp::begin(mFtpUser.c_str(), mFtpPassword.c_str());
        mOldFtpTime = time;
        mFtpInitialised = true;
      }
    }
  } else {
    if (mFtpInitialised) {
      mLogger->info("Stopping FTP Server");
      MDNS.end();
      ftp::end();
      mFtpInitialised = false;
    }
  }
}

void wifiHandler::ftpSaveCredentials() {

  // persist to disk
  rapidjson::Document d;
  d.SetObject();

  // Add data to the JSON document
  d.AddMember("enabled", mFtpEnabled, d.GetAllocator());
  d.AddMember("mDnsName", mmDNSName, d.GetAllocator());
  d.AddMember("user", mFtpUser, d.GetAllocator());
  d.AddMember("password", mFtpPassword, d.GetAllocator());

  std::ofstream file(FS_PATH "ftp.json", std::ios::out | std::ios::trunc);
  if (!file) {
    mLogger->error("Could not save FTP credentials.");
    return;
  }

  std::string jsonStr = OMOTE::JSON::ToString(d);
  file << jsonStr;
  file.close();

  mLogger->info("FTP credentials saved");
}

void wifiHandler::ftpRestoreCredentials() {
  // restore from disk
  std::ifstream file(FS_PATH "ftp.json", std::ios::in);
  if (!file) {
    mLogger->error("Could not load FTP credentials.");
    return;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  file.close();
  std::string content(buffer.str());

  rapidjson::Document d;
  if (!d.Parse(content.c_str()).HasParseError()) {
    if (d.HasMember("enabled") && d["enabled"].IsBool())
      mFtpEnabled = d["enabled"].GetBool();
    if (d.HasMember("mDnsName") && d["mDnsName"].IsString())
      mmDNSName = d["mDnsName"].GetString();
    if (d.HasMember("user") && d["user"].IsString())
      mFtpUser = d["user"].GetString();
    if (d.HasMember("password") && d["password"].IsString())
      mFtpPassword = d["password"].GetString();
    mLogger->info("FTP credentials restored");
    mLogger->debug("mDNS: " + mmDNSName + ", FTP: " + mFtpUser + (mFtpEnabled ? ", FTP enabled" : ", FTP disabled"));
  } else
    mLogger->info("FTP defaults used");
}
