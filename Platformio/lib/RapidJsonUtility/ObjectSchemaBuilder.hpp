#pragma once

#include <array>
#include <string_view>
#include <tuple>

/**
 * A compile-time builder for JSON schemas that generates a JSON schema string representing an object with specified required and optional members.
 *
 * The builder uses template metaprogramming to calculate the size of the resulting schema string at compile time,
 * allowing it to return a std::array<char, N> containing the schema string without any dynamic memory allocation.
 *
 * Usage:
 * constexpr auto mySchema = OMOTE::JSON::ObjectSchema()
 *                                 .Require("requiredField", "string")
 *                                 .Optional("optionalField", "number")
 *                                 .Build();
 *
 * @note: You can delay call to Build() until runtime but this will result in the schema string being generated at runtime instead of compile time.
 */

namespace OMOTE::JSON {

struct ObjectSchemaBuilderBase {
  struct Member {
    std::string_view key = "";
    std::string_view type = "";
    bool required = false;
  };

  static constexpr std::string_view BeginningString = R"({"type":"object","required":[)";
  static constexpr std::string_view PostRequiredMembersArrayString = R"(],"properties":{)";
  static constexpr std::string_view MemberTypeString = R"(":{"type":")";
  static constexpr std::string_view EndMemberTypeString = R"("})";
  static constexpr std::string_view EndString = R"(}})";
  static constexpr std::string_view QuotationMark = "\"";
  static constexpr std::string_view CommaQuotationMark = ",\"";

  template <typename... Ms>
  static constexpr size_t CalculateRequiredListSize(const Ms &...ms) {
    size_t size = 0;
    bool firstRequired = true;
    (void)std::initializer_list<int>{(ms.required ? (firstRequired ? (size += QuotationMark.size() + ms.key.size() + QuotationMark.size(), firstRequired = false)
                                                                   : (size += CommaQuotationMark.size() + ms.key.size() + QuotationMark.size(), 0))
                                                  : 0)...};
    return size;
  }

  template <typename... Ms>
  static constexpr size_t CalculateTypeObjectSize(const Ms &...ms) {
    size_t size = 0;
    bool first = true;
    [[maybe_unused]] auto dummy = {(first ? (size += QuotationMark.size() + ms.key.size() + MemberTypeString.size() + ms.type.size() + EndMemberTypeString.size(), first = false)
                                          : (size += CommaQuotationMark.size() + ms.key.size() + MemberTypeString.size() + ms.type.size() + EndMemberTypeString.size(), 0))...};
    return size;
  }
};

template <size_t N, size_t RequiredMembersCount, typename... Members>
struct ObjectSchemaBuilder : public ObjectSchemaBuilderBase {
public:
  constexpr ObjectSchemaBuilder(Members... ms) : members(ms...) {}

  template <size_t KeyLen, size_t TypeLen>
  constexpr auto Require(const char (&key)[KeyLen], const char (&type)[TypeLen]) const {
    return AddMember<KeyLen - 1, TypeLen - 1, true>(key, type);
  }

  template <size_t KeyLen, size_t TypeLen>
  constexpr auto Optional(const char (&key)[KeyLen], const char (&type)[TypeLen]) const {
    return AddMember<KeyLen - 1, TypeLen - 1, false>(key, type);
  }

  constexpr auto Build() const {
    return BuildImpl<N>();
  }

  constexpr size_t CalculateSize() const {
    return N;
  }

  template <size_t KeyLen, size_t TypeLen, bool isMemberRequired>
  constexpr auto AddMember(std::string_view key, std::string_view type) const {
    Member m{key, type, isMemberRequired};
    auto newMembers = std::tuple_cat(members, std::make_tuple(m));

    // Comma separated array of required members in the "required" field of the schema, with quotes around each key. Only contributes if the member being added is required.
    constexpr size_t arrayStringLength = RequiredMembersCount == 0
                                             ? QuotationMark.size() + KeyLen + QuotationMark.size()       // First one just has quotes no need for commas
                                             : CommaQuotationMark.size() + KeyLen + QuotationMark.size(); // Subsequent required members need a comma and quotes around the key
    constexpr size_t reqArrayKeyStringLength = isMemberRequired ? arrayStringLength : 0;

    constexpr size_t existingTotal = sizeof...(Members);

    // Object schema properties contribution for this member, which will be in the format "key":{"type":"type"}, with a comma if it's not the first property.
    // Always contributes regardless of whether the member is required or optional, since all members go in the properties object.
    constexpr size_t propertyObjectStringLength = existingTotal == 0
                                                      ? QuotationMark.size() + KeyLen + MemberTypeString.size() + TypeLen + EndMemberTypeString.size()
                                                      : CommaQuotationMark.size() + KeyLen + MemberTypeString.size() + TypeLen + EndMemberTypeString.size();

    constexpr size_t newMemberAdditionSize = reqArrayKeyStringLength + propertyObjectStringLength;
    constexpr size_t newSchemaSize = N + newMemberAdditionSize;
    constexpr size_t newRequiredMembersCount = RequiredMembersCount + (isMemberRequired ? 1 : 0);

    return std::apply(
        [](auto &&...ms) {
          return ObjectSchemaBuilder<newSchemaSize, newRequiredMembersCount, Members..., Member>(ms...);
        },
        newMembers);
  }

  constexpr void AppendRequiredMembers(const auto &aAppendFunc) const {
    bool firstRequired = true;
    std::apply([&](auto &&...ms) {
      ((ms.required ? (firstRequired ? (aAppendFunc(QuotationMark), aAppendFunc(ms.key), aAppendFunc(QuotationMark), firstRequired = false, 0)
                                     : (aAppendFunc(CommaQuotationMark), aAppendFunc(ms.key), aAppendFunc(QuotationMark), 0))
                    : 0),
       ...);
    },
               members);
  }

  constexpr void AppendTypeSchema(const auto &aAppendFunc) const {
    bool first = true;
    std::apply([&](auto &&...ms) {
      ((first ? (aAppendFunc(QuotationMark), aAppendFunc(ms.key), aAppendFunc(MemberTypeString), aAppendFunc(ms.type), aAppendFunc(EndMemberTypeString), first = false, 0)
              : (aAppendFunc(CommaQuotationMark), aAppendFunc(ms.key), aAppendFunc(MemberTypeString), aAppendFunc(ms.type), aAppendFunc(EndMemberTypeString), 0)),
       ...);
    },
               members);
  }

  template <size_t Sz>
  constexpr auto BuildImpl() const {
    std::array<char, Sz> result = {};
    size_t pos = 0;

    auto append = [&](std::string_view str) {
      for (char c : str) {
        result[pos++] = c;
      }
    };

    append(BeginningString);
    AppendRequiredMembers(append);
    append(PostRequiredMembersArrayString);
    AppendTypeSchema(append);
    append(EndString);

    if (pos == Sz - 1) {
      result[pos] = '\0'; // Null terminator for safety and cstring compatibility
    }
    return result;
  }

  std::tuple<Members...> members;
};

static constexpr size_t BaseSize =
    OMOTE::JSON::ObjectSchemaBuilderBase::BeginningString.size() +
    OMOTE::JSON::ObjectSchemaBuilderBase::PostRequiredMembersArrayString.size() +
    OMOTE::JSON::ObjectSchemaBuilderBase::EndString.size() +
    1; // +1 for null terminator

static constexpr size_t initialRequiredMembersCount = 0;

constexpr auto ObjectSchema() {
  return OMOTE::JSON::ObjectSchemaBuilder<BaseSize, initialRequiredMembersCount>{};
}
} // namespace OMOTE::JSON