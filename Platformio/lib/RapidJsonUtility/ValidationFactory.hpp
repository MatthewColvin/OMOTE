#pragma once

namespace OMOTE::JSON {

#include "RapidJsonUtilty.hpp"
#include <array>
#include <concepts>
#include <functional>
#include <string_view>
#include <type_traits>

template <typename T>
concept Schema = std::convertible_to<T, std::string_view>;

template <typename T>
concept EnumWithCount = requires {
  requires std::is_enum_v<T>;
  T::COUNT;
};

// Helper to convert a string literal to std::string_view at compile time
template <size_t N>
struct StringLiteral {
  constexpr StringLiteral(const char (&str)[N]) {
    std::copy_n(str, N, value);
  }
  char value[N];
};

/**
 * @brief This class is made to help with validating JSON data against a
 *        1) Base schema
 *        2) Secondary schema that is associated with a specific enum class id
 *
 *  FactoryIds - a enum class with COUNT as final member
 *  SchemaString - a schema json for the base object
 *
 * Example:
 * {
 *    type: "id probably",
 *    baseinfo: "other base data",
 *    secondaryData: {
 *      specificDataForFactorId: "maybe a string",
 *      someotherdata : 5
 *    }
 * }
 *
 * The BaseSchemaString would be used to validate things above secondaryData.
 *    IsValidBaseObject()
 * The Register Method should then be used to register a schema that can validate the object at the secondaryData key.
 *    IsValidSecondaryObject()
 */
template <EnumWithCount FactoryIds, StringLiteral BaseSchemaString>
class ValidationFactory {
public:
  ValidationFactory() = default;
  ~ValidationFactory() = default;

  bool IsValidBaseObject(const rapidjson::Value &aBaseObject);

  bool IsValidSecondaryData(FactoryIds aId,
                            const rapidjson::Value &aDataValue);

  static bool Register(FactoryIds aActionType,
                       std::string_view aSchemaJson);

private:
  bool IsValidAgainstSchema(const rapidjson::Value &aValueToValidate, const rapidjson::Document &aSchemaDocument);

  rapidjson::Document mBaseSchema;
  static inline std::array<std::string_view, static_cast<uint16_t>(FactoryIds::COUNT)> mSchemas;
};

template <EnumWithCount FactoryIds, StringLiteral BaseSchemaString>
inline bool ValidationFactory<FactoryIds, BaseSchemaString>::Register(FactoryIds aActionType,
                                                                      std::string_view aSchemaJson) {
  mSchemas[static_cast<size_t>(aActionType)] = aSchemaJson;
  return true;
}

template <EnumWithCount FactoryIds, StringLiteral BaseSchemaString>
inline bool ValidationFactory<FactoryIds, BaseSchemaString>::IsValidBaseObject(const rapidjson::Value &aBaseObject) {
  if (!mBaseSchema.IsObject()) { // Did we cache the schema yet?
    mBaseSchema.Parse(BaseSchemaString.value);
    if (mBaseSchema.HasParseError()) {
      // If the embedded schema invalid
      return false;
    }
  }

  return IsValidAgainstSchema(aBaseObject, mBaseSchema);
}

template <EnumWithCount FactoryIds, StringLiteral BaseSchemaString>
inline bool ValidationFactory<FactoryIds, BaseSchemaString>::IsValidSecondaryData(FactoryIds aId,
                                                                                  const rapidjson::Value &aDataValue) {
  auto schemaJsonString = mSchemas[static_cast<size_t>(aId)];
  if (schemaJsonString.empty()) {
    return false;
  }

  rapidjson::Document typeSpecificActionDataSchemaDoc = OMOTE::JSON::GetDocument(schemaJsonString);
  return typeSpecificActionDataSchemaDoc.HasParseError() ? false : IsValidAgainstSchema(aDataValue, typeSpecificActionDataSchemaDoc);
}

template <EnumWithCount FactoryIds, StringLiteral BaseSchemaString>
inline bool ValidationFactory<FactoryIds, BaseSchemaString>::IsValidAgainstSchema(const rapidjson::Value &aValueToValidate, const rapidjson::Document &aSchemaDocument) {
  rapidjson::SchemaDocument schemaDoc(aSchemaDocument);
  rapidjson::SchemaValidator validator(schemaDoc);
  return aValueToValidate.Accept(validator);
}

} // namespace OMOTE::JSON
