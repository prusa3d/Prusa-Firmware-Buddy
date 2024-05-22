#include <catch2/catch.hpp>
#include <cstring>

#include "puppies/fifo_encoder.hpp"
#include "puppies/fifo_decoder.hpp"

using namespace common::puppies::fifo;

static constexpr LogData log_fragment = { 'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o' };
static constexpr LoadcellRecord loadcell_fragment = { 0, 0x12345678 };
static constexpr AccelerometerFastData accelerometer_fast_fragment = { 0x87654321, 0x12354678 };

TEST_CASE("Encoder allignment") {
    std::array<uint16_t, MODBUS_FIFO_LEN> fifo;
    Encoder encoder(fifo);
    fifo.fill(0xaaaa);
    REQUIRE(encoder.encode(loadcell_fragment));
    encoder.padd();

    REQUIRE(fifo[encoder.position() - 1] == 0x0012);
}

TEST_CASE("Encode log") {
    std::array<uint16_t, MODBUS_FIFO_LEN> fifo = { 0 };

    Encoder encoder(fifo);

    REQUIRE(encoder.can_encode<LogData>());
    REQUIRE(encoder.encode(log_fragment));

    REQUIRE(encoder.can_encode<LogData>());
    REQUIRE(encoder.encode(log_fragment));

    REQUIRE(encoder.can_encode<LogData>());
    REQUIRE(encoder.encode(log_fragment));

    REQUIRE(encoder.can_encode<LogData>());
    REQUIRE(encoder.encode(log_fragment));

    encoder.padd();

    struct __attribute__((packed)) CompleteLogRecord {
        Header header;
        LogData data;
    };

    struct CompleteFifo {
        CompleteLogRecord records[4];
        uint8_t zero_padd[26];
    } const complete_fifo = {
        .records = {
            {
                .header = {
                    .type = MessageType::log,
                },
                .data = log_fragment,
            },
            {
                .header = {
                    .type = MessageType::log,
                },
                .data = log_fragment,
            },
            {
                .header = {
                    .type = MessageType::log,
                },
                .data = log_fragment,
            },
            {
                .header = {
                    .type = MessageType::log,
                },
                .data = log_fragment,
            },
        },
        .zero_padd = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
    };

    static_assert(sizeof(struct CompleteLogRecord) == 9);
    static_assert(sizeof(struct CompleteFifo) == 62);

    REQUIRE(fifo == *reinterpret_cast<const std::array<uint16_t, MODBUS_FIFO_LEN> *>(&complete_fifo));
}

TEST_CASE("Encode combination") {
    std::array<uint16_t, MODBUS_FIFO_LEN> fifo = { 0 };

    Encoder encoder(fifo);

    REQUIRE(encoder.can_encode<LogData>());
    REQUIRE(encoder.encode(log_fragment));

    REQUIRE(encoder.can_encode<LoadcellRecord>());
    REQUIRE(encoder.encode(loadcell_fragment));

    REQUIRE(encoder.can_encode<LogData>());
    REQUIRE(encoder.encode(log_fragment));

    REQUIRE(encoder.can_encode<LoadcellRecord>());
    REQUIRE(encoder.encode(loadcell_fragment));

    REQUIRE(encoder.can_encode<LogData>());
    REQUIRE(encoder.encode(log_fragment));

    REQUIRE(encoder.can_encode<LogData>());
    REQUIRE(encoder.encode(log_fragment));

    REQUIRE(!encoder.can_encode<LogData>());
    REQUIRE(!encoder.encode(log_fragment));

    encoder.padd();

    struct __attribute__((packed)) CompleteLogRecord {
        Header header;
        LogData data;
    };

    struct __attribute__((packed)) CompleteLoadcellRecord {
        Header header;
        LoadcellRecord data;
    };

    struct CompleteFifo {
        CompleteLogRecord rec1;
        CompleteLoadcellRecord rec2;
        CompleteLogRecord rec3;
        CompleteLoadcellRecord rec4;
        CompleteLogRecord rec5;
        CompleteLogRecord rec6;
        uint8_t zero_padd[8];
    } const complete_fifo = {
        .rec1 = {
            .header = {
                .type = MessageType::log,
            },
            .data = log_fragment,
        },
        .rec2 = {
            .header = {
                .type = MessageType::loadcell,
            },
            .data = loadcell_fragment,
        },
        .rec3 = {
            .header = {
                .type = MessageType::log,
            },
            .data = log_fragment,
        },
        .rec4 = {
            .header = {
                .type = MessageType::loadcell,
            },
            .data = loadcell_fragment,
        },
        .rec5 = {
            .header = {
                .type = MessageType::log,
            },
            .data = log_fragment,
        },
        .rec6 = {
            .header = {
                .type = MessageType::log,
            },
            .data = log_fragment,
        },
        .zero_padd = { 0, 0, 0, 0, 0, 0, 0, 0 },
    };

    static_assert(sizeof(struct CompleteLogRecord) == 9);
    static_assert(sizeof(struct CompleteLoadcellRecord) == 9);
    static_assert(sizeof(struct CompleteFifo) == 62);

    REQUIRE(fifo == *reinterpret_cast<const std::array<uint16_t, MODBUS_FIFO_LEN> *>(&complete_fifo));
}

TEST_CASE("Encode - null decode") {
    std::array<uint16_t, MODBUS_FIFO_LEN> fifo = { 0 };

    Encoder encoder(fifo);

    REQUIRE(encoder.can_encode<LogData>());
    REQUIRE(encoder.encode(log_fragment));
    encoder.padd();

    Decoder decoder(fifo, encoder.position());
    decoder.decode({});
}

TEST_CASE("Empty decode") {
    std::array<uint16_t, MODBUS_FIFO_LEN> fifo = { 0 };
    Decoder decoder(fifo, fifo.size());
    decoder.decode({});
}

TEST_CASE("Encode - decode") {
    std::array<uint16_t, MODBUS_FIFO_LEN> fifo = { 0 };

    Encoder encoder(fifo);

    REQUIRE(encoder.encode(log_fragment));
    REQUIRE(encoder.encode(loadcell_fragment));
    REQUIRE(encoder.encode(accelerometer_fast_fragment));
    encoder.padd();

    Decoder decoder(fifo, encoder.position());

    int num_log = 0;
    int num_loadcell = 0;
    int num_accelerometer_fast = 0;

    decoder.decode({
        [&num_log](LogData data) {
            REQUIRE(data == log_fragment);
            num_log++;
        },
        [&num_loadcell](LoadcellRecord data) {
            REQUIRE(data.timestamp == loadcell_fragment.timestamp);
            REQUIRE(data.loadcell_raw_value == loadcell_fragment.loadcell_raw_value);
            num_loadcell++;
        },
        [&num_accelerometer_fast](AccelerometerFastData data) {
            REQUIRE(data[0] == accelerometer_fast_fragment[0]);
            REQUIRE(data[1] == accelerometer_fast_fragment[1]);
            num_accelerometer_fast++;
        },
    });

    CHECK(num_log == 1);
    CHECK(num_loadcell == 1);
    CHECK(num_accelerometer_fast == 1);
}

TEST_CASE("Encode - partial decode") {
    std::array<uint16_t, MODBUS_FIFO_LEN> fifo = { 0 };

    Encoder encoder(fifo);

    REQUIRE(encoder.encode(log_fragment));
    REQUIRE(encoder.encode(loadcell_fragment));
    encoder.padd();

    Decoder decoder(fifo, encoder.position());

    int num_log = 0;

    decoder.decode({
        [&num_log](LogData data) {
            REQUIRE(data == log_fragment);
            num_log++;
        },
        NULL,
        NULL,
        NULL,
    });

    CHECK(num_log == 1);
}
