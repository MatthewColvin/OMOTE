#include "ObjectSchemaBuilder.hpp"
#include <algorithm>
#include <gtest/gtest.h>

static constexpr auto schemaBuilderOne = ObjectSchema()
                                             .Require("aTestKey", "string")
                                             .Require("aTestIntKey", "integer");
static constexpr const char *hardCodeSchemaMatchingOne = R"({
  "type":"object",
  "required":["aTestKey","aTestIntKey"],
  "properties":{
    "aTestKey":{"type":"string"},
    "aTestIntKey":{"type":"integer"}
  }
})";

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

TEST(ObjectSchemaBuilderTest, CalculateRequiredListSizeTest) {
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

TEST(ObjectSchemaBuilderTest, CalculateTypeObjectSizeTest) {
  // Expected format for properties members in test:
  //  "testKey":{"type":"string"},
  //  "anotherKey":{"type":"integer"}
  constexpr ObjectSchemaBuilder<>::Member m1{"testKey", "string", true};
  constexpr ObjectSchemaBuilder<>::Member m2{"anotherKey", "integer", true};
  constexpr auto expectedSize = ObjectSchemaBuilder<>::CalculateTypeObjectSize(m1, m2);

  auto firstObject = R"("testKey":{"type":"string"})";
  auto secondObject = R"("anotherKey":{"type":"integer"})";
  // Expected size is the sum of the two objects plus a comma between them
  ASSERT_EQ(expectedSize, strlen(firstObject) + 1 + strlen(secondObject));
}

TEST(ObjectSchemaBuilderTest, SchemaSizeTest) {
  std::string cleanSchema = hardCodeSchemaMatchingOne;
  cleanSchema.erase(std::remove_if(cleanSchema.begin(), cleanSchema.end(), ::isspace), cleanSchema.end());
  auto expectedSize = cleanSchema.length() + 1; // +1 for null terminator

  // Check we can calculate the schema size at compile time.
  constexpr auto calculatedSize = schemaBuilderOne.CalculateSize();
  ASSERT_EQ(calculatedSize, expectedSize);

  // Build the schema into a constexpr array and check its size matches the expected size.
  constexpr auto builtSchema = schemaBuilderOne.Build();
  ASSERT_EQ(builtSchema.size(), expectedSize);
}

TEST(ObjectSchemaBuilderTest, FullSchemaBuildTest) {
  constexpr auto schema = schemaBuilderOne.Build();
  std::string cleanSchema = hardCodeSchemaMatchingOne;
  cleanSchema.erase(std::remove_if(cleanSchema.begin(), cleanSchema.end(), ::isspace), cleanSchema.end());
  ASSERT_STREQ(cleanSchema.c_str(), schema.data());
}