#if defined(IS_SIMULATOR)

#include <cstdio>
#include <cstdlib>
#include <exception>

namespace {

void simOnTerminate() {
  std::fflush(stdout);
  std::fflush(stderr);
  if (auto e = std::current_exception()) {
    try {
      std::rethrow_exception(e);
    } catch (const std::exception &ex) {
      std::fprintf(stderr, "[sim] terminate: %s\n", ex.what());
    } catch (...) {
      std::fprintf(stderr, "[sim] terminate: unknown exception\n");
    }
  } else {
    std::fprintf(stderr, "[sim] terminate (no active exception)\n");
  }
  std::fflush(stderr);
  std::abort();
}

} // namespace

void simOnExit() {
  std::fprintf(stderr, "[sim] process exiting\n");
  std::fflush(stderr);
}

void simInstallCrashDiagnostics() {
  std::set_terminate(simOnTerminate);
  std::atexit(simOnExit);
  setvbuf(stdout, nullptr, _IOLBF, 0);
  setvbuf(stderr, nullptr, _IOLBF, 0);
  std::printf("[sim] crash diagnostics on (log = this terminal)\n");
  std::fflush(stdout);
}

#else

void simInstallCrashDiagnostics() {}

#endif
