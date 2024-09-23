#include <catch2/catch.hpp>
#include <esp_protocol/parser.hpp>
#include "generators.hpp"

static constexpr size_t DATA_START_OFFSET = sizeof(esp::MessagePrelude);
static constexpr size_t VAR_BYTE_LOCATION = offsetof(esp::MessagePrelude, header) + offsetof(esp::Header, variable_byte);

struct TestParserFallibleBase : public esp::RxParserBase {

    void process_scan_ap_count() override {
        REQUIRE(false);
    }

    void process_scan_ap_info() override {
        REQUIRE(false);
    }

    void process_invalid_message() override {
        REQUIRE(false);
    }

    bool start_packet() override {
        REQUIRE(false);
        return false;
    }

    void reset_packet() override {
        REQUIRE(false);
    }

    void update_packet(std::span<const uint8_t>) override {
        REQUIRE(false);
    }

    void process_packet() override {
        REQUIRE(false);
    }

    void process_esp_device_info() override {
        REQUIRE(false);
    }
};

struct TestDevInfoParser final : public TestParserFallibleBase {

    void process_esp_device_info() final {
        CHECK(buffer.size() == esp::MAC_SIZE);
        CHECK(msg.header.type == esp::MessageType::DEVICE_INFO_V2);
        CHECK(std::ranges::equal(buffer, mac_address_ref));
        CHECK(reference_version == msg.header.version);
        CHECK(checksum_valid);
        did_parse = true;
    }

    std::span<const uint8_t> mac_address_ref;
    uint8_t reference_version = 0;
    bool did_parse = false;
};

TEST_CASE("esp::RxParserBase device info basic tests", "[esp][uart_parser]") {
    std::vector<std::vector<uint8_t>> test_data = {
        { 0x55, 0x4E, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00, 0x0D, 0x00, 0x06, 0x50, 0x02, 0x3A, 0x81, 0x08, 0x3A, 0x8D, 0xFA, 0xA7, 0xD3 },
        { 0x55, 0x4E, 0x96, 0xD5, 0x37, 0x4C, 0x8A, 0x99, 0x00, 0x0D, 0x00, 0x06, 0x1C, 0x58, 0x40, 0xCF, 0x08, 0x3A, 0x8D, 0xFA, 0xA7, 0xD3 },
        { 0x55, 0x4E, 0xF4, 0x63, 0xE0, 0x40, 0x63, 0x2E, 0x00, 0x0D, 0x00, 0x06, 0xFA, 0xE0, 0x08, 0x8E, 0x08, 0x3A, 0x8D, 0xFA, 0xA7, 0xD3 },
    };
    TestDevInfoParser parser {};

    for (const auto &item : test_data) {
        esp::Intron intron;
        std::copy_n(item.begin(), esp::INTRON_SIZE, intron.begin());
        parser.set_intron(intron);
        parser.mac_address_ref = std::span<const uint8_t> { item.data() + DATA_START_OFFSET, esp::MAC_SIZE };
        parser.reference_version = item[VAR_BYTE_LOCATION];

        parser.process_data(item);
    }
    REQUIRE(parser.did_parse);
}

struct TestScanAPCountParser final : public TestParserFallibleBase {

    void process_scan_ap_count() final {
        CHECK(buffer.size() == 0);
        CHECK(msg.header.type == esp::MessageType::SCAN_AP_CNT);
        CHECK(reference_count == msg.header.ap_count);
        CHECK(checksum_valid);
        did_parse = true;
    }

    uint8_t reference_count;
    bool did_parse = false;
};

TEST_CASE("esp::RxParserBase scan ap count basic tests", "[esp][uart_parser]") {
    std::vector<std::vector<uint8_t>> test_data = {
        { 0x55, 0x4E, 0xF4, 0x63, 0xE0, 0x40, 0x63, 0x2E, 0x0A, 0x0A, 0x00, 0x00, 0x72, 0x1A, 0x91, 0x23 },
        { 0x55, 0x4E, 0xDC, 0x75, 0x0A, 0x05, 0xEB, 0x3C, 0x0A, 0x07, 0x00, 0x00, 0x15, 0x72, 0x4B, 0x8F },
        { 0x55, 0x4E, 0xDC, 0x75, 0x0A, 0x05, 0xEB, 0x3C, 0x0A, 0x09, 0x00, 0x00, 0x1F, 0xEC, 0x66, 0x85 },
    };
    TestScanAPCountParser parser {};

    for (const auto &item : test_data) {
        esp::Intron intron;
        std::copy_n(item.begin(), esp::INTRON_SIZE, intron.begin());
        parser.set_intron(intron);
        parser.reference_count = item[VAR_BYTE_LOCATION];

        parser.process_data(item);
    }
    REQUIRE(parser.did_parse);
}

struct TestScanAPGetParser final : public TestParserFallibleBase {

    void process_scan_ap_info() final {
        CHECK(buffer.size() == sizeof(esp::data::APInfo));
        CHECK(msg.header.type == esp::MessageType::SCAN_AP_GET);
        CHECK(msg.header.ap_index == reference_index);
        CHECK(std::ranges::equal(buffer, ap_info_data_ref));
        CHECK(checksum_valid);
        esp::data::APInfo *info = reinterpret_cast<esp::data::APInfo *>(buffer.data());
        CHECK(std::ranges::equal(info->ssid, ssid_ref));
        CHECK(info->requires_password == requires_password);
        did_parse = true;
    }

    uint8_t reference_index;
    std::span<const uint8_t> ap_info_data_ref;
    std::span<const uint8_t> ssid_ref;
    bool requires_password;
    bool did_parse = false;
};

static constexpr size_t REQUIRES_PASSWORD_OFFSET = sizeof(esp::MessagePrelude) + offsetof(esp::data::APInfo, requires_password);

TEST_CASE("esp::RxParserBase scan ap info basic tests", "[esp][uart_parser]") {
    std::vector<std::vector<uint8_t>> test_data = {
        { 0x55, 0x4E, 0xDC, 0x75, 0x0A, 0x05, 0xEB, 0x3C, 0x0B, 0x00, 0x00, 0x21, 0x40, 0x5C, 0x2B, 0x84, 0x57, 0x50, 0x41, 0x32, 0x2F, 0x33, 0x4D, 0x69, 0x78, 0x65, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 },
        { 0x55, 0x4E, 0xDC, 0x75, 0x0A, 0x05, 0xEB, 0x3C, 0x0B, 0x01, 0x00, 0x21, 0x32, 0x83, 0x35, 0x09, 0x57, 0x50, 0x41, 0x33, 0x2D, 0x74, 0x65, 0x73, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 },
        { 0x55, 0x4E, 0xDC, 0x75, 0x0A, 0x05, 0xEB, 0x3C, 0x0B, 0x04, 0x00, 0x21, 0xF5, 0x01, 0xA5, 0x77, 0x57, 0x54, 0x5F, 0x43, 0x45, 0x38, 0x36, 0x38, 0x35, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    };
    TestScanAPGetParser parser {};

    for (const auto &item : test_data) {
        esp::Intron intron;
        std::copy_n(item.begin(), esp::INTRON_SIZE, intron.begin());
        parser.set_intron(intron);
        parser.reference_index = item[VAR_BYTE_LOCATION];
        parser.ap_info_data_ref = std::span(item.data() + DATA_START_OFFSET, sizeof(esp::data::APInfo));
        parser.ssid_ref = std::span { item.data() + DATA_START_OFFSET, esp::SSID_LEN };
        parser.requires_password = item[REQUIRES_PASSWORD_OFFSET];

        parser.process_data(item);
    }
    REQUIRE(parser.did_parse);
}

struct TestPacketParser final : public TestParserFallibleBase {

    bool start_packet() final {
        needs_reset = true;
        return true;
    }

    void reset_packet() final {
        needs_reset = false;
    }

    void update_packet(std::span<const uint8_t> data) final {
        std::copy(data.begin(), data.end(), std::back_insert_iterator(packet_data));
    }

    void process_packet() final {
        CHECK(reference_link == msg.header.up);
        CHECK(checksum_valid);
        CHECK(msg.header.type == esp::MessageType::PACKET_V2);
        if (packet_data_ref.size() > 0) {
            CHECK(std::ranges::equal(packet_data, packet_data_ref));
            packet_data.clear();
        }
        did_parse = true;
    }

    std::vector<uint8_t> packet_data;
    std::span<const uint8_t> packet_data_ref;
    bool reference_link;
    bool did_parse;
    bool needs_reset;
};

struct TestPacketAllocFailParser final : public TestParserFallibleBase {

    bool start_packet() final {
        return false;
    }

    void reset_packet() final {
    }
};

TEST_CASE("esp::RxParserBase packet basic tests", "[esp][uart_parser]") {
    // TODO: Add test case with link down (not 100% needed)
    std::vector<std::vector<uint8_t>> test_data = {
        { 0x55, 0x4E, 0xC6, 0x93, 0x5A, 0x20, 0x25, 0x14, 0x07, 0x01, 0x00, 0x38, 0x98, 0x9E, 0xD3, 0x9E, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xA0, 0x51, 0x0B, 0x42, 0x42, 0x7B, 0x08, 0x06, 0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00, 0x01, 0xA0, 0x51, 0x0B, 0x42, 0x42, 0x7B, 0x0A, 0x19, 0xCE, 0xF6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x19, 0xF2, 0xC5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        { 0x55, 0x4E, 0xC6, 0x93, 0x5A, 0x20, 0x25, 0x14, 0x07, 0x01, 0x00, 0x56, 0x39, 0x47, 0x39, 0x2E, 0x33, 0x33, 0xFF, 0xCA, 0x21, 0xE7, 0xBC, 0xD0, 0x74, 0x04, 0xDF, 0x4D, 0x86, 0xDD, 0x60, 0x00, 0x00, 0x00, 0x00, 0x20, 0x3A, 0xFF, 0xFE, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x64, 0x2F, 0x4C, 0xA5, 0xFF, 0x3C, 0x8A, 0xFF, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xFF, 0xCA, 0x21, 0xE7, 0x87, 0x00, 0xBE, 0x3D, 0x00, 0x00, 0x00, 0x00, 0xFE, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, 0x49, 0x4C, 0x56, 0xEA, 0xCA, 0x21, 0xE7, 0x01, 0x01, 0xBC, 0xD0, 0x74, 0x04, 0xDF, 0x4D },
        { 0x55, 0x4E, 0xC6, 0x93, 0x5A, 0x20, 0x25, 0x14, 0x07, 0x01, 0x00, 0x00, 0x88, 0xC5, 0xF8, 0x5C },
    };
    TestPacketParser parser {};
    TestPacketAllocFailParser fail_parser {};

    for (const auto &item : test_data) {
        esp::Intron intron;
        std::copy_n(item.begin(), esp::INTRON_SIZE, intron.begin());
        parser.set_intron(intron);
        fail_parser.set_intron(intron);
        parser.reference_link = item[VAR_BYTE_LOCATION];
        parser.packet_data_ref = std::span(item.data() + DATA_START_OFFSET, item.size() - DATA_START_OFFSET);

        parser.process_data(item);
        fail_parser.process_data(item);

        REQUIRE(!parser.needs_reset);
    }
    REQUIRE(parser.did_parse);
}

struct TestInvalidParser final : public TestParserFallibleBase {

    void process_invalid_message() final {
        did_invalidate = true;
    }

    bool did_invalidate = false;
};

TEST_CASE("esp::RxParserBase invalid messages tests", "[esp][uart_parser]") {
    std::vector<std::vector<uint8_t>> test_data = {
        { 0x55, 0x4E, 0xC6, 0x93, 0x5A, 0x20, 0x25, 0x14, 0x01, 0x01, 0x00, 0x00, 0x78, 0xC6, 0x17, 0xA2 }, // type 1 is deprecated
        { 0x55, 0x4E, 0xF4, 0x63, 0xE0, 0x40, 0x63, 0x2E, 0x0A, 0x0A, 0x01, 0x00, 0x6C, 0x84, 0x2B, 0x97 }, // SCAN_AP_CNT with non zero size
        { 0x55, 0x4E, 0xF4, 0x63, 0xE0, 0x40, 0x63, 0x2E, 0x0A, 0x0A, 0x00, 0x01, 0x6C, 0x84, 0x2B, 0x97 }, // SCAN_AP_CNT with non zero size
        { 0x55, 0x4E, 0xDC, 0x75, 0x0A, 0x05, 0xEB, 0x3C, 0x0B, 0x00, 0x00, 0x50, 0x11, 0x50, 0xAC, 0x24, 0x57, 0x50, 0x41, 0x32, 0x2F, 0x33, 0x4D, 0x69, 0x78, 0x65, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 }, // SCAN_AP_GET with invalid size
        { 0x55, 0x4E, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00, 0x0D, 0x00, 0x04, 0xCC, 0x02, 0x72, 0x99, 0x08, 0x3A, 0x8D, 0xFA, 0xA7, 0xD3 }, // DEVICE_INFO_V2 with invalid size
    };

    TestInvalidParser parser {};

    for (const auto &item : test_data) {
        parser.did_invalidate = false;
        parser.process_data(item);
    }
}

struct TestUnfinishedParser final : public TestParserFallibleBase {

    void process_scan_ap_count() final {
        did_parse = true;
    }

    void process_scan_ap_info() final {
        did_parse = true;
    }

    bool start_packet() final {
        return false;
    }

    void reset_packet() final {
    }

    void process_packet() final {
        did_parse = true;
    }

    void process_esp_device_info() final {
        did_parse = true;
    }

    bool did_parse = false;
};

TEST_CASE("esp::RxParserBase unfinished messages tests", "[esp][uart_parser]") {
    std::vector<std::vector<uint8_t>> test_data = {
        { 0x55, 0x4E, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00, 0x0D, 0x00, 0x06, 0xCC, 0x02, 0x72, 0x99, 0x08, 0x3A, 0x8D, 0xFA, 0xA7 },
        { 0x55, 0x4E, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00, 0x0D, 0x00, 0x06, 0xCC, 0x02, 0x72 },
        { 0x55, 0x4E, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00, 0x0D, 0x00, 0x06, 0xCC, 0x02 },
        { 0x55, 0x4E, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05 },
    };

    TestUnfinishedParser parser {};

    for (const auto &item : test_data) {
        parser.did_parse = false;

        parser.process_data(item);

        CHECK(!parser.did_parse);
        parser.reset();
    }
}

struct TestValidMessageParser final : public TestParserFallibleBase {

    void process_scan_ap_count() final {
        did_parse = true;
    }

    void process_scan_ap_info() final {
        did_parse = true;
    }

    bool start_packet() final {
        packet_size = msg.header.size;
        return true;
    }

    void reset_packet() final {
    }

    void update_packet(std::span<const uint8_t> data) final {
        packet_size -= data.size();
    }

    void process_packet() final {
        did_parse = true;
    }

    void process_esp_device_info() final {
        did_parse = true;
    }

    bool did_parse = false;
    size_t packet_size = 0;
};

TEST_CASE("esp::RxParserBase random garbage recovery", "[esp][uart_parser]") {
    const auto &message = GENERATE(take(100, random_messages(esp::DEFAULT_INTRON)));
    const auto &message_prefix = GENERATE(take(10, random_bytes(0, 100)));

    TestValidMessageParser parser {};
    parser.set_intron(esp::DEFAULT_INTRON);
    parser.process_data(message_prefix);
    parser.process_data(message);
    CHECK(parser.did_parse);

    parser.did_parse = false;
    auto combined_message = std::vector(message_prefix);
    std::copy(message.begin(), message.end(), std::back_inserter(combined_message));

    parser.process_data(combined_message);
    CHECK(parser.did_parse);
    CHECK(parser.packet_size == 0);
}

TEST_CASE("esp::RxParserBase split data (2-parts)", "[esp][uart_parser]") {
    const auto &message = GENERATE(take(100, random_messages(esp::DEFAULT_INTRON)));
    const auto split_point = GENERATE_COPY(take(10, random(1, static_cast<int>(message.size() - 1))));

    const std::span<const uint8_t> part_a = std::span { message.data(), static_cast<size_t>(split_point) };
    const std::span<const uint8_t> part_b = std::span { message.data() + split_point, message.size() - split_point };

    TestValidMessageParser parser {};
    parser.set_intron(esp::DEFAULT_INTRON);
    parser.process_data(part_a);
    parser.process_data(part_b);
    CHECK(parser.did_parse);
}

struct MsgRecord {
    uint8_t type;
    uint8_t value;
    uint16_t len;
};

static MsgRecord captured_records[] = {
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 42 },
    { 7, 1, 0 },
    { 7, 1, 222 },
    { 7, 1, 60 },
    { 7, 1, 90 },
    { 7, 1, 0 },
    { 7, 1, 42 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 222 },
    { 7, 1, 342 },
    { 7, 1, 342 },
    { 7, 1, 77 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 222 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 222 },
    { 7, 1, 342 },
    { 7, 1, 0 },
    { 7, 1, 42 },
    { 7, 1, 60 },
    { 7, 1, 144 },
    { 7, 1, 0 },
    { 7, 1, 90 },
    { 7, 1, 42 },
    { 7, 1, 154 },
    { 7, 1, 104 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 222 },
    { 7, 1, 103 },
    { 7, 1, 103 },
    { 7, 1, 62 },
    { 7, 1, 77 },
    { 7, 1, 54 },
    { 7, 1, 1078 },
    { 7, 1, 265 },
    { 7, 1, 265 },
    { 7, 1, 0 },
    { 7, 1, 1078 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 222 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 222 },
    { 7, 1, 77 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 222 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 60 },
    { 7, 1, 54 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 105 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 222 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 122 },
    { 7, 1, 102 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 311 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 222 },
    { 7, 1, 54 },
    { 7, 1, 77 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 66 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 311 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 311 },
    { 7, 1, 54 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 222 },
    { 7, 1, 54 },
    { 7, 1, 370 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 60 },
    { 7, 1, 54 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 222 },
    { 7, 1, 54 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 60 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 60 },
    { 7, 1, 54 },
    { 7, 1, 222 },
    { 7, 1, 77 },
    { 7, 1, 0 },
    { 7, 1, 311 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 311 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 60 },
    { 7, 1, 311 },
    { 7, 1, 311 },
    { 7, 1, 0 },
    { 7, 1, 311 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 370 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 222 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 71 },
    { 7, 1, 71 },
    { 7, 1, 71 },
    { 7, 1, 121 },
    { 7, 1, 81 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 60 },
    { 7, 1, 54 },
    { 7, 1, 60 },
    { 7, 1, 54 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 222 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 60 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 311 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 311 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 222 },
    { 7, 1, 77 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 42 },
    { 7, 1, 0 },
    { 7, 1, 222 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 311 },
    { 7, 1, 222 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 77 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 222 },
    { 7, 1, 54 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 311 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 60 },
    { 7, 1, 174 },
    { 7, 1, 86 },
    { 7, 1, 0 },
    { 7, 1, 222 },
    { 7, 1, 86 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 86 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 77 },
    { 7, 1, 222 },
    { 7, 1, 54 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 311 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 222 },
    { 7, 1, 102 },
    { 7, 1, 122 },
    { 7, 1, 144 },
    { 7, 1, 0 },
    { 7, 1, 102 },
    { 7, 1, 122 },
    { 7, 1, 112 },
    { 7, 1, 82 },
    { 7, 1, 102 },
    { 7, 1, 405 },
    { 7, 1, 437 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 222 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 311 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 77 },
    { 7, 1, 222 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 222 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 311 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 222 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 77 },
    { 7, 1, 222 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 60 },
    { 7, 1, 54 },
    { 7, 1, 311 },
    { 7, 1, 0 },
    { 7, 1, 222 },
    { 7, 1, 73 },
    { 7, 1, 73 },
    { 7, 1, 73 },
    { 7, 1, 73 },
    { 7, 1, 73 },
    { 7, 1, 73 },
    { 7, 1, 243 },
    { 7, 1, 263 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 222 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 311 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 77 },
    { 7, 1, 222 },
    { 7, 1, 42 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 222 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 311 },
    { 7, 1, 222 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 154 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 77 },
    { 7, 1, 0 },
    { 7, 1, 222 },
    { 7, 1, 70 },
    { 7, 1, 86 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 222 },
    { 7, 1, 54 },
    { 7, 1, 311 },
    { 7, 1, 311 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 222 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 77 },
    { 7, 1, 222 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 311 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 222 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 222 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 311 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 60 },
    { 7, 1, 77 },
    { 7, 1, 222 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 222 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 311 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 222 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 77 },
    { 7, 1, 222 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 311 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 222 },
    { 7, 1, 42 },
    { 7, 1, 71 },
    { 7, 1, 71 },
    { 7, 1, 71 },
    { 7, 1, 121 },
    { 7, 1, 81 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 222 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 311 },
    { 7, 1, 60 },
    { 7, 1, 77 },
    { 7, 1, 222 },
    { 7, 1, 311 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 222 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 222 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 311 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 60 },
    { 7, 1, 77 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 222 },
    { 7, 1, 54 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 311 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 222 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 60 },
    { 7, 1, 77 },
    { 7, 1, 222 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 311 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 222 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 222 },
    { 7, 1, 122 },
    { 7, 1, 102 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 311 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 60 },
    { 7, 1, 222 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 222 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 54 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 311 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 42 },
    { 7, 1, 222 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 311 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 77 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 60 },
    { 7, 1, 222 },
    { 7, 1, 54 },
    { 7, 1, 311 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 222 },
    { 7, 1, 60 },
    { 7, 1, 73 },
    { 7, 1, 73 },
    { 7, 1, 73 },
    { 7, 1, 73 },
    { 7, 1, 73 },
    { 7, 1, 73 },
    { 7, 1, 243 },
    { 7, 1, 263 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 222 },
    { 7, 1, 60 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 311 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 77 },
    { 7, 1, 222 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 222 },
    { 7, 1, 60 },
    { 7, 1, 54 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 311 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 90 },
    { 7, 1, 0 },
    { 7, 1, 222 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 154 },
    { 7, 1, 104 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 77 },
    { 7, 1, 222 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 70 },
    { 7, 1, 54 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 311 },
    { 7, 1, 0 },
    { 7, 1, 222 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 222 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 311 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 77 },
    { 7, 1, 222 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 222 },
    { 7, 1, 60 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 311 },
    { 7, 1, 222 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 77 },
    { 7, 1, 222 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 311 },
    { 7, 1, 222 },
    { 7, 1, 0 },
    { 7, 1, 102 },
    { 7, 1, 122 },
    { 7, 1, 144 },
    { 7, 1, 60 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 82 },
    { 7, 1, 102 },
    { 7, 1, 405 },
    { 7, 1, 437 },
    { 7, 1, 0 },
    { 7, 1, 222 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 77 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 311 },
    { 7, 1, 222 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 222 },
    { 7, 1, 71 },
    { 7, 1, 71 },
    { 7, 1, 121 },
    { 7, 1, 81 },
    { 7, 1, 60 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 222 },
    { 7, 1, 54 },
    { 7, 1, 54 },
    { 7, 1, 311 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 60 },
    { 7, 1, 77 },
    { 7, 1, 222 },
    { 7, 1, 60 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 0 },
    { 7, 1, 222 },
    { 7, 1, 54 },
    { 7, 1, 0 },
    { 7, 1, 54 },
    { 7, 1, 311 },
    { 7, 1, 60 },
    { 7, 1, 60 },
};

struct TestCapturedSequence final : public TestParserFallibleBase {
    std::span<const MsgRecord> expected;
    TestCapturedSequence(std::span<const MsgRecord> expected)
        : expected(expected) {}

    bool in_packet = false;
    size_t position = 0;
    size_t pkt_len = 0;

    virtual bool start_packet() override {
        REQUIRE_FALSE(in_packet);
        REQUIRE(pkt_len == 0);
        REQUIRE(position < expected.size());
        REQUIRE(expected[position].type == 7);
        position++;
        in_packet = true;
        pkt_len = 0;
        return true;
    }

    virtual void update_packet(std::span<const uint8_t> data) override {
        REQUIRE(in_packet);
        pkt_len += data.size();
    }

    virtual void process_packet() override {
        REQUIRE(in_packet);
        REQUIRE(pkt_len == expected[position - 1].len);
        in_packet = false;
    }

    virtual void reset_packet() override {
        pkt_len = 0;
    }
};

TEST_CASE("esp::RxParseBase captured data", "[esp][uart_parser]") {
    FILE *f = fopen("uart.dump", "rb");
    REQUIRE(f);

    TestCapturedSequence parser(captured_records);
    const uint8_t intron[] = { 0x55, 0x4e, 0xd5, 0x2b, 0xa2, 0x82, 0x26, 0x6a };
    parser.set_intron(intron);

    while (!feof(f)) {
        REQUIRE(!ferror(f));
        uint32_t chunk_len = 0;
        if (fread(&chunk_len, sizeof chunk_len, 1, f) != 1) {
            // feof is set _after_ we crash into the end of the file, not ahead
            // of it.
            break;
        }
        std::vector<uint8_t> chunk;
        chunk.resize(chunk_len);
        REQUIRE(fread(chunk.data(), chunk_len, 1, f) == 1);
        parser.process_data(chunk);
    }
    // Yes, in case the test fails, we leak the file descriptor. Who cares in
    // tests...
    fclose(f);

    REQUIRE(parser.position == parser.expected.size());
}
