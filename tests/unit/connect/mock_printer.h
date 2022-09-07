#pragma once

#include <connect/printer.hpp>

#include <cstring>
#include <optional>

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
};

}
