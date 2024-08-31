#pragma once

#include "printer.hpp"
#include <common/crash_dump/dump.hpp>

namespace connect_client {

/// A replacement printer for redscreen/bluescreen cases.
///
/// Does not do much, but doesn't access marlin or USB. It just reports an
/// error state.
class ErrorPrinter final : public Printer {
protected:
    virtual Config load_config() override;

private:
    char title[crash_dump::MSG_TITLE_MAX_LEN + 1];
    char text[crash_dump::MSG_MAX_LEN + 1];
    uint16_t error_code;
    uint32_t dialog_id;
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
    virtual bool set_idle() override { return true; }
    virtual bool is_printing() const override;
    virtual bool is_in_error() const override;
    virtual bool is_idle() const override;
    virtual void init_connect(const char *token) override;
    virtual uint32_t cancelable_fingerprint() const override;
#if ENABLED(CANCEL_OBJECTS)
    virtual void cancel_object(uint8_t) override;
    virtual void uncancel_object(uint8_t) override;
    virtual const char *get_cancel_object_name(char *buffer, size_t size, size_t index) const override;
#endif
    virtual void reset_printer() override;
    virtual const char *dialog_action(uint32_t dialog_id, Response response) override;
    virtual std::optional<FinishedJobResult> get_prior_job_result(uint16_t job_id) const override;
    virtual void set_slot_info(size_t, const SlotInfo &);

public:
    ErrorPrinter();
};

} // namespace connect_client
