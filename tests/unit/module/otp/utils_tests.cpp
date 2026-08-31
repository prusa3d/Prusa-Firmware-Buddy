#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_all.hpp>

#include <otp/utils.hpp>

#include <algorithm>
#include <array>
#include <cstring>
#include <iterator>
#include <string_view>

namespace {

/// A 24-byte datamatrix built from its text form, zero-padded like a real OTP
std::array<uint8_t, 24> datamatrix_from(const char *text) {
    std::array<uint8_t, 24> buffer {};
    const size_t length = std::strlen(text);
    REQUIRE(length <= buffer.size());
    std::copy_n(text, length, buffer.begin());
    return buffer;
}

/// Minimal OTP_v2/v5 blob with the given version byte and datamatrix
std::array<uint8_t, sizeof(OTP_v2)> otp_v2_blob(uint8_t version, uint8_t bom_id, uint32_t timestamp, const char *datamatrix) {
    OTP_v2 otp {};
    otp.version = version;
    otp.size = sizeof(OTP_v2);
    otp.bomID = bom_id;
    otp.timestamp = timestamp;
    std::copy_n(datamatrix, std::strlen(datamatrix), std::begin(otp.datamatrix));

    std::array<uint8_t, sizeof(OTP_v2)> raw {};
    std::memcpy(raw.data(), &otp, sizeof(otp));
    return raw;
}

} // namespace

TEST_CASE("get_structure_version reads the first byte", "[otp_utils]") {
    const uint8_t version = GENERATE(as<uint8_t> {}, 0, 1, 2, 3, 4, 5, 42, 0xfe);

    std::array<uint8_t, 4> raw { version, 0, 0, 0 };

    const auto parsed = otp::helpers::get_structure_version(trivial_as_bytes(raw));

    INFO("version byte " << static_cast<int>(version));
    REQUIRE(parsed.has_value());
    REQUIRE(*parsed == version);
}

TEST_CASE("get_structure_version rejects unburned memory", "[otp_utils]") {
    std::array<uint8_t, 4> raw { 0xff, 0xff, 0xff, 0xff };
    REQUIRE_FALSE(otp::helpers::get_structure_version(trivial_as_bytes(raw)).has_value());
}

TEST_CASE("load_otp_unsafe copies the leading bytes of the span", "[otp_utils]") {
    const auto raw = otp_v2_blob(2, 7, 1672810819, "4558-02000019005259999");

    const auto loaded = otp::helpers::load_otp_unsafe<OTP_v2>(trivial_as_bytes(raw));

    REQUIRE(loaded.version == 2);
    REQUIRE(loaded.size == sizeof(OTP_v2));
    REQUIRE(loaded.bomID == 7);
    REQUIRE(loaded.timestamp == 1672810819);
    REQUIRE(std::memcmp(std::data(loaded.datamatrix), "4558-02000019005259999", 22) == 0);
}

TEST_CASE("load_otp_unsafe tolerates a span longer than the structure", "[otp_utils]") {
    // Real OTP reads hand over the whole memory window, not just the structure
    std::array<uint8_t, 527> raw {};
    const auto blob = otp_v2_blob(5, 9, 1645437492, "14558-020000190229999");
    std::copy(blob.begin(), blob.end(), raw.begin());

    const auto loaded = otp::helpers::load_otp_unsafe<OTP_v2>(trivial_as_bytes(raw));

    REQUIRE(loaded.version == 5);
    REQUIRE(loaded.bomID == 9);
    REQUIRE(loaded.timestamp == 1645437492);
}

TEST_CASE("parse_otp selects the structure matching the version byte", "[otp_utils]") {
    // Every version has its own alternative in AnyOTP, so the index matches the version byte.
    const auto [version, expected_index] = GENERATE(table<uint8_t, size_t>({
        { 0, 0 },
        { 1, 1 },
        { 2, 2 },
        { 3, 3 },
        { 4, 4 },
        { 5, 5 },
    }));

    std::array<uint8_t, 527> raw {};
    raw[0] = version;

    const auto parsed = otp::helpers::parse_otp(trivial_as_bytes(raw));

    INFO("version byte " << static_cast<int>(version));
    REQUIRE(parsed.has_value());
    REQUIRE(parsed->index() == expected_index);
}

TEST_CASE("parse_otp rejects unburned memory", "[otp_utils]") {
    std::array<uint8_t, 527> raw {};
    raw.fill(0xff);
    REQUIRE_FALSE(otp::helpers::parse_otp(trivial_as_bytes(raw)).has_value());
}

TEST_CASE("decode_datamatrix reads the daily date format", "[otp_utils]") {
    // Product IDs below 10381 encode the date as YMMDD
    const auto decoded = otp::decode_datamatrix(datamatrix_from("4558-02000019005259999"));

    REQUIRE(decoded.has_value());
    REQUIRE(decoded->product_id == 4558);
    REQUIRE(decoded->revision == 2);
    REQUIRE(std::string_view(decoded->supplier_id) == "000019");
    REQUIRE(decoded->production_year == 2020);
    REQUIRE(decoded->production_month == 5);
    REQUIRE(decoded->production_day == 25);
    REQUIRE(decoded->date_serial_number == 9999);
}

TEST_CASE("decode_datamatrix reads the calendar week format", "[otp_utils]") {
    // Product IDs from 10381 up encode the date as YWW
    const auto decoded = otp::decode_datamatrix(datamatrix_from("14558-0200001902299999"));

    REQUIRE(decoded.has_value());
    REQUIRE(decoded->product_id == 14558);
    REQUIRE(decoded->revision == 2);
    REQUIRE(std::string_view(decoded->supplier_id) == "000019");
    REQUIRE(decoded->production_year == 2020);
    REQUIRE(decoded->production_month == 5); // Thursday of week 22
    REQUIRE(decoded->production_day == 28);
    REQUIRE(decoded->date_serial_number == 99999);
}

TEST_CASE("decode_datamatrix picks the date format at the product id boundary", "[otp_utils]") {
    // 10380 is the last daily-date product id, 10381 the first calendar-week one
    const auto daily = otp::decode_datamatrix(datamatrix_from("10380-02000019005259999"));
    REQUIRE(daily.has_value());
    REQUIRE(daily->product_id == 10380);
    REQUIRE(daily->production_month == 5);
    REQUIRE(daily->production_day == 25);

    const auto weekly = otp::decode_datamatrix(datamatrix_from("10381-0200001902299999"));
    REQUIRE(weekly.has_value());
    REQUIRE(weekly->product_id == 10381);
    REQUIRE(weekly->production_month == 5); // Thursday of week 22
    REQUIRE(weekly->production_day == 28);
}

TEST_CASE("decode_datamatrix keeps letters in the supplier id", "[otp_utils]") {
    // Suppliers ran out of plain numbers, so the id is kept as text
    const auto decoded = otp::decode_datamatrix(datamatrix_from("14558-0200AB1902299999"));

    REQUIRE(decoded.has_value());
    REQUIRE(std::string_view(decoded->supplier_id) == "00AB19");

    // the letters must not shift the fields that follow
    REQUIRE(decoded->product_id == 14558);
    REQUIRE(decoded->revision == 2);
    REQUIRE(decoded->production_year == 2020);
    REQUIRE(decoded->production_month == 5);
    REQUIRE(decoded->production_day == 28);
    REQUIRE(decoded->date_serial_number == 99999);
}

TEST_CASE("decode_datamatrix rejects a malformed separator", "[otp_utils]") {
    const char *text = GENERATE(
        "455802000019005259999", // no '-' at all
        "123456789012345678901234" // all digits, the product id scan runs off the end
    );

    INFO("datamatrix " << text);
    REQUIRE_FALSE(otp::decode_datamatrix(datamatrix_from(text)).has_value());
}

TEST_CASE("decode_datamatrix reads the production year digit", "[otp_utils]") {
    // The year is a single digit offset from 2020, so the format only spans 2020-2029
    const int digit = GENERATE(range(0, 10));

    char text[] = "4558-02000019005259999";
    text[13] = static_cast<char>('0' + digit); // year digit

    const auto decoded = otp::decode_datamatrix(datamatrix_from(text));

    INFO("year digit " << digit);
    REQUIRE(decoded.has_value());
    REQUIRE(decoded->production_year == 2020 + digit);
}
