#include "LittleFsInterface.hpp"

bool LittleFsInterface::mount() {
  if (!mInited) {
    init();
  }
  if (mMounted) {
    return false;
  }
  int err = lfs_mount(&mLfs, &mConfig);
  if (err) {
    // First time mount might fail, try formatting
    err = lfs_format(&mLfs, &mConfig);
    if (err)
      return false;

    err = lfs_mount(&mLfs, &mConfig);
    if (err)
      return false;
  }
  mMounted = true;
  return true;
}

void LittleFsInterface::unmount() {
  if (mMounted) {
    lfs_unmount(&mLfs);
    mMounted = false;
  }
}

int LittleFsInterface::ReadImpl(const lfs_config *c, lfs_block_t block,
                                lfs_off_t off, void *buffer, lfs_size_t size) {
  if (mInstance) {
    return mInstance->Read(c, block, off, buffer, size);
  }
  return LFSInstanceNeverCreated;
}

int LittleFsInterface::ProgImpl(const lfs_config *c, lfs_block_t block,
                                lfs_off_t off, const void *buffer,
                                lfs_size_t size) {
  if (mInstance) {
    return mInstance->Prog(c, block, off, buffer, size);
  }
  return LFSInstanceNeverCreated;
}

int LittleFsInterface::EraseImpl(const lfs_config *c, lfs_block_t block) {
  if (mInstance) {
    return mInstance->Erase(c, block);
  }
  return LFSInstanceNeverCreated;
}

int LittleFsInterface::SyncImpl(const lfs_config *c) {
  if (mInstance) {
    return mInstance->Sync(c);
  }
  return LFSInstanceNeverCreated;
}

void LittleFsInterface::init() {
  auto aConfig = getDataFormatConfig();
  mConfig.read = ReadImpl;
  mConfig.prog = ProgImpl;
  mConfig.erase = EraseImpl;
  mConfig.sync = SyncImpl;
  mConfig.read_size = aConfig.readSize;
  mConfig.prog_size = aConfig.progSize;
  mConfig.block_size = aConfig.blockSize;
  mConfig.block_count = aConfig.blockCount;
  mConfig.block_cycles = aConfig.blockCycles;
  mConfig.cache_size = aConfig.cacheSize;
  mConfig.lookahead_size = aConfig.lookaheadSize;
}