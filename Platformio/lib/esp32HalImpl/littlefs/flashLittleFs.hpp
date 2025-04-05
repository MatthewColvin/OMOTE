#pragma once

#include "Hardware/Littlefs/LittleFsInterface.hpp"
#include "esp_partition.h"

class FlashLittleFs : public LittleFsInterface {
public:
  ~FlashLittleFs() override;

protected:
  int Read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off,
           void *buffer, lfs_size_t size) override;
  int Prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off,
           const void *buffer, lfs_size_t size) override;
  int Erase(const struct lfs_config *c, lfs_block_t block) override;
  int Sync(const struct lfs_config *c) override;

  FlashLittleFs(uint16_t aBlockSize, const std::string &aPartitionLabel);

protected:
  uint16_t mBlockSize;

  const esp_partition_t *mPartition;
  bool mMounted;
};
