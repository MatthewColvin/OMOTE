#include <fstream>
#include <string>
#include <unistd.h>

#define RAPIDJSON_HAS_STDSTRING 1
#include <fstream>
#include <iostream>
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/writer.h>

#include "observerHandles.hpp"
#include "wifiHandlerSim.hpp"
#include <../examples/templates/posix_sockets.h>
#include <mqtt.h>

#include "HardwareFactory.hpp"

using WifiInfo = wifiHandlerInterface::WifiInfo;

using namespace rapidjson;

struct mqtt_client mMqttClient;

wifiHandlerSim::wifiHandlerSim() {}

void wifiHandlerSim::begin() {
  restoreCredentials();
  setupMqttBroker();
}

void wifiHandlerSim::connect(std::string ssid, std::string password) {
  if (mFakeStatusThread.joinable()) {
    mFakeStatusThread.join();
    mCurrentStatus.ssid = ssid;
    mCurrentStatus.isConnected = true;
    mFakeStatusThread = std::thread([this] {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      mStatusUpdate->notify(mCurrentStatus);
    });
  }
}

static const WifiInfo wifis[] = {
    WifiInfo("High Signal Wifi", -49, false), WifiInfo("Mid Signal Wifi", -55, true),
    WifiInfo("Low Signal Wifi", -65, false), WifiInfo("No Signal Wifi", -90, false)};

void wifiHandlerSim::scan() {
  if (mFakeScanThread.joinable()) {
    mFakeScanThread.join();
    mFakeScanThread = std::thread([this] {
      std::vector<WifiInfo> info =
          std::vector(std::begin(wifis), std::end(wifis));
      std::this_thread::sleep_for(std::chrono::seconds(2));
      mScanNotification->notify(info);
    });
  }
}

struct fieldIdStruct {
  std::string field;
  uint32_t id;
};

std::multimap<std::string, fieldIdStruct> Subscriptions;

void wifiHandlerSim::mqttBindTextEvent(uint32_t bindId, std::string topic, std::string field) {
  if (mMqttConnected)
    mqtt_subscribe(&mMqttClient, topic.c_str(), 2);
  Subscriptions.insert({topic, {field, bindId}});
}

void wifiHandlerSim::mqttUnBindTextEvent(uint32_t unBindId) {
  for (std::multimap<std::string, fieldIdStruct>::iterator it = Subscriptions.begin(); it != Subscriptions.end();) {
    if (it->second.id == unBindId) {
      if (Subscriptions.count(it->first) == 1) // only delete if last topic entry
        mqtt_unsubscribe(&mMqttClient, it->first.c_str());
      it = Subscriptions.erase(it);
    } else
      ++it;
  }
}

void publish_cb(void **state, struct mqtt_response_publish *publish) {
  std::string topic((const char *)(publish->topic_name), publish->topic_name_size);
  std::string payload((const char *)(publish->application_message), publish->application_message_size);

  printf("Received a PUBLISH(topic=%s, DUP=%d, QOS=%d, RETAIN=%d, pid=%d) from the broker. Data='%s'\r\n",
         topic.c_str(), publish->dup_flag, publish->qos_level, publish->retain_flag, publish->packet_id,
         payload.c_str());

  auto range = Subscriptions.equal_range(topic);
  if (range.first != Subscriptions.end()) {
    // have match so parse json
    rapidjson::Document d;
    if (d.Parse(payload).HasParseError())
      return;

    for (auto i = range.first; i != range.second; ++i) {
      // see if we can find a json field that matches and if so call the observer on the id
      if (d.HasMember(i->second.field)) {
        UI::observerHandles::setText(i->second.id, d[i->second.field].GetString());
      }
    }
  }
}

struct reconnect_state_t {
  const char *hostname;
  const char *port;
  const char *clientName;
  const char *user;
  const char *password;
  uint8_t *sendbuf;
  size_t sendbufsz;
  uint8_t *recvbuf;
  size_t recvbufsz;
  wifiHandlerSim *thisPtr;
};

void reconnect_mqtt_cb(struct mqtt_client *client, void **reconnect_state_vptr) {
  struct reconnect_state_t *reconnect_state = *((struct reconnect_state_t **)reconnect_state_vptr);

  /* Close the clients socket if this isn't the initial reconnect call */
  if (client->error != MQTT_ERROR_INITIAL_RECONNECT) {
    close(client->socketfd);
  }

  /* Perform error handling here. */
  if (client->error != MQTT_ERROR_INITIAL_RECONNECT) {
    printf("reconnect_client: called while client was in error state \"%s\"\n",
           mqtt_error_str(client->error));
  }

  /* Open a new socket. */
  int sockfd = open_nb_socket(reconnect_state->hostname, reconnect_state->port);
  if (sockfd == -1) {
    perror("Failed to open socket: ");
    return;
  }

  /* Reinitialize the client. */
  mqtt_reinit(client, sockfd,
              reconnect_state->sendbuf, reconnect_state->sendbufsz,
              reconnect_state->recvbuf, reconnect_state->recvbufsz);

  /* Ensure we have a clean session */
  uint8_t connect_flags = MQTT_CONNECT_CLEAN_SESSION;
  /* Send connection request to the broker. */
  mqtt_connect(client, reconnect_state->clientName, NULL, NULL, 0, reconnect_state->user, reconnect_state->password, connect_flags, 30);

  if (client->error == MQTT_OK) {
    reconnect_state->thisPtr->mMqttConnected = true;
    reconnect_state->thisPtr->mqttSaveCredentials();
  }
}

void wifiHandlerSim::stop_mqtt() {
  mMqttConnected = false;
  mqtt_disconnect(&mMqttClient);
  mqtt_sync(&mMqttClient);
}

void wifiHandlerSim::setupMqttBroker() {
  stop_mqtt();
  init_mqtt();
}

void wifiHandlerSim::init_mqtt() {
  static uint8_t sendBuf[1024], recvBuf[1024];

  struct reconnect_state_t reconnect_state;
  reconnect_state.hostname = mMqttBroker.c_str();
  reconnect_state.port = mMqttPort.c_str();
  reconnect_state.clientName = mMqttClientName.c_str();
  reconnect_state.user = mMqttUser.c_str();
  reconnect_state.password = mMqttPassword.c_str();
  reconnect_state.sendbuf = sendBuf;
  reconnect_state.sendbufsz = sizeof(sendBuf);
  reconnect_state.recvbuf = recvBuf;
  reconnect_state.recvbufsz = sizeof(recvBuf);
  reconnect_state.thisPtr = this;

  mqtt_init_reconnect(&mMqttClient, reconnect_mqtt_cb, &reconnect_state, publish_cb);
  void *structPtr = &reconnect_state;
  reconnect_mqtt_cb(&mMqttClient, &structPtr); // ensure connection completes before subscriptions start
}

void wifiHandlerSim::mqttSend(std::string aTopic, std::string aMessage) {

  mqtt_publish(&mMqttClient, aTopic.c_str(), aMessage.c_str(), aMessage.size(), MQTT_PUBLISH_QOS_0);
}

void wifiHandlerSim::mqttSync() {
  if (mMqttConnected)
    mqtt_sync(&mMqttClient);
}

void wifiHandlerSim::mqttSaveCredentials() {
  if (mMqttSaveOnConnect) {
    // persist to disk
    Document d;
    d.SetObject();

    // Add data to the JSON document
    d.AddMember("enabled", mMqttEnabled, d.GetAllocator());
    d.AddMember("broker", mMqttBroker, d.GetAllocator());
    d.AddMember("port", mMqttPort, d.GetAllocator());
    d.AddMember("user", mMqttUser, d.GetAllocator());
    d.AddMember("password", mMqttPassword, d.GetAllocator());
    d.AddMember("client", mMqttClientName, d.GetAllocator());

    std::ofstream file(FS_PATH "mqtt.json", std::ios::out | std::ios::trunc);
    if (!file) {
      return;
    }

    std::string jsonStr = OMOTE::JSON::ToString(d);
    file << jsonStr;
    file.close();

    mMqttSaveOnConnect = false;
  }
}

void wifiHandlerSim::restoreCredentials() {
  // restore from disk
  std::ifstream file(FS_PATH "mqtt.json", std::ios::in);
  if (!file) {
    return;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  file.close();
  std::string content(buffer.str());

  rapidjson::Document d;
  d.Parse(content.c_str());

  if (d.HasMember("enabled"))
    mMqttEnabled = d["enabled"].GetBool();
  if (d.HasMember("broker"))
    mMqttBroker = d["broker"].GetString();
  if (d.HasMember("port"))
    mMqttPort = d["port"].GetString();
  if (d.HasMember("user"))
    mMqttUser = d["user"].GetString();
  if (d.HasMember("password"))
    mMqttPassword = d["password"].GetString();
  if (d.HasMember("client"))
    mMqttClientName = d["client"].GetString();
}
