#include <catch2/catch.hpp>
#include <cstring>
#include <random>
#include <sstream>
#include <filesystem>
#include <fstream>

// We're naughty and want full access to MediaPrefetchManager
#define private   public
#define protected public
#include <media_prefetch.hpp>
#include <prefetch_compression.hpp>
#undef private
#undef protected

#include <test_tools/gcode_provider.hpp>

using namespace media_prefetch;

using S = MediaPrefetchManager::Status;
using RR = MediaPrefetchManager::ReadResult;
using R = GCodeReaderResult;

std::string read_gcode(MediaPrefetchManager &mp, bool cropped = false) {
    MediaPrefetchManager::ReadResult c;
    return (mp.read_command(c) == S::ok && c.cropped == cropped) ? std::string(c.gcode.data()) : std::string {};
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
        p.add_gcode("G0");
        p.add_gcode("G1");
        p.add_breakpoint(R::RESULT_TIMEOUT);

        MediaPrefetchManager mp;
        mp.start(p.filename(), {});
        mp.issue_fetch();

        REQUIRE(read_gcode(mp) == "G0");
        REQUIRE(read_gcode(mp) == "G1");
        REQUIRE(mp.read_command(c) == S::end_of_buffer);

        // We have not issued a fetch, so we should still see end of buffer now
        REQUIRE(mp.read_command(c) == S::end_of_buffer);

        SECTION("Fetch after end of buffer -> end of file") {
            mp.issue_fetch();
        }

        SECTION("Fetch more date -> read the data") {
            p.add_gcode("G2");
            mp.issue_fetch();
            REQUIRE(read_gcode(mp) == "G2");
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
            REQUIRE(!mp.can_read_entry_raw(5));

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

TEST_CASE("media_prefetch::file_handle_tests") {
    MediaPrefetchManager::ReadResult c;

    SECTION("File gets closed after reading the whole file") {
        StubGcodeProviderMemory p;
        p.add_gcode("G0");

        MediaPrefetchManager mp;
        mp.start(p.filename(), {});
        mp.issue_fetch();
        CHECK(!mp.worker_state.gcode_reader.is_open());
    }

    SECTION("File stays closed after exhaustion") {
        StubGcodeProviderMemory p;
        p.add_gcode("G0");

        MediaPrefetchManager mp;
        mp.start(p.filename(), {});
        mp.issue_fetch();
        REQUIRE(read_gcode(mp) == "G0");
        REQUIRE(mp.read_command(c) == S::end_of_file);
        CHECK(!mp.worker_state.gcode_reader.is_open());

        // Stays closed even if more issue_fetches are called
        mp.issue_fetch();

        CHECK(!mp.worker_state.gcode_reader.is_open());
        REQUIRE(mp.read_command(c) == S::end_of_file);

        mp.issue_fetch();

        // It stays closed even if the reader _would_ return an error.
        p.add_breakpoint(R::RESULT_ERROR);
        CHECK(!mp.worker_state.gcode_reader.is_open());
        REQUIRE(mp.read_command(c) == S::end_of_file);
    }

    SECTION("File gets closed after an error") {
        StubGcodeProviderMemory p;
        p.add_gcode("G0");
        p.add_breakpoint(R::RESULT_ERROR);
        p.add_gcode("G1");

        MediaPrefetchManager mp;
        mp.start(p.filename(), {});
        mp.issue_fetch();

        REQUIRE(read_gcode(mp) == "G0");
        REQUIRE(mp.read_command(c) == S::usb_error);
        REQUIRE(!mp.worker_state.gcode_reader.is_open());

        mp.issue_fetch();
        REQUIRE(read_gcode(mp) == "G1");
        REQUIRE(mp.read_command(c) == S::end_of_file);
        REQUIRE(!mp.worker_state.gcode_reader.is_open());
    }

    SECTION("File gets closed after calling stop()") {
        StubGcodeProviderMemory p;
        p.add_breakpoint(R::RESULT_TIMEOUT);

        MediaPrefetchManager mp;
        mp.start(p.filename(), {});
        mp.issue_fetch();

        REQUIRE(mp.read_command(c) == S::end_of_buffer);
        REQUIRE(mp.worker_state.gcode_reader.is_open());

        mp.stop();
        REQUIRE(!mp.worker_state.gcode_reader.is_open());
    }
}

TEST_CASE("media_prefetch::feed_test") {
    StubGcodeProviderMemory p;
    MediaPrefetchManager mp;

    // Make sure that we feed enough commands, that shouldn't fit in the buffer
    const size_t command_count = mp.buffer_size * 2;

    const auto command_str = [](int i, bool for_writing = false) {
        std::stringbuf buf;
        std::ostream ss(&buf);

        ss << "G" << i;

        // Try to pass in a few comments at some places, the prefetch should ignore them out
        if (for_writing && (i % 3 == 0)) {
            ss << "; comment " << i;
        }

        return buf.str();
    };

    std::vector<uint32_t> command_replay_positions;

    // Put the commands in the buffer
    for (size_t i = 0; i < command_count; i++) {
        const auto replay_pos = p.add_gcode(command_str(i, true));
        command_replay_positions.push_back(replay_pos);
    }

    struct {
        size_t command_i = 0;
        size_t whole_buffer_count = 0;
    } read_state;

    RR r;
    S status;

    const auto read_single_command = [&] {
        status = mp.read_command(r);
        if (status != S::ok) {
            return false;
        }

        const std::string expected_command = command_str(read_state.command_i).data();
        CAPTURE(read_state.command_i, expected_command);

        REQUIRE(std::string(r.gcode.data()) == expected_command);
        REQUIRE(command_replay_positions[read_state.command_i] == r.replay_pos.offset);
        REQUIRE(r.resume_pos.offset == r.replay_pos.offset + command_str(read_state.command_i, true).size() + 1); // +1 for newline
        read_state.command_i++;
        return true;
    };

    const auto read_whole_buffer = [&] {
        read_state.whole_buffer_count++;

        while (read_single_command()) {
        }
    };

    // Start the prefetch, fetch one buffer worth
    mp.start(p.filename(), {});
    mp.issue_fetch();

    // Consequent fetch should not do anything
    {
        const auto old_read_tail = mp.shared_state.read_tail.buffer_pos;
        mp.issue_fetch();
        CHECK(mp.shared_state.read_tail.buffer_pos == old_read_tail);
    }

    // Read the buffer whole
    {
        read_state = {};
        read_whole_buffer();

        // There should definitely be more commands to be fetched
        CHECK(status == S::end_of_buffer);

        // We definitely shouldn't have been able to fetch more commands that would fit in the buffer
        // Assume each command needs to be encoded in at least 3 bytes
        CHECK(read_state.command_i <= mp.buffer_size / 3);

        // Fetch and check the rest of the file
        while (status == S::end_of_buffer) {
            mp.issue_fetch();
            read_whole_buffer();
        }

        CHECK(status == S::end_of_file);
    }

    const auto first_run_whole_buffer_count = read_state.whole_buffer_count;
    REQUIRE(read_state.command_i == command_count);

    read_state = {};

    SECTION("Reread with stops") {
        p.add_breakpoint(R::RESULT_TIMEOUT, 1);
        p.add_breakpoint(R::RESULT_TIMEOUT, 2);
        p.add_breakpoint(R::RESULT_TIMEOUT, 16);
        p.add_breakpoint(R::RESULT_TIMEOUT, 32);
        p.add_breakpoint(R::RESULT_TIMEOUT, 64);
        p.add_breakpoint(R::RESULT_TIMEOUT, 128);

        const auto breakpoint_count = p.breakpoint_count();

        // Restart the prefetch
        mp.start(p.filename(), {});

        do {
            mp.issue_fetch();
            read_whole_buffer();
        } while (status == S::end_of_buffer);

        CHECK(status == S::end_of_file);
        CHECK(read_state.command_i == command_count);

        // We should be able to read the buffer in the same number of whole reads, plus the breakpoints we've inserted
        CHECK(read_state.whole_buffer_count == first_run_whole_buffer_count + breakpoint_count);
    }

    SECTION("Stream resumes") {
        // Resume streams at various commands and check that all works well
        const std::vector<size_t> restart_positions {
            1,
            2,
            command_count / 4,
            command_count / 3,
            command_count / 2,
            command_count - 2,
            command_count - 1,
        };

        for (const auto restart_cmd_i : restart_positions) {
            const auto resume_position = command_replay_positions[restart_cmd_i];
            CAPTURE(restart_cmd_i, resume_position);

            read_state = {};
            read_state.command_i = restart_cmd_i;
            mp.start(p.filename(), GCodeReaderPosition { {}, resume_position });

            do {
                read_whole_buffer();
                mp.issue_fetch();
            } while (status == S::end_of_buffer);

            CHECK(status == S::end_of_file);
            CHECK(read_state.command_i == command_count);
        }
    }

    // Read buffer partially, then fetch to top it up.
    // This should provide standard results
    SECTION("Partial reads") {
        // Restart the prefetch
        mp.start(p.filename(), {});

        size_t seed;
        SECTION("Fixed seed") {
            // Keyboard-mash generated seed to ensure repeatibility of the unittest
            seed = 65468;
        }
        SECTION("Random seed") {
            seed = std::random_device()();
        }

        CAPTURE(seed);
        std::minstd_rand rand_gen;
        rand_gen.seed(seed);

        // We always read one to buffer_size / 8 commands
        std::uniform_int_distribution<> distrib(0, mp.buffer_size / 8);

        do {
            int read_cnt = distrib(rand_gen);
            do {
                read_single_command();
            } while (--read_cnt && status == S::ok);

            mp.issue_fetch();
        } while (status == S::end_of_buffer || status == S::ok);

        CHECK(status == S::end_of_file);
        CHECK(read_state.command_i == command_count);
    }

    // Same as before, but we also discard the jobs in random intervals
    SECTION("Partial reads with discarding") {
        // Restart the prefetch
        mp.start(p.filename(), {});

        size_t seed;
        SECTION("Fixed seed") {
            // Keyboard-mash generated seed to ensure repeatibility of the unittest
            seed = 13654;
        }
        SECTION("Random seed") {
            seed = std::random_device()();
        }

        CAPTURE(seed);
        std::minstd_rand rand_gen;
        rand_gen.seed(seed);

        // The media prefetch should check for discard at least once per command write
        const int min_discard_count = 32;
        std::uniform_int_distribution<> distrib(0, command_count / min_discard_count);

        size_t discarded_jobs_count = 0;

        std::vector<int> discards;

        do {
            int read_cnt = distrib(rand_gen);
            do {
                read_single_command();
            } while (--read_cnt && status == S::ok);

            mp.worker_job.discard_after = distrib(rand_gen);
            discards.push_back(*mp.worker_job.discard_after);
            mp.issue_fetch();

            if (mp.worker_job.was_discarded()) {
                // Discard only happens together with worker_reset_pending IRL, so we gotta issue that
                mp.shared_state.worker_reset_pending = true;
                discarded_jobs_count++;
            }
        } while (status == S::end_of_buffer || status == S::ok);

        CHECK(status == S::end_of_file);
        CHECK(read_state.command_i == command_count);
        CHECK(mp.worker_job.discard_check_count >= min_discard_count);

        // Sanity check that we have discarded anything at all
        CAPTURE(discarded_jobs_count);
        CAPTURE(discards);
        CHECK(discarded_jobs_count >= min_discard_count);
    }

    SECTION("Single continuous fetch") {
        // Each time the prefetch worker checks if it was discarded (which is happening every time he wants to sync its status with the manager),
        // we read the whole buffer.
        // This should result in the buffer being continuously read as the fetch command is running, so we should read the whole input in a single fetch call.
        mp.worker_job.discard_check_callback = [&] {
            read_whole_buffer();
        };

        // Restart the prefetch
        mp.start(p.filename(), {});
        mp.issue_fetch();

        // After flushing the last command ,there should be the last discard check when reporting the EOF. So we should have read everything now.
        REQUIRE(status == S::end_of_file);
    }
}

TEST_CASE("media_prefetch::command_buffer_overflow_text") {
    StubGcodeProviderMemory p;
    MediaPrefetchManager mp;

    const std::string long_gcode = "M28 And this is a very long gcode without a comment, it should get cropped eventually blah blah blah";
    const std::string long_cropped_gcode = long_gcode.substr(0, MAX_CMD_SIZE - 1);

    const std::string long_gcode_with_comment_removed = "G36X86";
    const std::string long_gcode_with_comment = long_gcode_with_comment_removed + "; And this is a comment that makes the gcode longer than the buffer blah blah blah";

    const std::string short_gcode = "A111";

    p.add_gcode(long_gcode);
    p.add_gcode(long_gcode_with_comment);
    p.add_gcode(short_gcode);

    mp.start(p.filename(), {});
    mp.issue_fetch();

    REQUIRE(read_gcode(mp, true) == long_cropped_gcode);
    REQUIRE(read_gcode(mp) == long_gcode_with_comment_removed);
    REQUIRE(read_gcode(mp) == short_gcode);

    RR r;
    CHECK(mp.read_command(r) == S::end_of_file);
}

TEST_CASE("media_prefetch::compacting") {
    const auto test_compacting = [](const char *input, const char *expected_output = nullptr) {
        if (!expected_output) {
            expected_output = input;
        }

        CAPTURE(input, expected_output);

        std::array<char, 96> buffer;
        strcpy(buffer.data(), input);
        const auto compacted_len = compact_gcode(buffer.data());

        CAPTURE(buffer.data(), compacted_len);

        REQUIRE(compacted_len == strlen(buffer.data()));
        REQUIRE(std::string(buffer.data()) == expected_output);

        // Subsequent compacting should do nothing
        std::array<char, 96> buffer2 = buffer;
        compact_gcode(buffer2.data());
        REQUIRE(buffer == buffer2);

        return compacted_len;
    };

    test_compacting("G1");
    test_compacting(" G1   X8 Z-3.3", "G1X8Z-3.3");
    test_compacting("", "");
    test_compacting("  ; test", "");
    test_compacting("; test", "");
    test_compacting(" M123 X Y Z", "M123 X Y Z"); // Not a Gx gcode, only skip initial spaces
    test_compacting("G1 Z5.015 X111.804 Y100.712 E0.00820 ;tt", "G1Z5.015X111.804Y100.712E0.00820");

    test_compacting("  ; test ccomment", "");
    test_compacting("  G0 X; test ccomment", "G0X");
    test_compacting("  M52 This is ; a test", "M52 This is ; a test"); // Keep comments for non-Gx gcodes, it might be not a comment
}

TEST_CASE("media_prefetch::compression") {
    const auto test_compression = [](const char *input) {
        std::array<uint8_t, 96> compressed_data { 0 };
        const auto compressed_len = compress_gcode(input, compressed_data);
        REQUIRE(compressed_len);
        REQUIRE(*compressed_len <= strlen(input));

        std::array<char, 96> decompressed_data { 0 };
        decompress_gcode(compressed_data.data(), *compressed_len, decompressed_data);
        REQUIRE(std::string(decompressed_data.data()) == input);
        return *compressed_len;
    };

    test_compression("G1");
    test_compression(" G1   X8 Z-3.3");
    test_compression("");
    test_compression(" M123 X Y Z");
    test_compression("G1X8Z-3.3");

    // Check that we have a decent compressio ratio - the decompressed string is 25 characters btw
    REQUIRE(test_compression("G1X120.414Y108.407E.00937") == 11);

    // Test unsupported symbols - should fail
    {
        std::array<uint8_t, 96> compressed_data { 0 };
        REQUIRE(!compress_gcode("ŽŽŽ", compressed_data));
    }

    // Test output buffer overflow - compress should fail
    {
        std::array<uint8_t, 3> compressed_data { 0 };
        REQUIRE(!compress_gcode("G1X0Y0000", compressed_data));
    }
}

TEST_CASE("media_prefetch::compression::deployment") {
    size_t cnt = 0;
    for (const auto &entry : std::filesystem::directory_iterator("data/compression_ratio")) {
        StubGcodeProviderMemory p;
        MediaPrefetchManager mp;

        CAPTURE(entry.path());
        std::ifstream f(entry.path(), std::ios::in | std::ios::ate);
        REQUIRE(f.is_open());

        const auto file_size = f.tellg();
        std::string str(file_size, '\0'); // construct string to stream size
        f.seekg(0);
        REQUIRE(f.read(&str[0], file_size));

        p.add_gcode(str);
        mp.start(p.filename(), {});
        mp.issue_fetch();

        const float compression_ratio = mp.shared_state.read_tail.buffer_pos / float(mp.shared_state.read_tail.gcode_pos.offset);

        // Check that we have a passable compression ratio
        CHECK(compression_ratio < 0.62f);

        cnt++;
    }

    REQUIRE(cnt > 0);
}
