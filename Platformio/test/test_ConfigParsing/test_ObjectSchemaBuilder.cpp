#include "ObjectSchemaBuilder.hpp"
#include <algorithm>
#include <gtest/gtest.h>

//////////////// START FUNCTIONAL TEST DATA ///////////////

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

constexpr auto innerSchema = OMOTE::JSON::ObjectSchema()
                                 .Require("innerField", "string")
                                 .Optional("innerNumber", "number");

// static constexpr gives the array static storage duration so .data()
// is a valid constant expression for the string_view constructor
static constexpr auto outerSchema = OMOTE::JSON::ObjectSchema()
                                        .Require("outerField", "string")
                                        .Require("nestedObject", innerSchema);

static constexpr const char *nestedSchemaTruth = R"({
    "type": "object",
    "required": ["outerField", "nestedObject"],
    "properties": {
      "outerField": { "type": "string" },
      "nestedObject": { 
        "type": "object",
        "required": ["innerField"],
        "properties": {
          "innerField": { "type": "string" },
          "innerNumber": { "type": "number" }
        }   
      }
    }
  })";

//////////////// END FUNCTIONAL TEST DATA  ///////////////

TEST(ObjectSchemaBuilderTest, BasicFunctionality) {
  auto schema = OMOTE::JSON::ObjectSchema()
                    .Require("name", "string")
                    .Optional("age", "integer")
                    .Build();
  ASSERT_NE(schema.data(), nullptr);
}

TEST(ObjectSchemaBuilderTest, SizeCalculation) {
  auto basicSchema = OMOTE::JSON::ObjectSchema()
                         .Require("id", "string")
                         .Build();
  ASSERT_GT(basicSchema.size(), 0u);
}

TEST(ObjectSchemaBuilderTest, CalculateRequiredListSizeTest) {
  constexpr OMOTE::JSON::ObjectSchemaBuilderBase::Member m1{"testKey", "string", true};
  constexpr auto expectedSize = OMOTE::JSON::ObjectSchemaBuilderBase::CalculateRequiredListSize(m1);
  ASSERT_EQ(expectedSize, 7 + 2); // "testKey"(7) + 2 quotes

  constexpr OMOTE::JSON::ObjectSchemaBuilderBase::Member m2{"anotherKey", "integer", true};
  constexpr auto expectedSizeTwoMems = OMOTE::JSON::ObjectSchemaBuilderBase::CalculateRequiredListSize(m1, m2);
  ASSERT_EQ(expectedSizeTwoMems, 7 + 2 + 1 + 10 + 2); // key1+quotes + comma + key2+quotes
}

TEST(ObjectSchemaBuilderTest, CalculateTypeObjectSizeTest) {
  constexpr OMOTE::JSON::ObjectSchemaBuilderBase::Member m1{"testKey", "string", true};
  constexpr OMOTE::JSON::ObjectSchemaBuilderBase::Member m2{"anotherKey", "integer", true};
  constexpr auto expectedSize = OMOTE::JSON::ObjectSchemaBuilderBase::CalculateTypeObjectSize(m1, m2);

  auto firstObject = R"("testKey":{"type":"string"})";
  auto secondObject = R"("anotherKey":{"type":"integer"})";
  ASSERT_EQ(expectedSize, strlen(firstObject) + 1 + strlen(secondObject));
}

void validateSizeCalculations(auto builder, const char *hardcoded) {
  std::string cleanSchema = hardcoded;
  cleanSchema.erase(std::remove_if(cleanSchema.begin(), cleanSchema.end(), ::isspace),
                    cleanSchema.end());
  auto expectedSize = cleanSchema.length() + 1; // +1 for null terminator

  // CalculateSize() is static so we can call it on the type without a constexpr instance
  auto calculatedSize = std::decay_t<decltype(builder)>::CalculateSize();
  ASSERT_EQ(calculatedSize, expectedSize);

  auto builtSchema = builder.Build();
  ASSERT_EQ(builtSchema.size(), expectedSize);
}

void validateSchemaFormat(auto builder, const char *hardcoded) {
  std::string cleanTruthSchema = hardcoded;
  cleanTruthSchema.erase(std::remove_if(cleanTruthSchema.begin(), cleanTruthSchema.end(), ::isspace),
                         cleanTruthSchema.end());
  std::string builtSchemaStr(builder.Build().data(), builder.Build().size());
  ASSERT_STREQ(cleanTruthSchema.c_str(), builtSchemaStr.c_str());
}

/////////////// FUNCTIONAL TEST CASES ///////////////

TEST(ObjectSchemaBuilderTest, SchemaOneTest) {
  validateSizeCalculations(schemaBuilderOne, hardCodeSchemaMatchingOne);
  validateSchemaFormat(schemaBuilderOne, hardCodeSchemaMatchingOne);
}

TEST(ObjectSchemaBuilderTest, SchemaTwoTest) {
  validateSizeCalculations(schemaBuilderTwo, hardCodeSchemaMatchingTwo);
  validateSchemaFormat(schemaBuilderTwo, hardCodeSchemaMatchingTwo);
}

TEST(ObjectSchemaBuilderTest, NestedSchemaTest) {
  validateSizeCalculations(outerSchema, nestedSchemaTruth);
  validateSchemaFormat(outerSchema, nestedSchemaTruth);
}
