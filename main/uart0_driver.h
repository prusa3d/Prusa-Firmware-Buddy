// UART0 driver
//
// Copyright (C) 2023 Prusa Research a.s - www.prusa3d.com
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Register custom UART0 driver.
// For description of `uart0_rx_task_handle` and `uart0_tx_task_handle` parameters,
// see documentation of `uart0_rx_bytes()` and `uart0_tx_bytes()` functions.
esp_err_t uart0_driver_install(TaskHandle_t uart0_rx_task_handle, TaskHandle_t uart0_tx_task_handle);

// Receive `size` bytes from UART0 into `data`.
// This must be called exclusively by RTOS task identified by `uart0_rx_task_handle`,
// because we are using task notifications to implement lightweight binary semaphore.
// Before calling this function for the first time, the task must call `ulTaskNotifyTake()`
// to ensure proper synchronization with `uart0_driver_install()`
void uart0_rx_bytes(uint8_t *data, uint32_t size);

// Transmit `size` bytes from `data` into UART0.
// This must be called exclusively by RTOS task identified by `uart0_tx_task_handle`,
// because we are using task notifications to implement lightweight binary semaphore.
// Before calling this function for the first time, the task must call `ulTaskNotifyTake()`
// to ensure proper synchronization with `uart0_driver_install()`
void uart0_tx_bytes(uint8_t *data, size_t size);

