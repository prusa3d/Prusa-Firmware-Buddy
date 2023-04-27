// otp.h - OTP memory mapping
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#pragma pack(push, 1)
typedef struct {
    uint16_t product_id;
    uint8_t revision;
    uint32_t supplier_id;
    uint16_t production_year;
    uint8_t production_month;
    uint8_t production_day;
    uint32_t date_serial_number;
} datamatrix_t;

typedef struct {
    uint8_t uuid[12];
} STM32_UUID;

typedef struct {
    uint8_t mac[6];
} MAC_addr;

typedef union {
    uint16_t br;
    uint8_t bytes[2];
} board_revision_t;

typedef struct {
    char txt[25];
} serial_nr_t;

#pragma pack(pop)

/// @param datamatrix structure to be filled with binary datamatrix representation (decompiled from textual information in OTP)
/// @returns true if successfully extracted (OTP v3 and newer), false otherwise
bool otp_get_datamatrix(datamatrix_t *datamatrix);

/// @param revision - 2 bytes of board revision (no longer 3 bytes since OTP v3 and needs to be unified with representation of older boards)
/// @returns true if successfully extracted (OTP old, v3 and newer),
///          false means the structure was not filled with valid data due to some internal error
bool otp_get_board_revision(board_revision_t *revision);

/// @returns UNIX Timestamp from 1970 (uint32_t little endian) - valid for all OTP data structures
uint32_t otp_get_timestamp();

/// @returns unique identifier of the STM32 CPU - 96bits
const STM32_UUID *otp_get_STM32_UUID();

/// @returns MAC address - valid for all OTP data structures
const MAC_addr *otp_get_mac_address();

/// @returns MAC address formatted as "XX:XX:XX:XX:XX:XX" string
const char *otp_get_mac_address_str();

/// Reads textual representation of the board's serial number.
/// Old boards have only 16 bytes (15 characters + zero terminated)
/// OTP v3 and newer have 24 bytes characters + zero termination is automatically added inside this method.
/// @returns number of valid bytes in the serial number structure including the '\0' at the end.
///          0 is returned in case of an internal error (the sn structure is not filled with valid data)
uint8_t otp_get_serial_nr(serial_nr_t *sn);

/**
 * @brief Get BOM ID from OTP.
 * @param bom_id output BOM ID
 * @return true if parsed correctly, false otherwise
 */
bool otp_get_bom_id(uint8_t *bom_id);

///----------------------------------------------------
/// @note Following functions parse OTP from external memory.
/// It can be used to parse OTP from puppies.

/**
 * @brief Parse datamatrix from external OTP memory.
 * @param datamatrix structure to be filled with binary datamatrix representation (decompiled from textual information in OTP)
 * @param memory parse from this memory
 * @param len span of valid memory in bytes
 * @returns true if successfully extracted (OTP v3 and newer), false otherwise
 */
bool otp_parse_datamatrix(datamatrix_t *datamatrix, const uint8_t *memory, size_t len);

/**
 * @brief Parse serial number (raw text of datamatrix) into binary datamatrix structure.
 * @param datamatrix output structure to be filled with binary datamatrix representation
 * @param sn input text with serial number
 * @return true if parsed correctly, false otherwise
 */
bool otp_serial_nr_to_datamatrix(datamatrix_t *datamatrix, const serial_nr_t *sn);

/**
 * @brief Parse board revision from external OTP memory.
 * @param revision output, 2 bytes of board revision (no longer 3 bytes since OTP v3 and needs to be unified with representation of older boards)
 * @param memory parse from this memory
 * @param len span of valid memory in bytes
 * @returns true if successfully extracted (OTP old, v3 and newer),
 *          false means the structure was not filled with valid data due to some internal error
 */
bool otp_parse_board_revision(board_revision_t *revision, const uint8_t *memory, size_t len);

/**
 * @brief Parse timestamp from external OTP memory.
 * @param timestamp output, UNIX Timestamp from 1970 (uint32_t little endian) - valid for all OTP data structures
 * @param memory parse from this memory
 * @param len span of valid memory in bytes
 * @return true if parsed correctly, false otherwise
 */
bool otp_parse_timestamp(uint32_t *timestamp, const uint8_t *memory, size_t len);

/**
 * @brief Parse MAC address from external OTP memory.
 * @param memory parse from this memory
 * @param len span of valid memory in bytes
 * @returns pointer to memory where MAC address is, or NULL if not parsed correctly
 */
const MAC_addr *otp_parse_mac_address(const uint8_t *memory, size_t len);

/**
 * @brief Parse serial number from external OTP memory.
 * @param sn output serial number string
 * @param memory parse from this memory
 * @param len span of valid memory in bytes
 * @return number of valid bytes in the serial number structure including the '\0' at the end.
        0 is returned in case of an internal error (the sn structure is not filled with valid data).
 */
uint8_t otp_parse_serial_nr(serial_nr_t *sn, const uint8_t *memory, size_t len);

/**
 * @brief Parse BOM ID from external OTP memory.
 * @param bom_id output BOM ID
 * @param memory parse from this memory
 * @param len span of valid memory in bytes
 * @return true if parsed correctly, false otherwise
 */
bool otp_parse_bom_id(uint8_t *bom_id, const uint8_t *memory, size_t len);

#ifdef __cplusplus
}
#endif
