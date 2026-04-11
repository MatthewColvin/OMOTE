#pragma once

#include <array>
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

rapidjson::Document GetDocument(const std::string_view &aStringToParse);

template <size_t N>
rapidjson::Document GetDocument(const std::array<char, N> &aStringArray) {
  return GetDocument(std::string_view(aStringArray.data(), aStringArray.size()));
}

rapidjson::Document GetDocument(const std::filesystem::path &aPathToJson);

enum class DocumentFileWriteResult {
  Success,
  FileOpenError,
  WriteError
};
DocumentFileWriteResult WriteDocumentToFile(const rapidjson::Document &aDoc,
                                            const std::filesystem::path &aPathToJson,
                                            bool aPretty = false);

} // namespace OMOTE::JSON
