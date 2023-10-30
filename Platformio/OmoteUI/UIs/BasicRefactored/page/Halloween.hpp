#pragma once
#include "Button.hpp"
#include "List.hpp"
#include "PageBase.hpp"
#include <esp_now.h>

namespace UI::Page {
class Halloween : public Base {
public:
  struct CommunicationData {
    bool isRequestingAllowingEntryRoutine = false;
    bool isRequestingDenyingEntryRoutine = false;
    bool isRequestingMarioRoutine = false;
    bool isRequestingJeopardyRoutine = false;
    bool isLightsOnInIdle = true;
  };

  Halloween();

  static void sendCb(const uint8_t *mac_addr, esp_now_send_status_t status);
  static void sendRequest(const uint8_t *mac_addr, CommunicationData &aData,
                          uint8_t aMsFailRetryTime = 0);

private:
  Widget::List *denyList;
  Widget::List *acceptList;
  Widget::List *marioList;
  Widget::List *jeopardyList;
  Widget::List *ledIdleList;

  std::vector<Widget::Button *> denyButtons;
};

} // namespace UI::Page
