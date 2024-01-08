#pragma once

static constexpr uint8_t touchscreen_gt911_config[] = {
    0x63, // 0x8047 Config_Version
    0x40, // 0x8048 X Output Max (Low Byte)
    0x01, // 0x8049 X Output Max (High Byte)
    0xe0, // 0x804A Y Output Max (Low Byte)
    0x01, // 0x804B Y Output Max (High Byte)
    0x02, // 0x804C Touch Number bit 0-3, Touch points supported: 1 to 5
    0x87, // 0x804D Module_Switch1 bit 0-1 INT triggering mechanism, 00: rising edge, 01: falling edge, 02: Low level, 03: High level
          //                       bit 2 Sito (Software noise reduction)
          //                       bit 3 X2Y (X,Y axis switch-over)
          //                       bit 4-5：Stretch_rank, stretching method, 00,01,02：Weak stretch 0.4P, 03：User-defined stretch
          //                       bit 6 driver reversal X (not reversed)
          //                       bit 7 driver reversal Y
    0x00, // 0x804E Module_Switch2
    0x01, // 0x804F Shake_Count De-jitter frequency when touch is being released De-jitter frequency when touch is pressing down
    0x08, // 0x8050 Filter First_Filter | Normal_Filter (Filter threshold for original coordinates, coefficient is 4)
    0x28, // 0x8051 Large_Touch Number of large-area touch points
    0x05, // 0x8052 Noise_Reduction Noise reduction value (0-15 valid, coefficient is 1)
    0x5a, // 0x8053 Screen_Touch_Level Threshold for touch to be detected
    0x30, // 0x8054 Screen_Leave_Level Threshold for touch to be released
    0x03, // 0x8055 Low_Power_Control Interval to enter lower power consumption mode (0s to 15s)
    0x00, // 0x8056 Refresh_Rate Pulse width setting for gesture wakeup | Coordinates report rate (period: 5+N ms)
    0x02, // 0x8057 x_threshold X coordinate output threshold: 0-255 (Based on the last reported coordinates; If configured to 0, GT911 will keep outputting coordinates continuously)
    0x02, // 0x8058 y_threshold Y coordinate output threshold: 0-255 (Based on the last reported coordinates. If configured to 0, GT911 will keep outputting coordinates continuously
    0x00, // 0x8059 X_Speed_Limit Reserved
    0x00, // 0x805A X_Speed_Limit Reserved
    0x55, // 0x805B Space Space of border top (coefficient: 32)  | Space of border bottom (coefficient: 32)
    0x00, // 0x805C Space Space of border left (coefficient: 32) | Space of border right (coefficient: 32)
    0x00, // 0x805D Mini_Filter Mini filter configuration during line drawing process, configured as 0 indicates 4
    0x18, // 0x805E Stretch_R0 coefficient of Stretch space 1
    0x1a, // 0x805F Stretch_R1 coefficient of Stretch space 2
    0x1e, // 0x8060 Stretch_R2 coefficient of Stretch space 3
    0x14, // 0x8061 Stretch_RM The base of multiple stretch spaces
    0x87, // 0x8062 Drv_GroupA_Num
    0x27, // 0x8063 Drv_GroupB_Num
    0x09, // 0x8064 Sensor_Num Sensor_Group_B_Number | Sensor_Group_A_Number
    0x2b, // 0x8065 FreqA_factor
    0x29, // 0x8066 FreqB_factor
    0x31, // 0x8067 Panel_BitFreqL
    0x0d, // 0x8068 Panel_BitFreqH
    0x00, // 0x8069 Reserved
    0x00, // 0x806A Reserved
    0x00, // 0x806B Panel_Tx_Gain
    0x02, // 0x806C Panel_Rx_Gain
    0x03, // 0x806D Panel_Dump_Shift Amplification factor of raw data (2^N) in Gesture Mode | on the touch panel
    0x25, // 0x806E Drv_Frame_Control
    0x00, // 0x806F Charging_Level_Up
    0x00, // 0x8070 Module_Switch3 bit6 Gesture_Hop_Dis | bit5 Strong_Smooth | bit0 Shape_En
    0x00, // 0x8071 GESTURE_DIS Valid distance for slide-up/down wakeup | Valid distance for slide-left/right wakeup
    0x00, // 0x8072 Gesture_Long_Press_Time The gesture recognizing processing aborting time period when long touching
    0x00, // 0x8073 X/Y_Slope_Adjust
    0x03, // 0x8074 Gesture_Control
    0x64, // 0x8075 Gesture_Switch1
    0x32, // 0x8076 Gesture_Switch2
    0x00, // 0x8077 Gesture_Refresh_Rate Report rate in Gesture mode (period is 5+ms)
    0x00, // 0x8078 Gesture_Touch_Level Touch threshold in Gesture mode
    0x00, // 0x8079 NewGreenWake UpLevel Threshold for NewGreen wakeup of Gesture wakeup function
    0x14, // 0x807A Freq_Hopping_Start
    0x58, // 0x807B Freq_Hopping_End
    0x94, // 0x807C Noise_Detect_Times
    0xc5, // 0x807D Hopping_Flag
    0x02, // 0x807E Hopping_Threshold
    0x07, // 0x807F Noise_Threshold
    0x00, // 0x8080 Noise_Min_Threshold
    0x00, // 0x8081 NC
    0x04, // 0x8082 Hopping_Sensor_Group Sections for Hopping Frequency Noise Detection (4 sections recommended)
    0x95, // 0x8083 Hopping_seg1_Normalize Seg1 Normalize coefficient ( sampling value *N / 128= Raw data)
    0x17, // 0x8084 Hopping_seg1_Factor Seg1 Central point Factor
    0x00, // 0x8085 Main_Clock_Adjust Fine adjustment of IC main clock Frequency, within the range of -7 to +8
    0x7f, // 0x8086 Hopping_seg2_Normalize Seg2 Normalize coefficient (sampling value *N / 128= Raw data)
    0x1f, // 0x8087 Hopping_seg2_Factor Seg2 Central point Factor
    0x00, // 0x8088 NC
    0x71, // 0x8089 Hopping_seg3_Normalize Seg3 Normalize coefficient (sampling value *N / 128= Raw data)
    0x2a, // 0x808A Hopping_seg3_Factor Seg3 Central point Factor
    0x00, // 0x808B NC
    0x62, // 0x808C Hopping_seg4_Normalize Seg4 Normalize coefficient (sampling value *N / 128= Raw data)
    0x39, // 0x808D Hopping_seg4_Factor Seg4 Central point Factor
    0x00, // 0x808E NC
    0x58, // 0x808F Hopping_seg5_Normalize Seg5 Normalize coefficient (sampling value *N / 128= Raw data)
    0x4c, // 0x8090 Hopping_seg5_Factor Seg5 Central point Factor
    0x00, // 0x8091 NC
    0x58, // 0x8092 Hopping_seg6_Normalize Seg6 Normalize coefficient (sampling value *N / 128= Raw data)
    0x00, // 0x8093 Key 1 address
    0x00, // 0x8094 Key 2 address
    0x00, // 0x8095 Key 3 address
    0x00, // 0x8096 Key 4 address
    0x00, // 0x8097 Key_Area
    0x00, // 0x8098 Key_Touch_Level Touch key touch threshold
    0x00, // 0x8099 Key_Leave_Level Touch key release threshold
    0x00, // 0x809A Key_Sens KeySens_1(sensitivity coefficient of Key 1) KeySens_2 (sensitivity coefficient of Key 2)
    0x00, // 0x809B Key_Sens KeySens_3(sensitivity coefficient of Key 3) KeySens_4 (sensitivity coefficient of Key 4)
    0x00, // 0x809C Key_Restrain
    0x00, // 0x809D Key_Restrain_Time
    0x00, // 0x809E GESTURE_LARGE_TOUCH
    0x00, // 0x809F NC
    0x00, // 0x80A0 NC
    0x00, // 0x80A1 Hotknot_Noise_Map 200K 250K 300K 350K 400K 450K
    0x00, // 0x80A2 Link_Threshold Link_NoiseThreshold
    0x00, // 0x80A3 Pxy_Threshold Pxy_NoiseThreshold
    0x00, // 0x80A4 GHot_Dump_Shift Rx_Self | Amplification factor of raw Data (2N)
    0x00, // 0x80A5 GHot_Rx_Gain
    0x00, // 0x80A6 Freq_Gain0
    0x00, // 0x80A7 Freq_Gain1
    0x00, // 0x80A8 Freq_Gain2
    0x00, // 0x80A9 Freq_Gain3
    // 0x80AA - 0x80B2 NC
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, // 0x80B3 Combine_Dis Distance for adjacent rectangles to be combined in Gesture mode Distance for adjacent rectangles to be combined
    0x00, // 0x80B4 Split_Set Distance for a large-area rectangle to be split Distance for a normal-size rectangle to be split
    0x00, // 0x80B5 NC
    0x00, // 0x80B6 NC
    // 0x80B7 - 0x80C4 Sensor_CH0 to Sensor_CH13 Channel number on chip corresponding to ITO Sensor
    0x12, 0x10, 0x0e, 0x0c, 0x0a, 0x08, 0x06, 0x04, 0x02, 0xff, 0xff, 0xff, 0xff, 0xff,
    // 0x80C5 - 0x80D4 NC
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // 0x80D5 - 0x80EE Driver_CH0 to Driver_CH25 Channel number on chip corresponding to ITO Driver
    0x00, 0x02, 0x04, 0x06, 0x08, 0x0a, 0x0c, 0x24, 0x22, 0x21, 0x20, 0x1f, 0x1e, 0x1d, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    // 0x80EF - 0x80FE NC
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xaf, // 0x80FF Configuration verification (checksum value of the bytes from 0x8047 to 0x80FE)
    0x01 // 0x8100 Configuration updated flag (the flag is written by the host)
};
