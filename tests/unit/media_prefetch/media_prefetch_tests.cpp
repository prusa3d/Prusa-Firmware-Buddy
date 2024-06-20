#include <catch2/catch.hpp>
#include <cstring>

// We're naughty and want full access to MediaPrefetchManager
#define private   public
#define protected public
#include <media_prefetch.hpp>
#undef private
#undef protected

#include <test_tools/gcode_provider.hpp>

using S = MediaPrefetchManager::Status;
using RR = MediaPrefetchManager::ReadResult;
using R = GcodeReaderResult;

bool read_gcode(MediaPrefetchManager &mp, const char *cmd) {
    MediaPrefetchManager::ReadResult c;
    return (mp.read_command(c) == S::ok) && !strcmp(c.gcode.data(), cmd);
}

TEST_CASE("media_prefetch::basic_test") {
    MediaPrefetchManager::ReadResult c;

    SECTION("Empty media prefetch should return end of file") {
        MediaPrefetchManager mp;
        CHECK(mp.read_command(c) == S::end_of_file);
        CHECK(mp.read_command(c) == S::end_of_file);
    }

    SECTION("Basic reading checks") {
        StubGcodeProviderMemory p;
        p.add_line("G0");
        p.add_line("G1");
        p.add_result(R::RESULT_TIMEOUT);

        MediaPrefetchManager mp;
        mp.start(p.filename(), {});

        REQUIRE(read_gcode(mp, "G0"));
        REQUIRE(read_gcode(mp, "G1"));
        REQUIRE(mp.read_command(c) == S::end_of_buffer);

        // We have not issued a fetch, so we should still see end of buffer now
        REQUIRE(mp.read_command(c) == S::end_of_buffer);

        SECTION("Fetch after end of buffer -> end of file") {
            mp.issue_fetch(true);
        }

        SECTION("Fetch more date -> read the data") {
            p.add_line("G2");
            mp.issue_fetch(true);
            REQUIRE(read_gcode(mp, "G2"));
        }

        REQUIRE(mp.read_command(c) == S::end_of_file);
        REQUIRE(p.has_read_all());
    }
}

// Test of the ring buffer guts
TEST_CASE("media_prefetch::buffer_test") {
    MediaPrefetchManager mp;
    auto &reader_read_head = mp.shared_state.read_head.buffer_pos;
    auto &read_tail = mp.shared_state.read_tail.buffer_pos;
    auto &write_tail = mp.worker_state.write_tail.buffer_pos;
    auto &writer_read_head = mp.worker_state.read_head.buffer_pos;

    SECTION("Default initialization checks") {
        CHECK(mp.can_read_entry_raw(0));
        CHECK(!mp.can_read_entry_raw(1));
        CHECK(!mp.can_read_entry_raw(64));

        CHECK(mp.can_write_entry_raw(0));
        CHECK(mp.can_write_entry_raw(1));
        CHECK(mp.can_write_entry_raw(64));
        CHECK(mp.can_write_entry_raw(mp.buffer_size - 1));

        {
            mp.write_entry<uint32_t>(0x12345678);
            // Not yet, the write_tail was not copied to read_tail
            CHECK(!mp.can_read_entry_raw(4));

            // Now we should be able to read
            read_tail = write_tail;
            REQUIRE(mp.can_read_entry_raw(4));

            // And check what we've read
            uint32_t val = 0;
            mp.read_entry(val);
            CHECK(val == 0x12345678);
        }
    }

    SECTION("Single-byte wrap checks") {
        // read_tail is 0, write_tail is just one byte behind it -> we shouldn't be able to write anything
        write_tail = mp.buffer_size - 1;
        CHECK(mp.can_write_entry_raw(0));
        CHECK(!mp.can_write_entry_raw(1));
        CHECK(!mp.can_write_entry_raw(10));

        const auto check_read_write = [&] {
            CHECK(mp.can_write_entry_raw(0));
            REQUIRE(mp.can_write_entry_raw(1));
            CHECK(!mp.can_write_entry_raw(2));
            CHECK(!mp.can_write_entry_raw(16));

            // Write the byte and read it back again
            mp.write_entry<uint8_t>(0xfe);

            // Not yet, the write_tail was not copied to read_tail
            CHECK(!mp.can_read_entry_raw(1));

            // Move and check that we can read exactly one byte
            read_tail = write_tail;
            writer_read_head = reader_read_head = (read_tail + mp.buffer_size - 1) % mp.buffer_size;
            CHECK(mp.can_read_entry_raw(0));
            REQUIRE(mp.can_read_entry_raw(1));
            CHECK(!mp.can_read_entry_raw(2));

            uint8_t val = 0;
            mp.read_entry(val);
            CHECK(val == 0xfe);
        };

        SECTION("Move write_tail back") {
            // if we move the write back a bit, we should be able to write one byte
            write_tail--;

            check_read_write();
        }
        SECTION("Move read_head forward") {
            // Alternatively, if we move the read head forward a bit, we should also be able to write one byte
            read_tail = writer_read_head = reader_read_head = reader_read_head + 1;

            check_read_write();
        }
    }

    SECTION("Multi-byte wrap checks") {
        // Somewhere in the middle, we don't care
        writer_read_head = 512;

        const auto check_read_write = [&] {
            constexpr uint32_t expected_data = 0x81AC12CF;

            reader_read_head = write_tail;

            REQUIRE(mp.can_write_entry_raw(4));
            mp.write_entry(expected_data);

            read_tail = write_tail;

            REQUIRE(mp.can_read_entry_raw(4));
            uint32_t data;
            mp.read_entry(data);
            CHECK(data == expected_data);

            // That is all, we shouldn't be able to read anything more
            CHECK(!mp.can_read_entry_raw(1));
        };

        SECTION("Wrap at 1") {
            write_tail = mp.buffer_size - 1;
            check_read_write();
        }
        SECTION("Wrap at 2") {
            write_tail = mp.buffer_size - 2;
            check_read_write();
        }
        SECTION("Wrap at 3") {
            write_tail = mp.buffer_size - 3;
            check_read_write();
        }
        SECTION("Do not wrap") {
            write_tail = mp.buffer_size - 4;
            check_read_write();
        }
    }
}

TEST_CASE("media_prefetch::feed_test") {
    StubGcodeProviderMemory p;
    MediaPrefetchManager mp;

    // Make sure that we feed enough commands, that shouldn't fit in the buffer
    constexpr size_t command_count = mp.buffer_size;

    const auto command_str = [](int i) {
        std::array<char, 96> r;
        snprintf(r.data(), r.size(), "G%i", i);
        return std::string(r.data());
    };

    // Put the commands in the buffer
    for (size_t i = 0; i < command_count; i++) {
        p.add_line(command_str(i));
    }

    // Start the prefetch, fetch one buffer worth
    mp.start(p.filename(), {});

    // Consequent fetch should not do anything
    {
        const auto old_read_tail = mp.shared_state.read_tail.buffer_pos;
        mp.issue_fetch(true);
        CHECK(mp.shared_state.read_tail.buffer_pos == old_read_tail);
    }

    size_t command_i = 0;
    size_t media_offset = 0;
    RR r;
    S status;
    const auto read_whole_buffer = [&] {
        while ((status = mp.read_command(r)) == S::ok) {
            const std::string expected_command = command_str(command_i).data();
            CHECK(std::string(r.gcode.data()) == expected_command);
            CHECK(r.replay_pos.offset == media_offset);
            CHECK(r.resume_pos.offset == media_offset + expected_command.size() + 1); // +1 for newline

            command_i++;
            media_offset = r.resume_pos.offset;
        }
    };
    read_whole_buffer();

    // There should definitely be more commands to be fetched
    CHECK(status == S::end_of_buffer);

    // We definitely shouldn't have been able to fetch more commands that would fit in the buffer
    // Assume each command needs to be encoded in at least 3 bytes
    CHECK(command_i <= mp.buffer_size / 3);

    // Fetch and check the rest of the file
    while (status == S::end_of_buffer) {
        mp.issue_fetch(true);
        read_whole_buffer();
    }

    CHECK(status == S::end_of_file);
}
