#include "Halloween.hpp"
#include <Arduino.h>
#include <WiFi.h>
#include <array>

using namespace UI::Page;

std::array<std::array<uint8_t, 6>, 10> hatAddresses{
    {{0x24, 0xD7, 0xEB, 0xCA, 0x81, 0x6C},
     {0x24, 0xD7, 0xEB, 0xCA, 0x6E, 0xAA},
     {0x08, 0x3a, 0x8d, 0xd4, 0x01, 0x73},
     {0x24, 0xD7, 0xEB, 0xCA, 0x66, 0x22},
     {0x24, 0xD7, 0xEB, 0xCA, 0x8A, 0xCA},
     {0x08, 0x3a, 0x8d, 0xd4, 0x25, 0x0d},
     {0x24, 0xD7, 0xEB, 0xCA, 0x79, 0xfe},
     {0x08, 0x3a, 0x8d, 0xd4, 0x07, 0x7a},
     {0x08, 0x3a, 0x8d, 0xd4, 0x26, 0xf8},
     {0x08, 0x3a, 0x8d, 0xd3, 0xfe, 0xd9}}};

std::array<std::string, 10> person{"David", "Josh",   "John",   "Katie",
                                   "Matt",  "Zach",   "Nathan", "Dan",
                                   "Sam",   "Kristin"};

std::array<Halloween::CommunicationData, 10> requests;

void resetRequest(Halloween::CommunicationData &aRequestToReset) {
  aRequestToReset.isRequestingAllowingEntryRoutine = false;
  aRequestToReset.isRequestingDenyingEntryRoutine = false;
  aRequestToReset.isRequestingMarioRoutine = false;
  aRequestToReset.isRequestingJeopardyRoutine = false;
}

bool allLedIdle = true;

Halloween::Halloween()
    : Base(ID()),
      denyList(AddElement<Widget::List>(std::make_unique<Widget::List>())),
      acceptList(AddElement<Widget::List>(std::make_unique<Widget::List>())),
      marioList(AddElement<Widget::List>(std::make_unique<Widget::List>())),
      jeopardyList(AddElement<Widget::List>(std::make_unique<Widget::List>())),
      ledIdleList(AddElement<Widget::List>(std::make_unique<Widget::List>())) {
  WiFi.mode(WIFI_STA);

  auto listHeight = 1000;

  if (esp_now_init() != 0) {
    Serial.print("Error init now");
  }
  // Register Addresses in esp_now
  for (auto &address : hatAddresses) {
    esp_now_peer_info hat = {};
    memcpy(hat.peer_addr, &address, sizeof(hat.peer_addr));
    hat.encrypt = false;
    hat.channel = 1;
    hat.ifidx = WIFI_IF_STA;
    if (esp_now_add_peer(&hat) != ESP_OK) {
      Serial.print("Error adding peer");
    };
  }

  // construct deny lists
  denyList->SetWidth((GetContentWidth() * 2) / 4);
  denyList->SetHeight(listHeight);
  denyList->AlignTo(this, LV_ALIGN_TOP_LEFT);
  denyList->SetAllPadding(0);
  for (int i = 0; i < hatAddresses.size(); i++) {
    denyList->AddItem(person[i], LV_SYMBOL_WARNING, [i]() {
      resetRequest(requests[i]);
      requests[i].isRequestingDenyingEntryRoutine = true;
      esp_now_send(hatAddresses[i].data(), (uint8_t *)&requests[i],
                   sizeof(CommunicationData));
    });
  }
  denyList->AddItem("All", LV_SYMBOL_WARNING, [] {
    for (int i = 0; i < hatAddresses.size(); i++) {
      resetRequest(requests[i]);
      requests[i].isRequestingDenyingEntryRoutine = true;
      esp_now_send(hatAddresses[i].data(), (uint8_t *)&requests[i],
                   sizeof(CommunicationData));
    }
  });

  // Construct Accept list
  acceptList->SetWidth(GetContentWidth() / 4);
  acceptList->SetHeight(listHeight);
  acceptList->AlignTo(denyList, LV_ALIGN_OUT_RIGHT_MID);
  acceptList->SetAllPadding(0);
  for (int i = 0; i < hatAddresses.size(); i++) {
    acceptList->AddItem("", LV_SYMBOL_OK, [i]() {
      resetRequest(requests[i]);
      requests[i].isRequestingAllowingEntryRoutine = true;
      esp_now_send(hatAddresses[i].data(), (uint8_t *)&requests[i],
                   sizeof(CommunicationData));
    });
  }
  acceptList->AddItem("", LV_SYMBOL_OK, [] {
    for (int i = 0; i < hatAddresses.size(); i++) {
      resetRequest(requests[i]);
      requests[i].isRequestingAllowingEntryRoutine = true;
      esp_now_send(hatAddresses[i].data(), (uint8_t *)&requests[i],
                   sizeof(CommunicationData));
    }
  });

  // Construct Mario list
  marioList->SetWidth(GetContentWidth() / 4);
  marioList->SetHeight(listHeight);
  marioList->AlignTo(acceptList, LV_ALIGN_OUT_RIGHT_MID);
  marioList->SetAllPadding(0);
  for (int i = 0; i < hatAddresses.size(); i++) {
    marioList->AddItem("", LV_SYMBOL_BELL, [i]() {
      resetRequest(requests[i]);
      requests[i].isRequestingMarioRoutine = true;
      esp_now_send(hatAddresses[i].data(), (uint8_t *)&requests[i],
                   sizeof(CommunicationData));
    });
  }
  marioList->AddItem("", LV_SYMBOL_BELL, [] {
    for (int i = 0; i < hatAddresses.size(); i++) {
      resetRequest(requests[i]);
      requests[i].isRequestingMarioRoutine = true;
      esp_now_send(hatAddresses[i].data(), (uint8_t *)&requests[i],
                   sizeof(CommunicationData));
    }
  });

  // Jeopardy list construction
  jeopardyList->SetWidth(GetContentWidth() / 4);
  jeopardyList->SetHeight(listHeight);
  jeopardyList->AlignTo(marioList, LV_ALIGN_OUT_RIGHT_MID);
  jeopardyList->SetAllPadding(0);
  for (int i = 0; i < hatAddresses.size(); i++) {
    jeopardyList->AddItem("j", nullptr, [i]() {
      resetRequest(requests[i]);
      requests[i].isRequestingJeopardyRoutine = true;
      esp_now_send(hatAddresses[i].data(), (uint8_t *)&requests[i],
                   sizeof(CommunicationData));
    });
  }
  jeopardyList->AddItem("j", nullptr, [] {
    for (int i = 0; i < hatAddresses.size(); i++) {
      resetRequest(requests[i]);
      requests[i].isRequestingJeopardyRoutine = true;
      esp_now_send(hatAddresses[i].data(), (uint8_t *)&requests[i],
                   sizeof(CommunicationData));
    }
  });

  // LED idle list construction
  ledIdleList->SetWidth(GetContentWidth() / 4);
  ledIdleList->SetHeight(listHeight);
  ledIdleList->AlignTo(jeopardyList, LV_ALIGN_OUT_RIGHT_MID);
  ledIdleList->SetAllPadding(0);
  for (int i = 0; i < hatAddresses.size(); i++) {
    ledIdleList->AddItem("", LV_SYMBOL_BATTERY_EMPTY, [i]() {
      resetRequest(requests[i]);
      requests[i].isLightsOnInIdle = !requests[i].isLightsOnInIdle;
      esp_now_send(hatAddresses[i].data(), (uint8_t *)&requests[i],
                   sizeof(CommunicationData));
    });
  }
  ledIdleList->AddItem("", LV_SYMBOL_BATTERY_EMPTY, [] {
    allLedIdle = !allLedIdle;
    for (int i = 0; i < hatAddresses.size(); i++) {
      resetRequest(requests[i]);
      requests[i].isLightsOnInIdle = allLedIdle;
      esp_now_send(hatAddresses[i].data(), (uint8_t *)&requests[i],
                   sizeof(CommunicationData));
    }
  });

  esp_now_register_send_cb(sendCb);
};

void Halloween::sendCb(const uint8_t *mac_addr, esp_now_send_status_t status) {
  std::string aStatus = status == ESP_NOW_SEND_SUCCESS ? "Success: " : "FAIL: ";
  Serial.print(aStatus.c_str());
  for (int i = 0; i < 6; i++) {
    Serial.print(mac_addr[i], HEX);
    Serial.print(":");
  }
  Serial.println("");
}