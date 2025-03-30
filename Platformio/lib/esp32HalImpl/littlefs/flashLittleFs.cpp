#include "flashLittleFs.hpp"

#include <cstring>

#include "esp_system.h"

FlashLittleFs::FlashLittleFs() : mPartition(nullptr), mMounted(false) {
  mPartition = esp_partition_find_first(
      ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, PARTITION_LABEL);
}

std::shared_ptr<FlashLittleFs> FlashLittleFs::getInstance() {
  if (!mInstance) {
    mInstance = std::shared_ptr<LittleFsInterface>(new FlashLittleFs());
  }
  return std::static_pointer_cast<FlashLittleFs>(mInstance);
}

FlashLittleFs::~FlashLittleFs() {
  if (mMounted) {
    unmount();
  }
}

LittleFsInterface::Config FlashLittleFs::getDataFormatConfig() {
  return Config{.readSize = READ_SIZE,
                .progSize = PROG_SIZE,
                .blockSize = BLOCK_SIZE,
                .blockCount = BLOCK_COUNT,
                .cacheSize = CACHE_SIZE,
                .lookaheadSize = LOOKAHEAD_SIZE,
                .blockCycles = BLOCK_CYCLES};
}

int FlashLittleFs::Read(const struct lfs_config *c, lfs_block_t block,
                        lfs_off_t off, void *buffer, lfs_size_t size) {
  if (!mPartition)
    return LFS_ERR_IO;

  size_t addr = block * BLOCK_SIZE + off;
  return esp_partition_read(mPartition, addr, buffer, size) == ESP_OK
             ? 0
             : LFS_ERR_IO;
}

int FlashLittleFs::Prog(const struct lfs_config *c, lfs_block_t block,
                        lfs_off_t off, const void *buffer, lfs_size_t size) {
  if (!mPartition)
    return LFS_ERR_IO;

  size_t addr = block * BLOCK_SIZE + off;
  return esp_partition_write(mPartition, addr, buffer, size) == ESP_OK
             ? 0
             : LFS_ERR_IO;
}

int FlashLittleFs::Erase(const struct lfs_config *c, lfs_block_t block) {
  if (!mPartition)
    return LFS_ERR_IO;

  size_t addr = block * BLOCK_SIZE;
  return esp_partition_erase_range(mPartition, addr, BLOCK_SIZE) == ESP_OK
             ? 0
             : LFS_ERR_IO;
}

int FlashLittleFs::Sync(const struct lfs_config *c) {
  return 0; // ESP32 flash writes are synchronous
}
