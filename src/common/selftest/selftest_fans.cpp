#include <selftest_fans.hpp>
#include <option/has_xbuddy_extension.h>
#if HAS_XBUDDY_EXTENSION()
    #include <puppies/xbuddy_extension.hpp>
    #include <puppies/xbuddy_extension_fan_results.hpp>
#endif
#include <logging/log.hpp>

LOG_COMPONENT_REF(Selftest);

using namespace fan_selftest;

void FanHandler::evaluate() {
    calculate_avg_rpm();
    bool passed = is_rpm_within_bounds(avg_rpm);
    log_info(Selftest, "%s fan %c: RPM %u %s range (%u - %u)",
        fan_type_names[fan_type],
        '0' + desc_num,
        avg_rpm,
        passed ? "in" : "out of",
        fan_range.rpm_min,
        fan_range.rpm_max);
    if (!passed) {
        failed = true;
    }
}

uint16_t FanHandler::calculate_avg_rpm() {
    avg_rpm = sample_count ? (sample_sum / sample_count) : 0;
    return avg_rpm;
}

void FanHandler::reset_samples() {
    sample_count = 0;
    sample_sum = 0;
    avg_rpm = 0;
}

TestResult FanHandler::test_result() const {
    return is_failed() ? TestResult_Failed : TestResult_Passed;
}

CommonFanHandler::CommonFanHandler(const FanType type, uint8_t tool_nr, FanRPMRange fan_range, CFanCtlCommon *fan_control)
    : FanHandler(type, fan_range, tool_nr)
    , fan(fan_control) {
    fan->enterSelftestMode();
}

CommonFanHandler::~CommonFanHandler() {
    fan->exitSelftestMode();
}

void CommonFanHandler::set_pwm(uint8_t pwm) {
    fan->selftestSetPWM(pwm);
}

void CommonFanHandler::record_sample() {
    sample_count++;
    sample_sum += fan->getActualRPM();
}

#if HAS_XBUDDY_EXTENSION()

static_assert(buddy::puppies::XBuddyExtension::FAN_CNT == XBEFanTestResults::fan_count, "Adjust the fan result structure in EEPROM (xbuddy_expansion_fan_result.hpp)");

XBEFanHandler::XBEFanHandler(const FanType type, uint8_t desc_num, FanRPMRange fan_range)
    : FanHandler(type, fan_range, desc_num) {
}

void XBEFanHandler::set_pwm(uint8_t pwm) {
    buddy::puppies::xbuddy_extension.set_fan_pwm(desc_num, pwm);
}

void XBEFanHandler::record_sample() {
    const auto rpm = buddy::puppies::xbuddy_extension.get_fan_rpm(desc_num);
    if (rpm.has_value()) {
        sample_count++;
        sample_sum += rpm.value();
    }
}
#endif
