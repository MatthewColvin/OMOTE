#pragma once
#include "HardwareFactory.hpp"
#include "IAction.hpp"
#include <cstdint>
#include <variant>

class IRAction : public IAction {
public:
  IRAction(const std::string &aName, const std::string &aProtocol, const std::string &aHexData);

  void execute() override;

protected:
  using protocolTy = std::variant<IRInterface::int64SendTypes, IRInterface::constInt64SendTypes, IRInterface::charArrSendType>;

  protocolTy getProtocol(const std::string &aProtocolString);
  void getData(protocolTy aProtocol, const std::string &aDataString);

private:
  using int64IrPair = std::pair<IRInterface::int64SendTypes, uint64_t>;
  using constInt64IrPair = std::pair<IRInterface::constInt64SendTypes, const uint64_t>;
  using constCharArrPair = std::pair<IRInterface::charArrSendType, char *>;

  std::variant<int64IrPair, constInt64IrPair, constCharArrPair> mData;
};
