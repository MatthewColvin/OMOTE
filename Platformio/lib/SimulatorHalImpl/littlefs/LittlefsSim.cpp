#include "LittlefsSim.hpp"
#include <cstring>
#include <fstream>

std::shared_ptr<LittleFsInterface> LittlefsSim::getInstance() {
  if (!mInstance) {
    mInstance = std::shared_ptr<LittlefsSim>(new LittlefsSim("flash.bin"));
  }
  return mInstance;
}

LittlefsSim::LittlefsSim(const std::string &flashFile)
    : mFlashFile(flashFile), mDirty(false) {
  mFlashMemory.resize(BLOCK_SIZE * BLOCK_COUNT, 0xFF);
  loadFlashFile();
}

LittlefsSim::~LittlefsSim() {
  if (mDirty) {
    saveFlashFile();
  }
}

LittleFsInterface::Config LittlefsSim::getDataFormatConfig() {
  return Config{
      .readSize = READ_SIZE,
      .progSize = PROG_SIZE,
      .blockSize = BLOCK_SIZE,
      .blockCount = BLOCK_COUNT,
      .cacheSize = CACHE_SIZE,
      .lookaheadSize = LOOKAHEAD_SIZE,
      .blockCycles = BLOCK_CYCLES};
}

int LittlefsSim::Read(const struct lfs_config *c, lfs_block_t block,
                      lfs_off_t off, void *buffer, lfs_size_t size) {
  size_t addr = block * BLOCK_SIZE + off;
  if (addr + size > mFlashMemory.size()) {
    return LFS_ERR_IO;
  }
  std::memcpy(buffer, mFlashMemory.data() + addr, size);
  return 0;
}

int LittlefsSim::Prog(const struct lfs_config *c, lfs_block_t block,
                      lfs_off_t off, const void *buffer, lfs_size_t size) {
  size_t addr = block * BLOCK_SIZE + off;
  if (addr + size > mFlashMemory.size()) {
    return LFS_ERR_IO;
  }
  std::memcpy(mFlashMemory.data() + addr, buffer, size);
  mDirty = true;
  return 0;
}

int LittlefsSim::Erase(const struct lfs_config *c, lfs_block_t block) {
  size_t addr = block * BLOCK_SIZE;
  if (addr >= mFlashMemory.size()) {
    return LFS_ERR_IO;
  }
  std::fill_n(mFlashMemory.data() + addr, BLOCK_SIZE, 0xFF);
  mDirty = true;
  return 0;
}

int LittlefsSim::Sync(const struct lfs_config *c) {
  return saveFlashFile() ? 0 : LFS_ERR_IO;
}

bool LittlefsSim::loadFlashFile() {
  std::ifstream file(mFlashFile, std::ios::binary);
  if (!file.is_open()) {
    return false;
  }
  file.read(reinterpret_cast<char *>(mFlashMemory.data()), mFlashMemory.size());
  return true;
}

bool LittlefsSim::saveFlashFile() {
  std::ofstream file(mFlashFile, std::ios::binary);
  if (!file.is_open()) {
    return false;
  }
  file.write(reinterpret_cast<const char *>(mFlashMemory.data()), mFlashMemory.size());
  mDirty = false;
  return true;
}
