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

#include <option/has_xbuddy_extension.h>
#if HAS_XBUDDY_EXTENSION()
    #include <feature/xbuddy_extension/xbuddy_extension.hpp>
    #include <puppies/xbuddy_extension.hpp> // For FAN_CNT
#endif
#include <logging/log.hpp>

LOG_COMPONENT_REF(Selftest);

using namespace fan_selftest;
using namespace buddy;
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
        float new_progress;
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

        if (new_progress != progress_percentage) {
            progress_percentage = new_progress;
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
        change_phase(PhasesFansSelftest::test_40_percent);
        set_benevolent_fan_range(); // Test only if fan is spinning
        set_up_measurement(0);
        wait(wait_rpm_0_percent_delay);
        set_up_measurement(pwm_40_percent);
        wait(wait_rpm_40_percent_delay);
        measure(measure_rpm_40_percent_delay);
        evaluate();
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
#if PRINTER_IS_PRUSA_iX()
            // On iX we do not evaluate HB fan to make it always pass
            if (fan->get_type() == FanType::heatbreak) {
                continue;
            }
#endif
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
        for (auto &tool : result.tools) {
            tool.printFan = TestResult_Unknown;
            tool.heatBreakFan = TestResult_Unknown;
            tool.fansSwitched = TestResult_Unknown;
        }
        config_store().selftest_result.set(result);

#if HAS_CHAMBER_API()
        switch (chamber().backend()) {

    #if XL_ENCLOSURE_SUPPORT()
        case Chamber::Backend::xl_enclosure:
            config_store().xl_enclosure_fan_selftest_result.set(TestResult_Unknown);
            break;
    #endif /* XL_ENCLOSURE_SUPPORT() */

    #if HAS_XBUDDY_EXTENSION()
        case Chamber::Backend::xbuddy_extension:
            config_store().xbe_fan_test_results.set({});
            break;
    #endif

        case Chamber::Backend::none:
            break;
        }
#endif /* HAS_CHAMBER_API() */
    }

    void set_benevolent_fan_range() {
        for (auto *fan : fans) {
            fan->set_range(benevolent_fan_range);
        }
    }

    bool save_selftest_results() {
        bool failed = false;
        SelftestResult result = config_store().selftest_result.get();
        for (auto *fan : fans) {
            switch (fan->get_type()) {
            case FanType::print:
                result.tools[fan->get_desc_num()].printFan = fan->test_result();
#if HAS_SWITCHED_FAN_TEST()
                // Also save fanSwitched
                result.tools[fan->get_desc_num()].fansSwitched = fans_switched[fan->get_desc_num()] ? TestResult_Failed : TestResult_Passed;
#endif
                break;
            case FanType::heatbreak:
                result.tools[fan->get_desc_num()].heatBreakFan = fan->test_result();
                break;
#if XL_ENCLOSURE_SUPPORT()
            case FanType::xl_enclosure:
                config_store().xl_enclosure_fan_selftest_result.set(fan->test_result());
                break;
#endif
#if HAS_XBUDDY_EXTENSION()
            case FanType::xbe_chamber: {
                assert(fan->get_desc_num() < puppies::XBuddyExtension::FAN_CNT);
                auto res = config_store().xbe_fan_test_results.get();
                res.fans[fan->get_desc_num()] = fan->test_result();
                config_store().xbe_fan_test_results.set(res);
                break;
            }
#endif
            case FanType::_count:
                assert(false);
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
    std::bitset<config_store_ns::max_tool_count> fans_switched {};
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

    auto print_fans = [&]<size_t... ix>(std::index_sequence<ix...>) {
        return std::array {
            CommonFanHandler(FanType::print, ix, print_fan_range, &Fans::print(ix))...
        };
    }(std::make_index_sequence<HOTENDS>());

    auto heatbreak_fans = [&]<size_t... ix>(std::index_sequence<ix...>) {
        return std::array {
            CommonFanHandler(FanType::heatbreak, ix, heatbreak_fan_range, &Fans::heat_break(ix))...
        };
    }(std::make_index_sequence<HOTENDS>());

    std::array<FanHandler *, HOTENDS * 2 + 5 /* enclosure/chamber fans with reserve */> fan_container;
    std::array<std::pair<FanHandler *, FanHandler *>, HOTENDS> tool_fan_pairs;

    size_t container_index = 0;
    uint8_t pairs = 0;
    for (uint8_t i = 0; i < HOTENDS; i++) {
#if HAS_TOOLCHANGER()
        if (!prusa_toolchanger.is_tool_enabled(i)) {
            continue;
        }
#endif
        fan_container[container_index++] = &print_fans[i];
        fan_container[container_index++] = &heatbreak_fans[i];
        tool_fan_pairs[pairs++] = std::make_pair(&print_fans[i], &heatbreak_fans[i]);
    }

#if XL_ENCLOSURE_SUPPORT()
    CommonFanHandler xl_enclosure_fan(FanType::xl_enclosure, 0, benevolent_fan_range, &Fans::enclosure());
#endif
#if HAS_XBUDDY_EXTENSION()
    std::array xbe_fans {
        XBEFanHandler(FanType::xbe_chamber, 0, chamber_fan_range),
        XBEFanHandler(FanType::xbe_chamber, 1, chamber_fan_range),
        XBEFanHandler(FanType::xbe_chamber, 2, filtration_fan_range),
    };
    static_assert(puppies::XBuddyExtension::FAN_CNT == 3);
#endif

#if HAS_CHAMBER_API()
    switch (chamber().backend()) {

    #if XL_ENCLOSURE_SUPPORT()
    case Chamber::Backend::xl_enclosure: {
        fan_container[container_index++] = &xl_enclosure_fan;
        break;
    }
    #endif /* XL_ENCLOSURE_SUPPORT() */

    #if HAS_XBUDDY_EXTENSION()
        static_assert(HAS_CHAMBER_FILTRATION_API());
    case Chamber::Backend::xbuddy_extension:
        if (xbuddy_extension().is_fan3_used()) {
            fan_container[container_index++] = &xbe_fans[2];
        } else {
            fan_container[container_index++] = &xbe_fans[0];
            fan_container[container_index++] = &xbe_fans[1];
        }
        break;
    #endif

    case Chamber::Backend::none:
        break;
    }
#endif /* HAS_CHAMBER_API() */

    assert(container_index && container_index <= fan_container.size());

    auto wizard = FanSelfTestWizard(
        PhasesFansSelftest::test_100_percent,
        std::span(fan_container.data(), container_index),
        std::span(tool_fan_pairs.data(), pairs));
    wizard.run_fan_selftest();
}

/** @}*/

} // namespace PrusaGcodeSuite
