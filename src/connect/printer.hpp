#pragma once

#include <otp.h>
#include <transfers/notify_filechange.hpp>

#include <cstdint>
#include <cstddef>
#include <tuple>
#include <optional>

namespace connect_client {

class Printer : public transfers::NotifyFilechange {
public:
    struct PrinterInfo {
        static constexpr size_t SER_NUM_STR_LEN = 19;
        static constexpr size_t SER_NUM_BUFR_LEN = (SER_NUM_STR_LEN + 1);

        static constexpr size_t FINGERPRINT_SIZE = 50;
        static constexpr size_t FINGERPRINT_BUFF_LEN = (FINGERPRINT_SIZE + 1);
        static constexpr size_t FINGERPRINT_HDR_SIZE = 16;

        bool appendix;
        const char *firmware_version;
        serial_nr_t serial_number;
        char fingerprint[FINGERPRINT_BUFF_LEN];
    };

    enum class DeviceState {
        Unknown,
        Idle,
        Printing,
        Paused,
        Finished,
        Ready,
        Busy,
        Attention,
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
        // Note: These strings live in a shared buffer in the real implementation. As a result:
        // * These may be set to NULL in case the buffer is in use by someone else.
        // * They get invalidated by someone else acquiring the buffer; that
        //   may happen in parsing a command, for example. If unsure, call
        //   renew() - that would set it to NULL in such case.
        const char *job_path;
        const char *job_lfn;
        // Type of filament loaded. Constant (in-code) strings.
        const char *material;
        uint16_t flow_factor;
        uint16_t job_id;
        uint16_t print_fan_rpm;
        uint16_t heatbreak_fan_rpm;
        uint16_t print_speed;
        uint32_t print_duration;
        uint32_t time_to_end;
        uint8_t progress_percent;
        bool has_usb;
        uint64_t usb_space_free;
        DeviceState state;

        uint32_t telemetry_fingerprint(bool include_xy_axes) const;
    };

    struct Config {
        static constexpr size_t CONNECT_URL_LEN = 35;
        static constexpr size_t CONNECT_URL_BUF_LEN = (CONNECT_URL_LEN + 1);
        static constexpr size_t CONNECT_TOKEN_LEN = 20;
        static constexpr size_t CONNECT_TOKEN_BUF_LEN = (CONNECT_TOKEN_LEN + 1);

        char host[CONNECT_URL_BUF_LEN] = "";
        char token[CONNECT_TOKEN_BUF_LEN] = "";
        uint16_t port = 0;
        bool tls = true;
        bool enabled = false;
        // Used only through loading.
        bool loaded = false;

        uint32_t crc() const;
    };

    enum class Iface {
        Ethernet,
        Wifi,
    };

    struct NetInfo {
        uint8_t ip[4];
        uint8_t mac[6];
    };

    struct NetCreds {
        static constexpr size_t SSID_BUF = 33;
        static constexpr size_t KEY_BUF = 17;
        char ssid[SSID_BUF];
        char pl_password[KEY_BUF];
    };

    enum class JobControl {
        Pause,
        Resume,
        Stop,
    };

protected:
    PrinterInfo info;
    virtual Config load_config() = 0;

private:
    // For checking if config changed. We ignore the 1:2^32 possibility of collision.
    uint32_t cfg_fingerprint = 0;

public:
    virtual void renew(/* TODO: Flags? */) = 0;
    const PrinterInfo &printer_info() const {
        return info;
    }

    virtual Params params() const = 0;
    virtual std::optional<NetInfo> net_info(Iface iface) const = 0;
    virtual NetCreds net_creds() const = 0;
    virtual bool job_control(JobControl) = 0;
    virtual bool start_print(const char *path) = 0;
    // Enqueues a gcode command (single one).
    //
    // FIXME: For now, this is a "black hole". It'll just submit it without any
    // kind of feedback. It'll either block if the queue is full, or just throw
    // it in. But that doesn't meen it has been executed.
    virtual void submit_gcode(const char *gcode) = 0;
    virtual bool set_ready(bool ready) = 0;
    virtual bool is_printing() const = 0;
    virtual uint32_t files_hash() const = 0;
    // Turn connect on and set the token.
    //
    // Part of registration.
    //
    // (The other config ‒ hostname, port, … ‒ are left unchanged).
    //
    // (Not const char * for technical reasons).
    virtual void init_connect(char *token) = 0;

    // Returns a newly reloaded config and a flag if it changed since last load
    // (unless the reset_fingerprint is set to false, in which case the flag is
    // kept).
    std::tuple<Config, bool> config(bool reset_fingerprint = true);

    virtual ~Printer() = default;

    uint32_t info_fingerprint() const;
};

}
