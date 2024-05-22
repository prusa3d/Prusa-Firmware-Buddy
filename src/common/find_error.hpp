#pragma once

#include "error_codes.hpp"
#include "error_list.hpp"

constexpr const ErrDesc &find_error(const ErrCode error_code) {
    // Iterating through error_list to find the error
    const auto error = std::ranges::find_if(error_list, [error_code](const auto &elem) { return (elem.err_code) == error_code; });
    if (error == std::end(error_list)) {
        bsod("Unknown error");
    }
    return *error;
}
