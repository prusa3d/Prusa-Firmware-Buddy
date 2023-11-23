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
        auto headers_count = GENERATE(0, 3);

        // make request
        auto request = Download::Request(
            "http://test_host.com",
            8081,
            "/test_path",
            [headers_count](size_t headers_size, http::HeaderOut *headers) -> size_t {
                size_t required = headers_count;
                if (headers_size >= required && required) {
                    headers[0] = http::HeaderOut { "Upgrade-Insecure-Requests", static_cast<size_t>(1), std::nullopt };
                    headers[1] = http::HeaderOut { "Connection", "keep-alive", std::nullopt };
                    headers[2] = http::HeaderOut { "Cache-Control", "max-age=0", std::optional<size_t>(1) };
                }
                return required;
            },
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

            auto headers_cnt = request.extra_headers(0, nullptr);
            REQUIRE(restored_request->extra_headers(0, nullptr) == headers_cnt);

            auto original_headers = std::make_unique<http::HeaderOut[]>(headers_cnt + 1);
            auto restored_headers = std::make_unique<http::HeaderOut[]>(headers_cnt + 1);

            request.extra_headers(headers_cnt, original_headers.get());
            original_headers[headers_cnt] = http::HeaderOut { nullptr, nullptr, std::nullopt };
            restored_request->extra_headers(headers_cnt, restored_headers.get());
            restored_headers[headers_cnt] = http::HeaderOut { nullptr, nullptr, std::nullopt };

            for (size_t i = 0; i < headers_cnt; ++i) {
                REQUIRE(strcmp(original_headers[i].name, restored_headers[i].name) == 0);
                std::visit([&](auto &&arg) {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, const char *>) {
                        auto recovered = std::get<const char *>(restored_headers[i].value);
                        REQUIRE(strcmp(arg, recovered) == 0);
                    } else if constexpr (std::is_same_v<T, size_t>) {
                        REQUIRE(arg == std::get<size_t>(restored_headers[i].value));
                    }
                },
                    restored_headers[i].value);
                if (!original_headers[i].size_limit.has_value()) {
                    INFO("Restored: " << restored_headers[i].size_limit.value_or(42));
                }
                REQUIRE(original_headers[i].size_limit == restored_headers[i].size_limit);
            }
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
