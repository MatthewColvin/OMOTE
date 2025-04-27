#include "wifihandler.hpp"

#define RAPIDJSON_HAS_STDSTRING 1
#include <Arduino.h>
#include <Preferences.h>
#include <rapidjson/document.h>

#include "HardwareAbstract.hpp"
#include "HardwareFactory.hpp"
#include "WiFi.h"
#include "observerHandles.hpp"
#include "omoteconfig.h"

#define MQTT_RETRY 60000

std::shared_ptr<wifiHandler> wifiHandler::mInstance = nullptr;
std::shared_ptr<wifiHandler> wifiHandler::getInstance() {
  if (mInstance) {
    return mInstance;
  }
  mInstance = std::shared_ptr<wifiHandler>(new wifiHandler());
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
  case ARDUINO_EVENT_WIFI_STA_GOT_IP:
  case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
  case ARDUINO_EVENT_WIFI_STA_LOST_IP:
  case ARDUINO_EVENT_WIFI_STA_STOP:
    UpdateStatus();
    break;
  default:
    break;
  }
  if (WiFi.status() == WL_CONNECT_FAILED) {
    Serial.println("connection failed.");
    WiFi.disconnect();
  }
  Serial.println(WiFi.status());
}

void wifiHandler::UpdateStatus() {
  Serial.println("update_status");

  IPAddress ip = WiFi.localIP();
  String ip_str = ip.toString();

  mCurrentStatus.isConnected = WiFi.isConnected();
  mCurrentStatus.IP = std::string(ip_str.c_str());
  mCurrentStatus.ssid = WiFi.SSID().c_str();

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
  Serial.println("scan called");
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
    Serial.println("no SSID or password stored");
    WiFi.disconnect();
  }

  WiFi.setSleep(true);
}

void wifiHandler::connect(std::string ssid, std::string password) {
  Serial.printf("Attempting Wifi Connection To %s \n", ssid.c_str());
  mConnectionAttemptPassword = password;
  mConnectionAttemptSSID = ssid;
  auto status = WiFi.begin(mConnectionAttemptSSID.c_str(),
                           mConnectionAttemptPassword.c_str());
}

void wifiHandler::mqttSend(std::string aTopic, std::string aMessage) {
  if (!mMqttClient.publish(aTopic.c_str(), aMessage.c_str()))
    Serial.println("Failed to Send MQTT due to Connection Failure");
}

struct fieldIdStruct {
  std::string field;
  uint32_t id;
};

std::multimap<std::string, fieldIdStruct> Subscriptions;

void wifiHandler::mqttBindTextEvent(uint32_t bindId, std::string topic, std::string field) {
  mMqttClient.subscribe(topic.c_str());
  Subscriptions.insert({topic, {field, bindId}});
  Serial.printf("Subscribing to topic: %s, field: %s\r\n", topic.c_str(), field.c_str());
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
  Serial.printf("MQTT: received topic %s with payload %s\r\n", topic.c_str(), payload.c_str());

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
  Serial.printf("Setting up Mqtt Connection to: %s, port: %d, %s \r\n", mMqttBroker.c_str(), port, mMqttPort.c_str());
  mMqttClient.setServer(mMqttBroker.c_str(), port);
  mMqttClient.setCallback(publish_cb);
  // connect handled by mqqtSync once wifi comes up
  mOldTime = millis() - MQTT_RETRY;
  mMqttInitDone = true;
}

void wifiHandler::mqttSync() {
  mMqttClient.loop();
  unsigned long time = millis();
  // Note - connect is a blocking call, timeout set to min of 1sec but still
  //     don't retry too often and only when WiFi connected
  if (((time - mOldTime) > MQTT_RETRY) && mMqttInitDone && WiFi.isConnected() && !mMqttClient.connected() && mMqttEnabled) {
    mOldTime = time;
    Serial.printf("Attempting Mqtt Connect, client: %s, user: %s \r\n", mMqttClientName.c_str(), mMqttUser.c_str());
    if (mMqttClient.connect(mMqttClientName.c_str(), mMqttUser.c_str(), mMqttPassword.c_str())) {
      Serial.println("MQTT Connected");
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

  File file = HardwareFactory::getAbstract().littleFs()->open("/mqtt.json", LFS_O_WRONLY | LFS_O_CREAT);

  if (!file)
    return;

  std::string jsonStr = ToString(d);
  file.write(jsonStr);
  file ? file.truncate(jsonStr.length()) : []() { return -1; }();
  // Serial.println("MQTT credentials saved");

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
  File fp = HardwareFactory::getAbstract().littleFs()->open("/mqtt.json", LFS_O_RDONLY);

  if (!fp)
    return;

  std::string content = fp.read(1000);
  
  MemConsciousDocument d;
  if (!d.Parse(content.c_str()).HasParseError()) {
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
  }

  // enabled kept in preferences as updated seperately
  Preferences preferences;
  preferences.begin("MqttSettings", false);
  mMqttEnabled = preferences.getBool("enabled", false);
  preferences.end();
  // Serial.println("MQTT credentials restored");
}
