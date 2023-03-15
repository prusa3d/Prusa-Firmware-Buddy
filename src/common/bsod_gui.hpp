#pragma once

#include "error_codes.hpp"

[[noreturn]] void raise_redscreen(ErrCode error_code, const char *error, const char *module);

template <typename... Args>
[[noreturn]] void fatal_error(const ErrCode error_code, Args &&... args) {

    // Iterating through error_list to find the error
    const auto corresponding_error = std::ranges::find_if(error_list, [error_code](const auto &elem) { return (elem.err_code) == error_code; });
    char err_msg[100];
    snprintf(err_msg, sizeof(err_msg), corresponding_error->err_text, std::forward<decltype(args)>(args)...);
    raise_redscreen(error_code, err_msg, corresponding_error->err_title);
}
