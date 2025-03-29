

#include <gmock/gmock.h>

#include "OmoteSetup.hpp"
#if (defined(IS_SIMULATOR))
int main(int argc, char **argv) {
  ::testing::InitGoogleMock(&argc, argv);
  if (RUN_ALL_TESTS())
    ;
  // Always return zero-code and allow PlatformIO to parse results
  return 0;
}
#else
#include <Arduino.h>

static bool sRanTests = false;
void setup() {
  OMOTE::setup();
  ::testing::InitGoogleMock();
}

void loop() {
  OMOTE::loop();
  // Wait for 2 seconds to run tests
  if (millis() > 2000 && !sRanTests) {
    // Run tests
    auto& hw = HardwareFactory::getAbstract();
    auto testsPassed = RUN_ALL_TESTS();
    auto debugMessage = testsPassed ? "Tests Passed" : "Tests Failed";
    hw.debugPrint(debugMessage);
    sRanTests = true;
  }
}

#endif