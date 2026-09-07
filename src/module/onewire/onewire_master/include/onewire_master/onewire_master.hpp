/// @file
#pragma once

#include <cstdint>
#include <utils/byte_utils.hpp>
#include <atomic>
#include <optional>

/// Class implementing a master controller for the 1-wire bus
class OneWireMaster final {

public:
    using DeviceAddress = uint64_t;

    static constexpr uint8_t device_address_bit_count = sizeof(DeviceAddress) * 8;

    static constexpr uint8_t no_discrepancy = 255;

    /// Timing constants, in microseconds
    struct Timing {
        /// Time for which the initial reset is held
        uint16_t reset_drive;

        /// Delay between reset release and presence sampling
        uint16_t reset_settle_before_presence_sample;

        /// Delay after presence sampling
        uint16_t reset_settle_after_presence_sample;

        /// Time for which the bus is driven low during bit 1 write
        uint16_t write_bit_1_drive;

        /// Delay after the bus is driven by write_bit_1_drive
        uint16_t write_bit_1_settle;

        /// Time for which the bus is driven low during bit 0 write
        uint16_t write_bit_0_drive;

        /// Delay after the bus is driven by write_bit_0_drive
        uint16_t write_bit_0_settle;

        /// Time of the initial bus drive during bit read
        uint16_t read_bit_drive;

        /// Delay between driving the bus and sampling during bit read
        uint16_t read_bit_settle_before_sample;

        /// Delay after reading the sample during bit read
        uint16_t read_bit_settle_after_sample;
    };

public:
    /// !!! Timing is stored by reference. The caller must ensure it exist the whole time.
    OneWireMaster(const Timing &timing);

    /// @returns whether the one wire has an active transaction
    bool is_active() const {
        return is_active_.load(std::memory_order_acquire);
    }

    /// Starts executing a transfer transaction
    /// That is, first writes @param tx bytes on the bus, then reads @param rx buffers back
    /// !!! is_active must be false
    void start_transfer(Bytes tx_buffer, WritableBytes rx_buffer);

    struct SearchData {
        DeviceAddress device_address;
        uint8_t last_discrepancy;

        /// @returns whether the search returned the last available device
        /// If true, further start_search() calls should not return anything
        constexpr bool is_last() const {
            return last_discrepancy == no_discrepancy;
        }
    };

    /// Starts a device address scan, will look for the first device with address >= @p min_address
    /// Iterating over all devices on the bus can be achieved by repeatedly calling start_scan(previous_search_result)
    /// Note: First bit received over the bus is considered MSB
    void start_search(const std::optional<SearchData> &previous_search = std::nullopt);

    /// @returns result of the last finished search
    std::optional<SearchData> search_result() const;

    /// @returns whether the last transaction detected any devices on the bus
    bool presence_detected() const;

public:
    struct StepResult {
        /// When the next isr_step() should be called, in microseconds
        /// isr_step() should not be called if finished == true
        uint16_t next_step_delay_us = 0;

        /// Whether the operation finished & there is nothing else to do
        bool finished : 1 = false;

        /// Whether the master should drive the bus low
        bool drive_bus_low : 1 = false;

        bool operator==(const StepResult &) const = default;
    };
    struct StepArgs {
        /// Readout of the bus. True if the bus is low
        bool bus_is_low : 1 = false;
    };

    /// Master step routine.
    // Expected to be called from the ISR in the intervals indicated by result.next_step_delay_us
    StepResult step(const StepArgs &args);

private:
    enum class Step : uint8_t {
        reset_start,
        reset_release,
        reset_presence_sample,
        reset_finished,

        tx_start,
        tx_bit,
        tx_finished,

        rx_start,
        rx_bit,
        rx_bit_finished,
        rx_finished,

        search_start,
        search_round_start,
        search_read_first_bit,
        search_read_complement_bit,
        search_decide_direction,

        // Write bit subop. Writes 1 or 0 (based on the selected subop) and then returns to subop_return_step
        subop_write_bit_1_start,
        subop_write_bit_1_release,

        subop_write_bit_0_start,
        subop_write_bit_0_release,

        // Read bit subop. Reads a bit to the state_.subop_read_bit and then returns to subop_return_step
        subop_read_bit_start,
        subop_read_bit_settle,
        subop_read_bit_sample,

        finished,
    };

    enum class TransactionType : uint8_t {
        /// Standard TX-then-RX transaction
        tx_rx,

        /// Rom search transaction
        rom_search
    };

    struct State {
        Bytes tx_buffer = {};
        WritableBytes rx_buffer = {};

        /// Accumulator for the device search algorithm
        DeviceAddress search_device_address_accumulator = 0;

        /// Position in one of the buffers, depending on the current operation
        uint16_t pos = 0;

        /// Last discrepancy from the previous search
        /// The max @p pos where we went low when we could have gone high
        uint8_t search_prev_last_discrepancy = no_discrepancy;

        /// New last discrepancy, accumulated by the current search
        uint8_t search_new_last_discrepancy = no_discrepancy;

        Step step = Step::reset_start;

        Step subop_return_step = Step::finished;

        TransactionType transaction_type : 1;

        /// Whether presence was detected
        bool presence_detected : 1 = false;

        /// Value that has been read by the read bit subop
        bool subop_read_bit : 1 = false;

        /// Stored first bit of the search
        bool search_first_bit : 1 = false;

        /// Whether the search has found a result
        bool search_found : 1 = false;

        /// Whether search_prev_last_discrepancy is discrepancy from the last search or from the current
        bool search_following_discrepancy : 1 = false;
    };

    State state_;
    std::atomic<bool> is_active_ = false;

    const Timing &timing_;
};
