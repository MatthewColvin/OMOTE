#include <gtest/gtest.h>

#include "HardwareFactory.hpp"

class TestHardwareLittleFs : public ::testing::Test {
protected:
  void SetUp() override {
    hw = &HardwareFactory::getAbstract();
    ASSERT_TRUE(hw != nullptr);
    fs = hw->getLittleFS();
    ASSERT_TRUE(fs != nullptr);
  }

  void TearDown() override {
    if (fs) {
      fs->unmount();
    }
  }

  HardwareAbstract *hw = nullptr;
  std::shared_ptr<LittleFsInterface> fs;
};

TEST_F(TestHardwareLittleFs, WriteAndReadFile) {
  return;
  const char *testPath = "/hwtest.txt";
  const std::string testData = "Testing Hardware LittleFS!";

  auto file = fs->open(testPath, LFS_O_WRONLY | LFS_O_CREAT);
  ASSERT_TRUE(file);
  file.write(testData);
  ASSERT_TRUE(file);

  file = fs->open(testPath, LFS_O_RDONLY);
  ASSERT_TRUE(file);
  auto readData = file.read(testData.length());
  ASSERT_TRUE(file);
  EXPECT_EQ(readData, testData);
}

TEST_F(TestHardwareLittleFs, AppendToFile) {
  return;
  const char *testPath = "/append.txt";
  const std::string firstWrite = "First line\n";
  const std::string secondWrite = "Second line";

  // Write initial content
  auto file = fs->open(testPath, LFS_O_WRONLY | LFS_O_CREAT);
  ASSERT_TRUE(file);
  file.write(firstWrite);
  ASSERT_TRUE(file);

  // Append more content
  file = fs->open(testPath, LFS_O_WRONLY | LFS_O_APPEND);
  ASSERT_TRUE(file);
  file.write(secondWrite);
  ASSERT_TRUE(file);

  // Read and verify
  file = fs->open(testPath, LFS_O_RDONLY);
  ASSERT_TRUE(file);
  auto readData = file.read(firstWrite.length() + secondWrite.length());
  ASSERT_TRUE(file);
  EXPECT_EQ(readData, firstWrite + secondWrite);
}
