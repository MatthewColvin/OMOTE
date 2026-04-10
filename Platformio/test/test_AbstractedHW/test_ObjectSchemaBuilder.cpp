#include "ObjectSchemaBuilder.hpp"
#include <algorithm>
#include <gtest/gtest.h>

// Simple test for ObjectSchemaBuilder functionality
TEST(ObjectSchemaBuilderTest, BasicFunctionality) {
  // Test that we can create a schema with required and optional fields
  auto schema = ObjectSchema()
                    .Require("name", "string")
                    .Optional("age", "integer")
                    .Build();

  // Test the build method returns a non-empty schema
  ASSERT_NE(schema.data(), nullptr);
}

// Test the size calculation works correctly
TEST(ObjectSchemaBuilderTest, SizeCalculation) {
  auto schema = ObjectSchema()
                    .Require("id", "string")
                    .Build();

  size_t size = schema.size();
  ASSERT_GT(size, 0);
}

TEST(ObjectSchemaBuilderTest, CalculateRequiredListSize) {
  // Strings keys will be quoted and comma-separated.
  constexpr ObjectSchemaBuilder<>::Member m1{"testKey", "string", true};
  constexpr auto expectedSize = ObjectSchemaBuilder<>::CalculateRequiredListSize(m1);
  ASSERT_EQ(expectedSize, 7 + 2);
  // (7)"testKey" + (2)quotes

  constexpr ObjectSchemaBuilder<>::Member m2{"anotherKey", "integer", true};
  constexpr auto expectedSizeTwoMems = ObjectSchemaBuilder<>::CalculateRequiredListSize(m1, m2);
  ASSERT_EQ(expectedSizeTwoMems, 7 + 2 + 1 + 10 + 2);
  // (7)"testKey" + (2)quotes + (1)comma + (10)"anotherKey" + (2)quotes
}

TEST(ObjectSchemaBuilderTest, GetSchemaSize) {
  static constexpr auto schema = ObjectSchema()
                                     .Require("aTestKey", "string")
                                     .Require("aTestIntKey", "integer")
                                     .Build();

  // Create a string with whitespace to test
  const char *hardCodeSchema = R"({
  "type":"object",
  "required":["aTestKey","aTestIntKey"],
  "properties":{
    "aTestKey":{"type":"string"},
    "aTestIntKey":{"type":"integer"}
  }
  })";

  // Remove whitespace and compare - using a simple approach without std::remove_if for now
  std::string cleanSchema = hardCodeSchema;
  // Just remove spaces, tabs, and newlines manually to simplify test
  cleanSchema.erase(std::remove_if(cleanSchema.begin(), cleanSchema.end(), ::isspace), cleanSchema.end());

  ASSERT_STREQ(cleanSchema.c_str(), schema.data());
}