#include "flashLittleFs.hpp"

#include <cstring>

#include "esp_system.h"

FlashLittleFs::FlashLittleFs(uint16_t aBlockSize,
                             const std::string &aPartitionLabel) : mBlockSize(aBlockSize),
                                                                   mPartition(nullptr),
                                                                   mMounted(false) {
  mPartition = esp_partition_find_first(
      ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS,
      aPartitionLabel.c_str());
}

FlashLittleFs::~FlashLittleFs() {
  if (mMounted) {
    unmount();
  }
}

int FlashLittleFs::Read(const struct lfs_config *c, lfs_block_t block,
                        lfs_off_t off, void *buffer, lfs_size_t size) {
  if (!mPartition)
    return LFS_ERR_IO;

  size_t addr = block * mBlockSize + off;
  return esp_partition_read(mPartition, addr, buffer, size) == ESP_OK
             ? 0
             : LFS_ERR_IO;
}

int FlashLittleFs::Prog(const struct lfs_config *c, lfs_block_t block,
                        lfs_off_t off, const void *buffer, lfs_size_t size) {
  if (!mPartition)
    return LFS_ERR_IO;

  size_t addr = block * mBlockSize + off;
  return esp_partition_write(mPartition, addr, buffer, size) == ESP_OK
             ? 0
             : LFS_ERR_IO;
}

int FlashLittleFs::Erase(const struct lfs_config *c, lfs_block_t block) {
  if (!mPartition)
    return LFS_ERR_IO;

  size_t addr = block * mBlockSize;
  return esp_partition_erase_range(mPartition, addr, mBlockSize) == ESP_OK
             ? 0
             : LFS_ERR_IO;
}

int FlashLittleFs::Sync(const struct lfs_config *c) {
  return 0; // ESP32 flash writes are synchronous
}
