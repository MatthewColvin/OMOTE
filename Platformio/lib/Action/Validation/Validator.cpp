#include "Validator.hpp"

bool Validator::IsAction(const rapidjson::Value &aActionValue) {
  static constexpr auto actionSchema = R"({
    "type": "object",
    "required": ["type", "name", "data"],
    "properties": {
      "type": { "type": "string" },
      "name": { "type": "string" },
      "data": { "type": "object" }
    }
  })";

  if (!mActionSchema.IsObject()) { // Did we cache the schema yet?
    mActionSchema.Parse(actionSchema);
    if (mActionSchema.HasParseError()) {
      // If the embedded schema invalid
      return false;
    }
  }

  return IsValid(aActionValue, mActionSchema);
}

bool Validator::IsValid(const rapidjson::Value &aValueToValidate, const rapidjson::Document &aSchemaDocument) {
  rapidjson::SchemaDocument schemaDoc(aSchemaDocument);
  rapidjson::SchemaValidator validator(schemaDoc);
  return aValueToValidate.Accept(validator);
}
