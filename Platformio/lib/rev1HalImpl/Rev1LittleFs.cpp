#include "Rev1LittleFs.hpp"

std::shared_ptr<LittleFsInterface> Rev1LittleFs::getInstance() {
  if (!mInstance) {
    mInstance = std::shared_ptr<Rev1LittleFs>(new Rev1LittleFs());
  }
  return mInstance;
}

Rev1LittleFs::Rev1LittleFs() : FlashLittleFs(BLOCK_SIZE, PARTITION_LABEL) {}

LittleFsInterface::Config Rev1LittleFs::getDataFormatConfig() {
  return Config{
      .readSize = READ_SIZE,
      .progSize = PROG_SIZE,
      .blockSize = BLOCK_SIZE,
      .blockCount = BLOCK_COUNT,
      .cacheSize = CACHE_SIZE,
      .lookaheadSize = LOOKAHEAD_SIZE,
      .blockCycles = BLOCK_CYCLES};
}
