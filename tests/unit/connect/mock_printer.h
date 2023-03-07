#pragma once

#include <connect/printer.hpp>

#include <cstring>
#include <optional>
#include <vector>
#include <string>

namespace connect_client {

constexpr Printer::Params params_idle() {
    Printer::Params params {};

    params.job_id = 13;
    params.state = Printer::DeviceState::Idle;

    return params;
}

class MockPrinter final : public Printer {
private:
    const Params &p;

public:
    MockPrinter(const Params &params)
        : p(params) {
        info.appendix = false;
        strcpy(info.fingerprint, "DEADBEEF");
        info.firmware_version = "TST-1234";
        strcpy(info.serial_number, "FAKE-1234");
    }

    std::vector<std::string> submitted_gcodes;

    virtual void renew() override {}
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

    virtual void submit_gcode(const char *gcode) override {
        submitted_gcodes.push_back(gcode);
    }

    virtual bool set_ready(bool) override {
        return true;
    }

    virtual bool is_printing() const override {
        return false;
    }

    virtual uint32_t files_hash() const override {
        return 0;
    }

    virtual void notify_filechange(const char *) override {}
    virtual void init_connect(char *) override {}
};

}
