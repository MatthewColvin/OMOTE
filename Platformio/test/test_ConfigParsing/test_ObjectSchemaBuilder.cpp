#include "ObjectSchemaBuilder.hpp"
#include "RapidJsonUtilty.hpp"
#include <algorithm>
#include <gtest/gtest.h>

static constexpr auto schemaBuilderOne = OMOTE::JSON::ObjectSchema()
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

static constexpr auto schemaBuilderTwo = OMOTE::JSON::ObjectSchema()
                                             .Require("aTestKey", "string")
                                             .Require("aTestIntKey", "integer")
                                             .Optional("aTestOptKey", "boolean")
                                             .Optional("aTestOptIntKey", "integer")
                                             .Require("aTestKey2", "string");

static constexpr const char *hardCodeSchemaMatchingTwo = R"({
  "type":"object",
  "required":["aTestKey","aTestIntKey","aTestKey2"],
  "properties":{
    "aTestKey":{"type":"string"},
    "aTestIntKey":{"type":"integer"},
    "aTestOptKey":{"type":"boolean"},
    "aTestOptIntKey":{"type":"integer"},
    "aTestKey2":{"type":"string"}
  }
})";

// Simple test for ObjectSchemaBuilder functionality
TEST(ObjectSchemaBuilderTest, BasicFunctionality) {
  // Test that we can create a schema with required and optional fields
  auto schema = OMOTE::JSON::ObjectSchema()
                    .Require("name", "string")
                    .Optional("age", "integer")
                    .Build();

  // Test the build method returns a non-empty schema
  ASSERT_NE(schema.data(), nullptr);
}

// Test the size calculation works correctly
TEST(ObjectSchemaBuilderTest, SizeCalculation) {
  auto basicSchema = OMOTE::JSON::ObjectSchema()
                         .Require("id", "string")
                         .Build();

  size_t size = basicSchema.size();
  ASSERT_GT(size, 0);
}

TEST(ObjectSchemaBuilderTest, CalculateRequiredListSizeTest) {
  // Strings keys will be quoted and comma-separated.
  constexpr OMOTE::JSON::ObjectSchemaBuilderBase::Member m1{"testKey", "string", true};
  constexpr auto expectedSize = OMOTE::JSON::ObjectSchemaBuilderBase::CalculateRequiredListSize(m1);
  ASSERT_EQ(expectedSize, 7 + 2);
  // (7)"testKey" + (2)quotes

  constexpr OMOTE::JSON::ObjectSchemaBuilderBase::Member m2{"anotherKey", "integer", true};
  constexpr auto expectedSizeTwoMems = OMOTE::JSON::ObjectSchemaBuilderBase::CalculateRequiredListSize(m1, m2);
  ASSERT_EQ(expectedSizeTwoMems, 7 + 2 + 1 + 10 + 2);
  // (7)"testKey" + (2)quotes + (1)comma + (10)"anotherKey" + (2)quotes
}

TEST(ObjectSchemaBuilderTest, CalculateTypeObjectSizeTest) {
  // Expected format for properties members in test:
  //  "testKey":{"type":"string"},
  //  "anotherKey":{"type":"integer"}
  constexpr OMOTE::JSON::ObjectSchemaBuilderBase::Member m1{"testKey", "string", true};
  constexpr OMOTE::JSON::ObjectSchemaBuilderBase::Member m2{"anotherKey", "integer", true};
  constexpr auto expectedSize = OMOTE::JSON::ObjectSchemaBuilderBase::CalculateTypeObjectSize(m1, m2);

  auto firstObject = R"("testKey":{"type":"string"})";
  auto secondObject = R"("anotherKey":{"type":"integer"})";
  // Expected size is the sum of the two objects plus a comma between them
  ASSERT_EQ(expectedSize, strlen(firstObject) + 1 + strlen(secondObject));
}

void validateSizeCalculations(auto builder, const char *hardcoded) {
  std::string cleanSchema = hardcoded;
  cleanSchema.erase(std::remove_if(cleanSchema.begin(), cleanSchema.end(), ::isspace), cleanSchema.end());
  auto expectedSize = cleanSchema.length() + 1; // +1 for null terminator on the built schema

  // Check we can calculate the schema size at compile time.
  constexpr auto calculatedSize = builder.CalculateSize();
  ASSERT_EQ(calculatedSize, expectedSize);

  // Check that the built schema size matches the expected size.
  // Cant be constexpr?
  auto builtSchema = builder.Build();
  ASSERT_EQ(builtSchema.size(), expectedSize);
}

void validateSchemaFormat(auto builder, const char *hardcoded) {
  std::string cleanSchema = hardcoded;
  cleanSchema.erase(std::remove_if(cleanSchema.begin(), cleanSchema.end(), ::isspace), cleanSchema.end());
  std::string schemaStr(builder.Build().data(), builder.Build().size());
  ASSERT_STREQ(cleanSchema.c_str(), schemaStr.c_str());
}

TEST(ObjectSchemaBuilderTest, SchemaOneTest) {
  validateSizeCalculations(schemaBuilderOne, hardCodeSchemaMatchingOne);
  validateSchemaFormat(schemaBuilderOne, hardCodeSchemaMatchingOne);
}

TEST(ObjectSchemaBuilderTest, SchemaTwoTest) {
  validateSizeCalculations(schemaBuilderTwo, hardCodeSchemaMatchingTwo);
  validateSchemaFormat(schemaBuilderTwo, hardCodeSchemaMatchingTwo);
}