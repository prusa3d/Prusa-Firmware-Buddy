#pragma once

#include "error.hpp"
#include "types.hpp"
#include "commands.hpp"

#include <cstddef>
#include <expected>

#include <utils/uncopyable.hpp>
#include <utils/byte_utils.hpp>

namespace nfcv {

class ReaderWriterInterface {
public:
    using AntennaID = uint8_t;

    virtual Result<void> field_up(AntennaID antenna) = 0;
    virtual void field_down() = 0;
    virtual AntennaID antenna_count() const = 0;

    [[nodiscard]] virtual nfcv::Result<void> nfcv_command(const Command &command) = 0;

public: //* Utility wrappers for nfcv commands
    enum class Register {
        /// 8-bit
        afi,

        /// 8-bit
        /// NOTE: cannot be password protected - gets hard locked instead
        dsfid,

        /// 1-bit (set/reset)
        eas,

        /// 32-bit
        /// NOTE: SLIX2 extension
        read_password,

        /// 32-bit
        /// NOTE: SLIX2 extension
        write_password,

        /// 32-bit
        /// NOTE: SLIX2 extension
        privacy_password,

        /// 32-bit
        /// NOTE: SLIX2 extension
        destroy_password,

        /// 32-bit
        /// NOTE: SLIX2 extension
        eas_afi_password,
    };

    static SLIX2PasswordID to_password_id(Register reg);

    enum class LockMode {
        /// Irreversibly locks the data, cannot be ever unlocked
        hard_lock,

        /// Protects the data with a password
        /// Assumes a relevant password is already set using \p set_password
        /// * Password protection is a SLIX2 extension
        /// !!! Some registers (DSFID) cannot be password-protected and will be locked instead
        password_protect,
    };

    Result<UID> inventory();
    Result<void> stay_quiet(const UID &uid);
    Result<TagInfo> get_system_info(const UID &uid);
    Result<void> read_single_block(const UID &uid, BlockID block_id, const WritableBytes &buffer);
    Result<void> write_single_block(const UID &uid, BlockID block_id, const Bytes &buffer);

    /// Writes to a specified register
    /// For passwords, this means changing the password. "Logging in" with the password is done with \p set_password
    /// !!! The register can be smaller than the 4-byte \p value
    Result<void> write_register(const UID &uid, Register reg, uint32_t value);

    /// Locks the specified register according to the specified policy
    /// NOTE: Password protection is a SLIX2 extension
    /// NOTE: \p password_protect locking is shared for AFI & EAS
    /// NOTE: \p dsfid cannot be password protected - gets hard locked instead
    Result<void> lock_register(const UID &uid, Register reg, LockMode mode);

    /// Sets appropriate passwords.
    /// NOTE: Password protection is a SLIX2 extension
    /// \returns \p Error::bad_request if you provide a different register than a password one
    Result<void> set_password(const UID &uid, Register reg, uint32_t value);
};

} // namespace nfcv
