#include "IRAction.hpp"

IRAction::IRAction(const std::string &aName, const std::string &aProtocol, const std::string &aHexData)
    : IAction(aName), mProtocol(getProtocol(aProtocol)), mData(hexStringToVector(aHexData)) {}

void IRAction::execute() {
    auto &hardware = HardwareFactory::getAbstract();
    auto ir = hardware.ir();
    ir->send({mProtocol, mData});
}

IRInterface::protocol IRAction::getProtocol(const std::string &aProtocolString) {
    return IRInterface::protocol::UNKNOWN;
}

std::vector<uint16_t> IRAction::hexStringToVector(const std::string &hexStr) {
    std::vector<uint16_t> result;
    std::string cleanHex = hexStr.substr(hexStr.find("0x") == 0 ? 2 : 0);

    for (size_t i = 0; i < cleanHex.length(); i += 2) {
        std::string byteStr = cleanHex.substr(i, 2);
        uint16_t value = std::stoi(byteStr, nullptr, 16);
        result.push_back(value);
    }
    return result;
}
