#include "RapidJsonUtilty.hpp"

#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include <fstream>

void MemConciousAllocator::Free(void *aVal) {}

void BuffDeleter::operator()(void *buffer) { free(buffer); }

void *MemConciousAllocator::Malloc(size_t aSize) {
  mBuffers.emplace_back(aSize);
  return mBuffers.back().data();
}

void *MemConciousAllocator::Realloc(void *originalPtr, size_t originalSize,
                                    size_t newSize) {
  if (originalPtr == nullptr) {
    return Malloc(newSize);
  }
  for (auto &buffer : mBuffers) {
    if (originalPtr == buffer.data()) {
      buffer.resize(newSize);
      return buffer.data();
    }
  }
  // Told us to realloc but didn't know about old buffer... bad
  return nullptr;
}

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

rapidjson::Document GetDocument(const std::filesystem::path &aPathToJson) {
  std::ifstream file(aPathToJson);
  rapidjson::Document doc;
  if (!file.is_open()) {
    return doc; // return empty doc if file couldn't be opened
  }
  rapidjson::IStreamWrapper fileStream(file);
  doc.ParseStream(fileStream);
  // If parsing failed return an empty object document
  if (doc.HasParseError()) {
    return rapidjson::Document(rapidjson::kObjectType);
  }
  return doc;
}
