/// @file
#include <puppies/PuppyModbus.hpp>

#include <bsod/bsod.h>
#include <common/metric.h>
#include <common/power_panic.hpp>
#include <common/timing.h>
#include <freertos/timing.hpp>
#include <logging/log.hpp>
#include <puppies/PuppyBus.hpp>

namespace buddy::puppies {

PuppyModbus puppyModbus;

constexpr auto default_modbus_severity = logging::Severity::info;
LOG_COMPONENT_DEF(Modbus, default_modbus_severity);

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

CommunicationStatus aggregate_communication_status(std::initializer_list<CommunicationStatus> statuses) {
    bool any_error = false;
    bool all_skipped = true;
    for (auto status : statuses) {
        any_error |= (status == CommunicationStatus::ERROR);
        all_skipped &= (status == CommunicationStatus::SKIPPED);
    }

    return any_error  ? CommunicationStatus::ERROR //
        : all_skipped ? CommunicationStatus::SKIPPED
                      : CommunicationStatus::OK;
}

PuppyModbus::ErrorLogSupressor::ErrorLogSupressor() {
    LOG_COMPONENT(Modbus).lowest_severity = logging::Severity::critical;
}

PuppyModbus::ErrorLogSupressor::~ErrorLogSupressor() {
    LOG_COMPONENT(Modbus).lowest_severity = default_modbus_severity;
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
        bsod("modbusMasterInit() failed: %u %u", err.source, err.error);
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
    PuppyModbus &puppy_modbus = *static_cast<PuppyModbus *>(master->context);
    (void)address;
    (void)function;
    puppy_modbus.last_exception_code = code;
    return MODBUS_ERROR_OTHER;
}

PuppyModbus::SingleRequestResult PuppyModbus::make_single_request(RequestTiming *const timing) {
    // Clear possible garbage pending in input buffer, cleanup possible errors
    PuppyBus::Flush();

    for (unsigned int i = 0; i < modbusMasterGetRequestLength(&master); ++i) {
        log_debug(Modbus, "Request data: %02x: %02x", i, modbusMasterGetRequest(&master)[i]);
    }

    PuppyBus::EnsurePause(PuppyBus::Pause::Short);

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

    last_exception_code = MODBUS_EXCEP_NONE;
    const ModbusErrorInfo err = modbusParseResponseRTU(
        &master,
        modbusMasterGetRequest(&master),
        modbusMasterGetRequestLength(&master),
        response_buffer.data(),
        read);

    // First, we look at the exception code, if set.
    switch (last_exception_code) {
    case MODBUS_EXCEP_ACK:
    case MODBUS_EXCEP_NACK:
    case MODBUS_EXCEP_ILLEGAL_FUNCTION:
    case MODBUS_EXCEP_ILLEGAL_ADDRESS:
    case MODBUS_EXCEP_ILLEGAL_VALUE:
        // request is not likely to succeed when retried
        return SingleRequestResult::fail;
    case MODBUS_EXCEP_SLAVE_FAILURE:
        // request is likely to succeed when retried
        // bus recovery is not needed
        return SingleRequestResult::retry;
    case MODBUS_EXCEP_NONE:
        // continue error handling
        break;
    }

    // Then, we look at the lightmodbus error code.
    switch (err.error) {
    case MODBUS_ERROR_OK:
        return SingleRequestResult::ok;
    case MODBUS_ERROR_LENGTH:
    case MODBUS_ERROR_CRC:
        // request is likely to succeed when retried
        // bus recovery is likely to fix the problem
        return SingleRequestResult::recover_and_retry;
    case MODBUS_ERROR_FUNCTION:
    case MODBUS_ERROR_ADDRESS:
    case MODBUS_ERROR_RANGE:
    case MODBUS_ERROR_VALUE:
    case MODBUS_ERROR_BAD_TRANSACTION:
        // CRC-valid frame that does not match the pending request: almost
        // certainly a stale response to a previous query.
        // Retry should recover
        return SingleRequestResult::retry;
    default:
        // continue error handling
        break;
    }

    // It is a failure.
    return SingleRequestResult::fail;
}

CommunicationStatus PuppyModbus::make_request(RequestTiming *const timing, size_t retries) {
    do {
        switch (make_single_request(timing)) {
        case SingleRequestResult::ok:
            return CommunicationStatus::OK;
        case SingleRequestResult::fail:
            return CommunicationStatus::ERROR;
        case SingleRequestResult::recover_and_retry:
            if (power_panic::panic_is_active()) {
                // Don't try to recover while in power panic. The bus likely died
                // because we cut power to puppies and we are in a hurry, so don't
                // wait for anything.
                return CommunicationStatus::ERROR;
            }
            PuppyBus::ErrorRecovery();
            [[fallthrough]];
        case SingleRequestResult::retry:
            metric_record_event(&modbus_reqfail);
            freertos::delay(10);
            continue;
        }
        bsod_unreachable();
    } while (retries-- > 0);
    return CommunicationStatus::ERROR;
}

ModbusDevice::ModbusDevice(uint8_t unit)
    : unit(unit) {}

CommunicationStatus PuppyModbus::read_input(uint8_t unit, bool *data, uint16_t count, uint16_t address, uint32_t &timestamp_ms, uint32_t max_age_ms) {
    if (max_age_ms && last_ticks_ms() - timestamp_ms < max_age_ms) {
        return CommunicationStatus::SKIPPED;
    }

    bool locked = false;
    auto lock = PuppyBus::LockGuard(locked);
    if (!locked) {
        return CommunicationStatus::SKIPPED; // Allow failure instead of bsod for toolchange and powerpanic cooperation
    }

    [[maybe_unused]] ModbusErrorInfo err = modbusBuildRequest02RTU(&master, unit, address, count);
    debug_assert(modbusIsOk(err));

    active_value = { data, unit, address, count };

    const auto status = make_request(nullptr);
    if (status == CommunicationStatus::OK) {
        timestamp_ms = last_ticks_ms();
    } else {
        log_error(Modbus, "Failed to read discrete input %u:0x%x@%u", unit, address, count);
    }
    return status;
}

CommunicationStatus PuppyModbus::read_input(uint8_t unit, uint16_t *data, uint16_t count, uint16_t address, RequestTiming *const timing, uint32_t &timestamp_ms, uint32_t max_age_ms) {
    if (max_age_ms && last_ticks_ms() - timestamp_ms < max_age_ms) {
        return CommunicationStatus::SKIPPED;
    }

    auto lock = PuppyBus::LockGuard();

    [[maybe_unused]] ModbusErrorInfo err = modbusBuildRequest04RTU(&master, unit, address, count);
    debug_assert(modbusIsOk(err));

    active_value = { data, unit, address, count };

    const auto status = make_request(timing);
    if (status == CommunicationStatus::OK) {
        timestamp_ms = last_ticks_ms();
    } else {
        log_error(Modbus, "Failed to read input register %u:0x%x@%u", unit, address, count);
        // Clear data to propagate error
        std::fill(data, data + count, INVALID_REGISTER_VALUE);
    }
    return status;
}

CommunicationStatus PuppyModbus::read_holding(uint8_t unit, uint16_t *data, uint16_t count, uint16_t address, uint32_t &timestamp_ms, uint32_t max_age_ms) {
    if (max_age_ms && last_ticks_ms() - timestamp_ms < max_age_ms) {
        return CommunicationStatus::SKIPPED;
    }

    auto lock = PuppyBus::LockGuard();

    [[maybe_unused]] ModbusErrorInfo err = modbusBuildRequest03RTU(&master, unit, address, count);
    debug_assert(modbusIsOk(err));

    active_value = { data, unit, address, count };

    const auto status = make_request(nullptr);
    if (status == CommunicationStatus::OK) {
        timestamp_ms = last_ticks_ms();
    } else {
        log_error(Modbus, "Failed to read holding register %u:0x%x@%u", unit, address, count);
        // Clear data to propagate error
        std::fill(data, data + count, INVALID_REGISTER_VALUE);
    }
    return status;
}

CommunicationStatus PuppyModbus::write_holding(uint8_t unit, const uint16_t *data, uint16_t count, uint16_t address, bool &dirty) {
    if (!dirty) {
        return CommunicationStatus::SKIPPED;
    }

    auto lock = PuppyBus::LockGuard();

    [[maybe_unused]] ModbusErrorInfo err = modbusBuildRequest16RTU(&master, unit, address, count, data);
    debug_assert(modbusIsOk(err));

    active_value = std::nullopt;

    const auto status = make_request(nullptr);
    if (status == CommunicationStatus::OK) {
        dirty = false;
    } else {
        log_error(Modbus, "Failed to write holding register %u:0x%x@%u", unit, address, count);
    }
    return status;
}

CommunicationStatus PuppyModbus::write_coil(uint8_t unit, bool value, uint16_t address, bool &dirty) {
    if (!dirty) {
        return CommunicationStatus::SKIPPED;
    }

    auto lock = PuppyBus::LockGuard();

    [[maybe_unused]] ModbusErrorInfo err = modbusBuildRequest05RTU(&master, unit, address, value);
    debug_assert(modbusIsOk(err));

    active_value = std::nullopt;

    const auto status = make_request(nullptr);
    if (status == CommunicationStatus::OK) {
        dirty = false;
    } else {
        log_error(Modbus, "Failed to write coil %u:0x%x", unit, address);
    }
    return status;
}

CommunicationStatus PuppyModbus::ReadFIFO(uint8_t unit, uint16_t address, std::array<uint16_t, 31> &buffer, size_t &read, size_t retries) {
    auto lock = PuppyBus::LockGuard();

    [[maybe_unused]] ModbusErrorInfo err = modbusBuildRequest24RTU(&master, unit, address);
    debug_assert(modbusIsOk(err));

    active_value = { static_cast<void *>(buffer.data()), unit, address, static_cast<uint16_t>(buffer.size()) };

    const auto status = make_request(nullptr, retries);
    if (status == CommunicationStatus::OK) {
        read = active_value->data_processed;
    } else {
        log_error(Modbus, "Failed to read fifo %u:0x%x", unit, address);
    }
    return status;
}

bool PuppyModbus::read_input_registers_impl(modbus::ServerAddress server_address, uint16_t address, std::span<uint16_t> registers) {
    uint32_t timestamp_ms;
    const CommunicationStatus status = read_input(
        std::to_underlying(server_address),
        registers.data(),
        registers.size(),
        address,
        nullptr,
        timestamp_ms,
        0);
    return status == CommunicationStatus::OK;
}

bool PuppyModbus::write_holding_registers_impl(modbus::ServerAddress server_address, uint16_t address, std::span<const uint16_t> registers) {
    bool dirty = true;
    const CommunicationStatus status = write_holding(
        std::to_underlying(server_address),
        registers.data(),
        registers.size(),
        address,
        dirty);
    return status == CommunicationStatus::OK;
}

} // namespace buddy::puppies
