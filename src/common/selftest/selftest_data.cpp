#include "selftest_data.hpp"

#include <concepts>

namespace {

template <selftest::EnumUnderAndFourBytes Enum>
selftest::TestData from_int(uint32_t raw_data) {
    return static_cast<Enum>(raw_data);
}

template <typename T>
selftest::TestData from_int(uint32_t)
    requires std::same_as<T, std::monostate>
{
    return std::monostate {};
}

template <selftest::TypeWithStaticDeserialize T>
selftest::TestData from_int(uint32_t raw_data) {
    return T::deserialize(raw_data);
}

template <typename T>
uint32_t to_int(const T &) {
    static_assert(!std::same_as<T, T>, "Unimplemented specilaization for to_int in TestData variant serialization");
    return 0;
}

template <>
uint32_t to_int(const std::monostate &) {
    return 0;
}

template <selftest::EnumUnderAndFourBytes Enum>
uint32_t to_int(const Enum &e) {
    return static_cast<uint32_t>(e);
}

template <selftest::TypeWithMemberSerialize T>
uint32_t to_int(const T &t) {
    return t.serialize();
}

template <typename... Types>
void deserialize_helper(std::variant<Types...> &res, std::uint8_t type_index, std::uint32_t raw_data) {
    using des_fn = std::variant<Types...> (*)(std::uint32_t);
    static constexpr des_fn funcs[] = { &::from_int<Types>... };
    res = funcs[type_index](raw_data);
}

} // namespace

namespace selftest {

uint32_t serialize_test_data_to_int(const TestData &test_data) {
    return std::visit([](const auto &inner) {
        return ::to_int(inner);
    },
        test_data);
}

TestData deserialize_test_data_from_int(const uint8_t type_index, const uint32_t raw_data) {
    if (type_index >= std::variant_size_v<TestData>) {
        return std::monostate {};
    }

    TestData res {};
    deserialize_helper(res, type_index, raw_data);
    return res;
}

} // namespace selftest
