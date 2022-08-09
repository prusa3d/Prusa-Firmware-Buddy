#pragma once

#include <cstdint>
#include <cstddef>
#include <tuple>

namespace con {

class Printer {
public:
    struct PrinterInfo {
        static constexpr size_t SER_NUM_STR_LEN = 19;
        static constexpr size_t SER_NUM_BUFR_LEN = (SER_NUM_STR_LEN + 1);

        static constexpr size_t FINGERPRINT_SIZE = 50;
        static constexpr size_t FINGERPRINT_BUFF_LEN = (FINGERPRINT_SIZE + 1);
        static constexpr size_t FINGERPRINT_HDR_SIZE = 16;

        bool appendix;
        const char *firmware_version;
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

    static constexpr size_t X_AXIS_POS = 0;
    static constexpr size_t Y_AXIS_POS = 1;
    static constexpr size_t Z_AXIS_POS = 2;

    struct Params {
        float temp_nozzle;
        float temp_bed;
        float target_nozzle;
        float target_bed;
        float pos[4];
        float filament_used;
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

    struct Config {
        static constexpr size_t CONNECT_URL_LEN = 30;
        static constexpr size_t CONNECT_URL_BUF_LEN = (CONNECT_URL_LEN + 1);
        static constexpr size_t CONNECT_TOKEN_LEN = 20;
        static constexpr size_t CONNECT_TOKEN_BUF_LEN = (CONNECT_TOKEN_LEN + 1);

        char host[CONNECT_URL_BUF_LEN] = "";
        char token[CONNECT_TOKEN_BUF_LEN] = "";
        uint16_t port = 0;
        bool tls = true;
        bool enabled = false;

        uint32_t crc() const;
    };

protected:
    PrinterInfo info;
    virtual Config load_config() = 0;

private:
    // For checking if config changed. We ignore the 1:2^32 possibility of collision.
    uint32_t cfg_fingerprint;

public:
    virtual void renew(/* TODO: Flags? */) = 0;
    const PrinterInfo &printer_info() const {
        return info;
    }

    virtual Params params() const = 0;

    // Returns a newly reloaded config and a flag if it changed since last load.
    std::tuple<Config, bool> config();

    virtual ~Printer() = default;
};

}
