#include "RapidJsonUtilty.hpp"

class Validator {
public:
  Validator() = default;
  ~Validator() = default;

  bool IsAction(const rapidjson::Value &aActionValue);

private:
  bool IsValid(const rapidjson::Value &aValueToValidate, const rapidjson::Document &aSchemaDocument);

  rapidjson::Document mActionSchema;
};