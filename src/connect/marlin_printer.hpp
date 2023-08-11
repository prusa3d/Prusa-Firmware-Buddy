#pragma once

#include "printer.hpp"

#include <common/shared_buffer.hpp>
#include <marlin_client.hpp>

namespace connect_client {

class MarlinPrinter final : public Printer {
private:
    std::optional<BorrowPaths> borrow;
    // The SET_PRINTER_READY & friends support. Eventually, this shall sync
    // with GUI and other places somehow. For now, only Connect-internal flag.
    bool ready = false;

protected:
    virtual Config load_config() override;

public:
    MarlinPrinter();
    MarlinPrinter(const MarlinPrinter &other) = delete;
    MarlinPrinter(MarlinPrinter &&other) = delete;
    MarlinPrinter &operator=(const MarlinPrinter &other) = delete;
    MarlinPrinter &operator=(MarlinPrinter &&other) = delete;

    virtual void renew(std::optional<SharedBuffer::Borrow> paths) override;
    virtual void drop_paths() override;
    virtual Params params() const override;
    virtual std::optional<NetInfo> net_info(Iface iface) const override;
    virtual NetCreds net_creds() const override;
    virtual bool job_control(JobControl) override;
    virtual bool start_print(const char *path) override;
    // If the state of the printer is "Finished" and we are
    // trying to delete the file, that just got printed,
    // this first exits the print and then deletes the file.
    virtual const char *delete_file(const char *path) override;
    virtual void submit_gcode(const char *code) override;
    virtual bool set_ready(bool ready) override;
    virtual bool is_printing() const override;
    virtual void init_connect(char *token) override;

    static bool load_cfg_from_ini();
};

} // namespace connect_client
