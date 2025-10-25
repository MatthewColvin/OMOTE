#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "rapidjson/document.h"
#include "rapidjson/reader.h"

using BigMessageHandle = rapidjson::BaseReaderHandler<>;

class MemConciousAllocator;

using MemConsciousDocument =
    rapidjson::GenericDocument<rapidjson::UTF8<>, MemConciousAllocator>;
using MemConciousValue =
    rapidjson::GenericValue<rapidjson::UTF8<>, MemConciousAllocator>;

struct BuffDeleter {
  void operator()(void *buffer);
};

class MemConciousAllocator {
public:
  using BufferType = std::unique_ptr<void, BuffDeleter>;
  static const bool kNeedFree = false;
  static void Free(void *aVal);
  void *Malloc(size_t aSize);
  void *Realloc(void *originalPtr, size_t originalSize, size_t newSize);

private:
  std::vector<std::vector<char>> mBuffers;
};

std::string ToString(const rapidjson::Document &aDoc);

std::string ToPrettyString(const rapidjson::Document &aDoc);

const rapidjson::Value *GetNestedField(const rapidjson::Value &aValue,
                                       const std::vector<std::string> &aFields);

rapidjson::Document GetDocument(const std::string &aStringToParse);

rapidjson::Document GetDocument(const std::filesystem::path &aPathToJson);
