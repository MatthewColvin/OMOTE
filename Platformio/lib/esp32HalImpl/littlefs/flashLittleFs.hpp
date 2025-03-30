#pragma once

#include "Hardware/Littlefs/LittleFsInterface.hpp"
#include "esp_partition.h"

class FlashLittleFs : public LittleFsInterface {
public:
  static std::shared_ptr<FlashLittleFs> getInstance();
  ~FlashLittleFs() override;

protected:
  Config getDataFormatConfig() override;
  int Read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off,
           void *buffer, lfs_size_t size) override;
  int Prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off,
           const void *buffer, lfs_size_t size) override;
  int Erase(const struct lfs_config *c, lfs_block_t block) override;
  int Sync(const struct lfs_config *c) override;

private:
  FlashLittleFs();

  const esp_partition_t *mPartition;
  bool mMounted;

  // Tightly coupled to the Rev1Partitions for setup.
  static constexpr const char *PARTITION_LABEL = "storage";
  static constexpr size_t READ_SIZE = 16;
  static constexpr size_t PROG_SIZE = 16;
  static constexpr size_t BLOCK_SIZE = 4096;
  // (475blocks * 4096bytes / 1024bytes) = 1900k size in Partitions file
  // Used one Less block because bumping right up to the
  // coredump was causing the coredumps to fail
  static constexpr size_t BLOCK_COUNT = 474;
  static constexpr size_t CACHE_SIZE = 512; // Increased for better performance
  static constexpr size_t LOOKAHEAD_SIZE = 128;
  static constexpr int32_t BLOCK_CYCLES = 500;
};
