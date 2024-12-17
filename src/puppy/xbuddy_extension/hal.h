/**
 * @file
 * Like hal.hpp but can be included from C code.
 */
#pragma once

/**
 * Like hal::panic() but can be called from C code.
 */
[[noreturn]] void hal_panic();
