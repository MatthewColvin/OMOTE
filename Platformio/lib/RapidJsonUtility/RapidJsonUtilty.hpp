#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "rapidjson/document.h"
#include "rapidjson/reader.h"
#include "rapidjson/schema.h"

namespace OMOTE::JSON {

std::string ToString(const rapidjson::Document &aDoc);

std::string ToPrettyString(const rapidjson::Document &aDoc);

const rapidjson::Value *GetNestedField(const rapidjson::Value &aValue,
                                       const std::vector<std::string> &aFields);

rapidjson::Document GetDocument(const std::string &aStringToParse);

rapidjson::Document GetDocument(const std::filesystem::path &aPathToJson);

} // namespace OMOTE::JSON
