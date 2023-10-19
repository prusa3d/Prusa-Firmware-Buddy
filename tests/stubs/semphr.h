#pragma once

#include "FreeRTOS.h"

// This is stub for tests
typedef int StaticSemaphore_t;

inline SemaphoreHandle_t xSemaphoreCreateBinary() { return NULL; }
#define vSemaphoreDelete(...)
#define xSemaphoreGive(...)
#define xSemaphoreTake(...)
