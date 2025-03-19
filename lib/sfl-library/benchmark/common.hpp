#ifndef SFL_BENCHMARK_COMMON_HPP
#define SFL_BENCHMARK_COMMON_HPP

#include <string_view>

///////////////////////////////////////////////////////////////////////////////

template <typename T>
constexpr std::string_view name_of_type_raw()
{
    #if defined(_MSC_VER)
    return __FUNCSIG__;
    #else
    return __PRETTY_FUNCTION__;
    #endif
}

template <typename T>
constexpr std::string_view name_of_type()
{
    using namespace std::literals;

    // idea from https://github.com/TheLartians/StaticTypeInfo/blob/master/include/static_type_info/type_name.h
    auto for_double = name_of_type_raw<double>();
    auto n_before = for_double.find("double"sv);
    auto n_after = for_double.size() - (n_before + "double"sv.size());

    auto str = name_of_type_raw<T>();
    str.remove_prefix(n_before);
    str.remove_suffix(n_after);
    return str;
}

///////////////////////////////////////////////////////////////////////////////

template <typename Container, typename = void>
struct has_emplace_front : std::false_type {};

template <typename Container>
struct has_emplace_front
<
    Container,
    std::void_t<decltype(std::declval<Container>().emplace_front())>
> : std::true_type {};

template <typename Container>
inline constexpr bool has_emplace_front_v = has_emplace_front<Container>::value;

///////////////////////////////////////////////////////////////////////////////

#endif // SFL_BENCHMARK_COMMON_HPP
