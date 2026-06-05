#include "HaAttrs.hpp"

#include <algorithm>
#include <cctype>

namespace {

bool attrIsNumber(const rapidjson::Value &v, float &out) {
  if (v.IsNumber())
    return out = static_cast<float>(v.GetDouble()), true;
  if (v.IsInt())
    return out = static_cast<float>(v.GetInt()), true;
  return false;
}

} // namespace

namespace HaClimate {

bool parseClimateAttrs(const rapidjson::Value &attr, const std::string &state, Attrs &out) {
  if (!attr.IsObject())
    return false;

  out.mode = state;
  if (attr.HasMember("hvac_mode") && attr["hvac_mode"].IsString())
    out.mode = attr["hvac_mode"].GetString();

  attrIsNumber(attr["current_temperature"], out.current);
  attrIsNumber(attr["temperature"], out.target);

  if (attr.HasMember("target_temp_low") && attr.HasMember("target_temp_high")) {
    float low = 0, high = 0;
    if (attrIsNumber(attr["target_temp_low"], low) && attrIsNumber(attr["target_temp_high"], high)) {
      out.low = low;
      out.high = high;
      out.hasRange = true;
    }
  } else {
    out.low = out.target;
    out.high = out.target;
    out.hasRange = false;
  }

  std::string unit;
  if (attr.HasMember("temperature_unit") && attr["temperature_unit"].IsString())
    unit = attr["temperature_unit"].GetString();
  else if (attr.HasMember("unit_of_measurement") && attr["unit_of_measurement"].IsString())
    unit = attr["unit_of_measurement"].GetString();

  bool useF = unit.find('F') != std::string::npos || unit.find('f') != std::string::npos;
  float minT = useF ? 50.f : 10.f;
  float maxT = useF ? 90.f : 32.f;
  attrIsNumber(attr["min_temp"], minT);
  attrIsNumber(attr["max_temp"], maxT);
  if (maxT <= minT) {
    minT = useF ? 50.f : 10.f;
    maxT = useF ? 90.f : 32.f;
  }
  out.minT = minT;
  out.maxT = maxT;
  return true;
}

bool parseClimateAttrsJson(const std::string &attributesJson, const std::string &state, Attrs &out) {
  if (attributesJson.empty())
    return false;
  rapidjson::Document doc;
  if (doc.Parse(attributesJson.c_str()).HasParseError() || !doc.IsObject())
    return false;
  return parseClimateAttrs(doc, state, out);
}

int readSliderValue(const std::string &domain, const rapidjson::Value &attributes, const std::string &attributeKey,
                    int defaultVal) {
  if (!attributes.IsObject())
    return defaultVal;

  std::string key = attributeKey;
  if (key.empty()) {
    if (domain == "light")
      key = "brightness";
    else if (domain == "cover")
      key = "current_position";
    else if (domain == "fan")
      key = "percentage";
    else
      return defaultVal;
  }

  if (!attributes.HasMember(key.c_str()))
    return defaultVal;
  const auto &v = attributes[key.c_str()];
  if (v.IsInt())
    return v.GetInt();
  if (v.IsUint())
    return static_cast<int>(v.GetUint());
  if (v.IsNumber())
    return static_cast<int>(v.GetDouble());
  return defaultVal;
}

} // namespace HaClimate
