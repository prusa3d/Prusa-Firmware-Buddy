#include "heaters_selftest.hpp"

#include "selftest_heater_config.hpp"
#include "selftest_heaters_type.hpp"
#include "algorithm_scale.hpp"
#include <common/marlin_server.hpp>
#include "../../Marlin/src/Marlin.h" // idle()
#include "../../Marlin/src/module/temperature.h" // thermalManager, degHeatbreak
#include <config_store/store_instance.hpp>
#include <feature/safety_timer/safety_timer.hpp>
#include <selftest/selftest_invocation.hpp>
#include <timing.h>
#include <logging/log.hpp>
#include <common/mapi/calibration_preamble.hpp>

#include <algorithm>
#include <cmath>
#include <optional>

#include <option/has_indx.h>
#include <option/has_heaters_selftest_bed_sheet_retry.h>
#include <option/has_heaters_selftest_revise.h>
#include <option/has_hotend_type_support.h>
#if HAS_HOTEND_TYPE_SUPPORT()
    #include <hotend_type.hpp>
#endif
#if HAS_INDX()
    #include <puppies/INDX.hpp>
    #include <tool/hotend/hotend/indx_hotend.hpp>
#endif

LOG_COMPONENT_REF(Selftest);

using namespace selftest;

namespace {

// Full-power on/off "P regulator" — same trick the legacy CSelftestPart_Heater used to run the
// heater flat-out during the measurement window.
constexpr PID_t p_only_pid { .Kp = 1000000, .Ki = 0, .Kd = 0 };

// How long to keep the final pass/fail on screen before the FSM closes (parity with M1978).
constexpr uint32_t show_results_delay_ms = 3000;

#if HAS_INDX()
// Induction-heater coil current window under load [mA] for the nozzle test: the peak sampled
// during the heat-up must land inside, otherwise the test fails like a thermal-protection trip.
// TODO: tune on real hardware; the max is at/below the head's overcurrent limit.
constexpr uint16_t heater_current_min_mA = 1700;
constexpr uint16_t heater_current_max_mA = 2500;
#endif

using marlin_server::fsm_change;
using marlin_server::wait_for_response;

// Tool whose fans we drive. On INDX this is the picked tool (the config temp/pid lambdas also
// operate on the currently-selected tool); elsewhere it is the config's fixed tool.
PhysicalToolIndex fan_tool(const HeaterConfig_t &config) {
#if HAS_INDX()
    return PhysicalToolIndex::currently_selected_opt().value_or(config.tool_nr);
#else
    return config.tool_nr;
#endif
}

// INDX nozzle: heat like a regular print (head-regulated, no full-power PID hack) and let the
// standard protection stack (thermal model, heater watch, thermal runaway) judge the heating —
// the selftest passes on reaching target-temp residency and fails on a protection trip (BFW-9039).
// Everything else keeps the full-power timed-window measurement.
constexpr bool use_protection_verdict([[maybe_unused]] bool is_nozzle) {
#if HAS_INDX()
    return is_nozzle;
#else
    return false;
#endif
}

// progress can only rise (mirrors CSelftestPart_Heater::actualizeProgress)
void bump_progress(SelftestHeater_t &result, float current, float start, float end) {
    if (start >= end) {
        return;
    }
    const auto p = static_cast<uint8_t>(scale_percent_avoid_overflow(current, start, end));
    result.progress = std::max(result.progress, p);
}

/// Drives one heater (nozzle or bed) through cooldown -> preheat -> timed measurement one tick at a
/// time (step()), so the nozzle and bed can be tested in parallel from a single loop, as the legacy
/// mask-based selftest did.
class HeaterRunner {
public:
    HeaterRunner(const HeaterConfig_t &cfg, SelftestHeater_t &result, bool is_nozzle)
        : config(cfg)
        , result(result)
        , is_nozzle(is_nozzle) {
        if (!use_protection_verdict(is_nozzle)) {
            original_pid = config.get_pid();
            config.set_pid(p_only_pid); // switch the regulator to on/off (full power)
            pid_overridden = true;
        }
        config.setTargetTemp(0);
    }

    ~HeaterRunner() { teardown(); }

    bool running() const { return stage != Stage::done; }
    TestResult result_value() const { return res; }
#if HAS_INDX()
    heaters_selftest::NozzleResult fail_result_value() const { return fail_result; }
#endif

    void step() {
#if HAS_TEMP_HEATBREAK_CONTROL
        // Heatbreak temperature correlation: if it leaves its window while the nozzle heats, fail
        // and stop heating right away — the legacy hook did the same via CSelftestPart::Fail().
        if (is_nozzle && stage != Stage::done && heatbreak_out_of_range()) {
            result.heatbreak_error = true;
            finish(TestResult::failed);
            return;
        }
#endif
        if (use_protection_verdict(is_nozzle) && stage != Stage::done) {
            if (verdict_tripped()) {
                finish_on_trip();
                return;
            }
            if (!step_heater_current()) {
                return; // overcurrent just failed the test
            }
        }
        switch (stage) {
        case Stage::setup:
            step_setup();
            break;
        case Stage::cooldown:
            step_cooldown();
            break;
        case Stage::preheat:
            step_preheat();
            break;
        case Stage::heat:
            step_heat();
            break;
        case Stage::done:
            break;
        }
    }

private:
    enum class Stage : uint8_t { setup,
        cooldown,
        preheat,
        heat,
        done };

    void enter_fans() {
        config.print_fan_fnc(fan_tool(config)).enter_selftest_mode();
        config.heatbreak_fan_fnc(fan_tool(config)).enter_selftest_mode();
        fans_taken = true;
    }

    void exit_fans() {
        if (!fans_taken) {
            return;
        }
        fans_taken = false;
        config.print_fan_fnc(fan_tool(config)).exit_selftest_mode();
        config.heatbreak_fan_fnc(fan_tool(config)).exit_selftest_mode();
    }

    void step_setup() {
        const auto t = config.getTemp();
        if (!t.has_value()) {
            return; // wait for a valid reading
        }
        begin_temp = t.value();
        const bool cooldown = begin_temp >= config.start_temp;
        if (is_nozzle && cooldown) {
            // Take control of the fans and blow them at full to cool the hotend down first.
            enter_fans();
            config.print_fan_fnc(fan_tool(config)).selftest_set_pwm(255);
            config.heatbreak_fan_fnc(fan_tool(config)).selftest_set_pwm(255);
        }
        if (cooldown) {
            result.prep_state = SelftestSubtestState_t::running;
            stage = Stage::cooldown;
        } else {
            enter_preheat();
        }
    }

    void step_cooldown() {
        const auto t = config.getTemp();
        if (!t.has_value()) {
            return;
        }
        if (t.value() <= config.undercool_temp) {
            exit_fans(); // hand the fans back to normal control for the heat-up
            enter_preheat();
            return;
        }
        bump_progress(result, t.value(), begin_temp, config.undercool_temp);
    }

    void enter_preheat() {
        result.prep_state = SelftestSubtestState_t::running;
        if (use_protection_verdict(is_nozzle)) {
            verdict_engage();
        }
        config.setTargetTemp(config.target_temp);
        stage = Stage::preheat;
    }

    void step_preheat() {
        const auto t = config.getTemp();
        if (!t.has_value()) {
            return;
        }
        if (t.value() >= config.start_temp) {
            result.prep_state = SelftestSubtestState_t::ok;
            result.heat_state = SelftestSubtestState_t::running;
            result.progress = 0;
            heat_start = ticks_ms();
            stage = Stage::heat;
            return;
        }
        bump_progress(result, t.value(), config.undercool_temp, config.start_temp);
    }

    void step_heat() {
        if (use_protection_verdict(is_nozzle)) {
            step_heat_verdict();
            return;
        }
        const uint32_t elapsed = ticks_ms() - heat_start;
        if (elapsed < config.heat_time_ms) {
            bump_progress(result, static_cast<float>(elapsed), 0.f, static_cast<float>(config.heat_time_ms));
            return;
        }
        // NOTE: config.tool_nr (== 0) is used for the hotend-type offset and the heatbreak reading.
        // Correct for every current non-XL variant (single physical hotend; on INDX both
        // HAS_HOTEND_TYPE_SUPPORT() and HAS_TEMP_HEATBREAK_CONTROL are 0, so this is compiled out).
        // A future non-XL multitool with a heatbreak / hotend-type would need the picked tool here.
        int16_t hw_diff = 0;
#if HAS_HOTEND_TYPE_SUPPORT()
        if (is_nozzle) {
            hw_diff = hotend_type_heater_selftest_offset(config_store().hotend_type.get(config.tool_nr.to_raw()));
        }
#endif
        const auto t = config.getTemp();
        const float temp = t.value_or(NAN);
        const bool in_range = t.has_value()
            && temp >= config.heat_min_temp + hw_diff
            && temp <= config.heat_max_temp + hw_diff;
        if (!in_range) {
            log_error(Selftest, "%s %d out of range (%d - %d)", config.partname, static_cast<int>(temp),
                static_cast<int>(config.heat_min_temp + hw_diff), static_cast<int>(config.heat_max_temp + hw_diff));
        }
        finish(in_range ? TestResult::passed : TestResult::failed);
    }

    // Protection-verdict test helpers (see use_protection_verdict()); no-op stubs elsewhere so
    // the call sites don't need guards — the compile-time-false condition strips them out.
#if HAS_INDX()
    // From here on, protection trips are our verdict instead of a fatal error.
    void verdict_engage() {
        IndxHotend::enter_heater_selftest_mode();
        last_head_reset_count = buddy::puppies::indx.get_reset_counter();
    }

    // Called only after the target is zeroed, so no trip can slip through to fatal_error.
    void verdict_disengage() {
        IndxHotend::exit_heater_selftest_mode();
    }

    bool verdict_tripped() const {
        return IndxHotend::heater_selftest_trip().has_value();
    }

    // Regular regulated heat-up: pass on reaching target-temp residency; fail on the deadline.
    // Protection trips are handled in step().
    void step_heat_verdict() {
        const auto tool = PhysicalToolIndex::currently_selected_opt();
        if (!tool) {
            // Tool got parked under us (toolchanger recovery); nothing to measure anymore.
            selftest_invocation::mark_aborted();
            finish(TestResult::skipped);
            return;
        }
        const auto &hotend = Hotend::for_tool(*tool);
        if (hotend.is_thermally_managed() && hotend.is_nozzle_temp_reached()) {
            // Target reached, but the coil never drew nominal current on the way — bad coil /
            // nozzle coupling. Judged only now: during regulation the duty (and current) drops
            // legitimately, so the peak is only meaningful once the whole heat-up has happened.
            if (peak_heater_current_mA < heater_current_min_mA) {
                log_error(Selftest, "%s coil current peak %u mA under limit (%u)", config.partname,
                    unsigned(peak_heater_current_mA), unsigned(heater_current_min_mA));
                fail_result = std::unexpected(heaters_selftest::CoilUndercurrent { peak_heater_current_mA });
                finish(TestResult::failed);
                return;
            }
            finish(TestResult::passed);
            return;
        }
        // A head reset re-arms the protections and re-pushes the target; give the heat-up a fresh
        // deadline instead of a spurious timeout.
        const uint32_t reset_count = buddy::puppies::indx.get_reset_counter();
        if (reset_count != last_head_reset_count) {
            last_head_reset_count = reset_count;
            heat_start = ticks_ms();
        }
        if (ticks_ms() - heat_start >= config.heat_timeout_ms) {
            log_error(Selftest, "%s timed out heating to target", config.partname);
            fail_result = std::unexpected(heaters_selftest::HeatTimeout {
                static_cast<uint16_t>(std::max(0.f, config.getTemp().value_or(0))) });
            finish(TestResult::failed);
            return;
        }
        const auto t = config.getTemp();
        if (t.has_value()) {
            bump_progress(result, t.value(), config.start_temp, config.target_temp);
        }
    }

    void finish_on_trip() {
        log_error(Selftest, "%s thermal protection tripped (%d)", config.partname,
            static_cast<int>(*IndxHotend::heater_selftest_trip()));
        // A fallen-out nozzle looks like a runaway; that is not a broken heater — abort instead
        // of failing (mirrors the presence recheck the regular path does before raising).
        const auto nozzle_present = buddy::puppies::indx.get_nozzle_present();
        if (nozzle_present.has_value() && !*nozzle_present) {
            selftest_invocation::mark_aborted();
            finish(TestResult::skipped);
            return;
        }
        fail_result = std::unexpected(heaters_selftest::ThermalProtectionTripped {
            static_cast<uint16_t>(*IndxHotend::heater_selftest_trip()) });
        finish(TestResult::failed);
    }

    // Track the coil-current peak while the heater drives (the heat-up ramp runs at full duty).
    // Overcurrent fails right away; undercurrent is judged at the pass point in step_heat_verdict().
    // @returns false when the check just concluded the test
    bool step_heater_current() {
        if (stage != Stage::preheat && stage != Stage::heat) {
            return true;
        }
        peak_heater_current_mA = std::max(peak_heater_current_mA, buddy::puppies::indx.get_heater_current_mA());
        if (peak_heater_current_mA > heater_current_max_mA) {
            log_error(Selftest, "%s coil current %u mA over limit (%u)", config.partname,
                unsigned(peak_heater_current_mA), unsigned(heater_current_max_mA));
            fail_result = std::unexpected(heaters_selftest::CoilOvercurrent { peak_heater_current_mA });
            finish(TestResult::failed);
            return false;
        }
        return true;
    }

    /// Last seen indx head reset counter, for restarting the heat deadline across head resets
    uint32_t last_head_reset_count = 0;

    /// Highest coil current sampled during the heat-up [mA]
    uint16_t peak_heater_current_mA = 0;

    /// Why the nozzle test failed (+ the measured value), for the nozzle_failed_dialog phase
    heaters_selftest::NozzleResult fail_result;
#else
    void verdict_engage() {}
    void verdict_disengage() {}
    bool verdict_tripped() const { return false; }
    void step_heat_verdict() {}
    void finish_on_trip() {}
    bool step_heater_current() { return true; }
#endif

    // Conclude the test: record the result and immediately stop heating / restore the regulator.
    void finish(TestResult r) {
        res = r;
        if (r == TestResult::passed) {
            result.Pass();
        } else {
            result.Fail();
        }
        teardown();
        stage = Stage::done;
    }

    // Stop heating this element, restore its PID regulator and release its fans. Idempotent.
    void teardown() {
        if (torn_down) {
            return;
        }
        torn_down = true;
        config.setTargetTemp(0);
        if (pid_overridden) {
            config.set_pid(original_pid);
        }
        if (use_protection_verdict(is_nozzle)) {
            verdict_disengage();
        }
        exit_fans(); // safety net: release the fans if we were torn down mid-cooldown
    }

    bool heatbreak_out_of_range() const {
#if HAS_TEMP_HEATBREAK_CONTROL
        // config.tool_nr — see the note in step_heat().
        const float t = thermalManager.degHeatbreak(config.tool_nr);
        return (t > config.heatbreak_max_temp) || (t < config.heatbreak_min_temp);
#else
        return false;
#endif
    }

    HeaterConfig_t config;
    SelftestHeater_t &result;
    bool is_nozzle;
    Stage stage = Stage::setup;
    PID_t original_pid {}; // only valid when pid_overridden
    float begin_temp = 0;
    uint32_t heat_start = 0;
    TestResult res = TestResult::failed;
    bool torn_down = false;
    bool fans_taken = false;
    bool pid_overridden = false;
};

class Wizard {
public:
    void run();

private:
#if HAS_INDX()
    marlin_server::FSM_Holder holder { PhasesHeatersSelftest::picking_tool };
#else
    marlin_server::FSM_Holder holder { PhasesHeatersSelftest::heating };
#endif

    HeatersSelftestData data;

    void send() { marlin_server::fsm_change_extended(PhasesHeatersSelftest::heating, data); }

    // Keep the heating screen (final data) up for the given time so the user can read the result.
    void dwell(uint32_t ms) {
        const uint32_t start = ticks_ms();
        while (ticks_ms() - start < ms) {
            send();
            idle(true);
        }
    }

    bool preamble();
};

bool Wizard::preamble() {
    const mapi::CalibrationPreamble preamble {
#if HAS_INDX()
        .tool_policy = mapi::CalibrationPreamble::ToolPolicy::ensure_picked,
#elif PRINTER_IS_PRUSA_XL()
    #error TODO ensure_parked + reporting
#endif
        .on_step = [](mapi::CalibrationPreamble::Step) {
#if HAS_INDX()
            fsm_change(PhasesHeatersSelftest::picking_tool);
#elif PRINTER_IS_PRUSA_XL()
    #error TODO ensure_parked + reporting
#endif
        },
    };
    return preamble.run();
}

void Wizard::run() {
    if (!preamble()) {
        selftest_invocation::mark_aborted();
        return;
    }

    const HeaterConfig_t noz_config = nozzle_heater_config();
    const HeaterConfig_t bed_config = bed_heater_config();

    bool rerun;
    do {
        rerun = false;

        // Pre-mark both heaters as failed so a mid-test reset / disconnected cable stays failed.
        {
            auto sr = config_store().selftest_result.get();
            sr.set_nozzle_heater(0, TestResult::failed);
            sr.set_bed_heater(TestResult::failed);
            config_store().selftest_result.set(sr);
        }
        data = HeatersSelftestData {};

        // The nozzle heater test is skipped if the hotend/heatbreak fan check did not pass — heating
        // the nozzle without a working hotend fan is unsafe. A skipped nozzle is recorded as skipped
        // (not failed), matching the legacy behaviour (an aborted part reports TestResult::skipped).
        const bool skip_nozzle = config_store().selftest_result.get().get_heatbreak_fan(noz_config.tool_nr) == TestResult::failed;
        TestResult noz_result = skip_nozzle ? TestResult::skipped : TestResult::failed;
        TestResult bed_result = TestResult::failed;
#if HAS_INDX()
        heaters_selftest::NozzleResult noz_fail_result;
#endif
        if (skip_nozzle) {
            fsm_change(PhasesHeatersSelftest::hotend_fan_failed_dialog);
            wait_for_response(PhasesHeatersSelftest::hotend_fan_failed_dialog); // Ok
            data.noz.Fail();
        }

        {
            // Nozzle and bed run in parallel — both powered at once — matching the legacy behaviour.
            // The bed heater does not depend on the hotend fan, so it is tested even when the nozzle
            // test was skipped (the legacy code suppressed the bed test there only to avoid a messy
            // parallel screen, which the dedicated screen no longer has).
            // The safety timer would zero the heater targets mid-measurement (BFW-7813).
            buddy::SafetyTimerBlocker safety_timer_blocker;
            HeaterRunner bed(bed_config, data.bed, /*is_nozzle=*/false);
            std::optional<HeaterRunner> noz;
            if (!skip_nozzle) {
                noz.emplace(noz_config, data.noz, /*is_nozzle=*/true);
            }
            send(); // switch to the heating screen (leaves the picking / fan-failed dialog phase)
            while ((noz && noz->running()) || bed.running()) {
                if (noz) {
                    noz->step();
                }
                bed.step();
                send();
                idle(true);
            }
            if (noz) {
                noz_result = noz->result_value();
#if HAS_INDX()
                noz_fail_result = noz->fail_result_value();
#endif
            }
            bed_result = bed.result_value();
        } // runners torn down here (heaters off, PID/fans restored) before any revise prompts

        {
            auto sr = config_store().selftest_result.get();
            sr.set_nozzle_heater(0, noz_result);
            sr.set_bed_heater(bed_result);
            config_store().selftest_result.set(sr);
        }

        // Let the user actually see the final pass/fail before the screen closes / a prompt appears.
        dwell(show_results_delay_ms);

#if HAS_INDX()
        // Nozzle failed: tell the user why — the fail icon alone gives no clue what to do next.
        if (noz_result == TestResult::failed) {
            fsm_change(PhasesHeatersSelftest::nozzle_failed_dialog,
                fsm::serialize_data(noz_fail_result.error_or(std::monostate {})));
            wait_for_response(PhasesHeatersSelftest::nozzle_failed_dialog); // Ok
        }
#endif

#if HAS_HEATERS_SELFTEST_BED_SHEET_RETRY()
        // Bed failed: offer to refit the steel sheet and retry.
        if (bed_result == TestResult::failed) {
            fsm_change(PhasesHeatersSelftest::ask_bed_sheet_after_fail);
            if (wait_for_response(PhasesHeatersSelftest::ask_bed_sheet_after_fail) == Response::Retry) {
                rerun = true;
                continue;
            }
        }
#endif
#if HAS_HEATERS_SELFTEST_REVISE()
        // Nozzle failed: offer to revise the printer setup (wrong nozzle/hotend config) and retry.
        if (noz_result == TestResult::failed) {
            fsm_change(PhasesHeatersSelftest::revise_ask_revise);
            if (wait_for_response(PhasesHeatersSelftest::revise_ask_revise) == Response::Adjust) {
                // The revise_revise frame opens ScreenPrinterSetup, which sends Response::Done.
                fsm_change(PhasesHeatersSelftest::revise_revise);
                wait_for_response(PhasesHeatersSelftest::revise_revise); // Done
                fsm_change(PhasesHeatersSelftest::revise_ask_retry);
                if (wait_for_response(PhasesHeatersSelftest::revise_ask_retry) == Response::Yes) {
                    rerun = true;
                    continue;
                }
            }
        }
#endif
    } while (rerun);
}

} // namespace

void heaters_selftest::run() {
    Wizard wizard;
    wizard.run();
}
