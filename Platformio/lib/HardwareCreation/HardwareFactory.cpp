#include "HardwareFactory.hpp"

#if defined(IS_SIMULATOR)
#include "HardwareSimulator.hpp"
#elif (defined(OMOTE_HARDWARE_REV5))
#include "HardwareRev5.hpp"
#else
#include "HardwareRev1.hpp"
#endif

std::unique_ptr<HardwareAbstract> HardwareFactory::mHardware = nullptr;

void HardwareFactory::Init() {
#if defined(IS_SIMULATOR)
  mHardware = std::make_unique<HardwareSimulator>();
#elif (defined(OMOTE_HARDWARE_REV5))
  mHardware = std::make_unique<HardwareRev5>();
#else
  mHardware = std::make_unique<HardwareRev1>();
#endif
  mHardware->init();
}

HardwareAbstract &HardwareFactory::getAbstract() { return *mHardware; }