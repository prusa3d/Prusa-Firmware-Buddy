#pragma once

#include "printer.hpp"

#include <common/shared_buffer.hpp>
#include <marlin_client.hpp>

namespace connect_client {

class MarlinPrinter final : public Printer {
private:
    std::optional<BorrowPaths> borrow;

    // The ready flag that allows connect to send jobs to us.
    //
    // Having it as static here may look a bit weird, but:
    // * We need it accessible from GUI thread too.
    // * Ready is a concept only Connect uses, nobody else, so managing it kind
    //   of belongs to Connect.
    static std::atomic<bool> ready;

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
    virtual bool is_idle() const override;
    virtual void init_connect(char *token) override;

    static bool load_cfg_from_ini();

    // Is the printer marked as ready now?
    //
    // The expected use case is eg. showing an icon in the status bar, not
    // necessarily a reliable synchronization (this might have some delay
    // against the actual state)
    //
    // This is thread safe.
    static bool is_printer_ready();

    // Sets or unsets the ready state.
    //
    // Returns if set successfully.
    //
    // (eg. returns false only if ready = True and it can't be turned to it
    // because currently printing or busy).
    //
    // This is thread safe.
    static bool set_printer_ready(bool ready);
};

} // namespace connect_client
