#include <cassert>

#include "puppies/PuppyModbus.hpp"
#include "puppies/PuppyBus.hpp"
#include "log.h"
#include "bsod.h"
#include "cmsis_os.h"
#include "timing.h"
#include "metric.h"

namespace buddy::puppies {

PuppyModbus puppyModbus;

LOG_COMPONENT_DEF(Modbus, LOG_SEVERITY_INFO);

std::array<uint8_t, PuppyModbus::MODBUS_RECEIVE_BUFFER_SIZE> modbus_buffer;

static metric_t modbus_reqfail = METRIC("modbus_reqfail", METRIC_VALUE_EVENT, 0, METRIC_HANDLER_ENABLE_ALL);

LIGHTMODBUS_WARN_UNUSED ModbusError modbusStaticAllocator(ModbusBuffer *buffer, uint16_t size, void *context) {
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

ModbusError PuppyModbus::static_exception_callback(const ModbusMaster *master, uint8_t address, uint8_t function, ModbusExceptionCode code) {
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
        if (!supress_error_logs)
            log_info(Modbus, "Error detected, recovering serial.");
        PuppyBus::ErrorRecovery();
    }
    return err;
}

ModbusErrorInfo PuppyModbus::make_request(RequestTiming *const timing) {
    uint32_t attempt = 1;
    static const uint32_t TOTAL_ATTEMPTS = 3;
    ModbusErrorInfo err;

    while (1) {
        err = make_single_request(timing);
        if (!modbusIsOk(err)) {
            // only repeat because of selected errors and maximal number of times
            bool repeat = (modbusGetResponseError(err) == MODBUS_ERROR_LENGTH || modbusGetResponseError(err) == MODBUS_ERROR_CRC)
                && attempt < TOTAL_ATTEMPTS;

            if (repeat) {
                if (!supress_error_logs)
                    log_error(Modbus, "Modbus request (attempt: %d/%d) failed, will repeat", attempt, TOTAL_ATTEMPTS);
                metric_record_event(&modbus_reqfail);
                attempt++;
                osDelay(10);
                continue;
            }
        }
        return err;
    }
}

void PuppyModbus::log_internal_error(ModbusErrorInfo error) {
    if (modbusIsOk(error) || supress_error_logs) {
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

ModbusErrorInfo PuppyModbus::read_input(uint8_t unit, bool *data, uint16_t count, uint16_t address) {
    auto lock = PuppyBus::LockGuard();

    log_debug(Modbus, "Communicate discrete input register unit: %d, data: %x, count: %d, address: %x", unit, data, count, address);

    ModbusErrorInfo err = modbusBuildRequest02RTU(&master, unit, address, count);
    log_internal_error(err);
    if (!modbusIsOk(err)) {
        return err;
    }

    active_value = { data, unit, address, count };

    return make_request(nullptr);
}

ModbusErrorInfo PuppyModbus::read_input(uint8_t unit, uint16_t *data, uint16_t count, uint16_t address, RequestTiming *const timing) {
    auto lock = PuppyBus::LockGuard();

    log_debug(Modbus, "Communicate input register unit: %d, data: %x, count: %d, address: %x", unit, data, count, address);

    ModbusErrorInfo err = modbusBuildRequest04RTU(&master, unit, address, count);
    log_internal_error(err);
    if (!modbusIsOk(err)) {
        return err;
    }

    active_value = { data, unit, address, count };

    err = make_request(timing);
    if (!modbusIsOk(err)) {
        // Clear data to propagate error
        std::fill(data, data + count, INVALID_REGISTER_VALUE);
    }
    return err;
}

ModbusErrorInfo PuppyModbus::read_holding(uint8_t unit, uint16_t *data, uint16_t count, uint16_t address) {
    auto lock = PuppyBus::LockGuard();

    log_debug(Modbus, "Read holding register unit: %d, data: %x, count: %d, address: %x", unit, data, count, address);

    ModbusErrorInfo err = modbusBuildRequest03RTU(&master, unit, address, count);
    log_internal_error(err);
    if (!modbusIsOk(err)) {
        return err;
    }

    active_value = { data, unit, address, count };

    err = make_request(nullptr);
    if (!modbusIsOk(err)) {
        // Clear data to propagate error
        std::fill(data, data + count, INVALID_REGISTER_VALUE);
    }
    return err;
}

ModbusErrorInfo PuppyModbus::write_holding(uint8_t unit, const uint16_t *data, uint16_t count, uint16_t address) {
    log_debug(Modbus, "Write holding register unit: %d, data: %x, count: %d, address: %x", unit, data, count, address);

    auto lock = PuppyBus::LockGuard();

    ModbusErrorInfo err = modbusBuildRequest16RTU(&master, unit, address, count, data);
    log_internal_error(err);
    if (!modbusIsOk(err)) {
        return err;
    }

    active_value = std::nullopt;

    return make_request(nullptr);
}

ModbusErrorInfo PuppyModbus::write_coil(uint8_t unit, bool value, bool pending, uint16_t address) {
    auto lock = PuppyBus::LockGuard();

    if (!pending) {
        return MODBUS_NO_ERROR();
    }

    log_debug(Modbus, "Communicate coil: %d, value: %x, pending: %x, address: %x", unit, value, pending, address);

    ModbusErrorInfo err = modbusBuildRequest05RTU(&master, unit, address, value);
    log_internal_error(err);
    if (!modbusIsOk(err)) {
        return err;
    }

    active_value = std::nullopt;

    err = make_request(nullptr);
    if (modbusIsOk(err)) {
        pending = false;
    }
    return err;
}

ModbusErrorInfo PuppyModbus::ReadFIFO(uint8_t unit, uint16_t address, std::array<uint16_t, 31> &buffer, size_t &read) {
    auto lock = PuppyBus::LockGuard();

    ModbusErrorInfo err = modbusBuildRequest24RTU(&master, unit, address);
    log_internal_error(err);
    if (!modbusIsOk(err)) {
        return err;
    }

    active_value = { static_cast<void *>(buffer.data()), unit, address, static_cast<uint16_t>(buffer.size()) };

    err = make_request(nullptr);
    read = active_value->data_processed;
    return err;
}

}
