#include "LittlefsSim.hpp"
#include <cstring>
#include <filesystem>
#include <fstream>

/**
 * When using std::filesystem::Path in this file we cafonvert paths to
 * string then c_str() to ensure portablity with windows as
 * std::filesystem::Path uses a different value_type in windows
 */

std::shared_ptr<LittlefsSim> LittlefsSim::getInstance() {
  if (!mInstance) {
    mInstance = std::shared_ptr<LittlefsSim>(new LittlefsSim("flash.bin"));
  }
  return std::static_pointer_cast<LittlefsSim>(mInstance);
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

bool LittlefsSim::dumpDirectory(lfs_t *lfs, const char *path, const std::string &outputPath) {
  lfs_dir_t dir;
  lfs_info info;

  if (lfs_dir_open(lfs, &dir, path) < 0) {
    return false;
  }

  while (lfs_dir_read(lfs, &dir, &info) > 0) {
    if (strcmp(info.name, ".") == 0 || strcmp(info.name, "..") == 0) {
      continue;
    }

    std::string fullLfsPath = std::string(path) + "/" + info.name;
    std::string fullOutputPath = outputPath + "/" + info.name;

    if (info.type == LFS_TYPE_DIR) {
      std::filesystem::create_directories(fullOutputPath);
      dumpDirectory(lfs, fullLfsPath.c_str(), fullOutputPath);
    } else {
      lfs_file_t file;
      if (lfs_file_open(lfs, &file, fullLfsPath.c_str(), LFS_O_RDONLY) < 0) {
        continue;
      }

      std::ofstream outFile(fullOutputPath, std::ios::binary);
      if (!outFile.is_open()) {
        lfs_file_close(lfs, &file);
        continue;
      }

      uint8_t buffer[1024];
      lfs_ssize_t read;
      while ((read = lfs_file_read(lfs, &file, buffer, sizeof(buffer))) > 0) {
        outFile.write(reinterpret_cast<char *>(buffer), read);
      }

      lfs_file_close(lfs, &file);
    }
  }

  lfs_dir_close(lfs, &dir);
  return true;
}

bool LittlefsSim::dumpContentsToFolder(const std::string &outputPath) {
  if (mMounted) {
    std::filesystem::create_directories(outputPath);
    bool result = dumpDirectory(get(), "/", outputPath);
    return result;
  }
  return false;
}

bool LittlefsSim::initFolderContents(const std::string &inputPath) {
  if (mMounted) {
    bool result = initDirectory(get(), "/", inputPath);
    return result;
  }
  return false;
}

#include <regex>

bool LittlefsSim::initDirectory(lfs_t *lfs, const char *path, const std::string &inputPath) {
  lfs_dir_t dir;
  lfs_info info;

  lfs_format(lfs, &mConfig);
  lfs_mount(lfs, &mConfig);

  namespace fs = std::filesystem;
  for (const auto &entry : fs::recursive_directory_iterator(inputPath)) {
    std::filesystem::path lfsPath(std::regex_replace(entry.path().string(), std::regex("/data"), ""));
    if (entry.is_directory()) {

      lfs_mkdir(lfs, lfsPath.string().c_str());
    } else if (entry.is_regular_file()) {
      std::ifstream inFile(entry.path(), std::ios::binary);
      if (inFile.is_open()) {
        uint8_t buffer[1024];
        lfs_file_t file;
        lfs_file_open(lfs, &file, lfsPath.string().c_str(), LFS_O_WRONLY | LFS_O_CREAT);

        while (inFile.read(reinterpret_cast<char *>(buffer), sizeof(buffer))) {
          lfs_file_write(lfs, &file, buffer, sizeof(buffer));
        }
        int remBytes = inFile.gcount();
        if (remBytes > 0)
          lfs_file_write(lfs, &file, buffer, remBytes);

        inFile.close();
        lfs_file_close(lfs, &file);
      }
    }
  }
  return true;
}