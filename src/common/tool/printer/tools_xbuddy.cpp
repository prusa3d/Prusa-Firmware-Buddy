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
    #include <bsod/bsod.h>

LOG_COMPONENT_DEF(HotendDetect, logging::Severity::info);

namespace {

/// What the boot-time reading looked like, for the log.
enum class EarlyReading : uint8_t {
    clear_ntc,
    ambiguous,
    open_or_shorted,
};

/// The boot-time classification. Kept until it can be logged: USB CDC logging only opens once a
/// host has attached, which is later than the detection runs.
struct EarlyClassification {
    hotend_detect::Outputs result;
    uint16_t adc_10bit;
    EarlyReading reading;
    bool stored_is_high_temp;
};

/// Written and read on the startup task.
EarlyClassification early {};

const char *to_string(EarlyReading reading) {
    switch (reading) {
    case EarlyReading::clear_ntc:
        return "NTC";
    case EarlyReading::ambiguous:
        return "ambiguous";
    case EarlyReading::open_or_shorted:
        return "open/short";
    }
    bsod_unreachable();
}

} // namespace

/// Read the ADC, derive the thresholds from the temptables and classify via detect_impl.
static EarlyClassification detect_hotend(bool stored_is_high_temp) {
    static_assert(PhysicalToolIndex::count == 1, "Hotend detection assumes a single physical tool");
    const uint16_t adc = AdcGet::nozzle(); // 10-bit, shifted from 12-bit by get_and_shift_channel()

    // Derive the PT1000 ADC range from the actual temptable (auto-adapts if the table changes)
    constexpr uint16_t detection_margin = 12; // 10-bit ADC counts
    const auto pt1000_range = MarlinTemptableRawMinMax::compute(TT_NAME(1010), 0, 400);

    // Open/short limits are the NTC table's full span, unpadded: its cold end is only a few
    // counts past the coldest supported ambient. Note the NTC inversion: raw_min is the cold end.
    constexpr int16_t ntc_table_coldest_c = -30;
    constexpr int16_t ntc_table_hottest_c = 320;
    const auto ntc_range = MarlinTemptableRawMinMax::compute(TT_NAME(2005), ntc_table_coldest_c, ntc_table_hottest_c);

    const hotend_detect::Thresholds thresholds {
        .clear_ntc_upper = static_cast<uint16_t>(pt1000_range.raw_max / OVERSAMPLENR + detection_margin),
        .clear_ntc_lower = static_cast<uint16_t>(pt1000_range.raw_min / OVERSAMPLENR - detection_margin),
        .shorted_max = static_cast<uint16_t>(ntc_range.raw_max / OVERSAMPLENR),
        .open_min = static_cast<uint16_t>(ntc_range.raw_min / OVERSAMPLENR),
    };

    const bool open_or_shorted = adc <= thresholds.shorted_max || adc >= thresholds.open_min;
    const bool in_band = adc >= thresholds.clear_ntc_lower && adc <= thresholds.clear_ntc_upper;
    return {
        .result = hotend_detect::detect_impl({ adc, stored_is_high_temp }, thresholds),
        .adc_10bit = adc,
        .reading = open_or_shorted ? EarlyReading::open_or_shorted
            : in_band              ? EarlyReading::ambiguous
                                   : EarlyReading::clear_ntc,
        .stored_is_high_temp = stored_is_high_temp,
    };
}

void log_hotend_detect_boot_result() {
    log_info(HotendDetect, "early: adc %u %s, stored %s -> %s%s",
        early.adc_10bit,
        to_string(early.reading),
        early.stored_is_high_temp ? "HT" : "std",
        early.result.is_high_temp ? "HT" : "std",
        early.result.dialog_pending ? ", ask" : "");
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

        early = detect_hotend(stored_is_ht);
        const auto result = early.result;

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
            .min_nozzle_temp = 5,
            .max_nozzle_temp = 305,
        },
        .nozzle_temp_table = TT_NAME(2005),
        .heatbreak_temp_table = heatbreak_temptable(),
        .nozzle_heater_marlin_pin = MARLIN_PIN(HEAT0),
        .nozzle_heater_soft_pwm = false,
    };

    // HT hotend config (PT1000)
    static const LocalHotend::Config pt1000_config {
        .base_config {
            .min_nozzle_temp = 5,
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
            .min_nozzle_temp = 5,
            .max_nozzle_temp = 305,
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
