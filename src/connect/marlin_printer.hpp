#pragma once

#include "printer.hpp"

#include <common/shared_buffer.hpp>
#include <marlin_client.hpp>

namespace connect_client {

class MarlinPrinter final : public Printer {
private:
    SharedBuffer &buffer;
    // The SET_PRINTER_READY & friends support. Eventually, this shall sync
    // with GUI and other places somehow. For now, only Connect-internal flag.
    bool ready = false;

protected:
    virtual Config load_config() override;

    char *job_path_ptr;
    char *job_lfn_ptr;

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
    virtual bool job_control(JobControl) override;
    virtual bool start_print(const char *path) override;
    virtual void submit_gcode(const char *code) override;
    virtual bool set_ready(bool ready) override;
    virtual bool is_printing() const override;
    virtual uint32_t files_hash() const override;

    virtual void notify_filechange(const char *filename) override;

    static bool load_cfg_from_ini();
};

}
