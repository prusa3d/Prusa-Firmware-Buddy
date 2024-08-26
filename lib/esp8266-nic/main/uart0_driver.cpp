// This UART0 driver uses minimal synchronization to obtain better performance.
// It is caller's responsibility to ensure all required synchronization.
// FreeRTOS task notification system is used to provide lightweigt semaphores.
//
// It is zero-copy and does most of the work inside the interrupt handler itself.
// For this to work, the receive and transmit tasks must be realtime and fast
// enough to keep the driver supplied with fresh buffers, otherwise it may start
// dropping received bytes or delay transmitted bytes.
//
// Copyright (C) 2023 Prusa Research a.s - www.prusa3d.com
// SPDX-License-Identifier: GPL-3.0-or-later
#include "uart0_driver.h"

#include "driver/uart.h"
#include "esp8266/pin_mux_register.h"
#include "esp8266/uart_register.h"
#include "esp8266/uart_struct.h"

const uint32_t rxfifo_full_thresh = 96;
const uint32_t rx_timeout_thresh = 16;
const uint32_t txfifo_empty_thresh = 32;
const uint32_t baud_rate = 4600000;

static TaskHandle_t tx_task_handle; // task to be notified after uart0_isr() finishes transmit
static uint8_t *tx_data;
static size_t tx_size;

static TaskHandle_t rx_task_handle; // task to be notified after uart0_isr() finishes receive
static uint8_t *rx_data;
static size_t rx_size;

#define FORCE_INLINE inline __attribute__((always_inline))

static FORCE_INLINE uint8_t *uart0_rx_fifo(uint8_t* data, size_t size) {
    for (const uint8_t* end = data + size; data != end; ++data) {
        *data = uart0.fifo.rw_byte; // read from register to drain hardware RX FIFO
    }
    return data;
}

static FORCE_INLINE uint8_t *uart0_tx_fifo(uint8_t* data, size_t size) {
    for (const uint8_t* end = data + size; data != end; ++data) {
        uart0.fifo.rw_byte = *data; // write to register to fill hardware TX FIFO
    }
    return data;
}

static void IRAM_ATTR uart0_isr(void *param) {
    // This ISR handles all UART0 conditions:
    //   * TX FIFO empty - happens when TX FIFO has less than `txfifo_empty_thresh` bytes
    //   * RX FIFO full  - happens when RX FIFO has more than `rxfifo_full_thresh` bytes
    //   * RX timeout    - happens when no byte is received for the duration of time it
    //                     would normally take to receive `rx_timeout_thresh` bytes
    // The handler is structured as a loop, to account for new conditions arising while
    // processing previous conditions.
    BaseType_t task_woken = pdFALSE;
    for (;;) {
        const uint32_t uart_intr_status = uart0.int_st.val;

        if (uart_intr_status == 0) {
            // No more conditions on UART0, optionally unblock waiting tasks and we are done.
            if (task_woken == pdTRUE) {
                portYIELD_FROM_ISR();
            }
            return;
        }

        if (uart_intr_status & UART_TXFIFO_EMPTY_INT_ST_M) {
            // Handle TX FIFO empty condition.

            const uint8_t tx_remain_fifo_cnt = UART_FIFO_LEN - uart0.status.txfifo_cnt;
            if (tx_size > tx_remain_fifo_cnt) {
                // Send partial data to fifo, do some bookkeeping and keep interrupt enabled.
                tx_data = uart0_tx_fifo(tx_data, tx_remain_fifo_cnt);
                tx_size -= tx_remain_fifo_cnt;
            } else {
                // Send complete data to fifo. Note that we do not need any more bookkeeping.
                (void)uart0_tx_fifo(tx_data, tx_size);

                // Disable more interrupts and unblock uart0_tx_bytes().
                uart0.int_ena.txfifo_empty = 0;
                vTaskNotifyGiveFromISR(tx_task_handle, &task_woken);
            }
            uart0.int_clr.txfifo_empty = 1;
        } else if (uart_intr_status & (UART_RXFIFO_FULL_INT_ST_M|UART_RXFIFO_TOUT_INT_ST_M)) {
            // Handle RX FIFO empty and RX timeout conditions.

            const uint8_t rx_fifo_cnt = uart0.status.rxfifo_cnt;
            if (rx_size > rx_fifo_cnt) {
                // Load partial data from fifo, do some bookkeeping and keep interrupt enabled.
                rx_data = uart0_rx_fifo(rx_data, rx_fifo_cnt);
                rx_size -= rx_fifo_cnt;
            } else {
                // Load complete data from fifo. Note that we do not need any more bookkeeping.
                (void)uart0_rx_fifo(rx_data, rx_size);

                // Disable more interrupts and unblock uart0_rx_bytes().
                uart0.int_ena.rxfifo_full = 0;
                uart0.int_ena.rxfifo_tout = 0;
                vTaskNotifyGiveFromISR(rx_task_handle, &task_woken);
            }
            uart0.int_clr.rxfifo_full = 1;
            uart0.int_clr.rxfifo_tout = 1;
        } else {
            // Note: This should never happen in current setup, other interrupts are not enabled.
            //       But just to be sure and to prevent infinite loop, let's clear the status bit.
            // TODO: Think more about overflows, frame errors and such.
            uart0.int_clr.val = uart_intr_status;
        }
    }
}

void IRAM_ATTR uart0_tx_bytes(uint8_t *data, size_t size) {
    const uint8_t tx_remain_fifo_cnt = UART_FIFO_LEN - uart0.status.txfifo_cnt;
    if (size > tx_remain_fifo_cnt) {
        // send partial data to fifo
        tx_data = uart0_tx_fifo(data, tx_remain_fifo_cnt);
        tx_size = size - tx_remain_fifo_cnt;

        // delegate rest of the work to interrupt handler
        taskENTER_CRITICAL();
        uart0.int_ena.txfifo_empty = 1;
        taskEXIT_CRITICAL();

        // wait for interrupt handler and we are done
        (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    } else {
        // send complete data to fifo and we are done
        (void)uart0_tx_fifo(data, size);
    }
}

void IRAM_ATTR uart0_rx_bytes(uint8_t *data, size_t size) {
    const uint8_t rx_fifo_cnt = uart0.status.rxfifo_cnt;
    if (size > rx_fifo_cnt) {
        // load partial data from fifo
        rx_data = uart0_rx_fifo(data, rx_fifo_cnt);
        rx_size = size - rx_fifo_cnt;

        // delegate rest of the work to interrupt handler
        taskENTER_CRITICAL();
        uart0.int_ena.rxfifo_full = 1;
        uart0.int_ena.rxfifo_tout = 1;
        taskEXIT_CRITICAL();

        // wait for interrupt handler and we are done
        (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    } else {
        // load complete data from fifo and we are done
        (void)uart0_rx_fifo(data, size);
    }
}

esp_err_t uart0_driver_install(TaskHandle_t uart0_rx_task_handle, TaskHandle_t uart0_tx_task_handle) {
    if (uart_is_driver_installed(UART_NUM_0)) {
        return ESP_FAIL;
    }

    portENTER_CRITICAL();
    PIN_PULLUP_DIS(PERIPHS_IO_MUX_U0TXD_U);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0RXD_U, FUNC_U0RXD);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD);
    uart0.clk_div.val = (uint32_t)(UART_CLK_FREQ / baud_rate) & 0xFFFFF;
    uart0.conf0.bit_num = UART_DATA_8_BITS;
    uart0.conf0.parity = (UART_PARITY_DISABLE & 0x1);
    uart0.conf0.parity_en = ((UART_PARITY_DISABLE >> 1) & 0x1);
    uart0.conf0.stop_bit_num = UART_STOP_BITS_1;
    uart0.conf0.tx_flow_en = 0;
    uart0.conf1.rx_flow_en = 0;
    uart0.conf1.rx_tout_en = 1;
    uart0.conf1.rx_tout_thrhd = (rx_timeout_thresh & 0x7f);
    uart0.conf1.rxfifo_full_thrhd = rxfifo_full_thresh;
    uart0.conf1.txfifo_empty_thrhd = txfifo_empty_thresh;

    // uart_reset_rx_fifo
    uart0.conf0.rxfifo_rst = 0x1;
    uart0.conf0.rxfifo_rst = 0x0;

    // uart_clear_intr_status
    uart0.int_clr.val |= UART_INTR_MASK;
    _xt_isr_unmask(0x1 << 0);

    portEXIT_CRITICAL();

    // Note: We plug into original ISR in order to keep UART_NUM_1 working.
    if (uart_isr_register(UART_NUM_0, uart0_isr, NULL) != ESP_OK) {
        return ESP_FAIL;
    }

    rx_task_handle = uart0_rx_task_handle;
    tx_task_handle = uart0_tx_task_handle;
    xTaskNotifyGive(tx_task_handle);
    xTaskNotifyGive(rx_task_handle);

    return ESP_OK;
}
