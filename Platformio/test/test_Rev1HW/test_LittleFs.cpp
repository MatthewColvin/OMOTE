#include <gtest/gtest.h>

#include <string>

#include "littlefs/flashLittleFs.hpp"

class TestLittleFs : public ::testing::Test {
 protected:
  void SetUp() override {
    fs = FlashLittleFs::getInstance();
    ASSERT_TRUE(fs != nullptr);
    ASSERT_TRUE(fs->mount());
  }

  void TearDown() override {
    if (fs) {
      fs->unmount();
    }
  }

  std::shared_ptr<FlashLittleFs> fs;
};

TEST_F(TestLittleFs, MountUnmount) {
  fs->unmount();
  EXPECT_TRUE(fs->mount());
}

TEST_F(TestLittleFs, WriteAndReadFile) {
  const char* testPath = "/test.txt";
  const std::string testData = "Hello, LittleFS!";

  // Write file
  lfs_file_t file;
  ASSERT_EQ(
      lfs_file_open(fs->get(), &file, testPath, LFS_O_WRONLY | LFS_O_CREAT), 0);
  ASSERT_EQ(
      lfs_file_write(fs->get(), &file, testData.c_str(), testData.length()),
      testData.length());
  ASSERT_EQ(lfs_file_close(fs->get(), &file), 0);

  // Read and verify
  char readBuffer[32] = {0};
  ASSERT_EQ(lfs_file_open(fs->get(), &file, testPath, LFS_O_RDONLY), 0);
  ASSERT_EQ(lfs_file_read(fs->get(), &file, readBuffer, testData.length()),
            testData.length());
  ASSERT_EQ(lfs_file_close(fs->get(), &file), 0);
  EXPECT_EQ(std::string(readBuffer), testData);
}

TEST_F(TestLittleFs, FileDelete) {
  const char* testPath = "/delete_test.txt";
  const char* testData = "Test Data";

  // Create file
  lfs_file_t file;
  ASSERT_EQ(
      lfs_file_open(fs->get(), &file, testPath, LFS_O_WRONLY | LFS_O_CREAT), 0);
  ASSERT_EQ(lfs_file_write(fs->get(), &file, testData, strlen(testData)),
            strlen(testData));
  ASSERT_EQ(lfs_file_close(fs->get(), &file), 0);

  // Delete file

  EXPECT_EQ(lfs_remove(fs->get(), testPath), 0);

  // Verify file is gone
  EXPECT_NE(lfs_file_open(fs->get(), &file, testPath, LFS_O_RDONLY), 0);
}

TEST_F(TestLittleFs, DirectoryOperations) {
  const char* dirPath = "/testdir";

  // Create directory
  ASSERT_EQ(lfs_mkdir(fs->get(), dirPath), 0);

  // Create file in directory
  lfs_file_t file;
  ASSERT_EQ(lfs_file_open(fs->get(), &file, "/testdir/test.txt",
                          LFS_O_WRONLY | LFS_O_CREAT),
            0);
  ASSERT_EQ(lfs_file_close(fs->get(), &file), 0);

  // Open directory and verify contents
  lfs_dir_t dir;
  struct lfs_info info;
  ASSERT_EQ(lfs_dir_open(fs->get(), &dir, dirPath), 0);

  bool foundFile = false;
  while (lfs_dir_read(fs->get(), &dir, &info) > 0) {
    if (std::string(info.name) == "test.txt") {
      foundFile = true;
      break;
    }
  }

  EXPECT_TRUE(foundFile);
  ASSERT_EQ(lfs_dir_close(fs->get(), &dir), 0);

  // Cleanup
  ASSERT_EQ(lfs_remove(fs->get(), "/testdir/test.txt"), 0);
  ASSERT_EQ(lfs_remove(fs->get(), dirPath), 0);
}

TEST_F(TestLittleFs, TestWriteReadInterfaceFile) {
  auto fileName = "/fileInterface.txt";
  std::string testStr = "This is a string test for the interface API";
  auto handleOpenError = []() { return; };
  {
    auto file = fs->open(fileName, LFS_O_WRONLY | LFS_O_CREAT);
    ASSERT_TRUE(file);
    file ? file.write(testStr) : handleOpenError();
    ASSERT_TRUE(file);
  }  // file closes on scope end
  {
    auto file = fs->open(fileName, LFS_O_RDONLY);
    ASSERT_TRUE(file);
    auto valOutOfFlash = file.read(testStr.length());
    ASSERT_TRUE(file);
    ASSERT_EQ(valOutOfFlash, testStr);
  }
  {
    auto file = fs->open(fileName, LFS_O_RDONLY);
    ASSERT_TRUE(file);
    auto valOutWithExtraSize = file.read(testStr.length() + 20);
    ASSERT_TRUE(file);
    ASSERT_EQ(valOutWithExtraSize, testStr);
  }
  {
    auto firstPart = testStr.substr(0, 4);
    auto secondPart = testStr.substr(firstPart.length());

    auto file = fs->open(fileName, LFS_O_RDONLY);
    ASSERT_TRUE(file);

    auto firstInFlash = file.read(firstPart.length());
    ASSERT_TRUE(file);
    ASSERT_EQ(firstPart, firstInFlash);

    auto theRestInFlash = file.read(secondPart.length());
    ASSERT_TRUE(file);
    ASSERT_EQ(secondPart, theRestInFlash);
  }
}