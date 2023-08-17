#pragma once
#include <stdint.h>
void __disable_irq();
void __enable_irq();
typedef void *osSemaphoreId;
typedef void *osMessageQid;
typedef void *osThreadId;
typedef void *osMessageQId;
typedef void *osMutexId;
typedef void *SemaphoreHandle_t;
#define osMutexDef(name)
void *osThreadGetId();
uint32_t osDelay(uint32_t);
typedef int USBH_StatusTypeDef;
