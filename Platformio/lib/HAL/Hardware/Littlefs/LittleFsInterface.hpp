#pragma once

#include <memory>
#include <vector>

#include "Hardware/Littlefs/File.hpp"
#include "lfs.h"
class LittleFsInterface {
public:
  static constexpr auto LFSInstanceNeverCreated = -10;
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

  lfs_t *get() { return &mLfs; }

  bool mount();
  void unmount();

  File open(std::string aFilePath, int aFlags = LFS_O_RDWR | LFS_O_CREAT) {
    return File(aFilePath, get(), aFlags);
  }

  std::vector<File> FilesIn(const std::string &aDirectory, int aFlags = LFS_O_RDWR);

  bool isDir(const std::string &aPath);
  bool isFile(const std::string &aPath);

protected:
  void init();

  virtual Config getDataFormatConfig() = 0;

  virtual int Read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off,
                   void *buffer, lfs_size_t size) = 0;
  virtual int Prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off,
                   const void *buffer, lfs_size_t size) = 0;
  virtual int Erase(const struct lfs_config *c, lfs_block_t block) = 0;
  virtual int Sync(const struct lfs_config *c) = 0;

  // Set this with a getInstance method in the Child Class
  static inline std::shared_ptr<LittleFsInterface> mInstance;
  bool mMounted = false;
  lfs_config mConfig{0};

private:
  bool isType(const std::string &aPath, lfs_type aType);

  static int ReadImpl(const lfs_config *c, lfs_block_t block, lfs_off_t off,
                      void *buffer, lfs_size_t size);
  static int ProgImpl(const lfs_config *c, lfs_block_t block, lfs_off_t off,
                      const void *buffer, lfs_size_t size);
  static int EraseImpl(const lfs_config *c, lfs_block_t block);
  static int SyncImpl(const lfs_config *c);

  bool mInited = false;
  lfs_t mLfs{0};
};