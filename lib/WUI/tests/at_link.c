/*
 * at_link.c
 *
 *  Created on: May 11, 2021
 *      Author: joshy
 */

#include "at_link.h"

#include "stm32f4xx_hal.h"
#include "main.h"
#include "cmsis_os.h"
#include "string.h"

typedef struct {
    uint8_t *start;
    size_t len;
} buffer_range_t;

osMailQDef(buff_range_pool, 10, buffer_range_t);
osMailQId(buff_range_pool_id);

typedef enum {
    AT_AT,
    AT_CWMODE,
    AT_CWJAP,
    AT_CIFSR,
    AT_CIPMUX,
    AT_PING,
    AT_DATA
} AT_COMMAND_SENT;

int atlink_input_process(uint8_t *start, size_t len) {
    buffer_range_t *atbuffer;
    atbuffer = (buffer_range_t *)osMailAlloc(buff_range_pool_id, 1000);
    atbuffer->start = start;
    atbuffer->len = len;
    osMailPut(buff_range_pool_id, atbuffer);
    return 0;
}

static uint32_t parse_reply(uint32_t delay, AT_COMMAND_SENT cmd) {
    osDelay(delay);
    uint32_t loopTrue = 1;
    uint32_t ret = -1;
    uint8_t packet_start = 0;
    uint8_t packet_index = 0;
    uint8_t start_cmp = 0;
    uint8_t temp_buff[20];
    do {
        osEvent event = osMailGet(buff_range_pool_id, 1000);
        if (event.status != osEventMail) {
            loopTrue = 0;
        } else {
            buffer_range_t *received = (buffer_range_t *)event.value.p;
            const char *start = (const char *)received->start;
            size_t len = received->len;
            switch (cmd) {
            case AT_PING:
                for (uint32_t i = 0; i < len; i++) {
                    if (!packet_start) {
                        if (0 == strncmp("+", (start), 1)) {
                            packet_start = 1;
                        }
                    }
                    if (packet_start && (packet_index < 20)) {
                        temp_buff[packet_index] = *(start + i);
                        if (!start_cmp) {
                            if (0 == strncmp("\r", (start + i), 1)) {
                                start_cmp = packet_index;
                            }
                        }
                        packet_index++;
                    }
                    if (start_cmp) {
                        if (0 == strncmp("\r\n\r\nOK\r\n", (const char *)(temp_buff + start_cmp), 8)) {
                            printf("Ping ok");
                            ret = 0;
                        }
                    }
                }
                break;
            default:
                break;
            }
            osMailFree(buff_range_pool_id, received);
        }
        osDelay(50); // delay to receive packets when buffer end reached and starts from beginning
    } while (loopTrue);

    return ret;
}

void esp_dma_test(void) {
    buff_range_pool_id = osMailCreate(osMailQ(buff_range_pool), NULL);
    reset_device(1);
    osDelay(1000);
    reset_device(0);
    osDelay(3000);
    configure_uart(115200);
    char at_command[1000];
    uint32_t success_count = 0;
    uint32_t fail_count = 0;
    for (uint32_t i = 0; i < ESP_DMA_TESTLOOP_CNT; i++) {
        strlcpy(at_command, "AT\r\n", 1000);
        send_data(at_command, strlen(at_command));
        parse_reply(1000, AT_AT);

        strlcpy(at_command, "AT+CWMODE=1\r\n", 1000);
        send_data(at_command, strlen(at_command));
        parse_reply(1000, AT_CWMODE);

        strlcpy(at_command, "AT+CWJAP=\"ssid\",\"passwd\"\r\n", 1000);
        send_data(at_command, strlen(at_command));
        parse_reply(6000, AT_CWJAP);

        strlcpy(at_command, "AT+CIFSR\r\n", 1000);
        send_data(at_command, strlen(at_command));
        parse_reply(1000, AT_CIFSR);

        strlcpy(at_command, "AT+PING=\"www.google.com\"\r\n", 1000);
        send_data(at_command, strlen(at_command));
        uint32_t retval = parse_reply(1000, AT_PING);
        if (0 == retval) {
            success_count++;
        } else {
            fail_count++;
        }
        if (success_count < fail_count) {
            Error_Handler();
        }
        osDelay(3000);
    }
}
