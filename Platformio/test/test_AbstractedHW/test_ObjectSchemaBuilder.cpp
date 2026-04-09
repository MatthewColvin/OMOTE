#include <ObjectSchemaBuilder.hpp>
#include <gtest/gtest.h>

// Simple test for ObjectSchemaBuilder functionality
TEST(ObjectSchemaBuilderTest, BasicFunctionality) {
  // Test that we can create a schema with required and optional fields
  auto schema = ObjectSchema()
                    .Require("name", "string")
                    .Optional("age", "integer")
                    .Build();

  // Test the build worked
  ASSERT_NE(schema.data(), nullptr);
}

// Test the size calculation works correctly
TEST(ObjectSchemaBuilderTest, SizeCalculation) {
  auto schema = ObjectSchema()
                    .Require("id", "string")
                    .Build();

  // This should work without compile errors
  size_t size = schema.size();
  ASSERT_GT(size, 0);
}

// Test the GetSchemaSize method (newly added)
TEST(ObjectSchemaBuilderTest, GetSchemaSize) {
  auto schema = ObjectSchema()
                    .Require("test", "string")
                    .Build();

  // This should work without runtime errors
  size_t size_from_method = schema.size();
  ASSERT_GT(size_from_method, 0);
}