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
}; // namespace lcd
