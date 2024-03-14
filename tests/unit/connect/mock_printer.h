#pragma once

#include <connect/printer.hpp>

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

    virtual void submit_gcode(const char *gcode) override {
        submitted_gcodes.push_back(gcode);
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

    virtual void init_connect(char *) override {}
};

} // namespace connect_client
