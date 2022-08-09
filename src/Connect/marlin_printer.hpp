#pragma once

#include "printer.hpp"

#include <marlin_client.h>

namespace con {

class MarlinPriter final : public Printer {
private:
    marlin_vars_t *marlin_vars;

protected:
    virtual Config load_config() override;

public:
    MarlinPriter();
    MarlinPriter(const MarlinPriter &other) = delete;
    MarlinPriter(MarlinPriter &&other) = delete;
    MarlinPriter &operator=(const MarlinPriter &other) = delete;
    MarlinPriter &operator=(MarlinPriter &&other) = delete;

    virtual void renew() override;
    virtual Params params() const override;

    static bool load_cfg_from_ini();
};

}
