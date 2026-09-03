#pragma once

#include "gcode_reader_interface.hpp"

class ClosedReader final : public IGcodeReader {
    bool stream_metadata_start(const Index * = nullptr) override {
        return false;
    }
    Result_t stream_gcode_start(uint32_t = 0, bool = false, const Index * = nullptr) override {
        return Result_t::RESULT_ERROR;
    }
    AbstractByteReader *stream_thumbnail_start(uint16_t, uint16_t, ImgType, bool = false) override {
        return nullptr;
    }
    Result_t stream_get_line(GcodeBuffer &, Continuations) override {
        return Result_t::RESULT_ERROR;
    }
    uint32_t get_gcode_stream_size_estimate() override {
        return 0;
    }
    uint32_t get_gcode_stream_size() override {
        return 0;
    }
    StreamRestoreInfo get_restore_info() override {
        return {};
    }
    void set_restore_info(const StreamRestoreInfo &) override {
    }
    Result_t stream_getc(char &) override {
        return Result_t::RESULT_ERROR;
    }
    bool valid_for_print([[maybe_unused]] bool full_check) override {
        return false;
    }
    void update_validity(const char *) override {
    }
    bool fully_valid() const override {
        return false;
    }
    bool handle_sees_whole_file() const override {
        return true;
    }
    bool has_error() const override {
        return true;
    }
    const char *error_str() const override {
        return "FIle is closed";
    }
#if HAS_E2EE_SUPPORT()
    bool has_identity_info() const override {
        return false;
    }

    e2ee::IdentityInfo get_identity_info() const override {
        return {};
    }

#endif
};
