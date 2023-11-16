#include "gcode_reader.hpp"
#include "catch2/catch.hpp"

#include <iostream>
#include <fstream>
#include <sys/stat.h>

constexpr static const char *PLAIN_TEST_FILE = "test_plain.gcode";
constexpr static const char *BINARY_NO_COMPRESSION_FILE = "test_binary_no_compression.bgcode";
constexpr static const char *BINARY_MEATPACK_FILE = "test_binary_meatpack.bgcode";
constexpr static const char *BINARY_HEATSHRINK_FILE = "test_binary_heatshrink.bgcode";
constexpr static const char *BINARY_HEATSHRINK_MEATPACK_FILE = "test_binary_heatshrink_meatpack.bgcode";

const std::vector<const char *> test_files = { PLAIN_TEST_FILE, BINARY_NO_COMPRESSION_FILE, BINARY_MEATPACK_FILE, BINARY_HEATSHRINK_FILE, BINARY_HEATSHRINK_MEATPACK_FILE };

using State = transfers::PartialFile::State;
using ValidPart = transfers::PartialFile::ValidPart;
using std::nullopt;
using std::string_view;

TEST_CASE("Extract data", "[GcodeReader]") {
    auto run_test = [](IGcodeReader *r, std::string base_name) {
        GcodeBuffer buffer;
        {
            REQUIRE(r->stream_metadata_start());
            std::ofstream fs(base_name + "-metadata.txt", std::ofstream::out);
            IGcodeReader::Result_t result;
            while ((result = r->stream_get_line(buffer)) == IGcodeReader::Result_t::RESULT_OK) {
                fs << buffer.line.begin << std::endl;
            }
            REQUIRE(result == IGcodeReader::Result_t::RESULT_EOF); // file was read fully without error
        }

        {
            REQUIRE(r->stream_gcode_start());
            std::ofstream fs(base_name + "-gcode.gcode", std::ofstream::out);
            IGcodeReader::Result_t result;
            while ((result = r->stream_get_line(buffer)) == IGcodeReader::Result_t::RESULT_OK) {
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
        REQUIRE(reader1.stream_gcode_start(0));
        size_t ctr = 0;

        PrusaPackGcodeReader::stream_restore_info_t restore_info;
        while (true) {
            auto size = sizes[ctr++ % std::size(sizes)]; // pick next size to read

            auto reader2_anyformat = AnyGcodeFormatReader(filename);
            auto reader2 = reader2_anyformat.get();
            auto reader2_pp = dynamic_cast<PrusaPackGcodeReader *>(reader2);
            if (reader2_pp) {
                reader2_pp->set_restore_info(restore_info);
            }
            REQUIRE(reader2->stream_gcode_start(offset));

            auto size1 = size;
            auto res1 = reader1.stream_get_block(buffer1.get(), size1);
            auto size2 = size;
            auto res2 = reader2->stream_get_block(buffer2.get(), size2);

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
            reader2->stream_get_block(buffer2.get(), size);

            if (reader2_pp) {
                restore_info = reader2_pp->get_restore_info();
            }
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
    REQUIRE(reader.get()->stream_gcode_start());
    REQUIRE(reader.get()->stream_get_line(buffer) == IGcodeReader::Result_t::RESULT_OK);

    // copy it elsewhere, and check that it can read file
    auto reader2 = std::move(reader); // move operator
    REQUIRE(reader2.is_open());
    REQUIRE(reader2.get() != nullptr);
    REQUIRE(reader2.get()->stream_gcode_start());
    REQUIRE(reader2.get()->stream_get_line(buffer) == IGcodeReader::Result_t::RESULT_OK);

    auto reader3(std::move(reader2)); // move constructor
    REQUIRE(reader3.is_open());
    REQUIRE(reader3.get() != nullptr);
    REQUIRE(reader3.get()->stream_gcode_start());
    REQUIRE(reader3.get()->stream_get_line(buffer) == IGcodeReader::Result_t::RESULT_OK);

    // but its not possible to read from original place
    REQUIRE(!reader.is_open());
    REQUIRE(!reader2.is_open());
}

TEST_CASE("validity-plain", "[GcodeReader]") {
    auto reader = AnyGcodeFormatReader(PLAIN_TEST_FILE);
    REQUIRE(reader.get() != nullptr);
    auto r = reader.get();

    struct stat st = {};
    REQUIRE(stat(PLAIN_TEST_FILE, &st) == 0);
    size_t size = st.st_size;
    r->set_validity(State { nullopt, nullopt, size });

    REQUIRE(r->stream_gcode_start());

    GcodeBuffer buffer;
    // Not available yet
    REQUIRE(r->stream_get_line(buffer) == IGcodeReader::Result_t::RESULT_OUT_OF_RANGE);
    r->set_validity(State { ValidPart(0, 0), nullopt, size });
    REQUIRE(r->stream_get_line(buffer) == IGcodeReader::Result_t::RESULT_OUT_OF_RANGE);
    r->set_validity(State { ValidPart(0, 1024), nullopt, size });
    REQUIRE(r->stream_get_line(buffer) == IGcodeReader::Result_t::RESULT_OK);

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
    REQUIRE(reader.get() != nullptr);
    auto r = reader.get();

    struct stat st = {};
    REQUIRE(stat(BINARY_HEATSHRINK_MEATPACK_FILE, &st) == 0);
    size_t size = st.st_size;

    GcodeBuffer buffer;
    // Not available yet
    r->set_validity(State { nullopt, nullopt, size });
    REQUIRE(r->stream_metadata_start() == false);
    REQUIRE(r->stream_gcode_start() == false);
    r->set_validity(State { ValidPart(0, 0), nullopt, size });
    REQUIRE(r->stream_metadata_start() == false);
    REQUIRE(r->stream_gcode_start() == false);

    // just printer metadata is valid
    r->set_validity(State { ValidPart(0, 613), nullopt, size });
    REQUIRE(r->stream_metadata_start());
    REQUIRE(r->stream_get_line(buffer) == IGcodeReader::Result_t::RESULT_OK);
    REQUIRE(r->stream_gcode_start() == false);

    // all metadata & first gcode block is valid
    r->set_validity(State { ValidPart(0, 119731), nullopt, size });
    REQUIRE(r->stream_metadata_start());
    REQUIRE(r->stream_get_line(buffer) == IGcodeReader::Result_t::RESULT_OK);
    REQUIRE(r->stream_gcode_start());

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
    REQUIRE(reader.get() != nullptr);
    auto r = reader.get();

    r->set_validity(State {});
    GcodeBuffer buffer;
    REQUIRE(r->stream_get_line(buffer) == IGcodeReader::Result_t::RESULT_OUT_OF_RANGE);
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
