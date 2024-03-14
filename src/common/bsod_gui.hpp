#pragma once

#include "error_codes.hpp"

#include <cstdio>
#include <utility>

[[noreturn]] void raise_redscreen(ErrCode error_code, const char *error, const char *module);

const ErrDesc &find_error(const ErrCode error_code);

template <typename... Args>
[[noreturn]] void fatal_error(const ErrCode error_code, Args &&...args) {
    const ErrDesc &corresponding_error = find_error(error_code);
    char err_msg[140];
    snprintf(err_msg, sizeof(err_msg), corresponding_error.err_text, std::forward<decltype(args)>(args)...);
    raise_redscreen(error_code, err_msg, corresponding_error.err_title);
}

namespace bsod_details {
/**
 * @brief Get reason for hardfault.
 * @return pointer to string useful as bsod title
 */
const char *get_hardfault_reason();

/**
 * @brief Get last known task name.
 * @param buffer buffer to copy task name to
 * @param buffer_size size of buffer, will be modified with size left after task name
 * @return pointer to next free space in buffer
 */
size_t get_task_name(char *&buffer, size_t buffer_size);

/**
 * @brief Get core registers as a string.
 * Copy string with registers content to buffer.
 * @param buffer buffer to copy registers to
 * @param buffer_size size of buffer, will be modified with size left after registers
 * @return pointer to next free space in buffer
 */
size_t get_regs(char *&buffer, size_t buffer_size);

/**
 * @brief Get stack as a string.
 * Copy string with stack content to buffer.
 * @param buffer buffer to copy stack to
 * @param buffer_size size of buffer, will be modified with size left after stack
 * @return pointer to next free space in buffer
 */
size_t get_stack(char *&buffer, size_t buffer_size);
}; // namespace bsod_details
