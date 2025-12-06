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
public:
  constexpr ObjectSchemaBuilder(Members... ms) : members(ms...) {}

  constexpr auto Require(std::string_view key, std::string_view type) {
    return AddMember(key, type, true);
  }

  constexpr auto Optional(std::string_view key, std::string_view type) {
    return AddMember(key, type, false);
  }

  constexpr auto Build() const {
    return BuildImpl();
  }

private:
  struct Member {
    std::string_view key = "";
    std::string_view type = "";
    bool required = false;
  };

  // Add a member - returns NEW builder with added member
  constexpr auto AddMember(std::string_view key, std::string_view type, bool required = false) const {
    Member m{key, type, required};
    auto newMembers = std::tuple_cat(members, std::make_tuple(m));
    return std::apply([](auto &&...ms) { return ObjectSchemaBuilder<Members..., Member>(ms...); }, newMembers);
  }

  template <typename... Ms>
  static constexpr size_t CalculateRequiredListSize(const Ms &...ms) {
    // Required array
    size_t size = 0;
    bool firstRequired = true;
    ((ms.required ? (firstRequired ? (size += 2 + ms.key.size(), firstRequired = false)
                                   : (size += 3 + ms.key.size(), 0))
                  : 0),
     ...);
    return size;
  }

  template <typename... Ms>
  static constexpr size_t CalculateRequiredObjectSize(const Ms &...ms) {
    // Properties
    size_t size = 0;
    bool first = true;
    ((first ? (size += 1 + ms.key.size() + 12 + ms.type.size() + 2, first = false)
            : (size += 2 + ms.key.size() + 12 + ms.type.size() + 2, 0)),
     ...);
    return size;
  }

  // Calculate size at compile time - helper that works with unpacked members
  template <typename... Ms>
  static constexpr size_t CalculateSizeHelper(const Ms &...ms) {
    size_t size = 0;
    size += BeginningString.size();
    size += CalculateRequiredListSize(ms...);
    size += PostRequiredMembersArrayString.size();
    size += CalculateRequiredObjectSize(ms...);
    size += EndString.size();

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
        result[pos++] = c;
      }
    };

    append(BeginningString);

    bool firstRequired = true;
    std::apply([&](auto &&...ms) {
      ((ms.required ? (firstRequired ? (append(QuotationMark), append(ms.key), append(QuotationMark), firstRequired = false, 0)
                                     : (append(CommaQuotationMark), append(ms.key), append(QuotationMark), 0))
                    : 0),
       ...);
    },
               members);

    append(PostRequiredMembersArrayString);

    bool first = true;
    std::apply([&](auto &&...ms) {
      ((first ? (append(QuotationMark), append(ms.key), append(MemberTypeString),
                 append(ms.type), append(EndMemberTypeString), first = false, 0)
              : (append(CommaQuotationMark), append(ms.key), append(MemberTypeString),
                 append(ms.type), append(EndMemberTypeString), 0)),
       ...);
    },
               members);

    append(EndString);
    result[pos] = '\0';

    return result;
  }

  std::tuple<Members...> members;

  static constexpr std::string_view BeginningString = R"({"type":"object","required":[)";
  static constexpr std::string_view PostRequiredMembersArrayString = R"(],"properties":{)";
  static constexpr std::string_view MemberTypeString = R"(":{"type":")";
  static constexpr std::string_view EndMemberTypeString = R"("})";
  static constexpr std::string_view EndString = R"(}})";
  static constexpr std::string_view Comma = ",";
  static constexpr std::string_view Colon = ":";
  static constexpr std::string_view QuotationMark = "\"";
  static constexpr std::string_view CommaQuotationMark = ",\"";
};

// Helper to start building
constexpr auto ObjectSchema() {
  return ObjectSchemaBuilder<>{};
}
