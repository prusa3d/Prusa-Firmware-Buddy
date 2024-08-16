#pragma once

#include "printer.hpp"

#include <common/shared_buffer.hpp>
#include <marlin_client.hpp>
#include <marlin_vars.hpp>

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

#if ENABLED(CANCEL_OBJECTS)
    // We don't want to include marlin_vars into the printer or render.cpp directly
    //, because we want to shield unit tests from marlin stuff, but we want to be
    // sure everything fits.
    static_assert(marlin_vars_t::CANCEL_OBJECT_NAME_LEN == Printer::CANCEL_OBJECT_NAME_LEN);
    static_assert(marlin_vars_t::CANCEL_OBJECTS_NAME_COUNT == Printer::CANCEL_OBJECT_NAME_COUNT);
#endif

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
    virtual const char *start_print(const char *path, const std::optional<ToolMapping> &tools_mapping) override;
    // If the state of the printer is "Finished" and we are
    // trying to delete the file, that just got printed,
    // this first exits the print and then deletes the file.
    virtual const char *delete_file(const char *path) override;
    virtual GcodeResult submit_gcode(const char *code) override;
    virtual bool set_ready(bool ready) override;
    virtual bool is_printing() const override;
    virtual bool is_in_error() const override;
    virtual bool is_idle() const override;
    virtual void init_connect(const char *token) override;
    virtual uint32_t cancelable_fingerprint() const override;
#if ENABLED(CANCEL_OBJECTS)
    virtual void cancel_object(uint8_t id) override;
    virtual void uncancel_object(uint8_t id) override;
    virtual const char *get_cancel_object_name(char *buffer, size_t size, size_t index) const override;
#endif
    virtual void reset_printer() override;
    virtual const char *dialog_action(uint32_t dialog_id, Response response) override;
    virtual std::optional<FinishedJobResult> get_prior_job_result(uint16_t job_id) const override;

    virtual void set_slot_info(size_t idx, const SlotInfo &info) override;

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
