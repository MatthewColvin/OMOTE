#include <gtest/gtest.h>

#include "HardwareFactory.hpp"
#include "rapidjson/writer.h"

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

TEST_F(TestHardwareLittleFs, CreateAndVerifyConfig) {
  // Create test config document
  rapidjson::Document doc;
  doc.SetObject();
  auto &allocator = doc.GetAllocator();

  constexpr auto NameKey = "name";
  constexpr auto VersionKey = "version";

  constexpr auto SettingsKey = "settings";
  constexpr auto EnableKey = "enabled";
  constexpr auto IntervalKey = "interval";
  constexpr auto ThresholdKey = "threshold";

  constexpr auto TestKey = "test";
  constexpr auto DebugKey = "debug";
  constexpr auto TagsKey = "tags";

  // Build config structure
  doc.AddMember("name", "TestDevice", allocator);
  doc.AddMember("version", 1, allocator);

  rapidjson::Value settings(rapidjson::kObjectType);
  settings.AddMember("enabled", true, allocator);
  settings.AddMember("interval", 1000, allocator);
  settings.AddMember("threshold", 3.14, allocator);
  doc.AddMember("settings", settings, allocator);

  rapidjson::Value tags(rapidjson::kArrayType);
  tags.PushBack("test", allocator);
  tags.PushBack("debug", allocator);
  doc.AddMember("tags", tags, allocator);

  // Write to file
  const char *testPath = "/TestConfig.json";
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  doc.Accept(writer);

  auto file = fs->open(testPath, LFS_O_WRONLY | LFS_O_CREAT);
  ASSERT_TRUE(file);
  file.write(buffer.GetString());
  ASSERT_TRUE(file);

  // Read and verify
  file = fs->open(testPath, LFS_O_RDONLY);
  ASSERT_TRUE(file);
  auto readData = file.read(buffer.GetLength());
  ASSERT_TRUE(file);
  EXPECT_EQ(readData, buffer.GetString());

  // Parse and verify content
  rapidjson::Document readDoc;
  readDoc.Parse(readData.c_str());

  EXPECT_TRUE(readDoc.HasMember("name"));
  EXPECT_STREQ(readDoc["name"].GetString(), "TestDevice");
  EXPECT_EQ(readDoc["version"].GetInt(), 1);
  EXPECT_TRUE(readDoc["settings"]["enabled"].GetBool());
  EXPECT_EQ(readDoc["settings"]["interval"].GetInt(), 1000);
  EXPECT_DOUBLE_EQ(readDoc["settings"]["threshold"].GetDouble(), 3.14);
  EXPECT_EQ(readDoc["tags"].Size(), 2);
  EXPECT_STREQ(readDoc["tags"][0].GetString(), "test");
  EXPECT_STREQ(readDoc["tags"][1].GetString(), "debug");
}