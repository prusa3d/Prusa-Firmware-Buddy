#include <nfcv/rw_interface.hpp>

#include <limits>
#include <utils/byte_utils.hpp>

using namespace nfcv;

SLIX2PasswordID ReaderWriterInterface::to_password_id(Register reg) {
    switch (reg) {

    case Register::read_password:
        return SLIX2PasswordID::read;

    case Register::write_password:
        return SLIX2PasswordID::write;

    case Register::privacy_password:
        return SLIX2PasswordID::privacy;

    case Register::destroy_password:
        return SLIX2PasswordID::destroy;

    case Register::eas_afi_password:
        return SLIX2PasswordID::eas_afi;

    case Register::afi:
    case Register::dsfid:
    case Register::eas:
        // Fallback to unreachable
        break;
    }

    std::unreachable();
}

Result<UID> ReaderWriterInterface::inventory() {
    command::Inventory::Response response;
    Command cmd { command::Inventory {
        .request {},
        .response { response },
    } };
    const auto res = nfcv_command(cmd);
    if (!res.has_value()) {
        return std::unexpected(res.error());
    }
    return { response };
}

Result<void> ReaderWriterInterface::stay_quiet(const UID &uid) {
    nfcv::Command cmd { nfcv::command::StayQuiet {
        .request { .uid = uid },
    } };
    return nfcv_command(cmd);
}

Result<TagInfo> ReaderWriterInterface::get_system_info(const UID &uid) {
    command::SystemInfo::Response response;
    nfcv::Command cmd { nfcv::command::SystemInfo {
        .request { .uid = uid },
        .response { response },
    } };
    const auto res = nfcv_command(cmd);
    if (!res.has_value()) {
        return std::unexpected(res.error());
    }
    return { response };
}

Result<void> ReaderWriterInterface::read_single_block(const UID &uid, BlockID block_id, const WritableBytes &buffer) {
    nfcv::Command cmd { nfcv::command::ReadSingleBlock {
        .request { .uid = uid, .block_address = block_id },
        .response { buffer },
    } };
    return nfcv_command(cmd);
}

Result<void> ReaderWriterInterface::write_single_block(const UID &uid, BlockID block_id, const Bytes &buffer) {
    nfcv::Command cmd { nfcv::command::WriteSingleBlock {
        .request { .uid = uid, .block_address = block_id, .block_buffer = buffer },
    } };
    return nfcv_command(cmd);
}

Result<void> ReaderWriterInterface::write_register(const UID &uid, Register reg, uint32_t value) {
    switch (reg) {

    case Register::afi: {
        if (value > std::numeric_limits<AFI>::max()) {
            return std::unexpected(Error::bad_request);
        }
        nfcv::Command cmd { nfcv::command::WriteAFI {
            .request {
                .uid = uid,
                .afi = static_cast<AFI>(value),
            },
        } };
        return nfcv_command(cmd);
    }

    case Register::dsfid: {
        if (value > std::numeric_limits<DSFID>::max()) {
            return std::unexpected(Error::bad_request);
        }
        nfcv::Command cmd { nfcv::command::WriteDSFID {
            .request {
                .uid = uid,
                .dsfid = static_cast<DSFID>(value),
            },
        } };
        return nfcv_command(cmd);
    }

    case Register::eas: {
        switch (value) {

        case 0: {
            nfcv::Command cmd { nfcv::command::ResetEAS { .request { uid } } };
            return nfcv_command(cmd);
        }

        case 1: {
            nfcv::Command cmd { nfcv::command::SetEAS { .request { uid } } };
            return nfcv_command(cmd);
        }

        default:
            return std::unexpected(Error::bad_request);
        }
    }

    case Register::read_password:
    case Register::write_password:
    case Register::privacy_password:
    case Register::destroy_password:
    case Register::eas_afi_password: {
        nfcv::Command cmd { nfcv::command::WritePassword {
            .request {
                .uid = uid,
                .password_id = to_password_id(reg),
                .password = value,
            },
        } };
        return nfcv_command(cmd);
    }
    }
    return std::unexpected(Error::bad_request);
}

Result<void> ReaderWriterInterface::lock_register(const UID &uid, Register reg, LockMode mode) {
    switch (reg) {

    case Register::afi:
        switch (mode) {
        case LockMode::hard_lock:
            return std::unexpected(Error::not_implemented);

        case LockMode::password_protect:
            return nfcv_command(nfcv::command::PasswordProtectEASAFI { { .uid = uid, .option = nfcv::command::PasswordProtectEASAFI::Request::Option::afi } });
        }
        // Fallback to unreachable
        break;

    case Register::eas:
        switch (mode) {
        case LockMode::hard_lock:
            return std::unexpected(Error::not_implemented);

        case LockMode::password_protect:
            return nfcv_command(nfcv::command::PasswordProtectEASAFI { { .uid = uid, .option = nfcv::command::PasswordProtectEASAFI::Request::Option::eas } });
        }
        // Fallback to unreachable
        break;

    case Register::dsfid:
        switch (mode) {
        case LockMode::hard_lock:
            return nfcv_command(nfcv::command::LockDSFID { { .uid = uid } });

        case LockMode::password_protect:
            // Cannot be password protected, can only be hard locked
            return std::unexpected(Error::bad_request);
        }
        // Fallback to unreachable
        break;

    case Register::read_password:
    case Register::write_password:
    case Register::privacy_password:
    case Register::destroy_password:
    case Register::eas_afi_password:
        return std::unexpected(Error::not_implemented);
    }
    return std::unexpected(Error::bad_request);
}

Result<void> ReaderWriterInterface::set_password(const UID &uid, Register reg, uint32_t value) {
    command::GetRandomNumber::Response random_number;
    Command rnd_cmd { command::GetRandomNumber { .request { uid }, .response = random_number } };
    if (auto r = nfcv_command(rnd_cmd); !r) {
        return std::unexpected(r.error());
    }

    static_assert(std::is_same_v<decltype(random_number), uint16_t>);
    static_assert(std::is_same_v<SLIX2Password, uint32_t>);
    const uint32_t encoded_pwd = value ^ (random_number | (random_number << 16));

    nfcv::Command cmd { nfcv::command::SetPassword {
        .request {
            .uid = uid,
            .password_id = to_password_id(reg),
            .password = encoded_pwd,
        },
    } };
    return nfcv_command(cmd);
}
