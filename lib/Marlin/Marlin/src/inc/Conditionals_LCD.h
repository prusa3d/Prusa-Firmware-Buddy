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
 * Conditionals_LCD.h
 * Conditionals that need to be set before Configuration_adv.h or pins.h
 */

#if ENABLED(CARTESIO_UI)

  #define DOGLCD
  #define IS_ULTIPANEL

#elif ENABLED(ZONESTAR_LCD)

  #define ADC_KEYPAD
  #define IS_RRW_KEYPAD
  #define REPRAPWORLD_KEYPAD_MOVE_STEP 10.0
  #define ADC_KEY_NUM 8
  #define IS_ULTIPANEL

  // This helps to implement ADC_KEYPAD menus
  #define REVERSE_MENU_DIRECTION
  #define ENCODER_PULSES_PER_STEP 1
  #define ENCODER_STEPS_PER_MENU_ITEM 1
  #define ENCODER_FEEDRATE_DEADZONE 2

#elif ENABLED(RADDS_DISPLAY)
  #define IS_ULTIPANEL
  #define ENCODER_PULSES_PER_STEP 2

#elif EITHER(ANET_FULL_GRAPHICS_LCD, BQ_LCD_SMART_CONTROLLER)

  #define IS_RRD_FG_SC

#elif ANY(miniVIKI, VIKI2, ELB_FULL_GRAPHIC_CONTROLLER, AZSMZ_12864)

  #define IS_ULTRA_LCD
  #define DOGLCD
  #define IS_ULTIPANEL

  #if ENABLED(miniVIKI)
    #define U8GLIB_ST7565_64128N
  #elif ENABLED(VIKI2)
    #define U8GLIB_ST7565_64128N
  #elif ENABLED(ELB_FULL_GRAPHIC_CONTROLLER)
    #define U8GLIB_LM6059_AF
    #define SD_DETECT_INVERTED
  #elif ENABLED(AZSMZ_12864)
    #define U8GLIB_ST7565_64128N
  #endif

#elif ENABLED(OLED_PANEL_TINYBOY2)

  #define IS_U8GLIB_SSD1306
  #define IS_ULTIPANEL

#elif ENABLED(RA_CONTROL_PANEL)

  #define LCD_I2C_TYPE_PCA8574
  #define LCD_I2C_ADDRESS 0x27   // I2C Address of the port expander
  #define IS_ULTIPANEL

#elif ENABLED(REPRAPWORLD_GRAPHICAL_LCD)

  #define DOGLCD
  #define U8GLIB_ST7920
  #define IS_ULTIPANEL

#elif ENABLED(CR10_STOCKDISPLAY)

  #define IS_RRD_FG_SC
  #ifndef ST7920_DELAY_1
    #define ST7920_DELAY_1 DELAY_NS(125)
  #endif
  #ifndef ST7920_DELAY_2
    #define ST7920_DELAY_2 DELAY_NS(125)
  #endif
  #ifndef ST7920_DELAY_3
    #define ST7920_DELAY_3 DELAY_NS(125)
  #endif

#elif ENABLED(MKS_12864OLED)

  #define IS_RRD_SC
  #define U8GLIB_SH1106

#elif ENABLED(MKS_12864OLED_SSD1306)

  #define IS_RRD_SC
  #define IS_U8GLIB_SSD1306

#elif ENABLED(MKS_MINI_12864)

  #define MINIPANEL

#elif ANY(FYSETC_MINI_12864_X_X, FYSETC_MINI_12864_1_2, FYSETC_MINI_12864_2_0, FYSETC_MINI_12864_2_1)

  #define FYSETC_MINI_12864
  #define DOGLCD
  #define IS_ULTIPANEL
  #define LED_COLORS_REDUCE_GREEN
  #if HAS_POWER_SWITCH && EITHER(FYSETC_MINI_12864_2_0, FYSETC_MINI_12864_2_1)
    #define LED_BACKLIGHT_TIMEOUT 10000
  #endif

  // Require LED backlighting enabled
  #if EITHER(FYSETC_MINI_12864_1_2, FYSETC_MINI_12864_2_0)
    #define RGB_LED
  #elif ENABLED(FYSETC_MINI_12864_2_1)
    #define LED_CONTROL_MENU
    #define NEOPIXEL_LED
    #undef NEOPIXEL_TYPE
    #define NEOPIXEL_TYPE       NEO_RGB
    #if NEOPIXEL_PIXELS < 3
      #undef NEOPIXELS_PIXELS
      #define NEOPIXEL_PIXELS     3
    #endif
    #ifndef NEOPIXEL_BRIGHTNESS
      #define NEOPIXEL_BRIGHTNESS 127
    #endif
    //#define NEOPIXEL_STARTUP_TEST
  #endif

#elif ENABLED(ULTI_CONTROLLER)

  #define IS_ULTIPANEL
  #define U8GLIB_SSD1309
  #define LCD_RESET_PIN LCD_PINS_D6 //  This controller need a reset pin
  #define ENCODER_PULSES_PER_STEP 2
  #define ENCODER_STEPS_PER_MENU_ITEM 2

#elif ENABLED(MAKEBOARD_MINI_2_LINE_DISPLAY_1602)

  #define IS_RRD_SC
  #define LCD_WIDTH 16
  #define LCD_HEIGHT 2

#endif

#if ENABLED(IS_RRD_FG_SC)
  #define REPRAP_DISCOUNT_FULL_GRAPHIC_SMART_CONTROLLER
#endif

#if EITHER(MAKRPANEL, MINIPANEL)
  #define IS_ULTIPANEL
  #define DOGLCD
  #if ENABLED(MAKRPANEL)
    #define U8GLIB_ST7565_64128N
  #endif
#endif

#if ENABLED(IS_U8GLIB_SSD1306)
  #define U8GLIB_SSD1306
#endif

#if ENABLED(OVERLORD_OLED)
  #define IS_ULTIPANEL
  #define U8GLIB_SH1106
  /**
   * PCA9632 for buzzer and LEDs via i2c
   * No auto-inc, red and green leds switched, buzzer
   */
  #define PCA9632
  #define PCA9632_NO_AUTO_INC
  #define PCA9632_GRN         0x00
  #define PCA9632_RED         0x02
  #define PCA9632_BUZZER
  #define PCA9632_BUZZER_DATA { 0x09, 0x02 }

  #define ENCODER_PULSES_PER_STEP     1 // Overlord uses buttons
  #define ENCODER_STEPS_PER_MENU_ITEM 1
#endif

// 128x64 I2C OLED LCDs - SSD1306/SSD1309/SH1106
#define HAS_SSD1306_OLED_I2C ANY(U8GLIB_SSD1306, U8GLIB_SSD1309, U8GLIB_SH1106)
#if HAS_SSD1306_OLED_I2C
  #define IS_ULTRA_LCD
  #define DOGLCD
#endif

// ST7920-based graphical displays
#if ANY(REPRAP_DISCOUNT_FULL_GRAPHIC_SMART_CONTROLLER, LCD_FOR_MELZI, SILVER_GATE_GLCD_CONTROLLER)
  #define DOGLCD
  #define U8GLIB_ST7920
  #define IS_RRD_SC
#endif

// RepRapDiscount LCD or Graphical LCD with rotary click encoder
#if ENABLED(IS_RRD_SC)
  #define REPRAP_DISCOUNT_SMART_CONTROLLER
#endif

/**
 * SPI Ultipanels
 */

// Basic Ultipanel-like displays
#if ANY(ULTIMAKERCONTROLLER, REPRAP_DISCOUNT_SMART_CONTROLLER, G3D_PANEL, RIGIDBOT_PANEL, PANEL_ONE, U8GLIB_SH1106)
  #define IS_ULTIPANEL
#endif

// Einstart OLED has Cardinal nav via pins defined in pins_EINSTART-S.h
#if ENABLED(U8GLIB_SH1106_EINSTART)
  #define DOGLCD
  #define IS_ULTIPANEL
#endif

// FSMC/SPI TFT Panels
#if ENABLED(FSMC_GRAPHICAL_TFT)
  #define DOGLCD
  #define IS_ULTIPANEL
  #define DELAYED_BACKLIGHT_INIT
#endif

/**
 * I2C Panels
 */

#if EITHER(LCD_SAINSMART_I2C_1602, LCD_SAINSMART_I2C_2004)

  #define LCD_I2C_TYPE_PCF8575
  #define LCD_I2C_ADDRESS 0x27   // I2C Address of the port expander

  #if ENABLED(LCD_SAINSMART_I2C_2004)
    #define LCD_WIDTH 20
    #define LCD_HEIGHT 4
  #endif

#elif ENABLED(LCD_I2C_PANELOLU2)

  // PANELOLU2 LCD with status LEDs, separate encoder and click inputs

  #define LCD_I2C_TYPE_MCP23017
  #define LCD_I2C_ADDRESS 0x20 // I2C Address of the port expander
  #define LCD_USE_I2C_BUZZER   // Enable buzzer on LCD (optional)
  #define IS_ULTIPANEL

#elif ENABLED(LCD_I2C_VIKI)

  /**
   * Panucatt VIKI LCD with status LEDs, integrated click & L/R/U/P buttons, separate encoder inputs
   *
   * This uses the LiquidTWI2 library v1.2.3 or later ( https://github.com/lincomatic/LiquidTWI2 )
   * Make sure the LiquidTWI2 directory is placed in the Arduino or Sketchbook libraries subdirectory.
   * Note: The pause/stop/resume LCD button pin should be connected to the Arduino
   *       BTN_ENC pin (or set BTN_ENC to -1 if not used)
   */
  #define LCD_I2C_TYPE_MCP23017
  #define LCD_I2C_ADDRESS 0x20 // I2C Address of the port expander
  #define LCD_USE_I2C_BUZZER   // Enable buzzer on LCD (requires LiquidTWI2 v1.2.3 or later)
  #define IS_ULTIPANEL

  #define ENCODER_FEEDRATE_DEADZONE 4

  #define STD_ENCODER_PULSES_PER_STEP 1
  #define STD_ENCODER_STEPS_PER_MENU_ITEM 2

#elif ENABLED(G3D_PANEL)

  #define STD_ENCODER_PULSES_PER_STEP 2
  #define STD_ENCODER_STEPS_PER_MENU_ITEM 1

#elif ANY(REPRAP_DISCOUNT_SMART_CONTROLLER, miniVIKI, VIKI2, ELB_FULL_GRAPHIC_CONTROLLER, AZSMZ_12864, OLED_PANEL_TINYBOY2, BQ_LCD_SMART_CONTROLLER, LCD_I2C_PANELOLU2)

  #define STD_ENCODER_PULSES_PER_STEP 4
  #define STD_ENCODER_STEPS_PER_MENU_ITEM 1

#endif

#ifndef STD_ENCODER_PULSES_PER_STEP
  #if ENABLED(TOUCH_BUTTONS)
    #define STD_ENCODER_PULSES_PER_STEP 1
  #else
    #define STD_ENCODER_PULSES_PER_STEP 5
  #endif
#endif
#ifndef STD_ENCODER_STEPS_PER_MENU_ITEM
  #define STD_ENCODER_STEPS_PER_MENU_ITEM 1
#endif
#ifndef ENCODER_PULSES_PER_STEP
  #define ENCODER_PULSES_PER_STEP STD_ENCODER_PULSES_PER_STEP
#endif
#ifndef ENCODER_STEPS_PER_MENU_ITEM
  #define ENCODER_STEPS_PER_MENU_ITEM STD_ENCODER_STEPS_PER_MENU_ITEM
#endif
#ifndef ENCODER_FEEDRATE_DEADZONE
  #define ENCODER_FEEDRATE_DEADZONE 6
#endif

// Shift register panels
// ---------------------
// 2 wire Non-latching LCD SR from:
// https://bitbucket.org/fmalpartida/new-liquidcrystal/wiki/schematics#!shiftregister-connection
#if ENABLED(FF_INTERFACEBOARD)
  #define SR_LCD_3W_NL    // Non latching 3 wire shift register
  #define IS_ULTIPANEL
#elif ENABLED(SAV_3DLCD)
  #define SR_LCD_2W_NL    // Non latching 2 wire shift register
  #define IS_ULTIPANEL
#endif

#if ENABLED(IS_ULTIPANEL)
  #define ULTIPANEL
#endif
#if ENABLED(ULTIPANEL)
  #define IS_ULTRA_LCD
  #ifndef NEWPANEL
    #define NEWPANEL
  #endif
#endif

#if ENABLED(IS_ULTRA_LCD)
  #define ULTRA_LCD
#endif

#if ENABLED(IS_RRW_KEYPAD)
  #define REPRAPWORLD_KEYPAD
#endif

// Keypad needs a move step
#if ENABLED(REPRAPWORLD_KEYPAD)
  #define NEWPANEL
  #ifndef REPRAPWORLD_KEYPAD_MOVE_STEP
    #define REPRAPWORLD_KEYPAD_MOVE_STEP 1.0
  #endif
#endif

// Aliases for LCD features
#define HAS_SPI_LCD          ENABLED(ULTRA_LCD)
#define HAS_DISPLAY         (HAS_SPI_LCD || ENABLED(EXTENSIBLE_UI))
#define HAS_GRAPHICAL_LCD    ENABLED(DOGLCD)
#define HAS_CHARACTER_LCD   (HAS_SPI_LCD && !HAS_GRAPHICAL_LCD)
#define HAS_LCD_MENU        (ENABLED(ULTIPANEL) && DISABLED(NO_LCD_MENUS))
#define HAS_ADC_BUTTONS      ENABLED(ADC_KEYPAD)

/**
 *  Multi-Material-Unit supported models
 */
#define PRUSA_MMU1             1
#define PRUSA_MMU2             2
#define PRUSA_MMU2S            3
#define EXTENDABLE_EMU_MMU2   12
#define EXTENDABLE_EMU_MMU2S  13

#ifdef MMU_MODEL
  #define HAS_MMU 1
  #if MMU_MODEL == PRUSA_MMU1
    #define HAS_PRUSA_MMU1 1
  #elif MMU_MODEL % 10 == PRUSA_MMU2
    #define HAS_PRUSA_MMU2 1
  #elif MMU_MODEL % 10 == PRUSA_MMU2S
    #define HAS_PRUSA_MMU2 1
    #define HAS_PRUSA_MMU2S 1
  #endif
  #if MMU_MODEL >= EXTENDABLE_EMU_MMU2
    #define HAS_EXTENDABLE_MMU 1
  #endif
#endif

#undef PRUSA_MMU1
#undef PRUSA_MMU2
#undef PRUSA_MMU2S
#undef EXTENDABLE_EMU_MMU2
#undef EXTENDABLE_EMU_MMU2S

#ifdef HAS_PRUSA_MMU2
  // TODO: compatibility stub for Marlin 2.0. Remove after merge. Use HAS_PRUSA_MMU2 instead.
  #define PRUSA_MMU2 1
#endif

/**
 * Extruders have some combination of stepper motors and hotends
 * so we separate these concepts into the defines:
 *
 *  EXTRUDERS    - Number of Selectable Tools
 *  HOTENDS      - Number of hotends, whether connected or separate
 *  E_STEPPERS   - Number of actual E stepper motors
 *  E_MANUAL     - Number of E steppers for LCD move options
 *
 * These defines must be simple constants for use in REPEAT, etc.
 */
#if EXTRUDERS
  #define HAS_EXTRUDERS 1
  #if EXTRUDERS > 1
    #define HAS_MULTI_EXTRUDER 1
  #endif
  #define E_AXIS_N(E) AxisEnum(E_AXIS + E_INDEX_N(E))
#else
  #undef EXTRUDERS
  #define EXTRUDERS 0
  #undef SINGLENOZZLE
  #undef SWITCHING_EXTRUDER
  #undef SWITCHING_NOZZLE
  #undef MIXING_EXTRUDER
  #undef HOTEND_IDLE_TIMEOUT
  #undef DISABLE_E
#endif

#define E_OPTARG(N) OPTARG(HAS_MULTI_EXTRUDER, N)
#define E_TERN_(N)  TERN_(HAS_MULTI_EXTRUDER, N)
#define E_TERN0(N)  TERN0(HAS_MULTI_EXTRUDER, N)

#if ENABLED(E_DUAL_STEPPER_DRIVERS) // E0/E1 steppers act in tandem as E0

  #define E_STEPPERS      2
  #define E_MANUAL        1

#elif ENABLED(SWITCHING_EXTRUDER)   // One stepper for every two EXTRUDERS

  #if EXTRUDERS > 4
    #define E_STEPPERS    3
  #elif EXTRUDERS > 2
    #define E_STEPPERS    2
  #else
    #define E_STEPPERS    1
  #endif
  #if DISABLED(SWITCHING_NOZZLE)
    #define HOTENDS       E_STEPPERS
  #endif

#elif ENABLED(MIXING_EXTRUDER)      // Multiple feeds are mixed proportionally

  #define E_STEPPERS      MIXING_STEPPERS
  #define E_MANUAL        1
  #if MIXING_STEPPERS == 2
    #define HAS_DUAL_MIXING 1
  #endif

#elif ENABLED(SWITCHING_TOOLHEAD)   // Toolchanger

  #define E_STEPPERS      EXTRUDERS
  #define E_MANUAL        EXTRUDERS

#elif HAS_PRUSA_MMU2                // Průša Multi-Material Unit v2

  #define E_STEPPERS      1
  #define E_MANUAL        1
#elif ENABLED(PRUSA_TOOLCHANGER)
  #define E_STEPPERS      1
  #define E_MANUAL        EXTRUDERS
#endif

// No inactive extruders with SWITCHING_NOZZLE or Průša MMU1
#if ENABLED(SWITCHING_NOZZLE) || HAS_PRUSA_MMU1
  #undef DISABLE_INACTIVE_EXTRUDER
#endif

// Průša MMU1, MMU(S) 2.0 and EXTENDABLE_EMU_MMU2(S) force SINGLENOZZLE
#if HAS_MMU
  #define SINGLENOZZLE
#endif

#if EITHER(SINGLENOZZLE, MIXING_EXTRUDER)         // One hotend, one thermistor, no XY offset
  #undef HOTENDS
  #define HOTENDS       1
  #undef HOTEND_OFFSET_X
  #undef HOTEND_OFFSET_Y
#endif

#ifndef HOTENDS
  #define HOTENDS EXTRUDERS
#endif
#ifndef E_STEPPERS
  #define E_STEPPERS EXTRUDERS
#endif
#ifndef E_MANUAL
  #define E_MANUAL EXTRUDERS
#endif

#if E_STEPPERS <= 7
  #undef INVERT_E7_DIR
  #if E_STEPPERS <= 6
    #undef INVERT_E6_DIR
    #if E_STEPPERS <= 5
      #undef INVERT_E5_DIR
      #if E_STEPPERS <= 4
        #undef INVERT_E4_DIR
        #if E_STEPPERS <= 3
          #undef INVERT_E3_DIR
          #if E_STEPPERS <= 2
            #undef INVERT_E2_DIR
            #if E_STEPPERS <= 1
              #undef INVERT_E1_DIR
              #if E_STEPPERS == 0
                #undef INVERT_E0_DIR
              #endif
            #endif
          #endif
        #endif
      #endif
    #endif
  #endif
#endif

/**
 * Number of Linear Axes (e.g., XYZIJKUVW)
 * All the logical axes except for the tool (E) axis
 */
#ifdef NUM_AXES
  #undef NUM_AXES
  #define NUM_AXES_WARNING 1
#endif

#ifdef W_DRIVER_TYPE
  #define NUM_AXES 9
#elif defined(V_DRIVER_TYPE)
  #define NUM_AXES 8
#elif defined(U_DRIVER_TYPE)
  #define NUM_AXES 7
#elif defined(K_DRIVER_TYPE)
  #define NUM_AXES 6
#elif defined(J_DRIVER_TYPE)
  #define NUM_AXES 5
#elif defined(I_DRIVER_TYPE)
  #define NUM_AXES 4
#elif defined(Z_DRIVER_TYPE)
  #define NUM_AXES 3
#elif defined(Y_DRIVER_TYPE)
  #define NUM_AXES 2
#else
  #define NUM_AXES 1
#endif
#if NUM_AXES >= XY
  #define HAS_Y_AXIS 1
  #if NUM_AXES >= XYZ
    #define HAS_Z_AXIS 1
    #ifdef Z4_DRIVER_TYPE
      #define NUM_Z_STEPPERS 4
    #elif defined(Z3_DRIVER_TYPE)
      #define NUM_Z_STEPPERS 3
    #elif defined(Z2_DRIVER_TYPE)
      #define NUM_Z_STEPPERS 2
    #else
      #define NUM_Z_STEPPERS 1
    #endif
    #if NUM_AXES >= 4
      #define HAS_I_AXIS 1
      #if NUM_AXES >= 5
        #define HAS_J_AXIS 1
        #if NUM_AXES >= 6
          #define HAS_K_AXIS 1
          #if NUM_AXES >= 7
            #define HAS_U_AXIS 1
            #if NUM_AXES >= 8
              #define HAS_V_AXIS 1
              #if NUM_AXES >= 9
                #define HAS_W_AXIS 1
              #endif
            #endif
          #endif
        #endif
      #endif
    #endif
  #endif
#endif

#if E_STEPPERS <= 0
  #undef E0_DRIVER_TYPE
#endif
#if E_STEPPERS <= 1
  #undef E1_DRIVER_TYPE
#endif
#if E_STEPPERS <= 2
  #undef E2_DRIVER_TYPE
#endif
#if E_STEPPERS <= 3
  #undef E3_DRIVER_TYPE
#endif
#if E_STEPPERS <= 4
  #undef E4_DRIVER_TYPE
#endif
#if E_STEPPERS <= 5
  #undef E5_DRIVER_TYPE
#endif
#if E_STEPPERS <= 6
  #undef E6_DRIVER_TYPE
#endif
#if E_STEPPERS <= 7
  #undef E7_DRIVER_TYPE
#endif

#if !HAS_Y_AXIS
  #undef ENDSTOPPULLUP_YMIN
  #undef ENDSTOPPULLUP_YMAX
  #undef Y_MIN_ENDSTOP_INVERTING
  #undef Y_MAX_ENDSTOP_INVERTING
  #undef Y2_DRIVER_TYPE
  #undef Y_ENABLE_ON
  #undef DISABLE_Y
  #undef INVERT_Y_DIR
  #undef Y_HOME_DIR
  #undef Y_MIN_POS
  #undef Y_MAX_POS
  #undef MANUAL_Y_HOME_POS
  #undef MIN_SOFTWARE_ENDSTOP_Y
  #undef MAX_SOFTWARE_ENDSTOP_Y
  #undef SAFE_BED_LEVELING_START_Y
#endif

#if !HAS_Z_AXIS
  #undef ENDSTOPPULLUP_ZMIN
  #undef ENDSTOPPULLUP_ZMAX
  #undef Z_MIN_ENDSTOP_INVERTING
  #undef Z_MAX_ENDSTOP_INVERTING
  #undef Z2_DRIVER_TYPE
  #undef Z3_DRIVER_TYPE
  #undef Z4_DRIVER_TYPE
  #undef Z_ENABLE_ON
  #undef DISABLE_Z
  #undef INVERT_Z_DIR
  #undef Z_HOME_DIR
  #undef Z_MIN_POS
  #undef Z_MAX_POS
  #undef MANUAL_Z_HOME_POS
  #undef MIN_SOFTWARE_ENDSTOP_Z
  #undef MAX_SOFTWARE_ENDSTOP_Z
  #undef SAFE_BED_LEVELING_START_Z
#endif

#if !HAS_I_AXIS
  #undef ENDSTOPPULLUP_IMIN
  #undef ENDSTOPPULLUP_IMAX
  #undef I_MIN_ENDSTOP_INVERTING
  #undef I_MAX_ENDSTOP_INVERTING
  #undef I_ENABLE_ON
  #undef DISABLE_I
  #undef INVERT_I_DIR
  #undef I_HOME_DIR
  #undef I_MIN_POS
  #undef I_MAX_POS
  #undef MANUAL_I_HOME_POS
  #undef MIN_SOFTWARE_ENDSTOP_I
  #undef MAX_SOFTWARE_ENDSTOP_I
  #undef SAFE_BED_LEVELING_START_I
#endif

#if !HAS_J_AXIS
  #undef ENDSTOPPULLUP_JMIN
  #undef ENDSTOPPULLUP_JMAX
  #undef J_MIN_ENDSTOP_INVERTING
  #undef J_MAX_ENDSTOP_INVERTING
  #undef J_ENABLE_ON
  #undef DISABLE_J
  #undef INVERT_J_DIR
  #undef J_HOME_DIR
  #undef J_MIN_POS
  #undef J_MAX_POS
  #undef MANUAL_J_HOME_POS
  #undef MIN_SOFTWARE_ENDSTOP_J
  #undef MAX_SOFTWARE_ENDSTOP_J
  #undef SAFE_BED_LEVELING_START_J
#endif

#if !HAS_K_AXIS
  #undef ENDSTOPPULLUP_KMIN
  #undef ENDSTOPPULLUP_KMAX
  #undef K_MIN_ENDSTOP_INVERTING
  #undef K_MAX_ENDSTOP_INVERTING
  #undef K_ENABLE_ON
  #undef DISABLE_K
  #undef INVERT_K_DIR
  #undef K_HOME_DIR
  #undef K_MIN_POS
  #undef K_MAX_POS
  #undef MANUAL_K_HOME_POS
  #undef MIN_SOFTWARE_ENDSTOP_K
  #undef MAX_SOFTWARE_ENDSTOP_K
  #undef SAFE_BED_LEVELING_START_K
#endif

#if !HAS_U_AXIS
  #undef ENDSTOPPULLUP_UMIN
  #undef ENDSTOPPULLUP_UMAX
  #undef U_MIN_ENDSTOP_INVERTING
  #undef U_MAX_ENDSTOP_INVERTING
  #undef U_ENABLE_ON
  #undef DISABLE_U
  #undef INVERT_U_DIR
  #undef U_HOME_DIR
  #undef U_MIN_POS
  #undef U_MAX_POS
  #undef MANUAL_U_HOME_POS
  #undef MIN_SOFTWARE_ENDSTOP_U
  #undef MAX_SOFTWARE_ENDSTOP_U
  #undef SAFE_BED_LEVELING_START_U
#endif

#if !HAS_V_AXIS
  #undef ENDSTOPPULLUP_VMIN
  #undef ENDSTOPPULLUP_VMAX
  #undef V_MIN_ENDSTOP_INVERTING
  #undef V_MAX_ENDSTOP_INVERTING
  #undef V_ENABLE_ON
  #undef DISABLE_V
  #undef INVERT_V_DIR
  #undef V_HOME_DIR
  #undef V_MIN_POS
  #undef V_MAX_POS
  #undef MANUAL_V_HOME_POS
  #undef MIN_SOFTWARE_ENDSTOP_V
  #undef MAX_SOFTWARE_ENDSTOP_V
  #undef SAFE_BED_LEVELING_START_V
#endif

#if !HAS_W_AXIS
  #undef ENDSTOPPULLUP_WMIN
  #undef ENDSTOPPULLUP_WMAX
  #undef W_MIN_ENDSTOP_INVERTING
  #undef W_MAX_ENDSTOP_INVERTING
  #undef W_ENABLE_ON
  #undef DISABLE_W
  #undef INVERT_W_DIR
  #undef W_HOME_DIR
  #undef W_MIN_POS
  #undef W_MAX_POS
  #undef MANUAL_W_HOME_POS
  #undef MIN_SOFTWARE_ENDSTOP_W
  #undef MAX_SOFTWARE_ENDSTOP_W
  #undef SAFE_BED_LEVELING_START_W
#endif

#ifdef X2_DRIVER_TYPE
  #define HAS_X2_STEPPER 1
  // Dual X Carriage isn't known yet. TODO: Consider moving it to Configuration.h.
#endif
#ifdef Y2_DRIVER_TYPE
  #define HAS_Y2_STEPPER 1
  #define HAS_DUAL_Y_STEPPERS 1
#endif

/**
 * Number of Primary Linear Axes (e.g., XYZ)
 * X, XY, or XYZ axes. Excluding duplicate axes (X2, Y2. Z2. Z3, Z4)
 */
#if NUM_AXES >= 3
  #define PRIMARY_LINEAR_AXES 3
#else
  #define PRIMARY_LINEAR_AXES NUM_AXES
#endif

/**
 * Number of Secondary Axes (e.g., IJKUVW)
 * All linear/rotational axes between XYZ and E.
 */
#define SECONDARY_AXES SUB3(NUM_AXES)

/**
 * Number of Rotational Axes (e.g., IJK)
 * All axes for which AXIS*_ROTATES is defined.
 * For these axes, positions are specified in angular degrees.
 */
#if ENABLED(AXIS9_ROTATES)
  #define ROTATIONAL_AXES 6
#elif ENABLED(AXIS8_ROTATES)
  #define ROTATIONAL_AXES 5
#elif ENABLED(AXIS7_ROTATES)
  #define ROTATIONAL_AXES 4
#elif ENABLED(AXIS6_ROTATES)
  #define ROTATIONAL_AXES 3
#elif ENABLED(AXIS5_ROTATES)
  #define ROTATIONAL_AXES 2
#elif ENABLED(AXIS4_ROTATES)
  #define ROTATIONAL_AXES 1
#else
  #define ROTATIONAL_AXES 0
#endif

/**
 * Number of Secondary Linear Axes (e.g., UVW)
 * All secondary axes for which AXIS*_ROTATES is not defined.
 * Excluding primary axes and excluding duplicate axes (X2, Y2, Z2, Z3, Z4)
 */
#define SECONDARY_LINEAR_AXES (NUM_AXES - PRIMARY_LINEAR_AXES - ROTATIONAL_AXES)

/**
 * Number of Logical Axes (e.g., XYZIJKUVWE)
 * All logical axes that can be commanded directly by G-code.
 * Delta maps stepper-specific values to ABC steppers.
 */
#if HAS_EXTRUDERS
  #define LOGICAL_AXES INCREMENT(NUM_AXES)
#else
  #define LOGICAL_AXES NUM_AXES
#endif

/**
 * DISTINCT_E_FACTORS is set to give extruders (some) individual settings.
 *
 * DISTINCT_AXES is the number of distinct addressable axes (not steppers).
 *  Includes all linear axes plus all distinguished extruders.
 *  The default behavior is to treat all extruders as a single E axis
 *  with shared motion and temperature settings.
 *
 * DISTINCT_E is the number of distinguished extruders. By default this
 *  well be 1 which indicates all extruders share the same settings.
 *
 * E_INDEX_N(E) should be used to get the E index of any item that might be
 *  distinguished.
 */
#if ENABLED(DISTINCT_E_FACTORS) && E_STEPPERS > 1
  #define DISTINCT_AXES (NUM_AXES + E_STEPPERS)
  #define DISTINCT_E E_STEPPERS
  #define E_INDEX_N(E) (E)
  #define UNUSED_E(E) NOOP
#else
  #undef DISTINCT_E_FACTORS
  #define DISTINCT_AXES LOGICAL_AXES
  #define DISTINCT_E 1
  #define E_INDEX_N(E) 0
  #define UNUSED_E(E) UNUSED(E)
#endif
#define XYZE_N (NUM_AXES + E_STEPPERS)

#if HOTENDS
  #define HAS_HOTEND 1
  #ifndef HOTEND_OVERSHOOT
    #define HOTEND_OVERSHOOT 15
  #endif
  #if HOTENDS > 1
    #define HAS_MULTI_HOTEND 1
    #define HAS_HOTEND_OFFSET 1
  #endif
#else
  #undef PID_PARAMS_PER_HOTEND
#endif

// Helper macros for extruder and hotend arrays
#define EXTRUDER_LOOP() for (int8_t e = 0; e < EXTRUDERS; e++)
#define HOTEND_LOOP() for (int8_t e = 0; e < HOTENDS; e++)
#define ARRAY_BY_EXTRUDERS(V...) ARRAY_N(EXTRUDERS, V)
#define ARRAY_BY_EXTRUDERS1(v1) ARRAY_N_1(EXTRUDERS, v1)
#define ARRAY_BY_HOTENDS(V...) ARRAY_N(HOTENDS, V)
#define ARRAY_BY_HOTENDS1(v1) ARRAY_N_1(HOTENDS, v1)

/**
 * The BLTouch Probe emulates a servo probe
 * and uses "special" angles for its state.
 */
#if ENABLED(BLTOUCH)
  #ifndef Z_PROBE_SERVO_NR
    #define Z_PROBE_SERVO_NR 0
  #endif
  #ifndef NUM_SERVOS
    #define NUM_SERVOS (Z_PROBE_SERVO_NR + 1)
  #endif
  #undef DEACTIVATE_SERVOS_AFTER_MOVE
  #if NUM_SERVOS == 1
    #undef SERVO_DELAY
    #define SERVO_DELAY { 50 }
  #endif

  // Always disable probe pin inverting for BLTouch
  #undef Z_MIN_PROBE_ENDSTOP_INVERTING
  #define Z_MIN_PROBE_ENDSTOP_INVERTING false
  #if ENABLED(Z_MIN_PROBE_USES_Z_MIN_ENDSTOP_PIN)
    #undef Z_MIN_ENDSTOP_INVERTING
    #define Z_MIN_ENDSTOP_INVERTING false
  #endif
#endif

#ifndef PREHEAT_1_LABEL
  #define PREHEAT_1_LABEL "PLA"
#endif

#ifndef PREHEAT_2_LABEL
  #define PREHEAT_2_LABEL "ABS"
#endif

/**
 * Set a flag for a servo probe
 */
#define HAS_Z_SERVO_PROBE (defined(Z_PROBE_SERVO_NR) && Z_PROBE_SERVO_NR >= 0)

/**
 * Set flags for enabled probes
 */
#define HAS_BED_PROBE (HAS_Z_SERVO_PROBE || ANY(FIX_MOUNTED_PROBE, TOUCH_MI_PROBE, Z_PROBE_ALLEN_KEY, Z_PROBE_SLED, SOLENOID_PROBE, SENSORLESS_PROBING, RACK_AND_PINION_PROBE))
#define PROBE_SELECTED (HAS_BED_PROBE || EITHER(PROBE_MANUALLY, MESH_BED_LEVELING))

#if HAS_BED_PROBE
  #define HAS_CUSTOM_PROBE_PIN  DISABLED(Z_MIN_PROBE_USES_Z_MIN_ENDSTOP_PIN)
  #if (Z_HOME_DIR < 0 && !HAS_CUSTOM_PROBE_PIN)
    #define HOMING_Z_WITH_PROBE 1
  #endif
  #ifndef Z_PROBE_LOW_POINT
    #define Z_PROBE_LOW_POINT -5
  #endif
  #if ENABLED(Z_PROBE_ALLEN_KEY)
    #define PROBE_TRIGGERED_WHEN_STOWED_TEST // Extra test for Allen Key Probe
  #endif
  #ifdef MULTIPLE_PROBING
    #if EXTRA_PROBING
      #define TOTAL_PROBING (MULTIPLE_PROBING + EXTRA_PROBING)
    #else
      #define TOTAL_PROBING MULTIPLE_PROBING
    #endif
  #endif
#else
  // Clear probe pin settings when no probe is selected
  #undef Z_MIN_PROBE_USES_Z_MIN_ENDSTOP_PIN
#endif

#ifdef GRID_MAX_POINTS_X
  #define GRID_MAX_POINTS ((GRID_MAX_POINTS_X) * (GRID_MAX_POINTS_Y))
#endif

#define HAS_SOFTWARE_ENDSTOPS        EITHER(MIN_SOFTWARE_ENDSTOPS, MAX_SOFTWARE_ENDSTOPS)
#define HAS_RESUME_CONTINUE          ANY(EXTENSIBLE_UI, NEWPANEL, EMERGENCY_PARSER)
#define HAS_COLOR_LEDS               ANY(BLINKM, RGB_LED, RGBW_LED, PCA9632, PCA9533, NEOPIXEL_LED)
#define HAS_LEDS_OFF_FLAG            (BOTH(PRINTER_EVENT_LEDS, SDSUPPORT) && HAS_RESUME_CONTINUE)
#define HAS_PRINT_PROGRESS           EITHER(SDSUPPORT, LCD_SET_PROGRESS_MANUALLY)
#define HAS_PRINT_PROGRESS_PERMYRIAD (HAS_PRINT_PROGRESS && EITHER(PRINT_PROGRESS_SHOW_DECIMALS, SHOW_REMAINING_TIME))
#define HAS_SERVICE_INTERVALS        (ENABLED(PRINTCOUNTER) && (SERVICE_INTERVAL_1 > 0 || SERVICE_INTERVAL_2 > 0 || SERVICE_INTERVAL_3 > 0))
#define HAS_FILAMENT_SENSOR          ENABLED(FILAMENT_RUNOUT_SENSOR)

#define Z_MULTI_STEPPER_DRIVERS EITHER(Z_DUAL_STEPPER_DRIVERS, Z_TRIPLE_STEPPER_DRIVERS)
#if EITHER(Z_DUAL_ENDSTOPS, Z_TRIPLE_ENDSTOPS)
  #define Z_MULTI_ENDSTOPS 1
#endif
#define HAS_EXTRA_ENDSTOPS     (EITHER(X_DUAL_ENDSTOPS, Y_DUAL_ENDSTOPS) || Z_MULTI_ENDSTOPS)

#define HAS_GAMES     ANY(MARLIN_BRICKOUT, MARLIN_INVADERS, MARLIN_SNAKE, MARLIN_MAZE)
#define HAS_GAME_MENU (1 < ENABLED(MARLIN_BRICKOUT) + ENABLED(MARLIN_INVADERS) + ENABLED(MARLIN_SNAKE) + ENABLED(MARLIN_MAZE))

#if ENABLED(MORGAN_SCARA)
  #define IS_SCARA 1
#endif
#if (ENABLED(DELTA) || IS_SCARA)
  #define IS_KINEMATIC 1
#endif
#define IS_CARTESIAN !IS_KINEMATIC

#ifndef INVERT_X_DIR
  #define INVERT_X_DIR false
#endif
#ifndef INVERT_Y_DIR
  #define INVERT_Y_DIR false
#endif
#ifndef INVERT_Z_DIR
  #define INVERT_Z_DIR false
#endif
#ifndef INVERT_E_DIR
  #define INVERT_E_DIR false
#endif

#if ENABLED(SLIM_LCD_MENUS)
  #define BOOT_MARLIN_LOGO_SMALL
#endif

#define IS_RE_ARM_BOARD MB(RAMPS_14_RE_ARM_EFB, RAMPS_14_RE_ARM_EEB, RAMPS_14_RE_ARM_EFF, RAMPS_14_RE_ARM_EEF, RAMPS_14_RE_ARM_SF)

#define HAS_SDCARD_CONNECTION EITHER(TARGET_LPC1768, ADAFRUIT_GRAND_CENTRAL_M4)

#define HAS_LINEAR_E_JERK (DISABLED(CLASSIC_JERK) && ENABLED(LIN_ADVANCE))

#ifndef SPI_SPEED
  #define SPI_SPEED SPI_FULL_SPEED
#endif

#if X_HOME_DIR || (HAS_Y_AXIS && Y_HOME_DIR) || (HAS_Z_AXIS && Z_HOME_DIR) || (HAS_I_AXIS && I_HOME_DIR) || (HAS_J_AXIS && J_HOME_DIR) || (HAS_K_AXIS && K_HOME_DIR)
  #define HAS_ENDSTOPS 1
  #define COORDINATE_OKAY(N,L,H) WITHIN(N,L,H)
#else
  #define COORDINATE_OKAY(N,L,H) true
#endif
