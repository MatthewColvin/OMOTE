#pragma once

#include <array>
#include <string_view>
#include <tuple>

// Base struct holding Member type and static size calculation helpers,
// independent of N so tests can access them as ObjectSchemaBuilderBase::Member etc.
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

template <size_t N, typename... Members>
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

    constexpr size_t existingRequired = CountRequired<Members...>();
    constexpr size_t keyLengthInRequiredArray = existingRequired == 0
                                                    ? QuotationMark.size() + KeyLen + QuotationMark.size()       // First one just has quotes no need for commas
                                                    : CommaQuotationMark.size() + KeyLen + QuotationMark.size(); // Subsequent required members need a comma and quotes around the key
    constexpr size_t reqContrib = isMemberRequired
                                      ? keyLengthInRequiredArray
                                      : 0; // Not required then does not go in the required array

    constexpr size_t existingTotal = sizeof...(Members);
    constexpr size_t propContrib = existingTotal == 0
                                       ? QuotationMark.size() + KeyLen + MemberTypeString.size() + TypeLen + EndMemberTypeString.size()
                                       : CommaQuotationMark.size() + KeyLen + MemberTypeString.size() + TypeLen + EndMemberTypeString.size();

    constexpr size_t newN = N + reqContrib + propContrib;

    return std::apply(
        [](auto &&...ms) { 
          // Build a new object builder to add the new member and update the total size we need for the schema. 
          return ObjectSchemaBuilder<newN, Members..., Member>(ms...); },
        newMembers);
  }

  template <typename... Ms>
  static constexpr size_t CountRequired() {
    if constexpr (sizeof...(Ms) == 0) {
      return 0;
    } else {
      return (size_t{0} + ... + (Ms{}.required ? 1 : 0));
    }
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

    return result;
  }

  std::tuple<Members...> members;
};

// Start with the base size as an empty object schema, then as each member
// is added we construct a new builder with the updated size based on the passed member.
static constexpr size_t BaseSize =
    ObjectSchemaBuilderBase::BeginningString.size() +
    ObjectSchemaBuilderBase::PostRequiredMembersArrayString.size() +
    ObjectSchemaBuilderBase::EndString.size() +
    1; // null terminator

constexpr auto ObjectSchema() {
  return ObjectSchemaBuilder<BaseSize>{};
}