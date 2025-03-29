#pragma once

#include <memory>

#include "lfs.h"
class LittleFsInterface {
 public:
  struct Config {
    lfs_size_t readSize;
    lfs_size_t progSize;
    lfs_size_t blockSize;
    lfs_size_t blockCount;
    lfs_size_t cacheSize;
    lfs_size_t lookaheadSize;
    int32_t blockCycles;
  };

  virtual ~LittleFsInterface() = default;

  void init();
  virtual lfs *get();

 protected:
  virtual Config getDataFormatConfig() = 0;

  virtual int Read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off,
                   void *buffer, lfs_size_t size) = 0;
  virtual int Prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off,
                   const void *buffer, lfs_size_t size) = 0;
  virtual int Erase(const struct lfs_config *c, lfs_block_t block) = 0;
  virtual int Sync(const struct lfs_config *c) = 0;

  // Set this with a getInstance method in the Child Class
  static inline std::shared_ptr<LittleFsInterface> mInstance;

 private:
  static int ReadImpl(const lfs_config *c, lfs_block_t block, lfs_off_t off,
                      void *buffer, lfs_size_t size);
  static int ProgImpl(const lfs_config *c, lfs_block_t block, lfs_off_t off,
                      const void *buffer, lfs_size_t size);
  static int EraseImpl(const lfs_config *c, lfs_block_t block);
  static int SyncImpl(const lfs_config *c);
};

inline int LittleFsInterface::ReadImpl(const lfs_config *c, lfs_block_t block,
                                       lfs_off_t off, void *buffer,
                                       lfs_size_t size) {
  if (mInstance) {
    return mInstance->Read(c, block, off, buffer, size);
  }
}

inline int LittleFsInterface::ProgImpl(const lfs_config *c, lfs_block_t block,
                                       lfs_off_t off, const void *buffer,
                                       lfs_size_t size) {
  if (mInstance) {
    return mInstance->Prog(c, block, off, buffer, size);
  }
}

inline int LittleFsInterface::EraseImpl(const lfs_config *c,
                                        lfs_block_t block) {
  if (mInstance) {
    return mInstance->Erase(c, block);
  }
}

inline int LittleFsInterface::SyncImpl(const lfs_config *c) {
  if (mInstance) {
    return mInstance->Sync(c);
  }
}

inline void LittleFsInterface::init() {
  auto aConfig = getDataFormatConfig();

  const struct lfs_config cfg = {// block device operations
                                 .read = ReadImpl,
                                 .prog = ProgImpl,
                                 .erase = EraseImpl,
                                 .sync = SyncImpl,

                                 // block device configuration
                                 .read_size = aConfig.readSize,
                                 .prog_size = aConfig.progSize,
                                 .block_size = aConfig.blockSize,
                                 .block_count = aConfig.blockCount,
                                 .block_cycles = aConfig.blockCycles,
                                 .cache_size = aConfig.cacheSize,
                                 .lookahead_size = aConfig.lookaheadSize

  };
}