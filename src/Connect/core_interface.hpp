/*
This is an interface function between core of the buddy firmware and Connect.
Firmware team is free to change the implementation.
Currently information about the inner parameters of printer is acquired
using marlin variables.
*/
#pragma once
#include "connect_error.h"

#include <marlin_client.h>
#include <otp.h>

#include <variant>

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
    bool appendix;
    char firmware_version[FW_VER_BUFR_LEN];
    char serial_number[SER_NUM_BUFR_LEN];
    char fingerprint[FINGERPRINT_BUFF_LEN];
};

enum class DeviceState {
    Unknown,
    Idle,
    Printing,
    Paused,
    Finished,
    Prepared,
    Error,
};

struct device_params_t {
    float temp_nozzle;
    float temp_bed;
    float target_nozzle;
    float target_bed;
    float pos[4];
    const char *job_path;
    uint16_t flow_factor;
    uint16_t job_id;
    uint16_t print_fan_rpm;
    uint16_t heatbreak_fan_rpm;
    uint16_t print_speed;
    uint32_t print_duration;
    uint32_t time_to_end;
    uint8_t progress_percent;
    DeviceState state;
};

class core_interface {
private:
    marlin_vars_t *marlin_vars;

public:
    core_interface();
    device_params_t get_data();
    printer_info_t get_printer_info();
    configuration_t get_connect_config();
};

bool load_config_from_ini();

}
