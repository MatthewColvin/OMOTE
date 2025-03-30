#pragma once

#include <map>
#include <string>

#include "lvgl.h"

namespace UI {

#define BATT_STATUS 0
#define SOC_STATUS 1
#define WIFI_STATUS 2
#define DYNAMIC_START 3

enum obsType { None,
               String,
               Int };

struct obsData {
  lv_subject_t Handler;
  obsType type = None;
  char *buff1 = NULL;
  char *buff2 = NULL;
  uint16_t buff_size = 0;
};

class observerHandles {
public:
  observerHandles();
  ~observerHandles();

  static uint32_t getNextID();

  static bool registerTextHandle(uint32_t key, uint16_t size, const char *Val);
  static bool registerIntHandle(uint32_t key, int32_t val);

  static bool deleteHandle(uint32_t key);

  static bool setText(uint32_t key, const char *val);
  static bool setInt(uint32_t key, uint32_t val);

  static bool bindLabelHandle(lv_obj_t *object, uint32_t key, const char *fmt);

private:
  static std::map<uint32_t, obsData> mHandleMap;
  static uint32_t mNextID;
};
} // namespace UI