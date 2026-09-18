/// @file

#include <algorithm>

#include <tool/tool/standard_fff_physical_tool.hpp>
#include <tool/hotend/hotend/local_hotend.hpp>
#include <module/thermistor/thermistors.h>
#include <hw_configuration.hpp>
#include <hotend_detect.hpp>
#include <hotend_type.hpp>
#include <adc.hpp>
#include <config_store/store_instance.hpp>
#include <logging/log.hpp>
#include <bsod/bsod.h>
#include <fanctl.hpp>
#include <freertos/timing.hpp>
#include <buddy/bootstrap_state.hpp>

#if HAS_HT_HOTEND()
    #include <atomic>
    #include <bsod/bsod.h>
    #include <buddy/bootstrap_state.hpp>
    #include <fanctl.hpp>
    #include <freertos/timing.hpp>
    #include <hotend_type.hpp>
    #include <hwio.h>
    #include <option/has_power_panic.h>
    #include <power_panic.hpp>
    #include <timing.h>
    #include <wdt.hpp>

// resolve() must recognise a power-panic resume to stay out of its way.
static_assert(HAS_POWER_PANIC());

LOG_COMPONENT_DEF(HotendDetect, logging::Severity::info);

namespace {

/// What the boot-time reading looked like, for the log.
enum class EarlyReading : uint8_t {
    clear_ntc,
    ambiguous,
    open_or_shorted,
};

/// The early classification, consumed when the type is settled.
struct EarlyClassification {
    hotend_detect::Outputs result;
    uint16_t adc_10bit;
    EarlyReading reading;
    bool stored_is_high_temp;
};

/// Written by classify_early() before the marlin task is created, read on that task by resolve()
/// (task creation orders the two) or on the startup task by init_skipped().
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

void log_early_classification() {
    log_info(HotendDetect, "early: adc %u %s, stored %s -> %s%s",
        early.adc_10bit,
        to_string(early.reading),
        early.stored_is_high_temp ? "HT" : "std",
        early.result.is_high_temp ? "HT" : "std",
        early.result.dialog_pending ? ", ask" : "");
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

namespace {

/// Lets PhysicalTool::for_index() catch a caller that arrives before the type is settled.
enum class DetectState : uint8_t {
    not_initialised,
    skipped, ///< settled from the early classification alone, without probing
    resolved, ///< settled by hotend_detect::resolve()
};

/// Thread sync: written once by settle(), read by PhysicalTool::for_index() (reached from the
/// temperature ISR). The write precedes every read in program order; atomic for visibility.
std::atomic<DetectState> detect_state { DetectState::not_initialised };
std::atomic<HotendType> settled_hotend_type { HotendType::stock };

/// Above this the nozzle holds real heat and must not be pulsed. Erring low is safe: "warm"
/// yields high_temp, and an NTC read through the PT1000 table over-reads rather than runs away.
constexpr int16_t warm_nozzle_c = 65;

/// Average the nozzle channel over ~200 ms, at full 12-bit resolution: the fan phase decides on
/// a few 10-bit counts.
uint16_t sample_nozzle_avg_12bit() {
    constexpr uint32_t sample_count = 64;
    constexpr uint32_t sample_interval_ms = 3; // ~200 ms in total
    uint32_t accumulator = 0;
    for (uint32_t i = 0; i < sample_count; ++i) {
        accumulator += adcDma1.get_channel(AdcChannel::hotend_T);
        freertos::delay(sample_interval_ms);
    }
    return static_cast<uint16_t>(accumulator / sample_count);
}

/// Raw PWM, no RPM regulation: the probe only wants maximum airflow.
void set_print_fan_pwm(uint8_t pwm) {
    Fans::print(PhysicalToolIndex::from_raw(0)).set_pwm(pwm);
}

/// How often the probe repaints the boot progress bar.
constexpr uint32_t progress_step_ms = 100;

/// The fan phase's share of the boot progress bar.
constexpr uint8_t progress_fan_end_pct = 50;

/// Cool the nozzle and see whether the reading runs away upward.
hotend_detect::FanProbeVerdict run_fan_probe() {
    // Long enough for the lagging sensor to show a hot NTC's rise even with a dead print fan.
    constexpr uint32_t fan_phase_ms = 8000;
    constexpr uint8_t fan_probe_pwm = 255;

    // Unguarded: a hang here can only leave the print fan running.
    const uint16_t start_12bit = sample_nozzle_avg_12bit();
    set_print_fan_pwm(fan_probe_pwm);
    for (uint32_t elapsed = 0; elapsed < fan_phase_ms; elapsed += progress_step_ms) {
        buddy::bootstrap_state_set(static_cast<uint8_t>(progress_fan_end_pct * elapsed / fan_phase_ms),
            buddy::BootstrapStage::detecting_hotend);
        freertos::delay(progress_step_ms);
    }
    const uint16_t end_12bit = sample_nozzle_avg_12bit();
    set_print_fan_pwm(0);

    constexpr int32_t rise_counts_10bit = 5;
    const hotend_detect::FanProbeThresholds thresholds {
        .rise_adc_12bit = rise_counts_10bit << AdcDma1::shift_bits,
        .warm_nozzle_adc_10bit = static_cast<uint16_t>(
            MarlinTemptableRawMinMax::compute(TT_NAME(1010), warm_nozzle_c, 400).raw_min / OVERSAMPLENR),
    };

    const int32_t delta = static_cast<int32_t>(end_12bit) - static_cast<int32_t>(start_12bit);
    const auto verdict = hotend_detect::classify_fan_probe(
        { .delta_adc_12bit = delta, .end_adc_10bit = static_cast<uint16_t>(end_12bit >> AdcDma1::shift_bits) },
        thresholds);

    log_info(HotendDetect, "fan: %u -> %u -> %s",
        start_12bit, end_12bit,
        verdict == hotend_detect::FanProbeVerdict::stock_ntc         ? "NTC"
            : verdict == hotend_detect::FanProbeVerdict::warm_pt1000 ? "warm"
                                                                     : "none");
    return verdict;
}

/// Reconcile a classification with the persisted type and publish the result.
///
/// Only a class change touches the store: a merged HotendType cannot recall the sock choice, so
/// a standard hotend comes back with its (mandatory) sock and HT without one.
void settle(hotend_detect::Outputs result, DetectState state) {
    const bool stored_is_ht = (config_store().hotend_type.get(0) == HotendType::high_temp);
    if (result.is_high_temp != stored_is_ht) {
        config_store().set_hotend_type_detected(PhysicalToolIndex::from_raw(0),
            result.is_high_temp ? HotendType::high_temp : HotendType::stock_with_sock);
    }
    settled_hotend_type = config_store().hotend_type.get(0);
    detect_state = state;
}

} // namespace

void hotend_detect::classify_early() {
    const bool stored_is_ht = (config_store().hotend_type.get(0) == HotendType::high_temp);
    early = detect_hotend(stored_is_ht);

    hotend_detect_dialog_pending = early.result.dialog_pending;
}

void hotend_detect::init_skipped() {
    settle(early.result, DetectState::skipped);
}

void hotend_detect::resolve() {
    // Logged here, not at the early reading: USB CDC logging only opens once a host has attached.
    log_early_classification();

    // Mid-print with a hot nozzle and no hardware swap: probing would only delay the resume.
    if (power_panic::state_stored()) {
        settle({ .is_high_temp = (config_store().hotend_type.get(0) == HotendType::high_temp),
                   .dialog_pending = false },
            DetectState::skipped);
        hotend_detect_dialog_pending = false;
        return;
    }

    // dialog_pending is exactly the case a probe can decide.
    if (!early.result.dialog_pending) {
        settle(early.result, DetectState::resolved);
        return;
    }

    auto result = early.result;
    switch (run_fan_probe()) {
    case hotend_detect::FanProbeVerdict::stock_ntc:
        result = { .is_high_temp = false, .dialog_pending = false };
        break;
    case hotend_detect::FanProbeVerdict::warm_pt1000:
        result = { .is_high_temp = true, .dialog_pending = false };
        break;
    case hotend_detect::FanProbeVerdict::inconclusive:
        // A cold PT1000 and a dead sensor look identical here; only a heat pulse separates
        // them. Until that exists, keep the early verdict and let the dialog ask.
        break;
    }

    hotend_detect_dialog_pending = result.dialog_pending;
    settle(result, DetectState::resolved);
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
    // The type is baked into the Hotend config here, so it must already be settled.
    if (detect_state == DetectState::not_initialised) {
        bsod_unreachable();
    }
    const HotendType hotend_type = settled_hotend_type;

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
