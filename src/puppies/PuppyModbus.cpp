#include <cassert>

#include "puppies/PuppyModbus.hpp"
#include "puppies/PuppyBus.hpp"
#include <logging/log.hpp>
#include "bsod.h"
#include "cmsis_os.h"
#include "timing.h"
#include "metric.h"

namespace buddy::puppies {

PuppyModbus puppyModbus;

LOG_COMPONENT_DEF(Modbus, logging::Severity::info);

std::array<uint8_t, PuppyModbus::MODBUS_RECEIVE_BUFFER_SIZE> modbus_buffer;

std::array<uint8_t, PuppyModbus::MODBUS_RECEIVE_BUFFER_SIZE> &PuppyModbus::share_buffer() {
    return modbus_buffer;
}

METRIC_DEF(modbus_reqfail, "modbus_reqfail", METRIC_VALUE_EVENT, 0, METRIC_ENABLED);

LIGHTMODBUS_WARN_UNUSED ModbusError modbusStaticAllocator([[maybe_unused]] ModbusBuffer *buffer, uint16_t size, [[maybe_unused]] void *context) {
    if (size > modbus_buffer.size()) {
        log_error(Modbus, "Allocation too big: %d > %d", size, modbus_buffer.size());
        return MODBUS_ERROR_ALLOC;
    }
    return MODBUS_OK;
}

PuppyModbus::PuppyModbus() {
    ModbusErrorInfo err = modbusMasterInit(
        &master,
        static_data_callback,
        static_exception_callback,
        modbusStaticAllocator,
        modbusMasterDefaultFunctions,
        modbusMasterDefaultFunctionCount);
    if (!modbusIsOk(err)) {
        log_internal_error(err);
        bsod("Failed to init modbus master");
    }

    master.context = this;
    master.request.data = modbus_buffer.data();
}

ModbusError PuppyModbus::static_data_callback(const ModbusMaster *master, const ModbusDataCallbackArgs *args) {
    return static_cast<PuppyModbus *>(master->context)->data_callback(args);
}

ModbusError PuppyModbus::data_callback(const ModbusDataCallbackArgs *args) {
    if (active_value) {
        if (active_value->unit != args->address) {
            return MODBUS_ERROR_RANGE;
        }

        if (args->function == 2) { // Read discrete input
            if (active_value->address <= args->index && args->index < active_value->address + active_value->data_count) {
                static_cast<bool *>(active_value->data)[args->index - active_value->address] = args->value;
                return MODBUS_OK;
            }
            return MODBUS_ERROR_RANGE;
        }
        if (args->function == 3 || args->function == 4) { // Read holding register or input register
            if (active_value->address <= args->index && args->index < active_value->address + active_value->data_count) {
                static_cast<uint16_t *>(active_value->data)[args->index - active_value->address] = args->value;
                return MODBUS_OK;
            }
            return MODBUS_ERROR_RANGE;
        }
        if (args->function == 24) { // Read FIFO
            if (active_value->address == args->index && args->offset < active_value->data_count) {
                static_cast<uint16_t *>(active_value->data)[args->offset] = args->value;
                if (args->offset + 1 > active_value->data_processed) {
                    active_value->data_processed = args->offset + 1;
                }
                return MODBUS_OK;
            }
            return MODBUS_ERROR_RANGE;
        }
    }

    log_warning(Modbus,
        "Received unhandled modbus data:\n"
        "\t from: %d\n"
        "\t  fun: %d\n"
        "\t type: %d\n"
        "\t   id: %d\n"
        "\tvalue: %d\n",
        args->address,
        args->function,
        args->type,
        args->index,
        args->value);

    return MODBUS_OK;
}

ModbusError PuppyModbus::static_exception_callback([[maybe_unused]] const ModbusMaster *master, uint8_t address, uint8_t function, ModbusExceptionCode code) {
#ifdef LIGHTMODBUS_DEBUG
    log_error(
        Modbus,
        "Exception unit: %03d, function: %03d, code: %03d (%s)\n",
        address,
        function,
        (int)code,
        modbusExceptionCodeStr(code));
#else
    log_error(
        Modbus,
        "Exception unit: %03d, function: %03d, code: %03d\n",
        address,
        function,
        (int)code);
#endif
    // return the error further
    return MODBUS_ERROR_OTHER;
}

ModbusErrorInfo PuppyModbus::make_single_request(RequestTiming *const timing) {
    // Clear possible garbage pending in input buffer, cleanup possible errors
    PuppyBus::Flush();

    for (unsigned int i = 0; i < modbusMasterGetRequestLength(&master); ++i) {
        log_debug(Modbus, "Request data: %02x: %02x", i, modbusMasterGetRequest(&master)[i]);
    }

    PuppyBus::EnsurePause();

    if (timing) {
        timing->begin_us = ticks_us();
    }
    PuppyBus::Write(modbusMasterGetRequest(&master), modbusMasterGetRequestLength(&master));
    size_t read = PuppyBus::Read(response_buffer.data(), response_buffer.size(), MODBUS_READ_TIMEOUT_MS);
    if (timing) {
        timing->end_us = ticks_us();
    }

    log_debug(Modbus, "Response length: %d", read);
    for (unsigned int i = 0; i < read; ++i) {
        log_debug(Modbus, "Response data: %02x: %02x", i, response_buffer[i]);
    }

    ModbusErrorInfo err = modbusParseResponseRTU(
        &master,
        modbusMasterGetRequest(&master),
        modbusMasterGetRequestLength(&master),
        response_buffer.data(),
        read);
    log_internal_error(err);
    if (!modbusIsOk(err)) {
        if (!suppress_error_logs) {
            log_info(Modbus, "Error detected, recovering serial.");
        }
        PuppyBus::ErrorRecovery();
    }
    return err;
}

ModbusErrorInfo PuppyModbus::make_request(RequestTiming *const timing, uint8_t retries) {
    ModbusErrorInfo err;

    while (1) {
        err = make_single_request(timing);
        if (!modbusIsOk(err)) {
            // only repeat because of selected errors and maximal number of times
            bool repeat = (modbusGetResponseError(err) == MODBUS_ERROR_LENGTH || modbusGetResponseError(err) == MODBUS_ERROR_CRC)
                && retries;

            if (repeat) {
                if (!suppress_error_logs) {
                    log_error(Modbus, "Request failed, will repeat: %d times", retries);
                }
                metric_record_event(&modbus_reqfail);
                retries--;
                osDelay(10);
                continue;
            }
        }
        return err;
    }
}

void PuppyModbus::log_internal_error(ModbusErrorInfo error) {
    if (modbusIsOk(error) || suppress_error_logs) {
        return;
    }
#ifdef LIGHTMODBUS_DEBUG
    log_error(Modbus, "Modbus error: %s: %s",
        modbusErrorSourceStr(modbusGetErrorSource(error)),
        modbusErrorStr(modbusGetErrorCode(error)));
#else
    log_error(Modbus, "Modbus error: %d: %d",
        modbusGetErrorSource(error),
        modbusGetErrorCode(error));
#endif
}

ModbusDevice::ModbusDevice(PuppyModbus &bus, uint8_t unit)
    : bus(bus)
    , unit(unit) {}

CommunicationStatus PuppyModbus::read_input(uint8_t unit, bool *data, uint16_t count, uint16_t address, uint32_t &timestamp_ms, uint32_t max_age_ms) {
    if (max_age_ms && last_ticks_ms() - timestamp_ms < max_age_ms) {
        return CommunicationStatus::SKIPPED;
    }

    bool locked = false;
    auto lock = PuppyBus::LockGuard(locked);
    if (!locked) {
        return CommunicationStatus::SKIPPED; // Allow failure instead of bsod for toolchange and powerpanic cooperation
    }

    log_debug(Modbus, "Communicate discrete input register unit: %d, data: %p, count: %d, address: %x", unit, data, count, address);

    [[maybe_unused]] ModbusErrorInfo err = modbusBuildRequest02RTU(&master, unit, address, count);
    assert(modbusIsOk(err));

    active_value = { data, unit, address, count };

    if (!modbusIsOk(make_request(nullptr))) {
        log_error(Modbus, "Failed to read discrete input %u:0x%x@%u", unit, address, count);
        return CommunicationStatus::ERROR;
    }

    timestamp_ms = last_ticks_ms();
    return CommunicationStatus::OK;
}

CommunicationStatus PuppyModbus::read_input(uint8_t unit, uint16_t *data, uint16_t count, uint16_t address, RequestTiming *const timing, uint32_t &timestamp_ms, uint32_t max_age_ms) {
    if (max_age_ms && last_ticks_ms() - timestamp_ms < max_age_ms) {
        return CommunicationStatus::SKIPPED;
    }

    auto lock = PuppyBus::LockGuard();

    log_debug(Modbus, "Communicate input register unit: %d, data: %p, count: %d, address: %x", unit, data, count, address);

    [[maybe_unused]] ModbusErrorInfo err = modbusBuildRequest04RTU(&master, unit, address, count);
    assert(modbusIsOk(err));

    active_value = { data, unit, address, count };

    if (!modbusIsOk(make_request(timing))) {
        log_error(Modbus, "Failed to read input %u:0x%x@%u", unit, address, count);
        // Clear data to propagate error
        std::fill(data, data + count, INVALID_REGISTER_VALUE);
        return CommunicationStatus::ERROR;
    }

    timestamp_ms = last_ticks_ms();
    return CommunicationStatus::OK;
}

CommunicationStatus PuppyModbus::read_holding(uint8_t unit, uint16_t *data, uint16_t count, uint16_t address, uint32_t &timestamp_ms, uint32_t max_age_ms) {
    if (max_age_ms && last_ticks_ms() - timestamp_ms < max_age_ms) {
        return CommunicationStatus::SKIPPED;
    }

    auto lock = PuppyBus::LockGuard();

    log_debug(Modbus, "Read holding register unit: %d, data: %p, count: %d, address: %x", unit, data, count, address);

    [[maybe_unused]] ModbusErrorInfo err = modbusBuildRequest03RTU(&master, unit, address, count);
    assert(modbusIsOk(err));

    active_value = { data, unit, address, count };

    if (!modbusIsOk(make_request(nullptr))) {
        log_error(Modbus, "Failed to read holding %u:0x%x@%u", unit, address, count);
        // Clear data to propagate error
        std::fill(data, data + count, INVALID_REGISTER_VALUE);
        return CommunicationStatus::ERROR;
    }

    timestamp_ms = last_ticks_ms();
    return CommunicationStatus::OK;
}

CommunicationStatus PuppyModbus::write_holding(uint8_t unit, const uint16_t *data, uint16_t count, uint16_t address, bool &dirty) {
    if (!dirty) {
        return CommunicationStatus::SKIPPED;
    }

    log_debug(Modbus, "Write holding register unit: %d, data: %p, count: %d, address: %x", unit, data, count, address);

    auto lock = PuppyBus::LockGuard();

    [[maybe_unused]] ModbusErrorInfo err = modbusBuildRequest16RTU(&master, unit, address, count, data);
    assert(modbusIsOk(err));

    active_value = std::nullopt;

    if (!modbusIsOk(make_request(nullptr))) {
        log_error(Modbus, "Failed to write holding %u:0x%x@%u", unit, address, count);
        return CommunicationStatus::ERROR;
    }

    dirty = false;
    return CommunicationStatus::OK;
}

CommunicationStatus PuppyModbus::write_coil(uint8_t unit, bool value, uint16_t address, bool &dirty) {
    if (!dirty) {
        return CommunicationStatus::SKIPPED;
    }

    auto lock = PuppyBus::LockGuard();

    log_debug(Modbus, "Communicate coil: %d, value: %x, address: %x", unit, value, address);

    [[maybe_unused]] ModbusErrorInfo err = modbusBuildRequest05RTU(&master, unit, address, value);
    assert(modbusIsOk(err));

    active_value = std::nullopt;

    if (!modbusIsOk(make_request(nullptr))) {
        log_error(Modbus, "Failed to write coil %u:0x%x", unit, address);
        return CommunicationStatus::ERROR;
    }

    dirty = false;
    return CommunicationStatus::OK;
}

CommunicationStatus PuppyModbus::ReadFIFO(uint8_t unit, uint16_t address, std::array<uint16_t, 31> &buffer, size_t &read) {
    auto lock = PuppyBus::LockGuard();

    [[maybe_unused]] ModbusErrorInfo err = modbusBuildRequest24RTU(&master, unit, address);
    assert(modbusIsOk(err));

    active_value = { static_cast<void *>(buffer.data()), unit, address, static_cast<uint16_t>(buffer.size()) };

    if (!modbusIsOk(make_request(nullptr, 0))) {
        log_error(Modbus, "Failed to read fifo %u:0x%x", unit, address);
        return CommunicationStatus::ERROR;
    }

    read = active_value->data_processed;
    return CommunicationStatus::OK;
}

} // namespace buddy::puppies
