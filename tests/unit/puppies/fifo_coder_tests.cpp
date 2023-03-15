#include <catch2/catch.hpp>
#include <cstring>

#include "puppies/fifo_encoder.hpp"
#include "puppies/fifo_decoder.hpp"

using namespace common::puppies::fifo;

static constexpr LogData_t log_fragment = { 'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o' };
static constexpr LoadCellData_t loadcell_fragment = 0x12345678;

TEST_CASE("Encoder allignment") {
    std::array<uint16_t, MODBUS_FIFO_LEN> fifo;
    Encoder encoder(fifo);
    fifo.fill(0xaaaa);
    REQUIRE(encoder.encode(0, loadcell_fragment));
    encoder.padd();

    REQUIRE(fifo[encoder.position() - 1] == 0x0012);
}

TEST_CASE("Encode log") {
    std::array<uint16_t, MODBUS_FIFO_LEN> fifo = { 0 };

    Encoder encoder(fifo);

    REQUIRE(encoder.can_encode<LogData_t>());
    REQUIRE(encoder.encode(1, log_fragment));

    REQUIRE(encoder.can_encode<LogData_t>());
    REQUIRE(encoder.encode(2, log_fragment));

    REQUIRE(encoder.can_encode<LogData_t>());
    REQUIRE(encoder.encode(3, log_fragment));

    REQUIRE(encoder.can_encode<LogData_t>());
    REQUIRE(encoder.encode(4, log_fragment));

    encoder.padd();

    struct __attribute__((packed)) CompleteLogRecord {
        Header_t header;
        LogData_t data;
    };

    struct CompleteFifo {
        CompleteLogRecord records[4];
        uint8_t zero_padd[10];
    } const complete_fifo = {
        .records = {
            {
                .header = {
                    .timestamp_us = 1,
                    .type = MessageType::log,
                },
                .data = log_fragment,
            },
            {
                .header = {
                    .timestamp_us = 2,
                    .type = MessageType::log,
                },
                .data = log_fragment,
            },
            {
                .header = {
                    .timestamp_us = 3,
                    .type = MessageType::log,
                },
                .data = log_fragment,
            },
            {
                .header = {
                    .timestamp_us = 4,
                    .type = MessageType::log,
                },
                .data = log_fragment,
            },
        },
        .zero_padd = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
    };

    static_assert(sizeof(struct CompleteLogRecord) == 13);
    static_assert(sizeof(struct CompleteFifo) == 62);

    REQUIRE(fifo == *reinterpret_cast<const std::array<uint16_t, MODBUS_FIFO_LEN> *>(&complete_fifo));
}

TEST_CASE("Encode combination") {
    std::array<uint16_t, MODBUS_FIFO_LEN> fifo = { 0 };

    Encoder encoder(fifo);

    REQUIRE(encoder.can_encode<LogData_t>());
    REQUIRE(encoder.encode(1, log_fragment));

    REQUIRE(encoder.can_encode<LoadCellData_t>());
    REQUIRE(encoder.encode(2, loadcell_fragment));

    REQUIRE(encoder.can_encode<LogData_t>());
    REQUIRE(encoder.encode(3, log_fragment));

    REQUIRE(encoder.can_encode<LoadCellData_t>());
    REQUIRE(encoder.encode(4, loadcell_fragment));

    REQUIRE(encoder.can_encode<LogData_t>());
    REQUIRE(encoder.encode(5, log_fragment));

    REQUIRE(!encoder.can_encode<LogData_t>());
    REQUIRE(!encoder.encode(6, log_fragment));

    encoder.padd();

    struct __attribute__((packed)) CompleteLogRecord {
        Header_t header;
        LogData_t data;
    };

    struct __attribute__((packed)) CompleteLoadcellRecord {
        Header_t header;
        LoadCellData_t data;
    };

    struct CompleteFifo {
        CompleteLogRecord rec1;
        CompleteLoadcellRecord rec2;
        CompleteLogRecord rec3;
        CompleteLoadcellRecord rec4;
        CompleteLogRecord rec5;
        uint8_t zero_padd[5];
    } const complete_fifo = {
        .rec1 = {
            .header = {
                .timestamp_us = 1,
                .type = MessageType::log,
            },
            .data = log_fragment,
        },
        .rec2 = {
            .header = {
                .timestamp_us = 2,
                .type = MessageType::loadcell,
            },
            .data = loadcell_fragment,
        },
        .rec3 = {
            .header = {
                .timestamp_us = 3,
                .type = MessageType::log,
            },
            .data = log_fragment,
        },
        .rec4 = {
            .header = {
                .timestamp_us = 4,
                .type = MessageType::loadcell,
            },
            .data = loadcell_fragment,
        },
        .rec5 = {
            .header = {
                .timestamp_us = 5,
                .type = MessageType::log,
            },
            .data = log_fragment,
        },
        .zero_padd = { 0, 0, 0, 0, 0 },
    };

    static_assert(sizeof(struct CompleteLogRecord) == 13);
    static_assert(sizeof(struct CompleteLoadcellRecord) == 9);
    static_assert(sizeof(struct CompleteFifo) == 62);

    REQUIRE(fifo == *reinterpret_cast<const std::array<uint16_t, MODBUS_FIFO_LEN> *>(&complete_fifo));
}

TEST_CASE("Encode - null decode") {
    std::array<uint16_t, MODBUS_FIFO_LEN> fifo = { 0 };

    Encoder encoder(fifo);

    REQUIRE(encoder.can_encode<LogData_t>());
    REQUIRE(encoder.encode(1, log_fragment));
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

    REQUIRE(encoder.encode(1, log_fragment));
    REQUIRE(encoder.encode(2, loadcell_fragment));
    encoder.padd();

    Decoder decoder(fifo, encoder.position());

    int num_log = 0;
    int num_loadcell = 0;

    decoder.decode({
        [&num_log](TimeStamp_us_t time_us, LogData_t data) {
            REQUIRE(time_us == 1);
            REQUIRE(data == log_fragment);
            num_log++;
        },
        [&num_loadcell](TimeStamp_us_t time_us, LoadCellData_t data) {
            REQUIRE(time_us == 2);
            REQUIRE(data == loadcell_fragment);
            num_loadcell++;
        },
    });

    CHECK(num_log == 1);
    CHECK(num_loadcell == 1);
}

TEST_CASE("Encode - partial decode") {
    std::array<uint16_t, MODBUS_FIFO_LEN> fifo = { 0 };

    Encoder encoder(fifo);

    REQUIRE(encoder.encode(1, log_fragment));
    REQUIRE(encoder.encode(2, loadcell_fragment));
    encoder.padd();

    Decoder decoder(fifo, encoder.position());

    int num_log = 0;

    decoder.decode({
        [&num_log](TimeStamp_us_t time_us, LogData_t data) {
            REQUIRE(time_us == 1);
            REQUIRE(data == log_fragment);
            num_log++;
        },
        NULL,
    });

    CHECK(num_log == 1);
}
