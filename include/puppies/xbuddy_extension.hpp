#pragma once
#include "PuppyModbus.hpp"
#include "PuppyBus.hpp"

#include <freertos/mutex.hpp>
#include <atomic>

namespace buddy::puppies {

class XBuddyExtension final : public ModbusDevice {
public:
    XBuddyExtension(PuppyModbus &bus, const uint8_t modbus_address);

    // These are called from whatever task that needs them.
    void set_fan_pwm(size_t fan_idx, uint8_t pwm);
    void set_white_led(uint8_t intensity);
    void set_rgbw_led(std::array<uint8_t, 4> rgbw);
    void set_usb_power(bool enabled);
    void set_mmu_power(bool enabled);
    void set_mmu_nreset(bool enabled);
    std::optional<uint16_t> get_fan_rpm(size_t fan_idx);
    std::optional<float> get_chamber_temp();

    uint8_t get_requested_fan_pwm(size_t fan_idx);

    // These are called from the puppy task.
    CommunicationStatus refresh();
    CommunicationStatus initial_scan();
    CommunicationStatus ping();

    // These post requests into the puppy task - only one request is active at a time - MMU protocol_logic behaves that way.
    // I.e. it is not possible/supported to post multiple requests at once and wait for their result.
    void post_read_mmu_register(const uint8_t modbus_address);
    void post_write_mmu_register(const uint8_t modbus_address, const uint16_t value);

    // Virtual MMU registers modelled on the ext board and translated to/from special messages
    // 252: RW current command
    // 253: R command response code and value
    // 254: R either Current_Progress_Code (maps onto MMU register 5) (x) Current_Error_Code (maps onto MMU register 6)
    // Response to this query are 3 16bit numbers:
    // 'T''0', progress code a error code, from that we can compose the MMU's protocol ResponseMsg
    void post_query_mmu();
    void post_command_mmu(uint8_t command, uint8_t param);

    /// @returns true of some MMU response message arrived over MODBUS
    bool mmu_response_received(uint32_t rqSentTimestamp_ms) const;

    struct MMUModbusRequest {
        uint32_t timestamp_ms; ///< timestamp of received response from modbus comm
        union {
            struct ReadRegister {
                uint16_t value; ///< value read from the register
                uint8_t address; ///< register address to read from
                bool accepted;
            } read;
            struct WriteRegister {
                uint16_t value; ///< value to be written into the register
                uint8_t address; ///< register address to write to
                bool accepted;
            } write;
            struct Query {
                uint16_t pec; ///< progress and error code (mutually exclusive)
            } query;
            struct Command {
                union {
                    uint8_t command;
                    uint8_t param;
                } u;
                uint16_t cp; // command and param combined, because that's what's flying over the wire in a single register
            } command;
        } u;
        enum class RW : uint8_t {
            read = 0,
            write = 1,
            query = 2,
            command = 3,
            inactive = 0x80,
            read_inactive = inactive | read, ///< highest bit tags the request as accomplished - hiding this fact inside the enum makes a usage cleaner
            write_inactive = inactive | write,
            query_inactive = inactive | query,
            command_inactive = inactive | command,
        };
        RW rw = RW::inactive; ///< type of request/response currently active

        static MMUModbusRequest make_read_register(uint8_t address);
        static MMUModbusRequest make_write_register(uint8_t address, uint16_t value);
        static MMUModbusRequest make_query();
        static MMUModbusRequest make_command(uint8_t command, uint8_t param);
    };

    std::atomic<bool> mmuValidResponseReceived;

    /// @note once MMU_response_received returned true, no locking is needed to access the data,
    /// because protocol_logic doesn't issue any other request until this one has been processed
    const MMUModbusRequest &mmu_modbus_rq() const { return mmuModbusRq; }

    static constexpr uint8_t mmuButtonRegisterAddress = 252; // shall be extracted elsewhere because the ext board must use it as well
    static constexpr uint8_t mmuCommandInProgressRegisterAddress = 253; // shall be extracted elsewhere because the ext board must use it as well
    static constexpr uint8_t mmuCommandStatusRegisterAddress = 254; // shall be extracted elsewhere because the ext board must use it as well
    static constexpr uint8_t mmuCommandProgressOrErrorCodeRegisterAddress = 255; // shall be extracted elsewhere because the ext board must use it as well

    union MMUCommandInProgress {
        uint8_t bytes[2];
        uint16_t word;
    };

    MODBUS_REGISTER MMUQueryMultiRegister {
        MMUCommandInProgress cip;
        uint16_t commandStatus; // accepted, rejected, progress, error - simply ResponseMsgParamCodes
        uint16_t pec; // either progressCode (x)or errorCode
    };
    using MMUQueryRegisters = ModbusInputRegisterBlock<mmuCommandInProgressRegisterAddress, MMUQueryMultiRegister>;

    const MMUQueryRegisters &mmu_query_registers() const { return mmuQuery; }

#ifndef UNITTESTS
private:
#endif

    static constexpr size_t FAN_CNT = 3;

    // The registers cached here are accessed from different tasks.
    mutable freertos::Mutex mutex;

    // If reading/refresh failed, this'll be in invalid state and we'll return
    // nullopt for queries.
    bool valid = false;

    // TODO: More registers?

    MODBUS_REGISTER Requiremnt {
        // 0-255
        std::array<uint16_t, FAN_CNT> fan_pwm = { 0, 0, 0 };

        // 0-255
        uint16_t white_led = 0;
        // Split into components, each 0-255, for convenience.
        std::array<uint16_t, 4> rgbw_led = { 0, 0, 0, 0 };

        // technicaly a boolean - enables power for usb port
        uint16_t usb_power_enable = true;

        // technicaly a boolean - enables power for the MMU port
        uint16_t mmu_power_enable = true;
        // technicaly a boolean - sets the MMU port non-reset pin
        uint16_t mmu_nreset = true;
    };
    ModbusHoldingRegisterBlock<0x9000, Requiremnt> requirement;

    MODBUS_REGISTER Status {
        uint16_t fan_rpm[FAN_CNT] = { 0, 0, 0 };
        // In degrees * 10 (eg. 23.5°C = 235 in the register)
        uint16_t chamber_temp = 0;
        uint16_t mmu_power_enable = true;
        uint16_t mmu_nreset = true;
    };
    ModbusInputRegisterBlock<0x8000, Status> status;

    CommunicationStatus refresh_holding();
    CommunicationStatus refresh_input(uint32_t max_age);

    MMUQueryRegisters mmuQuery;

    MMUModbusRequest mmuModbusRq;

    static constexpr uint8_t mmuUnitNr = 220; // @@TODO hard coded on 2 spots for now!

    CommunicationStatus refresh_mmu();
};

extern XBuddyExtension xbuddy_extension;

} // namespace buddy::puppies
