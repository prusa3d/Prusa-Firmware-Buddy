/**
 * @file display_hw_checks.hpp
 */

#pragma once

namespace lcd {
/**
 * @brief function to periodically check display HW
 * it also reinitializes display when reconnected - not guaranteed!!! not recommended to try!!!
 */
void communication_check();

/**
 * @brief minimal variant of communication_check() for gui_bare_loop()
 * only reinitializes a reconnected display; skips touchscreen recovery and the
 * display-problem warning, which need an initialized marlin_client
 */
void bare_communication_check();
}; // namespace lcd
