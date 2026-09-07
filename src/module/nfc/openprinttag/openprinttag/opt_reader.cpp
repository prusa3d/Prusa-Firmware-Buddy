#include "opt_reader.hpp"
#include "ndef.hpp"
#include "nanocbor_ext.hpp"

#include <algorithm>
#include <utils/byte_utils.hpp>

using namespace openprinttag;

struct OPTReader::CBORValue {
    nanocbor_value_t *item;

    inline operator nanocbor_value_t *() {
        return item;
    }
};

struct OPTReader::CBOREncoder {
    nanocbor_encoder_t *encoder;

    inline operator nanocbor_encoder_t *() {
        return encoder;
    }
};

OPTReader::Error OPTReader::to_reader_error(OPTBackend::IOError error) {
    switch (error) {

    case OPTBackend::IOError::outside_of_bounds:
    case OPTBackend::IOError::tag_invalid:
        return Error::tag_invalid;

    case OPTBackend::IOError::not_implemented:
        return Error::not_implemented;

    case OPTBackend::IOError::data_too_big:
        return Error::data_too_big;

    case OPTBackend::IOError::other:
    case OPTBackend::IOError::invalid_id:
        return Error::other;

    case OPTBackend::IOError::radio_disabled:
        return Error::radio_disabled;
    }

    // Unreachable
    std::abort();
}

OPTReader::OPTReader(OPTBackend &reader)
    : backend_(reader) {}

bool OPTReader::get_event(Event &event, uint32_t current_time_ms) {
    if (!backend_.get_event(event, current_time_ms)) {
        return false;
    }

    std::visit([this]<typename E>(const E &e) {
        if constexpr (std::is_same_v<E, OPTBackend::TagDetectedEvent>) {

        } else if constexpr (std::is_same_v<E, OPTBackend::TagLostEvent>) {
            invalidate_cache(e.tag);

        } else {
            static_assert(false);
        }
    },
        event);

    return true;
}

void OPTReader::invalidate_cache(TagID tag) {
    metadata_cache_.invalidate(tag);

    // Invalidate read buffer if it was storing the tracked tag
    if (read_buffer_cache_.tag == tag) {
        read_buffer_cache_ = {};
    }
}

void OPTReader::forget_tag(TagID tag) {
    invalidate_cache(tag);
    backend_.forget_tag(tag);
}

void OPTReader::reset_state() {
    metadata_cache_.clear();
    read_buffer_cache_ = {};
    backend_.reset_state();
}

void OPTReader::set_params(const Params &set) {
    params_ = set;
    metadata_cache_.clear();
    read_buffer_cache_ = {};
}

OPTReader::IOResult<const OPTReader::TagMetadata *> OPTReader::read_metadata(TagID tag) {
    bool found = false;
    TagMetadata &data = metadata_cache_.get(tag, &found);
    if (found) {
        if (!data.is_valid) {
            return std::unexpected(Error::tag_invalid);
        }

        return &data;
    }

    constexpr auto tag_invalid_error = std::unexpected(Error::tag_invalid);

    const auto handle_error = [&](Error err) -> IOResult<const TagMetadata *> {
        switch (err) {

        case Error::tag_invalid:
        case Error::region_corrupt:
        case Error::field_not_present:
        case Error::data_too_big:
        case Error::wrong_field_type:
        case Error::write_protected:
            // If we got to handling these errors, this means that something's wrong with either the chip or the meta section.
            // Retrying won't help, the chip needs to be reformatted.
            // data.is_valid is false by default, so the chip is marked as invalid in the cache
            return tag_invalid_error;

        case Error::radio_disabled:
            return std::unexpected(Error::radio_disabled);

        case Error::other:
            // We failed reading something. Invalidate the metadata, so that it is re-read later if requested.
            metadata_cache_.invalidate(tag);
            return std::unexpected(err);

        case Error::not_implemented:
            // This should not happen for elementary functions
            // Fallback to unreachable
            break;

        case Error::_cnt:
            // Fallback to unreachable
            break;
        }

        // Unreachable
        std::abort();
    };

    // Read tag info and determine TLV region
    OPTBackend::TagInfo tag_info;
    if (auto r = backend_.read_tag_info(tag, tag_info); !r) {
        return handle_error(to_reader_error(r.error()));
    }

    // Chew through the TLV records
    PayloadSpan ndef_message_span;
    for (PayloadPos tlv_pos = tag_info.tlv_span.offset;;) {
        static constexpr auto tlv_data_size = 4;

        if (tlv_pos + tlv_data_size > tag_info.tlv_span.end()) {
            // We've either run out of the chip memory or the remaining TLVs are smaller han tlv_data_size (terminator is single byte for example)
            // Either way, our NDEF record is definitely not there
            return tag_invalid_error;
        }

        // Read the TLV record
        const auto io_result = read_span({ .tag = tag, .span = { .offset = tlv_pos, .size = tlv_data_size } });
        if (!io_result) {
            return handle_error(io_result.error());
        }
        const auto &tlv_data = *io_result;

        const NDEFTLVTag tag = static_cast<NDEFTLVTag>(tlv_data[0]);
        if (tag == NDEFTLVTag::terminator) {
            // Nothing is after the terminator tag -> we've not found a NDEF TLV record
            return tag_invalid_error;
        }

        // Decode TLV length & skip the header
        PayloadPos value_length;
        if (tlv_data[1] == std::byte(0xFF)) {
            value_length = (static_cast<PayloadPos>(tlv_data[2]) << 8) | static_cast<PayloadPos>(tlv_data[3]);
            tlv_pos += 4;
        } else {
            value_length = static_cast<PayloadPos>(tlv_data[1]);
            tlv_pos += 2;
        }

        if (tag != NDEFTLVTag::ndef) {
            // This is not what we're looking for, skip this TLV
            tlv_pos += value_length;
            continue;
        }

        // We've found the NDEF TLV -> break this loop
        ndef_message_span = { .offset = tlv_pos, .size = value_length };
        break;
    }

    // Chew through the NDEF header
    PayloadSpan payload_span;
    for (PayloadPos next_record_offset = ndef_message_span.offset;;) {
        const auto record_offset = next_record_offset;

        if (record_offset == invalid_nfc_offset) {
            // We hit the end -> haven't found our tag
            return tag_invalid_error;
        }

        // Copy the header so that we can reuse the read buffer (and also for alignment)
        NDEFRecordFullHeader ndef_header;
        const PayloadSpan header_span { .offset = record_offset, .size = sizeof(NDEFRecordFullHeader) };
        if (header_span.end() > ndef_message_span.end()) {
            // We'd get outside of the NDEF TLV, the tag is either malformed or the last message is smaller than NDEFRecordFullHeader
            // Messages so small couldn't be what we are looking for
            return tag_invalid_error;
        }

        {
            auto io_result = read_span({ .tag = tag, .span = header_span });
            if (!io_result) {
                return handle_error(io_result.error());
            }
            memcpy(&ndef_header, io_result->data(), sizeof(ndef_header));
        }

        if (record_offset == ndef_message_span.offset && !ndef_header.message_begin) {
            // Not a message begin -> invalid data
            return tag_invalid_error;
        }

        // Calculate offset of the next NDEF record
        next_record_offset = ndef_header.message_end ? invalid_nfc_offset : (record_offset + ndef_header.record_length());

        if (
            ndef_header.type_name_format != NDEFTypeNameFormat::mime_media_type
            || ndef_header.type_length != params_.mime_type.length() //
        ) {
            continue;
        }

        // Check NDEF MIME type
        {
            // Read the NDEF type into the read buffer
            auto io_result = read_span({ .tag = tag, .span = ndef_header.dynamic_field_span(NDEFRecordFullHeader::DynamicField::type).added_offset(record_offset) });
            if (!io_result) {
                return handle_error(io_result.error());
            }

            // Different mime type -> the tag is invalid
            const std::string_view tag_mime_type { reinterpret_cast<const char *>(io_result->data()), io_result->size() };
            if (tag_mime_type != params_.mime_type) {
                continue;
            }
        }

        if (ndef_header.chunk_flag) {
            // OpenPrintTag payloads are not allowed to be chunked
            return tag_invalid_error;
        }

        // Parse and store the payload location
        payload_span = ndef_header.dynamic_field_span(NDEFRecordFullHeader::DynamicField::payload).added_offset(record_offset);

        // Found the OpenPrintTag NDEF record
        break;
    }

    // No meta region -> mark the whole payload as main region and call it a day
    if (!params_.has_meta_region) {
        data.region[Region::main].span = payload_span;
        data.is_valid = true;
        return &data;
    }

    // Read and parse the meta region
    PayloadPos meta_section_size;
    {
        if (payload_span.size < max_meta_region_size) {
            return tag_invalid_error;
        }

        const auto enum_callback = [&data](Field field, CBORValue v) -> EnumerateCallbackResult {
            switch (static_cast<MetaField>(field)) {

            case MetaField::main_region_offset:
                return nanocbor_get_uint16(v, &data.region[Region::main].span.offset);

            case MetaField::main_region_size:
                return nanocbor_get_uint16(v, &data.region[Region::main].span.size);

            case MetaField::aux_region_offset:
                return nanocbor_get_uint16(v, &data.region[Region::auxiliary].span.offset);

            case MetaField::aux_region_size:
                return nanocbor_get_uint16(v, &data.region[Region::auxiliary].span.size);

            default:
                return nanocbor_skip(v);
            }
        };

        auto io_result = enumerate_fields_callback({ .tag = tag, .span = { .offset = payload_span.offset, .size = max_meta_region_size } }, enum_callback);
        if (!io_result) {
            return handle_error(io_result.error());
        }

        // Determine meta region
        meta_section_size = *io_result;
    }

    // Deduce unspecified fields
    {
        auto &main_span = data.region[Region::main].span;
        auto &aux_span = data.region[Region::auxiliary].span;
        auto &meta_span = data.region[Region::meta].span;

        // Meta region non-writable, always starts at 0 and is always assumed to take only the size of the data
        meta_span = { .offset = 0, .size = meta_section_size };

        if (main_span.offset == 0) {
            // If main region span is not specified, that means that it starts immediately after the meta section
            main_span.offset = meta_span.end();

        } else {
            // Otherwise expand the meta section till the start of the other section
            meta_span.size = main_span.offset;

            // Aux region could be before the main region - accomodate for that
            if (aux_span.offset != 0) {
                meta_span.size = std::min(meta_span.size, aux_span.offset);
            }
        }

        // Deduce size of the main region (if not explicitly specified)
        if (main_span.size == 0) {
            if (aux_span.offset != 0 && aux_span.offset > main_span.offset) {
                // If the aux region is present and is after the main region,
                // span the main region till the begining of the aux region.
                main_span.size = aux_span.offset - main_span.offset;

            } else {
                // Otherwise span main region till the end of the payload
                main_span.size = payload_span.size - main_span.offset;
            }
        }

        // Deduce size of the aux region (if present and not explicitly specified)
        if (aux_span.offset != 0 && aux_span.size == 0) {
            if (aux_span.offset < main_span.offset) {
                // If the aux region is before the main region, span the aux region till the main region
                aux_span.size = main_span.offset - aux_span.offset;

            } else {
                // Otherwise span the aux region till the end of the payload
                aux_span.size = payload_span.size - aux_span.offset;
            }
        }

        // Misconfiguration - aux region size is set, but offset not.
        if (aux_span.size > 0 && aux_span.offset == 0) {
            return tag_invalid_error;
        }
    }

    // Transform payload-relative region spans to tag-relative
    for (auto &r : data.region) {
        if (!r.is_present()) {
            continue;
        }

        r.span.offset += payload_span.offset;

        // Check that someone hasn't misconfigured the chip, trying to do out-of-bounds attack
        if (!payload_span.contains(r.span)) {
            return std::unexpected(Error::tag_invalid);
        }
    }

    data.is_valid = true;
    return &data;
}

OPTReader::IOResult<std::span<const Field>> OPTReader::enumerate_fields(TagID tag, Section section, const std::span<Field> &result) {
    size_t cnt = 0;
    const auto enum_callback = [&](Field field, CBORValue v) -> EnumerateCallbackResult {
        if (cnt == result.size()) {
            return Error::data_too_big;
        }

        result[cnt++] = field;

        // Skip reading value
        return nanocbor_skip(v);
    };

    const auto r = enumerate_fields_callback(tag, section, enum_callback);
    if (!r) {
        return std::unexpected(r.error());
    }

    return std::span(result.data(), cnt);
}

OPTReader::IOResult<OPTReader::WriteReport> OPTReader::remove_field(const TagField &field) {
    return write_field_impl(field, nullptr);
}

template <typename T, auto f>
inline OPTReader::IOResult<T> OPTReader::read_field_primitive(const TagField &field) {
    T result;
    const auto r = read_field_impl(field, [&result](CBORValue v) {
        return f(v, &result);
    });
    if (!r) {
        return std::unexpected(r.error());
    }
    return result;
}

OPTReader::IOResult<bool> OPTReader::read_field_bool(const TagField &field) {
    return read_field_primitive<bool, nanocbor_get_bool>(field);
}

OPTReader::IOResult<int32_t> OPTReader::read_field_int32(const TagField &field) {
    return read_field_primitive<int32_t, nanocbor_get_int32>(field);
}

OPTReader::IOResult<int64_t> OPTReader::read_field_int64(const TagField &field) {
    return read_field_primitive<int64_t, nanocbor_get_int64>(field);
}

OPTReader::IOResult<float> OPTReader::read_field_float(const TagField &field) {
    float result;
    const auto r = read_field_impl(field, [&result](CBORValue v) -> ReadFieldCallbackResult {
        const auto type = nanocbor_get_type(v);
        if (type < 0) {
            return type;
        }
        switch (type) {

        case NANOCBOR_TYPE_NINT: {
            int64_t t_result;
            auto r = nanocbor_get_int64(v, &t_result);
            result = t_result;
            return r;
        }

        case NANOCBOR_TYPE_UINT: {
            uint64_t t_result;
            auto r = nanocbor_get_uint64(v, &t_result);
            result = t_result;
            return r;
        }

        case NANOCBOR_TYPE_FLOAT:
            return nanocbor_get_float(v, &result);

        default:
            return Error::wrong_field_type;
        }
    });
    if (!r) {
        return std::unexpected(r.error());
    }
    return result;
}

OPTReader::IOResult<std::string_view> OPTReader::read_field_string(const TagField &field, const std::span<char> &buffer) {
    std::string_view result;
    const auto r = read_field_impl(field, [&result, &buffer](CBORValue v) -> ReadFieldCallbackResult {
        const uint8_t *data;
        size_t len;
        const auto r2 = nanocbor_get_tstr(v, &data, &len);
        if (r2 < 0) {
            return r2;
        }
        if (len > buffer.size()) {
            return Error::data_too_big;
        }
        memcpy(buffer.data(), data, len);
        result = std::string_view(buffer.data(), len);
        return r2;
    });
    if (!r) {
        return std::unexpected(r.error());
    }
    return result;
}

OPTReader::IOResult<std::basic_string_view<std::byte>> OPTReader::read_field_bytes(const TagField &field, const WritableBytes &buffer) {
    std::basic_string_view<std::byte> result;
    const auto r = read_field_impl(field, [&result, &buffer](CBORValue v) -> ReadFieldCallbackResult {
        const uint8_t *data;
        size_t len;
        const auto r2 = nanocbor_get_bstr(v, &data, &len);
        if (r2 < 0) {
            return r2;
        }
        if (len > buffer.size()) {
            return Error::data_too_big;
        }
        memcpy(buffer.data(), data, len);
        result = std::basic_string_view<std::byte>(buffer.data(), len);
        return r2;
    });
    if (!r) {
        return std::unexpected(r.error());
    }
    return result;
}

OPTReader::IOResult<std::span<const uint16_t>> OPTReader::read_field_uint16_array(const TagField &field, const std::span<uint16_t> &buffer) {
    size_t count = 0;
    const auto r = read_field_impl(field, [&count, &buffer](CBORValue v) -> ReadFieldCallbackResult {
        nanocbor_value_t item;
        if (auto r = nanocbor_enter_array(v, &item); r < 0) {
            return r;
        }

        while (!nanocbor_at_end(&item)) {
            if (count == buffer.size()) {
                return Error::data_too_big;
            }

            uint16_t val;
            if (auto r = nanocbor_get_uint16(&item, &val); r < 0) {
                return r;
            }

            buffer[count++] = val;
        }

        nanocbor_leave_container(v, &item);
        return 0;
    });
    if (!r) {
        return std::unexpected(r.error());
    }
    return std::span(buffer.data(), count);
}

OPTReader::IOResult<OPTReader::WriteReport> OPTReader::write_field_bool(const TagField &field, bool value) {
    return write_field_impl(field, [value](CBOREncoder e) {
        return nanocbor_fmt_bool(e.encoder, value);
    });
}

OPTReader::IOResult<OPTReader::WriteReport> OPTReader::write_field_int32(const TagField &field, int32_t value) {
    return write_field_impl(field, [value](CBOREncoder e) {
        return nanocbor_fmt_int(e.encoder, value);
    });
}

OPTReader::IOResult<OPTReader::WriteReport> OPTReader::write_field_int64(const TagField &field, int64_t value) {
    return write_field_impl(field, [value](CBOREncoder e) {
        return nanocbor_fmt_int(e.encoder, value);
    });
}

OPTReader::IOResult<OPTReader::WriteReport> OPTReader::write_field_float(const TagField &field, float value) {
    return write_field_impl(field, [value](CBOREncoder e) {
        if (int64_t(value) == value) {
            return nanocbor_fmt_int(e.encoder, int64_t(value));
        } else {
            return nanocbor_fmt_float(e.encoder, value);
        }
    });
}

OPTReader::IOResult<OPTReader::WriteReport> OPTReader::write_field_string(const TagField &field, const std::string_view &value) {
    return write_field_impl(field, [&value](CBOREncoder e) {
        return nanocbor_put_tstrn(e.encoder, value.data(), value.size());
    });
}

OPTReader::IOResult<OPTReader::WriteReport> OPTReader::write_field_bytes(const TagField &field, const std::basic_string_view<std::byte> &value) {
    return write_field_impl(field, [&value](CBOREncoder e) {
        return nanocbor_put_bstr(e.encoder, reinterpret_cast<const uint8_t *>(value.data()), value.size());
    });
}

OPTReader::IOResult<OPTReader::WriteReport> OPTReader::write_field_uint16_array(const TagField &field, const std::span<const uint16_t> &value) {
    return write_field_impl(field, [&value](CBOREncoder e) {
        if (auto r = nanocbor_fmt_array_indefinite(e); r < 0) {
            return r;
        }

        for (auto item : value) {
            if (auto r = nanocbor_fmt_uint(e, item); r < 0) {
                return r;
            }
        }

        return nanocbor_fmt_end_indefinite(e);
    });
}

OPTReader::IOResult<WritableBytes> OPTReader::read_span(const TagPayloadSpan &span) {
    // The region is already in the read buffer -> we don't need to issue another read
    if (read_buffer_cache_.tag == span.tag && read_buffer_cache_.span.contains(span.span)) {
        return std::span(read_buffer_.begin() + span.span.offset - read_buffer_cache_.span.offset, span.span.size);
    }

    if (span.span.size > read_buffer_.size()) {
        return std::unexpected(Error::data_too_big);
    }
    const auto result = std::span(read_buffer_.begin(), span.span.size);

    if (auto r = backend_.read(span.tag, span.span.offset, result); !r) {
        return std::unexpected(to_reader_error(r.error()));
    }

    read_buffer_cache_ = {
        .tag = span.tag,
        .span = span.span,
    };
    return result;
}

OPTReader::IOResult<PayloadPos> OPTReader::enumerate_fields_callback(TagID tag, Section section, const EnumerateCallback &callback) {
    const auto metadata_r = read_metadata(tag);
    if (!metadata_r) {
        return std::unexpected(metadata_r.error());
    }

    const TagMetadata &metadata = **metadata_r;
    const auto &region_meta = metadata.region[section];
    if (!region_meta.is_present()) {
        return std::unexpected(Error::field_not_present);
    }

    return enumerate_fields_callback({ .tag = tag, .span = region_meta.span }, callback);
}

OPTReader::IOResult<PayloadPos> OPTReader::enumerate_fields_callback(const TagPayloadSpan &span, const EnumerateCallback &callback) {
    // Read the region into the read buffer (this will do nothing if the region is already in there)
    auto io_result = read_span(span);
    if (!io_result) {
        return std::unexpected(io_result.error());
    }

    nanocbor_value_t root;
    nanocbor_value_t item;
    const auto data_start = reinterpret_cast<uint8_t *>(io_result->data());
    nanocbor_decoder_init(&root, data_start, io_result->size());

    if (nanocbor_enter_map(&root, &item) < 0) {
        return std::unexpected(Error::region_corrupt);
    }

    while (!nanocbor_at_end(&item)) {
        Field field;
        if (nanocbor_get_uint16(&item, &field) < 0) {
            // The specification says that we should skip keys of unsupported types -> make this as robust as possible
            // Attempt to skip both key and value
            if (nanocbor_skip(&item) < 0 || nanocbor_skip(&item) < 0) {
                return std::unexpected(Error::region_corrupt);
            }
            continue;
        }

        const EnumerateCallbackResult callback_r = callback(field, { &item });
        if (std::holds_alternative<StopEnumerating>(callback_r)) {
            // Cannot compute region object size -> return 0
            return 0;

        } else if (auto err = std::get_if<Error>(&callback_r)) {
            return std::unexpected(*err);

        } else if (auto r = std::get_if<int>(&callback_r); r && *r < 0) {
            return std::unexpected(Error::region_corrupt);

        } else {
            // ContinueIterating or nanocbor function result was valid -> continue
        }
        static_assert(std::variant_size_v<EnumerateCallbackResult> == 4);
    }

    nanocbor_leave_container(&root, &item);

    return static_cast<PayloadPos>(root.cur - data_start);
}

OPTReader::IOResult<void> OPTReader::read_field_impl(const TagField &field, const ReadFieldCallback &callback) {
    // Put data in the struct to reduce context size of inplace_function
    struct {
        Field field;
        const ReadFieldCallback &callback;
        bool found = false;

    } ctx {
        .field = field.field,
        .callback = callback,
    };

    const auto enum_callback = [&ctx](Field field, CBORValue v) -> EnumerateCallbackResult {
        if (field != ctx.field) {
            return nanocbor_skip(v);
        }

        const auto r_variant = ctx.callback(v);
        if (auto err = std::get_if<Error>(&r_variant)) {
            return *err;
        }

        auto r = std::get<int>(r_variant);
        if (r >= 0) {
            // We've found our value, we can stop enumerating
            ctx.found = true;
            return StopEnumerating();
        }
        switch (r) {

        case NANOCBOR_ERR_INVALID_TYPE:
            // By default, ERR_INVALID_TYPE gets still interpreted as a corrupt region.
            // Only here, when we're really reading something we want to read, we return wrong field type
            return Error::wrong_field_type;

        case NANOCBOR_ERR_OVERFLOW:
            return Error::data_too_big;

        default:
            return r;
        }
    };

    const auto r = enumerate_fields_callback(field.tag, field.section, enum_callback);
    if (!r) {
        return std::unexpected(r.error());

    } else if (!ctx.found) {
        return std::unexpected(Error::field_not_present);

    } else {
        // Success, the callback function is responsible for storing the result, return void
        return {};
    }
}

OPTReader::IOResult<OPTReader::WriteReport> OPTReader::write_field_impl(const TagField &field, const WriteFieldCallback &callback) {
    // Do not allow writing to the meta region. The reader is not ready for it, would screw things up.
    if (field.section == Section::meta) {
        return std::unexpected(Error::write_protected);
    }

    const auto metadata_r = read_metadata(field.tag);
    if (!metadata_r) {
        return std::unexpected(metadata_r.error());
    }

    const TagMetadata &metadata = **metadata_r;
    const auto &region_meta = metadata.region[field.section];
    if (!region_meta.is_present()) {
        return std::unexpected(Error::field_not_present);
    }

    // Read the region into the read buffer (this will do nothing if the region is already in there)
    auto read_result = read_span({ .tag = field.tag, .span = region_meta.span });
    if (!read_result) {
        return std::unexpected(read_result.error());
    }

    // Prepare the write data to write buffer
    WriteReport result {
        .field_existed = false,
        .changed = false,
    };
    const auto prepare_data = [&] {
        nanocbor_value_t decoder_root;
        nanocbor_value_t decoder;
        const auto data_start = reinterpret_cast<uint8_t *>(read_result->data());
        nanocbor_decoder_init(&decoder_root, data_start, read_result->size());

        nanocbor_encoder_t encoder;
        nanocbor_encoder_init(&encoder, reinterpret_cast<uint8_t *>(write_buffer_.data()), region_meta.span.size);

        // Map start
        if (auto r = nanocbor_enter_map(&decoder_root, &decoder); r < 0) {
            return r;
        }
        if (auto r = nanocbor_fmt_map_indefinite(&encoder); r < 0) {
            return r;
        }

        while (!nanocbor_at_end(&decoder)) {
            Field key = -1;
            if (nanocbor_get_uint16(&decoder, &key) < 0) {
                // We failed reading the uint16 key, but that might mean that the key is just of a different type
                // The specification says that we should leave things that we don't understand as-is, so we try to copy it to the output anyway

                // Copy the unknown key - because nanocbor_get_uint16 failed, it didn't advance
                if (auto r = nanocbor_copy_value(&decoder, &encoder); r < 0) {
                    return r;
                }

                // And copy the value
                if (auto r = nanocbor_copy_value(&decoder, &encoder); r < 0) {
                    return r;
                }

                continue;
            }

            // This is a different field -> just copy the value and continue
            if (key != field.field) {
                if (auto r = nanocbor_fmt_uint(&encoder, key); r < 0) {
                    return r;
                }

                if (auto r = nanocbor_copy_value(&decoder, &encoder); r < 0) {
                    return r;
                }

                continue;
            }

            // Skip the original alue
            if (auto r = nanocbor_skip(&decoder); r < 0) {
                return r;
            }

            // If we have already found the field previously, that means that there is a double entry. Do not store the double entry.
            if (result.field_existed) {
                continue;
            }

            result.field_existed = true;

            // If callback is null, that means that we want to actually remove the entry.
            // This means that we will not copy the key to the output, but instead put together the output without our item.
            if (!callback) {
                continue;
            }

            // Write the key
            if (auto r = nanocbor_fmt_uint(&encoder, key); r < 0) {
                return r;
            }

            // We've found our field -> instead of writing the original value, write our new one by calling callback()
            // Keep the item on the same position in the array in hopes that it will be encoded into the same number of bytes, resulting in a very small write to the NFC tag
            if (auto r = callback({ &encoder }); r < 0) {
                return r;
            }

            // Continue - keep copying the rest of the object
        }

        // If the field did not exist previously, create it
        if (!result.field_existed && callback) {
            // Write key
            if (auto r = nanocbor_fmt_uint(&encoder, field.field); r < 0) {
                return r;
            }

            // Write the value
            if (auto r = callback({ &encoder }); r < 0) {
                return r;
            }
        }

        // Map end
        nanocbor_leave_container(&decoder_root, &decoder);
        if (auto r = nanocbor_fmt_end_indefinite(&encoder); r < 0) {
            return r;
        }

        // Fill the rest of the region with zeroes to tidy up
        memset(encoder.cur, 0, encoder.end - encoder.cur);

        return 0;
    };
    if (auto r = prepare_data(); r < 0) {
        switch (r) {

        case NANOCBOR_ERR_OVERFLOW:
        case NANOCBOR_ERR_END:
            // This is the only error the fmt function returns, other errors are from the read
            return std::unexpected(Error::data_too_big);

        default:
            return std::unexpected(Error::region_corrupt);
        }
    };

    // Determine the span of data that actually changed - we will only be writing that one
    size_t write_pos, write_len;
    {
        const auto read_start = read_result->begin();
        const auto read_end = read_result->end();

        const auto write_start = write_buffer_.begin();
        const auto write_end = write_buffer_.begin() + region_meta.span.size;

        const auto first_diff = std::mismatch(read_start, read_end, write_start, write_end).first;

        // We have actually done no changes -> early return success
        if (first_diff == read_end) {
            return result;
        }

        const auto last_diff = std::mismatch(std::reverse_iterator(read_end), std::reverse_iterator(first_diff), std::reverse_iterator(write_end), std::reverse_iterator(write_start)).first;

        write_pos = first_diff - read_start;
        write_len = last_diff.base() - first_diff;
        result.changed = true;
    }

    // Write the data to the chip
    const auto write_result = backend_.write(field.tag, region_meta.span.offset + write_pos, std::span(write_buffer_.data() + write_pos, write_len));
    if (!write_result) {
        // If writing failed, data on the tag could get cocked up, so invalidate the cache
        invalidate_cache(field.tag);

        return std::unexpected(to_reader_error(write_result.error()));
    }

    // Here's a neat trick - after writing, copy the write buffer to the read buffer.
    // We would need to invalidate the buffer anyway, so this will actually make it ready for subsequent reads
    // (and writes, which are basically always read to read buffer + prepare data to write buffer)
    memcpy(read_buffer_.data(), write_buffer_.data(), region_meta.span.size);
    read_buffer_cache_ = {
        .tag = field.tag,
        .span = region_meta.span,
    };

    return result;
}
