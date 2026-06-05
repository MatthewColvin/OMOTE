#pragma once

/** Minimal Arduino stand-in for the desktop simulator (millis provided in simMain.cpp). */
#include <cstdio>
#include <cstdint>

extern "C" unsigned long millis(void);

struct SimSerial {
  void println(const char *msg) const {
    std::printf("%s\n", msg);
    std::fflush(stdout);
  }
  template <typename... Args>
  void printf(const char *fmt, Args... args) const {
    std::printf(fmt, args...);
    std::fflush(stdout);
  }
};

inline SimSerial Serial;
