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
// char command_data[128];
std::string command_data = "";

/// dummy sending function to compare 
/// AT COMMANDS stored in `command_data` buffer
static size_t command_fn(const void *data, size_t len) {
    // if (len > 0 && data != NULL) {
        // strcpy(command_data, (char *)data);
    // }
    command_data += std::string((char *)data, len);
    return len;
}

TEST_CASE("ESP - AT API") {

    esp.ll.send_fn = command_fn;

    SECTION("RESET COMMAND") {
        // memset(command_data, '\0', 128);

        esp_msg_t msg;
        msg.cmd = ESP_CMD_RESET;
        esp.msg = &msg;

        // esp_cmd_t c = esp.msg->cmd;
        // CHECK(c == ESP_CMD_RESET);
        // CHECK(CMD_GET_CUR() == ESP_CMD_RESET);

        esp_res_t res = espi_initiate_cmd(&msg);
        // CHECK(res == espOK);

        // CHECK(esp.ll.send_fn != NULL);
        // CHECK(CMD_GET_CUR() == ESP_CMD_RESET);

        char cmd[] = "AT+RST\r\n";
        // char cmd[] = "\r\n";
        CHECK(cmd == command_data);
    }
}
