#pragma once
#include <stdint.h>
#include <functional>

namespace buddy {
namespace puppies {

    class BootloaderProtocol {
    public:
        /// States of bootloader protocol, and result codes of opreations
        enum status_t {

            // Actual statuses sent inside messagess:
            COMMAND_OK = 0x00,
            COMMAND_FAILED = 0x01,
            COMMAND_NOT_SUPPORTED = 0x02,
            INVALID_TRANSFER = 0x03,
            INVALID_CRC = 0x04,
            INVALID_ARGUMENTS = 0x05,

            // internal errors (caused by IO error etc, never actually transmitted via communication):
            WRITE_ERROR = 0x11,
            NO_RESPONSE = 0x12,
            INCOMPLETE_RESPONSE = 0x13,
            BAD_RESPONSE = 0x14,
            READ_DATA_ERROR = 0x15,
        };

        /// List of bootloader commands
        enum commands_t : uint8_t {
            GET_PROTOCOL_VERSION = 0x00,
            SET_ADDRESS = 0x01,
            GET_HARDWARE_INFO = 0x03,
            START_APPLICATION = 0x05,
            WRITE_FLASH = 0x06,
            FINALIZE_FLASH = 0x07,
            READ_FLASH = 0x08,
            GET_MAX_PACKET_LENGTH = 0x0c,
            GET_FINGERPRINT = 0x0e,
            COMPUTE_FINGERPRINT = 0x0f,
            READ_OTP = 0x10,

            // These were removed and should not be used
            RESERVED_02 = 0x02, ///< POWER_UP_DISPLAY
            RESERVED_04 = 0x04, ///< GET_SERIAL_NUMBER
            RESERVED_09 = 0x09, ///< GET_HARDWARE_REVISION
            RESERVED_0A = 0x0a, ///< GET_NUM_CHILDREN
            RESERVED_0B = 0x0b, ///< SET_CHILD_SELECT
            RESERVED_0D = 0x0d, ///< GET_EXTRA_INFO
            RESERVED_46 = 0x46, ///< RESET
            RESERVED_44 = 0x44, ///< RESET_ADDRESS
            END_OF_commands_t
        };

        /// Addresses (on puppy bus) that each puppy uses
        enum Address : uint8_t {
            DEFAULT_ADDRESS = 0x00, //< address that puppies will have after boot to bootloader
            FIRST_ASSIGNED = 0x0A, //< First assigned address in bootloader

            MODBUS_OFFSET = 0x1A, //< Address offset in modbus, this is the first assigned address after jump to app
        };

        /// Hardware info, used in response to GET_HARDWARE_INFO
        struct HwInfo {
            uint8_t hw_type; ///< Hardware type as in puppy_info
            uint16_t hw_revision; ///< Hardware revision
            uint32_t bl_version; ///< Bootloader version
            uint32_t application_size; ///< Space for application and fixed application size
        };

        /// Firmware fingerprint
        using fingerprint_t = std::array<uint8_t, 32>;

        /**
         * @brief Constructor.
         * @param buffer buffer for bootloader protocol
         * Buffer needs to be in regular RAM as it is used by DMA.
         * Buffer needs to be at least MAX_PACKET_LENGTH bytes large.
         */
        BootloaderProtocol(uint8_t *buffer)
            : write_buffer(buffer) {
            assert(write_buffer != nullptr);
        }

        ~BootloaderProtocol() {
        }

        /**
         * @brief Get buffer for write_command().
         * @return buffer where command can be put, size is MAX_REQUEST_DATA_LEN bytes
         */
        uint8_t *get_write_command_buffer() { return write_buffer + 2; };

        /**
         * @brief Send specified command
         * @note Data to send need to be stored in buffer given by call to get_write_command_buffer().
         * @param cmd command to send
         * @param len length of data stored in get_write_command_buffer() buffer
         * @return status_t
         */
        status_t write_command(commands_t cmd, uint8_t len);

        /**
         * @brief Read command result
         *
         * @param datain Buffer for received data
         * @param datain_max_len Max received data len
         * @param actualLen Actually received data length
         * @return status_t
         */
        status_t read_status(uint8_t *datain, size_t datain_max_len, size_t *actualLen = nullptr, uint32_t read_timeout_response = READ_TIMEOUT_RESPONSE_DEFAULT) const;

        /**
         * @brief Get buffer for run_transaction().
         * @return buffer where command can be put, size is MAX_REQUEST_DATA_LEN bytes
         */
        uint8_t *get_run_transaction_buffer() { return get_write_command_buffer(); };

        /**
         * @brief Send command, wait for result
         *
         * @note Data to send need to be stored in buffer given by call to get_write_command_buffer().
         * @param cmd Command to send
         * @param wr_data_len Length of data to write with command
         * @param read_data Buffer for received data
         * @param read_data_max Received data max length
         * @param read_data_len Received data actual length
         * @return status_t Result
         */
        status_t run_transaction(commands_t cmd, uint8_t wr_data_len, uint8_t *read_data = nullptr, size_t read_data_max = 0,
            size_t *read_data_len = nullptr, uint32_t read_timeout_response = READ_TIMEOUT_RESPONSE_DEFAULT);

        /**
         * @brief Get buffer for write_flash_cmd().
         * @return buffer where data to write can be put, size is MAX_FLASH_BLOCK_LENGTH bytes
         */
        uint8_t *get_write_flash_cmd_buffer() { return get_run_transaction_buffer() + 4; };

        /**
         * @brief Send command "WRITE_FLASH"
         *
         * @note Data to send need to be stored in buffer given by call to get_write_flash_buffer().
         * @param offset Offset of data to write
         * @param len Length of data to write
         * @return status_t
         */
        status_t write_flash_cmd(uint32_t offset, uint8_t len);

        /**
         * @brief Send command "READ_FLASH"
         *
         * @param offset Offset of data to read
         * @param data Buffer for read data
         * @param len Length of data to read
         * @return status_t
         */
        status_t read_flash_cmd(uint32_t offset, uint8_t *data, uint8_t len);

        /**
         * @brief Send command "READ_OTP"
         * This command is similar to read FLASH, but reads raw OTP area.
         * This command was added to protocol in version 0x0302.
         *
         * @param offset Offset of data to read
         * @param data Buffer for read data
         * @param len Length of data to read
         * @return status_t
         */
        status_t read_otp_cmd(uint32_t offset, uint8_t *dataout, uint8_t len);

        /**
         * @brief Do entire procedure of writing to flash, including flash finalize
         *
         * @param len Length of data to write
         * @param get_data Callback that will be called to obtain data for flashing
         * @return status_t operation result
         */
        status_t write_flash(uint32_t len, std::function<bool(uint32_t offset, size_t size, uint8_t *out_data)> get_data);

        /**
         * @brief Start application
         *
         * @param salt use this salt for fingerprint calculation
         * @param fingerprint puppy will check this fingerprint before starting the app
         * @return status_t  Result
         */
        status_t run_app(uint32_t salt, const fingerprint_t &fingerprint);

        /**
         * @brief Send command GET_PROTOCOL_VERSION
         *
         * @param version result version
         * @return status_t
         */
        status_t get_protocolversion(uint16_t &version);

        /**
         * @brief Send command GET_HARDWARE_INFO
         *
         * @param hw_info result data
         * @return status_t
         */
        status_t get_hwinfo(HwInfo &hw_info);

        /**
         * @brief Send command GET_HARDWARE_REVISION.
         *
         * @param revision result revision number
         * @return status_t
         */
        status_t get_hwrevision(uint8_t &revision) const;

        /**
         * @brief Compute fingerprint of application FW
         *
         * This asks the bootloader to start fingerprint computation.
         *
         * @param salt use this salt, added to sha before the application
         * @return status_t
         */
        status_t compute_fingerprint(uint32_t salt);

        /**
         * @brief Get fingerprint of application FW
         *
         * Note: This requires the fingerprint has been already computed by
         * a call to compute_fingerprint and enough time has been given to
         * bootloader to finish fingerprint computation.
         *
         * @param data result data
         * @param offset get only a chunk starting at offset
         * @param size get only a chunk of size bytes
         * @return status_t
         */
        status_t get_fingerprint(fingerprint_t &fingerprint, uint8_t offset = 0, uint8_t size = sizeof(fingerprint_t));

        /**
         * @brief Set currently used address (see Address)
         *
         * @param address
         */
        inline void set_address(Address address) {
            current_address = address;
        }

        /**
         * @brief Assign address to puppies on given current address
         *
         * @param current_address
         * @param new_address
         */
        status_t assign_address(Address current_address, Address new_address);

        static constexpr uint16_t MAX_PACKET_LENGTH = 255;
        static constexpr uint16_t MAX_REQUEST_DATA_LEN = MAX_PACKET_LENGTH - 4;
        static constexpr uint16_t MAX_RESPONSE_DATA_LEN = MAX_PACKET_LENGTH - 5;
        static constexpr uint16_t MAX_FLASH_BLOCK_LENGTH = MAX_REQUEST_DATA_LEN - 4; // 4B for offset

        // Protocol major version must match with the same number in bootloader
        static constexpr uint16_t BOOTLOADER_PROTOCOL_VERSION = 0x0302;

    private:
        uint8_t current_address = 0x08;

        uint8_t *write_buffer; ///< Buffer to transmit data

        static constexpr uint32_t MAX_FLASH_TOTAL_LENGTH = ((128 * 1024) - 8192);

        static constexpr uint32_t READ_TIMEOUT_RESPONSE_DEFAULT = 100; // Default time to wait until response starts coming
        static constexpr uint32_t READ_TIMEOUT_PER_CHAR = 1; // Timeout for each byte that will be received
        static constexpr uint32_t READ_TIMEOUT_FINALIZE_FLASH = 500; // timeout for finalize flash command (sha256 fingerprint is calculated, so it takes longer)
    };

} // namespace puppies
} // namespace buddy
