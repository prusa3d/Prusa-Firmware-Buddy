#pragma once

#include "types.hpp"

#include <cstdint>
#include <utils/byte_utils.hpp>
#include <variant>

namespace nfcv {

namespace command {
    struct Inventory {
        static constexpr std::byte cmd_id { 0x01 };
        struct Request {
            inline bool operator==(const Request &o) const = default;
        } request;
        using Response = UID;
        Response &response;
    };

    struct SystemInfo {
        static constexpr std::byte cmd_id { 0x2B };
        struct Request {
            UID uid;

            inline bool operator==(const Request &o) const = default;
        } request;
        using Response = TagInfo;
        Response &response;
    };

    struct ReadSingleBlock {
        static constexpr std::byte cmd_id { 0x20 };
        struct Request {
            UID uid;
            uint8_t block_address;

            inline bool operator==(const Request &o) const = default;
        } request;
        using Response = const WritableBytes;
        Response response;
    };

    struct WriteSingleBlock {
        static constexpr std::byte cmd_id { 0x21 };
        static constexpr bool is_write_alike = true;
        struct Request {
            UID uid;
            uint8_t block_address;
            Bytes block_buffer;
        } request;
        struct Response {
        } response;
    };

    struct StayQuiet {
        static constexpr std::byte cmd_id { 0x02 };
        struct Request {
            UID uid;

            inline bool operator==(const Request &o) const = default;
        } request = {};
        // No repsonse expected
    };

    struct WriteAFI {
        static constexpr std::byte cmd_id { 0x27 };
        static constexpr bool is_write_alike = true;
        struct Request {
            UID uid;
            AFI afi;

            inline bool operator==(const Request &o) const = default;
        } request;
        struct Response {
        } response;
    };

    struct WriteDSFID {
        static constexpr std::byte cmd_id { 0x29 };
        static constexpr bool is_write_alike = true;
        struct Request {
            UID uid;
            DSFID dsfid;

            inline bool operator==(const Request &o) const = default;
        } request;
        struct Response {
        } response;
    };

    struct LockDSFID {
        static constexpr std::byte cmd_id { 0x2A };
        static constexpr bool is_write_alike = true;
        struct Request {
            UID uid;

            inline bool operator==(const Request &o) const = default;
        } request;
        struct Response {
        } response;
    };

    ///* SLIX2 extension
    struct SetEAS {
        static constexpr std::byte cmd_id { 0xA2 };
        static constexpr bool is_write_alike = true;
        struct Request {
            UID uid;

            inline bool operator==(const Request &o) const = default;
        } request;
        struct Response {
        } response;
    };

    ///* SLIX2 extension
    struct ResetEAS {
        static constexpr std::byte cmd_id { 0xA3 };
        static constexpr bool is_write_alike = true;
        struct Request {
            UID uid;

            inline bool operator==(const Request &o) const = default;
        } request;
        struct Response {
        } response;
    };

    ///* SLIX2 extension
    struct GetRandomNumber {
        static constexpr std::byte cmd_id { 0xB2 };
        struct Request {
            UID uid;

            inline bool operator==(const Request &o) const = default;
        } request;
        using Response = uint16_t;
        Response &response;
    };

    ///* SLIX2 extension
    struct SetPassword {
        static constexpr std::byte cmd_id { 0xB3 };
        static constexpr bool is_write_alike = true;
        struct Request {
            UID uid;
            SLIX2PasswordID password_id;
            SLIX2Password password;

            inline bool operator==(const Request &o) const = default;
        } request;
        struct Response {
        } response;
    };

    ///* SLIX2 extension
    struct WritePassword {
        static constexpr std::byte cmd_id { 0xB4 };
        static constexpr bool is_write_alike = true;
        struct Request {
            UID uid;
            SLIX2PasswordID password_id;
            SLIX2Password password;

            inline bool operator==(const Request &o) const = default;
        } request;
        struct Response {
        } response;
    };

    ///* SLIX2 extension
    struct PasswordProtectEASAFI {
        static constexpr std::byte cmd_id { 0xA6 };
        static constexpr bool is_write_alike = true;
        struct Request {
            UID uid;

            enum class Option {
                eas,
                afi,
            };

            // What register to protect
            Option option;

            inline bool operator==(const Request &o) const = default;
        } request;
        struct Response {
        } response;
    };

    ///* SLIX2 extension
    struct ProtectPage {
        static constexpr std::byte cmd_id { 0xB6 };
        static constexpr bool is_write_alike = true;
        struct Request {
            UID uid;

            /// ID of the first block of page H
            uint8_t boundary_block_address;

            /// Protection of the L page [0, boundary_block_address)
            SLIX2PageProtection l_page_protection : 2;

            /// Protection of the H page [boundary_block_address, tag_size)
            SLIX2PageProtection h_page_protection : 2;

            inline bool operator==(const Request &o) const = default;
        } request;
        struct Response {
        } response;
    };

    using Command = std::variant<
        Inventory, SystemInfo, StayQuiet,
        ReadSingleBlock, WriteSingleBlock,
        WriteAFI, WriteDSFID,
        LockDSFID,
        GetRandomNumber, SetPassword, WritePassword,
        PasswordProtectEASAFI,
        ProtectPage,
        SetEAS, ResetEAS>;

} // namespace command

using Command = command::Command;

/// Utility function that checks if current command expects response
bool is_response_expected(const Command &command);
/// Utility function that checks if current command is "write-like" command
bool is_write_like_command(const Command &command);
} // namespace nfcv
