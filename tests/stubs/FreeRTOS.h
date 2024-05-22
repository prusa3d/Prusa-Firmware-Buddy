#pragma once

// This is stub for unit tests
typedef void *SemaphoreHandle_t;

typedef uint32_t TickType_t;
#define portMAX_DELAY UINT32_MAX

#define pdTRUE  1
#define pdFALSE 0

inline bool xPortIsInsideInterrupt() { return false; }
