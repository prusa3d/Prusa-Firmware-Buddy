#pragma once

#include "gcode_reader_interface.hpp"

class ClosedReader final : public IGcodeReader {
    bool stream_metadata_start() override {
        return false;
    }
    Result_t stream_gcode_start(uint32_t = 0) override {
        return Result_t::RESULT_ERROR;
    }
    bool stream_thumbnail_start(uint16_t, uint16_t, ImgType, bool = false) override {
        return false;
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
    FileVerificationResult verify_file(FileVerificationLevel, std::span<uint8_t> = std::span<uint8_t>()) const override {
        return { .is_ok = has_error(), .error_str = error_str() };
    }
    Result_t stream_getc(char &) override {
        return Result_t::RESULT_ERROR;
    }
    bool valid_for_print() override {
        return false;
    }
    void update_validity(const char *) override {
    }
    bool fully_valid() const override {
        return false;
    }
    bool has_error() const override {
        return true;
    }
    const char *error_str() const override {
        return "FIle is closed";
    }
};
