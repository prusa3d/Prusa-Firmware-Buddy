/*
This is an interface function between core of the buddy firmware and Connect.
Firmware team is free to change the implementation.
Currently information about the inner parameters of printer is acquired
using marlin variables.
*/
#pragma once
#include <marlin_client.h>
#include <optional>
#include <otp.h>
#include "connect_error.h"

namespace con {

constexpr uint8_t FW_VER_STR_LEN = 32;
constexpr uint8_t FW_VER_BUFR_LEN = (FW_VER_STR_LEN + 1);

constexpr uint8_t SER_NUM_STR_LEN = 19;
constexpr uint8_t SER_NUM_BUFR_LEN = (SER_NUM_STR_LEN + 1);

constexpr uint8_t FINGERPRINT_SIZE = 16;
constexpr uint8_t FINGERPRINT_BUFF_LEN = (FINGERPRINT_SIZE + 1);

constexpr uint8_t CONNECT_URL_LEN = 30;
constexpr uint8_t CONNECT_URL_BUF_LEN = (CONNECT_URL_LEN + 1);
constexpr uint8_t CONNECT_TOKEN_LEN = 20;
constexpr uint8_t CONNECT_TOKEN_BUF_LEN = (CONNECT_TOKEN_LEN + 1);

#define X_AXIS_POS 0
#define Y_AXIS_POS 1
#define Z_AXIS_POS 2

struct configuration_t {
    char host[CONNECT_URL_BUF_LEN] = "";
    char token[CONNECT_TOKEN_BUF_LEN] = "";
    uint16_t port = 0;
    bool tls = true;
    bool enabled = false;
};

struct printer_info_t {
    uint8_t printer_type;
    bool appendix;
    char firmware_version[FW_VER_BUFR_LEN];
    char serial_number[SER_NUM_BUFR_LEN];
    char fingerprint[FINGERPRINT_BUFF_LEN];
};

enum device_state {
    DEVICE_STATE_UNKNOWN,
    DEVICE_STATE_READY,
    DEVICE_STATE_PRINTING,
    DEVICE_STATE_PAUSED,
    DEVICE_STATE_FINISHED,
    DEVICE_STATE_PREPARED,
    DEVICE_STATE_ERROR,
};

struct device_params_t {
    float temp_nozzle;
    float temp_bed;
    float target_nozzle;
    float target_bed;
    float pos[4];
    uint16_t print_speed;
    uint16_t flow_factor;
    device_state state;
};

class core_interface {
private:
    marlin_vars_t *marlin_vars;

public:
    core_interface();
    void get_data(device_params_t *parmas);
    std::optional<Error> get_printer_info(printer_info_t *printer_info);
    configuration_t get_connect_config();
};

bool load_config_from_ini();

}
