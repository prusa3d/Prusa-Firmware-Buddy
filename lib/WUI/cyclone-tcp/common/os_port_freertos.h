/**
 * @file os_port_freertos.h
 * @brief RTOS abstraction layer (FreeRTOS)
 *
 * @section License
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2010-2021 Oryx Embedded SARL. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

#ifndef _OS_PORT_FREERTOS_H
#define _OS_PORT_FREERTOS_H

//Dependencies
#ifdef IDF_VER
   #include "freertos/FreeRTOS.h"
   #include "freertos/task.h"
   #include "freertos/semphr.h"
#else
   #include "FreeRTOS.h"
   #include "task.h"
   #include "semphr.h"
#endif

//Task priority (normal)
#ifndef OS_TASK_PRIORITY_NORMAL
   #define OS_TASK_PRIORITY_NORMAL (tskIDLE_PRIORITY + 1)
#endif

//Task priority (high)
#ifndef OS_TASK_PRIORITY_HIGH
   #define OS_TASK_PRIORITY_HIGH (tskIDLE_PRIORITY + 2)
#endif

//Milliseconds to system ticks
#ifndef OS_MS_TO_SYSTICKS
   #define OS_MS_TO_SYSTICKS(n) (n)
#endif

//System ticks to milliseconds
#ifndef OS_SYSTICKS_TO_MS
   #define OS_SYSTICKS_TO_MS(n) (n)
#endif

//Task prologue
#ifndef osEnterTask
   #define osEnterTask()
#endif

//Interrupt service routine prologue
#ifndef osEnterIsr
   #if defined(portENTER_SWITCHING_ISR)
      #define osEnterIsr() portENTER_SWITCHING_ISR()
   #else
      #define osEnterIsr()
   #endif
#endif

//Interrupt service routine epilogue
#ifndef osExitIsr
   #if defined(__XTENSA__)
      #define osExitIsr(flag) if(flag) portYIELD_FROM_ISR()
   #elif defined(portEXIT_SWITCHING_ISR)
      #define osExitIsr(flag) portEXIT_SWITCHING_ISR()
   #elif defined(portEND_SWITCHING_ISR)
      #define osExitIsr(flag) portEND_SWITCHING_ISR(flag)
   #elif defined(portYIELD_FROM_ISR)
      #define osExitIsr(flag) portYIELD_FROM_ISR(flag)
   #else
      #define osExitIsr(flag)
   #endif
#endif

//Static object allocation
#ifndef configSUPPORT_STATIC_ALLOCATION
   #define configSUPPORT_STATIC_ALLOCATION 0
#endif

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Task object
 **/

//typedef void OsTask;
#define OsTask void


/**
 * @brief Event object
 **/

typedef struct
{
   SemaphoreHandle_t handle;
#if (configSUPPORT_STATIC_ALLOCATION == 1)
   StaticSemaphore_t buffer;
#endif
} OsEvent;


/**
 * @brief Semaphore object
 **/

typedef struct
{
   SemaphoreHandle_t handle;
#if (configSUPPORT_STATIC_ALLOCATION == 1)
   StaticSemaphore_t buffer;
#endif
} OsSemaphore;


/**
 * @brief Mutex object
 **/

typedef struct
{
   SemaphoreHandle_t handle;
#if (configSUPPORT_STATIC_ALLOCATION == 1)
   StaticSemaphore_t buffer;
#endif
} OsMutex;


/**
 * @brief Task routine
 **/

typedef void (*OsTaskCode)(void *param);


//Kernel management
void osInitKernel(void);
void osStartKernel(void);

//Task management
OsTask *osCreateTask(const char_t *name, OsTaskCode taskCode,
   void *param, size_t stackSize, int_t priority);

void osDeleteTask(OsTask *task);
void osDelayTask(systime_t delay);
void osSwitchTask(void);
void osSuspendAllTasks(void);
void osResumeAllTasks(void);

//Event management
bool_t osCreateEvent(OsEvent *event);
void osDeleteEvent(OsEvent *event);
void osSetEvent(OsEvent *event);
void osResetEvent(OsEvent *event);
bool_t osWaitForEvent(OsEvent *event, systime_t timeout);
bool_t osSetEventFromIsr(OsEvent *event);

//Semaphore management
bool_t osCreateSemaphore(OsSemaphore *semaphore, uint_t count);
void osDeleteSemaphore(OsSemaphore *semaphore);
bool_t osWaitForSemaphore(OsSemaphore *semaphore, systime_t timeout);
void osReleaseSemaphore(OsSemaphore *semaphore);

//Mutex management
bool_t osCreateMutex(OsMutex *mutex);
void osDeleteMutex(OsMutex *mutex);
void osAcquireMutex(OsMutex *mutex);
void osReleaseMutex(OsMutex *mutex);

//System time
systime_t osGetSystemTime(void);

//Memory management
void *osAllocMem(size_t size);
void osFreeMem(void *p);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
