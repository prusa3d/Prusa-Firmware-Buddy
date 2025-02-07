#include <selftest_fans.hpp>
#include <option/has_xbuddy_extension.h>
#if HAS_XBUDDY_EXTENSION()
    #include <feature/xbuddy_extension/xbuddy_extension.hpp>
    #include <feature/xbuddy_extension/xbuddy_extension_fan_results.hpp>
    #include <puppies/xbuddy_extension.hpp> // For FAN_CNT
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
    if ((desc_num == 0 || desc_num == 1) && buddy::xbuddy_extension().has_fan1_fan2_auto_control()) {
        has_auto_mode = true;
    }
}

XBEFanHandler::~XBEFanHandler() {
    if ((desc_num == 0 || desc_num == 1) && has_auto_mode) {
        buddy::xbuddy_extension().set_fan1_fan2_auto_control(); // They are tested separately so this will be called twice
    }
}

void XBEFanHandler::set_pwm(uint8_t pwm) {
    if (desc_num == 0 || desc_num == 1) {
        buddy::xbuddy_extension().set_fan1_fan2_pwm(buddy::XBuddyExtension::FanPWM { pwm }); // They are tested separately so this will be called twice
    } else if (desc_num == 2) {
        buddy::xbuddy_extension().set_fan3_pwm(buddy::XBuddyExtension::FanPWM { pwm });
    } else {
        assert(false);
    }
}

void XBEFanHandler::record_sample() {
    std::optional<uint16_t> rpm;
    if (desc_num == 0) {
        rpm = buddy::xbuddy_extension().fan1_rpm();
    } else if (desc_num == 1) {
        rpm = buddy::xbuddy_extension().fan2_rpm();
    } else if (desc_num == 2) {
        rpm = buddy::xbuddy_extension().fan3_rpm();
    } else {
        assert(false);
    }

    if (rpm.has_value()) {
        sample_count++;
        sample_sum += rpm.value();
    }
}
#endif
