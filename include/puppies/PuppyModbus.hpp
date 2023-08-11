#pragma once

#include "modbus.h"

#include <stdint.h>
#include <array>
#include <cstring>
#include <type_traits>
#include <optional>

namespace buddy::puppies {

class ModbusDevice;

#define MODBUS_DISCRETE struct __attribute__((__packed__)) __attribute__((aligned(sizeof(bool))))
#define MODBUS_REGISTER struct __attribute__((__packed__)) __attribute__((aligned(sizeof(uint16_t))))

/**
 * Description of input block data
 */
template <uint16_t ADDRESS, typename DATA_T>
union ModbusDiscreteInputBlock {
    static_assert(sizeof(DATA_T) < 2000 * sizeof(bool), "Max register read size as defined by Modbus 584-844");
    static_assert(std::is_standard_layout<DATA_T>() && std::is_trivial<DATA_T>(), "This only works with plain data");
    static_assert(sizeof(DATA_T) % sizeof(bool) == 0, "Registers need to cover data");

    DATA_T value;                                  // Discrete register data block as application value
    bool registers[sizeof(DATA_T) / sizeof(bool)]; // Input register data block as Modbus registers
};

/**
 * Description of input register block data
 */
template <uint16_t ADDRESS, typename DATA_T>
union ModbusInputRegisterBlock {
    static_assert(sizeof(DATA_T) < 125 * sizeof(uint16_t), "Max register read size as defined by Modbus 584-844");
    static_assert(std::is_standard_layout<DATA_T>() && std::is_trivial<DATA_T>(), "This only works with plain data");
    static_assert(sizeof(DATA_T) % sizeof(uint16_t) == 0, "Registers need to cover data");

    DATA_T value;                                          // Input register data block as application value
    uint16_t registers[sizeof(DATA_T) / sizeof(uint16_t)]; // Input register data block as Modbus registers
};

/**
 * Description of holding register block data
 */
template <uint16_t ADDRESS, typename DATA_T>
union ModbusHoldingRegisterBlock {
    static_assert(sizeof(DATA_T) < 125 * sizeof(uint16_t), "Max register read size as defined by Modbus 584-844");
    static_assert(std::is_standard_layout<DATA_T>() && std::is_trivial<DATA_T>(), "This only works with plain data");
    static_assert(sizeof(DATA_T) % sizeof(uint16_t) == 0, "Registers need to cover data");

    DATA_T value;                                          // Holding register data block as application value
    uint16_t registers[sizeof(DATA_T) / sizeof(uint16_t)]; // Holding register data block as Modbus registers
};

template <uint16_t ADDRESS>
struct ModbusCoil {
    bool value;
    bool pending;
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

    PuppyModbus();

    /**
     * Synchronize input register with device
     */
    template <uint16_t ADDRESS, typename DATA_T>
    ModbusErrorInfo read(uint8_t unit, ModbusInputRegisterBlock<ADDRESS, DATA_T> &value, RequestTiming *const timing = nullptr) {
        return read_input(unit, value.registers, std::size(value.registers), ADDRESS, timing);
    }

    /**
     * Synchronize holding register with device
     *
     * This expects holding registers to be write only. Cached data are writtent to remote holding registers.
     */
    template <uint16_t ADDRESS, typename DATA_T>
    ModbusErrorInfo read(uint8_t unit, ModbusHoldingRegisterBlock<ADDRESS, DATA_T> &value) {
        return read_holding(unit, value.registers, std::size(value.registers), ADDRESS);
    }

    /**
     * Synchronize holding register with device
     *
     * This expects holding registers to be write only. Cached data are writtent to remote holding registers.
     */
    template <uint16_t ADDRESS, typename DATA_T>
    ModbusErrorInfo write(uint8_t unit, ModbusHoldingRegisterBlock<ADDRESS, DATA_T> &value) {
        return write_holding(unit, value.registers, std::size(value.registers), ADDRESS);
    }

    /**
     * Synchronize discrete input with device
     */
    template <uint16_t ADDRESS, typename DATA_T>
    ModbusErrorInfo read(uint8_t unit, ModbusDiscreteInputBlock<ADDRESS, DATA_T> &value) {
        return read_input(unit, value.registers, std::size(value.registers), ADDRESS);
    }

    /**
     * Synchronize coil with device
     */
    template <uint16_t ADDRESS>
    ModbusErrorInfo write(uint8_t unit, ModbusCoil<ADDRESS> &value) {
        return write_coil(unit, value.value, value.pending, ADDRESS);
    }

    /**
     * Read FIFO queue
     */
    ModbusErrorInfo ReadFIFO(uint8_t unit, uint16_t address, std::array<uint16_t, 31> &buffer, size_t &read);

    /// Last request begin timestamp
    uint32_t last_request_begin_ns;

    /// Last request end timestamp
    uint32_t last_request_end_ns;

    class ErrorLogSupressor {
    public:
        [[nodiscard]] ErrorLogSupressor() {
            puppyModbus.supress_error_logs = true;
        }
        ~ErrorLogSupressor() {
            puppyModbus.supress_error_logs = false;
        }
    };

private:
    static constexpr auto MODBUS_READ_TIMEOUT_MS = 30;
    static constexpr uint16_t INVALID_REGISTER_VALUE = 0;

    ModbusMaster master;
    std::optional<ModbusValueReference> active_value;
    std::array<uint8_t, MODBUS_RECEIVE_BUFFER_SIZE> response_buffer;
    bool supress_error_logs;

    ModbusErrorInfo make_request(RequestTiming *const timing);
    ModbusErrorInfo make_single_request(RequestTiming *const timing);
    static ModbusError static_data_callback(const ModbusMaster *master, const ModbusDataCallbackArgs *args);
    static ModbusError static_exception_callback(
        const ModbusMaster *master,
        uint8_t address,
        uint8_t function,
        ModbusExceptionCode code);
    ModbusError data_callback(const ModbusDataCallbackArgs *args);
    void log_internal_error(ModbusErrorInfo error);
    ModbusErrorInfo read_input(uint8_t unit, uint16_t *data, uint16_t count, uint16_t address, RequestTiming *const timing);
    ModbusErrorInfo read_holding(uint8_t unit, uint16_t *data, uint16_t count, uint16_t address);
    ModbusErrorInfo write_holding(uint8_t unit, const uint16_t *data, uint16_t count, uint16_t address);
    ModbusErrorInfo read_input(uint8_t unit, bool *data, uint16_t count, uint16_t address);
    ModbusErrorInfo write_coil(uint8_t unit, bool value, bool pending, uint16_t address);
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
