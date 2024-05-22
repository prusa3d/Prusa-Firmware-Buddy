#include "puppies/BootloaderProtocol.hpp"
#include <algorithm>
#include "Crc.h"
#include <cstring>
#include "buffered_serial.hpp"
#include "puppies/PuppyBus.hpp"
#include <functional>
#include <assert.h>

namespace buddy::puppies {

BootloaderProtocol::status_t BootloaderProtocol::write_command(commands_t cmd, uint8_t len) {
    assert(len <= MAX_REQUEST_DATA_LEN);

    PuppyBus::ErrorRecovery();
    PuppyBus::Flush();
    PuppyBus::EnsurePause();

    write_buffer[0] = current_address;
    write_buffer[1] = (uint8_t)cmd;
    // buffer[2] and onwards was prefilled with command data

    uint16_t crc = Crc16Ibm().update(write_buffer, len + 2).get();
    write_buffer[len + 2] = (crc & 0xff);
    write_buffer[len + 3] = (crc >> 8);

    if (!PuppyBus::Write(write_buffer, len + 4)) {
        return status_t::WRITE_ERROR;
    }
    return status_t::COMMAND_OK;
}

BootloaderProtocol::status_t BootloaderProtocol::read_status(uint8_t *datain, size_t datain_max_len, size_t *actualLen, uint32_t read_timeout_response) const {
    uint8_t buffer[3];

    size_t read = PuppyBus::Read(buffer, 3, read_timeout_response);
    if (read != 3) {
        if (read == 0) {
            return status_t::NO_RESPONSE;
        } else {
            return status_t::INCOMPLETE_RESPONSE;
        }
    }

    uint8_t addr = buffer[0];
    if (addr != current_address) {
        return status_t::BAD_RESPONSE;
    }

    uint8_t status = buffer[1];

    uint8_t len = buffer[2];
    if (len > MAX_RESPONSE_DATA_LEN || len > datain_max_len) {
        return status_t::BAD_RESPONSE;
    }
    if (actualLen != nullptr) {
        *actualLen = len;
    }

    if (len > 0) {
        read = PuppyBus::Read(datain, len, READ_TIMEOUT_PER_CHAR * len);
        if (read != len) {
            return status_t::INCOMPLETE_RESPONSE;
        }
    }

    uint8_t crc[2];
    read = PuppyBus::Read(crc, 2, READ_TIMEOUT_PER_CHAR * 2);
    if (read != 2) {
        return status_t::INCOMPLETE_RESPONSE;
    }

    uint16_t expectedCrc = Crc16Ibm().update(buffer, 3).update(datain, len).get();
    if (((crc[1] << 8) | crc[0]) != expectedCrc) {
        return status_t::INVALID_CRC;
    }

    return (status_t)status;
}

BootloaderProtocol::status_t BootloaderProtocol::run_transaction(commands_t cmd, uint8_t wr_data_len, uint8_t *read_data, size_t read_data_max, size_t *read_data_len, uint32_t read_timeout_response) {
    auto res = write_command(cmd, wr_data_len);
    if (res != status_t::COMMAND_OK) {
        return res;
    }

    res = read_status(read_data, read_data_max, read_data_len, read_timeout_response);
    if (res != status_t::COMMAND_OK) {
        return res;
    }

    return status_t::COMMAND_OK;
}

BootloaderProtocol::status_t BootloaderProtocol::write_flash_cmd(uint32_t offset, uint8_t len) {
    assert(len <= MAX_FLASH_BLOCK_LENGTH);
    uint8_t *cmd = get_run_transaction_buffer();

    cmd[0] = offset >> 24;
    cmd[1] = offset >> 16;
    cmd[2] = offset >> 8;
    cmd[3] = offset & 0xFF;
    // cmd[4] and onwards was prefilled with write data

    return run_transaction(commands_t::WRITE_FLASH, len + 4, nullptr, 0, nullptr, READ_TIMEOUT_FINALIZE_FLASH);
}

BootloaderProtocol::status_t BootloaderProtocol::read_flash_cmd(uint32_t offset, uint8_t *dataout, uint8_t len) {
    assert(len <= MAX_RESPONSE_DATA_LEN);
    uint8_t *cmd = get_run_transaction_buffer();

    cmd[0] = offset >> 24;
    cmd[1] = offset >> 16;
    cmd[2] = offset >> 8;
    cmd[3] = offset & 0xFF;
    cmd[4] = len;
    return run_transaction(commands_t::READ_FLASH, 5, dataout, len, nullptr, READ_TIMEOUT_RESPONSE_DEFAULT);
}

BootloaderProtocol::status_t BootloaderProtocol::read_otp_cmd(uint32_t offset, uint8_t *dataout, uint8_t len) {
    assert(len <= MAX_RESPONSE_DATA_LEN);
    uint8_t *cmd = get_run_transaction_buffer();

    cmd[0] = offset >> 24;
    cmd[1] = offset >> 16;
    cmd[2] = offset >> 8;
    cmd[3] = offset & 0xFF;
    cmd[4] = len;
    return run_transaction(commands_t::READ_OTP, 5, dataout, len, nullptr, READ_TIMEOUT_RESPONSE_DEFAULT);
}

BootloaderProtocol::status_t BootloaderProtocol::run_app(uint32_t salt, const fingerprint_t &fingerprint) {
    uint8_t *cmd = get_run_transaction_buffer();
    cmd[0] = static_cast<uint8_t>(salt >> 24);
    cmd[1] = static_cast<uint8_t>(salt >> 16);
    cmd[2] = static_cast<uint8_t>(salt >> 8);
    cmd[3] = static_cast<uint8_t>(salt);
    memcpy(&cmd[4], fingerprint.data(), fingerprint.size());

    size_t read_len = 0;
    uint8_t result;
    auto status = run_transaction(commands_t::START_APPLICATION, 4 + fingerprint.size(), &result, sizeof(result), &read_len);
    if ((status == status_t::COMMAND_OK)
        && ((read_len != 1) || (result != 0x01))) { // result == 1 means that puppy took salt and fingerprint and will not calculate sha again
        status = status_t::BAD_RESPONSE;
    }

    return status;
}

BootloaderProtocol::status_t BootloaderProtocol::get_protocolversion(uint16_t &version) {
    size_t read_len = 0;
    uint8_t version_reversed[2];
    auto status = run_transaction(commands_t::GET_PROTOCOL_VERSION, 0, version_reversed, sizeof(version_reversed), &read_len);
    if (status == status_t::COMMAND_OK && read_len != sizeof(version_reversed)) {
        status = status_t::BAD_RESPONSE;
    }
    version = version_reversed[0] << 8 | version_reversed[1]; // Reverse byte order
    return status;
}

BootloaderProtocol::status_t BootloaderProtocol::get_hwinfo(HwInfo &hw_info) {
    static const size_t EXPECTED_SIZE = 11;
    size_t read_len = 0;
    uint8_t hw_info_reversed[EXPECTED_SIZE];
    auto status = run_transaction(commands_t::GET_HARDWARE_INFO, 0, hw_info_reversed, EXPECTED_SIZE, &read_len);
    if (status == status_t::COMMAND_OK && read_len != EXPECTED_SIZE) {
        status = status_t::BAD_RESPONSE;
    }
    hw_info.hw_type = hw_info_reversed[0];
    hw_info.hw_revision = hw_info_reversed[1] << 8 | hw_info_reversed[2]; // Reverese byte order

    hw_info.bl_version = hw_info_reversed[3] << 24 | hw_info_reversed[4] << 16 | hw_info_reversed[5] << 8 | hw_info_reversed[6]; // Reverse byte order
    hw_info.application_size = hw_info_reversed[7] << 24 | hw_info_reversed[8] << 16 | hw_info_reversed[9] << 8 | hw_info_reversed[10]; // Reverse byte order
    return status;
}

BootloaderProtocol::status_t BootloaderProtocol::compute_fingerprint(uint32_t salt) {
    uint8_t *cmd = get_write_command_buffer();
    cmd[0] = static_cast<uint8_t>(salt >> 24);
    cmd[1] = static_cast<uint8_t>(salt >> 16);
    cmd[2] = static_cast<uint8_t>(salt >> 8);
    cmd[3] = static_cast<uint8_t>(salt);
    return write_command(commands_t::COMPUTE_FINGERPRINT, 4);
}

BootloaderProtocol::status_t BootloaderProtocol::get_fingerprint(fingerprint_t &fingerprint, uint8_t offset, uint8_t size) {
    if ((offset + size) > sizeof(fingerprint)) {
        return INVALID_ARGUMENTS;
    }

    uint8_t *cmd = get_run_transaction_buffer();
    cmd[0] = offset;
    cmd[1] = size;
    size_t read_len = 0;
    auto status = run_transaction(commands_t::GET_FINGERPRINT, 2, &fingerprint.data()[offset], size, &read_len);
    if (status == status_t::COMMAND_OK && read_len != size) {
        status = status_t::BAD_RESPONSE;
    }
    return status;
}

BootloaderProtocol::status_t BootloaderProtocol::write_flash(uint32_t len,
    std::function<bool(uint32_t offset, size_t size, uint8_t *out_data)> get_data) {
    assert(len <= MAX_FLASH_TOTAL_LENGTH);
    uint32_t offset = 0;

    while (offset < len) {
        size_t nextlen = std::min((size_t)MAX_FLASH_BLOCK_LENGTH, (size_t)(len - offset));

        uint8_t *buffer = get_write_flash_cmd_buffer();
        bool getDataRes = get_data(offset, nextlen, buffer);
        if (!getDataRes) {
            return status_t::READ_DATA_ERROR;
        }

        auto res = write_flash_cmd(offset, nextlen);
        if (res != status_t::COMMAND_OK) {
            return res;
        }

        offset += nextlen;
    }

    uint8_t erase_count = 0;
    size_t response_len = 0;
    auto res = run_transaction(commands_t::FINALIZE_FLASH, 0, &erase_count, sizeof(erase_count), &response_len, READ_TIMEOUT_FINALIZE_FLASH);
    if (res != status_t::COMMAND_OK) {
        return res;
    }

    if (response_len != sizeof(erase_count)) {
        return status_t::BAD_RESPONSE;
    }

    return status_t::COMMAND_OK;
}

BootloaderProtocol::status_t BootloaderProtocol::assign_address(Address current_address, Address new_address) {
    set_address(current_address);
    uint8_t *cmd = get_write_command_buffer();
    cmd[0] = static_cast<uint8_t>(new_address);
    cmd[1] = 0;
    auto res = write_command(commands_t::SET_ADDRESS, 2);
    return res;
}

} // namespace buddy::puppies
