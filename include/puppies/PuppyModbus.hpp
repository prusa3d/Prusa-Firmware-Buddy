#pragma once

#include "modbus.h"

#include <stdint.h>
#include <array>
#include <cstring>
#include <type_traits>
#include <optional>
#include <limits>

namespace buddy::puppies {

class ModbusDevice;

#define MODBUS_DISCRETE struct __attribute__((__packed__)) __attribute__((aligned(sizeof(bool))))
#define MODBUS_REGISTER struct __attribute__((__packed__)) __attribute__((aligned(sizeof(uint16_t))))

struct ModbusInputBase {
    uint32_t last_read_timestamp_ms = std::numeric_limits<uint32_t>::max();
};

struct ModbusOutputBase {
    bool dirty = false; // Whether the value needs write
};

/**
 * Description of input block data
 */
template <uint16_t ADDRESS, typename DATA_T>
struct ModbusDiscreteInputBlock : ModbusInputBase {
    static_assert(sizeof(DATA_T) < 2000 * sizeof(bool), "Max register read size as defined by Modbus 584-844");
    static_assert(std::is_standard_layout<DATA_T>(), "This only works with plain data");
    static_assert(sizeof(DATA_T) % sizeof(bool) == 0, "Registers need to cover data");

    union {
        DATA_T value {}; // Discrete register data block as application value
        bool registers[sizeof(DATA_T) / sizeof(bool)]; // Input register data block as Modbus registers
    };
};

/**
 * Description of register block data
 */
template <uint16_t ADDRESS, typename DATA_T>
struct ModbusRegisterBlock {
    static_assert(sizeof(DATA_T) < 125 * sizeof(uint16_t), "Max register read size as defined by Modbus 584-844");
    static_assert(std::is_standard_layout<DATA_T>(), "This only works with plain data");
    static_assert(sizeof(DATA_T) % sizeof(uint16_t) == 0, "Registers need to cover data");

    union {
        DATA_T value {}; // Register data block as application value
        uint16_t registers[sizeof(DATA_T) / sizeof(uint16_t)]; // Register data block as Modbus registers
    };
};

/**
 * Description of input register block data
 */
template <uint16_t ADDRESS, typename DATA_T>
struct ModbusInputRegisterBlock : ModbusRegisterBlock<ADDRESS, DATA_T>, ModbusInputBase {};

/**
 * Description of holding register block data
 */
template <uint16_t ADDRESS, typename DATA_T>
struct ModbusHoldingRegisterBlock : ModbusRegisterBlock<ADDRESS, DATA_T>, ModbusInputBase, ModbusOutputBase {};

/**
 * Description of coil data
 */
template <uint16_t ADDRESS>
struct ModbusCoil : ModbusOutputBase {
    bool value {};
};

struct ModbusValueReference {
    void *data;
    uint8_t unit;
    uint16_t address;
    uint16_t data_count;
    uint16_t data_processed { 0 };
};

struct RequestTiming {
    uint32_t begin_us;
    uint32_t end_us;
};

enum class CommunicationStatus {
    OK,
    ERROR,
    SKIPPED,
};

class PuppyModbus;
extern PuppyModbus puppyModbus;

class PuppyModbus {
    /**
     * This wraps liblightmodbus master and provides Modbus requests on top of PuppyBus interface.
     *
     * The idea is to have a copy of register data provided by
     *   ModbusDiscreteInputRegisterBlock, ModbusInputRegisterBlock, ModbusHoldingRegisterBlock and ModbusCoil
     * and use Communicate method to sync cached data with device registers.
     *
     * There is no locking mechanism to protect the communication chanell in place. All calls are
     * supposed to be performed from a single thread that pools all the subordinate devices.
     */
public:
    static constexpr auto MODBUS_RECEIVE_BUFFER_SIZE = 256; // 256 is maximum Modbus response size

    /**
     * @brief Share modbus buffer to other protocol on the same line.
     * Must be exclusive with ModBus.
     * @return buffer to be shared with other protocol
     */
    static std::array<uint8_t, MODBUS_RECEIVE_BUFFER_SIZE> &share_buffer();

    PuppyModbus();

    /**
     * Synchronize input register with device
     */
    template <uint16_t ADDRESS, typename DATA_T>
    CommunicationStatus read(uint8_t unit, ModbusInputRegisterBlock<ADDRESS, DATA_T> &value, uint32_t max_age_ms = 0, RequestTiming *const timing = nullptr) {
        return read_input(unit, value.registers, std::size(value.registers), ADDRESS, timing, value.last_read_timestamp_ms, max_age_ms);
    }

    /**
     * Synchronize holding register with device
     *
     * This expects holding registers to be write only. Cached data are writtent to remote holding registers.
     */
    template <uint16_t ADDRESS, typename DATA_T>
    CommunicationStatus read(uint8_t unit, ModbusHoldingRegisterBlock<ADDRESS, DATA_T> &value, uint32_t max_age_ms = 0) {
        return read_holding(unit, value.registers, std::size(value.registers), ADDRESS, value.last_read_timestamp_ms, max_age_ms);
    }

    /**
     * Synchronize holding register with device
     *
     * This expects holding registers to be write only. Cached data are writtent to remote holding registers.
     */
    template <uint16_t ADDRESS, typename DATA_T>
    CommunicationStatus write(uint8_t unit, ModbusHoldingRegisterBlock<ADDRESS, DATA_T> &value) {
        return write_holding(unit, value.registers, std::size(value.registers), ADDRESS, value.dirty);
    }

    /**
     * Synchronize discrete input with device
     */
    template <uint16_t ADDRESS, typename DATA_T>
    CommunicationStatus read(uint8_t unit, ModbusDiscreteInputBlock<ADDRESS, DATA_T> &value, uint32_t max_age_ms = 0) {
        return read_input(unit, value.registers, std::size(value.registers), ADDRESS, value.last_read_timestamp_ms, max_age_ms);
    }

    /**
     * Synchronize coil with device
     */
    template <uint16_t ADDRESS>
    CommunicationStatus write(uint8_t unit, ModbusCoil<ADDRESS> &value) {
        return write_coil(unit, value.value, ADDRESS, value.dirty);
    }

    /**
     * Read FIFO queue
     */
    CommunicationStatus ReadFIFO(uint8_t unit, uint16_t address, std::array<uint16_t, 31> &buffer, size_t &read);

    /// Last request begin timestamp
    uint32_t last_request_begin_ns;

    /// Last request end timestamp
    uint32_t last_request_end_ns;

    class ErrorLogSupressor {
    public:
        [[nodiscard]] ErrorLogSupressor() {
            puppyModbus.suppress_error_logs = true;
        }
        ~ErrorLogSupressor() {
            puppyModbus.suppress_error_logs = false;
        }
    };

    static constexpr auto MODBUS_READ_TIMEOUT_MS = 30;

private:
    static constexpr uint16_t INVALID_REGISTER_VALUE = 0;

    ModbusMaster master;
    std::optional<ModbusValueReference> active_value;
    std::array<uint8_t, MODBUS_RECEIVE_BUFFER_SIZE> response_buffer;
    bool suppress_error_logs;

    ModbusErrorInfo make_request(RequestTiming *const timing, uint8_t retries = 3);
    ModbusErrorInfo make_single_request(RequestTiming *const timing);
    static ModbusError static_data_callback(const ModbusMaster *master, const ModbusDataCallbackArgs *args);
    static ModbusError static_exception_callback(
        const ModbusMaster *master,
        uint8_t address,
        uint8_t function,
        ModbusExceptionCode code);
    ModbusError data_callback(const ModbusDataCallbackArgs *args);
    void log_internal_error(ModbusErrorInfo error);
    CommunicationStatus read_input(uint8_t unit, uint16_t *data, uint16_t count, uint16_t address, RequestTiming *const timing, uint32_t &timestamp_ms, uint32_t max_age_ms);

public:
    CommunicationStatus read_holding(uint8_t unit, uint16_t *data, uint16_t count, uint16_t address, uint32_t &timestamp_ms, uint32_t max_age_ms);
    CommunicationStatus write_holding(uint8_t unit, const uint16_t *data, uint16_t count, uint16_t address, bool &dirty);

private:
    CommunicationStatus read_input(uint8_t unit, bool *data, uint16_t count, uint16_t address, uint32_t &timestamp_ms, uint32_t max_age_ms);
    CommunicationStatus write_coil(uint8_t unit, bool value, uint16_t address, bool &dirty);
};

class ModbusDevice {
    /**
     * Base class for abstraction of a Modbus device
     *
     * This is supposed to serve as a base for implementing custom modbus devices by inheriting from this class and
     * including ModbusValue instances.
     */

public:
    ModbusDevice(PuppyModbus &bus, uint8_t unit);

    inline bool is_enabled() const {
        return enabled;
    }

    inline void set_enabled(bool enabled) {
        this->enabled = enabled;
    }

protected:
    PuppyModbus &bus;
    uint8_t unit;
    bool enabled; //< When true, this puppy is connected and enabled
};

} // namespace buddy::puppies
