#include "catch2/catch.hpp"

#include <iostream>
#include <assert.h>
#include <array>

#include "lwesp/lwesp.h"
#include "lwesp/lwesp_private.h"

lwesp_t esp;
std::string command_data = "";

/// dummy sending function to compare
/// AT COMMANDS stored in `command_data` buffer
static size_t command_fn(const void *data, size_t len) {
    command_data += std::string((char *)data, len);
    return len;
}

// old char* fn
// char command_data[128];
// static size_t command_fn(const void *data, size_t len) {
// if (len > 0 && data != NULL) {
// strcpy(command_data, (char *)data);
// }
// return len;
// }

TEST_CASE("ESP - AT API") {

    /// add dummy fn for sending commands
    esp.ll.send_fn = command_fn;
    /// create msg to store into esp global value
    lwesp_msg_t msg;
    esp.msg = &msg;
    /// response
    lwespr_t res = lwespERR;

    // SECTION("TEST - old") {
    // memset(command_data, '\0', 128);
    // esp_msg_t msg;
    // msg.cmd = ESP_CMD_RESET;
    // esp.msg = &msg;
    // esp_res_t res = espi_initiate_cmd(&msg);
    // CHECK(res == espOK);
    // char cmd[] = "AT+RST\r\n";
    // CHECK(strcmp(cmd, command_data) == 0);
    // }

    SECTION("RESET COMMAND") {
        /// clear check string
        command_data = "";
        /// set AT command
        esp.msg->cmd = LWESP_CMD_RESET;
        /// build AT command
        res = lwespi_initiate_cmd(&msg);
        CHECK(res == lwespOK);
        /// check generated AT command
        char cmd[] = "AT+RST\r\n";
        CHECK(cmd == command_data);
    }

    SECTION("GET VERSION COMMAND") {
        command_data = "";
        esp.msg->cmd = LWESP_CMD_GMR;
        res = lwespi_initiate_cmd(&msg);
        CHECK(res == lwespOK);
        char cmd[] = "AT+GMR\r\n";
        CHECK(cmd == command_data);
    }

    SECTION("JOIN AP - test with arguments") {
        command_data = "";

        esp.msg->cmd = LWESP_CMD_WIFI_CWJAP;
        esp.msg->msg.sta_join.name = "chleba";
        esp.msg->msg.sta_join.pass = "maslo";
        esp.msg->msg.sta_join.mac = NULL;

        res = lwespi_initiate_cmd(&msg);
        CHECK(res == lwespOK);
        char cmd[] = "AT+CWJAP=\"chleba\",\"maslo\"\r\n";
        CHECK(cmd == command_data);
    }

    SECTION("WIFI MODE - test esp_utils") {
        command_data = "";

        esp.msg->cmd = LWESP_CMD_WIFI_CWMODE;
        // need to be def (is set when called via API fn, now just for test)
        esp.msg->cmd_def = LWESP_CMD_WIFI_CWMODE;
        esp.msg->msg.wifi_mode.mode = LWESP_MODE_AP;

        res = lwespi_initiate_cmd(&msg);
        CHECK(res == lwespOK);
        char cmd[] = "AT+CWMODE=2\r\n";
        CHECK(cmd == command_data);
    }

    SECTION("TCP - start connection (TCP)") {
        command_data = "";

        esp.msg->cmd = LWESP_CMD_TCPIP_CIPSTART;
        esp.msg->msg.conn_start.type = LWESP_CONN_TYPE_TCP;
        esp.msg->msg.conn_start.local_ip = NULL;
        esp.msg->msg.conn_start.tcp_ssl_keep_alive = 0;
        esp.msg->msg.conn_start.remote_host = "prusa3d.cz";
        esp.msg->msg.conn_start.remote_port = 80;

        res = lwespi_initiate_cmd(&msg);
        CHECK(res == lwespOK);

        // check created connection
        lwesp_conn_t *conn = NULL;
        conn = *esp.msg->msg.conn_start.conn;
        CHECK(conn != NULL);

        // char cmd[] = "AT+CIPSTART=4,\"TCP\",\"prusa3d.cz\",80,0\r\n";
        std::string c = "AT+CIPSTART=4,\"TCP\",\"prusa3d.cz\",80,0\r\n";
        CHECK(command_data.find(c) != std::string::npos);
        CHECK(c == command_data);
    }

    SECTION("TCP - send data") {
      // sending data of predefined size
      // for test we just test AT command generation without any data
        command_data = "";
        esp.msg->cmd = LWESP_CMD_TCPIP_CIPSEND;
        res = lwespi_initiate_cmd(&msg);
        CHECK(res == lwespOK);
        // we don't have any msg stored so zero length will be send
        std::string c = "AT+CIPSEND=0,0\r\n";
        CHECK(command_data.find(c) != std::string::npos);
        CHECK(c == command_data);
    }

    SECTION("STATION MODE API test") {
      command_data = "";
      // esp_sta_join("chleba", "maslo", NULL, NULL, NULL, 0);
      CHECK(1 == 1);


    }

}
