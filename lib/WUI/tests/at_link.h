/*
 * at_link.h
 *
 *  Created on: May 11, 2021
 *      Author: joshy
 */
#pragma once

#include "cmsis_os.h"

#define ESP_DMA_TESTLOOP_CNT 100

void esp_dma_test(void);

int atlink_input_process(uint8_t *, size_t);

size_t send_data(const void *data, size_t len);

uint8_t reset_device(uint8_t state);

void configure_uart(uint32_t baudrate);

void handle_lwesp_rx_data();
