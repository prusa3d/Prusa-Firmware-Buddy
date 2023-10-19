/**
 * @file filament_sensor_adc.hpp
 * @author Radek Vana
 * @brief basic api of analog filament sensor
 * @date 2019-12-16
 */

#pragma once

#include "filament_sensor.hpp"
#include <app_metrics.h>

struct metric_s;

class FSensorADC : public FSensor {
public:
    static constexpr float fs_selftest_span_multipler { 1.2 }; // when doing selftest, fs with filament and without has to be different by this value times configured span to pass selftest

protected:
    int32_t fs_value_span { 0 }; ///< minimal difference of raw values between the two states of the filament sensor
    int32_t fs_ref_ins_value { 0 }; ///< value of filament insert in extruder
    int32_t fs_ref_nins_value { 0 }; ///< value of filament not inserted in extruder

    std::atomic<int32_t> fs_filtered_value; ///< current filtered value set from interrupt

    /**
     * @brief Get filtered sensor-specific value.
     * @return filtered ADC value
     */
    virtual int32_t GetFilteredValue() const override { return fs_filtered_value.load(); };

    uint8_t tool_index;
    bool is_side { false };

    bool flg_load_settings { true };
    CalibrateRequest req_calibrate { CalibrateRequest::NoCalibration };
    bool flg_invalid_calib { false };

    virtual void set_state(fsensor_t st) override;

    virtual void enable() override;
    virtual void disable() override;
    virtual void cycle() override;

    /**
     * @brief Calibrate reference value with filament NOT inserted
     */
    void CalibrateNotInserted(int32_t filtered_value);

    /**
     * @brief Calibrate reference value with filament inserted
     */
    void CalibrateInserted(int32_t filtered_value);

public:
    fsensor_t WaitInitialized();

    FSensorADC(uint8_t tool_index, bool is_side_sensor);

    /**
     * @brief calibrate filament sensor and store it to eeprom
     * thread safe, only sets flag --> !!! is not done instantly !!!
     * use FSensor::WaitInitialized if valid state is needed
     */
    virtual void SetCalibrateRequest(CalibrateRequest) override;
    virtual bool IsCalibrationFinished() const override;
    virtual void SetLoadSettingsFlag() override;
    virtual void SetInvalidateCalibrationFlag() override;

    void load_settings();

    void set_filtered_value_from_IRQ(int32_t filtered_value);

    void invalidate_calibration();

    virtual void record_raw(int32_t val) = 0;
};

class FSensorAdcExtruder : public FSensorADC {
protected:
    // Limit metrics recording for each tool
    buddy::metrics::RunApproxEvery limit_record;
    buddy::metrics::RunApproxEvery limit_record_raw;

    virtual void record_state() override; // record metrics
    void MetricsSetEnabled(bool) override;

public:
    static metric_s &get_metric_raw__static();
    static metric_s &get_metric__static();

    FSensorAdcExtruder(uint8_t tool_index, bool is_side_sensor);

    virtual void record_raw(int32_t val) override;
};

class FSensorAdcSide : public FSensorADC {
protected:
    // Limit metrics recording for each tool
    buddy::metrics::RunApproxEvery limit_record;
    buddy::metrics::RunApproxEvery limit_record_raw;

    virtual void record_state() override; // record metrics
    void MetricsSetEnabled(bool) override;

public:
    static metric_s &get_metric_raw__static();
    static metric_s &get_metric__static();

    FSensorAdcSide(uint8_t tool_index, bool is_side_sensor);

    virtual void record_raw(int32_t val) override;
};
