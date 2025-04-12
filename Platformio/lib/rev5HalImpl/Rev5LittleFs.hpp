#pragma once

#include "../esp32HalImpl/littlefs/flashLittleFs.hpp"

class Rev5LittleFs : public FlashLittleFs {
public:
  static std::shared_ptr<LittleFsInterface> getInstance();

protected:
  Config getDataFormatConfig() override;

private:
  Rev5LittleFs();

  // Rev1 specific configuration very tightly coupled to the
  // Rev1Partitions.csv!!
  static constexpr const char *PARTITION_LABEL = "storage";
  static constexpr size_t READ_SIZE = 16;
  static constexpr size_t PROG_SIZE = 16;
  static constexpr size_t BLOCK_SIZE = 4096;
  static constexpr size_t BLOCK_COUNT = 0;//375; // (375blocks * 4096bytes / 1024bytes) = 1500k
  static constexpr size_t CACHE_SIZE = 512;
  static constexpr size_t LOOKAHEAD_SIZE = 128;
  static constexpr int32_t BLOCK_CYCLES = 500;
};
