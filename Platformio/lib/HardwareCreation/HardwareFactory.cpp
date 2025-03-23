#include "HardwareFactory.hpp"

#if defined(IS_SIMULATOR)
  #include "HardwareSimulator.hpp"
#else
  #include "HardwareRevX.hpp"
#endif

std::unique_ptr<HardwareAbstract> HardwareFactory::mHardware = nullptr;

void HardwareFactory::Init() {
#if defined(IS_SIMULATOR)
  mHardware = std::make_unique<HardwareSimulator>();
#else
  #if defined(OMOTE_HARDWARE_REV5)
    //#if defined(OMOTE_KEYBRD_3661)
    mHardware = std::make_unique<HardwareRevX>();
    //#else
    //  mHardware = std::make_unique<HardwareRevX>();
    //#endif
  #else
      mHardware = std::make_unique<HardwareRevX>();
  #endif
#endif
  mHardware->init();
}

HardwareAbstract &HardwareFactory::getAbstract() { return *mHardware; }