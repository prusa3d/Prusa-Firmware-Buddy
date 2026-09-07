#pragma once

#include "gcode_reader_interface.hpp"

#include <base64/base64.hpp>
#include <utils/byte_utils.hpp>

/**
 * @brief Implementation of IGcodeReader for plaintext gcodes
 */
class PlainGcodeReader final : public GcodeReaderCommon {
public:
    PlainGcodeReader(unique_file_ptr &&f, const struct stat &stat_info);
    PlainGcodeReader(PlainGcodeReader &&other) = default;
    PlainGcodeReader &operator=(PlainGcodeReader &&other) = default;

    virtual bool stream_metadata_start(const Index *index = nullptr) override;
    virtual Result_t stream_gcode_start(uint32_t offset = 0, bool ignore_crc = false, const Index *index = nullptr) override;
    virtual AbstractByteReader *stream_thumbnail_start(uint16_t expected_width, uint16_t expected_height, ImgType expected_type, bool allow_larger = false) override;
    virtual Result_t stream_get_line(GcodeBuffer &buffer, Continuations) override;
    virtual uint32_t get_gcode_stream_size_estimate() override;
    virtual uint32_t get_gcode_stream_size() override;
    virtual bool valid_for_print(bool) override;
    virtual StreamRestoreInfo get_restore_info() override { return {}; }
    virtual void set_restore_info(const StreamRestoreInfo &) override {}

    // Tries to parse the given comment (already stripped of the ; at the front) as a thumbnail start.
    //
    // If successful, provides the details.
    static std::optional<ThumbnailDetails> thumbnail_details(GcodeBuffer::String comment);

private:
    // Size of header that have to be valid before we start printing
    // One big file was observed to have header with size of 740 kB, so this adds some headroom
    static constexpr const size_t header_metadata_size = 1024 * 1024;

    // when reading metadata and we encounter this number of gcodes, skip to end of file to search further
    static constexpr const uint32_t stop_metadata_after_gcodes_num = 1;

    // Search this many last bytes for "metadata" comments.
    // With increasing size of the comment section, this will have to be increased as well
    static constexpr const size_t search_last_x_bytes = 75000;

    uint32_t gcodes_in_metadata = 0;
    uint32_t file_size = 0;
    // when scanning for metadata, this will be set to position of first gcode, so subsequent calls to stream_gcode_start will start directly from gcodes
    uint32_t first_gcode_pos;

    Result_t stream_getc_impl(char &out);

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

    struct ThumbnailReader final : public AbstractByteReader {
        PlainGcodeReader *gcode_reader = nullptr;
        uint32_t thumbnail_size = 0;
        base64::Base64Decoder base64_decoder;

        WritableBytes read(WritableBytes buffer) final;
        Result_t getc(char &out);
    };
    ThumbnailReader thumbnail_reader;
};
