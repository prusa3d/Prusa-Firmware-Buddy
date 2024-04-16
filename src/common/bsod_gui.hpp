#pragma once

#include "error_codes.hpp"

#include <cstdio>
#include <utility>

[[noreturn]] void raise_redscreen(ErrCode error_code, const char *error, const char *module);

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

/**
 * @brief Mark when BSOD would be shown and allow dumping a new BSOD.
 */
void bsod_mark_shown();
