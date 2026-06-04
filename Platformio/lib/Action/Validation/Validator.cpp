#include "Validator.hpp"
#include "ObjectSchemaBuilder.hpp"

bool Validator::IsAction(const rapidjson::Value &aActionValue) {

  static constexpr auto actionSchema = OMOTE::JSON::ObjectSchema()
                                           .Require("type", "string")
                                           .Require("name", "string")
                                           .Require("data", "object")
                                           .Build();

  return OMOTE::JSON::IsJsonValid(aActionValue, actionSchema);
}

bool Validator::IsDataValid(ActionTypes aActionType,
                            const rapidjson::Value &aDataValue) {
  auto schemaJsonString = mSchemas[static_cast<size_t>(aActionType)];
  if (schemaJsonString.empty()) {
    return false;
  }

  return OMOTE::JSON::IsJsonValid(aDataValue, schemaJsonString);
}
