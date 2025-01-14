#pragma once

#include "base64_stream_decoder.h"
#include "gcode_reader_interface.hpp"

/**
 * @brief Implementation of IGcodeReader for plaintext gcodes
 */
class PlainGcodeReader final : public GcodeReaderCommon {
public:
    PlainGcodeReader(FILE &f, const struct stat &stat_info);
    PlainGcodeReader(PlainGcodeReader &&other) = default;
    PlainGcodeReader &operator=(PlainGcodeReader &&other) = default;

    virtual bool stream_metadata_start() override;
    virtual Result_t stream_gcode_start(uint32_t offset = 0) override;
    virtual bool stream_thumbnail_start(uint16_t expected_width, uint16_t expected_height, ImgType expected_type, bool allow_larger = false) override;
    virtual Result_t stream_get_line(GcodeBuffer &buffer, Continuations) override;
    virtual uint32_t get_gcode_stream_size_estimate() override;
    virtual uint32_t get_gcode_stream_size() override;
    virtual FileVerificationResult verify_file(FileVerificationLevel level, std::span<uint8_t> crc_calc_buffer) const override;
    virtual bool valid_for_print() override;
    virtual StreamRestoreInfo get_restore_info() override { return {}; }
    virtual void set_restore_info(const StreamRestoreInfo &) override {}

private:
    // Size of header that have to be valid before we start printing
    // One big file was observed to have header with size of 740 kB, so this adds some headroom
    static constexpr const size_t header_metadata_size = 1024 * 1024;

    // when reading metadata and we encounter this number of gcodes, skip to end of file to search further
    static constexpr const uint32_t stop_metadata_after_gcodes_num = 1;

    // Search this many last bytes for "metadata" comments.
    // With increasing size of the comment section, this will have to be increased as well
    static constexpr const size_t search_last_x_bytes = 50000;

    uint32_t gcodes_in_metadata = 0;
    uint32_t thumbnail_size = 0;
    Base64StreamDecoder base64_decoder;
    uint32_t file_size = 0;
    // when scanning for metadata, this will be set to position of first gcode, so subsequent calls to stream_gcode_start will start directly from gcodes
    uint32_t first_gcode_pos;

    Result_t stream_getc_impl(char &out);
    Result_t stream_getc_thumbnail_impl(char &out);

    /**
     * @brief Check if the line is thumbnail begin.
     * @param expected_width expected image width
     * @param expected_height expected image height
     * @param expected_type expected image format
     * @param allow_larger allow larger images
     * @param num_bytes number of bytes of thumbnail
     * @return true if the line is thumbnail begin
     */
    bool IsBeginThumbnail(GcodeBuffer &buffer, uint16_t expected_width, uint16_t expected_height, ImgType expected_type, bool allow_larger, unsigned long &num_bytes) const;

    void set_ptr_stream_getc(IGcodeReader::Result_t (PlainGcodeReader::*ptr_stream_getc)(char &out)) {
        // this converts PlainGcodeReader::some_getc_function to IGcodeReader::some_function,
        // note that this conversion is only possible if PlainGcodeReader is subclass of IGcodeReader, and class doesn't have multiple parents
        this->ptr_stream_getc = static_cast<stream_getc_type>(ptr_stream_getc);
    }
};
