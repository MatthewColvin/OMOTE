#include "RapidJsonUtilty.hpp"

#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include <fstream>

namespace OMOTE::JSON {

std::string ToString(const rapidjson::Document &aDoc) {
  rapidjson::StringBuffer buff;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buff);
  aDoc.Accept(writer);
  return std::string(buff.GetString());
}

std::string ToPrettyString(const rapidjson::Document &aDoc) {
  rapidjson::StringBuffer buff;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> prettyWrite(buff);
  prettyWrite.SetIndent(' ', 2);
  aDoc.Accept(prettyWrite);
  return std::string(buff.GetString());
}

const rapidjson::Value *GetNestedField(
    const rapidjson::Value &aValue, const std::vector<std::string> &aFields) {
  const rapidjson::Value *value = &aValue;
  for (const auto &field : aFields) {
    if (!value || !value->IsObject() || !value->HasMember(field.c_str())) {
      return nullptr;
    }
    value = &(*value)[field.c_str()];
  }
  return value;
}

rapidjson::Document GetDocument(
    const std::string &aStringToParse) {
  rapidjson::Document doc;
  doc.Parse(aStringToParse.c_str());
  return doc;
}

rapidjson::Document GetDocument(const std::string_view &aStringToParse) {
  rapidjson::Document doc;
  doc.Parse(aStringToParse.data(), aStringToParse.size());
  return doc;
}

rapidjson::Document GetDocument(const std::filesystem::path &aPathToJson) {
  std::ifstream file(aPathToJson);
  rapidjson::Document doc;
  if (!file.is_open()) {
    return doc; // return empty doc if file couldn't be opened
  }
  rapidjson::IStreamWrapper fileStream(file);
  doc.ParseStream(fileStream);
  return doc;
}

DocumentFileWriteResult WriteDocumentToFile(
    const rapidjson::Document &aDoc,
    const std::filesystem::path &aPathToJson,
    bool aPretty) {
  std::ofstream file(aPathToJson, std::ios::out | std::ios::trunc);
  if (!file.is_open()) {
    return DocumentFileWriteResult::FileOpenError;
  }

  std::string jsonStr;
  jsonStr = aPretty ? ToPrettyString(aDoc) : jsonStr = ToString(aDoc);

  file << jsonStr;
  if (file.fail()) {
    return DocumentFileWriteResult::WriteError;
  }

  file.close();
  return DocumentFileWriteResult::Success;
}

} // namespace OMOTE::JSON