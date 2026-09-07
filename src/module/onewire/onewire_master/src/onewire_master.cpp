/// @file
#include <onewire_master/onewire_master.hpp>

#include <algorithm>
#include <bsod/bsod.h>
#include <utils/byte_utils.hpp>

OneWireMaster::OneWireMaster(const Timing &timing)
    : timing_(timing) {}

void OneWireMaster::start_transfer(Bytes tx_buffer, WritableBytes rx_buffer) {
    debug_assert(!is_active());

    // Pre-clear the rx buffer
    std::ranges::fill(rx_buffer, std::byte { 0 });

    state_ = State {
        .tx_buffer = tx_buffer,
        .rx_buffer = rx_buffer,
        .transaction_type = TransactionType::tx_rx,
    };

    // !!! MUST be AFTER setting the state
    is_active_.store(true, std::memory_order_release);
}

void OneWireMaster::start_search(const std::optional<SearchData> &data) {
    debug_assert(!is_active());

    static constexpr std::array rom_search_cmd {
        std::byte { 0xF0 },
    };

    state_ = State {
        .tx_buffer = rom_search_cmd,
        .transaction_type = TransactionType::rom_search,
    };

    if (data.has_value()) {
        state_.search_device_address_accumulator = data->device_address;
        state_.search_prev_last_discrepancy = data->last_discrepancy;
        state_.search_following_discrepancy = true;
    }

    // !!! MUST be AFTER setting the state
    is_active_.store(true, std::memory_order_release);
}

std::optional<OneWireMaster::SearchData> OneWireMaster::search_result() const {
    debug_assert(!is_active() && state_.transaction_type == TransactionType::rom_search);
    if (!state_.search_found) {
        return std::nullopt;
    }

    return SearchData {
        .device_address = state_.search_device_address_accumulator,
        .last_discrepancy = state_.search_new_last_discrepancy,
    };
}

bool OneWireMaster::presence_detected() const {
    debug_assert(!is_active());
    return state_.presence_detected;
}

OneWireMaster::StepResult OneWireMaster::step(const StepArgs &args) {
    if (!is_active_.load(std::memory_order_acquire)) {
        return StepResult {
            .finished = true,
        };
    }

    // The while loop is for immediate state transitions that don't require wait
    // Those use `break` in the switch instead of `return`
    while (true) {
        switch (state_.step) {

            // =========================================
            // RESET
            // =========================================

        case Step::reset_start:
            state_.step = Step::reset_release;
            return StepResult {
                .next_step_delay_us = timing_.reset_drive,
                .drive_bus_low = true,
            };

        case Step::reset_release:
            state_.step = Step::reset_presence_sample;
            return StepResult {
                .next_step_delay_us = timing_.reset_settle_before_presence_sample,
            };

        case Step::reset_presence_sample:
            state_.presence_detected = args.bus_is_low;
            state_.step = Step::reset_finished;
            return StepResult {
                .next_step_delay_us = timing_.reset_settle_after_presence_sample,
            };

        case Step::reset_finished:
            if (!state_.presence_detected) {
                // No point in trasmitting when noone's listening
                // Give up and deliberate about life choices
                state_.step = Step::finished;
                break;
            }

            state_.step = Step::tx_start;
            break;

            // =========================================
            // WRITE
            // =========================================

        case Step::tx_start:
            state_.pos = 0;
            state_.step = Step::tx_bit;
            break;

        case Step::tx_bit: {
            const uint8_t pos_byte = state_.pos / 8;
            const uint8_t pos_bit = state_.pos % 8;

            if (pos_byte >= state_.tx_buffer.size()) {
                state_.step = Step::tx_finished;
                break;
            }

            state_.pos++;
            state_.step = bool((state_.tx_buffer[pos_byte] >> pos_bit) & std::byte { 1 }) ? Step::subop_write_bit_1_start : Step::subop_write_bit_0_start;
            state_.subop_return_step = Step::tx_bit;
            break;
        }

        case Step::tx_finished:
            switch (state_.transaction_type) {

            case TransactionType::tx_rx:
                state_.step = Step::rx_start;
                break;

            case TransactionType::rom_search:
                state_.step = Step::search_start;
                break;
            }
            break;

            // =========================================
            // READ
            // =========================================

        case Step::rx_start:
            state_.pos = 0;
            state_.step = Step::rx_bit;
            break;

        case Step::rx_bit: {
            const uint8_t pos_byte = state_.pos / 8;

            if (pos_byte >= state_.rx_buffer.size()) {
                state_.step = Step::rx_finished;
                break;
            }

            state_.step = Step::subop_read_bit_start;
            state_.subop_return_step = Step::rx_bit_finished;
            break;
        }

        case Step::rx_bit_finished: {
            const uint8_t pos_byte = state_.pos / 8;
            const uint8_t pos_bit = state_.pos % 8;

            state_.rx_buffer[pos_byte] |= std::byte { state_.subop_read_bit } << pos_bit;
            state_.pos++;
            state_.step = Step::rx_bit;
            break;
        }

        case Step::rx_finished:
            state_.step = Step::finished;
            break;

            // =========================================
            // ROM SEARCH
            // =========================================

        case Step::search_start:
            state_.pos = 0;
            state_.step = Step::search_round_start;
            break;

        case Step::search_round_start:
            if (state_.pos >= device_address_bit_count) {
                // If we were still following a previous discrepancy, that means that we haven't found a new device
                state_.search_found = !state_.search_following_discrepancy;
                state_.step = Step::finished;
                break;
            }

            state_.step = Step::search_read_first_bit;
            break;

        case Step::search_read_first_bit:
            state_.subop_return_step = Step::search_read_complement_bit;
            state_.step = Step::subop_read_bit_start;
            break;

        case Step::search_read_complement_bit:
            state_.search_first_bit = state_.subop_read_bit;
            state_.subop_return_step = Step::search_decide_direction;
            state_.step = Step::subop_read_bit_start;
            break;

        case Step::search_decide_direction: {
            // Noone pulled the bus down for the actual bit - there is no device with address bit 0
            const bool no_device_with_low_bit = state_.search_first_bit;

            // Noone pulled the bus down for the complement bit - there is no device with address bit 1
            const bool no_device_with_high_bit = state_.subop_read_bit;

            const bool accumulator_bit = bool((state_.search_device_address_accumulator >> state_.pos) & 1);

            bool direction_bit;

            if (no_device_with_high_bit && no_device_with_low_bit) {
                // Nothing matches what we wanted
                state_.step = Step::finished;
                break;

            } else if (!no_device_with_low_bit && !no_device_with_high_bit) {
                // Multiple devices - discrepancy

                if (!state_.search_following_discrepancy) {
                    // We're in uncharted teritorry, always go low
                    direction_bit = false;

                } else if (state_.pos == state_.search_prev_last_discrepancy) {
                    debug_assert(accumulator_bit == false);

                    // We went 0 in the previous search, now go 1
                    direction_bit = true;

                } else {
                    // Follow the previous search
                    direction_bit = accumulator_bit;
                }

                if (!direction_bit) {
                    state_.search_new_last_discrepancy = state_.pos;
                }

            } else {
                // Everything clear here, we have a single path we can take
                direction_bit = no_device_with_low_bit;
            }

            if (direction_bit != accumulator_bit) {
                // We're diverging from previous iteration, stop following the accumulator
                state_.search_following_discrepancy = false;
            }

            // Set the the bit in address acumulator to direction_bit
            // This xors the bit with the original accumulator_bit, setting it to zero,
            // and then with direction_bit, setting it to direction_bit
            state_.search_device_address_accumulator ^= DeviceAddress(direction_bit ^ accumulator_bit) << state_.pos;

            state_.pos++;

            // Write the direction bit and continue the search
            state_.subop_return_step = Step::search_round_start;
            state_.step = direction_bit ? Step::subop_write_bit_1_start : Step::subop_write_bit_0_start;
            break;
        }

        // =========================================
        // WRITE BIT SUBOP
        // =========================================
        case Step::subop_write_bit_1_start:
            state_.step = Step::subop_write_bit_1_release;

            return StepResult {
                .next_step_delay_us = timing_.write_bit_1_drive,
                .drive_bus_low = true,
            };

        case Step::subop_write_bit_1_release:
            state_.step = state_.subop_return_step;

            return StepResult {
                .next_step_delay_us = timing_.write_bit_1_settle
            };

        case Step::subop_write_bit_0_start:
            state_.step = Step::subop_write_bit_0_release;

            return StepResult {
                .next_step_delay_us = timing_.write_bit_0_drive,
                .drive_bus_low = true,
            };

        case Step::subop_write_bit_0_release:
            state_.step = state_.subop_return_step;

            return StepResult {
                .next_step_delay_us = timing_.write_bit_0_settle,
            };

            // =========================================
        // READ BIT SUBOP
        // =========================================
        case Step::subop_read_bit_start: {
            state_.step = Step::subop_read_bit_settle;
            return StepResult {
                .next_step_delay_us = timing_.read_bit_drive,
                .drive_bus_low = true,
            };
        }

        case Step::subop_read_bit_settle:
            state_.step = Step::subop_read_bit_sample;
            return StepResult {
                .next_step_delay_us = timing_.read_bit_settle_before_sample,
            };

        case Step::subop_read_bit_sample: {
            state_.subop_read_bit = !args.bus_is_low;
            state_.step = state_.subop_return_step;

            return StepResult {
                .next_step_delay_us = timing_.read_bit_settle_after_sample,
            };
        }

            // =========================================
            // FINISHED
            // =========================================

        case Step::finished:
            // !!! Must be AFTER the last manipulation of state_
            is_active_.store(false, std::memory_order_release);
            return {
                .finished = true
            };
        }
    }
}
