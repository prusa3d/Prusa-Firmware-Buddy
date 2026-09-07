#pragma once

#include <fanctl/CFanCtlCommon.hpp>
#include <option/has_switched_fan_test.h>
#include <array>
#include <span>
#include <selftest_fans_config.hpp>
#include <utils/enum_array.hpp>
#include <option/xl_enclosure_support.h>
#include <option/xbuddy_extension_variant.h>
#include <pwm_utils.hpp>
#include <option/has_bed_fan.h>
#include <option/has_psu_fan.h>
#include <option/has_indx.h>
#include <option/has_cpu_fan.h>
#include <option/has_xl_can.h>

#if HAS_BED_FAN()
    #include <feature/bed_fan/controller.hpp>
#endif

namespace fan_selftest {

enum class FanType {
    print = 0,
    heatbreak,
#if HAS_INDX()
    dock,
#endif
#if XL_ENCLOSURE_SUPPORT()
    xl_enclosure,
#endif
#if XBUDDY_EXTENSION_VARIANT_IS_STANDARD()
    xbe_chamber,
#endif
#if HAS_BED_FAN()
    bed,
#endif
#if HAS_PSU_FAN()
    psu,
#endif
#if HAS_CPU_FAN()
    cpu,
#endif
#if HAS_XL_CAN()
    bed_mcu,
#endif
    _count,
};

static constexpr EnumArray<FanType, const char *, FanType::_count> fan_type_names {
    { FanType::print, N_("Print") },
        { FanType::heatbreak, N_("Heatbreak") },
#if HAS_INDX()
        { FanType::dock, N_("Dock") },
#endif
#if XL_ENCLOSURE_SUPPORT()
        { FanType::xl_enclosure, N_("XL Enclosure") },
#endif
#if XBUDDY_EXTENSION_VARIANT_IS_STANDARD()
        { FanType::xbe_chamber, N_("Chamber") },
#endif
#if HAS_BED_FAN()
        { FanType::bed, N_("Bed") },
#endif
#if HAS_PSU_FAN()
        { FanType::psu, "PSU" },
#endif
#if HAS_CPU_FAN()
        { FanType::cpu, N_("CPU") },
#endif
#if HAS_XL_CAN()
        { FanType::bed_mcu, N_("Bed MCU") },
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
    FanHandler(const FanType type, const FanRPMRange range, const uint8_t desc_num, FanRPMRange low_range)
        : fan_type(type)
        , fan_range(range)
        , low_fan_range(low_range)
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
    void set_low_range() { fan_range = low_fan_range; }
    uint8_t get_desc_num() const { return desc_num; }

protected:
    const FanType fan_type;
    FanRPMRange fan_range; ///< range of acceptable RPM values for test at 100% PWM
    FanRPMRange low_fan_range; ///< range of acceptable RPM values for test at 40% PWM
    const uint8_t desc_num; ///< Description number (Tool number or Chamber fan number)
    bool failed { false };
    uint16_t sample_count { 0 };
    uint32_t sample_sum { 0 };
    uint16_t avg_rpm { 0 };
};

class CommonFanHandler : public FanHandler {
public:
    CommonFanHandler(const FanType type, const uint8_t tool_nr, const FanRPMRange fan_range, CFanCtlCommon *fan_control, const FanRPMRange low_fan_range = benevolent_fan_range);
    ~CommonFanHandler();

    virtual void set_pwm(const uint8_t pwm) override;
    virtual void record_sample() override;

private:
    CFanCtlCommon *fan;
};

#if XBUDDY_EXTENSION_VARIANT_IS_STANDARD()
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

#if HAS_BED_FAN()
class BedFanHandler final : public FanHandler {
private:
    bed_fan::Controller::Mode mode;

public:
    BedFanHandler(uint8_t desc_num, const FanRPMRange, const FanRPMRange);
    ~BedFanHandler();

    void set_pwm(uint8_t pwm) final;
    void record_sample() final;
};
#endif

#if HAS_PSU_FAN()
class PSUFanHandler final : public FanHandler {
public:
    PSUFanHandler(const FanRPMRange, const FanRPMRange);
    ~PSUFanHandler();

    void set_pwm(uint8_t pwm) final;
    void record_sample() final;
};
#endif

#if HAS_XL_CAN()
/// XLS Modular Bed cooling fan, driven remotely over the XL-CAN bridge.
class BedMcuFanHandler final : public FanHandler {
public:
    BedMcuFanHandler(const FanRPMRange, const FanRPMRange);
    ~BedMcuFanHandler();

    void set_pwm(uint8_t pwm) final;
    void record_sample() final;

private:
    uint8_t original_pwm;
};
#endif

} // namespace fan_selftest
