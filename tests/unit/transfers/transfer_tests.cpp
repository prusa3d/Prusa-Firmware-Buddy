#include <transfers/transfer.hpp>
#include <transfers/transfer_recovery_private.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "errno.h"
#include <catch2/catch_test_macros.hpp>
#include <common/crc32.h>
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <limits>
#include <optional>
#include <type_traits>
#include <vector>

using namespace transfers;
using std::get_if;

namespace {

// Holds the header and raw string data of a backup file, so tests can
// hand-craft corrupted backups. Assumes header and data lengths stay fixed
// (only header fields are tampered with, not string bytes).
struct BackupFile {
    SerializedTransfer header;
    std::vector<uint8_t> data; // raw string bytes after the header, without the trailing crc

    static BackupFile read(FILE *file) {
        REQUIRE(fseek(file, 0, SEEK_END) == 0);
        long total = ftell(file);
        REQUIRE(total >= static_cast<long>(sizeof(SerializedTransfer) + sizeof(uint32_t)));
        REQUIRE(fseek(file, 0, SEEK_SET) == 0);

        BackupFile result;
        REQUIRE(fread(&result.header, sizeof(result.header), 1, file) == 1);
        const size_t data_size = static_cast<size_t>(total) - sizeof(SerializedTransfer) - sizeof(uint32_t);
        result.data.resize(data_size);
        if (data_size > 0) {
            REQUIRE(fread(result.data.data(), data_size, 1, file) == 1);
        }
        return result;
    }

    void write(FILE *file) {
        REQUIRE(fseek(file, 0, SEEK_SET) == 0);
        REQUIRE(fwrite(&header, sizeof(header), 1, file) == 1);
        if (!data.empty()) {
            REQUIRE(fwrite(data.data(), data.size(), 1, file) == 1);
        }
        const uint32_t crc = compute_crc();
        REQUIRE(fwrite(&crc, sizeof(crc), 1, file) == 1);
    }

private:
    uint32_t compute_crc() const {
        const size_t start = offsetof(SerializedTransfer, fw_version);
        uint32_t crc = crc32_calc_ex(0, reinterpret_cast<const uint8_t *>(&header) + start, sizeof(header) - start);
        crc = crc32_calc_ex(crc, data.data(), data.size());
        return crc;
    }
};

} // namespace

TEST_CASE("Serialization and Deserialization of a Transfer", "[transfers]") {
    Monitor monitor;

    SECTION("serialization is successful") {
        // open a file to serialize to
        auto temp_file_path = std::tmpnam(nullptr);
        Monitor::Slot slot = monitor.allocate(Monitor::Type::Connect, "/test_path", 1024).value();
        auto temp_file = std::fopen(temp_file_path, "w+");

        // make request
        auto request = Download::Request("test_hash", 42, 1024);

        // make partial file state
        auto partial_file_state = transfers::PartialFile::State();
        partial_file_state.valid_head = GENERATE(std::optional<transfers::PartialFile::ValidPart>(std::nullopt), transfers::PartialFile::ValidPart { 0, 100 });
        partial_file_state.valid_tail = GENERATE(std::optional<transfers::PartialFile::ValidPart>(std::nullopt), transfers::PartialFile::ValidPart { 200, 300 });

        // make a backup
        REQUIRE(Transfer::make_backup(temp_file, request, partial_file_state, slot) == true);
        std::cout << "temp_file_path: " << temp_file_path << std::endl;

        SECTION("Deserialization has the right data") {
            auto restored = Transfer::restore(temp_file);
            REQUIRE(restored.has_value());

            auto restored_partial_file_state = restored->get_partial_file_state();
            REQUIRE(memcmp(&restored_partial_file_state, &partial_file_state, sizeof(PartialFile::State)) == 0);

            auto restored_request = restored->get_download_request();
            REQUIRE(restored_request.has_value());
            auto in = get_if<Download::Request::Inline>(&request.data);
            REQUIRE(in != nullptr);
            auto restored_in = get_if<Download::Request::Inline>(&restored_request->data);
            REQUIRE(restored_in != nullptr);
            REQUIRE(strcmp(restored_in->hash, in->hash) == 0);
            REQUIRE(restored_in->team_id == in->team_id);
            REQUIRE(restored_in->orig_size == in->orig_size);
        }

        SECTION("Update of download progress and deserialization still has the right data") {
            auto updated_partial_file_state = PartialFile::State { PartialFile::ValidPart { 10, 20 }, std::nullopt };
            Transfer::update_backup(temp_file, updated_partial_file_state);

            auto restored = Transfer::restore(temp_file);
            REQUIRE(restored.has_value());

            auto restored_partial_file_state = restored->get_partial_file_state();
            REQUIRE(memcmp(&restored_partial_file_state, &updated_partial_file_state, sizeof(PartialFile::State)) == 0);
        }
    }
}

TEST_CASE("Malformed backup files are rejected", "[transfers]") {
    Monitor monitor;
    char *temp_file_path = std::tmpnam(nullptr);
    Monitor::Slot slot = monitor.allocate(Monitor::Type::Connect, "/test_path", 1024).value();
    unique_file_ptr temp_file(fopen(temp_file_path, "w+"));

    Download::Request request("test_hash", 42, 1024);
    PartialFile::State partial_file_state;

    REQUIRE(Transfer::make_backup(temp_file.get(), request, partial_file_state, slot) == true);

    SECTION("Negative offset is rejected") {
        BackupFile backup = BackupFile::read(temp_file.get());
        backup.header.url_path_or_hash.offset = -256;
        backup.write(temp_file.get());

        std::optional<Transfer::RestoredTransfer> restored = Transfer::restore(temp_file.get());
        REQUIRE(restored.has_value());
        REQUIRE(!restored->get_download_request().has_value());
    }

    SECTION("Size running off the end (wrapping) is rejected") {
        BackupFile backup = BackupFile::read(temp_file.get());
        const size_t offset = static_cast<size_t>(backup.header.url_path_or_hash.offset);
        // offset + size wraps around size_t back to a tiny, in-range-looking value.
        backup.header.url_path_or_hash.size = std::numeric_limits<size_t>::max() - offset + 2;
        backup.write(temp_file.get());

        std::optional<Transfer::RestoredTransfer> restored = Transfer::restore(temp_file.get());
        REQUIRE(restored.has_value());
        REQUIRE(!restored->get_download_request().has_value());
    }

    SECTION("Unterminated string within bounds is rejected") {
        BackupFile backup = BackupFile::read(temp_file.get());
        // Point the hash at a single, non-NUL byte inside the fw_version string.
        backup.header.url_path_or_hash.offset = backup.header.fw_version.offset;
        backup.header.url_path_or_hash.size = 1;
        backup.write(temp_file.get());

        std::optional<Transfer::RestoredTransfer> restored = Transfer::restore(temp_file.get());
        REQUIRE(restored.has_value());
        REQUIRE(!restored->get_download_request().has_value());
    }

    SECTION("Zero size is rejected") {
        BackupFile backup = BackupFile::read(temp_file.get());
        backup.header.url_path_or_hash.size = 0;
        backup.write(temp_file.get());

        std::optional<Transfer::RestoredTransfer> restored = Transfer::restore(temp_file.get());
        REQUIRE(restored.has_value());
        REQUIRE(!restored->get_download_request().has_value());
    }
}
