#pragma once

#include "error_codes.hpp"

#include <cstdio>
#include <utility>

[[noreturn]] void raise_redscreen(ErrCode error_code, const char *error, const char *module);

const ErrDesc &find_error(const ErrCode error_code);

template <typename... Args>
[[noreturn]] void fatal_error(const ErrCode error_code, Args &&...args) {
    const ErrDesc &corresponding_error = find_error(error_code);
    char err_msg[100];
    snprintf(err_msg, sizeof(err_msg), corresponding_error.err_text, std::forward<decltype(args)>(args)...);
    raise_redscreen(error_code, err_msg, corresponding_error.err_title);
}
