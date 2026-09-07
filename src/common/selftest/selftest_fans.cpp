#include <selftest_fans.hpp>
#include <option/xbuddy_extension_variant.h>
#if XBUDDY_EXTENSION_VARIANT_IS_STANDARD()
    #include <feature/xbuddy_extension/xbuddy_extension.hpp>
    #include <feature/xbuddy_extension/xbuddy_extension_fan_results.hpp>
    #include <puppies/xbuddy_extension.hpp> // For FAN_CNT
#endif
#include <logging/log.hpp>
#if HAS_BED_FAN()
    #include <feature/bed_fan/controller.hpp>
    #include <feature/bed_fan/bed_fan.hpp>
#endif
#if HAS_PSU_FAN()
    #include <feature/psu_fan/psu_fan.hpp>
#endif
#if HAS_XL_CAN()
    #include <puppies/xl_can.hpp>
#endif

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
    return is_failed() ? TestResult::failed : TestResult::passed;
}

CommonFanHandler::CommonFanHandler(const FanType type, uint8_t tool_nr, FanRPMRange fan_range, CFanCtlCommon *fan_control, FanRPMRange low_fan_range)
    : FanHandler(type, fan_range, tool_nr, low_fan_range)
    , fan(fan_control) {
    fan->enter_selftest_mode();
}

CommonFanHandler::~CommonFanHandler() {
    fan->exit_selftest_mode();
}

void CommonFanHandler::set_pwm(uint8_t pwm) {
    fan->selftest_set_pwm(pwm);
}

void CommonFanHandler::record_sample() {
    sample_count++;
    sample_sum += fan->get_actual_rpm();
}

#if XBUDDY_EXTENSION_VARIANT_IS_STANDARD()

static_assert(buddy::puppies::XBuddyExtension::FAN_CNT == XBEFanTestResults::fan_count, "Adjust the fan result structure in EEPROM (xbuddy_expansion_fan_result.hpp)");

XBEFanHandler::XBEFanHandler(const FanType type, uint8_t desc_num, FanRPMRange fan_range)
    : FanHandler(type, fan_range, desc_num, benevolent_fan_range) {
    original_pwm = buddy::xbuddy_extension().fan_target_pwm(static_cast<buddy::XBuddyExtension::Fan>(desc_num));
}

XBEFanHandler::~XBEFanHandler() {
    buddy::xbuddy_extension().set_fan_target_pwm(static_cast<buddy::XBuddyExtension::Fan>(desc_num), original_pwm);
}

void XBEFanHandler::set_pwm(uint8_t pwm) {
    buddy::xbuddy_extension().set_fan_target_pwm(static_cast<buddy::XBuddyExtension::Fan>(desc_num), buddy::XBuddyExtension::FanPWM { pwm });
}

void XBEFanHandler::record_sample() {
    const std::optional<uint16_t> rpm = buddy::xbuddy_extension().fan_rpm(static_cast<buddy::XBuddyExtension::Fan>(desc_num));
    if (rpm.has_value()) {
        sample_count++;
        sample_sum += rpm.value();
    }
}
#endif

#if HAS_BED_FAN()
BedFanHandler::BedFanHandler(uint8_t desc_num, FanRPMRange high_fan_range, FanRPMRange low_fan_range)
    : FanHandler(FanType::bed, high_fan_range, desc_num, low_fan_range)
    , mode { bed_fan::controller().get_mode() } {
    set_pwm(0);
}

BedFanHandler::~BedFanHandler() {
    bed_fan::controller().set_mode(mode);
}

void BedFanHandler::set_pwm(uint8_t pwm) {
    bed_fan::controller().set_mode(bed_fan::Controller::ManualMode { .pwm = pwm });
}

void BedFanHandler::record_sample() {
    const auto rpm = bed_fan::bed_fan().get_rpm();
    if (rpm.has_value()) {
        sample_count++;
        sample_sum += rpm.value()[desc_num];
    }
}
#endif

#if HAS_PSU_FAN()
PSUFanHandler::PSUFanHandler(FanRPMRange high_fan_range, FanRPMRange low_fan_range)
    : FanHandler(FanType::psu, high_fan_range, 0 /*no used*/, low_fan_range) {
    // We cannot save previous value, it's not readable
}

PSUFanHandler::~PSUFanHandler() {
    set_pwm(0); // We cannot restore previous value, it's not readable
}

void PSUFanHandler::set_pwm(uint8_t pwm) {
    psu_fan::psu_fan().set_pwm(pwm);
}

void PSUFanHandler::record_sample() {
    const std::optional<uint16_t> rpm = psu_fan::psu_fan().get_rpm();
    if (rpm.has_value()) {
        sample_count++;
        sample_sum += rpm.value();
    }
}
#endif

#if HAS_XL_CAN()
BedMcuFanHandler::BedMcuFanHandler(FanRPMRange high_fan_range, FanRPMRange low_fan_range)
    : FanHandler(FanType::bed_mcu, high_fan_range, 0 /*not used*/, low_fan_range)
    , original_pwm(buddy::puppies::xl_can.get_fan_pwm()) {
}

BedMcuFanHandler::~BedMcuFanHandler() {
    buddy::puppies::xl_can.set_fan_pwm(original_pwm, buddy::puppies::XlCan::FanSelftestMode::exit_selftest);
}

void BedMcuFanHandler::set_pwm(uint8_t pwm) {
    buddy::puppies::xl_can.set_fan_pwm(pwm, buddy::puppies::XlCan::FanSelftestMode::set_selftest);
}

void BedMcuFanHandler::record_sample() {
    const std::optional<uint16_t> rpm = buddy::puppies::xl_can.get_fan_rpm();
    if (rpm.has_value()) {
        sample_count++;
        sample_sum += rpm.value();
    }
}
#endif
