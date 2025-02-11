/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2019 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

/**
 * temperature.h - temperature controller
 */

#include "thermistor/thermistors.h"

#include "../inc/MarlinConfig.h"

#if ENABLED(AUTO_POWER_CONTROL)
  #include "../feature/power.h"
#endif


#if ENABLED(MODULAR_HEATBED)
  #include "modular_heatbed.h"
#endif

#if ENABLED(PRUSA_TOOLCHANGER)
  #include "prusa/toolchanger.h"
#endif

#include <atomic>

#ifndef SOFT_PWM_SCALE
  #define SOFT_PWM_SCALE 0
#endif

#if HOTENDS <= 1
  #define HOTEND_INDEX  0
  #define E_NAME
#else
  #define HOTEND_INDEX  e
  #define E_NAME e
#endif

// Identifiers for other heaters
typedef enum : int8_t {
  INDEX_NONE = -5,
  H_REDUNDANT, H_CHAMBER, H_BOARD, H_BED,
  H_E0, H_E1, H_E2, H_E3, H_E4, H_E5,
  H_HEATBREAK_E0, H_HEATBREAK_E1, H_HEATBREAK_E2, H_HEATBREAK_E3, H_HEATBREAK_E4, H_HEATBREAK_E5,
} heater_ind_t;
static_assert(H_E0 == 0); // lots of places in are indexed by this, and assumes H_E0 is zero
static_assert(EXTRUDERS <= 6);


// PID storage
typedef struct { float Kp, Ki, Kd;     } PID_t;
typedef struct { float Kp, Ki, Kd, Kc; } PIDC_t;
#if ENABLED(PID_EXTRUSION_SCALING)
  typedef PIDC_t hotend_pid_t;
#else
  typedef PID_t hotend_pid_t;
#endif

#define DUMMY_PID_VALUE 3000.0f

#if ENABLED(PIDTEMP)
  #define _PID_Kp(H) Temperature::temp_hotend[H].pid.Kp
  #define _PID_Ki(H) Temperature::temp_hotend[H].pid.Ki
  #define _PID_Kd(H) Temperature::temp_hotend[H].pid.Kd
  #if ENABLED(PID_EXTRUSION_SCALING)
    #define _PID_Kc(H) Temperature::temp_hotend[H].pid.Kc
  #else
    #define _PID_Kc(H) 1
  #endif
#else
  #define _PID_Kp(H) DUMMY_PID_VALUE
  #define _PID_Ki(H) DUMMY_PID_VALUE
  #define _PID_Kd(H) DUMMY_PID_VALUE
  #define _PID_Kc(H) 1
#endif

#define PID_PARAM(F,H) _PID_##F(H)

/**
 * States for ADC reading in the ISR
 */
enum ADCSensorState : char {
  StartSampling,
  #if HAS_TEMP_ADC_0
    PrepareTemp_0, MeasureTemp_0,
  #endif
  #if HAS_HEATED_BED
    PrepareTemp_BED, MeasureTemp_BED,
  #endif
  #if HAS_TEMP_CHAMBER
    PrepareTemp_CHAMBER, MeasureTemp_CHAMBER,
  #endif
  #if HAS_TEMP_HEATBREAK
    PrepareTemp_HEATBREAK, MeasureTemp_HEATBREAK,
  #endif
  #if HAS_TEMP_BOARD
    PrepareTemp_BOARD, MeasureTemp_BOARD,
  #endif
  #if PRINTER_IS_PRUSA_iX()
    PrepareTemp_PSU, MeasureTemp_PSU,
    PrepareTemp_AMBIENT, MeasureTemp_AMBIENT,
  #endif
  #if HAS_TEMP_ADC_1
    PrepareTemp_1, MeasureTemp_1,
  #endif
  #if HAS_TEMP_ADC_2
    PrepareTemp_2, MeasureTemp_2,
  #endif
  #if HAS_TEMP_ADC_3
    PrepareTemp_3, MeasureTemp_3,
  #endif
  #if HAS_TEMP_ADC_4
    PrepareTemp_4, MeasureTemp_4,
  #endif
  #if HAS_TEMP_ADC_5
    PrepareTemp_5, MeasureTemp_5,
  #endif
  #if HAS_JOY_ADC_X
    PrepareJoy_X, MeasureJoy_X,
  #endif
  #if HAS_JOY_ADC_Y
    PrepareJoy_Y, MeasureJoy_Y,
  #endif
  #if HAS_JOY_ADC_Z
    PrepareJoy_Z, MeasureJoy_Z,
  #endif
  #if ENABLED(FILAMENT_WIDTH_SENSOR)
    Prepare_FILWIDTH, Measure_FILWIDTH,
  #endif
  #if HAS_ADC_BUTTONS
    Prepare_ADC_KEY, Measure_ADC_KEY,
  #endif
  SensorsReady, // Temperatures ready. Delay the next round of readings to let ADC pins settle.
  StartupDelay  // Startup, delay initial temp reading a tiny bit so the hardware can settle
};

// Minimum number of Temperature::ISR loops between sensor readings.
// Multiplied by 16 (OVERSAMPLENR) to obtain the total time to
// get all oversampled sensor readings
#define MIN_ADC_ISR_LOOPS 10

#define ACTUAL_ADC_SAMPLES _MAX(int(MIN_ADC_ISR_LOOPS), int(SensorsReady))

#if HAS_PID_HEATING
  #define PID_K2 (1-float(PID_K1))
  #define HEATBREAK_PID_K2 (1-float(HEATBREAK_PID_K1))
  #define PID_dT ((OVERSAMPLENR * float(ACTUAL_ADC_SAMPLES)) / TEMP_TIMER_FREQUENCY)

  // Apply the scale factors to the PID values
  #define scalePID_i(i)   ( float(i) * PID_dT )
  #define unscalePID_i(i) ( float(i) / PID_dT )
  #define scalePID_d(d)   ( float(d) / PID_dT )
  #define unscalePID_d(d) ( float(d) * PID_dT )
#endif

#define G26_CLICK_CAN_CANCEL (HAS_LCD_MENU && ENABLED(G26_MESH_VALIDATION))

// A temperature sensor
typedef struct TempInfo {
  static constexpr float celsius_uninitialized = -1.0f;

  uint16_t acc;
  int16_t raw;
  float celsius = celsius_uninitialized;
  inline void reset() { acc = 0; }
  inline void sample(const uint16_t s) { acc += s; }
  inline void update() { raw = acc; }
} temp_info_t;

// A PWM heater with temperature sensor
typedef struct HeaterInfo : public TempInfo {
  int16_t target;
  uint8_t soft_pwm_amount;
} heater_info_t;

// A heater with PID stabilization
template<typename T>
struct PIDHeaterInfo : public HeaterInfo {
  T pid;  // Initialized by settings.load()
};

// Modular heater
#if ENABLED(MODULAR_HEATBED)
struct ModularBedHeater: public HeaterInfo {
  uint16_t enabled_mask = 0xffff;
};
#endif

#if ENABLED(PIDTEMP)
  typedef struct PIDHeaterInfo<hotend_pid_t> hotend_info_t;
#else
  typedef heater_info_t hotend_info_t;
#endif
#if HAS_HEATED_BED
  #if ENABLED(PIDTEMPBED)
    typedef struct PIDHeaterInfo<PID_t> bed_info_t;
  #elif ENABLED(MODULAR_HEATBED)
    typedef ModularBedHeater bed_info_t;
  #else
    typedef heater_info_t bed_info_t;
  #endif
#endif
#if HAS_HEATED_CHAMBER
  typedef heater_info_t chamber_info_t;
#elif HAS_TEMP_CHAMBER
  typedef temp_info_t chamber_info_t;
#endif

#if HAS_TEMP_HEATBREAK
  #if ENABLED(PIDTEMPHEATBREAK)
    typedef struct PIDHeaterInfo<PID_t> heatbreak_info_t;
  #elif HAS_TEMP_HEATBREAK_CONTROL
    typedef heater_info_t heatbreak_info_t;
  #else
    typedef temp_info_t heatbreak_info_t;
  #endif

#endif

#if HAS_TEMP_BOARD
  typedef temp_info_t board_info_t;
#endif

// Heater idle handling
typedef struct {
  millis_t timeout_ms;
  bool timed_out;
  inline void update(const millis_t &ms) { if (!timed_out && timeout_ms && ELAPSED(ms, timeout_ms)) timed_out = true; }
  inline void start(const millis_t &ms) { timeout_ms = millis() + ms; timed_out = false; }
  inline void reset() { timeout_ms = 0; timed_out = false; }
  inline void expire() { start(0); }
} heater_idle_t;

// Heater watch handling
typedef struct {
  uint16_t target;
  millis_t next_ms;
  inline bool elapsed(const millis_t &ms) { return next_ms && ELAPSED(ms, next_ms); }
  inline bool elapsed() { return elapsed(millis()); }
} heater_watch_t;

// Temperature sensor read value ranges
typedef struct { int16_t raw_min, raw_max; } raw_range_t;
typedef struct { int16_t mintemp, maxtemp; } celsius_range_t;
typedef struct { int16_t raw_min, raw_max, mintemp, maxtemp; } temp_range_t;

#define THERMISTOR_ADC_RESOLUTION       1024           // 10-bit ADC .. shame to waste 12-bits of resolution on 32-bit
#define THERMISTOR_ABS_ZERO_C           -273.15f       // bbbbrrrrr cold !
#define THERMISTOR_RESISTANCE_NOMINAL_C 25.0f          // mmmmm comfortable

#if HAS_USER_THERMISTORS

  enum CustomThermistorIndex : uint8_t {
    #if ENABLED(HEATER_0_USER_THERMISTOR)
      CTI_HOTEND_0,
    #endif
    #if ENABLED(HEATER_1_USER_THERMISTOR)
      CTI_HOTEND_1,
    #endif
    #if ENABLED(HEATER_2_USER_THERMISTOR)
      CTI_HOTEND_2,
    #endif
    #if ENABLED(HEATER_3_USER_THERMISTOR)
      CTI_HOTEND_3,
    #endif
    #if ENABLED(HEATER_4_USER_THERMISTOR)
      CTI_HOTEND_4,
    #endif
    #if ENABLED(HEATER_5_USER_THERMISTOR)
      CTI_HOTEND_5,
    #endif
    #if ENABLED(HEATER_BED_USER_THERMISTOR)
      CTI_BED,
    #endif
    #if ENABLED(HEATER_CHAMBER_USER_THERMISTOR)
      CTI_CHAMBER,
    #endif
    #if ENABLED(HEATBREAK_USER_THERMISTOR)
      CTI_HEATBREAK,
    #endif
    #if ENABLED(BOARD_USER_THERMISTOR)
      CTI_BOARD,
    #endif
    USER_THERMISTORS
  };

  // User-defined thermistor
  typedef struct {
    bool pre_calc;     // true if pre-calculations update needed
    float sh_c_coeff,  // Steinhart-Hart C coefficient .. defaults to '0.0'
          sh_alpha,
          series_res,
          res_25, res_25_recip,
          res_25_log,
          beta, beta_recip;
  } user_thermistor_t;

#endif

class Temperature {

  public:

    static volatile bool in_temp_isr;

    #if HOTENDS
      #if ENABLED(TEMP_SENSOR_1_AS_REDUNDANT)
        #define HOTEND_TEMPS (HOTENDS + 1)
      #else
        #define HOTEND_TEMPS HOTENDS
      #endif
      static hotend_info_t temp_hotend[HOTEND_TEMPS];

      #if TEMP_RESIDENCY_TIME > 0
        // timestamp when temeperature reached target +-TEMP_WINDOW, 0 when outside this window
        // note: 0 is valid timestamp, but if temperature reaches window at time 0, it will just be evaluated again little later, so it doesn't cause any bug
        static uint32_t temp_hotend_residency_start_ms[HOTEND_TEMPS];
      #endif
      
    #endif

    #if HAS_HEATED_BED
      static bed_info_t temp_bed;
    #endif

    #if HAS_TEMP_CHAMBER
      static chamber_info_t temp_chamber;
    #endif
    #if HAS_TEMP_BOARD
      static board_info_t temp_board;
    #endif

    #if HAS_TEMP_HEATBREAK
      static heatbreak_info_t temp_heatbreak[HOTENDS];
    #endif

    #if PRINTER_IS_PRUSA_iX()
      static TempInfo temp_psu;
      static TempInfo temp_ambient;
    #endif

    #if ENABLED(AUTO_POWER_E_FANS)
      static uint8_t autofan_speed[HOTENDS];
    #endif

    #if ENABLED(AUTO_POWER_CHAMBER_FAN)
      static uint8_t chamberfan_speed;
    #endif

    #if ENABLED(FAN_SOFT_PWM)
      static uint8_t soft_pwm_amount_fan[FAN_COUNT],
                     soft_pwm_count_fan[FAN_COUNT];
    #endif

    // For metrics only
    #if !HAS_MODULARBED()
      std::atomic<int> bed_pwm;
    #endif
    std::atomic<int> nozzle_pwm;

    #if ENABLED(PREVENT_COLD_EXTRUSION)
      static bool allow_cold_extrude;
      static int16_t extrude_min_temp;
      FORCE_INLINE static bool tooCold(const int16_t temp) { return allow_cold_extrude ? false : temp < extrude_min_temp; }
      FORCE_INLINE static bool tooColdToExtrude(const uint8_t E_NAME) {
        return tooCold(degHotend(HOTEND_INDEX));
      }
      FORCE_INLINE static bool targetTooColdToExtrude(const uint8_t E_NAME) {
        return tooCold(degTargetHotend(HOTEND_INDEX));
      }
    #else
      FORCE_INLINE static bool tooColdToExtrude(const uint8_t) { return false; }
      FORCE_INLINE static bool targetTooColdToExtrude(const uint8_t) { return false; }
    #endif

    FORCE_INLINE static bool hotEnoughToExtrude(const uint8_t e) { return !tooColdToExtrude(e); }
    FORCE_INLINE static bool targetHotEnoughToExtrude(const uint8_t e) { return !targetTooColdToExtrude(e); }

    #if HEATER_IDLE_HANDLER
      static heater_idle_t hotend_idle[HOTENDS];
      #if HAS_HEATED_BED
        static heater_idle_t bed_idle;
      #endif
      #if HAS_HEATED_CHAMBER
        static heater_idle_t chamber_idle;
      #endif
    #endif

    #if ENABLED(PID_EXTRUSION_SCALING)
      FORCE_INLINE static bool getExtrusionScalingEnabled() { return extrusion_scaling_enabled; }
      FORCE_INLINE static void setExtrusionScalingEnabled(bool enabled) { extrusion_scaling_enabled = enabled; }
    #endif

  private:

    #if EARLY_WATCHDOG
      static bool inited;   // If temperature controller is running
    #endif

    static volatile bool temp_meas_ready;

    #if WATCH_HOTENDS
      static heater_watch_t watch_hotend[HOTENDS];
    #endif

    #if ENABLED(TEMP_SENSOR_1_AS_REDUNDANT)
      static uint16_t redundant_temperature_raw;
      static float redundant_temperature;
    #endif

    #if ENABLED(PID_EXTRUSION_SCALING)
      static uint32_t last_e_position;
      static bool extrusion_scaling_enabled;
    #endif

    #if HOTENDS
      static temp_range_t temp_range[HOTENDS];
    #endif

    #if HAS_HEATED_BED
      #if WATCH_BED
        static heater_watch_t watch_bed;
      #endif
      #if DISABLED(PIDTEMPBED)
        static millis_t next_bed_check_ms;
      #endif
      #ifdef BED_MINTEMP
        static int16_t mintemp_raw_BED;
      #endif
      #ifdef BED_MAXTEMP
        static int16_t maxtemp_raw_BED;
      #endif
    #endif

    #if HAS_TEMP_HEATBREAK
      #if WATCH_HEATBREAK
        static heater_watch_t watch_heatbreak;
      #endif
      static millis_t next_heatbreak_check_ms;
      #ifdef HEATBREAK_MINTEMP
        static int16_t mintemp_raw_HEATBREAK;
      #endif
      #ifdef HEATBREAK_MAXTEMP
        static int16_t maxtemp_raw_HEATBREAK;
      #endif
    #endif

    #if HAS_TEMP_BOARD
      #ifdef BOARD_MINTEMP
        static int16_t mintemp_raw_BOARD;
      #endif
      #ifdef BOARD_MAXTEMP
        static int16_t maxtemp_raw_BOARD;
      #endif
    #endif

    #if HAS_HEATED_CHAMBER
      #if WATCH_CHAMBER
        static heater_watch_t watch_chamber;
      #endif
      static millis_t next_chamber_check_ms;
      #ifdef CHAMBER_MINTEMP
        static int16_t mintemp_raw_CHAMBER;
      #endif
      #ifdef CHAMBER_MAXTEMP
        static int16_t maxtemp_raw_CHAMBER;
      #endif
    #endif

    #ifdef MAX_CONSECUTIVE_LOW_TEMPERATURE_ERROR_ALLOWED
      static uint8_t consecutive_low_temperature_error[HOTENDS];
    #endif

    #ifdef MILLISECONDS_PREHEAT_TIME
      static millis_t preheat_end_time[HOTENDS];
    #endif

    #if HAS_AUTO_FAN
      static millis_t next_auto_fan_check_ms;
    #endif

    #if ENABLED(PROBING_HEATERS_OFF)
      static bool paused;
    #endif

  public:
    #if HAS_ADC_BUTTONS
      static uint32_t current_ADCKey_raw;
      static uint8_t ADCKey_count;
    #endif

    #if ENABLED(PID_EXTRUSION_SCALING)
      static int16_t lpq_len;
    #endif

    /**
     * Instance Methods
     */

    void init();

    /**
     * Static (class) methods
     */

    #if HAS_USER_THERMISTORS
      static user_thermistor_t user_thermistor[USER_THERMISTORS];
      static void log_user_thermistor(const uint8_t t_index, const bool eprom=false);
      static void reset_user_thermistors();
      static float user_thermistor_to_deg_c(const uint8_t t_index, const int raw);
      static bool set_pull_up_res(int8_t t_index, float value) {
        //if (!WITHIN(t_index, 0, USER_THERMISTORS - 1)) return false;
        if (!WITHIN(value, 1, 1000000)) return false;
        user_thermistor[t_index].series_res = value;
        return true;
      }
      static bool set_res25(int8_t t_index, float value) {
        if (!WITHIN(value, 1, 10000000)) return false;
        user_thermistor[t_index].res_25 = value;
        user_thermistor[t_index].pre_calc = true;
        return true;
      }
      static bool set_beta(int8_t t_index, float value) {
        if (!WITHIN(value, 1, 1000000)) return false;
        user_thermistor[t_index].beta = value;
        user_thermistor[t_index].pre_calc = true;
        return true;
      }
      static bool set_sh_coeff(int8_t t_index, float value) {
        if (!WITHIN(value, -0.01f, 0.01f)) return false;
        user_thermistor[t_index].sh_c_coeff = value;
        user_thermistor[t_index].pre_calc = true;
        return true;
      }
    #endif

    #if HOTENDS
      static float analog_to_celsius_hotend(const int raw, const uint8_t e);
    #endif

    #if HAS_HEATED_BED
      static float analog_to_celsius_bed(const int raw);
    #endif
    #if HAS_TEMP_CHAMBER
      static float analog_to_celsius_chamber(const int raw);
    #endif
    #if HAS_TEMP_BOARD
      static float analog_to_celsius_board(const int raw);
    #endif

    #if HAS_TEMP_HEATBREAK
      static float analog_to_celsius_heatbreak(const int raw);
    #endif

    #if FAN_COUNT > 0

      static uint8_t fan_speed[FAN_COUNT];
      #define FANS_LOOP(I) LOOP_L_N(I, FAN_COUNT)

      static void set_fan_speed(const uint8_t target, const uint16_t speed);

      #if EITHER(PROBING_FANS_OFF, ADVANCED_PAUSE_FANS_PAUSE)
        static bool fans_paused;
        static uint8_t saved_fan_speed[FAN_COUNT];
      #endif

      static constexpr inline uint8_t fanPercent(const uint8_t speed) { return ui8_to_percent(speed); }

      #if ENABLED(ADAPTIVE_FAN_SLOWING)
        static uint8_t fan_speed_scaler[FAN_COUNT];
      #endif

      static inline uint8_t scaledFanSpeed([[maybe_unused]] const uint8_t target, const uint8_t fs) {
        return (fs * uint16_t(
          #if ENABLED(ADAPTIVE_FAN_SLOWING)
            fan_speed_scaler[target]
          #else
            128
          #endif
        )) >> 7;
      }

      static inline uint8_t scaledFanSpeed(const uint8_t target) {
        return scaledFanSpeed(target, fan_speed[target]);
      }

      #if ENABLED(EXTRA_FAN_SPEED)
        static uint8_t old_fan_speed[FAN_COUNT], new_fan_speed[FAN_COUNT];
        static void set_temp_fan_speed(const uint8_t fan, const uint16_t tmp_temp);
      #endif

      #if EITHER(PROBING_FANS_OFF, ADVANCED_PAUSE_FANS_PAUSE)
        void set_fans_paused(const bool p);
      #endif

    #endif // FAN_COUNT > 0

    static inline void zero_fan_speeds() {
      #if FAN_COUNT > 0
        FANS_LOOP(i) set_fan_speed(i, 0);
      #endif
    }

    /**
     * Called from the Temperature ISR
     */
    static void readings_ready();
    static void isr();

    /**
     * Call periodically to manage heaters
     */
    static void manage_heater() __O2; // __O2 added to work around a compiler error
    static inline void task() { manage_heater(); } // stub

    // Return true if the temperatures have been sampled at least once
    static bool temperatures_ready();

    /**
     * Preheating hotends
     */
    #ifdef MILLISECONDS_PREHEAT_TIME
      static bool is_preheating(const uint8_t E_NAME) {
        return preheat_end_time[HOTEND_INDEX] && PENDING(millis(), preheat_end_time[HOTEND_INDEX]);
      }
      static void start_preheat_time(const uint8_t E_NAME) {
        preheat_end_time[HOTEND_INDEX] = millis() + MILLISECONDS_PREHEAT_TIME;
      }
      static void reset_preheat_time(const uint8_t E_NAME) {
        preheat_end_time[HOTEND_INDEX] = 0;
      }
    #else
      #define is_preheating(n) (false)
    #endif

    //high level conversion routines, for use outside of temperature.cpp
    //inline so that there is no performance decrease.
    //deg=degreeCelsius

    FORCE_INLINE static float degHotend(const uint8_t E_NAME) {
      return (0
        #if HOTENDS
          + temp_hotend[HOTEND_INDEX].celsius
        #endif
      );
    }

    #if ENABLED(SHOW_TEMP_ADC_VALUES)
      FORCE_INLINE static int16_t rawHotendTemp(const uint8_t E_NAME) {
        return (0
          #if HOTENDS
            + temp_hotend[HOTEND_INDEX].raw
          #endif
        );
      }
    #endif

    FORCE_INLINE static int16_t degTargetHotend(const uint8_t E_NAME) {
      return (0
        #if HOTENDS
          + temp_hotend[HOTEND_INDEX].target
        #endif
      );
    }

    #if WATCH_HOTENDS
      static void start_watching_hotend(const uint8_t e=0);
    #else
      static inline void start_watching_hotend(const uint8_t=0) {}
    #endif

    #if HOTENDS

      static void setTargetHotend(const int16_t celsius, const uint8_t E_NAME) {
        const uint8_t ee = HOTEND_INDEX;
        const int16_t new_temp = _MIN(celsius, temp_range[ee].maxtemp - HEATER_MAXTEMP_SAFETY_MARGIN);

        #ifdef MILLISECONDS_PREHEAT_TIME
          if (celsius == 0)
            reset_preheat_time(ee);
          else if (temp_hotend[ee].target == 0)
            start_preheat_time(ee);
        #endif
        #if ENABLED(AUTO_POWER_CONTROL)
          if (celsius) {
            powerManager.power_on();
          }
        #endif
        
        #if TEMP_RESIDENCY_TIME > 0
          // target changed, reset time when it reached target
          if (temp_hotend[ee].target != new_temp){
            temp_hotend_residency_start_ms[ee] = 0;
          }
        #endif

        temp_hotend[ee].target = new_temp;
        
        start_watching_hotend(ee);
        #if ENABLED(PRUSA_TOOLCHANGER)
          prusa_toolchanger.getTool(ee).set_hotend_target_temp(temp_hotend[ee].target);
        #endif

      }

      FORCE_INLINE static bool isHeatingHotend(const uint8_t E_NAME) {
        return temp_hotend[HOTEND_INDEX].target > temp_hotend[HOTEND_INDEX].celsius;
      }

      FORCE_INLINE static bool isCoolingHotend(const uint8_t E_NAME) {
        return temp_hotend[HOTEND_INDEX].target < temp_hotend[HOTEND_INDEX].celsius;
      }

      #if HAS_TEMP_HOTEND
        static bool wait_for_hotend(const uint8_t target_extruder, const bool no_wait_for_cooling=true, bool fan_cooling=false
          #if G26_CLICK_CAN_CANCEL
            , const bool click_to_cancel=false
          #endif
        );
      #endif

      FORCE_INLINE static bool still_heating(const uint8_t e) {
        return degTargetHotend(e) > TEMP_HYSTERESIS && ABS(degHotend(e) - degTargetHotend(e)) > TEMP_HYSTERESIS;
      }

    #endif // HOTENDS

    #if HAS_HEATED_BED

      #if ENABLED(SHOW_TEMP_ADC_VALUES)
        FORCE_INLINE static int16_t rawBedTemp()  { return temp_bed.raw; }
      #endif

      FORCE_INLINE static float degBed()          { return temp_bed.celsius; }
      FORCE_INLINE static int16_t degTargetBed()  { return temp_bed.target; }
      FORCE_INLINE static bool isHeatingBed()     { return temp_bed.target > temp_bed.celsius; }
      FORCE_INLINE static bool isCoolingBed()     { return temp_bed.target < temp_bed.celsius; }

      #if ENABLED(MODULAR_HEATBED)
        FORCE_INLINE static uint16_t getEnabledBedletMask() {
          return temp_bed.enabled_mask;
        }
        FORCE_INLINE static void setEnabledBedletMask(const uint16_t enabled_mask) {
          temp_bed.enabled_mask = enabled_mask;
          for(uint8_t x = 0; x < X_HBL_COUNT; ++x) {
            for(uint8_t y = 0; y < Y_HBL_COUNT; ++y) {
              int16_t target_temp = 0;
              if(temp_bed.enabled_mask & (1 << advanced_modular_bed->idx(x, y))) {
                target_temp = temp_bed.target;
              }
              advanced_modular_bed->set_target(x, y, target_temp);
            }
          }
          advanced_modular_bed->update_bedlet_temps(temp_bed.enabled_mask, temp_bed.target);
          updateModularBedTemperature(); // update current temperature of modular bed - it will be now calculated from different bedlets

        }
        static void updateModularBedTemperature(); // will update temp_bed.celsius based on currently enabled bedlets

      #endif

      #if WATCH_BED
        static void start_watching_bed();
      #else
        static inline void start_watching_bed() {}
      #endif

      static void setTargetBed(const int16_t celsius) {
        #if ENABLED(AUTO_POWER_CONTROL)
          if (celsius) {
            powerManager.power_on();
          }
        #endif
        temp_bed.target =
          #ifdef BED_MAXTEMP
            _MIN(celsius, BED_MAXTEMP - BED_MAXTEMP_SAFETY_MARGIN)
          #else
            celsius
          #endif
        ;

        #if ENABLED(MODULAR_HEATBED)
          for(uint8_t x = 0; x < X_HBL_COUNT; ++x) {
            for(uint8_t y = 0; y < Y_HBL_COUNT; ++y) {
              int16_t target_temp = 0;
              if(temp_bed.enabled_mask & (1 << advanced_modular_bed->idx(x, y))) {
                target_temp = temp_bed.target;
              }
              advanced_modular_bed->set_target(x, y, target_temp);
            }
          }
          advanced_modular_bed->update_bedlet_temps(temp_bed.enabled_mask, temp_bed.target);
        #endif

        start_watching_bed();
      }

      static bool wait_for_bed(const bool no_wait_for_cooling=true
        #if G26_CLICK_CAN_CANCEL
          , const bool click_to_cancel=false
        #endif
      );

    #endif // HAS_HEATED_BED

    #if HAS_TEMP_CHAMBER
      #if ENABLED(SHOW_TEMP_ADC_VALUES)
        FORCE_INLINE static int16_t rawChamberTemp()    { return temp_chamber.raw; }
      #endif
      FORCE_INLINE static float degChamber()            { return temp_chamber.celsius; }
      #if HAS_HEATED_CHAMBER
        FORCE_INLINE static int16_t degTargetChamber()  { return temp_chamber.target; }
        FORCE_INLINE static bool isHeatingChamber()     { return temp_chamber.target > temp_chamber.celsius; }
        FORCE_INLINE static bool isCoolingChamber()     { return temp_chamber.target < temp_chamber.celsius; }

        static bool wait_for_chamber(const bool no_wait_for_cooling=true);
      #endif
    #endif // HAS_TEMP_CHAMBER

    #if WATCH_CHAMBER
      static void start_watching_chamber();
    #else
      static inline void start_watching_chamber() {}
    #endif

    #if HAS_HEATED_CHAMBER
      static void setTargetChamber(const int16_t celsius) {
        temp_chamber.target =
          #ifdef CHAMBER_MAXTEMP
            _MIN(celsius, CHAMBER_MAXTEMP)
          #else
            celsius
          #endif
        ;
        start_watching_chamber();
      }
    #endif // HAS_HEATED_CHAMBER

    #if HAS_TEMP_HEATBREAK
      #if ENABLED(SHOW_TEMP_ADC_VALUES)
        FORCE_INLINE static int16_t rawHeatbreakTemp(const uint8_t E_NAME)    { return temp_heatbreak[HOTEND_INDEX].raw; }
      #endif
      FORCE_INLINE static float degHeatbreak(const uint8_t E_NAME)            { return temp_heatbreak[HOTEND_INDEX].celsius; }
      #if HAS_TEMP_HEATBREAK_CONTROL
        FORCE_INLINE static int16_t degTargetHeatbreak(const uint8_t E_NAME)  { return temp_heatbreak[HOTEND_INDEX].target; }
        FORCE_INLINE static bool isHeatingHeatbreak(const uint8_t E_NAME)     { return temp_heatbreak[HOTEND_INDEX].target > temp_heatbreak[HOTEND_INDEX].celsius; }
        FORCE_INLINE static bool isCoolingHeatbreak(const uint8_t E_NAME)     { return temp_heatbreak[HOTEND_INDEX].target < temp_heatbreak[HOTEND_INDEX].celsius; }

        static void suspend_heatbreak_fan(millis_t ms);
      #endif
    #endif // HAS_TEMP_HEATBREAK

    #if WATCH_HEATBREAK
      static void start_watching_heatbreak();
    #else
      static inline void start_watching_heatbreak() {}
    #endif

    #if HAS_TEMP_HEATBREAK_CONTROL
      static void setTargetHeatbreak(const int16_t celsius, const uint8_t E_NAME) {
        temp_heatbreak[HOTEND_INDEX].target =
          #ifdef HEATBREAK_MAXTEMP
            _MIN(celsius, HEATBREAK_MAXTEMP)
          #else
            celsius
          #endif
        ;
        #if ENABLED(PRUSA_TOOLCHANGER)
          prusa_toolchanger.getTool(HOTEND_INDEX).set_heatbreak_target_temp(celsius);
        #endif
        start_watching_heatbreak();
      }
    #endif // HAS_TEMP_HEATBREAK

    #if HAS_TEMP_BOARD
      #if ENABLED(SHOW_TEMP_ADC_VALUES)
        FORCE_INLINE static int16_t rawBoardTemp()    { return temp_board.raw; }
      #endif
      FORCE_INLINE static float degBoard()            { return temp_board.celsius; }
    #endif // HAS_TEMP_BOARD

    #if PRINTER_IS_PRUSA_iX()
      FORCE_INLINE static float deg_psu() { return temp_psu.celsius; }
      FORCE_INLINE static float deg_ambient() { return temp_ambient.celsius; }
    #endif

    /**
     * The software PWM power for a heater
     */
    static int16_t getHeaterPower(const heater_ind_t heater);

private:
    enum class disable_bed_t : bool {no, yes};
    /**
     * used by disable_all_heaters and disable_hotend
     */
    static void disable_heaters(disable_bed_t disable_bed);
  
    #if TEMP_RESIDENCY_TIME > 0
      static void update_temp_residency_hotend(uint8_t hotend);
    #endif
public:
    /**
     * Switch off all heaters, set all target temperatures to 0
     */
    static void disable_all_heaters();
    /**
     * Like above, but disables only heaters on local CPU.
     *
     * The ones run by a separate CPU is left intact. Can be used in
     * interrupts, as this avoids interprocessor communication.
     */
    static void disable_local_heaters();
    /**
     * Switch off all hotends, set all hotend target temperatures to 0
     */
    static void disable_hotend();

    /**
     * Perform auto-tuning for hotend or bed in response to M303
     */
    #if HAS_PID_HEATING
      #if ENABLED(PID_AUTOTUNE)
        static void PID_autotune(const float &target, const heater_ind_t hotend, const int8_t ncycles, const bool set_result=false);
      #endif

      #if ENABLED(NO_FAN_SLOWING_IN_PID_TUNING)
        static bool adaptive_fan_slowing;
      #elif ENABLED(ADAPTIVE_FAN_SLOWING)
        static constexpr bool adaptive_fan_slowing = true;
      #endif

      /**
       * Update the temp manager when PID values change
       */
      #if ENABLED(PIDTEMP)
        FORCE_INLINE static void updatePID() {
          #if ENABLED(PID_EXTRUSION_SCALING)
            last_e_position = 0;
          #endif
          #if ENABLED(PRUSA_TOOLCHANGER)
            // Set PID parameters to all dwarves
            HOTEND_LOOP() {
              buddy::puppies::dwarfs[e].set_pid(Temperature::temp_hotend[e].pid.Kp, Temperature::temp_hotend[e].pid.Ki, Temperature::temp_hotend[e].pid.Kd);
            }
          #endif /*HAS_DWARF()*/
        }
      #endif

    #endif

    #if ENABLED(PROBING_HEATERS_OFF)
      static void pause(const bool p);
      FORCE_INLINE static bool is_paused() { return paused; }
    #endif

    #if HEATER_IDLE_HANDLER

      static void reset_heater_idle_timer(const uint8_t E_NAME) {
        hotend_idle[HOTEND_INDEX].reset();
        start_watching_hotend(HOTEND_INDEX);
      }

      #if HAS_HEATED_BED
        static void reset_bed_idle_timer() {
          bed_idle.reset();
          start_watching_bed();
        }
      #endif

    #endif // HEATER_IDLE_HANDLER

    #if HAS_TEMP_SENSOR
      static void print_heater_states(const uint8_t target_extruder
        #if ENABLED(TEMP_SENSOR_1_AS_REDUNDANT)
          , const bool include_r=false
        #endif
      );
      #if ENABLED(AUTO_REPORT_TEMPERATURES)
        static uint8_t auto_report_temp_interval;
        static millis_t next_temp_report_ms;
        static void auto_report_temperatures();
        static inline void set_auto_report_interval(uint8_t v) {
          NOMORE(v, 60);
          auto_report_temp_interval = v;
          next_temp_report_ms = millis() + 1000UL * v;
        }
      #endif
    #endif

    #if HAS_DISPLAY
      static void set_heating_message(const uint8_t e);
    #endif

    #if ENABLED(MODEL_DETECT_STUCK_THERMISTOR)
      static bool saneTempReadingHotend(const uint8_t E_NAME) {
          if (failed_cycles[HOTEND_INDEX] > THERMAL_PROTECTION_MODEL_PERIOD) return false;
          else return true;
      }
    #else
      static bool saneTempReadingHotend(const uint8_t){return true;}
    #endif

  private:
    static void set_current_temp_raw();
    static void updateTemperaturesFromRawValues();

    #define HAS_MAX6675 EITHER(HEATER_0_USES_MAX6675, HEATER_1_USES_MAX6675)
    #if HAS_MAX6675
      #if BOTH(HEATER_0_USES_MAX6675, HEATER_1_USES_MAX6675)
        #define COUNT_6675 2
      #else
        #define COUNT_6675 1
      #endif
      #if COUNT_6675 > 1
        #define READ_MAX6675(N) read_max6675(N)
      #else
        #define READ_MAX6675(N) read_max6675()
      #endif
      static int read_max6675(
        #if COUNT_6675 > 1
          const uint8_t hindex=0
        #endif
      );
    #endif

    static void checkExtruderAutoFans();

    static float get_pid_output_hotend(
#if ENABLED(MODEL_DETECT_STUCK_THERMISTOR)
            float &feed_forward ,
#endif
            const uint8_t e
      );
    static float get_model_output_hotend(float &last_target, float &expected, const uint8_t e);

    #if ENABLED(PIDTEMPBED)
      static float get_pid_output_bed();
    #endif

    #if HAS_HEATED_CHAMBER
      static float get_pid_output_chamber();
    #endif

    #if ENABLED(PIDTEMPHEATBREAK)
      static float get_pid_output_heatbreak();
    #endif

    static void _temp_error(const heater_ind_t e, PGM_P const serial_msg, PGM_P const lcd_msg);
    static void min_temp_error(const heater_ind_t e);
    static void max_temp_error(const heater_ind_t e);

    #define HAS_THERMAL_PROTECTION (EITHER(THERMAL_PROTECTION_HOTENDS, THERMAL_PROTECTION_CHAMBER) || HAS_THERMALLY_PROTECTED_BED)

    #if HAS_THERMAL_PROTECTION

      enum TRState : char { TRInactive, TRFirstHeating, TRStable, TRRunaway };

      typedef struct {
        millis_t timer = 0;
        TRState state = TRInactive;
      } tr_state_machine_t;

      #if ENABLED(THERMAL_PROTECTION_HOTENDS)
        static tr_state_machine_t tr_state_machine[HOTENDS];
      #endif
      #if HAS_THERMALLY_PROTECTED_BED
        static tr_state_machine_t tr_state_machine_bed;
      #endif
      #if ENABLED(THERMAL_PROTECTION_CHAMBER)
        static tr_state_machine_t tr_state_machine_chamber;
      #endif


      static void thermal_runaway_protection(tr_state_machine_t &state, const float &current, const float &target, const heater_ind_t heater_id, const uint16_t period_seconds, const uint16_t hysteresis_degc);

      #if ENABLED(MODEL_DETECT_STUCK_THERMISTOR)
        static int_least8_t failed_cycles[HOTENDS];
        static constexpr int_least8_t self_healing_cycles = 10;
        static_assert((THERMAL_PROTECTION_MODEL_PERIOD + self_healing_cycles) < INT_LEAST8_MAX, "THERMAL_PROTECTION_MODEL_PERIOD doesn't fit int_least8_t.");
        static void thermal_model_protection(const float &pid_output, const float &feed_forward, const uint8_t e);
      #endif
    #endif // HAS_THERMAL_PROTECTION
};

extern Temperature thermalManager;
