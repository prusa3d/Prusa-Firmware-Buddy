/**
 * @file utility_extensions.hpp
 * @brief This file contains various extensions to the std <utility> header
 */

#pragma once

#include <utility>

namespace ftrstd { // future std - simple to refactor out once our c++ standard increases

// (since C++23)
template <class Enum>
constexpr std::underlying_type_t<Enum> to_underlying(Enum e) noexcept {
    return static_cast<std::underlying_type_t<Enum>>(e);
}

};
