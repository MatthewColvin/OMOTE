#pragma once
#include "HardwareFactory.hpp"
#include "IAction.hpp"
#include <cstdint>

class IRAction : public IAction {
public:
  IRAction(const std::string &aName, const std::string &aProtocol, const std::string &aHexData);

  void execute() override;

protected:
  IRInterface::protocol getProtocol(const std::string &aProtocolString);
  std::vector<uint16_t> hexStringToVector(const std::string &hexStr);

private:
  IRInterface::protocol mProtocol;
  std::vector<uint16_t> mData;
};
