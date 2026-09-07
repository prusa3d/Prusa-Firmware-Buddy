#include <marlin_stubs/PrusaGcodeSuite.hpp>

#include <selftest_fans_config.hpp>
#include <selftest_fans.hpp>
#include <fanctl.hpp>
#include <client_response.hpp>
#include <common/fsm_base_types.hpp>
#include <common/marlin_server.hpp>
#include <logging/log.hpp>
#include <common/conversions.hpp>
#include <span>
#include <array>
#include <bitset>
#include <config_store/store_instance.hpp>
#include <printers.h>
#include <option/has_switched_fan_test.h>

#include <option/has_indx.h>
#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include <Marlin/src/module/prusa/toolchanger.h>
#endif

#include <option/xl_enclosure_support.h>
#if XL_ENCLOSURE_SUPPORT()
    #include <xl_enclosure.hpp>
#endif

#include <option/has_chamber_api.h>
#if HAS_CHAMBER_API()
    #include <feature/chamber/chamber.hpp>
#endif

#include <option/has_chamber_filtration_api.h>
#if HAS_CHAMBER_FILTRATION_API()
    #include <feature/chamber_filtration/chamber_filtration.hpp>
#endif

#include <option/xbuddy_extension_variant.h>
#if XBUDDY_EXTENSION_VARIANT_IS_STANDARD()
    #include <feature/xbuddy_extension/xbuddy_extension.hpp>
    #include <puppies/xbuddy_extension.hpp> // For FAN_CNT
#endif
#include <logging/log.hpp>

#include <option/has_bed_fan.h>
#if HAS_BED_FAN()
    #include <feature/bed_fan/controller.hpp>
    #include <feature/bed_fan/selftest_result.hpp>
#endif

#include <option/has_psu_fan.h>
#include <bsod/bsod.h>
#if HAS_PSU_FAN()
    #include <puppies/ac_controller.hpp>
#endif

#include <common/printer_model.hpp>
#include <option/has_cpu_fan.h>
#include <option/has_xl_can.h>

LOG_COMPONENT_REF(Selftest);

using namespace fan_selftest;
using marlin_server::wait_for_response;

namespace {

constexpr uint32_t measure_rpm_delay = 5000;
constexpr uint32_t wait_rpm_100_percent_delay = 6000;
constexpr uint32_t measure_rpm_100_percent_delay = measure_rpm_delay;
constexpr uint32_t wait_rpm_0_percent_delay = 6000;
constexpr uint32_t wait_rpm_40_percent_delay = 3000;
constexpr uint32_t measure_rpm_40_percent_delay = measure_rpm_delay;
constexpr uint32_t show_results_delay = 4000;

constexpr uint32_t full_time_estimate
    = wait_rpm_100_percent_delay
    + measure_rpm_100_percent_delay
    + wait_rpm_0_percent_delay
    + wait_rpm_40_percent_delay
    + measure_rpm_40_percent_delay
    + show_results_delay;

constexpr uint32_t test_100_time_estimate
    = wait_rpm_100_percent_delay
    + measure_rpm_100_percent_delay;

constexpr std::uint8_t percentage_to_pwm(std::uint8_t target_percentage) {
    return val_mapping(true, target_percentage, 100, 255);
}

// Start at 100% and wait longer to allow spreading stuck lubricant after first assembly.
// Then set PWM to 0% and wait for quite a long time to ensure fan stopped.
// (We can't measure RPM without at least some PWM)
// Then continue with 40% PWM to test if 40% is enough to start spinning.
constexpr uint8_t pwm_100_percent = percentage_to_pwm(100);
constexpr uint8_t pwm_40_percent = percentage_to_pwm(40);

/// Runtime print-fan RPM range selector. On XL, returns XL or XLS range
/// based on extended printer type; elsewhere returns print_fan_range.
inline const FanRPMRange &current_print_fan_range() {
#if PRINTER_IS_PRUSA_XL()
    return (PrinterModelInfo::current().model == PrinterModel::xls) ? print_fan_range_xls : print_fan_range_xl;
#else
    return print_fan_range;
#endif
}

class FanSelfTestWizard {
public:
    FanSelfTestWizard(PhasesFansSelftest phase, const std::span<FanHandler *> &fans, [[maybe_unused]] const std::span<std::pair<FanHandler *, FanHandler *>> &tool_pairs)
        : phase(phase)
        , phase_change_time(ticks_ms())
        , fans(fans)
#if HAS_SWITCHED_FAN_TEST()
        , switched_fan_pairs(tool_pairs)
#endif
    {
    }

    void change_phase(PhasesFansSelftest new_phase) {
        phase = new_phase;
        marlin_server::fsm_change(phase, { progress_percentage });
        phase_change_time = ticks_ms();
    }

    void update_progress() {
        float new_progress = 0;
        // Manual check pauses the progress, so we cannot calculate just (now - start)
        switch (phase) {
#if PRINTER_IS_PRUSA_MK3_5() && HAS_SWITCHED_FAN_TEST()
        case PhasesFansSelftest::manual_check:
            return;
#endif
        case PhasesFansSelftest::test_100_percent:
            new_progress = 0;
            break;
        case PhasesFansSelftest::test_40_percent:
            new_progress = test_100_time_estimate;
            break;
        case PhasesFansSelftest::results:
            new_progress = (full_time_estimate - show_results_delay);
            break;
        }
        new_progress += (ticks_ms() - phase_change_time);
        new_progress /= full_time_estimate;
        new_progress *= 100;
        uint8_t new_progress_percentage = static_cast<uint8_t>(new_progress);

        if (new_progress_percentage != progress_percentage) {
            progress_percentage = new_progress_percentage;
            marlin_server::fsm_change(phase, { progress_percentage });
        }
    }

    void run_fan_selftest() {
        marlin_server::FSM_Holder holder { phase, { progress_percentage } };
        reset_fan_result();
        set_up_measurement(pwm_100_percent);
        wait(wait_rpm_100_percent_delay);
        measure(measure_rpm_100_percent_delay);
#if PRINTER_IS_PRUSA_MK3_5()
        check_alt_fans(); // Might overwrite fan ranges
#endif
        evaluate();
#if HAS_SWITCHED_FAN_TEST()
        if (check_fan_switched()) {
            // FAILED Aborting fan tests
            finish_and_show_results();
            return;
        }
#endif
#if PRINTER_IS_PRUSA_MK3_5() && HAS_SWITCHED_FAN_TEST()
        manual_check_init();
        wait(wait_rpm_0_percent_delay);
        change_phase(PhasesFansSelftest::manual_check); // Set up fsm for manual check dialog
        if (manual_check_ask()) {
            finish_and_show_results();
            return;
        }
#endif
#if !HAS_INDX()
        change_phase(PhasesFansSelftest::test_40_percent);
        set_low_speed_fan_range();
        set_up_measurement(0);
        wait(wait_rpm_0_percent_delay);
        set_up_measurement(pwm_40_percent);
        wait(wait_rpm_40_percent_delay);
        measure(measure_rpm_40_percent_delay);
        evaluate();
#endif
        finish_and_show_results();
    }

    void finish_and_show_results() {
        save_selftest_results();
        set_up_measurement(0);
        change_phase(PhasesFansSelftest::results);
        wait(show_results_delay);
    }

private:
    void set_up_measurement(const uint8_t pwm) {
        for (auto *fan : fans) {
            fan->set_pwm(pwm);
            fan->reset_samples();
        }
    }

    void wait(const uint32_t delay) {
        uint32_t timestamp = ticks_ms();
        while (ticks_ms() - timestamp <= delay) {
            update_progress();
            idle(true);
        }
    }

    void measure(const uint32_t record_period) {
        uint32_t timestamp = ticks_ms();
        while (ticks_ms() - timestamp <= record_period) {
            for (auto *fan : fans) {
                fan->record_sample();
            }
            update_progress();
            idle(true);
        }
    }

    void evaluate() {
        for (auto *fan : fans) {
            fan->evaluate();
        }
    }

#if PRINTER_IS_PRUSA_MK3_5()
    void check_alt_fans() {
        uint16_t print_fan_rpm = fans[0]->calculate_avg_rpm();
        uint16_t heatbreak_fan_rpm = fans[1]->calculate_avg_rpm();

        if (print_fan_rpm > 6000 || heatbreak_fan_rpm > 6000) {
            // this rpm is unreachable by noctua therefore the fans are a lot faster and pwm fix is needed to make printer quiet
            // check both fans because they could be switched.
            config_store().has_alt_fans.set(true);

            // Create config specifically for alt fans presence of which cannot be done compile-time.
            fans[0]->set_range({ .rpm_min = 3000, .rpm_max = 4500 });
            fans[1]->set_range({ .rpm_min = 7000, .rpm_max = 10000 });
        } else {
            config_store().has_alt_fans.set(false);
        }
    }
#endif // PRINTER_IS_PRUSA_MK3_5

#if HAS_SWITCHED_FAN_TEST()
    bool check_fan_switched() {
        bool failed = false;
        // Samples have to be recorded and average RPM have to be already calculated
        for (auto fan_pair : switched_fan_pairs) {
            if (fan_pair.first->is_failed() && fan_pair.second->is_failed()) {
                // try if the rpms fit into the ranges when switched, if yes, fail the
                // "fans switched" test and pass the RPM tests
                if (fan_pair.second->is_rpm_within_bounds(fan_pair.first->get_avg_rpm()) && fan_pair.first->is_rpm_within_bounds(fan_pair.second->get_avg_rpm())) {
                    log_error(Selftest, "Fans test: print and hotend fans appear to be switched (the RPM of each fits into the range of the other)");
                    // Since fans switched isn't the last check, it cannot tell whether the fans are ok or not. All that is certain at this point is that they are switched. They still can fail on 20 % test.

                    fans_switched.set(fan_pair.first->get_desc_num());
                    fan_pair.first->set_failed(false);
                    fan_pair.second->set_failed(false);
                    failed = true;
                }
            }
        }
        return failed;
    }
#endif

#if PRINTER_IS_PRUSA_MK3_5() && HAS_SWITCHED_FAN_TEST()
    void manual_check_init() {
        // stop print_fan since heatbreak_fan is the critical one
        fans[0]->set_pwm(0);
    }

    bool manual_check_ask() {
        switch (wait_for_response(PhasesFansSelftest::manual_check)) {
        case Response::No:
            fans_switched.set(0);
            // Fans are not connected correctly - Fail test
            return true;
        case Response::Yes:
            fans_switched.reset(0);
            break;
        default:
            bsod("manual_check_ask");
        }

        return false;
    }
#endif // PRINTER_IS_PRUSA_MK3_5() && HAS_SWITCHED_FAN_TEST()

    void reset_fan_result() {
        SelftestResult result = config_store().selftest_result.get();
        for (auto tool : PhysicalToolIndex::all()) {
            result.set_print_fan(tool, TestResult::unknown);
            result.set_heatbreak_fan(tool, TestResult::unknown);
            result.set_fans_switched(tool, TestResult::unknown);
        }
#if HAS_INDX()
        result.set_dock_fan(TestResult::unknown);
#endif
        config_store().selftest_result.set(result);

#if HAS_CHAMBER_API()
        switch (buddy::chamber().backend()) {

    #if XL_ENCLOSURE_SUPPORT()
        case buddy::Chamber::Backend::xl_enclosure:
            config_store().xl_enclosure_fan_selftest_result.set(TestResult::unknown);
            break;
    #endif /* XL_ENCLOSURE_SUPPORT() */

    #if XBUDDY_EXTENSION_VARIANT_IS_STANDARD()
        case buddy::Chamber::Backend::xbuddy_extension:
            config_store().xbe_fan_test_results.set({});
            break;
    #endif

        case buddy::Chamber::Backend::none:
            break;
        }
#endif /* HAS_CHAMBER_API() */
#if HAS_BED_FAN()
        config_store().bed_fan_selftest_result.set(bed_fan::SelftestResult {});
#endif
#if HAS_PSU_FAN()
        config_store().psu_fan_selftest_result.set(TestResult::unknown);
#endif
#if HAS_CPU_FAN()
        config_store().cpu_fan_selftest_result.set(TestResult::unknown);
#endif
#if HAS_XL_CAN()
        config_store().bed_mcu_fan_selftest_result.set(TestResult::unknown);
#endif
    }

    void set_low_speed_fan_range() {
        for (auto *fan : fans) {
            fan->set_low_range();
        }
    }

    bool save_selftest_results() {
        bool failed = false;
        SelftestResult result = config_store().selftest_result.get();
        for (auto *fan : fans) {
            switch (fan->get_type()) {
            case FanType::print:
                result.set_print_fan(fan->get_desc_num(), fan->test_result());
#if HAS_SWITCHED_FAN_TEST()
                // Also save fanSwitched
                result.set_fans_switched(fan->get_desc_num(), fans_switched[fan->get_desc_num()] ? TestResult::failed : TestResult::passed);
#endif
                break;
            case FanType::heatbreak:
                result.set_heatbreak_fan(fan->get_desc_num(), fan->test_result());
                break;
#if HAS_INDX()
            case FanType::dock:
                result.set_dock_fan(fan->test_result());
                break;
#endif
#if XL_ENCLOSURE_SUPPORT()
            case FanType::xl_enclosure:
                config_store().xl_enclosure_fan_selftest_result.set(fan->test_result());
                break;
#endif
#if XBUDDY_EXTENSION_VARIANT_IS_STANDARD()
            case FanType::xbe_chamber: {
                debug_assert(fan->get_desc_num() < buddy::puppies::XBuddyExtension::FAN_CNT);
                auto res = config_store().xbe_fan_test_results.get();
                res.fans[fan->get_desc_num()] = fan->test_result();
                config_store().xbe_fan_test_results.set(res);
                break;
            }
#endif
#if HAS_BED_FAN()
            case FanType::bed: {
                static_assert(bed_fan::SelftestResult::fan_count == 2, "Adjust the fan result structure");
                debug_assert(fan->get_desc_num() < bed_fan::SelftestResult::fan_count);
                auto res = config_store().bed_fan_selftest_result.get();
                res.fans[fan->get_desc_num()] = fan->test_result();
                config_store().bed_fan_selftest_result.set(res);
                break;
            }
#endif
#if HAS_PSU_FAN()
            case FanType::psu:
                config_store().psu_fan_selftest_result.set(fan->test_result());
                break;
#endif
#if HAS_CPU_FAN()
            case FanType::cpu:
                config_store().cpu_fan_selftest_result.set(fan->test_result());
                break;
#endif
#if HAS_XL_CAN()
            case FanType::bed_mcu:
                config_store().bed_mcu_fan_selftest_result.set(fan->test_result());
                break;
#endif
            case FanType::_count:
                debug_assert(false);
            }

            if (fan->is_failed()) {
                log_info(Selftest, "Test of %s fan %u failed", fan_type_names[fan->get_type()], fan->get_desc_num());
                failed = true;
            }
        }
        config_store().selftest_result.set(result);
        return !failed;
    }

    PhasesFansSelftest phase;
    uint32_t phase_change_time;
    std::span<FanHandler *> fans;
#if HAS_SWITCHED_FAN_TEST()
    std::span<std::pair<FanHandler *, FanHandler *>> switched_fan_pairs;
    std::bitset<PhysicalToolIndex::count> fans_switched {};
#endif
    uint8_t progress_percentage { 0 };
};

} // namespace

namespace PrusaGcodeSuite {

/** \addtogroup G-Codes
 * @{
 */

/**
 *### M1978: Fan Selftest Dialog
 *
 * Internal GCode
 *
 *#### Usage
 *
 *    M1978
 *
 */
void M1978() {

#if HAS_INDX()
    // INDX has a single shared print fan and heatbreak fan on the body,
    // not per-nozzle fans. Create one handler for each.
    CommonFanHandler indx_print_fan(FanType::print, 0, print_fan_range,
        &Fans::print(PhysicalToolIndex::from_raw(0)));
    CommonFanHandler indx_heatbreak_fan(FanType::heatbreak, 0, heatbreak_fan_range,
        &Fans::heat_break(PhysicalToolIndex::from_raw(0)));
    // Auxiliary dock fan on the xBuddy print-fan pin (same hardware as the C1 print fan).
    CommonFanHandler indx_dock_fan(FanType::dock, 0, dock_fan_range,
        &Fans::dock_fan());

    std::array<FanHandler *, 3 + 5 /* reserve for chamber/bed/psu fans */> fan_container;
    std::array<std::pair<FanHandler *, FanHandler *>, 1> tool_fan_pairs;

    size_t container_index = 0;
    uint8_t pairs = 0;
    fan_container[container_index++] = &indx_print_fan;
    fan_container[container_index++] = &indx_heatbreak_fan;
    fan_container[container_index++] = &indx_dock_fan;
    tool_fan_pairs[pairs++] = std::make_pair<FanHandler *, FanHandler *>(&indx_print_fan, &indx_heatbreak_fan);
#else
    auto print_fans = [&]<size_t... ix>(std::index_sequence<ix...>) {
        return StrongIndexArray<CommonFanHandler, PhysicalToolIndex::count, PhysicalToolIndex, PhysicalToolIndex::to_raw_static> {
            CommonFanHandler(FanType::print, ix, current_print_fan_range(), &Fans::print(PhysicalToolIndex::from_raw(ix)), print_low_fan_range)...
        };
    }(std::make_index_sequence<PhysicalToolIndex::count>());

    auto heatbreak_fans = [&]<size_t... ix>(std::index_sequence<ix...>) {
        return StrongIndexArray<CommonFanHandler, PhysicalToolIndex::count, PhysicalToolIndex, PhysicalToolIndex::to_raw_static> {
            CommonFanHandler(FanType::heatbreak, ix, heatbreak_fan_range, &Fans::heat_break(PhysicalToolIndex::from_raw(ix)))...
        };
    }(std::make_index_sequence<PhysicalToolIndex::count>());

    std::array<FanHandler *, HOTENDS * 2 + 5 /* enclosure/chamber fans (1-2) + AC fans (2) + reserve */> fan_container;
    std::array<std::pair<FanHandler *, FanHandler *>, PhysicalToolIndex::count> tool_fan_pairs;

    size_t container_index = 0;
    uint8_t pairs = 0;
    for (auto tool : PhysicalToolIndex::all().skip_all_disabled()) {
        fan_container[container_index++] = &print_fans[tool];
        fan_container[container_index++] = &heatbreak_fans[tool];
        tool_fan_pairs[pairs++] = std::make_pair(&print_fans[tool], &heatbreak_fans[tool]);
    }
#endif

#if XL_ENCLOSURE_SUPPORT()
    CommonFanHandler xl_enclosure_fan(FanType::xl_enclosure, 0, benevolent_fan_range, &Fans::enclosure());
#endif
#if HAS_CPU_FAN()
    // CPU cooling fan exists physically only on the XLS sandwich board; on plain XL the
    // pin is unconnected and the controller never spins it. Construct unconditionally
    // (HAS_CPU_FAN is master-board-level) but include in the test only on XLS.
    CommonFanHandler cpu_fan(FanType::cpu, 0, cpu_fan_range, &Fans::cpu());
    if (has_board_fans()) {
        fan_container[container_index++] = &cpu_fan;
    }
#endif
#if HAS_XL_CAN()
    // Modular Bed cooling fan lives on the XL-CAN bridge, present only on XLS.
    // Construct unconditionally (HAS_XL_CAN is master-board-level) but include in
    // the test only on XLS. A bridge that failed to enumerate (MbResetCheck::
    // uncontrolled - boot continues with only the XlCanWiringSuspected warning)
    // then fails the test on zero RPM samples rather than dropping the fan from it.
    BedMcuFanHandler bed_mcu_fan(bed_mcu_fan_range, benevolent_fan_range);
    if (has_board_fans()) {
        fan_container[container_index++] = &bed_mcu_fan;
    }
#endif
#if XBUDDY_EXTENSION_VARIANT_IS_STANDARD()
    std::array xbe_fans {
        XBEFanHandler(FanType::xbe_chamber, 0, chamber_fan_range),
        XBEFanHandler(FanType::xbe_chamber, 1, chamber_fan_range),
        XBEFanHandler(FanType::xbe_chamber, 2, filtration_fan_range),
    };
    static_assert(buddy::puppies::XBuddyExtension::FAN_CNT == 3);
#endif

#if HAS_CHAMBER_API()
    switch (buddy::chamber().backend()) {

    #if XL_ENCLOSURE_SUPPORT()
    case buddy::Chamber::Backend::xl_enclosure: {
        fan_container[container_index++] = &xl_enclosure_fan;
        break;
    }
    #endif /* XL_ENCLOSURE_SUPPORT() */

    #if XBUDDY_EXTENSION_VARIANT_IS_STANDARD()
        static_assert(HAS_CHAMBER_FILTRATION_API());
    case buddy::Chamber::Backend::xbuddy_extension:
        if (buddy::xbuddy_extension().using_filtration_fan_instead_of_cooling_fans()) {
            fan_container[container_index++] = &xbe_fans[2];
        } else {
            fan_container[container_index++] = &xbe_fans[0];
            fan_container[container_index++] = &xbe_fans[1];
        }
        break;
    #endif

    case buddy::Chamber::Backend::none:
        break;
    }
#endif /* HAS_CHAMBER_API() */

#if HAS_BED_FAN()
    static constexpr auto HIGH_BED_FAN_RANGE = fan_selftest::FanRPMRange { .rpm_min = 5400, .rpm_max = 6700 };
    static constexpr auto LOW_BED_FAN_RANGE = fan_selftest::FanRPMRange { .rpm_min = 2100, .rpm_max = 2600 };
    BedFanHandler bed_fan_0 { 0, HIGH_BED_FAN_RANGE, LOW_BED_FAN_RANGE };
    fan_container[container_index++] = &bed_fan_0;
    BedFanHandler bed_fan_1 { 1, HIGH_BED_FAN_RANGE, LOW_BED_FAN_RANGE };
    fan_container[container_index++] = &bed_fan_1;
#endif
#if HAS_PSU_FAN()
    static constexpr auto HIGH_PSU_FAN_RANGE = fan_selftest::FanRPMRange { .rpm_min = 4500, .rpm_max = 5500 };
    static constexpr auto LOW_PSU_FAN_RANGE = fan_selftest::FanRPMRange { .rpm_min = 1800, .rpm_max = 2500 };
    PSUFanHandler psu_fan { HIGH_PSU_FAN_RANGE, LOW_PSU_FAN_RANGE };
    fan_container[container_index++] = &psu_fan;
#endif

    debug_assert(container_index && container_index <= fan_container.size());

    auto wizard = FanSelfTestWizard(
        PhasesFansSelftest::test_100_percent,
        std::span(fan_container.data(), container_index),
        std::span(tool_fan_pairs.data(), pairs));
    wizard.run_fan_selftest();
}

/** @}*/

} // namespace PrusaGcodeSuite
