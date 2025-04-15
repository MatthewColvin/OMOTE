#include "IRAction.hpp"
#include "magic_enum.hpp"

IRAction::IRAction(const std::string &aName, const std::string &aProtocol, const std::string &aHexData)
    : IAction(aName) {}

void IRAction::execute() {
  auto &hardware = HardwareFactory::getAbstract();
  auto ir = hardware.ir();

  if (std::holds_alternative<int64IrPair>(mData)) {
    auto pair = std::get<int64IrPair>(mData);
    ir->send(pair.first, pair.second);
  }
}

IRAction::protocolTy IRAction::getProtocol(const std::string &aProtocolString) {
  for (auto protocol : magic_enum::enum_values<IRInterface::int64SendTypes>()) {
    if (aProtocolString.compare(magic_enum::enum_name(protocol)) == 0) {
      return protocol;
    }
  }
  // Default to NEC
  return IRInterface::int64SendTypes::NEC;
}

void IRAction::getData(protocolTy aProtocol, const std::string &aDataString) {
  if (std::holds_alternative<IRInterface::int64SendTypes>(aProtocol)) {
    auto protocol = std::get<IRInterface::int64SendTypes>(aProtocol);
    uint64_t value = std::stoull(aDataString, nullptr, 16);
    mData = std::make_pair(protocol, value);
  }
}
