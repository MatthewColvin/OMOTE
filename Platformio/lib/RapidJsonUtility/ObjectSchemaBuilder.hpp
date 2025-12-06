#pragma once

#include <array>
#include <string_view>

// Helper to count elements
template <typename... Args>
struct Counter {
  static constexpr size_t value = sizeof...(Args);
};

// Immutable builder using variadic templates
template <typename... Members>
struct ObjectSchemaBuilder {

  struct Member {
    std::string_view key = "";
    std::string_view type = "";
    bool required = false;
  };

  std::tuple<Members...> members;

  constexpr ObjectSchemaBuilder(Members... ms) : members(ms...) {}

  constexpr auto Require(std::string_view key, std::string_view type) {
    return AddMember(key, type, true);
  }

  constexpr auto Optional(std::string_view key, std::string_view type) {
    return AddMember(key, type, false);
  }

  // Add a member - returns NEW builder with added member
  constexpr auto AddMember(std::string_view key, std::string_view type, bool required = false) const {
    Member m{key, type, required};
    auto newMembers = std::tuple_cat(members, std::make_tuple(m));
    return std::apply([](auto &&...ms) { return ObjectSchemaBuilder<Members..., Member>(ms...); }, newMembers);
  }

  // Calculate size at compile time - helper that works with unpacked members
  template <typename... Ms>
  static constexpr size_t CalculateSizeHelper(const Ms &...ms) {
    size_t size = std::string_view{R"({"type":"object","required":[)"}.size();

    // Required array
    bool firstRequired = true;
    ((ms.required ? (firstRequired ? (size += 2 + ms.key.size(), firstRequired = false)
                                   : (size += 3 + ms.key.size(), 0))
                  : 0),
     ...);

    size += std::string_view{R"(],"properties":{)"}.size();

    // Properties
    bool first = true;
    ((first ? (size += 1 + ms.key.size() + 12 + ms.type.size() + 2, first = false)
            : (size += 2 + ms.key.size() + 12 + ms.type.size() + 2, 0)),
     ...);

    size += 2; // }}

    constexpr auto THE_MAGIC_NUMBER_BECAUSE_CALCUATION_ABOVE_IS_WRONG = 44;
    return size + THE_MAGIC_NUMBER_BECAUSE_CALCUATION_ABOVE_IS_WRONG;
  }

  // Calculate size at compile time by unpacking the tuple
  constexpr size_t CalculateSize() const {
    return std::apply([](const auto &...ms) { return CalculateSizeHelper(ms...); }, members);
  }

  // Calculate size from member types directly (compile-time)
  static constexpr size_t SchemaSize = std::apply(
      [](auto &&...ms) { return CalculateSizeHelper(ms...); },
      std::tuple<Members...>{});

  // Build the schema string with exact size from template parameter
  template <size_t N = SchemaSize>
  constexpr auto BuildImpl() const {
    std::array<char, N> result = {}; // Exact size at compile time
    size_t pos = 0;

    auto append = [&](std::string_view str) {
      for (char c : str) {
        result[pos] = c;
        pos++;
      }
    };

    append(R"({"type":"object","required":[)");

    bool firstRequired = true;
    std::apply([&](auto &&...ms) {
      ((ms.required ? (firstRequired ? (append("\""), append(ms.key), append("\""), firstRequired = false, 0)
                                     : (append(",\""), append(ms.key), append("\""), 0))
                    : 0),
       ...);
    },
               members);

    append(R"(],"properties":{)");

    bool first = true;
    std::apply([&](auto &&...ms) {
      ((first ? (append("\""), append(ms.key), append(R"(":{"type":")"),
                 append(ms.type), append(R"("})"), first = false, 0)
              : (append(",\""), append(ms.key), append(R"(":{"type":")"),
                 append(ms.type), append(R"("})"), 0)),
       ...);
    },
               members);

    append("}}");
    result[pos] = '\0';

    return result;
  }

  // Public Build() delegates to BuildImpl with deduced size
  constexpr auto Build() const {
    return BuildImpl();
  }
};

// Helper to start building
constexpr auto ObjectSchema() {
  return ObjectSchemaBuilder<>{};
}
