// dump.h
#pragma once
extern "C" {
#include "CrashCatcher.h"
}

namespace crash_dump {

/// Dump types and flags (when set to zero, flag is active)
enum class DumpFlags : uint8_t {
    EXPORTED = 0x01, ///< dump not exported to usb flash or send via internet
    DISPL = 0x02, ///< dump not displayed after startup
    DEFAULT = 0xFF, ///< Initial value when crash dump is created
};

/// Codes for the message type item of message struct
enum class MsgType : uint8_t {
    RSOD = 0, ///< Red screen of death
    IWDGW = 2, // IWDG warning
    BSOD = 3, // BSOD dump
    STACK_OVF = 4, // stack overflow dump

    EMPTY = 0xff, ///< Nothing dumped
};

inline DumpFlags operator|(const DumpFlags a, const DumpFlags b) {
    return DumpFlags(ftrstd::to_underlying(a) | ftrstd::to_underlying(b));
}

inline DumpFlags operator&(const DumpFlags a, const DumpFlags b) {
    return DumpFlags(ftrstd::to_underlying(a) & ftrstd::to_underlying(b));
}

inline DumpFlags operator~(const DumpFlags a) {
    return DumpFlags(~ftrstd::to_underlying(a));
}

inline bool any(const DumpFlags a) {
    return ftrstd::to_underlying(a);
}

// DUMP constants for error message
inline constexpr size_t MSG_TITLE_MAX_LEN { 40 };
inline constexpr size_t MSG_MAX_LEN { 140 };

/**
 * @brief Check if dump is valid
 */
bool dump_is_valid();

/**
 * @brief Check if dump was saved to
 * @return true for saved
 */
bool dump_is_exported();

/**
 * @brief Check if dump was already displayed.
 * @return true for displayed
 */
bool dump_is_displayed();

/**
 * @brief Return size of dump data
 */
size_t dump_get_size();

/**
 * @brief Erase dump.
 */
void dump_reset();

/**
 * @brief Set dump as displayed.
 */
void dump_set_displayed();

/**
 * @brief Set dump as exported
 */
void dump_set_exported();

/**
 * @brief Store dump to USB.
 * @param fn Filename to store dump to
 * @return true on success
 */
bool save_dump_to_usb(const char *fn);

/**
 * @brief Read data of dump
 */
bool dump_read_data(size_t offset, size_t size, uint8_t *ptr);

/**
 * @brief Dump error message to XFLASH.
 * This is used for redscreen error message and for BSOD error message.
 * @param invalid RSOD or BSOD, erased value means message invalid
 * @param error_code enum from the error list, not used for BSOD (use 0 or ERR_UNDEF instead)
 * @note This uses uint16_t to minimize dependencies. Include <error_codes.hpp> and use ftrstd::to_underlying(ERR_WHATEVER).
 * @param error longer error message
 * @param title shorter error title, or file and line for BSOD
 */
void save_message(MsgType invalid, uint16_t error_code, const char *error, const char *title);

/**
 * @brief Copy error message from XFLASH.
 * @param msg_dst [out] - will be filled with address to dumped error message
 * @param msg_dst_size [in] - size of passed message buffer
 * @param title_dst [out] - will be filled with address to dumped error title
 * @param msg_dst_size [in] - size of passed title buffer
 * @retval true - valid read
 * @retval false - read error occurred
 */
bool load_message(char *msg_dst, size_t msg_dst_size, char *tit_dst, size_t tit_dst_size);

/**
 * @brief Check if message in flash is valid
 */
bool message_is_valid();

/**
 * @brief Check if error message in XFLASH is valid.
 * @return type of the error message structure or EMPTY
 */
MsgType message_get_type();

/**
 * @brief Returns if error message was already displayed.
 * @return true if it was
 */
bool message_is_displayed();

/**
 * @brief Set message as displayed.
 */
void message_set_displayed();

/**
 * @brief Get RSOD error code.
 * @note This returns uint16_t to minimize dependencies. Include <error_codes.hpp> and use ErrCode(load_message_error_code()).
 * @return error code,
 */
uint16_t load_message_error_code();

/**
 * Function that should be called before error messages & dumps are issued.
 * It will trigger breakpoint if necessary and put printer to safe state
 */
void before_dump();

[[noreturn]] void trigger_crash_dump();
} // namespace crash_dump
