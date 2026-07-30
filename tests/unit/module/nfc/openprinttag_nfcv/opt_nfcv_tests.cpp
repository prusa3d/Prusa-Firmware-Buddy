#include <openprinttag_nfcv/opt_nfcv.hpp>

#include <nfcv/rw_interface.hpp>
#include <nfcv/encode.hpp>
#include <catch2/catch_test_macros.hpp>
#include <test_utils/formatters.hpp>

#include <algorithm>
#include <optional>
#include <span>
#include <vector>
#include <variant>
#include <unordered_set>
#include <map>

using namespace openprinttag;

/// Stand-in for the platform RNG. A constant is enough here: how wide the arbitration backoff
/// window is belongs to nfcv::FieldManager's own tests, and all that matters at this level is
/// that a lost round costs time.
uint32_t no_random() {
    return 0;
}

/// The backend's FieldManager reads this whenever it needs the time. Tests move it with \ref at,
/// so that the field's bookkeeping and the timestamps handed to get_event() agree.
uint32_t test_now_ms = 0;

uint32_t test_clock() {
    return test_now_ms;
}

/// Never really sleeps: an arbitration backoff just moves the clock forward
void test_delay(uint32_t delay_ms) {
    test_now_ms += delay_ms;
}

/// Platform hooks for the backend under test
constexpr OPTBackend_NFCV::System test_system {
    .now_ms = test_clock,
    .delay_ms = test_delay,
    .random_u32 = no_random,
};

/// Moves the fake clock, for the operations that are not driven through \ref poll
void at(uint32_t now_ms) {
    test_now_ms = now_ms;
}

/// One NFC task loop iteration: the housekeeping tick first, then a poll for an event.
/// The task loop is what drives tick() on hardware, so the tests drive it the same way.
bool poll(OPTBackend_NFCV &backend, OPTBackend::Event &e, uint32_t now_ms) {
    at(now_ms);
    backend.tick();
    return backend.get_event(e, now_ms);
}

/// One task loop iteration at whatever the fake clock currently says.
/// For the tests where a pass moves the clock itself: on hardware the clock keeps running through
/// the arbitration, and the next iteration reads it as it is rather than rewinding it.
bool poll_now(OPTBackend_NFCV &backend, OPTBackend::Event &e) {
    backend.tick();
    return backend.get_event(e, test_now_ms);
}

struct TagData {
    nfcv::TagInfo info;
    std::optional<bool> eas;
    std::vector<std::byte> *data = nullptr;
    std::optional<uint16_t> random_number;
    bool eas_password_protected = false;
    bool afi_password_protected = false;
    bool dsfid_locked = false;

    /// Antennas the tag is detected by
    std::unordered_set<ReaderAntenna> antennas;

    /// Whether the tag should not respond to Inventory commands
    bool stay_quiet = false;

    using Passwords = std::array<nfcv::SLIX2Password, std::to_underlying(nfcv::SLIX2PasswordID::_password_count)>;
    static size_t password_index(nfcv::SLIX2PasswordID id) {
        return std::countr_zero(std::to_underlying(id));
    }

    static constexpr Passwords default_stored_password { 0, 0, 0x0F0F0F0F, 0x0F0F0F0F, 0 };
    static constexpr Passwords invalid_set_password { 1, 1, 1, 1, 1 };

    Passwords stored_password = default_stored_password;
    Passwords set_password = invalid_set_password;

    bool is_password_correct(nfcv::SLIX2PasswordID password) const {
        UNSCOPED_INFO("Stored pwd: " << stored_password[password_index(password)]);
        UNSCOPED_INFO("Set pwd: " << set_password[password_index(password)]);
        return set_password[password_index(password)] == stored_password[password_index(password)];
    }

    uint8_t page_protection_boundary_block_index = 0;
    nfcv::SLIX2PageProtection l_page_protection = nfcv::SLIX2PageProtection::none;
    nfcv::SLIX2PageProtection h_page_protection = nfcv::SLIX2PageProtection::none;

    bool can_access_block(uint8_t block_index, bool write) const {
        using Protection = nfcv::SLIX2PageProtection;
        const Protection protection = (block_index < page_protection_boundary_block_index ? l_page_protection : h_page_protection);
        switch (protection) {

        case Protection::none:
            return true;

        case Protection::rw_read_password:
            return is_password_correct(nfcv::SLIX2PasswordID::read);

        case Protection::write:
            return !write || is_password_correct(nfcv::SLIX2PasswordID::write);

        case Protection::rw_separate_passwords:
            return is_password_correct(write ? nfcv::SLIX2PasswordID::write : nfcv::SLIX2PasswordID::read);
        }

        std::unreachable();
    }
};

struct FieldUp {
    nfcv::ReaderWriterInterface::AntennaID antenna;

    bool operator==(const FieldUp &other) const = default;
};

struct FieldUpFailed {
    nfcv::ReaderWriterInterface::AntennaID antenna;
    nfcv::Error error;

    bool operator==(const FieldUpFailed &other) const = default;
};

struct FieldDown {
    bool operator==(const FieldDown &other) const = default;
};

using Inventory = nfcv::command::Inventory::Request;
using StayQuiet = nfcv::command::StayQuiet::Request;
using SystemInfo = nfcv::command::SystemInfo::Request;
using ReadSingleBlock = nfcv::command::ReadSingleBlock::Request;
using WriteAFI = nfcv::command::WriteAFI::Request;
using WriteDSFID = nfcv::command::WriteDSFID::Request;
using LockDSFID = nfcv::command::LockDSFID::Request;
using SetEAS = nfcv::command::SetEAS::Request;
using ResetEAS = nfcv::command::ResetEAS::Request;
using GetRandomNumber = nfcv::command::GetRandomNumber::Request;
using SetPassword = nfcv::command::SetPassword::Request;
using PasswordProtectEASAFI = nfcv::command::PasswordProtectEASAFI::Request;
using ProtectPage = nfcv::command::ProtectPage::Request;

struct WriteSingleBlock {
    nfcv::UID uid;
    nfcv::BlockID block_address;
    std::vector<std::byte> data;
    bool operator==(const WriteSingleBlock &other) const = default;
};

using Event = std::variant<
    FieldUp, FieldUpFailed, FieldDown,
    Inventory, StayQuiet, SystemInfo,
    ReadSingleBlock, WriteSingleBlock,
    SetEAS, ResetEAS,
    GetRandomNumber, SetPassword,
    PasswordProtectEASAFI,
    ProtectPage,
    LockDSFID,
    WriteAFI, WriteDSFID>;

namespace std {
std::ostream &operator<<(std::ostream &os, const OPTBackend::TagDetectedEvent &e) {
    os << "TagDetectedEvent{tag: " << e.tag << ", antenna: " << e.antenna << "}";
    return os;
}
std::ostream &operator<<(std::ostream &os, const OPTBackend::TagLostEvent &e) {
    os << "TagLostEvent{tag: " << e.tag << "}";
    return os;
}
} // namespace std

struct EventLogger : public nfcv::ReaderWriterInterface {
    /// Fails every bring-up, for the cases where a caller has to lose its whole arbitration window
    std::optional<nfcv::Error> always_fail_field_up;

    /// Every try_field_up, the contended retries included
    unsigned field_up_attempts = 0;

    nfcv::Result<void> try_field_up(AntennaID antenna) final {
        field_up_attempts++;

        if (const auto error = always_fail_field_up) {
            events.push_back(FieldUpFailed { .antenna = antenna, .error = *error });
            return std::unexpected(*error);
        }
        antenna_index = antenna;
        events.push_back(FieldUp { .antenna = antenna });
        return {};
    }
    void field_down() final {
        antenna_index = 65535;
        events.push_back(FieldDown {});

        for (auto &tag : tags) {
            tag.second.random_number = std::nullopt;
            tag.second.set_password = TagData::invalid_set_password;
            tag.second.stay_quiet = false;
        }
    }

    AntennaID antenna_count() const final {
        return antenna_count_;
    }

    [[nodiscard]] nfcv::Result<void> nfcv_command(const nfcv::Command &command) final {
        // Check that construct_command doesn't throw any asserts
        stdext::inplace_vector<std::byte, 512> msg_builder;
        std::ignore = nfcv::construct_command(msg_builder, command);

        return std::visit([&](const auto &command) { return nfcv_command_impl(command); }, command);
    }

    nfcv::Result<void> nfcv_command_impl(const nfcv::command::Inventory &command) {
        events.push_back(command.request);

        for (auto &tag : tags) {
            if (!tag.second.stay_quiet && tag.second.antennas.contains(antenna_index)) {
                command.response = tag.first;
                return {};
            }
        }

        return std::unexpected(nfcv::Error::no_response);
    }

    nfcv::Result<void> nfcv_command_impl(const nfcv::command::StayQuiet &command) {
        return tag_op(command, [](auto &, auto &tag) {
            tag.stay_quiet = true;
        });
    }

    nfcv::Result<void> nfcv_command_impl(const nfcv::command::SystemInfo &command) {
        return tag_op(command, [](auto &command, auto &tag) {
            command.response = tag.info;
        });
    }

    nfcv::Result<void> nfcv_command_impl(const nfcv::command::ReadSingleBlock &command) {
        return tag_op(command, [](auto &command, auto &tag) -> nfcv::Result<void> {
            if (!tag.can_access_block(command.request.block_address, false)) {
                return std::unexpected(nfcv::Error::response_is_error);
            }

            const auto &buffer = command.response;
            auto it = tag.data->begin();
            std::advance(it, command.request.block_address * tag.info.mem_size->block_size);
            std::copy_n(it, buffer.size(), buffer.begin());
            return {};
        });
    }

    nfcv::Result<void> nfcv_command_impl(const nfcv::command::WriteSingleBlock &command) {
        // Special event type, not handled by tag_op
        events.push_back(WriteSingleBlock {
            .uid = command.request.uid,
            .block_address = command.request.block_address,
            .data { command.request.block_buffer.begin(), command.request.block_buffer.end() },
        });

        return tag_op(command, [](auto &command, auto &tag) -> nfcv::Result<void> {
            if (!tag.can_access_block(command.request.block_address, true)) {
                return std::unexpected(nfcv::Error::response_is_error);
            }

            const auto &buffer = command.request.block_buffer;
            auto it = tag.data->begin();
            std::advance(it, command.request.block_address * tag.info.mem_size->block_size);
            std::copy_n(buffer.begin(), buffer.size(), it);
            return {};
        });
    }

    nfcv::Result<void> nfcv_command_impl(const nfcv::command::WriteAFI &command) {
        return tag_op(command, [](auto &command, auto &tag) -> nfcv::Result<void> {
            if (tag.afi_password_protected && !tag.is_password_correct(nfcv::SLIX2PasswordID::eas_afi)) {
                return std::unexpected(nfcv::Error::response_is_error);
            }

            tag.info.afi = command.request.afi;
            return {};
        });
    }

    nfcv::Result<void> nfcv_command_impl(const nfcv::command::WriteDSFID &command) {
        return tag_op(command, [](auto &command, auto &tag) -> nfcv::Result<void> {
            if (tag.dsfid_locked) {
                return std::unexpected(nfcv::Error::response_is_error);
            }

            tag.info.dsfid = command.request.dsfid;
            return {};
        });
    }

    nfcv::Result<void> nfcv_command_impl(const nfcv::command::LockDSFID &command) {
        return tag_op(command, [](auto &command, auto &tag) -> nfcv::Result<void> {
            if (tag.dsfid_locked) {
                return std::unexpected(nfcv::Error::response_is_error);
            }

            tag.dsfid_locked = true;
            return {};
        });
    }

    nfcv::Result<void> nfcv_command_impl(const nfcv::command::SetEAS &command) {
        return tag_op(command, [](auto &, auto &tag) -> nfcv::Result<void> {
            if (tag.eas_password_protected && !tag.is_password_correct(nfcv::SLIX2PasswordID::eas_afi)) {
                return std::unexpected(nfcv::Error::response_is_error);
            }

            tag.eas = true;
            return {};
        });
    }

    nfcv::Result<void> nfcv_command_impl(const nfcv::command::ResetEAS &command) {
        return tag_op(command, [](auto &, auto &tag) -> nfcv::Result<void> {
            if (tag.eas_password_protected && !tag.is_password_correct(nfcv::SLIX2PasswordID::eas_afi)) {
                return std::unexpected(nfcv::Error::response_is_error);
            }

            tag.eas = false;
            return {};
        });
    }

    nfcv::Result<void> nfcv_command_impl(const nfcv::command::GetRandomNumber &command) {
        return tag_op(command, [](auto &command, auto &tag) {
            tag.random_number = rand();
            command.response = *tag.random_number;
            UNSCOPED_INFO("random_number: " << std::hex << *tag.random_number);
        });
    }

    nfcv::Result<void> nfcv_command_impl(const nfcv::command::SetPassword &command) {
        return tag_op(command, [](auto &command, auto &tag) -> nfcv::Result<void> {
            REQUIRE(tag.random_number.has_value());
            const auto decoded_pwd = command.request.password ^ (*tag.random_number | (*tag.random_number << 16));
            const auto i = TagData::password_index(command.request.password_id);

            if (tag.stored_password[i] != decoded_pwd) {
                UNSCOPED_INFO("PWD id: " << std::hex << std::to_underlying(command.request.password_id));
                UNSCOPED_INFO("Stored pwd: " << std::hex << tag.stored_password[i]);
                UNSCOPED_INFO("Login pwd: " << std::hex << decoded_pwd);
                return std::unexpected(nfcv::Error::response_is_error);
            }

            tag.set_password[i] = decoded_pwd;
            return {};
        });
    }

    nfcv::Result<void> nfcv_command_impl(const nfcv::command::WritePassword &command) {
        return tag_op(command, [](auto &command, auto &tag) -> nfcv::Result<void> {
            if (!tag.is_password_correct(command.request.password_id)) {
                return std::unexpected(nfcv::Error::response_is_error);
            }

            tag.stored_password[TagData::password_index(command.request.password_id)] = command.request.password;
            return {};
        });
    }

    nfcv::Result<void> nfcv_command_impl(const nfcv::command::PasswordProtectEASAFI &command) {
        return tag_op(command, [](auto &command, auto &tag) -> nfcv::Result<void> {
            if (!tag.is_password_correct(nfcv::SLIX2PasswordID::eas_afi)) {
                return std::unexpected(nfcv::Error::response_is_error);
            }

            using Option = nfcv::command::PasswordProtectEASAFI::Request::Option;
            switch (command.request.option) {

            case Option::eas:
                tag.eas_password_protected = true;
                return {};

            case Option::afi:
                tag.afi_password_protected = true;
                return {};
            }

            std::unreachable();
        });
    }

    nfcv::Result<void> nfcv_command_impl(const nfcv::command::ProtectPage &command) {
        return tag_op(command, [](auto &command, auto &tag) -> nfcv::Result<void> {
            if (!tag.is_password_correct(nfcv::SLIX2PasswordID::read) || !tag.is_password_correct(nfcv::SLIX2PasswordID::write)) {
                return std::unexpected(nfcv::Error::response_is_error);
            }

            tag.l_page_protection = command.request.l_page_protection;
            tag.h_page_protection = command.request.h_page_protection;
            tag.page_protection_boundary_block_index = command.request.boundary_block_address;

            return {};
        });
    }

    template <typename C, typename F>
    nfcv::Result<void> tag_op(C &command, F &&f) {
        if constexpr (requires { events.push_back(command.request); }) {
            events.push_back(command.request);
        }

        auto tag = get_tag(command.request.uid);
        if (!tag) {
            return std::unexpected(nfcv::Error::other);
        }

        if constexpr (std::is_same_v<decltype(f(command, *tag)), void>) {
            f(command, *tag);
            return {};
        } else {
            return f(command, *tag);
        }
    }

    TagData *get_tag(const nfcv::UID &uid) {
        const auto r = std::ranges::find_if(tags, [&](const auto &item) { return item.first == uid; });
        return r != tags.end() ? &r->second : nullptr;
    }

    std::vector<Event> events {};
    std::map<nfcv::UID, TagData> tags {};
    size_t antenna_index = 65535;
    size_t antenna_count_ = 2;
};

namespace data {
constexpr nfcv::UID uid1 { std::byte { 0x01 }, std::byte { 0x02 }, std::byte { 0x02 }, std::byte { 0x03 }, std::byte { 0x39 }, std::byte { 0x01 }, std::byte { 0x04 }, nfcv::UID_MSB };
constexpr nfcv::UID uid2 { std::byte { 0x04 }, std::byte { 0x05 }, std::byte { 0x06 }, std::byte { 0x07 }, std::byte { 0x39 }, std::byte { 0x01 }, std::byte { 0x04 }, nfcv::UID_MSB };
constexpr nfcv::UID uid3 { std::byte { 0x08 }, std::byte { 0x09 }, std::byte { 0x0a }, std::byte { 0x0b }, std::byte { 0x39 }, std::byte { 0x01 }, std::byte { 0x04 }, nfcv::UID_MSB };

static nfcv::UID make_uid(uint8_t ix) {
    return nfcv::UID { std::byte { ix }, std::byte { 0x02 }, std::byte { 0x02 }, std::byte { 0x03 }, std::byte { 0x39 }, std::byte { 0x01 }, std::byte { 0x04 }, nfcv::UID_MSB };
}

// Actual UID of a sample SLIX2 tag
constexpr nfcv::UID uid1_slix2 { std::byte { 0xBC }, std::byte { 0x6F }, std::byte { 0x2F }, std::byte { 0x66 }, std::byte { 0x08 }, std::byte { 0x01 }, std::byte { 0x04 }, nfcv::UID_MSB };

constexpr nfcv::TagInfo tag_info1 { .dsfid = std::nullopt, .afi = std::nullopt, .mem_size = nfcv::TagInfo::MemorySize { .block_size = 4, .block_count = 64 }, .ic_ref = std::nullopt };
} // namespace data

TEST_CASE("Test NFC-V tag discovery and tag lost detection", "[nfcv][openprinttag]") {
    EventLogger logger {};

    OPTBackend_NFCV reader(logger, test_system,
        OPTBackend::Config {
            // We're not testing tag limiting here
            .max_known_tags_per_antenna = 4,

            // Forget tags immediately - max_missed_discoveries has a dedicated test
            .tag_max_missed_discoveries = 0,

            .enable_radio = true,
        });

    const auto discovery_interval = reader.config().discovery_interval_ms;

    OPTBackend::Event event;

    SECTION("Test simple TagDisovered -> TagLost functionality") {
        logger.events = {};
        logger.tags[data::uid1] = TagData {
            .info = data::tag_info1,
            .antennas = { 0 },
        };

        auto config = reader.config();
        config.tag_max_missed_discoveries = 4;
        reader.set_config(config);

        uint32_t time = 1;
        auto res = poll(reader, event, time);
        REQUIRE(res == true);
        REQUIRE(std::holds_alternative<OPTBackend::TagDetectedEvent>(event));
        CHECK(std::get<OPTBackend::TagDetectedEvent>(event).tag == 0);
        CHECK(std::ranges::equal(logger.events, std::vector<Event> {
                                                    FieldDown {},
                                                    FieldUp { .antenna = 0 },
                                                    Inventory {},
                                                    StayQuiet { .uid = data::uid1 },
                                                    SystemInfo { .uid = data::uid1 },
                                                    Inventory {},
                                                    FieldDown {},
                                                }));
        logger.events.clear();

        time += discovery_interval / 2;
        res = poll(reader, event, time);
        REQUIRE(res == false);
        CHECK(logger.events.empty());

        logger.events.clear();

        logger.tags[data::uid1].antennas = {};

        for (int i = 0;; i++) {
            time += discovery_interval;
            res = poll(reader, event, time);
            REQUIRE(res == false);
            CHECK(std::ranges::equal(logger.events, std::vector<Event> {
                                                        FieldDown {},
                                                        FieldUp { .antenna = 1 },
                                                        Inventory {},
                                                        FieldDown {},
                                                    }));
            logger.events.clear();

            if (i == config.tag_max_missed_discoveries) {
                break;
            }

            time += discovery_interval;
            res = poll(reader, event, discovery_interval);
            REQUIRE(res == false);
            CHECK(std::ranges::equal(logger.events, std::vector<Event> {
                                                        FieldDown {},
                                                        FieldUp { .antenna = 0 },
                                                        Inventory {},
                                                        FieldDown {},
                                                    }));

            logger.events.clear();
        }

        time += discovery_interval;
        res = poll(reader, event, discovery_interval);
        REQUIRE(res == true);
        REQUIRE(std::holds_alternative<OPTBackend::TagLostEvent>(event));
        REQUIRE(std::get<OPTBackend::TagLostEvent>(event).tag == 0);
        CHECK(std::ranges::equal(logger.events, std::vector<Event> {
                                                    FieldDown {},
                                                    FieldUp { .antenna = 0 },
                                                    Inventory {},
                                                    FieldDown {},
                                                }));
    }

    SECTION("Test mutliple tags discovered scenario - all at once") {
        logger.events = {};
        logger.tags[data::uid1] = TagData {
            .info = data::tag_info1,
            .antennas = { 0 },
        };
        logger.tags[data::uid2] = TagData {
            .info = data::tag_info1,
            .antennas = { 0 },
        };

        auto res = poll(reader, event, 1);
        REQUIRE(res == true);
        REQUIRE(std::holds_alternative<OPTBackend::TagDetectedEvent>(event));
        CHECK(std::get<OPTBackend::TagDetectedEvent>(event).tag == 0);
        CHECK(std::ranges::equal(logger.events, std::vector<Event> {
                                                    FieldDown {},
                                                    FieldUp { .antenna = 0 },
                                                    Inventory {},
                                                    StayQuiet { .uid = data::uid1 },
                                                    SystemInfo { .uid = data::uid1 },
                                                    Inventory {},
                                                    StayQuiet { .uid = data::uid2 },
                                                    SystemInfo { .uid = data::uid2 },
                                                    Inventory {},
                                                    FieldDown {},
                                                }));
        logger.events.clear();
        res = poll(reader, event, 10);
        REQUIRE(res == true);
        REQUIRE(std::holds_alternative<OPTBackend::TagDetectedEvent>(event));
        CHECK(std::get<OPTBackend::TagDetectedEvent>(event).tag == 1);
        CHECK(logger.events.empty());

        logger.events.clear();
        res = poll(reader, event, 20);
        REQUIRE(res == false);
        CHECK(logger.events.empty());
    }

    SECTION("Test mutliple tags discovered scenario - on multiple antennas") {
        logger.events = {};
        logger.tags[data::uid1] = TagData {
            .info = data::tag_info1,
            .antennas = { 0 },
        };
        logger.tags[data::uid2] = TagData {
            .info = data::tag_info1,
            .antennas = { 0 },
        };
        logger.tags[data::uid3] = TagData {
            .info = data::tag_info1,
            .antennas = { 1 },
        };

        auto res = poll(reader, event, 1);
        REQUIRE(res == true);
        REQUIRE(std::holds_alternative<OPTBackend::TagDetectedEvent>(event));
        CHECK(std::get<OPTBackend::TagDetectedEvent>(event).tag == 0);
        CHECK(std::ranges::equal(logger.events, std::vector<Event> {
                                                    FieldDown {},
                                                    FieldUp { .antenna = 0 },
                                                    Inventory {},
                                                    StayQuiet { .uid = data::uid1 },
                                                    SystemInfo { .uid = data::uid1 },
                                                    Inventory {},
                                                    StayQuiet { .uid = data::uid2 },
                                                    SystemInfo { .uid = data::uid2 },
                                                    Inventory {},
                                                    FieldDown {},
                                                }));
        logger.events.clear();
        res = poll(reader, event, 10);
        REQUIRE(res == true);
        REQUIRE(std::holds_alternative<OPTBackend::TagDetectedEvent>(event));
        CHECK(std::get<OPTBackend::TagDetectedEvent>(event).tag == 1);
        CHECK(logger.events.empty());

        logger.events.clear();
        res = poll(reader, event, 20);
        REQUIRE(res == false);
        CHECK(logger.events.empty());

        logger.events.clear();
        res = poll(reader, event, 251);
        REQUIRE(res == true);
        REQUIRE(std::holds_alternative<OPTBackend::TagDetectedEvent>(event));
        CHECK(std::get<OPTBackend::TagDetectedEvent>(event).tag == 2);
        CHECK(std::ranges::equal(logger.events, std::vector<Event> {
                                                    FieldDown {},
                                                    FieldUp { .antenna = 1 },
                                                    Inventory {},
                                                    StayQuiet { .uid = data::uid3 },
                                                    SystemInfo { .uid = data::uid3 },
                                                    Inventory {},
                                                    FieldDown {},
                                                }));
    }

    SECTION("Test tag switch antenna scenario") {
        logger.events = {};
        logger.tags[data::uid1] = TagData {
            .info = data::tag_info1,
            .antennas = { 0 },
        };
        logger.tags[data::uid2] = TagData {
            .info = data::tag_info1,
            .antennas = { 0 },
        };

        auto res = poll(reader, event, 1);
        REQUIRE(res == true);
        REQUIRE(std::holds_alternative<OPTBackend::TagDetectedEvent>(event));
        CHECK(std::get<OPTBackend::TagDetectedEvent>(event).tag == 0);
        CHECK(std::ranges::equal(logger.events, std::vector<Event> {
                                                    FieldDown {},
                                                    FieldUp { .antenna = 0 },
                                                    Inventory {},
                                                    StayQuiet { .uid = data::uid1 },
                                                    SystemInfo { .uid = data::uid1 },
                                                    Inventory {},
                                                    StayQuiet { .uid = data::uid2 },
                                                    SystemInfo { .uid = data::uid2 },
                                                    Inventory {},
                                                    FieldDown {},
                                                }));
        logger.events.clear();
        res = poll(reader, event, 10);
        REQUIRE(res == true);
        REQUIRE(std::holds_alternative<OPTBackend::TagDetectedEvent>(event));
        CHECK(std::get<OPTBackend::TagDetectedEvent>(event).tag == 1);
        CHECK(logger.events.empty());

        logger.events.clear();
        res = poll(reader, event, 251);
        REQUIRE(res == false);
        CHECK(std::ranges::equal(logger.events, std::vector<Event> {
                                                    FieldDown {},
                                                    FieldUp { .antenna = 1 },
                                                    Inventory {},
                                                    FieldDown {},
                                                }));

        logger.tags[data::uid1].antennas = { 0 };
        logger.tags[data::uid2].antennas = { 1 };

        logger.events.clear();
        res = poll(reader, event, 501);
        REQUIRE(res == true);
        REQUIRE(std::holds_alternative<OPTBackend::TagLostEvent>(event));
        CHECK(std::get<OPTBackend::TagLostEvent>(event).tag == 1);
        CHECK(std::ranges::equal(logger.events, std::vector<Event> {
                                                    FieldDown {},
                                                    FieldUp { .antenna = 0 },
                                                    Inventory {},
                                                    StayQuiet { .uid = data::uid1 },
                                                    Inventory {},
                                                    FieldDown {},
                                                }));

        logger.events.clear();
        res = poll(reader, event, 751);
        REQUIRE(res == false);
        CHECK(std::ranges::equal(logger.events, std::vector<Event> {
                                                    FieldDown {},
                                                    FieldUp { .antenna = 1 },
                                                    Inventory {},
                                                    StayQuiet { .uid = data::uid2 },
                                                    Inventory {},
                                                    FieldDown {},
                                                }));

        logger.events.clear();
        res = poll(reader, event, 1001);
        REQUIRE(res == false);
        CHECK(std::ranges::equal(logger.events, std::vector<Event> {
                                                    FieldDown {},
                                                    FieldUp { .antenna = 0 },
                                                    Inventory {},
                                                    StayQuiet { .uid = data::uid1 },
                                                    Inventory {},
                                                    FieldDown {},
                                                }));

        logger.events.clear();
        res = poll(reader, event, 1251);
        REQUIRE(res == false);
        CHECK(std::ranges::equal(logger.events, std::vector<Event> {
                                                    FieldDown {},
                                                    FieldUp { .antenna = 1 },
                                                    Inventory {},
                                                    StayQuiet { .uid = data::uid2 },
                                                    Inventory {},
                                                    FieldDown {},
                                                }));

        reader.forget_tag(1);

        logger.events.clear();
        res = poll(reader, event, 1501);
        REQUIRE(res == false);
        CHECK(std::ranges::equal(logger.events, std::vector<Event> {
                                                    FieldDown {},
                                                    FieldUp { .antenna = 0 },
                                                    Inventory {},
                                                    StayQuiet { .uid = data::uid1 },
                                                    Inventory {},
                                                    FieldDown {},
                                                }));

        logger.events.clear();
        res = poll(reader, event, 1751);
        REQUIRE(res == true);
        REQUIRE(std::holds_alternative<OPTBackend::TagDetectedEvent>(event));
        CHECK(std::get<OPTBackend::TagDetectedEvent>(event).tag == 1);
        CHECK(std::ranges::equal(logger.events, std::vector<Event> {
                                                    FieldDown {},
                                                    FieldUp { .antenna = 1 },
                                                    Inventory {},
                                                    StayQuiet { .uid = data::uid2 },
                                                    SystemInfo { .uid = data::uid2 },
                                                    Inventory {},
                                                    FieldDown {},
                                                }));
    }
}

TEST_CASE("Test NFC-V tag read ops", "[nfcv][openprinttag]") {
    std::vector<std::byte> tag1 {};
    tag1.resize(data::tag_info1.mem_size.value().block_size * data::tag_info1.mem_size.value().block_count);
    std::ranges::generate(tag1, [n = 0]() mutable { return static_cast<std::byte>(n++); });
    EventLogger logger {};
    logger.events = {};
    logger.tags[data::uid1] = TagData {
        .info = data::tag_info1,
        .data = &tag1,
        .antennas = { 0 },
    };

    OPTBackend_NFCV reader { logger, test_system, OPTBackend::Config { .enable_radio = true } };

    OPTBackend::Event event;
    auto res = poll(reader, event, 0);
    REQUIRE(res == true);
    REQUIRE(std::holds_alternative<OPTBackend::TagDetectedEvent>(event));
    CHECK(std::get<OPTBackend::TagDetectedEvent>(event).tag == 0);
    logger.events.clear();

    SECTION("Reading small chunk of data (inside of 1 block)") {
        std::array<std::byte, 2> data;
        data.fill(std::byte { 0 });
        const auto read_res = reader.read(0, 1, std::span { data });

        REQUIRE(read_res.has_value());
        REQUIRE(std::ranges::all_of(std::views::zip(data, std::views::iota(1)), [](const auto &item) { return std::get<0>(item) == static_cast<std::byte>(std::get<1>(item)); }));
        CHECK(std::ranges::equal(logger.events, std::vector<Event> {
                                                    FieldUp { .antenna = 0 },
                                                    ReadSingleBlock { .uid = data::uid1, .block_address = 0 },
                                                }));
    }

    SECTION("Reading small chunk of data (on the edge of a block)") {
        std::array<std::byte, 2> data;
        data.fill(std::byte { 0 });
        const auto read_res = reader.read(0, 3, std::span { data });

        REQUIRE(read_res.has_value());
        REQUIRE(std::ranges::all_of(std::views::zip(data, std::views::iota(3)), [](const auto &item) { return std::get<0>(item) == static_cast<std::byte>(std::get<1>(item)); }));
        CHECK(std::ranges::equal(logger.events, std::vector<Event> {
                                                    FieldUp { .antenna = 0 },
                                                    ReadSingleBlock { .uid = data::uid1, .block_address = 0 },
                                                    ReadSingleBlock { .uid = data::uid1, .block_address = 1 },
                                                }));
    }

    SECTION("Reading multiple blocks of data alligned") {
        std::array<std::byte, 32> data;
        data.fill(std::byte { 0 });
        const auto read_res = reader.read(0, 16, std::span { data });

        REQUIRE(read_res.has_value());
        REQUIRE(std::ranges::all_of(std::views::zip(data, std::views::iota(16)), [](const auto &item) { return std::get<0>(item) == static_cast<std::byte>(std::get<1>(item)); }));
        CHECK(std::ranges::equal(logger.events, std::vector<Event> {
                                                    FieldUp { .antenna = 0 },
                                                    ReadSingleBlock { .uid = data::uid1, .block_address = 4 },
                                                    ReadSingleBlock { .uid = data::uid1, .block_address = 5 },
                                                    ReadSingleBlock { .uid = data::uid1, .block_address = 6 },
                                                    ReadSingleBlock { .uid = data::uid1, .block_address = 7 },
                                                    ReadSingleBlock { .uid = data::uid1, .block_address = 8 },
                                                    ReadSingleBlock { .uid = data::uid1, .block_address = 9 },
                                                    ReadSingleBlock { .uid = data::uid1, .block_address = 10 },
                                                    ReadSingleBlock { .uid = data::uid1, .block_address = 11 },
                                                }));
    }

    SECTION("Reading multiple blocks of data unalligned") {
        std::array<std::byte, 36> data;
        data.fill(std::byte { 0 });
        const auto read_res = reader.read(0, 15, std::span { data });

        REQUIRE(read_res.has_value());
        REQUIRE(std::ranges::all_of(std::views::zip(data, std::views::iota(15)), [](const auto &item) { return std::get<0>(item) == static_cast<std::byte>(std::get<1>(item)); }));
        CHECK(std::ranges::equal(logger.events, std::vector<Event> {
                                                    FieldUp { .antenna = 0 },
                                                    ReadSingleBlock { .uid = data::uid1, .block_address = 3 },
                                                    ReadSingleBlock { .uid = data::uid1, .block_address = 4 },
                                                    ReadSingleBlock { .uid = data::uid1, .block_address = 5 },
                                                    ReadSingleBlock { .uid = data::uid1, .block_address = 6 },
                                                    ReadSingleBlock { .uid = data::uid1, .block_address = 7 },
                                                    ReadSingleBlock { .uid = data::uid1, .block_address = 8 },
                                                    ReadSingleBlock { .uid = data::uid1, .block_address = 9 },
                                                    ReadSingleBlock { .uid = data::uid1, .block_address = 10 },
                                                    ReadSingleBlock { .uid = data::uid1, .block_address = 11 },
                                                    ReadSingleBlock { .uid = data::uid1, .block_address = 12 },
                                                }));
    }

    SECTION("Reading data on invalid tag") {
        std::array<std::byte, 2> data;
        data.fill(std::byte { 0 });
        const auto read_res = reader.read(1, 3, std::span { data });

        REQUIRE(!read_res.has_value());
        REQUIRE(read_res.error() == OPTBackend::IOError::invalid_id);
    }

    SECTION("Reading data out of range - totaly out of range (big offset)") {
        std::array<std::byte, 2> data;
        data.fill(std::byte { 0 });
        const auto read_res = reader.read(0, 270, std::span { data });

        REQUIRE(!read_res.has_value());
        REQUIRE(read_res.error() == OPTBackend::IOError::outside_of_bounds);
    }

    SECTION("Reading data out of range - overlaping range") {
        std::array<std::byte, 100> data;
        data.fill(std::byte { 0 });
        const auto read_res = reader.read(0, 250, std::span { data });

        REQUIRE(!read_res.has_value());
        REQUIRE(read_res.error() == OPTBackend::IOError::outside_of_bounds);
    }
}

TEST_CASE("Test NFC-V tags write ops", "[nfcv][openprinttag]") {
    std::vector<std::byte> tag1 {};
    tag1.resize(data::tag_info1.mem_size.value().block_size * data::tag_info1.mem_size.value().block_count);
    std::ranges::generate(tag1, [n = 0]() mutable { return static_cast<std::byte>(n++); });
    EventLogger logger {};
    logger.events = {};
    logger.tags[data::uid1] = TagData {
        .info = data::tag_info1,
        .data = &tag1,
        .antennas = { 0 },
    };

    OPTBackend_NFCV reader { logger, test_system, OPTBackend::Config { .enable_radio = true } };

    OPTBackend::Event event;
    auto res = poll(reader, event, 0);
    REQUIRE(res == true);
    REQUIRE(std::holds_alternative<OPTBackend::TagDetectedEvent>(event));
    CHECK(std::get<OPTBackend::TagDetectedEvent>(event).tag == 0);
    logger.events.clear();

    SECTION("Write aligned data") {
        constexpr PayloadPos offset = 0;
        std::array<std::byte, 32> data;
        std::ranges::generate(data, [n = 0]() mutable { return static_cast<std::byte>(255 - (n++)); });
        const auto write_res = reader.write(0, offset, std::span { data });

        REQUIRE(write_res.has_value());
        for (size_t i = 0; i < data.size(); ++i) {
            CHECK(tag1[i + offset] == data[i]);
        }
        CHECK(tag1[data.size() + offset] == static_cast<std::byte>(data.size() + offset));
        CHECK(std::ranges::equal(logger.events, std::vector<Event> {
                                                    FieldUp { .antenna = 0 },
                                                    WriteSingleBlock { .uid = data::uid1, .block_address = 0, .data = std::vector { std::byte { 0xff }, std::byte { 0xfe }, std::byte { 0xfd }, std::byte { 0xfc } } },
                                                    WriteSingleBlock { .uid = data::uid1, .block_address = 1, .data = std::vector { std::byte { 0xfb }, std::byte { 0xfa }, std::byte { 0xf9 }, std::byte { 0xf8 } } },
                                                    WriteSingleBlock { .uid = data::uid1, .block_address = 2, .data = std::vector { std::byte { 0xf7 }, std::byte { 0xf6 }, std::byte { 0xf5 }, std::byte { 0xf4 } } },
                                                    WriteSingleBlock { .uid = data::uid1, .block_address = 3, .data = std::vector { std::byte { 0xf3 }, std::byte { 0xf2 }, std::byte { 0xf1 }, std::byte { 0xf0 } } },
                                                    WriteSingleBlock { .uid = data::uid1, .block_address = 4, .data = std::vector { std::byte { 0xef }, std::byte { 0xee }, std::byte { 0xed }, std::byte { 0xec } } },
                                                    WriteSingleBlock { .uid = data::uid1, .block_address = 5, .data = std::vector { std::byte { 0xeb }, std::byte { 0xea }, std::byte { 0xe9 }, std::byte { 0xe8 } } },
                                                    WriteSingleBlock { .uid = data::uid1, .block_address = 6, .data = std::vector { std::byte { 0xe7 }, std::byte { 0xe6 }, std::byte { 0xe5 }, std::byte { 0xe4 } } },
                                                    WriteSingleBlock { .uid = data::uid1, .block_address = 7, .data = std::vector { std::byte { 0xe3 }, std::byte { 0xe2 }, std::byte { 0xe1 }, std::byte { 0xe0 } } },
                                                }));
    }

    SECTION("Write unaligned data - overflow in the beginning") {
        constexpr PayloadPos offset = 2;
        std::array<std::byte, 18> data;
        std::ranges::generate(data, [n = 0]() mutable { return static_cast<std::byte>(255 - (n++)); });
        const auto write_res = reader.write(0, offset, std::span { data });

        REQUIRE(write_res.has_value());
        for (size_t i = 0; i < data.size(); ++i) {
            CHECK(tag1[i + offset] == data[i]);
        }
        CHECK(tag1[offset - 1] == static_cast<std::byte>(offset - 1));
        CHECK(tag1[data.size() + offset] == static_cast<std::byte>(data.size() + offset));
        CHECK(std::ranges::equal(logger.events, std::vector<Event> {
                                                    FieldUp { .antenna = 0 },
                                                    ReadSingleBlock { .uid = data::uid1, .block_address = 0 },
                                                    WriteSingleBlock { .uid = data::uid1, .block_address = 0, .data = std::vector { std::byte { 0x00 }, std::byte { 0x01 }, std::byte { 0xff }, std::byte { 0xfe } } },
                                                    WriteSingleBlock { .uid = data::uid1, .block_address = 1, .data = std::vector { std::byte { 0xfd }, std::byte { 0xfc }, std::byte { 0xfb }, std::byte { 0xfa } } },
                                                    WriteSingleBlock { .uid = data::uid1, .block_address = 2, .data = std::vector { std::byte { 0xf9 }, std::byte { 0xf8 }, std::byte { 0xf7 }, std::byte { 0xf6 } } },
                                                    WriteSingleBlock { .uid = data::uid1, .block_address = 3, .data = std::vector { std::byte { 0xf5 }, std::byte { 0xf4 }, std::byte { 0xf3 }, std::byte { 0xf2 } } },
                                                    WriteSingleBlock { .uid = data::uid1, .block_address = 4, .data = std::vector { std::byte { 0xf1 }, std::byte { 0xf0 }, std::byte { 0xef }, std::byte { 0xee } } },
                                                }));
    }

    SECTION("Write unaligned data - overflow at the end") {
        constexpr PayloadPos offset = 4;
        std::array<std::byte, 18> data;
        std::ranges::generate(data, [n = 0]() mutable { return static_cast<std::byte>(255 - (n++)); });
        const auto write_res = reader.write(0, offset, std::span { data });

        CAPTURE(data);
        CAPTURE(tag1);
        REQUIRE(write_res.has_value());
        for (size_t i = 0; i < data.size(); ++i) {
            CHECK(tag1[i + offset] == data[i]);
        }
        CHECK(tag1[offset - 1] == static_cast<std::byte>(offset - 1));
        CHECK(tag1[data.size() + offset] == static_cast<std::byte>(data.size() + offset));
        CHECK(std::ranges::equal(logger.events, std::vector<Event> {
                                                    FieldUp { .antenna = 0 },
                                                    WriteSingleBlock { .uid = data::uid1, .block_address = 1, .data = std::vector { std::byte { 0xff }, std::byte { 0xfe }, std::byte { 0xfd }, std::byte { 0xfc } } },
                                                    WriteSingleBlock { .uid = data::uid1, .block_address = 2, .data = std::vector { std::byte { 0xfb }, std::byte { 0xfa }, std::byte { 0xf9 }, std::byte { 0xf8 } } },
                                                    WriteSingleBlock { .uid = data::uid1, .block_address = 3, .data = std::vector { std::byte { 0xf7 }, std::byte { 0xf6 }, std::byte { 0xf5 }, std::byte { 0xf4 } } },
                                                    WriteSingleBlock { .uid = data::uid1, .block_address = 4, .data = std::vector { std::byte { 0xf3 }, std::byte { 0xf2 }, std::byte { 0xf1 }, std::byte { 0xf0 } } },
                                                    ReadSingleBlock { .uid = data::uid1, .block_address = 5 },
                                                    WriteSingleBlock { .uid = data::uid1, .block_address = 5, .data = std::vector { std::byte { 0xef }, std::byte { 0xee }, std::byte { 0x16 }, std::byte { 0x17 } } },
                                                }));
    }

    SECTION("Write data smaller then sector - start of sector") {
        constexpr PayloadPos offset = 0;
        std::array<std::byte, 2> data;
        std::ranges::generate(data, [n = 0]() mutable { return static_cast<std::byte>(255 - (n++)); });
        const auto write_res = reader.write(0, offset, std::span { data });

        CAPTURE(data);
        CAPTURE(tag1);
        REQUIRE(write_res.has_value());
        for (size_t i = 0; i < data.size(); ++i) {
            CHECK(tag1[i + offset] == data[i]);
        }
        CHECK(tag1[data.size() + offset] == static_cast<std::byte>(data.size() + offset));
        CHECK(std::ranges::equal(logger.events, std::vector<Event> {
                                                    FieldUp { .antenna = 0 },
                                                    ReadSingleBlock { .uid = data::uid1, .block_address = 0 },
                                                    WriteSingleBlock { .uid = data::uid1, .block_address = 0, .data = std::vector { std::byte { 0xff }, std::byte { 0xfe }, std::byte { 0x02 }, std::byte { 0x03 } } },
                                                }));
    }

    SECTION("Write data smaller then sector - middle of sector") {
        constexpr PayloadPos offset = 1;
        std::array<std::byte, 2> data;
        std::ranges::generate(data, [n = 0]() mutable { return static_cast<std::byte>(255 - (n++)); });
        const auto write_res = reader.write(0, offset, std::span { data });

        CAPTURE(data);
        CAPTURE(tag1);
        REQUIRE(write_res.has_value());
        for (size_t i = 0; i < data.size(); ++i) {
            CHECK(tag1[i + offset] == data[i]);
        }
        CHECK(tag1[offset - 1] == static_cast<std::byte>(offset - 1));
        CHECK(tag1[data.size() + offset] == static_cast<std::byte>(data.size() + offset));
        CHECK(std::ranges::equal(logger.events, std::vector<Event> {
                                                    FieldUp { .antenna = 0 },
                                                    ReadSingleBlock { .uid = data::uid1, .block_address = 0 },
                                                    WriteSingleBlock { .uid = data::uid1, .block_address = 0, .data = std::vector { std::byte { 0x00 }, std::byte { 0xff }, std::byte { 0xfe }, std::byte { 0x03 } } },
                                                }));
    }

    SECTION("Write data smaller then sector - end of sector") {
        constexpr PayloadPos offset = 2;
        std::array<std::byte, 2> data;
        std::ranges::generate(data, [n = 0]() mutable { return static_cast<std::byte>(255 - (n++)); });
        const auto write_res = reader.write(0, offset, std::span { data });

        CAPTURE(data);
        CAPTURE(tag1);
        REQUIRE(write_res.has_value());
        for (size_t i = 0; i < data.size(); ++i) {
            CHECK(tag1[i + offset] == data[i]);
        }
        CHECK(tag1[offset - 1] == static_cast<std::byte>(offset - 1));
        CHECK(tag1[data.size() + offset] == static_cast<std::byte>(data.size() + offset));
        CHECK(std::ranges::equal(logger.events, std::vector<Event> {
                                                    FieldUp { .antenna = 0 },
                                                    ReadSingleBlock { .uid = data::uid1, .block_address = 0 },
                                                    WriteSingleBlock { .uid = data::uid1, .block_address = 0, .data = std::vector { std::byte { 0x00 }, std::byte { 0x01 }, std::byte { 0xff }, std::byte { 0xfe } } },
                                                }));
    }

    SECTION("Write data smaller then sector - overlapping sectors") {
        constexpr PayloadPos offset = 3;
        std::array<std::byte, 2> data;
        std::ranges::generate(data, [n = 0]() mutable { return static_cast<std::byte>(255 - (n++)); });
        const auto write_res = reader.write(0, offset, std::span { data });

        CAPTURE(data);
        CAPTURE(tag1);
        REQUIRE(write_res.has_value());
        for (size_t i = 0; i < data.size(); ++i) {
            CHECK(tag1[i + offset] == data[i]);
        }
        CHECK(tag1[offset - 1] == static_cast<std::byte>(offset - 1));
        CHECK(tag1[data.size() + offset] == static_cast<std::byte>(data.size() + offset));
        CHECK(std::ranges::equal(logger.events, std::vector<Event> {
                                                    FieldUp { .antenna = 0 },
                                                    ReadSingleBlock { .uid = data::uid1, .block_address = 0 },
                                                    WriteSingleBlock { .uid = data::uid1, .block_address = 0, .data = std::vector { std::byte { 0x00 }, std::byte { 0x01 }, std::byte { 0x02 }, std::byte { 0xff } } },
                                                    ReadSingleBlock { .uid = data::uid1, .block_address = 1 },
                                                    WriteSingleBlock { .uid = data::uid1, .block_address = 1, .data = std::vector { std::byte { 0xfe }, std::byte { 0x05 }, std::byte { 0x06 }, std::byte { 0x07 } } },
                                                }));
    }
}

TEST_CASE("Test NFC-V register commands", "[nfcv]") {
    EventLogger logger;
    logger.events = {};
    logger.tags[data::uid1_slix2] = TagData {
        .info = {
            .mem_size = nfcv::TagInfo::MemorySize { .block_size = 4, .block_count = 64 },
        },
        .antennas = { 0 },
    };

    const auto &tag = logger.tags[data::uid1_slix2];

    SECTION("Password setting/writing") {
        CHECK(tag.set_password != tag.stored_password);

        // Password not set - cannot write password
        CHECK(logger.write_register(data::uid1_slix2, nfcv::ReaderWriterInterface::Register::read_password, 0xafaf) == std::unexpected(nfcv::Error::response_is_error));

        // Set correct password
        {
            CHECK(logger.set_password(data::uid1_slix2, nfcv::ReaderWriterInterface::Register::read_password, 0) == nfcv::Result<void> {});
            CHECK(logger.set_password(data::uid1_slix2, nfcv::ReaderWriterInterface::Register::write_password, 0) == nfcv::Result<void> {});
            CHECK(logger.set_password(data::uid1_slix2, nfcv::ReaderWriterInterface::Register::eas_afi_password, 0) == nfcv::Result<void> {});
            CHECK(logger.set_password(data::uid1_slix2, nfcv::ReaderWriterInterface::Register::privacy_password, 0x0F0F0F0F) == nfcv::Result<void> {});
            CHECK(logger.set_password(data::uid1_slix2, nfcv::ReaderWriterInterface::Register::destroy_password, 0x0F0F0F0F) == nfcv::Result<void> {});

            CAPTURE(tag.set_password);
            CAPTURE(tag.stored_password);
            CAPTURE(*tag.random_number);
            CHECK(tag.set_password == tag.stored_password);
        }

        // Now write the new password
        {

            CAPTURE(tag.set_password);
            CAPTURE(tag.stored_password);
            CAPTURE(*tag.random_number);
            CHECK(logger.write_register(data::uid1_slix2, nfcv::ReaderWriterInterface::Register::read_password, 0xafaf) == nfcv::Result<void> {});
        }

        // Writing password again should fail, because the password has changed and we need to set it
        CHECK(logger.write_register(data::uid1_slix2, nfcv::ReaderWriterInterface::Register::read_password, 0xafaf) == std::unexpected(nfcv::Error::response_is_error));

        // Set some wrong password
        CHECK(logger.set_password(data::uid1_slix2, nfcv::ReaderWriterInterface::Register::read_password, 0x1234) == std::unexpected(nfcv::Error::response_is_error));

        // Still failing
        CHECK(logger.write_register(data::uid1_slix2, nfcv::ReaderWriterInterface::Register::read_password, 0xdeadbeef) == std::unexpected(nfcv::Error::response_is_error));

        // Set correct password
        CHECK(logger.set_password(data::uid1_slix2, nfcv::ReaderWriterInterface::Register::read_password, 0xafaf) == nfcv::Result<void> {});

        // Write the same password again
        CHECK(logger.write_register(data::uid1_slix2, nfcv::ReaderWriterInterface::Register::read_password, 0xafaf) == nfcv::Result<void> {});

        // Write it again - should still be no error
        CHECK(logger.write_register(data::uid1_slix2, nfcv::ReaderWriterInterface::Register::read_password, 0xafaf) == nfcv::Result<void> {});
    }
}

TEST_CASE("Test NFC-V tag initialization", "[nfcv][openprinttag]") {
    constexpr auto tag_uid = data::uid1_slix2;

    EventLogger logger;
    logger.events = {};
    logger.tags[tag_uid] = TagData {
        .info = {
            .mem_size = nfcv::TagInfo::MemorySize { .block_size = 4, .block_count = 64 },
        },
        .antennas = { 0 },
    };

    OPTBackend_NFCV reader { logger, test_system, OPTBackend::Config { .enable_radio = true } };
    OPTBackend::Event event;

    auto &tag = logger.tags[tag_uid];

    std::vector<std::byte> tag_data {};
    tag_data.resize(tag.info.mem_size->block_size * tag.info.mem_size->block_count);
    tag.data = &tag_data;

    // Process tag detection
    {
        OPTBackend::Event event;
        auto res = poll(reader, event, 0);
        REQUIRE(res == true);
        REQUIRE(std::holds_alternative<OPTBackend::TagDetectedEvent>(event));
        const auto &ev = std::get<OPTBackend::TagDetectedEvent>(event);
        CHECK(ev.tag == 0);
        logger.events.clear();
    }

    constexpr auto io_success = OPTBackend::IOResult<void>();

    constexpr PayloadPos lock_boundary = 32;

    constexpr std::array test1 { std::byte { 0x12 }, std::byte { 0x34 } };
    constexpr std::array test2 { std::byte { 0x56 }, std::byte { 0x78 } };
    std::array<std::byte, 2> read_buf;

    const auto check_write_access = [&](bool locked, bool registers_locked) {
        CAPTURE(locked, registers_locked, lock_boundary);

        const OPTBackend::IOResult<void> expected_write_result = locked ? std::unexpected(OPTBackend::IOError::other) : OPTBackend::IOResult<void> {};

        // Reads should always succeed
        CHECK(reader.read(0, lock_boundary, read_buf) == io_success);
        CHECK(reader.read(0, lock_boundary - test1.size(), read_buf) == io_success);

        CHECK(reader.write(0, lock_boundary - test1.size(), test1) == expected_write_result);

        // Writes after lock boundary should always succeed
        CHECK(reader.write(0, lock_boundary, test2) == io_success);

        // Check registers writing
        {
            using Register = nfcv::ReaderWriterInterface::Register;
            const nfcv::Result<void> expected_ll_write_result = registers_locked ? std::unexpected(nfcv::Error::response_is_error) : nfcv::Result<void> {};

            logger.try_field_up(0);

            CHECK(logger.write_register(tag_uid, Register::afi, 135) == expected_ll_write_result);
            CHECK(logger.write_register(tag_uid, Register::dsfid, 231) == expected_ll_write_result);
            CHECK(logger.write_register(tag_uid, Register::eas, 1) == expected_ll_write_result);

            // At no point we should be able to just write a password (we would first need to set_password either to 0 or to the protection one)
            CHECK(logger.write_register(tag_uid, Register::write_password, 1234) == std::unexpected(nfcv::Error::response_is_error));

            logger.field_down();
        }
    };

    {
        INFO("Initial write access check");
        check_write_access(false, false);
    }

    using Params = OPTBackend::InitializeTagParams;
    Params params {
        .password = 0xdeadbeef,
        .protect_first_num_bytes = lock_boundary,
    };

    const auto post_init_checks = [&] {
        INFO("Post-init checks");
        CAPTURE(params.protection_policy);

        // Check that the tag has been initialized properly
        CHECK(tag.info.afi == 0);
        CHECK(tag.info.dsfid == 0);

        const bool locked = params.protection_policy != Params::ProtectionPolicy::none;

        if (locked) {
            // Double locking should fail
            CHECK(reader.initialize_tag(0, params) == std::unexpected(OPTBackend::IOError::other));
        }

        check_write_access(locked, locked);
    };

    SECTION("No protection") {
        const auto r = reader.initialize_tag(0, params);
        REQUIRE(r == io_success);

        post_init_checks();
    }

    SECTION("Locking") {
        params.protection_policy = Params::ProtectionPolicy::lock;
        const auto r = reader.initialize_tag(0, params);

        // Update unittests if this gets supported
        REQUIRE(r == std::unexpected(OPTBackend::IOError::not_implemented));
    }

    SECTION("Write password protection") {
        params.protection_policy = Params::ProtectionPolicy::write_password;
        const auto r = reader.initialize_tag(0, params);
        REQUIRE(r == io_success);

        post_init_checks();

        {
            INFO("Unlocking");

            REQUIRE(reader.unlock_tag(0, 0xdeadbeef));

            // Check that the memory is unlocked; registers should still remain locked
            check_write_access(false, true);
        }

        {
            INFO("Best effort re-locking");

            // Try initializing again with best-effort, this should protect the memory again
            params.best_effort = true;
            REQUIRE(reader.initialize_tag(0, params) == io_success);
            check_write_access(true, true);
        }
    }
}

TEST_CASE("Test NFC debug mode", "[nfcv][openprinttag]") {
    EventLogger logger;
    logger.events = {};
    logger.tags[data::uid1] = TagData {
        .info = data::tag_info1,
        .antennas = { 0 },
    };
    logger.tags[data::uid2] = TagData {
        .info = data::tag_info1,
        .antennas = { 1 },
    };

    OPTBackend::Config config {
        // Forget tags immediately, not testing that functionality here
        .tag_max_missed_discoveries = 0,
        .enable_radio = true,
    };
    OPTBackend_NFCV reader { logger, test_system, config };
    const auto discovery_interval = reader.config().discovery_interval_ms;

    OPTBackend::Event event;

    SECTION("No debug") {
        uint32_t time = 0;
        CHECK(poll(reader, event, time));
        CHECK(event == OPTBackend::Event { OPTBackend::TagDetectedEvent { .tag = 0, .antenna = 0 } });
        time += discovery_interval;

        CHECK(poll(reader, event, time));
        CHECK(event == OPTBackend::Event { OPTBackend::TagDetectedEvent { .tag = 1, .antenna = 1 } });
        time += discovery_interval;

        // Remove first tag from the reader
        logger.tags[data::uid1].antennas = {};

        CHECK(poll(reader, event, time));
        CHECK(event == OPTBackend::Event { OPTBackend::TagLostEvent { .tag = 0 } });
        time += discovery_interval;

        // Put the tag back
        logger.tags[data::uid1].antennas = { 0 };

        // No event should happen - the tag was not forgotten and thus should not be redetected
        CHECK(!poll(reader, event, time));
        time += discovery_interval;

        CHECK(!poll(reader, event, time));
        time += discovery_interval;
    }

    SECTION("Auto forget") {
        config.auto_forget_tag = true;
        reader.set_config(config);

        uint32_t time = 0;
        CHECK(poll(reader, event, time));
        CHECK(event == OPTBackend::Event { OPTBackend::TagDetectedEvent { .tag = 0, .antenna = 0 } });
        time += discovery_interval;

        CHECK(poll(reader, event, time));
        CHECK(event == OPTBackend::Event { OPTBackend::TagDetectedEvent { .tag = 1, .antenna = 1 } });
        time += discovery_interval;

        // Remove first tag from the reader
        logger.tags[data::uid1].antennas = {};

        CHECK(poll(reader, event, time));
        CHECK(event == OPTBackend::Event { OPTBackend::TagLostEvent { .tag = 0 } });
        time += discovery_interval;

        CHECK(!poll(reader, event, time));
        time += discovery_interval;

        CHECK(!poll(reader, event, time));
        time += discovery_interval;

        // Put the tag back
        logger.tags[data::uid1].antennas = { 0 };

        // The reader is alternating antennas, there should be no changes on antenna 1
        CHECK(!poll(reader, event, time));
        time += discovery_interval;

        // The tag should be reported again and the ID reused
        CHECK(poll(reader, event, time));
        CHECK(event == OPTBackend::Event { OPTBackend::TagDetectedEvent { .tag = 0, .antenna = 0 } });
        time += discovery_interval;
    }

    SECTION("Force antenna") {
        config.enforced_antenna = 1;
        reader.set_config(config);

        uint32_t time = 0;
        CHECK(poll(reader, event, time));
        CHECK(event == OPTBackend::Event { OPTBackend::TagDetectedEvent { .tag = 0, .antenna = 1 } });
        time += discovery_interval;

        // Remove first tag from the reader - should do nothing
        logger.tags[data::uid1].antennas = {};

        CHECK(!poll(reader, event, time));
        time += discovery_interval;

        // Remove second tag from the reader - should result in tag lost event
        logger.tags[data::uid2].antennas = {};

        CHECK(poll(reader, event, time));
        CHECK(event == OPTBackend::Event { OPTBackend::TagLostEvent { .tag = 0 } });
        time += discovery_interval;
    }
}

TEST_CASE("Test NFC get_tag_uid", "[nfcv][openprinttag]") {
    EventLogger logger;
    logger.events = {};
    logger.tags[data::uid1] = TagData {
        .info = data::tag_info1,
        .antennas = { 0 },
    };

    OPTBackend_NFCV reader { logger, test_system, OPTBackend::Config { .enable_radio = true } };
    OPTBackend::Event event;

    CHECK(poll(reader, event, 0));
    CHECK(event == OPTBackend::Event { OPTBackend::TagDetectedEvent { .tag = 0, .antenna = 0 } });

    nfcv::UID uid;
    std::array<std::byte, 1> small_buffer;
    REQUIRE(reader.get_tag_uid(0, uid) == sizeof(uid));
    CHECK(uid == data::uid1);

    CHECK(reader.get_tag_uid(1, uid) == std::unexpected(OPTBackend::IOError::invalid_id));
    CHECK(reader.get_tag_uid(0, small_buffer) == std::unexpected(OPTBackend::IOError::data_too_big));
}

TEST_CASE("NFCV::max_known_tags_per_antenna", "[nfcv][openprinttag]") {
    EventLogger logger;
    logger.events = {};

    OPTBackend::Config config {
        .max_known_tags_per_antenna = 2,
        // Not testing that here
        .tag_max_missed_discoveries = 0,
        .enable_radio = true,
    };
    OPTBackend_NFCV reader { logger, test_system, config };
    OPTBackend::Event event;

    REQUIRE(config.max_known_tags_per_antenna <= OPTBackend_NFCV::MAX_KNOWN_TAGS / 2);

    for (int i = 0; i < 4; i++) {
        logger.tags[data::make_uid(i)] = TagData {
            .info = data::tag_info1,
            .antennas = { 0 },
        };
    }

    nfcv::UID uid;

    uint32_t time = 1;

    std::unordered_map<openprinttag::TagID, nfcv::UID> tag_uids;

    // Check that we've detected all the expected tags in one go
    for (int i = 0; i < config.max_known_tags_per_antenna; i++) {
        REQUIRE(poll(reader, event, time));
        REQUIRE(std::holds_alternative<OPTBackend::TagDetectedEvent>(event));
        const auto tag_id = std::get<OPTBackend::TagDetectedEvent>(event).tag;
        REQUIRE(reader.get_tag_uid(tag_id, uid));
        REQUIRE(uid == data::make_uid(i));
        tag_uids[tag_id] = uid;
    }

    // Tag limit reached
    REQUIRE(!poll(reader, event, time));

    // Move tag 0 to antenna 1
    logger.tags[data::make_uid(0)].antennas = { 1 };

    // Next discovery should be for antenna 1, but the tag was not lost on antenna 0 yet
    time += config.discovery_interval_ms;
    REQUIRE(!poll(reader, event, time));

    // Antenna 0 discovery - should lose tag 0
    {
        time += config.discovery_interval_ms;
        REQUIRE(poll(reader, event, time));
        REQUIRE(std::holds_alternative<OPTBackend::TagLostEvent>(event));
        const auto tag_id = std::get<OPTBackend::TagLostEvent>(event).tag;
        REQUIRE(tag_uids[tag_id] == data::make_uid(0));
        reader.forget_tag(tag_id);

        REQUIRE(!poll(reader, event, time));
    }

    // Antenna 1 discovery - detect tag 0
    {
        time += config.discovery_interval_ms;
        REQUIRE(poll(reader, event, time));
        REQUIRE(std::holds_alternative<OPTBackend::TagDetectedEvent>(event));
        const auto tag_id = std::get<OPTBackend::TagDetectedEvent>(event).tag;
        REQUIRE(std::get<OPTBackend::TagDetectedEvent>(event).antenna == 1);
        REQUIRE(reader.get_tag_uid(tag_id, uid));
        REQUIRE(uid == data::make_uid(0));

        REQUIRE(!poll(reader, event, time));
    }

    // Antenna 0 discovery - get tag 2
    {
        time += config.discovery_interval_ms;
        REQUIRE(poll(reader, event, time));
        REQUIRE(std::holds_alternative<OPTBackend::TagDetectedEvent>(event));
        const auto tag_id = std::get<OPTBackend::TagDetectedEvent>(event).tag;
        REQUIRE(reader.get_tag_uid(tag_id, uid));
        REQUIRE(uid == data::make_uid(2));

        REQUIRE(!poll(reader, event, time));
    }
}

namespace {

/// Config for the field tests: radio on.
///
/// The default discovery interval is deliberately enormous, for the tests that must not have a
/// pass interleave with what they measure. Note that RateLimiter treats a last_event_ of 0 as
/// "never ran", so the initial detection pass has to be triggered at t=1 rather than t=0 for
/// the limiter to start counting at all.
OPTBackend::Config lifetime_config(uint32_t discovery_interval_ms = 100000) {
    return OPTBackend::Config {
        .tag_max_missed_discoveries = 0,
        .discovery_interval_ms = discovery_interval_ms,
        .enable_radio = true,
    };
}

size_t count_field_ups(const std::vector<Event> &events) {
    return std::ranges::count_if(events, [](const Event &e) { return std::holds_alternative<FieldUp>(e); });
}

size_t count_field_downs(const std::vector<Event> &events) {
    return std::ranges::count_if(events, [](const Event &e) { return std::holds_alternative<FieldDown>(e); });
}

constexpr uint32_t linger_ms = nfcv::FieldManager::linger_ms;

} // namespace

TEST_CASE("NFC-V discovery yields the field between passes", "[nfcv][openprinttag][field]") {
    EventLogger logger;
    logger.tags[data::uid1] = TagData {
        .info = data::tag_info1,
        .antennas = { 0 },
    };

    OPTBackend_NFCV reader { logger, test_system, lifetime_config(250) };
    const auto config = reader.config();

    OPTBackend::Event event;

    SECTION("a pass ends with the field down") {
        REQUIRE(poll(reader, event, 1));
        REQUIRE(!logger.events.empty());
        CHECK(std::holds_alternative<FieldDown>(logger.events.back()));
    }

    SECTION("a contended pass is skipped without counting a missed discovery") {
        REQUIRE(poll(reader, event, 1));
        REQUIRE(std::holds_alternative<OPTBackend::TagDetectedEvent>(event));

        // With the tag gone and tag_max_missed_discoveries at 0, any pass that actually ran
        // would report it lost on its very first miss
        logger.tags[data::uid1].antennas = {};
        logger.always_fail_field_up = nfcv::Error::field_collision;
        logger.events.clear();

        for (int i = 0; i < 4; i++) {
            // Off the fake clock: a pass that arbitrated its whole window moved the clock forward,
            // and the interval counts from where that pass left off
            at(test_now_ms + config.discovery_interval_ms);
            logger.field_up_attempts = 0;
            CHECK(!poll_now(reader, event));

            // Every pass really did try, and re-arbitrated within its window rather than
            // giving up on the first collision
            CHECK(logger.field_up_attempts > 1);
        }

        CHECK(count_field_ups(logger.events) == 0);
    }

    SECTION("a skipped pass does not advance the antenna") {
        logger.always_fail_field_up = nfcv::Error::field_collision;

        CHECK(!poll(reader, event, 1));
        CHECK(std::ranges::find(logger.events, Event { FieldUpFailed { .antenna = 0, .error = nfcv::Error::field_collision } }) != logger.events.end());

        logger.always_fail_field_up.reset();
        logger.events.clear();
        at(test_now_ms + config.discovery_interval_ms);
        std::ignore = poll_now(reader, event);
        CHECK(std::ranges::find(logger.events, Event { FieldUp { .antenna = 0 } }) != logger.events.end());
    }
}

namespace {

/// Drives the task loop off the fake clock and \returns, for each of the first \p passes discovery
/// passes, when it started and when it was over. Both timestamps come from the fake clock, so a
/// pass that had to arbitrate a contended field really does show up as a long one - which is the
/// whole point of what these tests measure.
std::vector<std::pair<uint32_t, uint32_t>> pass_windows(OPTBackend_NFCV &reader, EventLogger &logger, size_t passes) {
    std::vector<std::pair<uint32_t, uint32_t>> windows;
    OPTBackend::Event event;

    // Starts at 1 so that the very first pass clears RateLimiter's zero sentinel
    at(1);
    while (windows.size() < passes) {
        const uint32_t start = test_now_ms;
        logger.field_up_attempts = 0;
        std::ignore = poll_now(reader, event);
        if (logger.field_up_attempts > 0) {
            windows.emplace_back(start, test_now_ms);
        }

        // The loop runs far more often than the discovery cadence on hardware too
        at(test_now_ms + 1);
    }
    return windows;
}

} // namespace

TEST_CASE("NFC-V discovery counts its interval from the end of a pass", "[nfcv][openprinttag][field]") {
    constexpr uint32_t interval = 250;

    EventLogger logger;
    logger.tags[data::uid1] = TagData {
        .info = data::tag_info1,
        .antennas = { 0 },
    };

    OPTBackend_NFCV reader { logger, test_system, lifetime_config(interval) };

    SECTION("an instant pass keeps an exact cadence") {
        const auto windows = pass_windows(reader, logger, 4);

        for (size_t i = 0; i < windows.size(); i++) {
            CAPTURE(i, windows[i]);
            CHECK(windows[i].first == windows[i].second);
            if (i > 0) {
                CHECK(windows[i].first - windows[i - 1].first == interval);
            }
        }
    }

    SECTION("a pass that arbitrated pushes the next one back by what it spent") {
        // A neighbour holds the field for good, so every pass burns its whole arbitration window
        logger.always_fail_field_up = nfcv::Error::field_collision;

        const auto windows = pass_windows(reader, logger, 4);

        for (size_t i = 0; i < windows.size(); i++) {
            const auto [start, end] = windows[i];
            CAPTURE(i, start, end);

            // Without an arbitration that costs time there is nothing to drift by
            CHECK(end > start);

            if (i > 0) {
                CHECK(start == windows[i - 1].second + interval);
                CHECK(start - windows[i - 1].first > interval);
            }
        }
    }
}

TEST_CASE("NFC-V field lingers after an io op", "[nfcv][openprinttag][field]") {
    std::vector<std::byte> tag1(data::tag_info1.mem_size->block_size * data::tag_info1.mem_size->block_count);

    EventLogger logger;
    logger.tags[data::uid1] = TagData {
        .info = data::tag_info1,
        .data = &tag1,
        .antennas = { 0 },
    };

    OPTBackend_NFCV reader { logger, test_system, lifetime_config() };

    OPTBackend::Event event;
    REQUIRE(poll(reader, event, 1));
    logger.events.clear();

    std::array<std::byte, 2> buffer {};

    SECTION("the field survives the linger window and drops after it") {
        at(1000);
        REQUIRE(reader.read(0, 0, buffer));
        logger.events.clear();

        // The release armed the linger; a tick inside the window must not drop the field
        std::ignore = poll(reader, event, 1000 + linger_ms - 1);
        CHECK(count_field_downs(logger.events) == 0);

        std::ignore = poll(reader, event, 1000 + linger_ms);
        CHECK(count_field_downs(logger.events) == 1);
    }

    SECTION("a second op inside the window does not re-arbitrate") {
        at(1000);
        REQUIRE(reader.read(0, 0, buffer));
        logger.events.clear();

        at(1100);
        REQUIRE(reader.read(0, 0, buffer));
        // The driver's same-antenna early-out is what makes the second op cheap, but only if
        // the backend has not torn the field down in between
        CHECK(count_field_downs(logger.events) == 0);
    }

    SECTION("discovery is deferred while the linger is armed, and does not burn the limiter") {
        EventLogger paced_logger;
        paced_logger.tags[data::uid1] = TagData {
            .info = data::tag_info1,
            .data = &tag1,
            .antennas = { 0 },
        };

        OPTBackend_NFCV paced { paced_logger, test_system, lifetime_config(250) };

        REQUIRE(poll(paced, event, 1));

        // Arms the linger
        at(10);
        REQUIRE(paced.read(0, 0, buffer));

        paced_logger.events.clear();
        // Well past the discovery interval, but inside the linger window
        std::ignore = poll(paced, event, 10 + linger_ms - 1);
        CHECK(paced_logger.events.empty());

        // The linger expires and, because the limiter was never consumed while deferred,
        // the pass runs on the very same tick rather than waiting another full interval
        paced_logger.events.clear();
        std::ignore = poll(paced, event, 10 + linger_ms);
        CHECK(count_field_ups(paced_logger.events) == 1);
    }
}

TEST_CASE("NFC-V deliberate field drops clear the linger bookkeeping", "[nfcv][openprinttag][field]") {
    constexpr auto tag_uid = data::uid1_slix2;

    std::vector<std::byte> tag_data(4 * 64);

    EventLogger logger;
    logger.tags[tag_uid] = TagData {
        .info = { .mem_size = nfcv::TagInfo::MemorySize { .block_size = 4, .block_count = 64 } },
        .data = &tag_data,
        .antennas = { 0 },
    };

    OPTBackend_NFCV reader { logger, test_system, lifetime_config() };
    const auto config = reader.config();

    OPTBackend::Event event;
    REQUIRE(poll(reader, event, 1));
    logger.events.clear();

    /// A phantom linger armed over an already-dead field would be stamped by the first tick
    /// and fire a redundant drop on the second. No discovery can run at this cadence, so a
    /// clean bookkeeping state means no events at all.
    const auto check_no_phantom_linger = [&] {
        logger.events.clear();
        std::ignore = poll(reader, event, 1000);
        std::ignore = poll(reader, event, 1000 + linger_ms);
        CHECK(logger.events.empty());
    };

    SECTION("initialize_tag ends with the field down and no linger armed") {
        REQUIRE(reader.initialize_tag(0, OPTBackend::InitializeTagParams {}));
        REQUIRE(!logger.events.empty());
        CHECK(std::holds_alternative<FieldDown>(logger.events.back()));

        check_no_phantom_linger();
    }

    SECTION("unlock_tag ends with the field down and no linger armed") {
        REQUIRE(reader.unlock_tag(0, 0));
        REQUIRE(!logger.events.empty());
        CHECK(std::holds_alternative<FieldDown>(logger.events.back()));

        check_no_phantom_linger();
    }

    SECTION("the mid-op power cycle is a real down-then-up") {
        // Lock the tag first, so that a best-effort re-init hits the wrong-password path
        OPTBackend::InitializeTagParams params {
            .password = 0xdeadbeef,
            .protect_first_num_bytes = 32,
            .protection_policy = OPTBackend::InitializeTagParams::ProtectionPolicy::write_password,
        };
        REQUIRE(reader.initialize_tag(0, params));

        logger.events.clear();
        params.best_effort = true;
        REQUIRE(reader.initialize_tag(0, params));

        // A FieldDown immediately followed by a FieldUp - the tag power cycle must not be elided
        const auto down = std::ranges::find_if(logger.events, [](const Event &e) { return std::holds_alternative<FieldDown>(e); });
        REQUIRE(down != logger.events.end());
        REQUIRE(std::next(down) != logger.events.end());
        CHECK(std::holds_alternative<FieldUp>(*std::next(down)));
    }

    SECTION("disabling the radio clears the linger") {
        std::array<std::byte, 2> buffer {};
        at(1000);
        REQUIRE(reader.read(0, 0, buffer));

        logger.events.clear();
        auto disabled = config;
        disabled.enable_radio = false;
        reader.set_config(disabled);
        CHECK(count_field_downs(logger.events) == 1);

        logger.events.clear();
        std::ignore = poll(reader, event, 1000 + linger_ms);
        // The linger is gone, so no second drop over a dead field
        CHECK(count_field_downs(logger.events) == 0);
    }
}
