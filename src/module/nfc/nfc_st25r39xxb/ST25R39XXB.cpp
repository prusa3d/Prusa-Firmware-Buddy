#include <st25r39xxb/ST25R39XXB.hpp>
#include <st25r39xxb/ST25R39XXB_defs.hpp>

#include <raii/scope_guard.hpp>
#include <iso13239/crc.hpp>
#include <inplace_vector.hpp>
#include <nfcv/encode.hpp>
#include <nfcv/decode.hpp>
#include <utils/overloaded_visitor.hpp>
#include <utils/byte_utils.hpp>
#include <logging/log.hpp>

LOG_COMPONENT_DEF(St25r, logging::Severity::info);

namespace {

/// Whether a failed command is the expected "nothing answered" outcome rather than a fault.
///
/// Discovery terminates its inventory loop by getting no answer, so an unanswered Inventory is
/// the steady state of an idle antenna and must not produce any log traffic. Every other command
/// is addressed to a tag we already know is there, so silence means something went wrong.
bool is_expected_silence(const nfcv::Command &command, nfcv::Error error) {
    return error == nfcv::Error::no_response && std::holds_alternative<nfcv::command::Inventory>(command);
}

/// Whether asking the tag again could plausibly get a different answer.
bool is_worth_retrying(nfcv::Error error) {
    // No default case: a new Error value should be classified deliberately
    switch (error) {
    // Our own bug or a fixed fact - the next attempt builds the same request against the same
    // chip and gets the same answer
    case nfcv::Error::invalid_chip:
    case nfcv::Error::response_is_error:
    case nfcv::Error::bad_request:
    case nfcv::Error::not_implemented:
        return false;

    // The answer never arrived or arrived damaged, which is what a neighbouring reader's field
    // does to one
    case nfcv::Error::timeout:
    case nfcv::Error::bad_oscilator:
    case nfcv::Error::invalid_crc:
    // Overloaded: a decode runs out of received bytes with this, which is transient, and an
    // encode reports our own buffer being too small with it, which is not
    case nfcv::Error::buffer_overflow:
    case nfcv::Error::device_hard_framing_error:
    case nfcv::Error::device_soft_framing_error:
    case nfcv::Error::device_parity_error:
    case nfcv::Error::device_crc_error:
    case nfcv::Error::no_response:
    case nfcv::Error::response_invalid_size:
    case nfcv::Error::response_format_invalid:
    case nfcv::Error::field_collision:
    case nfcv::Error::other:
    case nfcv::Error::unknown:
        return true;
    }

    bsod_unreachable();
}

unsigned log_cmd_id(const nfcv::Command &command) {
    return std::to_integer<unsigned>(nfcv::command_id(command));
}

} // namespace

static constexpr std::byte OPER_CONTROL_TX_ENABLE { 0b0000'1000 };
static constexpr std::byte OPER_CONTROL_RX_ENABLE { 0b0100'0000 };

/// io_configuration_1 bit rfo2: which driver and which receiver input single-driver mode uses.
/// 0 selects RFO1/RFI1, 1 selects RFO2/RFI2 (DS13541 Table 20).
static constexpr std::byte ANTENNA_CONTROL_MASK { 0b0100'0000 };

static constexpr nfcv::Error convert_error(st25r39xxb::IRQType error_irqs) {
    using namespace st25r39xxb;
    if (std::to_underlying(error_irqs & IRQType::hard_framing_error)) {
        return nfcv::Error::device_hard_framing_error;
    } else if (std::to_underlying(error_irqs & IRQType::soft_framing_error)) {
        return nfcv::Error::device_soft_framing_error;
    } else if (std::to_underlying(error_irqs & IRQType::parity_error)) {
        return nfcv::Error::device_parity_error;
    } else if (std::to_underlying(error_irqs & IRQType::crc_error)) {
        return nfcv::Error::device_crc_error;
    }
    std::abort();
}

nfcv::Result<void> st25r39xxb::ST25R39XXB::await_interrupt(st25r39xxb::IRQType irqs_to_wait_for, uint32_t timeout_ms, st25r39xxb::IRQType inner_timer_irq, nfcv::Error inner_timer_error) {
    while (true) {
        // Read before sleeping: the read clears the chip's status registers, so its IRQ line is
        // down by the time we wait and the next event still arrives as an edge.
        const auto actionable = irqs_to_wait_for | inner_timer_irq | IRQType::errors;
        const auto irqs = fetch_interrupts();

        if (!std::to_underlying(irqs & actionable)) {
            if (timeout_ms == 0) {
                return std::unexpected(nfcv::Error::timeout);
            }
            timeout_ms = sys_int.await_interrupt(timeout_ms);
            continue;
        }

        if (std::to_underlying(irqs & IRQType::errors)) {
            consume_interrupts(IRQType::errors);
            return std::unexpected(convert_error(irqs));
        }
        // Checked before the awaited IRQs, and so before the timeout: one read can latch several
        // bits (DS13541 §4.3.1), and when the abort condition is among them it is the one that
        // decides. Letting the awaited bits win would report a collision as a field we brought up,
        // or a response that arrived after the no-response deadline as a good one.
        if (static_cast<bool>(inner_timer_irq & irqs)) {
            consume_interrupts(inner_timer_irq);
            return std::unexpected(inner_timer_error);
        }
        consume_interrupts(irqs_to_wait_for & irqs);
        irqs_to_wait_for = irqs_to_wait_for & (~irqs);
        if (std::to_underlying(irqs_to_wait_for) == 0) {
            return {};
        }
    }
}

st25r39xxb::IRQType st25r39xxb::ST25R39XXB::read_interrupt() {
    static_assert(std::endian::native == std::endian::little);
    IRQType res;
    hw_int.read_registers_continuous(RegisterA::main_interrupt, std::span { reinterpret_cast<std::byte *>(&res), sizeof(res) });
    return res;
}

st25r39xxb::IRQType st25r39xxb::ST25R39XXB::fetch_interrupts() {
    // Masked sources still latch and still come out of the read. Keeping them would leave the latch
    // permanently non-empty over bits nobody ever waits for, and every await would stop sleeping.
    latched_irqs = latched_irqs | (read_interrupt() & unmasked_irqs);
    return latched_irqs;
}

void st25r39xxb::ST25R39XXB::consume_interrupts(st25r39xxb::IRQType irqs) {
    latched_irqs = latched_irqs & (~irqs);
}

void st25r39xxb::ST25R39XXB::discard_interrupts() {
    static_cast<void>(read_interrupt());
    latched_irqs = IRQType::none;
}

void st25r39xxb::ST25R39XXB::set_interrupt_mask(st25r39xxb::IRQType mask) {
    static_assert(std::endian::native == std::endian::little);
    hw_int.write_registers_continuous(RegisterA::mask_main_interrupt, std::span { reinterpret_cast<std::byte *>(&mask), sizeof(mask) });
    unmasked_irqs = ~mask;
    // Bits we just stopped caring about must not sit in the latch keeping awaits awake
    latched_irqs = latched_irqs & unmasked_irqs;
}

nfcv::Result<void> st25r39xxb::ST25R39XXB::init() {
    hw_int.direct_command(Command::set_default_2);

    hw_int.write_register(RegisterA::io_configuration_2, std::byte { 0x04 }); // IO configuration register 2

    auto version = hw_int.read_register(RegisterA::ic_identity);
    static constexpr std::byte TYPE_CODE_MASK { 0b1111'1000 };
    static constexpr std::byte ST25R39XXB_ID { 0b0011'0000 };
    if ((version & TYPE_CODE_MASK) != ST25R39XXB_ID) {
        log_error(St25r, "unexpected chip id 0x%02x", std::to_integer<unsigned>(version));
        return std::unexpected(nfcv::Error::invalid_chip);
    }

    // Disable IRQs
    set_interrupt_mask(IRQType::all);
    // Clear IRQs
    discard_interrupts();
    // Flipper here disables overheat protection - we probably don't have this problem so we don't need to do it.
    // But it might be problem in the future. Also It uses undocumented register to do so
    // change_register(static_cast<RegisterB>(0x04), std::byte { 0x10 }, std::byte { 0x10 });

    // Enable internal oscilator
    if (auto res = turn_on_oscilator(); !res.has_value()) {
        return res;
    }

    // Set single antena drving
    hw_int.write_register(RegisterA::io_configuration_1, std::byte { 0x80 });

    // Enable MISO pulldowns
    hw_int.write_register(RegisterA::io_configuration_2, std::byte { 0x1C });

    // Set TX driver resistance
    set_output_amplitude(Amplitude::percent_82);
    sys_int.delay(1);

    // Enables automatic anticollision in NFC-A - why? we are wotking with NFC-V
    // Disables automatic responses in SENSF_RES. WTF is this
    // and again why here?
    hw_int.write_register(RegisterA::passive_target, std::byte { 0x50 });

    // Enables reception even if there are errors in the first 4 bits of the frame
    // TODO: Documentation mentiones ISO-A (NFC-A), make sure that this is needed
    hw_int.write_register(RegisterB::emd_suppression_configuration, std::byte { 0x40 });

    // Do we want to set this? Or even set it here? This values is mode dependent, maybe move it some other mode
    // intialization? Also probably not relevant for NFC-V
    // Also ST25R3919B doesn't support pasive target anything
    hw_int.write_register(RegisterA::passive_target_modulation, std::byte { 0x2F });

    return adjust_regulators();
}

nfcv::Result<void> st25r39xxb::ST25R39XXB::adjust_regulators() {
    // DS13541 §4.4.10: reg_s must be toggled 1 -> 0 before the Adjust regulators command. It resets the
    // adjustment logic and hands the definition of the regulated voltages back to the command.
    // Read-modify-write so the mpsv bits sharing this register are preserved.
    static constexpr std::byte REG_S { 0b1000'0000 };
    hw_int.register_set_bits(RegisterA::regulator_voltage_control, REG_S);
    hw_int.register_clear_bits(RegisterA::regulator_voltage_control, REG_S);

    set_interrupt_mask(~IRQType::direct_command_finished);
    hw_int.direct_command(Command::adjust_regulators);

    /// The Adjust regulators command takes 5 ms max (DS13541 §4.4.10), the rest is slack.
    static constexpr uint32_t adjust_regulators_timeout_ms = 50;

    auto res = await_interrupt(IRQType::direct_command_finished, adjust_regulators_timeout_ms);
    set_interrupt_mask(IRQType::all);

    if (!res.has_value()) {
        log_error(St25r, "regulator adjustment failed: %s", nfcv::to_str(res.error()));
    }
    return res;
}

nfcv::Result<void> st25r39xxb::ST25R39XXB::turn_on_oscilator() {
    static constexpr std::byte OSCILATOR_ENABLE { 0b1000'0000 };
    if ((hw_int.read_register(RegisterA::operation_control) & OSCILATOR_ENABLE) != OSCILATOR_ENABLE) {
        set_interrupt_mask(~IRQType::oscilator_freq_stable);
        hw_int.change_register(RegisterA::operation_control, OSCILATOR_ENABLE, OSCILATOR_ENABLE);
        auto res = await_interrupt(IRQType::oscilator_freq_stable, 10);
        if (!res.has_value()) {
            log_error(St25r, "oscillator did not stabilize: %s", nfcv::to_str(res.error()));
            return std::unexpected(res.error());
        }
    }

    set_interrupt_mask(IRQType::all);
    static constexpr std::byte OSCILATOR_OK { 0b0001'0000 };
    if ((hw_int.read_register(RegisterA::auxilary_display) & OSCILATOR_OK) != OSCILATOR_OK) {
        log_error(St25r, "oscillator reports not ok");
        return std::unexpected(nfcv::Error::bad_oscilator);
    }

    return {};
}

nfcv::Result<void> st25r39xxb::ST25R39XXB::nfcv_command(const nfcv::Command &command) {
    const auto cmd_id = log_cmd_id(command);

    set_interrupt_mask(~(IRQType::tx_end | IRQType::rx_start | IRQType::rx_end | IRQType::no_response_timer_expire | IRQType::general_purpose_timer_expire | IRQType::errors));

    // For write commands we need to wait longer for responses - according to iso from 10 to 20 ms.
    // FIXME: write the damn registers in 1 command
    if (nfcv::is_write_like_command(command)) {
        hw_int.write_register(RegisterA::no_response_timer_1, std::byte { 0x10 });
        hw_int.write_register(RegisterA::no_response_timer_2, std::byte { 0x8e });
    } else {
        hw_int.write_register(RegisterA::no_response_timer_1, std::byte { 0x00 });
        hw_int.write_register(RegisterA::no_response_timer_2, std::byte { 0x52 });
    }

    // Retry the whole exchange a few times if it failed
    for (uint8_t attempt = 0;; attempt++) {

        // stop all previous operations, also clear FIFO
        hw_int.direct_command(Command::stop_all_1);

        // Datasheet (DS13541 §4.2.13) calls for reset rx gain on every transceive - if tag was moved between
        // transmissions, this calibrates the sensitivity and is cheap enough to do every time.
        hw_int.direct_command(Command::reset_rx_gain);

        // Every attempt re-runs the whole transmit-receive sequence, so anything still pending
        // belongs to the previous one. Left alone, a leftover rx_end would satisfy this attempt's
        // await before the tag has said anything. Stop all activities clears the chip's status
        // bits, but our own latch is ours to drop.
        discard_interrupts();

        // The response shares this one buffer with the request, so the request has to be built
        // again for every attempt rather than reused
        buffer.clear();
        if (const auto res = nfcv::construct_command(buffer, command); !res.has_value()) {
            // Not a communication fault - we failed to build the request in the first place
            log_error(St25r, "cmd 0x%02x encode failed: %s", cmd_id, nfcv::to_str(res.error()));
            return res;
        }

        const auto result = nfcv_command_inner(command).and_then([&] {
            return read_nfcv_response(command, cmd_id);
        });
        if (result.has_value()) {
            break;
        }

        const bool give_up = attempt >= nfcv_command_max_attempts - 1 || !is_worth_retrying(result.error());

        if (!is_expected_silence(command, result.error())) {
            log_event(give_up ? logging::Severity::error : logging::Severity::warning, St25r,
                "cmd 0x%02x attempt %u/%u failed: %s",
                cmd_id, static_cast<unsigned>(attempt) + 1u, static_cast<unsigned>(nfcv_command_max_attempts),
                nfcv::to_str(result.error()));
        }

        if (give_up) {
            return result;
        }

        sys_int.delay(retry_delay_ms);
    }

    if (!nfcv::is_response_expected(command)) {
        // We trigger GPT on RX end, but since the command doesn't
        // send a response, then we need to start it manually via command
        hw_int.direct_command(Command::start_general_purpose_timer);
    }

    {
        const auto res = await_interrupt(IRQType::general_purpose_timer_expire, 2);
        if (!res.has_value() && res != std::unexpected(nfcv::Error::timeout)) {
            log_warning(St25r, "cmd 0x%02x inter-command wait failed: %s", cmd_id, nfcv::to_str(res.error()));
            return res;
        }
    }

    return {};
}

[[nodiscard]] nfcv::Result<void> st25r39xxb::ST25R39XXB::read_nfcv_response(const nfcv::Command &command, unsigned cmd_id) {
    if (!nfcv::is_response_expected(command)) {
        return {};
    }

    buffer.clear();
    buffer.resize(get_fifo_len());
    if (!buffer.empty()) {
        hw_int.read_fifo(buffer);
    }

    const auto decoded = nfcv::decode(buffer, buffer);
    if (!decoded.has_value()) {
        // The tag answered, but we could not make sense of the bits
        log_warning(St25r, "cmd 0x%02x response decode failed: %s", cmd_id, nfcv::to_str(decoded.error()));
        return std::unexpected(decoded.error());
    }
    if (decoded->size() < 3) {
        log_warning(St25r, "cmd 0x%02x response too short: %u B", cmd_id, static_cast<unsigned>(decoded->size()));
        return std::unexpected(nfcv::Error::response_invalid_size);
    }

    const auto parsed = nfcv::parse_response(*decoded, command);
    if (!parsed.has_value()) {
        log_warning(St25r, "cmd 0x%02x response parse failed: %s", cmd_id, nfcv::to_str(parsed.error()));
        return std::unexpected(parsed.error());
    }

    return {};
}

[[nodiscard]] nfcv::Result<void> st25r39xxb::ST25R39XXB::nfcv_command_inner(const nfcv::Command &command) {
    // Pass tx_buffer size to the chip (the length is calculated in bits, that why * 8)
    // The size is technically in bits, but only NFC-A supports writing in bits
    uint16_t tx_buffer_len = buffer.size() * 8;
    // Because how the registers are ordererd we need to convert the number to "big endian"
    tx_buffer_len = std::byteswap(tx_buffer_len);
    hw_int.write_registers_continuous(RegisterA::number_of_transmitted_bytes_1, std::span { reinterpret_cast<std::byte *>(&tx_buffer_len), sizeof(tx_buffer_len) });

    // Write the data to the fifo
    hw_int.write_fifo(buffer);

    // Start transmission
    hw_int.direct_command(Command::transmit_without_crc); // Temporary without CRC until I make it calculate it

    {
        const auto res = await_interrupt(IRQType::tx_end, 10);
        if (!res.has_value()) {
            return res;
        }
    }
    // Transmission finished

    // Some commands doesn't send a response according to the iso - like StayQuiet
    if (!nfcv::is_response_expected(command)) {
        return {};
    }

    {
        const auto res = await_interrupt(IRQType::rx_start, 20, IRQType::no_response_timer_expire);
        if (!res.has_value()) {
            // We timed out - we didn't receive any response
            if (res.error() == nfcv::Error::timeout) {
                return std::unexpected(nfcv::Error::no_response);
            }
            return res;
        }
    }

    {
        const auto res = await_interrupt(IRQType::rx_end, 100);
        if (!res.has_value()) {
            return res;
        }
    }

    return {};
}

uint16_t st25r39xxb::ST25R39XXB::get_fifo_len() {
    // Count spans two different registers - 1Eh and 1Fh - one continuous read takes both (DS13541 Tables 66, 67).
    std::array<std::byte, 2> fifo_status {};
    hw_int.read_registers_continuous(st25r39xxb::RegisterA::fifo_status_1, fifo_status);
    return (std::to_integer<uint16_t>(fifo_status.at(0)) | ((std::to_integer<uint16_t>(fifo_status.at(1)) & 0xC0) << 2));
}

void st25r39xxb::ST25R39XXB::set_output_impedance(st25r39xxb::Impedance target_impedance) {
    hw_int.change_register(RegisterA::tx_driver, std::byte { 0x0f }, std::byte { std::to_underlying(target_impedance) });
}

void st25r39xxb::ST25R39XXB::set_output_amplitude(st25r39xxb::Amplitude target_amplitude) {
    hw_int.change_register(RegisterA::tx_driver, std::byte { 0xf0 }, std::byte { std::to_underlying(target_amplitude) });
}

void st25r39xxb::ST25R39XXB::clear_field_bits() {
    hw_int.register_clear_bits(RegisterA::operation_control, OPER_CONTROL_TX_ENABLE | OPER_CONTROL_RX_ENABLE);
}

nfcv::Result<void> st25r39xxb::ST25R39XXB::field_on_with_collision_avoidance() {
    set_interrupt_mask(~(IRQType::collision_detected | IRQType::pwp_field_active));

    // A collision_detected left over from a previous attempt would look like a fresh collision
    discard_interrupts();

    hw_int.direct_command(Command::nfc_init_field_on);

    const auto res = await_interrupt(IRQType::pwp_field_active, field_on_timeout_ms, IRQType::collision_detected, nfcv::Error::field_collision);

    set_interrupt_mask(IRQType::all);

    if (!res.has_value()) {
        // Whatever the command left behind, leave nothing radiating
        clear_field_bits();
    }

    // Deliberately silent: this runs once per arbitration attempt, and whoever asked for the field
    // reports the outcome once - see nfcv::FieldManager
    return res;
}

nfcv::Result<void> st25r39xxb::ST25R39XXB::init_nfcv_poller(const st25r39xxb::ModulationConfiguration &settings) {
    // Enable oscilator and regulator
    // It is recommended that bits en_fd_c<1:0> of the operation control register are set to 01b in Reader mode.
    hw_int.write_register(RegisterA::operation_control, std::byte { 0x81 });
    // Set generic parameter n for NFC * field ON command
    hw_int.write_register(RegisterA::auxilary_definition, std::byte { 0x01 });

    // The peer-detection nibbles keep their reset values: reader mode runs the detector with
    // en_fd_c<1:0> = 01b, which consults only the collision-avoidance ones (DS13541 Table 22).
    static constexpr std::byte PEER_ACTIVATION_RESET { 0x30 };
    static constexpr std::byte PEER_DEACTIVATION_RESET { 0x20 };

    hw_int.write_register(RegisterA::external_field_detector_activation_threshold,
        PEER_ACTIVATION_RESET | std::byte { efd_collision_activation_code });
    hw_int.write_register(RegisterA::external_field_detector_deactivation_threshold,
        PEER_DEACTIVATION_RESET | std::byte { efd_collision_deactivation_code });

    hw_int.write_register(RegisterB::nfc_field_on_guard_timer, std::byte { 0x00 });

    // TODO: Verify this settings, by disabling this command the signal is worse
    // By preliminary testing the 0x03 also worked
    hw_int.write_register(RegisterA::receiver_configuration_1, std::byte { 0x13 });

    // AM and PM correlation signals summed before digitizing (summation mode)
    // Set collision detection level to 53% (compared to data detection level)
    hw_int.write_register(RegisterB::correlator_configuration_1, std::byte { 0x13 });
    // Must be set to 1 for 424 kHz subcarrier stream mode.
    hw_int.write_register(RegisterB::correlator_configuration_2, std::byte { 0x01 });

    // Set:
    // Period of stream mode Tx modulator to 106kHz
    // Number of sub-carrier pulses in report period to 8
    // Sub-carrier frequence to 424kHz
    hw_int.write_register(RegisterA::stream_mode_definition, std::byte { 0x38 });
    {
        // Set operation mode to Sub-carrier stream mode
        std::byte value { 0x70 };
        // and RF modulation mode based on the settings (3rd lowest bit 0 - ook 1 - am)
        if (std::holds_alternative<config::AMModulation>(settings)) {
            value |= std::byte { 1 << 2 };
        }

        hw_int.change_register(RegisterA::mode_definition, std::byte { 0x7c }, value);
    }

    {
        const Amplitude target_amplitude = std::visit(
            Overloaded {
                [](const config::AMModulation &mod) {
                    return mod.target_amplitude;
                },
                [](const auto &) {
                    return Amplitude::percent_82;
                } },
            settings);

        set_output_amplitude(target_amplitude);
    }

    set_output_impedance(Impedance::ohm2);

    // Try Setup AWS
    if (std::visit([](const auto &settings) {
            return settings.aws.has_value();
        },
            settings)) {
        // Default values taken from reference manual
        // TODO: anotate what they do
        hw_int.write_register(RegisterB::auxiliary_modulation_setting, std::byte { 0x94 });
        hw_int.write_register(RegisterB::aws_config_1, std::byte { 0x08 });

        // Specific setting for eather type of modulation
        std::visit(Overloaded {
                       [&](const config::OOKModulation &) {
                           // For OOK we enable strong sink during modulation and non-symetrical shape
                           hw_int.change_register(RegisterB::aws_config_2, std::byte { 0xf0 }, std::byte { 0x10 });
                       },
                       [&](const config::AMModulation &) {
                           // For ASK we enable weak sink during modulation and symetrical shape
                           hw_int.change_register(RegisterB::aws_config_2, std::byte { 0xf0 }, std::byte { 0x20 });
                       } },
            settings);
        // Reference manual mentions setting modulation amplitude, but that is done above

        const auto aws_preset = std::visit([](const auto &settings) -> config::AWSTransient { return settings.aws.value(); }, settings);
        // Set transients based on reference manual
        // TODO: We might want to set these manually too and maybe keep these as default "presets"
        switch (aws_preset) {
        case config::AWSTransient::slow:
            hw_int.change_register(RegisterB::aws_config_2, std::byte { 0x0f }, std::byte { 0x0c });
            hw_int.write_register(RegisterB::aws_time_1, std::byte { 0x01 });
            hw_int.write_register(RegisterB::aws_time_3, std::byte { 0x9c });
            hw_int.write_register(RegisterB::aws_time_4, std::byte { 0x0a });
            break;
        case config::AWSTransient::medium:
            hw_int.change_register(RegisterB::aws_config_2, std::byte { 0x0f }, std::byte { 0x08 });
            hw_int.write_register(RegisterB::aws_time_1, std::byte { 0x01 });
            hw_int.write_register(RegisterB::aws_time_3, std::byte { 0x79 });
            hw_int.write_register(RegisterB::aws_time_4, std::byte { 0x07 });
            break;
        case config::AWSTransient::fast:
            hw_int.change_register(RegisterB::aws_config_2, std::byte { 0x0f }, std::byte { 0x04 });
            hw_int.write_register(RegisterB::aws_time_1, std::byte { 0x01 });
            hw_int.write_register(RegisterB::aws_time_3, std::byte { 0x57 });
            hw_int.write_register(RegisterB::aws_time_4, std::byte { 0x06 });
            break;
        }
    }

    hw_int.direct_command(Command::change_am_modulation_state);

    // Force general purpose timer (gpt) to start automatically at the end of RX
    // It can always be started with Command::start_general_purpose_timer
    // Also we set Mask receive timer step size to 0 => 64/fc 4.72us - please be very careful when changing
    hw_int.write_register(RegisterA::timer_and_emv_control, std::byte { 0x20 });
    // We will be using gpt for waiting between two commands (which is about 300us)
    // Every "tick" of gpt is ~590ns. So 300'000/590 = 508.484 => 509 (0x01fd)
    hw_int.write_register(RegisterA::general_purpose_timer_1, std::byte { 0x01 }); // MSB
    hw_int.write_register(RegisterA::general_purpose_timer_2, std::byte { 0xfd }); // LSB

    // Set modulation width to 112 periods of 13.56MHz clock
    // Disable parity bit generation
    hw_int.write_register(RegisterA::ISO14443A, std::byte { 0xdc });
    // Sets time after end of TX during which receiver output is ignored.
    // 4.72us * 0x41 = 306.8 us
    hw_int.write_register(RegisterA::receiver_timer_mask, std::byte { 0x41 });

    // Leave nothing radiating: a board that finished init early would otherwise jam every
    // later-booting neighbour's own init until its first discovery pass.
    // Clearing the bits directly rather than calling field_down(), which early-outs while
    // active_antenna is unset - which it always is here.
    clear_field_bits();

    return {};
}

nfcv::Result<void> st25r39xxb::ST25R39XXB::try_field_up(AntennaID antenna) {
    if (active_antenna == antenna) {
        return {};
    }

    // Electro recommended having the field down during switching
    field_down();

    // Select antenna.
    hw_int.change_register(RegisterA::io_configuration_1, ANTENNA_CONTROL_MASK, antenna ? ANTENNA_CONTROL_MASK : std::byte { 0 });

    hw_int.write_register(RegisterB::nfc_field_on_guard_timer, std::byte { 0 });

    if (const auto res = field_on_with_collision_avoidance(); !res.has_value()) {
        return res;
    }

    // Idempotent if nfc_init_field_on already raised them, essential if it did not
    hw_int.register_set_bits(RegisterA::operation_control, OPER_CONTROL_RX_ENABLE | OPER_CONTROL_TX_ENABLE);

    hw_int.direct_command(Command::stop_all_1);
    hw_int.direct_command(Command::reset_rx_gain);

    sys_int.delay(field_settle_ms);

    active_antenna = antenna;

    return {};
}

void st25r39xxb::ST25R39XXB::field_down() {
    if (!active_antenna.has_value()) {
        return;
    }

    clear_field_bits();

    // The next command fails if we do field_down - field_up - command cycle without this delay
    sys_int.delay(30);

    active_antenna = std::nullopt;
}
