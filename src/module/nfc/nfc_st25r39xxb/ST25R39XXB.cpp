#include <st25r39xxb/ST25R39XXB.hpp>
#include <st25r39xxb/ST25R39XXB_defs.hpp>

#include <raii/scope_guard.hpp>
#include <iso13239/crc.hpp>
#include <inplace_vector.hpp>
#include <nfcv/encode.hpp>
#include <nfcv/decode.hpp>
#include <utils/overloaded_visitor.hpp>
#include <utils/byte_utils.hpp>

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

nfcv::Result<void> st25r39xxb::ST25R39XXB::await_interrupt(st25r39xxb::IRQType irqs_to_wait_for, uint32_t timeout_ms, st25r39xxb::IRQType inner_timer_irq) {
    while (true) {
        timeout_ms = sys_int.await_interrupt(timeout_ms);
        const auto irqs = read_interrupt();
        if (std::to_underlying(irqs & IRQType::errors)) {
            return std::unexpected(convert_error(irqs));
        }
        irqs_to_wait_for = irqs_to_wait_for & (~irqs);
        if (std::to_underlying(irqs_to_wait_for) == 0) {
            break;
        }
        if (timeout_ms == 0 || static_cast<bool>(inner_timer_irq & irqs)) {
            return std::unexpected(nfcv::Error::timeout);
        }
    }

    return {};
}

st25r39xxb::IRQType st25r39xxb::ST25R39XXB::read_interrupt() {
    static_assert(std::endian::native == std::endian::little);
    IRQType res;
    hw_int.read_registers_continuous(RegisterA::main_interrupt, std::span { reinterpret_cast<std::byte *>(&res), sizeof(res) });
    return res;
}

void st25r39xxb::ST25R39XXB::set_interrupt_mask(st25r39xxb::IRQType mask) {
    static_assert(std::endian::native == std::endian::little);
    hw_int.write_registers_continuous(RegisterA::mask_main_interrupt, std::span { reinterpret_cast<std::byte *>(&mask), sizeof(mask) });
}

nfcv::Result<void> st25r39xxb::ST25R39XXB::init() {
    hw_int.direct_command(Command::set_default_2);

    hw_int.write_register(RegisterA::io_configuration_2, std::byte { 0x04 }); // IO configuration register 2

    auto version = hw_int.read_register(RegisterA::ic_identity);
    static constexpr std::byte TYPE_CODE_MASK { 0b1111'1000 };
    static constexpr std::byte ST25R39XXB_ID { 0b0011'0000 };
    if ((version & TYPE_CODE_MASK) != ST25R39XXB_ID) {
        return std::unexpected(nfcv::Error::invalid_chip);
    }

    // Disable IRQs
    set_interrupt_mask(IRQType::all);
    // Clear IRQs
    [[maybe_unused]] const auto v = read_interrupt();
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

    set_interrupt_mask(~IRQType::direct_command_finished);
    hw_int.direct_command(Command::adjust_regulators);

    if (auto res = await_interrupt(IRQType::direct_command_finished, 500); !res.has_value()) {
        return res;
    }

    set_interrupt_mask(IRQType::all);

    return {};
}

nfcv::Result<void> st25r39xxb::ST25R39XXB::turn_on_oscilator() {
    static constexpr std::byte OSCILATOR_ENABLE { 0b1000'0000 };
    if ((hw_int.read_register(RegisterA::operation_control) & OSCILATOR_ENABLE) != OSCILATOR_ENABLE) {
        set_interrupt_mask(~IRQType::oscilator_freq_stable);
        hw_int.change_register(RegisterA::operation_control, OSCILATOR_ENABLE, OSCILATOR_ENABLE);
        auto res = await_interrupt(IRQType::oscilator_freq_stable, 10);
        if (!res.has_value()) {
            return std::unexpected(res.error());
        }
    }

    set_interrupt_mask(IRQType::all);
    static constexpr std::byte OSCILATOR_OK { 0b0001'0000 };
    if ((hw_int.read_register(RegisterA::auxilary_display) & OSCILATOR_OK) != OSCILATOR_OK) {
        return std::unexpected(nfcv::Error::bad_oscilator);
    }

    return {};
}

nfcv::Result<void> st25r39xxb::ST25R39XXB::nfcv_command(const nfcv::Command &command) {
    // Prepare message
    buffer.clear();
    {
        const auto construct_res = nfcv::construct_command(buffer, command);
        if (!construct_res.has_value()) {
            return construct_res;
        }
    }

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

    // Retry sending the message a few times if we failed
    for (uint8_t attempt = 0;; attempt++) {
        // Clear fifos before each retry
        hw_int.direct_command(Command::clear_fifo);

        const auto result = nfcv_command_inner(command);
        if (result.has_value()) {
            break;
        }

        if (attempt >= nfcv_command_max_attempts - 1) {
            // Failed too many times - return the last failure
            return result;
        }

        sys_int.delay(retry_delay_ms);
    }

    if (nfcv::is_response_expected(command)) {
        // Read received data
        buffer.clear();
        buffer.resize(get_fifo_len());
        if (!buffer.empty()) {
            hw_int.read_fifo(buffer);
        }

        WritableBytes decoded_data;
        {
            const auto res = nfcv::decode(buffer, buffer);
            if (!res.has_value()) {
                return std::unexpected(res.error());
            }

            decoded_data = *res;
            if (decoded_data.size() < 3) {
                return std::unexpected(nfcv::Error::response_invalid_size);
            }
        }

        // Parse response to message
        {
            const auto res = nfcv::parse_response(decoded_data, command);
            if (!res.has_value()) {
                return std::unexpected(res.error());
            }
        }

    } else {
        // We trigger GPT on RX end, but since the command doesn't
        // send a response, then we need to start it manually via command
        hw_int.direct_command(Command::start_general_purpose_timer);
    }

    {
        const auto res = await_interrupt(IRQType::general_purpose_timer_expire, 2);
        if (!res.has_value() && res != std::unexpected(nfcv::Error::timeout)) {
            return res;
        }
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
    std::array<std::byte, 2> fifo_status {};
    hw_int.read_registers_continuous(st25r39xxb::RegisterA::fifo_status_1, fifo_status);
    return (std::to_integer<uint16_t>(fifo_status.at(0)) | ((std::to_integer<uint16_t>(fifo_status.at(1)) & 0xB0) << 2));
}

void st25r39xxb::ST25R39XXB::set_output_impedance(st25r39xxb::Impedance target_impedance) {
    hw_int.change_register(RegisterA::tx_driver, std::byte { 0x0f }, std::byte { std::to_underlying(target_impedance) });
}

void st25r39xxb::ST25R39XXB::set_output_amplitude(st25r39xxb::Amplitude target_amplitude) {
    hw_int.change_register(RegisterA::tx_driver, std::byte { 0xf0 }, std::byte { std::to_underlying(target_amplitude) });
}

nfcv::Result<void> st25r39xxb::ST25R39XXB::init_nfcv_poller(const st25r39xxb::ModulationConfiguration &settings) {
    // Enable oscilator and regulator
    // It is recommended that bits en_fd_c<1:0> of the operation control register are set to 01b in Reader mode.
    hw_int.write_register(RegisterA::operation_control, std::byte { 0x81 });
    // Set generic parameter n for NFC * field ON command
    hw_int.write_register(RegisterA::auxilary_definition, std::byte { 0x01 });

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

    set_interrupt_mask(~(IRQType::collision_detected | IRQType::pwp_field_active));
    hw_int.direct_command(Command::nfc_init_field_on);

    if (const auto res = await_interrupt(IRQType::pwp_field_active, 100, IRQType::collision_detected); !res.has_value()) {
        return res;
    }

    set_interrupt_mask(IRQType::all);

    return {};
}

nfcv::Result<void> st25r39xxb::ST25R39XXB::field_up(AntennaID antenna) {
    if (active_antenna == antenna) {
        return {};
    }

    // Electro recommended having the field down during switching
    field_down();

    // Select antenna
    static constexpr std::byte ANTENNA_CONTROL_MASK { 0b0100'0000 };
    hw_int.change_register(RegisterA::io_configuration_1, ANTENNA_CONTROL_MASK, antenna ? ANTENNA_CONTROL_MASK : std::byte { 0 });

    hw_int.write_register(RegisterB::nfc_field_on_guard_timer, std::byte { 0 });
    static constexpr std::byte OPER_CONTROL_TX_ENABLE { 0b0000'1000 };
    static constexpr std::byte OPER_CONTROL_RX_ENABLE { 0b0100'0000 };
    hw_int.register_set_bits(RegisterA::operation_control, OPER_CONTROL_RX_ENABLE | OPER_CONTROL_TX_ENABLE);

    hw_int.direct_command(Command::stop_all_1);
    hw_int.direct_command(Command::reset_rx_gain);

    sys_int.delay(50);

    active_antenna = antenna;

    return {};
}

void st25r39xxb::ST25R39XXB::field_down() {
    if (!active_antenna.has_value()) {
        return;
    }

    static constexpr std::byte OPER_CONTROL_TX_ENABLE { 0b0000'1000 };
    static constexpr std::byte OPER_CONTROL_RX_ENABLE { 0b0100'0000 };
    hw_int.register_clear_bits(RegisterA::operation_control, OPER_CONTROL_TX_ENABLE | OPER_CONTROL_RX_ENABLE);

    // The next command fails if we do field_down - field_up - command cycle without this delay
    sys_int.delay(30);

    active_antenna = std::nullopt;
}
