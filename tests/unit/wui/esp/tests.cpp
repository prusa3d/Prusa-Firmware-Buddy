#include "catch2/catch.hpp"

#include <iostream>
#include <assert.h>
#include <array>

// #include "esp.h"
#include "esp_debug.h"
#include "esp_opt.h"
#include "esp_utils.h"
#include "esp_private.h"
#include "esp_int.h"
// #include "esp_int.h"

esp_t esp;
char command_data[128];

static size_t command_fn(const void *data, size_t len) {
    // command_data
    strcpy(command_data, (char *)data);
    return len;
}

TEST_CASE("ESP - AT API") {

    SECTION("RESET COMMAND") {
        memset(command_data, '\0', 128);
        esp.msg->cmd = ESP_CMD_RESET;
        espi_initiate_cmd(esp.msg);

        // char cmd[] = "AT+RST\r\n";
        char cmd[] = "\r\n";
        CHECK(strcmp(cmd, command_data) == 0);
    }
}
