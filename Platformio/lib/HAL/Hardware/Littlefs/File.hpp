#pragma once

#include <algorithm>
#include <array>
#include <cstring>
#include <string>

#include "lfs.h"

class File {
public:
  using lfsStatusCode = int;

  File(const std::string &aFileName, lfs_t *aLfs,
       int aFlags = LFS_O_RDWR | LFS_O_CREAT)
      : mLfs(aLfs),
        mFile(std::make_unique<lfs_file_t>()),
        mPath(aFileName) {
    lfs_info info;
    if (lfs_stat(mLfs, aFileName.c_str(), &info) == 0) {
      if (info.type == LFS_TYPE_REG) {
        mLastStatus = lfs_file_open(mLfs, mFile.get(), aFileName.c_str(), aFlags);
        mIsOpen = mLastStatus == 0;
      }
    }
  }

  // Don't allow copy as then you would could have double close issues
  File(const File &) = delete;
  File &operator=(const File &) = delete;

  // Moving is fine as then you will be fine with the close
  File(File &&aOther) noexcept
      : mFile(std::move(aOther.mFile)),
        mPath(aOther.mPath),
        mLfs(aOther.mLfs),
        mLastStatus(aOther.mLastStatus),
        mIsOpen(aOther.mIsOpen) {
    // Clear the source object's members WITHOUT closing the file
    aOther.mLfs = nullptr;
    aOther.mIsOpen = false;
  }

  File &operator=(File &&aOther) noexcept {
    if (this != &aOther) {
      // Close our current file if we have one
      close();

      // Take ownership of the other file's members
      mFile = std::move(aOther.mFile);
      mLfs = aOther.mLfs;
      mLastStatus = aOther.mLastStatus;
      mIsOpen = aOther.mIsOpen;
      mPath = aOther.mPath;

      // Clear the source object's members WITHOUT closing the file
      aOther.mLfs = nullptr;
      aOther.mIsOpen = false;
    }
    return *this;
  }

  virtual ~File() {
    close();
  }

  std::string read(size_t aMaxReadSize) {
    if (!mLfs) {
      return "No LFS Ptr";
    }
    auto totalSize = lfs_file_size(mLfs, mFile.get());
    size_t sizeToFileEnd = totalSize - mFile->pos;
    auto readSize = std::min(aMaxReadSize, sizeToFileEnd);

    std::string readData;
    readData.resize(readSize);
    auto bytesRead =
        lfs_file_read(mLfs, mFile.get(), readData.data(), readData.size());
    return bytesRead > 0 ? readData : "";
  }

  void write(const std::string &aStringToWrite) {
    if (!mLfs) {
      return;
    }
    auto bytesWritten = lfs_file_write(mLfs, mFile.get(), aStringToWrite.c_str(),
                                       aStringToWrite.length());
    if (bytesWritten != aStringToWrite.length()) {
      mLastStatus = -9; // Todo name this?
    }
  }

  lfs_soff_t seek(lfs_soff_t offset, int whence) {
    if (!mLfs) {
      return 0;
    }
    return lfs_file_seek(mLfs, mFile.get(), offset, whence);
  }

  int tell(lfs_soff_t &offset) {
    if (!mLfs) {
      return 0;
    }
    return lfs_file_tell(mLfs, mFile.get());
  }

  int size() {
    if (!mLfs) {
      return 0;
    }
    return lfs_file_size(mLfs, mFile.get());
  }

  int truncate(lfs_off_t aTruncationSize) {
    return lfs_file_truncate(mLfs, mFile.get(), aTruncationSize);
  }

  std::string GetPath() const {
    return mPath;
  }

  operator bool() const { return mLfs && mLastStatus == 0 && mIsOpen; }

protected:
  // No need to manually call it will call on falling out of scope
  void close() {
    if (mLfs && mIsOpen) {
      mLastStatus = lfs_file_close(mLfs, mFile.get());
    }
  }

private:
  std::unique_ptr<lfs_file_t> mFile = nullptr;
  std::string mPath;
  lfs_t *mLfs;
  lfsStatusCode mLastStatus = 0;
  bool mIsOpen = false;
};
