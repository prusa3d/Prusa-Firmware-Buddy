#pragma once

#include <st25r39xxb/ST25R39XXB.hpp>
#include <st25r39xxb/ST25R39XXB_defs.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <limits>
#include <span>
#include <utility>
#include <vector>

namespace fake {

using st25r39xxb::Command;
using st25r39xxb::IRQType;
using st25r39xxb::RegisterA;
using st25r39xxb::RegisterB;

constexpr std::byte oper_control_tx_enable { 0b0000'1000 };
constexpr std::byte oper_control_rx_enable { 0b0100'0000 };

class FakeSys;

/// Minimal register-file model of the chip, with a scripted response to the commands the driver
/// waits on, and a faithful model of the IRQ line.
///
/// The line is a level, not a pulse: it is asserted while an unmasked status bit is latched and
/// released only by a read of the status registers or by masking the source (DS13541 §4.3.1).
/// Every low-to-high transition calls trigger_interrupt on the attached FakeSys, the way a
/// rising-edge interrupt on the host would.
class FakeChip final : public st25r39xxb::HWInterface {
public:
    /// Reset values of the registers whose defaults matter to the driver (DS13541 Tables 83, 86).
    /// Zero would be a 75 mV threshold - a far more forgiving default than the real 205 mV one.
    static constexpr std::byte efd_activation_reset { 0x33 };
    static constexpr std::byte efd_deactivation_reset { 0x22 };

    FakeChip() {
        regs_a_.at(std::to_underlying(RegisterA::external_field_detector_activation_threshold)) = efd_activation_reset;
        regs_a_.at(std::to_underlying(RegisterA::external_field_detector_deactivation_threshold)) = efd_deactivation_reset;
    }

    /// IRQ bits the chip raises on each successive nfc_init_field_on.
    /// The last entry repeats forever once the script is exhausted.
    std::vector<IRQType> field_on_script { IRQType::pwp_field_active };

    /// IRQ bits the chip raises around a transmission. Entry 0 lands with the transmit command
    /// itself, and each later entry lands on the status read that follows the previous one - so a
    /// single entry models a tag whose whole answer is latched before the driver looks even once.
    std::vector<IRQType> transmit_script { IRQType::tx_end | IRQType::rx_start | IRQType::rx_end };

    /// What the FIFO holds after each successive transmission, in the wire form a tag answers in
    /// (see \ref fake::encode_tag_response). Entry 0 belongs to the first transmission, and the last
    /// entry repeats forever once the script is exhausted. The default is an empty FIFO.
    std::vector<std::vector<std::byte>> response_script { {} };

    std::vector<Command> commands;

    /// The bytes handed to \ref write_fifo, one entry per call - the requests as they went out
    std::vector<std::vector<std::byte>> transmitted;

    /// How many bytes each \ref read_fifo asked for - what the driver made of the status registers
    std::vector<size_t> fifo_reads;

    /// The bits of FIFO status 2 that are not part of the length: the FIFO error flags, the
    /// last-byte bit count and np_lb. Set by a test to model a reception the length has to be read
    /// out of anyway (DS13541 Table 67).
    std::byte fifo_status_2_flags {};

    unsigned field_on_count = 0;

    void attach(FakeSys &sys) {
        sys_ = &sys;
    }

    std::byte reg_a(RegisterA reg) const {
        return regs_a_.at(std::to_underlying(reg));
    }

    bool field_bits_set() const {
        return (reg_a(RegisterA::operation_control) & (oper_control_tx_enable | oper_control_rx_enable)) != std::byte { 0 };
    }

    size_t command_count(Command command) const {
        return std::ranges::count(commands, command);
    }

    /// Whether the chip is driving its IRQ pin high right now
    bool irq_line() const {
        return std::to_underlying(pending_irqs_ & ~mask_) != 0;
    }

    /// Raises \p irqs in the status registers, asserting the line if any of them is unmasked.
    void latch(IRQType irqs) {
        pending_irqs_ = pending_irqs_ | irqs;
        refresh_line();
    }

    void read_registers_continuous(RegisterA reg, const std::span<std::byte> &data) final {
        if (reg == RegisterA::main_interrupt && data.size() == sizeof(IRQType)) {
            // Reading the interrupt registers presents and clears them, masked bits included
            const auto raw = std::to_underlying(pending_irqs_);
            for (size_t i = 0; i < data.size(); i++) {
                data[i] = static_cast<std::byte>((raw >> (8 * i)) & 0xff);
            }
            pending_irqs_ = IRQType::none;
            refresh_line();
            advance_transmit_script();
            return;
        }
        read_from(regs_a_, std::to_underlying(reg), data);
    }

    void read_registers_continuous(RegisterB reg, const std::span<std::byte> &data) final {
        read_from(regs_b_, std::to_underlying(reg), data);
    }

    void write_registers_continuous(RegisterA reg, const std::span<const std::byte> &data) final {
        write_to(regs_a_, std::to_underlying(reg), data);
        if (reg == RegisterA::mask_main_interrupt && data.size() == sizeof(IRQType)) {
            uint32_t raw = 0;
            for (size_t i = 0; i < data.size(); i++) {
                raw |= static_cast<uint32_t>(std::to_integer<uint8_t>(data[i])) << (8 * i);
            }
            mask_ = IRQType { raw };
            // Unmasking a source whose bit is already set asserts the line immediately
            refresh_line();
        }
    }

    void write_registers_continuous(RegisterB reg, const std::span<const std::byte> &data) final {
        write_to(regs_b_, std::to_underlying(reg), data);
    }

    void read_fifo(const std::span<std::byte> &data) final {
        fifo_reads.push_back(data.size());
        std::ranges::fill(data, std::byte { 0 });
        std::copy_n(fifo_.begin(), std::min(data.size(), fifo_.size()), data.begin());
    }

    void write_fifo(const std::span<const std::byte> &data) final {
        transmitted.emplace_back(data.begin(), data.end());
    }

    void direct_command(Command command) final {
        commands.push_back(command);
        if (command == Command::nfc_init_field_on) {
            const auto index = std::min<size_t>(field_on_count, field_on_script.size() - 1);
            field_on_count++;
            latch(field_on_script.at(index));
            return;
        }
        if (command == Command::clear_fifo) {
            clear_fifo();
            return;
        }
        if (command == Command::stop_all_1 || command == Command::stop_all_2) {
            // Stop all activities performs a Clear FIFO of its own (DS13541 §4.4.2).
            //
            // It clears the chip's IRQ status bits too, which is deliberately not modelled: the
            // driver drops its own latch right afterwards anyway, and a chip that wiped the status
            // registers here would take the teeth out of the tests that put a stale bit in them.
            clear_fifo();
            return;
        }
        if (command == Command::transmit_without_crc || command == Command::transmit_with_crc) {
            set_fifo(scripted_response());
            transmit_count_++;
            next_transmit_entry_ = 0;
            advance_transmit_script();
        }
    }

private:
    /// Clear FIFO clears the status registers along with the FIFO (DS13541 §4.4.3)
    void clear_fifo() {
        fifo_.clear();
        regs_a_.at(std::to_underlying(RegisterA::fifo_status_1)) = std::byte { 0 };
        regs_a_.at(std::to_underlying(RegisterA::fifo_status_2)) = std::byte { 0 };
    }

    static void read_from(const std::array<std::byte, 256> &regs, uint8_t offset, const std::span<std::byte> &data) {
        for (size_t i = 0; i < data.size(); i++) {
            data[i] = regs.at(offset + i);
        }
    }
    static void write_to(std::array<std::byte, 256> &regs, uint8_t offset, const std::span<const std::byte> &data) {
        for (size_t i = 0; i < data.size(); i++) {
            regs.at(offset + i) = data[i];
        }
    }

    void advance_transmit_script() {
        if (next_transmit_entry_ >= transmit_script.size()) {
            return;
        }
        latch(transmit_script.at(next_transmit_entry_));
        next_transmit_entry_++;
    }

    /// The entry of \ref response_script belonging to the transmission being started
    std::vector<std::byte> scripted_response() const {
        if (response_script.empty()) {
            return {};
        }
        return response_script.at(std::min<size_t>(transmit_count_, response_script.size() - 1));
    }

    /// Puts \p data in the FIFO and reports its length the way the chip splits it: the low eight
    /// bits in status 1, and fifo_b<9:8> as the top two bits of status 2, whose remaining bits carry
    /// \ref fifo_status_2_flags (DS13541 Tables 66, 67).
    void set_fifo(std::vector<std::byte> data) {
        assert(data.size() <= st25r39xxb::constant::FIFO_SIZE);
        fifo_ = std::move(data);
        regs_a_.at(std::to_underlying(RegisterA::fifo_status_1)) = static_cast<std::byte>(fifo_.size() & 0xff);
        regs_a_.at(std::to_underlying(RegisterA::fifo_status_2)) = (static_cast<std::byte>(fifo_.size() >> 2) & std::byte { 0xc0 }) | fifo_status_2_flags;
    }

    /// Reports a low-to-high transition of the line to the host, and nothing otherwise
    void refresh_line();

    std::array<std::byte, 256> regs_a_ {};
    std::array<std::byte, 256> regs_b_ {};

    IRQType pending_irqs_ = IRQType::none;
    /// Chip reset leaves every source unmasked
    IRQType mask_ = IRQType::none;
    bool line_asserted_ = false;
    size_t next_transmit_entry_ = std::numeric_limits<size_t>::max();
    size_t transmit_count_ = 0;
    std::vector<std::byte> fifo_;
    FakeSys *sys_ = nullptr;
};

/// Virtual clock plus the one-deep wakeup flag the real SysImpl keeps in a binary semaphore.
/// Never sleeps: delay() and an unsatisfied await both just advance now_.
class FakeSys final : public st25r39xxb::SystemInterface {
public:
    explicit FakeSys(FakeChip &chip, uint32_t start_ms = 0)
        : chip_(chip)
        , now_(start_ms) {
        chip.attach(*this);
    }

    std::vector<uint32_t> delays;

    uint32_t now_ms() const {
        return now_;
    }

    /// Set to false to model a host that misses the assertion entirely - an edge-triggered input
    /// that was already high when it was armed, or an edge spent by an earlier await.
    bool deliver_wakeups = true;

    void delay(uint32_t delay_ms) final {
        delays.push_back(delay_ms);
        now_ += delay_ms;
    }

    uint32_t await_interrupt(uint32_t timeout_ms) final {
        if (wakeup_pending_) {
            wakeup_pending_ = false;
            return timeout_ms;
        }
        now_ += timeout_ms;
        return 0;
    }

    /// Saturates rather than counting, the way the one-deep flag in the real SysImpl does.
    /// The chip asserts its line more often than the driver waits, so a give onto an already
    /// full semaphore is normal and must not abort.
    void trigger_interrupt() final {
        if (deliver_wakeups) {
            wakeup_pending_ = true;
        }
    }

private:
    FakeChip &chip_;
    uint32_t now_;
    bool wakeup_pending_ = false;
};

inline void FakeChip::refresh_line() {
    const bool asserted = irq_line();
    if (asserted && !line_asserted_ && sys_ != nullptr) {
        sys_->trigger_interrupt();
    }
    line_asserted_ = asserted;
}

} // namespace fake
