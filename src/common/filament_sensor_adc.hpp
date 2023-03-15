/**
 * @file filament_sensor_adc.hpp
 * @author Radek Vana
 * @brief basic api of analog filament sensor
 * @date 2019-12-16
 */

#pragma once

#include "filament_sensor.hpp"

struct metric_s;

class FSensorADC : public FSensor {
public:
    static constexpr int32_t fs_filtered_value_not_ready { INT32_MIN }; // invalid value of fs_filtered_value
    static constexpr int32_t fs_ref_value_not_calibrated { INT32_MIN }; // invalid value of fs_filtered_value

protected:
    int32_t fs_value_span { 0 }; // minimal difference of raw values between the two states of the filament sensor
    int32_t fs_ref_value { 0 };  // value of filament insert in extruder

    eevar_id eeprom_span_id;
    eevar_id eeprom_ref_id;

    std::atomic<int32_t> fs_filtered_value; // current filtered value set from interrupt

    /**
     * @brief Get filtered sensor-specific value.
     * @return filtered ADC value
     */
    virtual int32_t GetFilteredValue() const override { return fs_filtered_value.load(); };

    uint8_t tool_index;

    bool flg_load_settings { true };
    bool flg_calibrate { false };
    bool flg_invalid_calib { false };

    virtual void set_state(fsensor_t st) override;

    virtual void enable() override;
    virtual void disable() override;
    virtual void cycle() override;

    virtual void record_filtered() = 0;

public:
    fsensor_t WaitInitialized();

    FSensorADC(eevar_id span_value, eevar_id ref_value, uint8_t tool_index);

    virtual eevar_id get_eeprom_span_id() const override;
    virtual eevar_id get_eeprom_ref_id() const override;

    /**
    * @brief calibrate filament sensor and store it to eeprom
    * thread safe, only sets flag --> !!! is not done instantly !!!
    * use FSensor::WaitInitialized if valid state is needed
    */
    virtual void SetCalibrateFlag() override;
    virtual void SetLoadSettingsFlag() override;
    virtual void SetInvalidateCalibrationFlag() override;

    int32_t load_settings();

    fsensor_t evaluate_state(int32_t filtered_value);

    void set_filtered_value_from_IRQ(int32_t filtered_value);

    void save_calibration(int32_t value);

    void invalidate_calibration();

    virtual void record_raw(int32_t val) = 0;
};

class FSensorAdcExtruder : public FSensorADC {
protected:
    virtual void record_state() override; // record metrics
    virtual void record_filtered() override;

public:
    static metric_s &get_metric_raw__static();
    static metric_s &get_metric_state__static();
    static metric_s &get_metric_filtered__static();

    FSensorAdcExtruder(eevar_id span_value, eevar_id ref_value, uint8_t tool_index);

    virtual void record_raw(int32_t val) override;
};

class FSensorAdcSide : public FSensorADC {
protected:
    virtual void record_state() override; // record metrics
    virtual void record_filtered() override;

public:
    static metric_s &get_metric_raw__static();
    static metric_s &get_metric_state__static();
    static metric_s &get_metric_filtered__static();

    FSensorAdcSide(eevar_id span_value, eevar_id ref_value, uint8_t tool_index);

    virtual void record_raw(int32_t val) override;
};
