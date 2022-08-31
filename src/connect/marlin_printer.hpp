#pragma once

#include "buffer.hpp"
#include "printer.hpp"

#include <marlin_client.h>

namespace connect {

class MarlinPrinter final : public Printer {
private:
    marlin_vars_t *marlin_vars;
    SharedBuffer &buffer;

protected:
    virtual Config load_config() override;

public:
    MarlinPrinter(SharedBuffer &buffer);
    MarlinPrinter(const MarlinPrinter &other) = delete;
    MarlinPrinter(MarlinPrinter &&other) = delete;
    MarlinPrinter &operator=(const MarlinPrinter &other) = delete;
    MarlinPrinter &operator=(MarlinPrinter &&other) = delete;

    virtual void renew() override;
    virtual Params params() const override;
    virtual std::optional<NetInfo> net_info(Iface iface) const override;
    virtual NetCreds net_creds() const override;

    static bool load_cfg_from_ini();
};

}
