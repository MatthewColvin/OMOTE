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
  // Compatibility struct for direct unit testing of size helpers
  struct Member {
    std::string_view key = "";
    std::string_view type = "";
    bool required = false;
    std::string_view nestedSchemaString = "";
    bool hasNestedSchema = false;
  };

  static constexpr std::string_view BeginningString = R"({"type":"object","required":[)";
  static constexpr std::string_view PostRequiredMembersArray = R"(],"properties":{)";
  static constexpr std::string_view MemberTypeString = R"(":{"type":")";
  static constexpr std::string_view NestedMemberOpenString = R"(":)";
  static constexpr std::string_view EndMemberTypeString = R"("})";
  static constexpr std::string_view EndString = R"(}})";
  static constexpr std::string_view QuotationMark = "\"";
  static constexpr std::string_view CommaQuotationMark = ",\"";

  template <typename... Ms>
  static constexpr size_t CalculateRequiredListSize(const Ms &...ms) {
    size_t size = 0;
    bool firstRequired = true;
    ([&] {
      if (!ms.required)
        return;
      size += (firstRequired ? QuotationMark.size() : CommaQuotationMark.size()) + ms.key.size() + QuotationMark.size();
      firstRequired = false;
    }(),
     ...);
    return size;
  }

  template <typename... Ms>
  static constexpr size_t CalculateTypeObjectSize(const Ms &...ms) {
    size_t size = 0;
    bool first = true;
    ([&] {
      size += (first ? QuotationMark.size() : CommaQuotationMark.size()) + ms.key.size() + MemberTypeString.size() + ms.type.size() + EndMemberTypeString.size();
      first = false;
    }(),
     ...);
    return size;
  }
};

// ---------------------------------------------------------------------------
// Member descriptors
// ---------------------------------------------------------------------------

template <size_t KeyLen, size_t TypeLen>
struct ScalarMember {
  std::array<char, KeyLen> key;
  std::array<char, TypeLen> type;
  bool required;

  constexpr std::string_view keyView() const { return {key.data(), KeyLen}; }
  constexpr std::string_view typeView() const { return {type.data(), TypeLen}; }
};

template <size_t KeyLen, size_t SchemaLen>
struct NestedMember {
  std::array<char, KeyLen> key;
  std::array<char, SchemaLen> schema;
  bool required;

  constexpr std::string_view keyView() const { return {key.data(), KeyLen}; }
  constexpr std::string_view schemaView() const { return {schema.data(), SchemaLen}; }
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

template <size_t N>
constexpr std::array<char, N> toArrayFromView(std::string_view sv) {
  std::array<char, N> a{};
  for (size_t i = 0; i < N; ++i)
    a[i] = sv[i];
  return a;
}

// ---------------------------------------------------------------------------
// The builder
// ---------------------------------------------------------------------------

template <size_t N, size_t RequiredCount, typename... Members>
struct ObjectSchemaBuilder : ObjectSchemaBuilderBase {

  std::tuple<Members...> members;

  constexpr explicit ObjectSchemaBuilder(Members... ms) : members(ms...) {}

  // Returns the total array size Build() will produce (content + null terminator)
  static constexpr size_t CalculateSize() { return N + 1; }

  // ------------------------------------------------------------------
  // Require / Optional — scalar type
  // ------------------------------------------------------------------
  template <size_t KeyLen, size_t TypeLen>
  constexpr auto Require(const char (&key)[KeyLen], const char (&type)[TypeLen]) const {
    return addScalar<KeyLen - 1, TypeLen - 1, true>(key, type);
  }

  template <size_t KeyLen, size_t TypeLen>
  constexpr auto Optional(const char (&key)[KeyLen], const char (&type)[TypeLen]) const {
    return addScalar<KeyLen - 1, TypeLen - 1, false>(key, type);
  }

  // ------------------------------------------------------------------
  // Require / Optional — nested ObjectSchemaBuilder
  // NestN is the content size; Build() returns array<char, NestN+1>,
  // so we pass NestN+1 as SchemaLen to addNested.
  // ------------------------------------------------------------------
  template <size_t KeyLen, size_t NestN, size_t NestReq, typename... NestMs>
  constexpr auto Require(const char (&key)[KeyLen],
                         ObjectSchemaBuilder<NestN, NestReq, NestMs...> nested) const {
    return addNested<KeyLen - 1, NestN + 1, true>(key, nested.Build());
  }

  template <size_t KeyLen, size_t NestN, size_t NestReq, typename... NestMs>
  constexpr auto Optional(const char (&key)[KeyLen],
                          ObjectSchemaBuilder<NestN, NestReq, NestMs...> nested) const {
    return addNested<KeyLen - 1, NestN + 1, false>(key, nested.Build());
  }

  // ------------------------------------------------------------------
  // Build — returns std::array<char, N+1>
  // ------------------------------------------------------------------
  constexpr auto Build() const {
    std::array<char, N + 1> result{};
    size_t pos = 0;

    auto append = [&](std::string_view sv) {
      for (char c : sv)
        result[pos++] = c;
    };

    append(BeginningString);

    {
      bool firstReq = true;
      std::apply([&](auto &&...ms) {
        auto emitReq = [&](auto &m) {
          if (!m.required)
            return;
          append(firstReq ? QuotationMark : CommaQuotationMark);
          append(m.keyView());
          append(QuotationMark);
          firstReq = false;
        };
        (emitReq(ms), ...);
      },
                 members);
    }

    append(PostRequiredMembersArray);

    {
      bool firstProp = true;
      std::apply([&](auto &&...ms) {
        auto emitScalar = [&]<size_t KL, size_t TL>(const ScalarMember<KL, TL> &m) {
          append(firstProp ? QuotationMark : CommaQuotationMark);
          append(m.keyView());
          append(MemberTypeString);
          append(m.typeView());
          append(EndMemberTypeString);
          firstProp = false;
        };

        auto emitNested = [&]<size_t KL, size_t SL>(const NestedMember<KL, SL> &m) {
          append(firstProp ? QuotationMark : CommaQuotationMark);
          append(m.keyView());
          append(NestedMemberOpenString);
          append(m.schemaView());
          firstProp = false;
        };

        ([&] {
          if constexpr (requires { ms.schemaView(); })
            emitNested(ms);
          else
            emitScalar(ms);
        }(),
         ...);
      },
                 members);
    }

    append(EndString);
    result[pos] = '\0';
    return result;
  }

private:
  template <size_t KL, size_t TL, bool Req>
  constexpr auto addScalar(std::string_view key, std::string_view type) const {
    ScalarMember<KL, TL> m{toArrayFromView<KL>(key), toArrayFromView<TL>(type), Req};

    constexpr bool isFirstProp = sizeof...(Members) == 0;
    constexpr bool isFirstReq = RequiredCount == 0;

    constexpr size_t reqDelta = Req
                                    ? (isFirstReq ? QuotationMark.size() : CommaQuotationMark.size()) + KL + QuotationMark.size()
                                    : 0;

    constexpr size_t propDelta = (isFirstProp ? QuotationMark.size() : CommaQuotationMark.size()) + KL + MemberTypeString.size() + TL + EndMemberTypeString.size();

    constexpr size_t newN = N + reqDelta + propDelta;
    constexpr size_t newReq = RequiredCount + (Req ? 1 : 0);

    auto newMembers = std::tuple_cat(members, std::make_tuple(m));
    return std::apply([](auto &&...ms) {
      return ObjectSchemaBuilder<newN, newReq, Members..., ScalarMember<KL, TL>>(ms...);
    },
                      newMembers);
  }

  // SchemaLen = NestN+1 (includes null terminator); we store SchemaLen-1 content bytes.
  template <size_t KL, size_t SchemaLen, bool Req>
  constexpr auto addNested(std::string_view key,
                           std::array<char, SchemaLen> schemaWithNull) const {
    constexpr size_t SL = SchemaLen - 1;

    NestedMember<KL, SL> m;
    m.required = Req;
    for (size_t i = 0; i < KL; ++i)
      m.key[i] = key[i];
    for (size_t i = 0; i < SL; ++i)
      m.schema[i] = schemaWithNull[i];

    constexpr bool isFirstProp = sizeof...(Members) == 0;
    constexpr bool isFirstReq = RequiredCount == 0;

    constexpr size_t reqDelta = Req
                                    ? (isFirstReq ? QuotationMark.size() : CommaQuotationMark.size()) + KL + QuotationMark.size()
                                    : 0;

    constexpr size_t propDelta = (isFirstProp ? QuotationMark.size() : CommaQuotationMark.size()) + KL + NestedMemberOpenString.size() + SL;

    constexpr size_t newN = N + reqDelta + propDelta;
    constexpr size_t newReq = RequiredCount + (Req ? 1 : 0);

    auto newMembers = std::tuple_cat(members, std::make_tuple(m));
    return std::apply([](auto &&...ms) {
      return ObjectSchemaBuilder<newN, newReq, Members..., NestedMember<KL, SL>>(ms...);
    },
                      newMembers);
  }
};

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------
static constexpr size_t BaseSize =
    ObjectSchemaBuilderBase::BeginningString.size() +
    ObjectSchemaBuilderBase::PostRequiredMembersArray.size() +
    ObjectSchemaBuilderBase::EndString.size();

constexpr auto ObjectSchema() {
  return ObjectSchemaBuilder<BaseSize, 0>{};
}

} // namespace OMOTE::JSON