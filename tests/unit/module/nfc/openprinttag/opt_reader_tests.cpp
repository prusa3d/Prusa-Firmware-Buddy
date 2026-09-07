#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <unordered_map>
#include <string>
#include <vector>
#include <fstream>
#include <span>
#include <utils/byte_utils.hpp>
#include <ranges>

#include <test_utils/formatters.hpp>

#include <openprinttag/opt_reader.hpp>
#include <openprinttag/opt_backend.hpp>
#include <openprinttag/opt_fields.hpp>

using namespace openprinttag;

using ByteString = std::basic_string<std::byte>;

#include <autogen/sample_data.cpp.in>

class OPTBackend_Mock final : public OPTBackend {
public:
    struct ReadLog {
        TagID tag;
        size_t seq;
        size_t offset;
        size_t bytes;
    };
    struct WriteLog {
        TagID tag;
        size_t seq;
        size_t offset;
        size_t bytes;
    };
    struct Log {
        std::vector<ReadLog> reads;
        std::vector<WriteLog> writes;

        size_t seq_counter = 0;
    };
    Log log;

    std::unordered_map<TagID, ByteString> tag_data;
    std::vector<std::byte> tag_uid { std::byte { 0xBC }, std::byte { 0x6F }, std::byte { 0x2F }, std::byte { 0x66 }, std::byte { 0x08 }, std::byte { 0x01 }, std::byte { 0x04 }, std::byte { 0xE0 } };

    [[nodiscard]] virtual IOResult<void> read(TagID tag, PayloadPos start, const WritableBytes &buffer) final {

        log.reads.push_back(ReadLog {
            .tag = tag,
            .seq = log.seq_counter++,
            .offset = start,
            .bytes = buffer.size(),
        });

        if (!tag_data.contains(tag)) {
            return std::unexpected(IOError::other);
        }

        auto &data = tag_data[tag];
        if (start + buffer.size() > data.size()) {
            return std::unexpected(IOError::outside_of_bounds);
        }

        memcpy(buffer.data(), data.data() + start, buffer.size());
        return {};
    }

    [[nodiscard]] virtual IOResult<void> write(TagID tag, PayloadPos start, const Bytes &buffer) final {
        log.writes.push_back(WriteLog {
            .tag = tag,
            .seq = log.seq_counter++,
            .offset = start,
            .bytes = buffer.size(),
        });

        if (!tag_data.contains(tag)) {
            return std::unexpected(IOError::other);
        }

        auto &data = tag_data[tag];
        if (start + buffer.size() > data.size()) {
            return std::unexpected(IOError::outside_of_bounds);
        }

        memcpy(data.data() + start, buffer.data(), buffer.size());
        return {};
    }

    [[nodiscard]] virtual bool get_event(Event &e, uint32_t current_timestamp_ms) final {
        return false;
    }

    [[nodiscard]] IOResult<size_t> get_tag_uid(TagID tag, const WritableBytes &buffer) final {
        if (tag != 0) {
            return std::unexpected(IOError::invalid_id);
        }

        if (buffer.size() < tag_uid.size()) {
            return std::unexpected(IOError::data_too_big);
        }

        memcpy(buffer.data(), tag_uid.data(), tag_uid.size());
        return tag_uid.size();
    }

    OPTBackend::IOResult<void> read_tag_info(TagID tag, TagInfo &target) final {
        if (!tag_data.contains(tag)) {
            return std::unexpected(IOError::other);
        }

        target = {
            .tlv_span = PayloadSpan::from_offset_end(4, tag_data[tag].size()),
        };

        return {};
    }

    virtual void
    forget_tag(TagID tag) final {
    }

    virtual void reset_state() final {
    }
};

TagField field(auto f, TagID tag = 0) {
    return { .tag = tag, .section = field_section(f), .field = std::to_underlying(f) };
};

namespace std {
std::ostream &operator<<(std::ostream &os, const OPTReader::WriteReport &report) {
    os << "{existed: " << report.field_existed << "}";
    return os;
}
} // namespace std

template <typename A>
bool std::operator==(const std::span<A> &a, const std::ranges::range auto &b) {
    return std::ranges::equal(a, b);
}

void save_tag_data(const std::string_view &filename, const ByteString &data) {
    std::ofstream f;
    f.open(std::string(filename), std::ios_base::out | std::ios_base::binary);
    f.write((const char *)data.data(), data.size());
}

void test_standard_tag_main(OPTReader &reader, OPTBackend_Mock &mock) {
    mock.log = {};

    std::array<char, 128> string_buffer;
    std::array<std::byte, 128> bytes_buffer;
    std::array<uint16_t, 2> tags_buffer;
    std::array<Field, 32> fields_buffer;

    const std::array expected_fields {
        (Field)MainField::material_class,
        (Field)MainField::material_type,
        (Field)MainField::brand_name,
        (Field)MainField::material_name,
        (Field)MainField::nominal_netto_full_weight,
        (Field)MainField::transmission_distance,
        (Field)MainField::tags,
        (Field)MainField::brand_specific_instance_id,
        (Field)MainField::material_uuid,
    };
    CHECK(reader.enumerate_fields(0, Region::main, fields_buffer) == expected_fields);

    CHECK(reader.read_field_int32(field(MainField::material_type)) == std::to_underlying(MaterialType::ASA));
    CHECK(reader.read_field_int32(field(MainField::material_class)) == std::to_underlying(MaterialClass::FFF));

    CHECK(reader.read_field_int64(field(MainField::nominal_netto_full_weight)) == 0x1ffffffff);
    CHECK(reader.read_field_float(field(MainField::material_type)) == std::to_underlying(MaterialType::ASA));

    CHECK_THAT(reader.read_field_float(field(MainField::transmission_distance)).value_or(0), Catch::Matchers::WithinAbs(0.3f, 0.001f));

    CHECK(reader.read_field_string(field(MainField::brand_name), string_buffer) == std::string_view("Prusament"));
    CHECK(reader.read_field_string(field(MainField::material_name), string_buffer) == std::string_view("PLA Prusa Galaxy Black"));

    CHECK(reader.read_field_string(field(MainField::brand_specific_instance_id), string_buffer) == std::string_view { "1" });
    CHECK(reader.read_field_bytes(field(MainField::material_uuid), bytes_buffer) == tag_data::material_uuid);

    CHECK(reader.read_field_uint16_array(field(MainField::tags), tags_buffer) == std::array { (uint16_t)MaterialTag::glitter, (uint16_t)MaterialTag::abrasive });

    // Problematic reads
    std::array<char, 4> small_string_buffer;
    std::array<uint16_t, 1> small_tags_buffer;

    CHECK(reader.read_field_int32(field(MainField::nominal_netto_full_weight)) == std::unexpected(OPTReader::Error::data_too_big));

    CHECK(reader.read_field_int32(field(MainField::transmission_distance)) == std::unexpected(OPTReader::Error::wrong_field_type));

    CHECK(reader.read_field_string(field(MainField::material_name), small_string_buffer) == std::unexpected(OPTReader::Error::data_too_big));
    CHECK(reader.read_field_int32(field(MainField::secondary_color_0)) == std::unexpected(OPTReader::Error::field_not_present));

    CHECK(reader.read_field_uint16_array(field(MainField::tags), small_tags_buffer) == std::unexpected(OPTReader::Error::data_too_big));

    // Expected one read to cache in the main region
    CHECK(mock.log.reads.size() == 1);
}

TEST_CASE("OPTReader::reading_standard_sample_tag") {
    OPTBackend_Mock mock;
    OPTReader reader(mock);

    namespace td = tag_data::standard_sample_tag;
    mock.tag_data[0] = td::data;

    // Verify metadata structure
    {
        auto meta_r = reader.read_metadata(0);

        // Expecting (NOT CC - read_tag_info fakes it), TLV, NDEF header, NDEF type, metadata region
        CHECK(mock.log.reads.size() == 4);
        CHECK(mock.log.writes.size() == 0);

        REQUIRE(meta_r.has_value());
        REQUIRE(*meta_r);

        const OPTReader::TagMetadata &meta = **meta_r;
        CHECK(meta.is_valid);

        CHECK(meta.region[Region::meta].span == td::meta_span);
        CHECK(meta.region[Region::main].span == td::main_span);
        CHECK(!meta.region[Region::auxiliary].is_present());
    }

    // Read from meta region (it's actually empty)
    {
        mock.log = {};

        CHECK(reader.read_field_int32(field(MetaField::main_region_size)) == std::unexpected(OPTReader::Error::field_not_present));

        // The meta region should be in the cache from the read_metadata call, no extra reads should have happened
        CHECK(mock.log.reads.size() == 0);
    }

    test_standard_tag_main(reader, mock);

    // Read from aux region
    {
        mock.log = {};

        CHECK(reader.read_field_int32(field(AuxField::consumed_weight)) == std::unexpected(OPTReader::Error::field_not_present));

        // No extra reads - the reader should remmeber that there is no aux section
        CHECK(mock.log.reads.size() == 0);
    }
}

TEST_CASE("OPTReader::reading_standard_sample_tag_with_aux") {
    OPTBackend_Mock mock;
    OPTReader reader(mock);

    namespace td = tag_data::standard_sample_tag_with_aux;
    mock.tag_data[0] = td::data;

    std::array<Field, 32> fields_buffer;

    // Verify metadata structure
    {
        auto meta_r = reader.read_metadata(0);

        // Expecting - (NOT CC, read_tag_info fakes it), TLV, NDEF header, NDEF type, metadata region
        CHECK(mock.log.reads.size() == 4);
        CHECK(mock.log.writes.size() == 0);

        // Reset log, start tracking from new
        mock.log = {};

        REQUIRE(meta_r.has_value());
        REQUIRE(*meta_r);

        const OPTReader::TagMetadata &meta = **meta_r;
        CHECK(meta.is_valid);

        CHECK(meta.region[Region::meta].span == td::meta_span);
        CHECK(meta.region[Region::main].span == td::main_span);
        CHECK(meta.region[Region::auxiliary].span == td::aux_span);
    }

    // Read from meta region
    {
        mock.log = {};

        CHECK(reader.read_field_int32(field(MetaField::main_region_offset)) == std::unexpected(OPTReader::Error::field_not_present));
        CHECK(reader.read_field_int32(field(MetaField::aux_region_offset)) == td::aux_payload_offset);

        // The meta region should be in the cache from the read_metadata call, no extra reads should have happened
        CHECK(mock.log.reads.size() == 0);
    }

    test_standard_tag_main(reader, mock);

    // Read from aux region
    {
        mock.log = {};

        CHECK(reader.enumerate_fields(0, Region::auxiliary, fields_buffer) == std::array { (Field)AuxField::consumed_weight });
        CHECK(reader.read_field_int32(field(AuxField::consumed_weight)) == 100);

        // Expected one read reading the aux section
        CHECK(mock.log.reads.size() == 1);
    }
}

TEST_CASE("OPTReader::writing") {
    OPTBackend_Mock mock;
    OPTReader reader(mock);

    std::array<char, 128> string_buffer;
    std::array<Field, 32> fields_buffer;

    namespace td = tag_data::standard_sample_tag_with_aux;
    mock.tag_data[0] = td::data;

    CHECK(reader.read_field_int32(field(MainField::material_type)) == std::to_underlying(MaterialType::ASA));
    // Expect reads of (NOT CC - faked), TLV, NDEF header, mime type, meta region, main region
    CHECK(mock.log.reads.size() == 5);
    mock.log = {};

    CHECK(reader.read_field_int32(field(AuxField::consumed_weight)) == 100);
    // Expect read of aux region
    CHECK(mock.log.reads.size() == 1);
    mock.log = {};

    // Try writing an int while we have a different region cached
    {
        CHECK(reader.write_field_int32(field(MainField::material_type), std::to_underlying(MaterialType::PETG)) == OPTReader::WriteReport { .field_existed = true, .changed = true });
        // We had aux region cached last, so main region read is expected
        CHECK(mock.log.reads.size() == 1);
        // And one write
        REQUIRE(mock.log.writes.size() == 1);
        CHECK(mock.log.writes[0].bytes == 1);
        mock.log = {};

        CHECK(reader.read_field_int32(field(MainField::material_type)) == std::to_underlying(MaterialType::PETG));
        CHECK(mock.tag_data[0] == tag_data::write_sample_1::data);
        // save_tag_data("write_sample_1.bin", mock.tag_data[0]);

        // The write op should have updated the read cache, so we expect no reads
        CHECK(mock.log.reads.size() == 0);
    }

    // Try writing a string
    {
        CHECK(reader.read_field_string(field(MainField::material_name), string_buffer) == std::string_view("PLA Prusa Galaxy Black"));
        CHECK(reader.write_field_string(field(MainField::material_name), "PLA Kura Balaxy Klack") == OPTReader::WriteReport { .field_existed = true, .changed = true });

        CHECK(mock.log.writes.size() == 1);
        CHECK(mock.log.reads.size() == 0);
        mock.log = {};

        // Try both reads again
        CHECK(reader.read_field_int32(field(MainField::material_type)) == std::to_underlying(MaterialType::PETG));
        CHECK(reader.read_field_string(field(MainField::material_name), string_buffer) == std::string_view("PLA Kura Balaxy Klack"));
        CHECK(mock.tag_data[0] == tag_data::write_sample_2::data);
        // save_tag_data("write_sample_2.bin", mock.tag_data[0]);

        CHECK(mock.log.reads.size() == 0);
        mock.log = {};
    }

    // Try writing a field that already wasn't there
    {
        CHECK(reader.read_field_float(field(MainField::min_nozzle_diameter)) == std::unexpected(OPTReader::Error::field_not_present));
        CHECK(reader.write_field_float(field(MainField::min_nozzle_diameter), 12.5f) == OPTReader::WriteReport { .field_existed = false, .changed = true });
        CHECK(mock.tag_data[0] == tag_data::write_sample_3::data);
        // save_tag_data("write_sample_3.bin", mock.tag_data[0]);

        CHECK(reader.read_field_float(field(MainField::min_nozzle_diameter)) == 12.5f);

        CHECK(mock.log.writes.size() == 1);
        CHECK(mock.log.reads.size() == 0);
        CHECK(mock.log.writes[0].bytes == 6);
        mock.log = {};
    }

    // Try writing into the aux region
    {
        CHECK(reader.write_field_float(field(AuxField::consumed_weight), 969.12f) == OPTReader::WriteReport { .field_existed = true, .changed = true });
        CHECK(mock.tag_data[0] == tag_data::write_sample_4::data);
        // save_tag_data("write_sample_4.bin", mock.tag_data[0]);

        CHECK_THAT(reader.read_field_float(field(AuxField::consumed_weight)).value_or(0), Catch::Matchers::WithinAbs(969.12f, 0.001f));

        CHECK(mock.log.writes.size() == 1);
        CHECK(mock.log.reads.size() == 1);
        CHECK(mock.log.writes[0].bytes == 6);
        mock.log = {};
    }

    // Try writing float that can be encoded as int
    {
        CHECK(reader.write_field_float(field(AuxField::consumed_weight), 12) == OPTReader::WriteReport { .field_existed = true, .changed = true });
        CHECK(mock.tag_data[0] == tag_data::write_sample_5::data);
        // save_tag_data("write_sample_5.bin", mock.tag_data[0]);

        CHECK(reader.read_field_float(field(AuxField::consumed_weight)) == 12);
        CHECK(reader.read_field_int32(field(AuxField::consumed_weight)) == 12);

        CHECK(mock.log.writes.size() == 1);
        CHECK(mock.log.reads.size() == 0);
        mock.log = {};
    }

    // Try writing the same value again
    {
        CHECK(reader.write_field_float(field(AuxField::consumed_weight), 12) == OPTReader::WriteReport { .field_existed = true, .changed = false });

        CHECK(mock.log.writes.size() == 0);
        CHECK(mock.log.reads.size() == 0);
        mock.log = {};
    }

    // Try writing the same value in a non-cached region
    {
        CHECK(reader.write_field_float(field(MainField::material_type), std::to_underlying(MaterialType::PETG)) == OPTReader::WriteReport { .field_existed = true, .changed = false });

        CHECK(mock.log.writes.size() == 0);
        CHECK(mock.log.reads.size() == 1);
        mock.log = {};
    }

    // Remove a field from the main region
    {
        CHECK(reader.remove_field(field(MainField::material_type)) == OPTReader::WriteReport { .field_existed = true, .changed = true });

        CHECK(mock.log.writes.size() == 1);
        CHECK(mock.log.reads.size() == 0);
        CHECK(mock.tag_data[0] == tag_data::write_sample_6::data);
        // save_tag_data("write_sample_6.bin", mock.tag_data[0]);
        mock.log = {};

        CHECK(reader.read_field_float(field(MainField::material_type)) == std::unexpected(OPTReader::Error::field_not_present));
        CHECK(reader.remove_field(field(MainField::material_type)) == OPTReader::WriteReport { .field_existed = false, .changed = false });
        CHECK(mock.log.reads.size() == 0);
    }

    // Remove a field from the aux region
    {
        CHECK(reader.remove_field(field(AuxField::consumed_weight)) == OPTReader::WriteReport { .field_existed = true, .changed = true });

        CHECK(mock.log.writes.size() == 1);
        CHECK(mock.log.reads.size() == 1);
        CHECK(mock.tag_data[0] == tag_data::write_sample_7::data);
        // save_tag_data("write_sample_7.bin", mock.tag_data[0]);
        mock.log = {};

        CHECK(reader.read_field_float(field(AuxField::consumed_weight)) == std::unexpected(OPTReader::Error::field_not_present));
        CHECK(reader.remove_field(field(AuxField::consumed_weight)) == OPTReader::WriteReport { .field_existed = false, .changed = false });
        CHECK(reader.enumerate_fields(0, Region::auxiliary, fields_buffer) == std::span<Field> {});

        CHECK(mock.log.reads.size() == 0);
    }

    // Try writing bytes
    {
        CHECK(reader.write_field_string(field(MainField::brand_specific_instance_id), "12") == OPTReader::WriteReport { .field_existed = true, .changed = true });
        CHECK(mock.tag_data[0] == tag_data::write_sample_8::data);
        mock.log = {};
    }

    // Try writing tags
    {
        std::array<uint16_t, 8> tags_buffer;
        auto r = reader.read_field_uint16_array(field(MainField::tags), tags_buffer);
        REQUIRE(r.has_value());

        size_t cnt = r->size();
        REQUIRE(cnt < tags_buffer.size());

        tags_buffer[cnt++] = (uint16_t)MaterialTag::abrasive;

        CHECK(reader.write_field_uint16_array(field(MainField::material_name), std::span(tags_buffer.data(), cnt)) == OPTReader::WriteReport { .field_existed = true, .changed = true });

        CHECK(mock.log.writes.size() == 1);
        mock.log = {};
    }
}

TEST_CASE("OPTReader::edge_cases") {
    OPTBackend_Mock mock;
    OPTReader reader(mock);

    SECTION("Try writing a string that definitely doesn't fit into the memory") {
        mock.tag_data[0] = tag_data::standard_sample_tag_with_aux::data;

        std::vector<char> str;
        str.resize(300);
        std::ranges::fill(str, 'a');

        CHECK(reader.write_field_string(field(MainField::material_name), std::string_view(str.data(), str.size())) == std::unexpected(OPTReader::Error::data_too_big));

        str.resize(200);
        CHECK(reader.write_field_string(field(MainField::material_name), std::string_view(str.data(), str.size())) == std::unexpected(OPTReader::Error::data_too_big));

        CHECK(mock.log.writes.size() == 0);
    }

    SECTION("Enumerate fields with a small buffer") {
        mock.tag_data[0] = tag_data::standard_sample_tag_with_aux::data;

        std::array<Field, 7> fields_buffer;
        CHECK(reader.enumerate_fields(0, Region::main, fields_buffer) == std::unexpected(OPTReader::Error::data_too_big));
    }

    SECTION("Tags with invalid metadata") {
        SECTION("Tag with main section outside of the tag itself") {
            mock.tag_data[0] = tag_data::corrupt_tag_1::data;
        }

        SECTION("Tag with main section bigger than the payload") {
            mock.tag_data[0] = tag_data::corrupt_tag_2::data;
        }

        CHECK(reader.read_field_int32(field(MainField::brand_name)) == std::unexpected(OPTReader::Error::tag_invalid));

        // Expecting reads of (NOT CC - faked), TLV, NDEF header, ID and meta region
        CHECK(mock.log.reads.size() == 4);
        mock.log = {};

        // The tag should be marked as invalid in the cache and we shouldn't be getting any reads
        CHECK(reader.read_field_bool(field(AuxField::consumed_weight)) == std::unexpected(OPTReader::Error::tag_invalid));
        CHECK(reader.write_field_int32(field(MainField::brand_name), 0) == std::unexpected(OPTReader::Error::tag_invalid));
        CHECK(mock.log.reads.size() == 0);
        CHECK(mock.log.writes.size() == 0);
    }

    SECTION("Tags with a different mime type") {
        mock.tag_data[0] = tag_data::wrong_mime::data;
        mock.tag_data[1] = tag_data::wrong_mime_2::data;
        mock.tag_data[2] = tag_data::standard_sample_tag::data;

        CHECK(reader.read_field_int32(field(MainField::brand_name, 0)) == std::unexpected(OPTReader::Error::tag_invalid));
        UNSCOPED_INFO("Expecting reads of a TLV and NDEF headers (NOT CC - faked). Different MIME type length, so we don't even don't need to read it");
        CHECK(mock.log.reads.size() == 2);
        mock.log = {};

        CHECK(reader.read_field_float(field(MainField::brand_name, 1)) == std::unexpected(OPTReader::Error::tag_invalid));
        UNSCOPED_INFO("Expecting reads of (NOT CC - faked), TLV, NDEF header + mime ID");
        CHECK(mock.log.reads.size() == 3);
        mock.log = {};

        CHECK(reader.read_field_int32(field(MainField::material_type, 2)) == std::to_underlying(MaterialType::ASA));
        UNSCOPED_INFO("Read (NOT CC - faked), TLV, NDEF header, mime type, metadata, main data");
        CHECK(mock.log.reads.size() == 5);
        mock.log = {};

        {
            INFO("Information that the tags are invalid should be stored in the cache");
            CHECK(reader.read_field_int32(field(MainField::material_type, 0)) == std::unexpected(OPTReader::Error::tag_invalid));
            CHECK(mock.log.reads.size() == 0);

            CHECK(reader.read_field_int32(field(MainField::material_type, 1)) == std::unexpected(OPTReader::Error::tag_invalid));
            CHECK(mock.log.reads.size() == 0);
            mock.log = {};
        }

        {
            INFO("Invalidating tag 1 should result in the tag being re-read when requested");
            reader.invalidate_cache(1);

            CHECK(reader.read_field_int32(field(MainField::material_type, 0)) == std::unexpected(OPTReader::Error::tag_invalid));
            CHECK(mock.log.reads.size() == 0);

            CHECK(reader.read_field_int32(field(MainField::material_type, 1)) == std::unexpected(OPTReader::Error::tag_invalid));
            UNSCOPED_INFO("Should read (NOT CC - faked) TLV, NDEF header & mime type");
            CHECK(mock.log.reads.size() == 3);
            mock.log = {};
        }
    }

    SECTION("Tag with a malformed section") {
        std::array<char, 128> string_buffer;

        namespace td = tag_data::tag_with_big_meta_section;
        auto tag_data = td::data;
        std::fill(tag_data.begin() + 96, tag_data.end(), std::byte('\xbf')); // bf - indefinite container open
        mock.tag_data[0] = tag_data;

        {
            INFO("Reads in the non-damaged part - should return values still");
            CHECK(reader.read_field_int32(field(MetaField::main_region_offset)) == td::main_payload_offset);
            CHECK(mock.log.reads.size() == 4);
            mock.log = {};

            CHECK(reader.read_field_string(field(MainField::brand_name), string_buffer) == std::string_view("Prusament"));
            CHECK(mock.log.reads.size() == 1);
            mock.log = {};
        }

        {
            INFO("Reads in the damaged part - should return error");
            CHECK(reader.read_field_int32(field(MainField::transmission_distance)) == std::unexpected(OPTReader::Error::region_corrupt));
        }

        {
            INFO("Writing to the damaged part should fail without any writes");
            CHECK(reader.write_field_float(field(MainField::transmission_distance), 18) == std::unexpected(OPTReader::Error::region_corrupt));
            CHECK(mock.log.reads.size() == 0); // Should have been cached
            CHECK(mock.log.writes.size() == 0);
            mock.log = {};
        }
    }

    SECTION("Tag without the container open") {
        namespace td = tag_data::empty_tag_with_big_meta_section;
        auto tag_data = td::data;
        mock.tag_data[0] = tag_data;

        const auto main_region_span = (**reader.read_metadata(0)).main_region().span;

        // Zero out the whole main region - that means that there is no cbor container tag at the beginning
        std::fill(tag_data.begin() + main_region_span.offset, tag_data.begin() + main_region_span.end(), std::byte(0));

        mock.tag_data[0] = tag_data;
        reader.invalidate_cache(0);

        // Meta region should be okay
        CHECK(reader.read_field_int32(field(MetaField::main_region_offset)) == td::main_payload_offset);

        // Main region should return corrupt
        CHECK(reader.read_field_int32(field(MainField::brand_name)) == std::unexpected(OPTReader::Error::region_corrupt));

        // Meta region should be still okay
        CHECK(reader.read_field_int32(field(MetaField::main_region_offset)) == td::main_payload_offset);

        // Writing to the main region should fail
        CHECK(reader.write_field_float(field(MainField::transmission_distance), 18) == std::unexpected(OPTReader::Error::region_corrupt));
    }

    SECTION("Minimal meta section") {
        namespace td = tag_data::empty_tag_with_minimal_meta_section;
        mock.tag_data[0] = td::data;

        auto meta_r = reader.read_metadata(0);
        REQUIRE(meta_r.has_value());

        const auto &meta = **meta_r;
        CHECK(meta.is_valid);

        CHECK(meta.region[Region::meta].span == td::meta_span);
        CHECK(meta.region[Region::main].span == td::main_span);
        CHECK(!meta.region[Region::auxiliary].is_present());
    }

    SECTION("Big meta section") {
        namespace td = tag_data::empty_tag_with_big_meta_section;
        mock.tag_data[0] = td::data;

        auto meta_r = reader.read_metadata(0);
        REQUIRE(meta_r.has_value());

        const auto &meta = **meta_r;
        CHECK(meta.is_valid);

        CHECK(meta.region[Region::meta].span == td::meta_span);
        CHECK(meta.region[Region::main].span == td::main_span);
        CHECK(meta.region[Region::auxiliary].span == td::aux_span);
    }

    SECTION("Disallow writing to the meta section") {
        mock.tag_data[0] = tag_data::standard_sample_tag_with_aux::data;
        CHECK(reader.write_field_int32(field(MetaField::aux_region_offset), 0) == std::unexpected(OPTReader::Error::write_protected));
    }

    SECTION("Disallow writing to the meta section") {
        mock.tag_data[0] = tag_data::standard_sample_tag_with_aux::data;
        CHECK(reader.write_field_int32(field(MetaField::aux_region_offset), 0) == std::unexpected(OPTReader::Error::write_protected));
    }
}

TEST_CASE("OPTReader::caching") {
    OPTBackend_Mock mock;
    OPTReader reader(mock);
    for (size_t i = 0; i < OPTReader::metadata_cache_capacity + 2; i++) {
        mock.tag_data[(TagID)i] = tag_data::standard_sample_tag_with_aux::data;
    }

    const auto check_full_read = [&](TagID tag) {
        INFO("Full read " << tag);

        auto meta_r = reader.read_metadata(tag);
        volatile auto rr = meta_r;
        CHECK(meta_r.has_value());

        // Expecting reads of (NOT CC - faked), TLV, NDEF header, ID and meta region
        CHECK(mock.log.reads.size() == 4);
        for (const auto &read : mock.log.reads) {
            CHECK(read.tag == tag);
        }
        mock.log = {};
    };

    const auto check_no_read = [&](TagID tag) {
        INFO("No read " << tag);

        auto meta_r = reader.read_metadata(tag);
        CHECK(meta_r.has_value());

        CHECK(mock.log.reads.size() == 0);
        mock.log = {};
    };

    for (size_t i = 0; i < OPTReader::metadata_cache_capacity; i++) {
        INFO("Fill the cache");
        check_full_read(i);
    }

    for (size_t i = 0; i < OPTReader::metadata_cache_capacity; i++) {
        INFO("Read again through all items that should be in the cache - no reads should be needed for reading metadata");
        check_no_read(i);
    }

    {
        INFO("Now read another chip, exceeding the capacity");
        check_full_read(OPTReader::metadata_cache_capacity);
    }

    {
        INFO("Reading again the same should result in no reads");
        check_no_read(OPTReader::metadata_cache_capacity);
    }

    {
        INFO("But the oldest-accessed tag should have been thrown out of the cache");
        INFO("Now we should have tag 0 replacing tag 1");
        check_full_read(0);
        check_no_read(0);
    }

    {
        INFO("Tag 2 should be in the cache still");
        check_no_read(2);
        check_no_read(0);
        check_no_read(OPTReader::metadata_cache_capacity);
    }

    {
        INFO("Tag 1 should be out of the cache now. Reading it should replace tag 2");
        check_full_read(1);
        check_full_read(3);
    }
}

TEST_CASE("OPTReader::not_first_ndef_record") {
    OPTBackend_Mock mock;
    OPTReader reader(mock);

    namespace td = tag_data::sample_tag_with_uri;
    mock.tag_data[0] = td::data;

    // Verify metadata structure
    {
        auto meta_r = reader.read_metadata(0);

        // Expecting three reads - (NOT CC - faked), TLV, URI NDEF header, OpenPrintTag NDEF header, NDEF type, metadata region
        CHECK(mock.log.reads.size() == 5);
        CHECK(mock.log.writes.size() == 0);

        // Reset log, start tracking from new
        mock.log = {};

        REQUIRE(meta_r.has_value());
        REQUIRE(*meta_r);

        const OPTReader::TagMetadata &meta = **meta_r;
        CHECK(meta.is_valid);

        CHECK(meta.region[Region::meta].span == td::meta_span);
        CHECK(meta.region[Region::main].span == td::main_span);
        CHECK(meta.region[Region::auxiliary].span == td::aux_span);
    }
}
