#include "ActionFactory.hpp"
#include "HardwareFactory.hpp"

std::unique_ptr<IAction> ActionFactory::createAction(const MemConciousValue &value) {
  if (!value.HasMember("type") || !value.HasMember("name") || !value.HasMember("data"))
    return nullptr;

  const std::string type = value["type"].GetString();
  const std::string name = value["name"].GetString();
  const auto &data = value["data"];

  if (type == "IRAction") {
    return createIRAction(name, data);
  }

  return nullptr;
}

std::unique_ptr<IRAction> ActionFactory::createIRAction(const std::string &aName,
                                                        const MemConciousValue &aData) {
  if (!aData.HasMember("protocol") || !aData.HasMember("data")) {
    return nullptr;
  }

  const std::string protocol = aData["protocol"].GetString();
  const std::string hexData = aData["data"].GetString();

  return std::make_unique<IRAction>(aName, protocol, hexData);
}

std::unique_ptr<IAction> ActionFactory::createAction(const std::string &aActionName) {
  auto fs = HardwareFactory::getAbstract().littleFs();

  static constexpr auto MaxActionFileLength = 500;
  for (auto &file : fs->FilesIn("Actions")) {
    MemConsciousDocument actionDoc;
    auto actionJsonStr = file.read(MaxActionFileLength);
    actionDoc.Parse(actionJsonStr.c_str());
    if (actionDoc.HasMember("name") && actionDoc.IsString()) {
      return createAction(actionDoc);
    }
  }
  return nullptr;
}
