#pragma once

#include <otp.hpp>
#include <common/shared_buffer.hpp>

#include <cstdint>
#include <cstddef>
#include <tuple>
#include <optional>

#include "printer_type.hpp"
#include <state/printer_state.hpp>

namespace connect_client {

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
        serial_nr_t serial_number;
        char fingerprint[FINGERPRINT_BUFF_LEN];
    };

    static constexpr size_t X_AXIS_POS = 0;
    static constexpr size_t Y_AXIS_POS = 1;
    static constexpr size_t Z_AXIS_POS = 2;

    class Params {
    private:
        // Living in the Printer we come from
        const std::optional<BorrowPaths> &paths;

    public:
        Params(const std::optional<BorrowPaths> &paths);
        float temp_nozzle = 0;
        float temp_bed = 0;
        float target_nozzle = 0;
        float target_bed = 0;
        float pos[4] = {};
        float filament_used = 0;
        // FIXME: We should handle XL with up to 5 nozzles, but the network protocol
        // does not support it as of now, so for the time being we just send the first one.
        float nozzle_diameter = 0;
        // Note: These strings live in a shared buffer in the real implementation. As a result:
        // * These are NULL unless paths was passed to the constructor.
        // * They get invalidated by calling drop_paths or new renew() on the printer.
        const char *job_path() const;
        const char *job_lfn() const;
        // Type of filament loaded. Constant (in-code) strings.
        const char *material = nullptr;
        uint16_t flow_factor = 0;
        uint16_t job_id = 0;
        uint16_t print_fan_rpm = 0;
        uint16_t heatbreak_fan_rpm = 0;
        uint16_t print_speed = 0;
        uint32_t print_duration = 0;
        uint32_t time_to_end = 0;
        uint8_t progress_percent = 0;
        bool has_usb = false;
        uint64_t usb_space_free = 0;
        PrinterVersion version;
        printer_state::DeviceState state = printer_state::DeviceState::Unknown;

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
    virtual void renew(std::optional<SharedBuffer::Borrow> paths) = 0;
    virtual void drop_paths() = 0;
    const PrinterInfo &printer_info() const {
        return info;
    }

    virtual Params params() const = 0;
    virtual std::optional<NetInfo> net_info(Iface iface) const = 0;
    virtual NetCreds net_creds() const = 0;
    virtual bool job_control(JobControl) = 0;
    virtual bool start_print(const char *path) = 0;
    // Deletes a file.
    //
    // returns nullptr on success, message with reason of failure otherwise
    virtual const char *delete_file(const char *path) = 0;
    // Enqueues a gcode command (single one).
    //
    // FIXME: For now, this is a "black hole". It'll just submit it without any
    // kind of feedback. It'll either block if the queue is full, or just throw
    // it in. But that doesn't meen it has been executed.
    virtual void submit_gcode(const char *gcode) = 0;
    virtual bool set_ready(bool ready) = 0;
    virtual bool is_printing() const = 0;
    virtual bool is_idle() const = 0;
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

} // namespace connect_client
