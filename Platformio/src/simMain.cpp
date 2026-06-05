#include "HardwareFactory.hpp"
#include "OmoteSetup.hpp"
#include "sim_crash_diag.hpp"

extern "C" unsigned long millis(void) {
  return HardwareFactory::getAbstract().getMillis();
}

int main() {
  simInstallCrashDiagnostics();
  OMOTE::setup();
  while (true) {
    OMOTE::loop();
  }
}