#include "puppies/BootloaderProtocol.hpp"
#include <algorithm>
#include "Crc.h"
#include <cstring>
#include "buffered_serial.hpp"
#include "puppies/PuppyBus.hpp"
#include <functional>
#include <assert.h>

namespace buddy::puppies {

BootloaderProtocol::status_t BootloaderProtocol::write_command(commands_t cmd, uint8_t *dataout, uint8_t len) const {
    assert(len <= MAX_REQUEST_DATA_LEN);

    PuppyBus::ErrorRecovery();
    PuppyBus::Flush();
    PuppyBus::EnsurePause();

    uint8_t buffer[MAX_PACKET_LENGTH];
    size_t pos = 0;
    buffer[pos++] = current_address;
    buffer[pos++] = (uint8_t)cmd;

    if (len > 0) {
        std::memcpy(&buffer[2], dataout, len);
        pos += len;
    }

    uint16_t crc = Crc16Ibm().update(current_address).update(cmd).update(dataout, len).get();
    buffer[pos++] = (crc & 0xff);
    buffer[pos++] = (crc >> 8);

    if (!PuppyBus::Write(buffer, pos)) {
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

BootloaderProtocol::status_t BootloaderProtocol::run_transaction(commands_t cmd, uint8_t *wr_data, uint8_t wr_data_len, uint8_t *read_data, size_t read_data_max, size_t *read_data_len, uint32_t read_timeout_response) const {
    auto res = write_command(cmd, wr_data, wr_data_len);
    if (res != status_t::COMMAND_OK)
        return res;

    res = read_status(read_data, read_data_max, read_data_len, read_timeout_response);
    if (res != status_t::COMMAND_OK)
        return res;

    return status_t::COMMAND_OK;
}

BootloaderProtocol::status_t BootloaderProtocol::write_flash_cmd(uint32_t offset, uint8_t *data, uint8_t len) const {
    assert(len <= MAX_FLASH_BLOCK_LENGTH);
    uint8_t dataout[MAX_REQUEST_DATA_LEN];

    dataout[0] = offset >> 24;
    dataout[1] = offset >> 16;
    dataout[2] = offset >> 8;
    dataout[3] = offset & 0xFF;
    memcpy(dataout + 4, data, len);
    return run_transaction(commands_t::WRITE_FLASH, dataout, len + 4, nullptr, 0, nullptr, READ_TIMEOUT_FINALIZE_FLASH);
}

BootloaderProtocol::status_t BootloaderProtocol::read_flash_cmd(uint32_t offset, uint8_t *dataout, uint8_t len) const {
    assert(len <= MAX_RESPONSE_DATA_LEN);
    uint8_t datain[5];

    datain[0] = offset >> 24;
    datain[1] = offset >> 16;
    datain[2] = offset >> 8;
    datain[3] = offset & 0xFF;
    datain[4] = len;
    return run_transaction(commands_t::READ_FLASH, datain, 5, dataout, len, nullptr, READ_TIMEOUT_RESPONSE_DEFAULT);
}

BootloaderProtocol::status_t BootloaderProtocol::read_otp_cmd(uint32_t offset, uint8_t *dataout, uint8_t len) const {
    assert(len <= MAX_RESPONSE_DATA_LEN);
    uint8_t datain[5];

    datain[0] = offset >> 24;
    datain[1] = offset >> 16;
    datain[2] = offset >> 8;
    datain[3] = offset & 0xFF;
    datain[4] = len;
    return run_transaction(commands_t::READ_OTP, datain, 5, dataout, len, nullptr, READ_TIMEOUT_RESPONSE_DEFAULT);
}

BootloaderProtocol::status_t BootloaderProtocol::run_app(uint32_t salt, const fingerprint_t &fingerprint) {
    uint8_t buffer[sizeof(salt) + sizeof(fingerprint)] = { static_cast<uint8_t>(salt >> 24), static_cast<uint8_t>(salt >> 16), static_cast<uint8_t>(salt >> 8), static_cast<uint8_t>(salt) };
    memcpy(&buffer[4], fingerprint.data(), fingerprint.size());

    size_t read_len = 0;
    uint8_t result;
    auto status = run_transaction(commands_t::START_APPLICATION, buffer, sizeof(buffer), &result, sizeof(result), &read_len);
    if ((status == status_t::COMMAND_OK)
        && ((read_len != 1) || (result != 0x01))) { // result == 1 means that puppy took salt and fingerprint and will not calculate sha again
        status = status_t::BAD_RESPONSE;
    }

    return status;
}

BootloaderProtocol::status_t BootloaderProtocol::get_protocolversion(uint16_t &version) const {
    size_t read_len = 0;
    uint8_t version_reversed[2];
    auto status = run_transaction(commands_t::GET_PROTOCOL_VERSION, nullptr, 0, version_reversed, sizeof(version_reversed), &read_len);
    if (status == status_t::COMMAND_OK && read_len != sizeof(version_reversed)) {
        status = status_t::BAD_RESPONSE;
    }
    version = version_reversed[0] << 8 | version_reversed[1]; //Reverse byte order
    return status;
}

BootloaderProtocol::status_t BootloaderProtocol::get_hwinfo(HwInfo &hw_info) const {
    static const size_t EXPECTED_SIZE = 11;
    size_t read_len = 0;
    uint8_t hw_info_reversed[EXPECTED_SIZE];
    auto status = run_transaction(commands_t::GET_HARDWARE_INFO, nullptr, 0, hw_info_reversed, EXPECTED_SIZE, &read_len);
    if (status == status_t::COMMAND_OK && read_len != EXPECTED_SIZE) {
        status = status_t::BAD_RESPONSE;
    }
    hw_info.hw_type = hw_info_reversed[0];
    hw_info.hw_revision = hw_info_reversed[1] << 8 | hw_info_reversed[2]; //Reverese byte order

    hw_info.bl_version = hw_info_reversed[3] << 24 | hw_info_reversed[4] << 16 | hw_info_reversed[5] << 8 | hw_info_reversed[6];        //Reverse byte order
    hw_info.application_size = hw_info_reversed[7] << 24 | hw_info_reversed[8] << 16 | hw_info_reversed[9] << 8 | hw_info_reversed[10]; //Reverse byte order
    return status;
}

BootloaderProtocol::status_t BootloaderProtocol::compute_fingerprint(uint32_t salt) const {
    uint8_t buffer[4] = { static_cast<uint8_t>(salt >> 24), static_cast<uint8_t>(salt >> 16), static_cast<uint8_t>(salt >> 8), static_cast<uint8_t>(salt) };
    return write_command(commands_t::COMPUTE_FINGERPRINT, buffer, sizeof(buffer));
}

BootloaderProtocol::status_t BootloaderProtocol::get_fingerprint(fingerprint_t &fingerprint, uint8_t offset, uint8_t size) const {
    if ((offset + size) > sizeof(fingerprint)) {
        return INVALID_ARGUMENTS;
    }

    uint8_t dataout[2] = { offset, size };
    size_t read_len = 0;
    auto status = run_transaction(commands_t::GET_FINGERPRINT, dataout, sizeof(dataout), &fingerprint.data()[offset], size, &read_len);
    if (status == status_t::COMMAND_OK && read_len != size) {
        status = status_t::BAD_RESPONSE;
    }
    return status;
}

BootloaderProtocol::status_t BootloaderProtocol::write_flash(uint32_t len,
    std::function<bool(uint32_t offset, size_t size, uint8_t *out_data)> get_data) const {
    assert(len <= MAX_FLASH_TOTAL_LENGTH);
    uint32_t offset = 0;
    uint8_t buffer[MAX_FLASH_BLOCK_LENGTH];

    while (offset < len) {
        size_t nextlen = std::min((size_t)MAX_FLASH_BLOCK_LENGTH, (size_t)(len - offset));

        bool getDataRes = get_data(offset, nextlen, &buffer[0]);
        if (!getDataRes) {
            return status_t::READ_DATA_ERROR;
        }

        auto res = write_flash_cmd(offset, buffer, nextlen);
        if (res != status_t::COMMAND_OK)
            return res;

        offset += nextlen;
    }

    uint8_t erase_count = 0;
    size_t response_len = 0;
    auto res = run_transaction(commands_t::FINALIZE_FLASH, nullptr, 0, &erase_count, sizeof(erase_count), &response_len, READ_TIMEOUT_FINALIZE_FLASH);
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
    uint8_t data[2] = { (uint8_t)new_address, 0 };
    auto res = write_command(commands_t::SET_ADDRESS, data, 2);
    return res;
}

}
