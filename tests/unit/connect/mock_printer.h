#pragma once

#include <connect/printer.hpp>
#include <common/general_response.hpp>

#include <cstring>
#include <optional>
#include <vector>
#include <string>

namespace connect_client {

inline Printer::Params params_idle() {
    Printer::Params params(std::nullopt);

    params.job_id = 13;
    params.state = printer_state::DeviceState::Idle;
    params.nozzle_diameter = 0.4;
    params.version = { 2, 3, 0 };
    params.slot_mask = 1;

    return params;
}

constexpr Response yes_no[] = { Response::Yes, Response::No, Response::_none, Response::_none };

inline Printer::Params params_dialog() {
    Printer::Params params(std::nullopt);

    params.job_id = 13;
    params.state = printer_state::StateWithDialog(printer_state::DeviceState::Attention, ErrCode::ERR_UNDEF, 42, yes_no);
    params.nozzle_diameter = 0.4;
    params.version = { 2, 3, 0 };
    params.slot_mask = 1;

    return params;
}

class MockPrinter : public Printer {
private:
    const Params &p;

public:
    MockPrinter(const Params &params)
        : p(params) {
        info.appendix = false;
        strcpy(info.fingerprint, "DEADBEEF");
        info.firmware_version = "TST-1234";
        strcpy(info.serial_number.begin(), "FAKE-1234");
    }

    std::vector<std::string> submitted_gcodes;

    virtual void renew(std::optional<SharedBuffer::Borrow> borrow) override {}
    virtual void drop_paths() override {}
    virtual Config load_config() override {
        return Config();
    }

    virtual Params params() const override {
        return p;
    }

    virtual std::optional<NetInfo> net_info(Iface iface) const override {
        return std::nullopt;
    }

    virtual NetCreds net_creds() const override {
        return {};
    }

    virtual bool job_control(JobControl) override {
        return false;
    }

    virtual bool start_print(const char *) override {
        return false;
    }

    virtual const char *delete_file(const char *) override {
        return nullptr;
    }

    virtual GcodeResult submit_gcode(const char *gcode) override {
        submitted_gcodes.push_back(gcode);
        return GcodeResult::Submitted;
    }

    virtual bool set_ready(bool) override {
        return true;
    }

    virtual bool is_printing() const override {
        return false;
    }

    virtual bool is_idle() const override {
        return false;
    }

    virtual void init_connect(const char *) override {}

    virtual uint32_t cancelable_fingerprint() const override {
        return 0;
    }

    virtual bool is_in_error() const override {
        return false;
    }

    void reset_printer() override {
        abort();
    }

    virtual const char *dialog_action(uint32_t dialog_id, Response response) {
        return nullptr;
    }
};

} // namespace connect_client
