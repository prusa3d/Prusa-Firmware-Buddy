#ifndef CTHASH_INTERNAL_DEDUCE_HPP
#define CTHASH_INTERNAL_DEDUCE_HPP

#include <concepts>
#include <cstdint>

namespace cthash::internal {

// support

template <typename T>
concept digest_length_provided = requires() //
{
    { static_cast<size_t>(T::digest_length) }
    ->std::same_as<size_t>;
};

template <typename T>
concept digest_length_bit_provided = requires() //
{
    { static_cast<size_t>(T::digest_length_bit) }
    ->std::same_as<size_t>;
};

template <typename T>
concept initial_values_provided = requires() //
{
    { static_cast<size_t>(T::initial_values.size() * sizeof(typename decltype(T::initial_values)::value_type)) }
    ->std::same_as<size_t>;
};

template <typename>
static constexpr bool dependent_false = false;

template <typename Config>
constexpr size_t digest_bytes_length_of = [] {
    if constexpr (digest_length_provided<Config>) {
        return static_cast<size_t>(Config::digest_length);
    } else if constexpr (digest_length_bit_provided<Config>) {
        return static_cast<size_t>(Config::digest_length_bit) / 8u;
    } else if constexpr (initial_values_provided<Config>) {
        return static_cast<size_t>(Config::initial_values.size() * sizeof(typename decltype(Config::initial_values)::value_type));
    } else {
        static_assert(dependent_false<Config>);
    }
}();

} // namespace cthash::internal

#endif
