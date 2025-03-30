#pragma once

#include <algorithm>
#include <array>
#include <string>

#include "lfs.h"

class File {
 public:
  using lfsStatusCode = int;

  File(std::string aFileName, lfs_t *aLfs,
       int aFlags = LFS_O_RDWR | LFS_O_CREAT)
      : mLfs(aLfs) {
    mLastStatus = lfs_file_open(mLfs, &mFile, aFileName.c_str(), aFlags);
    mIsOpen = mLastStatus == 0;
  }

  // Don't allow copy as then you would could have double close issues
  File(const File &) = delete;
  File &operator=(const File &) = delete;
  // Moving is fine as then you will be fine with the close
  File(File &&) = default;
  File &operator=(File &&) = default;

  virtual ~File() { close(); }

  std::string read(size_t aMaxReadSize) {
    auto totalSize = lfs_file_size(mLfs, &mFile);
    size_t sizeToFileEnd = totalSize - mFile.pos;
    auto readSize = std::min(aMaxReadSize, sizeToFileEnd);

    std::string readData;
    readData.resize(readSize);
    auto bytesRead =
        lfs_file_read(mLfs, &mFile, readData.data(), readData.size());
    return bytesRead > 0 ? readData : "";
  }

  void write(const std::string &aStringToWrite) {
    auto bytesWritten = lfs_file_write(mLfs, &mFile, aStringToWrite.c_str(),
                                       aStringToWrite.length());
    if (bytesWritten != aStringToWrite.length()) {
      mLastStatus = -9;  // Todo name this?
    }
  }

  lfs_soff_t seek(lfs_soff_t offset, int whence) {
    return lfs_file_seek(mLfs, &mFile, offset, whence);
  }

  int tell(lfs_soff_t &offset) { return lfs_file_tell(mLfs, &mFile); }

  int size(lfs_soff_t &size) { return lfs_file_size(mLfs, &mFile); }

  operator bool() const { return mLastStatus == 0; }

 protected:
  // No need to manually call it will call on falling out of scope
  void close() {
    if (mIsOpen) {
      mLastStatus = lfs_file_close(mLfs, &mFile);
    }
  }

 private:
  lfs_file_t mFile{0};
  lfs_t *mLfs;
  lfsStatusCode mLastStatus = 0;
  bool mIsOpen = false;
};
