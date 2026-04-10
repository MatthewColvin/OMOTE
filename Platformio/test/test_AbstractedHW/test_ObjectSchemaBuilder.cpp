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

TEST(ObjectSchemaBuilderTest, GetSchemaSize) {
  static constexpr auto schema = ObjectSchema()
                                      .Require("aTestKey", "string")
                                      .Require("aTestIntKey", "integer")
                                      .Build();

  // Create a string with whitespace to test
  const char* hardCodeSchema = R"({"type":"object","required":["aTestKey","aTestIntKey"],"properties":{"aTestKey":{"type":"string"},"aTestIntKey":{"type":"integer"}}})";

  // Remove whitespace and compare - using a simple approach without std::remove_if for now
  std::string cleanSchema = hardCodeSchema;
  // Just remove spaces, tabs, and newlines manually to simplify test 
  cleanSchema.erase(std::remove_if(cleanSchema.begin(), cleanSchema.end(), ::isspace), cleanSchema.end());

  ASSERT_STREQ(cleanSchema.c_str(), schema.data());
}