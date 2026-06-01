#pragma once

#include <rapidjson/document.h>
#include <string>

namespace HaClimate {

struct Attrs {
  float current = 0;
  float target = 0;
  float low = 0;
  float high = 0;
  float minT = 50;
  float maxT = 90;
  bool hasRange = false;
  std::string mode;
};

/** Parse HA climate attributes from a state_changed attributes object. */
bool parseClimateAttrs(const rapidjson::Value &attributes, const std::string &state, Attrs &out);

/** Parse attributes JSON string (object only). */
bool parseClimateAttrsJson(const std::string &attributesJson, const std::string &state, Attrs &out);

int readSliderValue(const std::string &domain, const rapidjson::Value &attributes, const std::string &attributeKey,
                    int defaultVal = 0);

} // namespace HaClimate
