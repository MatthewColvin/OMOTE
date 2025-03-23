#ifndef HARDWARE_REV1_HPP
#define HARDWARE_REV1_HPP

#include "HardwareRevX.hpp"

class HardwareRev1 : public HardwareRevX {
 public:
  HardwareRev1() = default;
  virtual ~HardwareRev1() = default;

 private:
  void initIO() override;
};

#endif  // HARDWARE_REV1_HPP