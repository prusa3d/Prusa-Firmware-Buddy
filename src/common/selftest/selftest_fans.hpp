#pragma once

#include <fanctl/CFanCtlCommon.hpp>
#include <option/has_switched_fan_test.h>
#include <array>
#include <span>
#include <selftest_fans_config.hpp>
#include <enum_array.hpp>
#include <option/xl_enclosure_support.h>
#include <option/has_xbuddy_extension.h>
#include <pwm_utils.hpp>

namespace fan_selftest {

enum class FanType {
    print = 0,
    heatbreak,
#if XL_ENCLOSURE_SUPPORT()
    xl_enclosure,
#endif
#if HAS_XBUDDY_EXTENSION()
    xbe_chamber,
#endif
    _count,
};

static constexpr EnumArray<FanType, const char *, FanType::_count> fan_type_names {
    { FanType::print, N_("Print") },
        { FanType::heatbreak, N_("Heatbreak") },
#if XL_ENCLOSURE_SUPPORT()
        { FanType::xl_enclosure, N_("XL Enclosure") },
#endif
#if HAS_XBUDDY_EXTENSION()
        { FanType::xbe_chamber, N_("Chamber") },
#endif
};

/**
 *  Abstract handler class for different fan types.
 *
 *  Different fans have different control mechanisms.
 *  Idea is, that each different HW will have it's own FanHandler derived class.
 *  Print fans and XL Enclosure fan use CFanCtlCommon base class.
 *
 */
class FanHandler {
public:
    FanHandler(const FanType type, const FanRPMRange range, const uint8_t desc_num)
        : fan_type(type)
        , fan_range(range)
        , desc_num(desc_num) {}

    virtual void set_pwm(const uint8_t pwm) = 0;
    virtual void record_sample() = 0;

    void evaluate();
    uint16_t calculate_avg_rpm();
    void reset_samples();
    TestResult test_result() const;

    inline bool is_rpm_within_bounds(const uint16_t rpm) const {
        return rpm > fan_range.rpm_min && rpm < fan_range.rpm_max;
    }

    bool is_failed() const { return failed; }
    void set_failed(const bool set) { failed = set; }
    uint16_t get_avg_rpm() const { return avg_rpm; }
    FanType get_type() const { return fan_type; }
    FanRPMRange get_range() const { return fan_range; }
    void set_range(const FanRPMRange new_range) { fan_range = new_range; }
    uint8_t get_desc_num() const { return desc_num; }

protected:
    const FanType fan_type;
    FanRPMRange fan_range;
    const uint8_t desc_num; ///< Description number (Tool number or Chamber fan number)
    bool failed { false };
    uint16_t sample_count { 0 };
    uint32_t sample_sum { 0 };
    uint16_t avg_rpm { 0 };
};

class CommonFanHandler : public FanHandler {
public:
    CommonFanHandler(const FanType type, const uint8_t tool_nr, const FanRPMRange fan_range, CFanCtlCommon *fan_control);
    ~CommonFanHandler();

    virtual void set_pwm(const uint8_t pwm) override;
    virtual void record_sample() override;

private:
    CFanCtlCommon *fan;
};

#if HAS_XBUDDY_EXTENSION()
class XBEFanHandler : public FanHandler {
public:
    XBEFanHandler(const FanType type, const uint8_t desc_nr, const FanRPMRange fan_range);
    ~XBEFanHandler();

    virtual void set_pwm(const uint8_t pwm) override;
    virtual void record_sample() override;

private:
    PWM255OrAuto original_pwm;
};
#endif

} // namespace fan_selftest
