#include "catch2/catch.hpp"

#include <iostream>
#include <assert.h>
#include <array>

// esp_t esp;

// #include "esp.h"
#include "esp_debug.h"
#include "esp_opt.h"
#include "esp_utils.h"
#include "esp_private.h"
#include "esp_int.h"

esp_t esp;
#include "esp_int.h"
char command_data[128];

static size_t command_fn(const void *data, size_t len) {
    // command_data
    strcpy(command_data, (char *)data);
    return len;
}

// esp.ll.send_fn = command_fn;

// esp_ll_t *ll = &esp.ll;
// ll->send_fn = command_fn;

TEST_CASE("ESP - AT API") {

    esp.ll.send_fn = command_fn;

    SECTION("RESET COMMAND") {
        memset(command_data, '\0', 128);

        esp_msg_t msg;
        msg.cmd = ESP_CMD_RESET;
        esp.msg = &msg;
        // esp.msg->cmd = ESP_CMD_RESET;

        esp_cmd_t c = esp.msg->cmd;

        // CHECK(c == ESP_CMD_RESET);
        // CHECK(CMD_GET_CUR() == ESP_CMD_RESET);

        espi_initiate_cmd(esp.msg);

        CHECK(CMD_GET_CUR() == ESP_CMD_RESET);

        // char cmd[] = "AT+RST\r\n";
        // char cmd[] = "\r\n";
        // CHECK(strcmp(cmd, command_data) == 0);
    }
}
