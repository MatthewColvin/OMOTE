#include "ActionTypes.hpp"
#include "RapidJsonUtilty.hpp"
#include <array>
#include <functional>
#include <type_traits>

class Validator {
public:
  Validator() = default;
  ~Validator() = default;

  bool IsAction(const rapidjson::Value &aActionValue);

  bool IsDataValid(ActionTypes aActionType,
                   const rapidjson::Value &aDataValue);

  static bool Register(ActionTypes aActionType,
                       std::string_view aSchemaJson);

private:
  static inline std::array<std::string_view, static_cast<uint16_t>(ActionTypes::COUNT)> mSchemas;
};

inline bool Validator::Register(ActionTypes aActionType,
                                std::string_view aSchemaJson) {
  mSchemas[static_cast<size_t>(aActionType)] = aSchemaJson;
  return true;
}