#include <transfers/transfer.hpp>

#include "errno.h"
#include <catch2/catch.hpp>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <type_traits>

using namespace transfers;

TEST_CASE("Serialization and Deserialization of a Transfer", "[transfers]") {
    Monitor monitor;

    SECTION("serialization is successful") {
        // open a file to serialize to
        auto temp_file_path = std::tmpnam(nullptr);
        Monitor::Slot slot = monitor.allocate(Monitor::Type::Connect, "/test_path", 1024).value();
        auto temp_file = std::fopen(temp_file_path, "w+");

        // make request
        auto request = Download::Request(
            "http://test_host.com",
            8081,
            "/test_path",
            nullptr);

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
            REQUIRE(strcmp(restored_request->host, request.host) == 0);
            REQUIRE(strcmp(restored_request->url_path, request.url_path) == 0);
            REQUIRE(restored_request->port == request.port);
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
