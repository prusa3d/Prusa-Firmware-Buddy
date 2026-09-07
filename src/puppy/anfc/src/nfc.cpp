#include "nfc.hpp"
#include "hal.hpp"
#include "device/peripherals.hpp"

#include <st25r39xxb/ST25R39XXB.hpp>
#include <freertos/timing.hpp>
#include <freertos/binary_semaphore.hpp>

#include <option/nfc_st25r3919b_aws_config.h>
#include <option/nfc_st25r3919b_modulation.h>

#include <bsod/bsod.h>

#include <stm32c0xx_hal.h>
#include <utils/byte_utils.hpp>

namespace nfc {
namespace {
    class HWImpl : public st25r39xxb::SpiInterface {
    public:
        HWImpl(SPI_HandleTypeDef *spi, GPIO_TypeDef *chip_select_port, uint16_t chip_select_pin)
            : spi(spi)
            , chip_select_port(chip_select_port)
            , chip_select_pin(chip_select_pin) {}

        void chip_select() final {
            HAL_GPIO_WritePin(chip_select_port, chip_select_pin, GPIO_PIN_RESET);
        }

        void chip_deselect() final {
            HAL_GPIO_WritePin(chip_select_port, chip_select_pin, GPIO_PIN_SET);
        }

        void unsafe_transmit(const Bytes &tx) final {
            [[maybe_unused]] const auto res = HAL_SPI_Transmit(spi, reinterpret_cast<const uint8_t *>(tx.data()), tx.size(), 100);
            if (res != HAL_OK) {
                hal::panic();
            }
        }

        void unsafe_receive(const WritableBytes &rx) final {
            [[maybe_unused]] const auto res = HAL_SPI_Receive(spi, reinterpret_cast<uint8_t *>(rx.data()), rx.size(), 100);
            if (res != HAL_OK) {
                hal::panic();
            }
        }

    protected:
        SPI_HandleTypeDef *spi;
        GPIO_TypeDef *chip_select_port;
        uint16_t chip_select_pin;
    };

    struct SysImpl : public st25r39xxb::SystemInterface {
        void delay(uint32_t delay_ms) final {
            freertos::delay(delay_ms);
        }

        uint32_t await_interrupt(uint32_t timeout_ms) final {
            const auto start = freertos::millis();
            irq_triggered.try_acquire_for(timeout_ms);
            const auto passed = freertos::millis() - start;
            if (passed > timeout_ms) {
                return 0;
            }
            return timeout_ms - passed;
        }

        void trigger_interrupt() final {
            irq_triggered.release_from_isr();
        }

    protected:
        freertos::BinarySemaphore irq_triggered;
    };

    namespace nfcr1 {
        static const auto chip_select_port = GPIOA;
        static constexpr auto chip_select_pin = GPIO_PIN_4;

        HWImpl hw_impl(&hal::peripherals::hspi1, chip_select_port, chip_select_pin);
        SysImpl sys_impl {};
    } // namespace nfcr1
} // namespace
st25r39xxb::ST25R39XXB reader_1 { nfcr1::hw_impl, nfcr1::sys_impl };
} // namespace nfc

static void configure_readers(const st25r39xxb::ModulationConfiguration &mod_conf) {
    auto init_res = nfc::reader_1.init();
    if (!init_res.has_value()) {
        hal::panic();
    }

    init_res = nfc::reader_1.init_nfcv_poller(mod_conf);
    if (!init_res.has_value()) {
        hal::panic();
    }
}

static constexpr st25r39xxb::Amplitude floor_to_valid_amplitude(uint8_t raw_value) {
    static constexpr std::array<std::pair<uint8_t, st25r39xxb::Amplitude>, 16> conversion_table {
        std::pair { 0, st25r39xxb::Amplitude::percent_0 },
        std::pair { 8, st25r39xxb::Amplitude::percent_8 },
        std::pair { 10, st25r39xxb::Amplitude::percent_10 },
        std::pair { 11, st25r39xxb::Amplitude::percent_11 },
        std::pair { 12, st25r39xxb::Amplitude::percent_12 },
        std::pair { 13, st25r39xxb::Amplitude::percent_13 },
        std::pair { 14, st25r39xxb::Amplitude::percent_14 },
        std::pair { 15, st25r39xxb::Amplitude::percent_15 },
        std::pair { 20, st25r39xxb::Amplitude::percent_20 },
        std::pair { 25, st25r39xxb::Amplitude::percent_25 },
        std::pair { 30, st25r39xxb::Amplitude::percent_30 },
        std::pair { 40, st25r39xxb::Amplitude::percent_40 },
        std::pair { 50, st25r39xxb::Amplitude::percent_50 },
        std::pair { 60, st25r39xxb::Amplitude::percent_60 },
        std::pair { 70, st25r39xxb::Amplitude::percent_70 },
        std::pair { 82, st25r39xxb::Amplitude::percent_82 },
    };

    const auto it = std::ranges::lower_bound(conversion_table, raw_value, std::less {}, [](const auto &a) { return a.first; });
    debug_assert(it != std::end(conversion_table));
    return it->second;
}

void nfc::readers_init() {
    static constexpr auto aws_conf = []() consteval -> st25r39xxb::config::AWS {
        switch (option::nfc_st25r3919b_aws_config) {
        case option::NfcSt25r3919bAwsConfig::no_aws:
            return { std::nullopt };
        case option::NfcSt25r3919bAwsConfig::slow:
            return { st25r39xxb::config::AWSTransient::slow };
        case option::NfcSt25r3919bAwsConfig::medium:
            return { st25r39xxb::config::AWSTransient::medium };
        case option::NfcSt25r3919bAwsConfig::fast:
            return { st25r39xxb::config::AWSTransient::fast };
        }
    }();
    static_assert(option::nfc_st25r3919b_modulation >= 0 && option::nfc_st25r3919b_modulation <= 100);
    if constexpr (option::nfc_st25r3919b_modulation == 100) {
        configure_readers({ st25r39xxb::config::OOKModulation { { aws_conf } } });
    } else {
        configure_readers({ st25r39xxb::config::AMModulation { { aws_conf }, floor_to_valid_amplitude(option::nfc_st25r3919b_modulation) } });
    }
}

void nfc::reconfigure_readers(const prusa3d_nfc_request_config_ModulationConfig_1_0 &config) {
    st25r39xxb::ModulationConfiguration res;

    // If target amplitude is set to 100% use OOK Modulation which does technically the same
    if (config.target_amplitude >= 100) {
        res = st25r39xxb::config::OOKModulation { { std::nullopt } };
    } else {
        // for other values we need to "floor" the desired value to a value that can be configured
        res = st25r39xxb::config::AMModulation { { std::nullopt }, floor_to_valid_amplitude(config.target_amplitude) };
    }

    // If we have an AWS preset then propagate correct preset to readers
    if (config.aws_config.value != prusa3d_nfc_request_config_AwsConfig_1_0_NO_AWS) {
        st25r39xxb::config::AWS aws_config {};

        switch (config.aws_config.value) {
        case prusa3d_nfc_request_config_AwsConfig_1_0_AWS_SLOW_TRANSIENT:
            aws_config = st25r39xxb::config::AWSTransient::slow;
            break;
        case prusa3d_nfc_request_config_AwsConfig_1_0_AWS_MEDIUM_TRANSIENT:
            aws_config = st25r39xxb::config::AWSTransient::medium;
            break;
        case prusa3d_nfc_request_config_AwsConfig_1_0_AWS_FAST_TRANSIENT:
            aws_config = st25r39xxb::config::AWSTransient::fast;
            break;
        default:
            bsod_unreachable();
        }

        std::visit([&](auto &a) { a.aws = aws_config; }, res);
    }
    configure_readers(res);
}

void nfc::irq() {
    nfcr1::sys_impl.trigger_interrupt();
}
