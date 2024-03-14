/**
 * @file
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

int osGetCPUUsage();

/**
 * @brief Set a pointer to idle task watchdog reset function.
 * @note function MUST NOT, UNDER ANY CIRCUMSTANCES,
 * CALL A FUNCTION THAT MIGHT BLOCK.
 */
void osSetIdleTaskWatchdog(void (*function)());

#ifdef __cplusplus
}
#endif
