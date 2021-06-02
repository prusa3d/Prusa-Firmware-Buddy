/**
 * @file os_port_freertos.c
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

//Switch to the appropriate trace level
#define TRACE_LEVEL TRACE_LEVEL_OFF

//Dependencies
#include <stdio.h>
#include <stdlib.h>
#include "os_port.h"
#include "os_port_freertos.h"
#include "cyclone_debug.h"


/**
 * @brief Kernel initialization
 **/

void osInitKernel(void)
{
}


/**
 * @brief Start kernel
 **/

void osStartKernel(void)
{
   //Start the scheduler
   vTaskStartScheduler();
}


/**
 * @brief Create a new task
 * @param[in] name A name identifying the task
 * @param[in] taskCode Pointer to the task entry function
 * @param[in] param A pointer to a variable to be passed to the task
 * @param[in] stackSize The initial size of the stack, in words
 * @param[in] priority The priority at which the task should run
 * @return If the function succeeds, the return value is a pointer to the
 *   new task. If the function fails, the return value is NULL
 **/

OsTask *osCreateTask(const char_t *name, OsTaskCode taskCode,
   void *param, size_t stackSize, int_t priority)
{
   portBASE_TYPE status;
   TaskHandle_t task = NULL;

   //Create a new task
   status = xTaskCreate((TaskFunction_t) taskCode, name, stackSize, param,
      priority, &task);

   //Check whether the task was successfully created
   if(status == pdPASS)
      return task;
   else
      return NULL;
}


/**
 * @brief Delete a task
 * @param[in] task Pointer to the task to be deleted
 **/

void osDeleteTask(OsTask *task)
{
   //Delete the specified task
   vTaskDelete((TaskHandle_t) task);
}


/**
 * @brief Delay routine
 * @param[in] delay Amount of time for which the calling task should block
 **/

void osDelayTask(systime_t delay)
{
   //Delay the task for the specified duration
   vTaskDelay(OS_MS_TO_SYSTICKS(delay));
}


/**
 * @brief Yield control to the next task
 **/

void osSwitchTask(void)
{
   //Force a context switch
   taskYIELD();
}


/**
 * @brief Suspend scheduler activity
 **/

void osSuspendAllTasks(void)
{
   //Make sure the operating system is running
   if(xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
   {
      //Suspend all tasks
      vTaskSuspendAll();
   }
}


/**
 * @brief Resume scheduler activity
 **/

void osResumeAllTasks(void)
{
   //Make sure the operating system is running
   if(xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
   {
      //Resume all tasks
      xTaskResumeAll();
   }
}


/**
 * @brief Create an event object
 * @param[in] event Pointer to the event object
 * @return The function returns TRUE if the event object was successfully
 *   created. Otherwise, FALSE is returned
 **/

bool_t osCreateEvent(OsEvent *event)
{
#if (configSUPPORT_STATIC_ALLOCATION == 1)
   //Create a binary semaphore
   event->handle = xSemaphoreCreateBinaryStatic(&event->buffer);
#else
   //Create a binary semaphore
   event->handle = xSemaphoreCreateBinary();
#endif

   //Check whether the returned handle is valid
   if(event->handle != NULL)
      return TRUE;
   else
      return FALSE;
}


/**
 * @brief Delete an event object
 * @param[in] event Pointer to the event object
 **/

void osDeleteEvent(OsEvent *event)
{
   //Make sure the handle is valid
   if(event->handle != NULL)
   {
      //Properly dispose the event object
      vSemaphoreDelete(event->handle);
   }
}


/**
 * @brief Set the specified event object to the signaled state
 * @param[in] event Pointer to the event object
 **/

void osSetEvent(OsEvent *event)
{
   //Set the specified event to the signaled state
   xSemaphoreGive(event->handle);
}


/**
 * @brief Set the specified event object to the nonsignaled state
 * @param[in] event Pointer to the event object
 **/

void osResetEvent(OsEvent *event)
{
   //Force the specified event to the nonsignaled state
   xSemaphoreTake(event->handle, 0);
}


/**
 * @brief Wait until the specified event is in the signaled state
 * @param[in] event Pointer to the event object
 * @param[in] timeout Timeout interval
 * @return The function returns TRUE if the state of the specified object is
 *   signaled. FALSE is returned if the timeout interval elapsed
 **/

bool_t osWaitForEvent(OsEvent *event, systime_t timeout)
{
   portBASE_TYPE ret;

   //Wait until the specified event is in the signaled state
   if(timeout == INFINITE_DELAY)
   {
      //Infinite timeout period
      ret = xSemaphoreTake(event->handle, portMAX_DELAY);
   }
   else
   {
      //Wait for the specified time interval
      ret = xSemaphoreTake(event->handle, OS_MS_TO_SYSTICKS(timeout));
   }

   //The return value tells whether the event is set
   return ret;
}


/**
 * @brief Set an event object to the signaled state from an interrupt service routine
 * @param[in] event Pointer to the event object
 * @return TRUE if setting the event to signaled state caused a task to unblock
 *   and the unblocked task has a priority higher than the currently running task
 **/

bool_t osSetEventFromIsr(OsEvent *event)
{
   portBASE_TYPE flag = FALSE;

   //Set the specified event to the signaled state
   xSemaphoreGiveFromISR(event->handle, &flag);

   //A higher priority task has been woken?
   return flag;
}


/**
 * @brief Create a semaphore object
 * @param[in] semaphore Pointer to the semaphore object
 * @param[in] count The maximum count for the semaphore object. This value
 *   must be greater than zero
 * @return The function returns TRUE if the semaphore was successfully
 *   created. Otherwise, FALSE is returned
 **/

bool_t osCreateSemaphore(OsSemaphore *semaphore, uint_t count)
{
#if (configSUPPORT_STATIC_ALLOCATION == 1)
   //Create a semaphore
   semaphore->handle = xSemaphoreCreateCountingStatic(count, count,
      &semaphore->buffer);
#else
   //Create a semaphore
   semaphore->handle = xSemaphoreCreateCounting(count, count);
#endif

   //Check whether the returned handle is valid
   if(semaphore->handle != NULL)
      return TRUE;
   else
      return FALSE;
}


/**
 * @brief Delete a semaphore object
 * @param[in] semaphore Pointer to the semaphore object
 **/

void osDeleteSemaphore(OsSemaphore *semaphore)
{
   //Make sure the handle is valid
   if(semaphore->handle != NULL)
   {
      //Properly dispose the specified semaphore
      vSemaphoreDelete(semaphore->handle);
   }
}


/**
 * @brief Wait for the specified semaphore to be available
 * @param[in] semaphore Pointer to the semaphore object
 * @param[in] timeout Timeout interval
 * @return The function returns TRUE if the semaphore is available. FALSE is
 *   returned if the timeout interval elapsed
 **/

bool_t osWaitForSemaphore(OsSemaphore *semaphore, systime_t timeout)
{
   portBASE_TYPE ret;

   //Wait until the specified semaphore becomes available
   if(timeout == INFINITE_DELAY)
   {
      //Infinite timeout period
      ret = xSemaphoreTake(semaphore->handle, portMAX_DELAY);
   }
   else
   {
      //Wait for the specified time interval
      ret = xSemaphoreTake(semaphore->handle, OS_MS_TO_SYSTICKS(timeout));
   }

   //The return value tells whether the semaphore is available
   return ret;
}


/**
 * @brief Release the specified semaphore object
 * @param[in] semaphore Pointer to the semaphore object
 **/

void osReleaseSemaphore(OsSemaphore *semaphore)
{
   //Release the semaphore
   xSemaphoreGive(semaphore->handle);
}


/**
 * @brief Create a mutex object
 * @param[in] mutex Pointer to the mutex object
 * @return The function returns TRUE if the mutex was successfully
 *   created. Otherwise, FALSE is returned
 **/

bool_t osCreateMutex(OsMutex *mutex)
{
#if (configSUPPORT_STATIC_ALLOCATION == 1)
   //Create a mutex object
   mutex->handle = xSemaphoreCreateMutexStatic(&mutex->buffer);
#else
   //Create a mutex object
   mutex->handle = xSemaphoreCreateMutex();
#endif

   //Check whether the returned handle is valid
   if(mutex->handle != NULL)
      return TRUE;
   else
      return FALSE;
}


/**
 * @brief Delete a mutex object
 * @param[in] mutex Pointer to the mutex object
 **/

void osDeleteMutex(OsMutex *mutex)
{
   //Make sure the handle is valid
   if(mutex->handle != NULL)
   {
      //Properly dispose the specified mutex
      vSemaphoreDelete(mutex->handle);
   }
}


/**
 * @brief Acquire ownership of the specified mutex object
 * @param[in] mutex Pointer to the mutex object
 **/

void osAcquireMutex(OsMutex *mutex)
{
   //Obtain ownership of the mutex object
   xSemaphoreTake(mutex->handle, portMAX_DELAY);
}


/**
 * @brief Release ownership of the specified mutex object
 * @param[in] mutex Pointer to the mutex object
 **/

void osReleaseMutex(OsMutex *mutex)
{
   //Release ownership of the mutex object
   xSemaphoreGive(mutex->handle);
}


/**
 * @brief Retrieve system time
 * @return Number of milliseconds elapsed since the system was last started
 **/

systime_t osGetSystemTime(void)
{
   systime_t time;

   //Get current tick count
   time = xTaskGetTickCount();

   //Convert system ticks to milliseconds
   return OS_SYSTICKS_TO_MS(time);
}


/**
 * @brief Allocate a memory block
 * @param[in] size Bytes to allocate
 * @return A pointer to the allocated memory block or NULL if
 *   there is insufficient memory available
 **/

void *osAllocMem(size_t size)
{
   void *p;

   //Allocate a memory block
   p = pvPortMalloc(size);

   //Debug message
   TRACE_DEBUG("Allocating %" PRIuSIZE " bytes at 0x%08" PRIXPTR "\r\n", size, (uintptr_t) p);

   //Return a pointer to the newly allocated memory block
   return p;
}


/**
 * @brief Release a previously allocated memory block
 * @param[in] p Previously allocated memory block to be freed
 **/

void osFreeMem(void *p)
{
   //Make sure the pointer is valid
   if(p != NULL)
   {
      //Debug message
      TRACE_DEBUG("Freeing memory at 0x%08" PRIXPTR "\r\n", (uintptr_t) p);

      //Free memory block
      vPortFree(p);
   }
}


#if 0

/**
 * @brief FreeRTOS stack overflow hook
 **/

void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
   (void) pcTaskName;
   (void) pxTask;

   taskDISABLE_INTERRUPTS();
   while(1);
}


/**
 * @brief Trap FreeRTOS errors
 **/

void vAssertCalled(const char *pcFile, unsigned long ulLine)
{
   volatile unsigned long ul = 0;

   (void) pcFile;
   (void) ulLine;

   taskENTER_CRITICAL();

   //Set ul to a non-zero value using the debugger to step out of this function
   while(ul == 0)
   {
      portNOP();
   }

   taskEXIT_CRITICAL();
}

#endif
