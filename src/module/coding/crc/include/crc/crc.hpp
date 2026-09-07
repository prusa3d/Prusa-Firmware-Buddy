///
/// This header file provides functions for encoding and decoding data using the
/// Consistent Overhead Byte Stuffing (COBS) algorithm. COBS ensures that the
/// data bytes do not contain the packet delimiter (0x00), making packet
/// framing unambiguous.
///
/// The implementation and configuration is based on the CRCs described in https://reveng.sourceforge.io/crc-catalogue/16.htm
#pragma once

#include <stdint.h>
#include <utils/byte_utils.hpp>
#include <cstddef>
#ifdef __AVR__
    // #error dead code found by automatic analyses (see BFW-5461)
    // AVR has optimized assembly versions of some crc functions
    #include <util/crc16.h>
#else
// These are the equivalent C implementation as documented in the AVR
// util/crc16.h header.

/**
 * @brief Compute 16-bit CCITT-FALSE CRC (polynomial 0x1021)
 * @param crc Current CRC value (initialize with 0xFFFF)
 * @param data Input byte to process
 * @return Updated CRC value
 * @note Characteristics:
 * - Polynomial: 0x1021
 * - Initial value: 0xFFFF
 * - Final XOR: 0x0000
 * - Non-reflected
 */
inline uint16_t _crc16_ccitt_false_update(uint16_t crc, uint8_t data) {
    crc ^= (uint16_t)data << 8;
    for (int i = 0; i < 8; ++i) {
        if (crc & 0x8000) {
            crc = (crc << 1) ^ 0x1021;
        } else {
            crc <<= 1;
        }
    }
    return crc;
}

/**
 * @brief CRC-16/MODBUS (polynomial 0x8005)
 * @param crc Current CRC value (initialize with 0x0000)
 * @param data Input byte to process
 * @return Updated CRC value
 * @note Characteristics:
 * - Polynomial: 0x8005 (reversed -> 0xA001)
 * - Initial value: 0xFFFF
 * - Final XOR: 0x0000
 * - Reflected (LSB first)
 */
inline uint16_t _crc16_modbus_update(uint16_t crc, uint8_t a) {
    crc ^= a;
    for (int i = 0; i < 8; ++i) {
        if (crc & 1) {
            crc = (crc >> 1) ^ 0xA001;
        } else {
            crc = (crc >> 1);
        }
    }

    return crc;
}
#endif

/// CRC-8/MAXIM (1-Wire)
inline uint8_t _crc8_maxim_update(uint8_t crc, uint8_t a) {
    for (int i = 0; i < 8; i++) {
        uint8_t mix = (crc ^ a) & 0x01;
        crc >>= 1;
        if (mix) {
            crc ^= 0x8C;
        }
        a >>= 1;
    }
    return crc;
}

/**
 * Helper class to calculate crcs for transfers. To use it, create an
 * instance, call update() for each byte and/or buffer of bytes to
 * calculate the CRC over, and then call get() to get the result.
 * update() is chainable, so you can do e.g.:
 *
 *   uint8_t crc = Crc<...>().update(first_byte).update(rest_of_bytes, len).get();
 */
template <typename T, T Update(T, uint8_t), T Initial>
class Crc {
public:
    Crc &update(uint8_t b) {
        this->crc = Update(this->crc, b);
        return *this;
    }

    Crc &update(Bytes bytes) {
        const auto *data = reinterpret_cast<const uint8_t *>(bytes.data());
        for (size_t i = 0; i < bytes.size(); ++i) {
            this->update(data[i]);
        }
        return *this;
    }

    [[deprecated("Use overload taking span of bytes")]]
    Crc &update(const uint8_t *buf, size_t len) {
        return update(Bytes(
            reinterpret_cast<const std::byte *>(buf), len));
    }

    T get() {
        return this->crc;
    }

    Crc &reset() {
        this->crc = Initial;
        return *this;
    }

private:
    T crc = Initial;
};

using Crc16CcittFalse = Crc<uint16_t, _crc16_ccitt_false_update, 0xffff>;

// Called CRC16-IBM (or CRC16-ANSI or just CRC16) by wikipedia, used by ModBus
using Crc16Modbus = Crc<uint16_t, _crc16_modbus_update, 0xffff>;

/// Called CRC8-MAXIM
using Crc8OneWire = Crc<uint8_t, _crc8_maxim_update, 0>;
