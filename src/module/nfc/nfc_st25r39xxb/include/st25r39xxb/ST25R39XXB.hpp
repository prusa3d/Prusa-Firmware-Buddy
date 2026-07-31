#pragma once

#include <array>
#include <cstdint>
#include <inplace_function.hpp>
#include <inplace_vector.hpp>
#include <nfcv/commands.hpp>
#include <nfcv/rw_interface.hpp>

#include "ST25R39XXB_defs.hpp"
#include "hw_interface.hpp"
#include "system_interface.hpp"

namespace st25r39xxb {

class ST25R39XXB : public nfcv::ReaderWriterInterface {
public:
    /// Max retries attempted for each failed nfcv_command
    static constexpr uint8_t nfcv_command_max_attempts = 4;

    /// Delay after each failed retry
    static constexpr uint32_t retry_delay_ms = 10;

    /// Collision-avoidance threshold ladder as seen on RFI1, in mV peak-to-peak (DS13541 Table 88).
    /// Indexed by rfe_t<3:0>; entries 8..15 (rfe_3 = 1) are the sensitive half of the ladder.
    static constexpr std::array<uint16_t, 16> efd_threshold_ladder_mv {
        75, 105, 150, 205, 290, 400, 560, 800,
        25, 33, 47, 64, 90, 125, 175, 250
    };

    /// rfe_t<3:0> / rfe_td<3:0>: how strong a foreign field has to be before the chip refuses to
    /// bring our own field up. The difference between the two is the hysteresis.
    /// actual mV value is efd_threshold_ladder_mv[efd_collision_activation_code]
    static constexpr uint8_t efd_collision_activation_code = 0xA; ///< 47 mVpp on RFI1
    static constexpr uint8_t efd_collision_deactivation_code = 0x9; ///< 33 mVpp on RFI1

    // Both constraints come from the footnotes on DS13541 Tables 83 and 86.
    static_assert(efd_threshold_ladder_mv[efd_collision_activation_code] >= efd_threshold_ladder_mv[efd_collision_deactivation_code],
        "the activation threshold must be at or above the deactivation one - their difference is the hysteresis");
    static_assert((efd_collision_activation_code & 0x8) == (efd_collision_deactivation_code & 0x8),
        "rfe_3 must be equal in the activation and deactivation threshold registers");

    ST25R39XXB(HWInterface &hw_int, SystemInterface &sys_int)
        : hw_int(hw_int)
        , sys_int(sys_int) //
    {}

    /// Generic initialization function. Should be always called
    [[nodiscard]] nfcv::Result<void> init();

    /// Init NFC-V poller values. Should be called after @ref init
    [[nodiscard]] nfcv::Result<void> init_nfcv_poller(const ModulationConfiguration &settings);

    nfcv::Result<void> try_field_up(AntennaID antenna) final;
    void field_down() final;

    AntennaID antenna_count() const final {
        return 2;
    }

    [[nodiscard]] nfcv::Result<void> nfcv_command(const nfcv::Command &command) final;

private:
    HWInterface &hw_int;
    SystemInterface &sys_int;
    stdext::inplace_vector<std::byte, constant::FIFO_SIZE> buffer;

    /// Current antenna that has the field_up
    std::optional<AntennaID> active_antenna;

    [[nodiscard]] nfcv::Result<void> nfcv_command_inner(const nfcv::Command &command);

    /// Reads what the tag answered out of the FIFO and decodes it into \p command's output.
    ///
    /// Part of one attempt rather than of the command as a whole: an answer damaged in flight is
    /// worth asking for again, and a neighbouring reader's field is exactly what damages one.
    /// A no-op for the commands the ISO leaves unanswered.
    [[nodiscard]] nfcv::Result<void> read_nfcv_response(const nfcv::Command &command, unsigned cmd_id);

    [[nodiscard]] uint16_t get_fifo_len();
    /**
     * @brief Masks which interupt should ST25R39XXb send
     *
     * @attention The @p mask denotes which interrupts should be masked.
     * So if you want to enable an interrupt, then you will need to set the bit 0.
     * (So all 0xffffffff will disable all the interrupts).
     *
     * @param mask Mask of interrupts to "mask" (don't use)
     */
    void set_interrupt_mask(st25r39xxb::IRQType mask);
    [[nodiscard]] st25r39xxb::IRQType read_interrupt();

    /// Interrupts unmasked by the last \ref set_interrupt_mask, ie. the ones the driver currently
    /// cares about. Reading the status registers presents masked bits too (DS13541 §4.3.1); this is
    /// what \ref fetch_interrupts filters them out with.
    st25r39xxb::IRQType unmasked_irqs = st25r39xxb::IRQType::none;

    /// Interrupts already read out of the chip but not yet acted upon, and always a subset of
    /// \ref unmasked_irqs.
    ///
    /// The chip clears its status registers on read, so whoever reads consumes the bits for
    /// everyone. Without somewhere to park them, an interrupt that arrives early - an rx_end that
    /// lands while we are still waiting for rx_start - is destroyed by the read that was looking
    /// for something else, and the await that does want it waits out its full timeout.
    st25r39xxb::IRQType latched_irqs = st25r39xxb::IRQType::none;

    /// Reads the chip's status registers and merges the unmasked bits into \ref latched_irqs.
    [[nodiscard]] st25r39xxb::IRQType fetch_interrupts();

    /// Forgets \p irqs, in the latch. Bits still set in the chip are picked up by the next fetch.
    void consume_interrupts(st25r39xxb::IRQType irqs);

    /// Throws away everything pending, both in the chip and in the latch. For the points where
    /// whatever happened before is by definition stale.
    void discard_interrupts();
    /**
     *
     * @brief Block the code execution until we receive all the interrupts from @p irqs_to_wait_for or the @p timeout_ms expires or some inner timer expires.
     *
     * @param irqs_to_wait_for - mask to interrupts to wait for - every interrupt needs to be triggered
     * @param timeout_ms - timeout after we give up trying to read the interupts - unit is in ms
     * @param inner_timer_irq - if specified and such interrupt is received we will return @p inner_timer_error - only one can be specified
     * @param inner_timer_error - what the inner IRQ means to the caller. Defaults to Error::timeout, which is
     *        indistinguishable from a real timeout - pass something else when the caller needs to tell them apart.
     * @returns nfcv::Result<void> - return std::unexpected errors on timeout and if we receive any of the error iterrupts
     */
    [[nodiscard]] nfcv::Result<void> await_interrupt(st25r39xxb::IRQType irqs_to_wait_for, uint32_t timeout_ms, st25r39xxb::IRQType inner_timer_irq = st25r39xxb::IRQType::none, nfcv::Error inner_timer_error = nfcv::Error::timeout);

    [[nodiscard]] nfcv::Result<void> turn_on_oscilator();

    /// Lets the chip pick the regulated voltages itself, 250 mV below the current supply level.
    [[nodiscard]] nfcv::Result<void> adjust_regulators();

    void set_output_impedance(st25r39xxb::Impedance target_impedance);
    void set_output_amplitude(st25r39xxb::Amplitude target_amplitude);

    /// How long to wait for the collision-avoidance sequence to settle either way
    static constexpr uint32_t field_on_timeout_ms = 100;

    /// Settling time after the field bits go up, before the first command
    static constexpr uint32_t field_settle_ms = 50;

    /// Runs the chip's RF collision avoidance sequence once.
    /// \returns Error::field_collision if a foreign field is already radiating
    [[nodiscard]] nfcv::Result<void> field_on_with_collision_avoidance();

    void clear_field_bits();
};

} // namespace st25r39xxb
