#ifndef HARDWARE_REV1_HPP
#define HARDWARE_REV1_HPP

#include "HardwareRevX.hpp"

class HardwareRev1 : public HardwareRevX {
 public:
  HardwareRev1() = default;
  virtual ~HardwareRev1() = default;

  void init() override;

 private:
  void initIO() override;

  void sleepDisplayPins() override;
};

#endif  // HARDWARE_REV1_HPP