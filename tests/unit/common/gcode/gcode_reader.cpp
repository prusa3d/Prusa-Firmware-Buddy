#include "gcode_reader_any.hpp"
#include "catch2/catch.hpp"

#include <deque>
#include <iostream>
#include <fstream>
#include <sys/stat.h>

namespace {

constexpr static const char *PLAIN_TEST_FILE = "test_plain.gcode";
constexpr static const char *BINARY_NO_COMPRESSION_FILE = "test_binary_no_compression.bgcode";
constexpr static const char *BINARY_MEATPACK_FILE = "test_binary_meatpack.bgcode";
constexpr static const char *BINARY_HEATSHRINK_FILE = "test_binary_heatshrink.bgcode";
constexpr static const char *BINARY_HEATSHRINK_MEATPACK_FILE = "test_binary_heatshrink_meatpack.bgcode";
// These are made from the test_binary_no_compression.bgcode by mangling a specific CRC.
// See the utils/crckill.

// A thumbnail with bad CRC
constexpr static const char *BINARY_BAD_CRC_INTRO = "test_bad_crc_intro.bgcode";
// The CRC on the first gcode block
constexpr static const char *BINARY_BAD_CRC_FIRST_GCODE = "test_bad_crc_first_gcode.bgcode";
// Some later gcode block
constexpr static const char *BINARY_BAD_CRC_OTHER_GCODE = "test_bad_crc_gcode.bgcode";

constexpr static const std::string_view DUMMY_DATA_LONG = "; Short line\n"
                                                          ";Long line012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789\n"
                                                          ";Another short line";
constexpr static const std::string_view DUMMY_DATA_EXACT = ";01234567890123456789012345678901234567890123456789012345678901234567890123456789\n"
                                                           ";Another line";
constexpr static const std::string_view DUMMY_DATA_EXACT_EOF = ";01234567890123456789012345678901234567890123456789012345678901234567890123456789";
constexpr static const std::string_view DUMMY_DATA_ERR = ";01234567890123456789012345678901234567890123456789012345678901234567890123456789012345";

const std::vector<const char *> test_files = { PLAIN_TEST_FILE, BINARY_NO_COMPRESSION_FILE, BINARY_MEATPACK_FILE, BINARY_HEATSHRINK_FILE, BINARY_HEATSHRINK_MEATPACK_FILE };

using State = transfers::PartialFile::State;
using ValidPart = transfers::PartialFile::ValidPart;
using std::nullopt;
using std::string_view;

IGcodeReader::Result_t stream_get_block(IGcodeReader &reader, char *target, size_t &size) {
    auto end = target + size;
    while (target != end) {
        const auto res = reader.stream_getc(*(target++));
        if (res != IGcodeReader::Result_t::RESULT_OK) {
            size -= (end - target);
            return res;
        }
    }
    return IGcodeReader::Result_t::RESULT_OK;
};

struct DummyReader : public GcodeReaderCommon {
    std::deque<char> data;
    Result_t final_result;

    DummyReader(const std::string_view &input, Result_t final_result)
        : data(input.begin(), input.end())
        , final_result(final_result) {
        // Grouchy Smurf: I hate pointer-to-member-function casts
        ptr_stream_getc = static_cast<stream_getc_type>(&DummyReader::dummy_getc);
    }

    virtual bool stream_metadata_start() override {
        return true;
    }

    virtual Result_t stream_gcode_start(uint32_t) override {
        return Result_t::RESULT_OK;
    }

    virtual bool stream_thumbnail_start(uint16_t, uint16_t, ImgType, bool) override {
        return true;
    }

    virtual uint32_t get_gcode_stream_size_estimate() override {
        return 0;
    }

    virtual uint32_t get_gcode_stream_size() override {
        return 0;
    }

    virtual FileVerificationResult verify_file(FileVerificationLevel, std::span<uint8_t>) const override {
        return FileVerificationResult { true };
    }

    virtual bool valid_for_print() override {
        return true;
    }

    virtual Result_t stream_get_line(GcodeBuffer &buffer, Continuations continuations) {
        return stream_get_line_common(buffer, continuations);
    }

    Result_t dummy_getc(char &out) {
        if (data.empty()) {
            return final_result;
        } else {
            out = data.front();
            data.pop_front();
            return Result_t::RESULT_OK;
        }
    }

    virtual StreamRestoreInfo get_restore_info() override { return {}; }

    virtual void set_restore_info(const StreamRestoreInfo &) override {}
};

} // namespace

TEST_CASE("Extract data", "[GcodeReader]") {
    auto run_test = [](IGcodeReader *r, std::string base_name) {
        GcodeBuffer buffer;
        {
            REQUIRE(r->stream_metadata_start());
            std::ofstream fs(base_name + "-metadata.txt", std::ofstream::out);
            IGcodeReader::Result_t result;
            while ((result = r->stream_get_line(buffer, IGcodeReader::Continuations::Discard)) == IGcodeReader::Result_t::RESULT_OK) {
                fs << buffer.line.begin << std::endl;
            }
            REQUIRE(result == IGcodeReader::Result_t::RESULT_EOF); // file was read fully without error
        }

        {
            REQUIRE(r->stream_gcode_start() == IGcodeReader::Result_t::RESULT_OK);
            std::ofstream fs(base_name + "-gcode.gcode", std::ofstream::out);
            IGcodeReader::Result_t result;
            while ((result = r->stream_get_line(buffer, IGcodeReader::Continuations::Discard)) == IGcodeReader::Result_t::RESULT_OK) {
                fs << buffer.line.begin << std::endl;
            }
            REQUIRE(result == IGcodeReader::Result_t::RESULT_EOF); // file was read fully without error
        }
        {
            REQUIRE(r->stream_thumbnail_start(440, 240, IGcodeReader::ImgType::PNG, false));
            std::ofstream fs(base_name + "-thumb.png", std::ofstream::out);
            char c;
            IGcodeReader::Result_t result;
            while ((result = r->stream_getc(c)) == IGcodeReader::Result_t::RESULT_OK) {
                fs << c;
            }
            REQUIRE(result == IGcodeReader::Result_t::RESULT_EOF); // file was read fully without error
        }
    };

    for (auto &filename : test_files) {
        SECTION(std::string("Test-file: ") + filename) {
            AnyGcodeFormatReader reader(filename);
            REQUIRE(reader.is_open());
            REQUIRE(reader.get()->verify_file(IGcodeReader::FileVerificationLevel::full));
            run_test(reader.get(), filename);
        }
    }
}

TEST_CASE("stream restore at offset", "[GcodeReader]") {
    // tests reads reader1 continuously, and seeks in reader2 to same position
    //  as where it is in reader1 and compares if they give same results while seeking to middle of file

    auto run_test = [](IGcodeReader &reader1, const char *filename) {
        const size_t sizes[] = { 101, 103, 107, 109, 113, 3037, 3041, 3049, 3061, 3067 };
        std::unique_ptr<char[]> buffer1(new char[*std::max_element(sizes, sizes + std::size(sizes))]);
        std::unique_ptr<char[]> buffer2(new char[*std::max_element(sizes, sizes + std::size(sizes))]);
        long unsigned int offset = 0;
        REQUIRE(reader1.stream_gcode_start(0) == IGcodeReader::Result_t::RESULT_OK);
        size_t ctr = 0;

        GCodeReaderStreamRestoreInfo restore_info;
        bool has_restore_info = false;
        while (true) {
            auto size = sizes[ctr++ % std::size(sizes)]; // pick next size to read

            auto reader2_anyformat = AnyGcodeFormatReader(filename);
            auto reader2 = reader2_anyformat.get();
            if (has_restore_info) {
                reader2->set_restore_info(restore_info);
            }
            REQUIRE(reader2->stream_gcode_start(offset) == IGcodeReader::Result_t::RESULT_OK);

            auto size1 = size;
            auto res1 = stream_get_block(reader1, buffer1.get(), size1);
            auto size2 = size;
            auto res2 = stream_get_block(*reader2, buffer2.get(), size2);

            REQUIRE(res1 == res2);
            REQUIRE(((res1 == IGcodeReader::Result_t::RESULT_EOF) || (res1 == IGcodeReader::Result_t::RESULT_OK)));
            if (res1 == IGcodeReader::Result_t::RESULT_OK) {
                // if read went OK, requested number of bytes have to be returned, not less
                REQUIRE(size == size1);
            }
            REQUIRE(size1 == size2);

            REQUIRE(memcmp(buffer1.get(), buffer2.get(), size1) == 0);

            if (res1 == IGcodeReader::Result_t::RESULT_EOF) {
                break;
            }
            REQUIRE(res1 == IGcodeReader::Result_t::RESULT_OK);

            offset += size;
            // read something from the buffer2, so that file position moves and we could see if stream_gcode_start doesn't return to correct position
            stream_get_block(*reader2, buffer2.get(), size);

            restore_info = reader2->get_restore_info();
            has_restore_info = true;
        }
    };

    for (auto &filename : test_files) {
        SECTION(std::string("Test-file: ") + filename) {
            auto reader1 = AnyGcodeFormatReader(filename);
            REQUIRE(reader1.is_open());
            run_test(*reader1.get(), filename);
        }
    }
}

TEST_CASE("copy & move operators", "[GcodeReader]") {
    GcodeBuffer buffer;

    // open file
    auto reader = AnyGcodeFormatReader(PLAIN_TEST_FILE);
    REQUIRE(reader.is_open());
    REQUIRE(reader.get() != nullptr);
    REQUIRE(reader.get()->stream_gcode_start() == IGcodeReader::Result_t::RESULT_OK);
    REQUIRE(reader.get()->stream_get_line(buffer, IGcodeReader::Continuations::Discard) == IGcodeReader::Result_t::RESULT_OK);

    // copy it elsewhere, and check that it can read file
    auto reader2 = std::move(reader); // move operator
    REQUIRE(reader2.is_open());
    REQUIRE(reader2.get() != nullptr);
    REQUIRE(reader2.get()->stream_gcode_start() == IGcodeReader::Result_t::RESULT_OK);
    REQUIRE(reader2.get()->stream_get_line(buffer, IGcodeReader::Continuations::Discard) == IGcodeReader::Result_t::RESULT_OK);

    auto reader3(std::move(reader2)); // move constructor
    REQUIRE(reader3.is_open());
    REQUIRE(reader3.get() != nullptr);
    REQUIRE(reader3.get()->stream_gcode_start() == IGcodeReader::Result_t::RESULT_OK);
    REQUIRE(reader3.get()->stream_get_line(buffer, IGcodeReader::Continuations::Discard) == IGcodeReader::Result_t::RESULT_OK);

    // but its not possible to read from original place
    REQUIRE(!reader.is_open());
    REQUIRE(!reader2.is_open());
}

TEST_CASE("validity-plain", "[GcodeReader]") {
    auto reader = AnyGcodeFormatReader(PLAIN_TEST_FILE);
    auto r = dynamic_cast<PlainGcodeReader *>(reader.get());
    REQUIRE(r != nullptr);

    struct stat st = {};
    REQUIRE(stat(PLAIN_TEST_FILE, &st) == 0);
    size_t size = st.st_size;
    r->set_validity(State { nullopt, nullopt, size });

    REQUIRE(r->stream_gcode_start() == IGcodeReader::Result_t::RESULT_OK);

    GcodeBuffer buffer;
    // Not available yet
    REQUIRE(r->stream_get_line(buffer, IGcodeReader::Continuations::Discard) == IGcodeReader::Result_t::RESULT_OUT_OF_RANGE);
    r->set_validity(State { ValidPart(0, 0), nullopt, size });
    REQUIRE(r->stream_get_line(buffer, IGcodeReader::Continuations::Discard) == IGcodeReader::Result_t::RESULT_OUT_OF_RANGE);
    r->set_validity(State { ValidPart(0, 1024), nullopt, size });
    REQUIRE(r->stream_get_line(buffer, IGcodeReader::Continuations::Discard) == IGcodeReader::Result_t::RESULT_OK);

    size_t len = buffer.line.end - buffer.line.begin;
    auto f = unique_file_ptr(fopen(PLAIN_TEST_FILE, "r"));
    char buff_exp[len];
    REQUIRE(fread(buff_exp, len, 1, f.get()) == 1);

    // stream_get_line zero terminates, so we do the same so it compares OK
    buff_exp[len] = '\0';
    REQUIRE(string_view(buffer.line.begin, buffer.line.end) == string_view(buff_exp, buff_exp + len));
}

TEST_CASE("validity-bgcode", "[GcodeReader]") {
    auto reader = AnyGcodeFormatReader(BINARY_HEATSHRINK_MEATPACK_FILE);
    auto r = dynamic_cast<PrusaPackGcodeReader *>(reader.get());
    REQUIRE(r != nullptr);

    struct stat st = {};
    REQUIRE(stat(BINARY_HEATSHRINK_MEATPACK_FILE, &st) == 0);
    size_t size = st.st_size;

    GcodeBuffer buffer;
    // Not available yet
    r->set_validity(State { nullopt, nullopt, size });
    REQUIRE(r->stream_metadata_start() == false);
    REQUIRE(r->stream_gcode_start() == IGcodeReader::Result_t::RESULT_OUT_OF_RANGE);
    r->set_validity(State { ValidPart(0, 0), nullopt, size });
    REQUIRE(r->stream_metadata_start() == false);
    REQUIRE(r->stream_gcode_start() == IGcodeReader::Result_t::RESULT_OUT_OF_RANGE);

    // just printer metadata is valid
    r->set_validity(State { ValidPart(0, 613), nullopt, size });
    REQUIRE(r->stream_metadata_start());
    REQUIRE(r->stream_get_line(buffer, IGcodeReader::Continuations::Discard) == IGcodeReader::Result_t::RESULT_OK);
    REQUIRE(r->stream_gcode_start() == IGcodeReader::Result_t::RESULT_OUT_OF_RANGE);

    // all metadata & first gcode block is valid
    r->set_validity(State { ValidPart(0, 119731), nullopt, size });
    REQUIRE(r->stream_metadata_start());
    REQUIRE(r->stream_get_line(buffer, IGcodeReader::Continuations::Discard) == IGcodeReader::Result_t::RESULT_OK);
    REQUIRE(r->stream_gcode_start() == IGcodeReader::Result_t::RESULT_OK);

    // read entire first block,, that should go fine, then it shoudl return OUT_OF_RANGE on first character on next block
    size_t first_block_size = 59693;
    char c;
    while (first_block_size--) {
        REQUIRE(r->stream_getc(c) == IGcodeReader::Result_t::RESULT_OK);
    }
    REQUIRE(r->stream_getc(c) == IGcodeReader::Result_t::RESULT_OUT_OF_RANGE);
}

TEST_CASE("gcode-reader-empty-validity", "[GcodeReader]") {
    // Test the "empty validity" (which, implicitly has size = 0, even if the
    // file should be considered bigger) prevents reading from it even though
    // the ranges are capped. Basically, checking interaction of internal
    // implementation nuances don't change and the default State prevents
    // reading from the file no matter what.
    auto reader = AnyGcodeFormatReader(PLAIN_TEST_FILE);
    auto r = dynamic_cast<PlainGcodeReader *>(reader.get());
    REQUIRE(r != nullptr);

    r->set_validity(State {});
    GcodeBuffer buffer;
    REQUIRE(r->stream_get_line(buffer, IGcodeReader::Continuations::Discard) == IGcodeReader::Result_t::RESULT_OUT_OF_RANGE);
}

TEST_CASE("File size estimate", "[GcodeReader]") {
    for (auto &filename : test_files) {
        SECTION(std::string("Test-file: ") + filename) {
            auto reader = AnyGcodeFormatReader(filename);
            auto estimate = reader.get()->get_gcode_stream_size_estimate();
            auto real = reader.get()->get_gcode_stream_size();
            float ratio = (float)estimate / real;
            std::cout << "Real: " << real << ", estimate: " << estimate << ", ratio: " << ratio << std::endl;
            REQUIRE_THAT(ratio, Catch::Matchers::WithinAbs(1, 0.1));
        }
    }
}

TEST_CASE("Reader: Long comment, split") {
    DummyReader reader(DUMMY_DATA_LONG, IGcodeReader::Result_t::RESULT_EOF);
    GcodeBuffer buffer;

    REQUIRE(reader.stream_get_line(buffer, IGcodeReader::Continuations::Split) == IGcodeReader::Result_t::RESULT_OK);
    REQUIRE(buffer.line == "; Short line");
    // Checking both, because len bases it on end-begin, strlen on \0 position
    REQUIRE(buffer.line.len() == 12);
    REQUIRE(strlen(buffer.line.c_str()) == 12);
    REQUIRE(buffer.line_complete);

    REQUIRE(reader.stream_get_line(buffer, IGcodeReader::Continuations::Split) == IGcodeReader::Result_t::RESULT_OK);
    REQUIRE(buffer.line == ";Long line01234567890123456789012345678901234567890123456789012345678901234567890");
    // Note: In the split mode, it is _not_ \0 terminated here.
    // Therefore, no strlen and using all 81 characters.
    REQUIRE(buffer.line.len() == 81);
    REQUIRE_FALSE(buffer.line_complete);

    // The continuation
    REQUIRE(reader.stream_get_line(buffer, IGcodeReader::Continuations::Split) == IGcodeReader::Result_t::RESULT_OK);
    REQUIRE(buffer.line == "1234567890123456789012345678901234567890123456789");
    REQUIRE(buffer.line.len() == 49);
    REQUIRE(strlen(buffer.line.c_str()) == 49);
    REQUIRE(buffer.line_complete);

    REQUIRE(reader.stream_get_line(buffer, IGcodeReader::Continuations::Split) == IGcodeReader::Result_t::RESULT_OK);
    REQUIRE(buffer.line == ";Another short line");
    REQUIRE(buffer.line.len() == 19);
    REQUIRE(strlen(buffer.line.c_str()) == 19);
    REQUIRE(buffer.line_complete);

    REQUIRE(reader.stream_get_line(buffer, IGcodeReader::Continuations::Split) == IGcodeReader::Result_t::RESULT_EOF);
}

TEST_CASE("Reader: Long comment, discard") {
    DummyReader reader(DUMMY_DATA_LONG, IGcodeReader::Result_t::RESULT_EOF);
    GcodeBuffer buffer;

    REQUIRE(reader.stream_get_line(buffer, IGcodeReader::Continuations::Discard) == IGcodeReader::Result_t::RESULT_OK);
    REQUIRE(buffer.line == "; Short line");
    // Checking both, because len bases it on end-begin, strlen on \0 position
    REQUIRE(buffer.line.len() == 12);
    REQUIRE(strlen(buffer.line.c_str()) == 12);
    REQUIRE(buffer.line_complete);

    REQUIRE(reader.stream_get_line(buffer, IGcodeReader::Continuations::Discard) == IGcodeReader::Result_t::RESULT_OK);
    REQUIRE(buffer.line == ";Long line0123456789012345678901234567890123456789012345678901234567890123456789");
    REQUIRE(buffer.line.len() == 80);
    REQUIRE(strlen(buffer.line.c_str()) == 80);
    REQUIRE_FALSE(buffer.line_complete);

    // The continuation is not present

    REQUIRE(reader.stream_get_line(buffer, IGcodeReader::Continuations::Discard) == IGcodeReader::Result_t::RESULT_OK);
    REQUIRE(buffer.line == ";Another short line");
    REQUIRE(buffer.line.len() == 19);
    REQUIRE(strlen(buffer.line.c_str()) == 19);
    REQUIRE(buffer.line_complete);

    REQUIRE(reader.stream_get_line(buffer, IGcodeReader::Continuations::Discard) == IGcodeReader::Result_t::RESULT_EOF);
}

TEST_CASE("Reader: Exact long, split") {
    DummyReader reader(DUMMY_DATA_EXACT, IGcodeReader::Result_t::RESULT_EOF);
    GcodeBuffer buffer;

    // The first line fits exactly. But the reader doesn't know it ended.
    REQUIRE(reader.stream_get_line(buffer, IGcodeReader::Continuations::Split) == IGcodeReader::Result_t::RESULT_OK);
    REQUIRE(buffer.line == ";01234567890123456789012345678901234567890123456789012345678901234567890123456789");
    REQUIRE(buffer.line.len() == 81);
    REQUIRE_FALSE(buffer.line_complete);

    // There's an empty continuation to mark it is complete
    REQUIRE(reader.stream_get_line(buffer, IGcodeReader::Continuations::Split) == IGcodeReader::Result_t::RESULT_OK);
    REQUIRE(buffer.line.is_empty());
    REQUIRE(buffer.line_complete);

    // Then the rest can be read
    REQUIRE(reader.stream_get_line(buffer, IGcodeReader::Continuations::Split) == IGcodeReader::Result_t::RESULT_OK);
    REQUIRE(buffer.line == ";Another line");
    REQUIRE(buffer.line_complete);

    REQUIRE(reader.stream_get_line(buffer, IGcodeReader::Continuations::Split) == IGcodeReader::Result_t::RESULT_EOF);
}

TEST_CASE("Reader: Exact long, discard") {
    DummyReader reader(DUMMY_DATA_EXACT, IGcodeReader::Result_t::RESULT_EOF);
    GcodeBuffer buffer;

    // The first line fits exactly. But the reader doesn't know it ended.
    REQUIRE(reader.stream_get_line(buffer, IGcodeReader::Continuations::Discard) == IGcodeReader::Result_t::RESULT_OK);
    REQUIRE(buffer.line == ";0123456789012345678901234567890123456789012345678901234567890123456789012345678");
    REQUIRE(buffer.line.len() == 80);
    REQUIRE_FALSE(buffer.line_complete);

    REQUIRE(reader.stream_get_line(buffer, IGcodeReader::Continuations::Discard) == IGcodeReader::Result_t::RESULT_OK);
    REQUIRE(buffer.line == ";Another line");
    REQUIRE(buffer.line_complete);

    REQUIRE(reader.stream_get_line(buffer, IGcodeReader::Continuations::Discard) == IGcodeReader::Result_t::RESULT_EOF);
}

TEST_CASE("Reader: Exact at EOF, split") {
    DummyReader reader(DUMMY_DATA_EXACT_EOF, IGcodeReader::Result_t::RESULT_EOF);
    GcodeBuffer buffer;

    // The first line fits exactly. But the reader doesn't know it ended.
    REQUIRE(reader.stream_get_line(buffer, IGcodeReader::Continuations::Split) == IGcodeReader::Result_t::RESULT_OK);
    REQUIRE(buffer.line == ";01234567890123456789012345678901234567890123456789012345678901234567890123456789");
    REQUIRE(buffer.line.len() == 81);
    REQUIRE_FALSE(buffer.line_complete);

    // There's an empty continuation to mark it is complete
    REQUIRE(reader.stream_get_line(buffer, IGcodeReader::Continuations::Split) == IGcodeReader::Result_t::RESULT_OK);
    REQUIRE(buffer.line.is_empty());
    REQUIRE(buffer.line_complete);

    REQUIRE(reader.stream_get_line(buffer, IGcodeReader::Continuations::Split) == IGcodeReader::Result_t::RESULT_EOF);
}

TEST_CASE("Reader: Exact at EOF, discard") {
    DummyReader reader(DUMMY_DATA_EXACT_EOF, IGcodeReader::Result_t::RESULT_EOF);
    GcodeBuffer buffer;

    // The first line fits exactly. But the reader doesn't know it ended.
    REQUIRE(reader.stream_get_line(buffer, IGcodeReader::Continuations::Discard) == IGcodeReader::Result_t::RESULT_OK);
    REQUIRE(buffer.line == ";0123456789012345678901234567890123456789012345678901234567890123456789012345678");
    REQUIRE(buffer.line.len() == 80);
    REQUIRE_FALSE(buffer.line_complete);

    REQUIRE(reader.stream_get_line(buffer, IGcodeReader::Continuations::Discard) == IGcodeReader::Result_t::RESULT_EOF);
}

TEST_CASE("Reader: Error in long, split") {
    DummyReader reader(DUMMY_DATA_ERR, IGcodeReader::Result_t::RESULT_ERROR);
    GcodeBuffer buffer;

    // The first line fits exactly. But the reader doesn't know it ended.
    REQUIRE(reader.stream_get_line(buffer, IGcodeReader::Continuations::Split) == IGcodeReader::Result_t::RESULT_OK);
    REQUIRE(buffer.line == ";01234567890123456789012345678901234567890123456789012345678901234567890123456789");
    REQUIRE(buffer.line.len() == 81);
    REQUIRE_FALSE(buffer.line_complete);

    // Error reading the continuation.
    REQUIRE(reader.stream_get_line(buffer, IGcodeReader::Continuations::Discard) == IGcodeReader::Result_t::RESULT_ERROR);
}

TEST_CASE("Reader: Error in long, discard") {
    DummyReader reader(DUMMY_DATA_ERR, IGcodeReader::Result_t::RESULT_ERROR);
    GcodeBuffer buffer;

    // The first line fits exactly. But the reader doesn't know it ended.
    REQUIRE(reader.stream_get_line(buffer, IGcodeReader::Continuations::Discard) == IGcodeReader::Result_t::RESULT_OK);
    REQUIRE(buffer.line == ";0123456789012345678901234567890123456789012345678901234567890123456789012345678");
    REQUIRE(buffer.line.len() == 80);
    REQUIRE_FALSE(buffer.line_complete);

    // Interestingly, this is not when reading the end of the line, but reading
    // the next line.. but it still results in ERROR.
    REQUIRE(reader.stream_get_line(buffer, IGcodeReader::Continuations::Discard) == IGcodeReader::Result_t::RESULT_ERROR);
}

TEST_CASE("Reader CRC: incorrect before gcode") {
    AnyGcodeFormatReader reader("test_bad_crc_intro.bgcode");
    REQUIRE(reader.is_open());
    REQUIRE(reader.get()->stream_gcode_start() == IGcodeReader::Result_t::RESULT_CORRUPT);
}

TEST_CASE("Reader CRC: incorrect on first gcode") {
    AnyGcodeFormatReader reader("test_bad_crc_first_gcode.bgcode");
    REQUIRE(reader.is_open());
    // The first gcode block is checked during the start
    REQUIRE(reader.get()->stream_gcode_start() == IGcodeReader::Result_t::RESULT_CORRUPT);
}

TEST_CASE("Reader CRC: incorrect on another gcode") {
    AnyGcodeFormatReader reader("test_bad_crc_gcode.bgcode");
    REQUIRE(reader.is_open());
    // This checks only the beginning, not the whole gcode and so far we didn't find the "broken" part yet.
    REQUIRE(reader.get()->stream_gcode_start() == IGcodeReader::Result_t::RESULT_OK);

    char buffer[128];
    IGcodeReader::Result_t result = IGcodeReader::Result_t::RESULT_OK;

    while (result == IGcodeReader::Result_t::RESULT_OK) {
        size_t size = sizeof buffer;
        result = stream_get_block(*reader, buffer, size);
    }

    // We finish by finding a corruption, not running until the very end.
    REQUIRE(result == IGcodeReader::Result_t::RESULT_CORRUPT);
}
