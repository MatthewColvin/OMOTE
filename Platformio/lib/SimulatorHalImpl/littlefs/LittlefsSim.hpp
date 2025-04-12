#pragma once

#include "Hardware/Littlefs/LittleFsInterface.hpp"
#include <string>
#include <vector>

class LittlefsSim : public LittleFsInterface {
public:
  static std::shared_ptr<LittlefsSim> getInstance();
  ~LittlefsSim() override;

  bool dumpContentsToFolder(const std::string &outputPath);
  bool initFolderContents(const std::string &inputPath);

protected:
  LittlefsSim(const std::string &flashFile);
  Config getDataFormatConfig() override;
  int Read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off,
           void *buffer, lfs_size_t size) override;
  int Prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off,
           const void *buffer, lfs_size_t size) override;
  int Erase(const struct lfs_config *c, lfs_block_t block) override;
  int Sync(const struct lfs_config *c) override;

private:
  static constexpr lfs_size_t READ_SIZE = 16;
  static constexpr lfs_size_t PROG_SIZE = 16;
  static constexpr lfs_size_t BLOCK_SIZE = 4096;
  static constexpr lfs_size_t BLOCK_COUNT = 256;
  static constexpr lfs_size_t CACHE_SIZE = 16;
  static constexpr lfs_size_t LOOKAHEAD_SIZE = 16;
  static constexpr int32_t BLOCK_CYCLES = 500;

  bool loadFlashFile();
  bool saveFlashFile();
  bool dumpDirectory(lfs_t *lfs, const char *path, const std::string &outputPath);
  bool initDirectory(lfs_t *lfs, const char *path, const std::string &inputPath);
  bool createDirectoryRecursive(const std::string &path);

  std::vector<uint8_t> mFlashMemory;
  std::string mFlashFile;
  bool mDirty;
};
