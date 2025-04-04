#include "observerHandles.hpp"

#include <memory>

using namespace UI;

std::map<uint32_t, obsData> observerHandles::mHandleMap;
uint32_t observerHandles::mNextID = DYNAMIC_START;

observerHandles::observerHandles() {}

observerHandles::~observerHandles() {
  for (auto it = mHandleMap.begin(); it != mHandleMap.end(); it++) {
    free(it->second.buff1);
    free(it->second.buff2);
  }
  mHandleMap.clear();
}

bool observerHandles::registerTextHandle(uint32_t key, uint16_t size,
                                         const char *Val) {
  if (mHandleMap.count(key))
    return false;

  obsData obs;
  obs.type = String;
  obs.buff_size = size;
  obs.buff1 = (char *)malloc(size);
  obs.buff2 = (char *)malloc(size);
  lv_subject_init_string(&obs.Handler, obs.buff1, obs.buff2, size, Val);

  const auto [it, success] =
      mHandleMap.insert(std::pair<uint32_t, obsData>(key, obs));
  return success;
}

bool observerHandles::registerIntHandle(uint32_t key, int32_t val) {
  if (mHandleMap.count(key))
    return false;

  obsData obs;
  obs.type = Int;
  lv_subject_init_int(&obs.Handler, val);

  const auto [it, success] =
      mHandleMap.insert(std::pair<uint32_t, obsData>(key, obs));
  return success;
}

bool observerHandles::deleteHandle(uint32_t key) {
  auto search = mHandleMap.find(key);
  if (search == mHandleMap.end())
    return false;
  else {
    free(search->second.buff1);
    free(search->second.buff2);
    mHandleMap.erase(search);
    return true;
  }
}

bool observerHandles::setText(uint32_t key, const char *val) {
  auto search = mHandleMap.find(key);
  if (search == mHandleMap.end())
    return false;
  else {
    if (search->second.type == String) {
      lv_subject_copy_string(&search->second.Handler, val);
      return true;
    } else
      return false;
  }
}

bool observerHandles::setInt(uint32_t key, uint32_t val) {
  auto search = mHandleMap.find(key);
  if (search == mHandleMap.end())
    return false;
  else {
    if (search->second.type == Int) {
      lv_subject_set_int(&search->second.Handler, val);
      return true;
    } else
      return false;
  }
}

bool observerHandles::bindLabelHandle(lv_obj_t *object, uint32_t key,
                                      const char *fmt) {
  auto search = mHandleMap.find(key);
  if (search == mHandleMap.end())
    return false;
  else {
    lv_subject_t *handle = &search->second.Handler;
    lv_label_bind_text(object, handle, fmt);
    return true;
  }
}

uint32_t observerHandles::registerNextID(uint16_t bufSize, const char *val) {
  registerTextHandle(mNextID, bufSize, val);
  return mNextID++;
}
