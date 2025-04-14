#include "Rev5LittleFs.hpp"

std::shared_ptr<LittleFsInterface> Rev5LittleFs::getInstance() {
  if (!mInstance) {
    mInstance = std::shared_ptr<Rev5LittleFs>(new Rev5LittleFs());
  }
  return mInstance;
}

Rev5LittleFs::Rev5LittleFs() : FlashLittleFs(BLOCK_SIZE, PARTITION_LABEL) {}

LittleFsInterface::Config Rev5LittleFs::getDataFormatConfig() {
  return Config{
      .readSize = READ_SIZE,
      .progSize = PROG_SIZE,
      .blockSize = BLOCK_SIZE,
      .blockCount = BLOCK_COUNT,
      .cacheSize = CACHE_SIZE,
      .lookaheadSize = LOOKAHEAD_SIZE,
      .blockCycles = BLOCK_CYCLES};
}
