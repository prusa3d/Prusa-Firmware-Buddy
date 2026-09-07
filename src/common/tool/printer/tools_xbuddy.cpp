/// @file

#include <algorithm>

#include <tool/tool/standard_fff_physical_tool.hpp>
#include <tool/hotend/hotend/local_hotend.hpp>
#include <module/thermistor/thermistors.h>
#include <hw_configuration.hpp>
#include <hotend_detect.hpp>
#include <adc.hpp>
#include <config_store/store_instance.hpp>
#include <logging/log.hpp>

#if HAS_HT_HOTEND()
LOG_COMPONENT_DEF(HotendDetect, logging::Severity::info);

/// Production wrapper: read the ADC, derive the PT1000 thresholds from the temptable, and
/// delegate to the pure hotend_detect::detect_impl (which lives in hotend_detect.hpp so unit
/// tests can reach it without ADC / config_store dependencies). Mapping the result onto the
/// stored HotendType and its side effects live at the call site.
static hotend_detect::Outputs detect_hotend(bool stored_is_high_temp) {
    static_assert(PhysicalToolIndex::count == 1, "Hotend detection assumes a single physical tool");
    const uint16_t adc = AdcGet::nozzle(); // 10-bit, shifted from 12-bit by get_and_shift_channel()

    // Derive the PT1000 ADC range from the actual temptable (auto-adapts if the table changes)
    constexpr uint16_t detection_margin = 12; // 10-bit ADC counts
    const auto pt1000_range = MarlinTemptableRawMinMax::compute(TT_NAME(1010), 0, 400);
    const hotend_detect::Thresholds thresholds {
        .clear_ntc_upper = static_cast<uint16_t>(pt1000_range.raw_max / OVERSAMPLENR + detection_margin),
        .clear_ntc_lower = static_cast<uint16_t>(pt1000_range.raw_min / OVERSAMPLENR - detection_margin),
    };

    const auto result = hotend_detect::detect_impl({ adc, stored_is_high_temp }, thresholds);
    log_info(HotendDetect, "ADC %u, stored_ht %u -> ht %u, dialog %u",
        adc, static_cast<unsigned>(stored_is_high_temp),
        static_cast<unsigned>(result.is_high_temp), static_cast<unsigned>(result.dialog_pending));
    return result;
}
#endif

#if !PRINTER_IS_PRUSA_MK3_5()
static MarlinTempTable heatbreak_temptable() {
    if (buddy::hw::Configuration::Instance().needs_heatbreak_thermistor_table_5()) {
        return TT_NAME(5);
    } else {
        return TT_NAME(TEMP_SENSOR_HEATBREAK);
    }
}
#endif

PhysicalTool &PhysicalTool::for_index(PhysicalToolIndex) {
#if HAS_HT_HOTEND()
    // Detect the sensor once and reconcile it with the persisted HotendType. See
    // hotend_detect.hpp for the classification; the force-on-transition policy lives here.
    static const HotendType hotend_type = [] {
        const auto tool = PhysicalToolIndex::from_raw(0);
        const bool stored_is_ht = (config_store().hotend_type.get(0) == HotendType::high_temp);

        const auto result = detect_hotend(stored_is_ht);

        // ScreenSplash reads this on the GUI task after the scheduler starts.
        hotend_detect_dialog_pending = result.dialog_pending;

        // Force the sock only on a class change (NTC <-> PT1000): a merged HotendType can't recall
        // the prior sock choice, so switching to HT clears it and switching back to a standard
        // hotend forces it on (a CoreONE needs one). No class change -> keep the stored choice.
        if (result.is_high_temp != stored_is_ht) {
            config_store().set_hotend_type_detected(tool,
                result.is_high_temp ? HotendType::high_temp : HotendType::stock_with_sock);
        }

        return config_store().hotend_type.get(0);
    }();

    // Standard hotend config (NTC thermistor) — same limits as the non-HT xbuddy build.
    static const LocalHotend::Config ntc_config {
        .base_config {
            .min_nozzle_temp = HEATER_0_MINTEMP,
            .max_nozzle_temp = HEATER_0_MAXTEMP,
        },
        .nozzle_temp_table = TT_NAME(2005),
        .heatbreak_temp_table = heatbreak_temptable(),
        .nozzle_heater_marlin_pin = MARLIN_PIN(HEAT0),
        .nozzle_heater_soft_pwm = false,
    };

    // HT hotend config (PT1000)
    static const LocalHotend::Config pt1000_config {
        .base_config {
            .min_nozzle_temp = HEATER_0_MINTEMP,
            .max_nozzle_temp = ht_hotend_max_nozzle_temp,
            // Protect the print fan at high nozzle temperatures: above 300 °C, cap it at
            // ~30 %. nozzle_temp() is OptionalTemperature; treat "unknown" as cold
            // (value_or(0)) so the fan is never clamped before the first ADC reading.
            .print_fan_pwm_mapping = +[](const Hotend &hotend, PWM255 requested_pwm) -> PWM255 {
                constexpr Hotend::TargetTemperature clamp_above_nozzle_temp = 300;
                constexpr PWM255 clamped_max_pwm { 77 }; // ~30 % of 255
                if (hotend.nozzle_temp().value_or(0) > clamp_above_nozzle_temp) {
                    return std::min(requested_pwm, clamped_max_pwm);
                }
                return requested_pwm;
            },
        },
        .nozzle_temp_table = TT_NAME(1010),
        .heatbreak_temp_table = heatbreak_temptable(),
        .nozzle_heater_marlin_pin = MARLIN_PIN(HEAT0),
        .nozzle_heater_soft_pwm = false,
        .nozzle_filter_max_temp = 500, // Always filter — PT1000 benefits from smoothing at all temps
    };

    // hotend_type is read from EEPROM; anything other than high_temp (including a corrupted or
    // future value) falls back to the standard, lower-temperature config rather than bricking
    // the printer (config-store enum policy). A cold boot re-runs detection.
    const LocalHotend::Config *config = (hotend_type == HotendType::high_temp) ? &pt1000_config : &ntc_config;

    static StandardFFFPhysicalTool<LocalHotend> physical_tool { PhysicalToolIndex::from_raw(0), config };
    static_assert(PhysicalToolIndex::count == 1);

    return physical_tool;
#else
    static const LocalHotend::Config hotend_config {
        .base_config {
            // TODO: Get rid of the macros, put the values directly into this file
            .min_nozzle_temp = HEATER_0_MINTEMP,
            .max_nozzle_temp = HEATER_0_MAXTEMP,
        },
            .nozzle_temp_table = TT_NAME(THERMISTOR_HEATER_0),
    #if !PRINTER_IS_PRUSA_MK3_5()
            .heatbreak_temp_table = heatbreak_temptable(),
    #endif
            .nozzle_heater_marlin_pin = MARLIN_PIN(HEAT0),
            .nozzle_heater_soft_pwm = false,
    };
    static StandardFFFPhysicalTool<LocalHotend> physical_tool { PhysicalToolIndex::from_raw(0), &hotend_config };
    static_assert(PhysicalToolIndex::count == 1);

    return physical_tool;
#endif
}
