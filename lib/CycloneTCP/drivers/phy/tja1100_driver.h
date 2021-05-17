/**
 * @file tja1100_driver.h
 * @brief TJA1100 100Base-T1 Ethernet PHY driver
 *
 * @section License
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2010-2021 Oryx Embedded SARL. All rights reserved.
 *
 * This file is part of CycloneTCP Open.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

#ifndef _TJA1100_DRIVER_H
#define _TJA1100_DRIVER_H

//Dependencies
#include "core/nic.h"

//PHY address
#ifndef TJA1100_PHY_ADDR
   #define TJA1100_PHY_ADDR 0
#elif (TJA1100_PHY_ADDR < 0 || TJA1100_PHY_ADDR > 31)
   #error TJA1100_PHY_ADDR parameter is not valid
#endif

//TJA1100 PHY registers
#define TJA1100_BASIC_CTRL                            0x00
#define TJA1100_BASIC_STAT                            0x01
#define TJA1100_PHY_ID1                               0x02
#define TJA1100_PHY_ID2                               0x03
#define TJA1100_EXTENDED_STAT                         0x0F
#define TJA1100_PHY_ID3                               0x10
#define TJA1100_EXTENDED_CTRL                         0x11
#define TJA1100_CONFIG1                               0x12
#define TJA1100_CONFIG2                               0x13
#define TJA1100_SYM_ERR_COUNTER                       0x14
#define TJA1100_INT_SRC                               0x15
#define TJA1100_INT_EN                                0x16
#define TJA1100_COMM_STAT                             0x17
#define TJA1100_GENERAL_STAT                          0x18
#define TJA1100_EXTERNAL_STAT                         0x19
#define TJA1100_LINK_FAIL_COUNTER                     0x1A

//Basic control register
#define TJA1100_BASIC_CTRL_RESET                      0x8000
#define TJA1100_BASIC_CTRL_LOOPBACK                   0x4000
#define TJA1100_BASIC_CTRL_SPEED_SEL_LSB              0x2000
#define TJA1100_BASIC_CTRL_AUTONEG_EN                 0x1000
#define TJA1100_BASIC_CTRL_POWER_DOWN                 0x0800
#define TJA1100_BASIC_CTRL_ISOLATE                    0x0400
#define TJA1100_BASIC_CTRL_RE_AUTONEG                 0x0200
#define TJA1100_BASIC_CTRL_DUPLEX_MODE                0x0100
#define TJA1100_BASIC_CTRL_COL_TEST                   0x0080
#define TJA1100_BASIC_CTRL_SPEED_SEL_MSB              0x0040
#define TJA1100_BASIC_CTRL_UNIDIRECT_EN               0x0020

//Basic status register
#define TJA1100_BASIC_STAT_100BT4                     0x8000
#define TJA1100_BASIC_STAT_100BTX_FD                  0x4000
#define TJA1100_BASIC_STAT_100BTX_HD                  0x2000
#define TJA1100_BASIC_STAT_10BT_FD                    0x1000
#define TJA1100_BASIC_STAT_10BT_HD                    0x0800
#define TJA1100_BASIC_STAT_100BT2_FD                  0x0400
#define TJA1100_BASIC_STAT_100BT2_HD                  0x0200
#define TJA1100_BASIC_STAT_EXTENDED_STATUS            0x0100
#define TJA1100_BASIC_STAT_UNIDIRECT_ABILITY          0x0080
#define TJA1100_BASIC_STAT_MF_PREAMBLE_SUPPR          0x0040
#define TJA1100_BASIC_STAT_AUTONEG_COMPLETE           0x0020
#define TJA1100_BASIC_STAT_REMOTE_FAULT               0x0010
#define TJA1100_BASIC_STAT_AUTONEG_ABILITY            0x0008
#define TJA1100_BASIC_STAT_LINK_STATUS                0x0004
#define TJA1100_BASIC_STAT_JABBER_DETECT              0x0002
#define TJA1100_BASIC_STAT_EXTENDED_CAPABILITY        0x0001

//PHY identification 1 register
#define TJA1100_PHY_ID1_OUI_MSB                       0xFFFF
#define TJA1100_PHY_ID1_OUI_MSB_DEFAULT               0x0180

//PHY identification 2 register
#define TJA1100_PHY_ID2_OUI_LSB                       0xFC00
#define TJA1100_PHY_ID2_OUI_LSB_DEFAULT               0xDC00
#define TJA1100_PHY_ID2_TYPE_NO                       0x03F0
#define TJA1100_PHY_ID2_TYPE_NO_DEFAULT               0x0040
#define TJA1100_PHY_ID2_REVISION_NO                   0x000F

//Extended status register
#define TJA1100_EXTENDED_STAT_1000BX_FD               0x8000
#define TJA1100_EXTENDED_STAT_1000BX_HD               0x4000
#define TJA1100_EXTENDED_STAT_1000BT_FD               0x2000
#define TJA1100_EXTENDED_STAT_1000BT_HD               0x1000
#define TJA1100_EXTENDED_STAT_100BT1                  0x0080
#define TJA1100_EXTENDED_STAT_1000BT1                 0x0040

//PHY identification 3 register
#define TJA1100_PHY_ID3_VERSION_NO                    0x00FF

//Extended control register
#define TJA1100_EXTENDED_CTRL_LINK_CONTROL            0x8000
#define TJA1100_EXTENDED_CTRL_POWER_MODE              0x7800
#define TJA1100_EXTENDED_CTRL_POWER_MODE_NO_CHANGE    0x0000
#define TJA1100_EXTENDED_CTRL_POWER_MODE_NORMAL       0x1800
#define TJA1100_EXTENDED_CTRL_POWER_MODE_SLEEP_REQ    0x5800
#define TJA1100_EXTENDED_CTRL_POWER_MODE_STANDBY      0x6000
#define TJA1100_EXTENDED_CTRL_SLAVE_JITTER_TEST       0x0400
#define TJA1100_EXTENDED_CTRL_TRAINING_RESTART        0x0200
#define TJA1100_EXTENDED_CTRL_TEST_MODE               0x01C0
#define TJA1100_EXTENDED_CTRL_TEST_MODE_0             0x0000
#define TJA1100_EXTENDED_CTRL_TEST_MODE_1             0x0040
#define TJA1100_EXTENDED_CTRL_TEST_MODE_2             0x0080
#define TJA1100_EXTENDED_CTRL_TEST_MODE_3             0x00C0
#define TJA1100_EXTENDED_CTRL_TEST_MODE_4             0x0100
#define TJA1100_EXTENDED_CTRL_TEST_MODE_5             0x0140
#define TJA1100_EXTENDED_CTRL_TEST_MODE_6             0x0180
#define TJA1100_EXTENDED_CTRL_CABLE_TEST              0x0020
#define TJA1100_EXTENDED_CTRL_LOOPBACK_MODE           0x0018
#define TJA1100_EXTENDED_CTRL_LOOPBACK_MODE_INTERNAL  0x0000
#define TJA1100_EXTENDED_CTRL_LOOPBACK_MODE_EXTERNAL  0x0008
#define TJA1100_EXTENDED_CTRL_LOOPBACK_MODE_REMOTE    0x0018
#define TJA1100_EXTENDED_CTRL_CONFIG_EN               0x0004
#define TJA1100_EXTENDED_CTRL_CONFIG_INH              0x0002
#define TJA1100_EXTENDED_CTRL_WAKE_REQUEST            0x0001

//Configuration 1 register
#define TJA1100_CONFIG1_MASTER_SLAVE                  0x8000
#define TJA1100_CONFIG1_AUTO_OP                       0x4000
#define TJA1100_CONFIG1_LINK_LENGTH                   0x2000
#define TJA1100_CONFIG1_TX_AMPLITUDE                  0x0C00
#define TJA1100_CONFIG1_TX_AMPLITUDE_500MV            0x0000
#define TJA1100_CONFIG1_TX_AMPLITUDE_750MV            0x0400
#define TJA1100_CONFIG1_TX_AMPLITUDE_1000MV           0x0800
#define TJA1100_CONFIG1_TX_AMPLITUDE_1250MV           0x0C00
#define TJA1100_CONFIG1_MII_MODE                      0x0300
#define TJA1100_CONFIG1_MII_MODE_MII                  0x0000
#define TJA1100_CONFIG1_MII_MODE_RMII_50MHZ_REFCLK_IN 0x0100
#define TJA1100_CONFIG1_MII_MODE_RMII_25MHZ_XTAL      0x0200
#define TJA1100_CONFIG1_MII_MODE_REV_MII              0x0300
#define TJA1100_CONFIG1_MII_DRIVER                    0x0080
#define TJA1100_CONFIG1_MII_DRIVER_STANDARD           0x0000
#define TJA1100_CONFIG1_MII_DRIVER_REDUCED            0x0080
#define TJA1100_CONFIG1_LED_MODE                      0x0030
#define TJA1100_CONFIG1_LED_MODE_LINK_UP              0x0000
#define TJA1100_CONFIG1_LED_MODE_FRAME_RX             0x0010
#define TJA1100_CONFIG1_LED_MODE_SYM_ERR              0x0020
#define TJA1100_CONFIG1_LED_MODE_CRS                  0x0030
#define TJA1100_CONFIG1_LED_ENABLE                    0x0008
#define TJA1100_CONFIG1_CONFIG_WAKE                   0x0004
#define TJA1100_CONFIG1_AUTO_PWD                      0x0002

//Configuration 2 register
#define TJA1100_CONFIG2_PHYAD                         0xF800
#define TJA1100_CONFIG2_SQI_AVERAGING                 0x0600
#define TJA1100_CONFIG2_SQI_AVERAGING_32_SYMBOLS      0x0000
#define TJA1100_CONFIG2_SQI_AVERAGING_64_SYMBOLS      0x0200
#define TJA1100_CONFIG2_SQI_AVERAGING_128_SYMBOLS     0x0400
#define TJA1100_CONFIG2_SQI_AVERAGING_256_SYMBOLS     0x0600
#define TJA1100_CONFIG2_SQI_WLIMIT                    0x01C0
#define TJA1100_CONFIG2_SQI_WLIMIT_NONE               0x0000
#define TJA1100_CONFIG2_SQI_WLIMIT_CLASS_A            0x0040
#define TJA1100_CONFIG2_SQI_WLIMIT_CLASS_B            0x0080
#define TJA1100_CONFIG2_SQI_WLIMIT_CLASS_C            0x00C0
#define TJA1100_CONFIG2_SQI_WLIMIT_CLASS_D            0x0100
#define TJA1100_CONFIG2_SQI_WLIMIT_CLASS_E            0x0140
#define TJA1100_CONFIG2_SQI_WLIMIT_CLASS_F            0x0180
#define TJA1100_CONFIG2_SQI_WLIMIT_CLASS_G            0x01C0
#define TJA1100_CONFIG2_SQI_FAILLIMIT                 0x0038
#define TJA1100_CONFIG2_SQI_FAILLIMIT_NONE            0x0000
#define TJA1100_CONFIG2_SQI_FAILLIMIT_CLASS_A         0x0008
#define TJA1100_CONFIG2_SQI_FAILLIMIT_CLASS_B         0x0010
#define TJA1100_CONFIG2_SQI_FAILLIMIT_CLASS_C         0x0018
#define TJA1100_CONFIG2_SQI_FAILLIMIT_CLASS_D         0x0020
#define TJA1100_CONFIG2_SQI_FAILLIMIT_CLASS_E         0x0028
#define TJA1100_CONFIG2_SQI_FAILLIMIT_CLASS_F         0x0030
#define TJA1100_CONFIG2_SQI_FAILLIMIT_CLASS_G         0x0038
#define TJA1100_CONFIG2_JUMBO_ENABLE                  0x0004
#define TJA1100_CONFIG2_SLEEP_REQUEST_TO              0x0003
#define TJA1100_CONFIG2_SLEEP_REQUEST_TO_0_4MS        0x0000
#define TJA1100_CONFIG2_SLEEP_REQUEST_TO_1MS          0x0001
#define TJA1100_CONFIG2_SLEEP_REQUEST_TO_4MS          0x0002
#define TJA1100_CONFIG2_SLEEP_REQUEST_TO_16MS         0x0003

//Symbol error counter register
#define TJA1100_SYM_ERR_COUNTER_SYM_ERR_CNT           0xFFFF

//Interrupt source register
#define TJA1100_INT_SRC_PWON                          0x8000
#define TJA1100_INT_SRC_WAKEUP                        0x4000
#define TJA1100_INT_SRC_PHY_INIT_FAIL                 0x0800
#define TJA1100_INT_SRC_LINK_STATUS_FAIL              0x0400
#define TJA1100_INT_SRC_LINK_STATUS_UP                0x0200
#define TJA1100_INT_SRC_SYM_ERR                       0x0100
#define TJA1100_INT_SRC_TRAINING_FAILED               0x0080
#define TJA1100_INT_SRC_SQI_WARNING                   0x0040
#define TJA1100_INT_SRC_CONTROL_ERR                   0x0020
#define TJA1100_INT_SRC_UV_ERR                        0x0008
#define TJA1100_INT_SRC_UV_RECOVERY                   0x0004
#define TJA1100_INT_SRC_TEMP_ERR                      0x0002
#define TJA1100_INT_SRC_SLEEP_ABORT                   0x0001

//Interrupt enable register
#define TJA1100_INT_EN_PWON                           0x8000
#define TJA1100_INT_EN_WAKEUP                         0x4000
#define TJA1100_INT_EN_PHY_INIT_FAIL                  0x0800
#define TJA1100_INT_EN_LINK_STATUS_FAIL               0x0400
#define TJA1100_INT_EN_LINK_STATUS_UP                 0x0200
#define TJA1100_INT_EN_SYM_ERR                        0x0100
#define TJA1100_INT_EN_TRAINING_FAILED                0x0080
#define TJA1100_INT_EN_SQI_WARNING                    0x0040
#define TJA1100_INT_EN_CONTROL_ERR                    0x0020
#define TJA1100_INT_EN_UV_ERR                         0x0008
#define TJA1100_INT_EN_UV_RECOVERY                    0x0004
#define TJA1100_INT_EN_TEMP_ERR                       0x0002
#define TJA1100_INT_EN_SLEEP_ABORT                    0x0001

//Communication status register
#define TJA1100_COMM_STAT_LINK_UP                     0x8000
#define TJA1100_COMM_STAT_TX_MODE                     0x6000
#define TJA1100_COMM_STAT_TX_MODE_DISABLED            0x0000
#define TJA1100_COMM_STAT_TX_MODE_SEND_N              0x2000
#define TJA1100_COMM_STAT_TX_MODE_SEND_I              0x4000
#define TJA1100_COMM_STAT_TX_MODE_SEND_Z              0x6000
#define TJA1100_COMM_STAT_LOC_RCVR_STATUS             0x1000
#define TJA1100_COMM_STAT_REM_RCVR_STATUS             0x0800
#define TJA1100_COMM_STAT_SCR_LOCKED                  0x0400
#define TJA1100_COMM_STAT_SSD_ERR                     0x0200
#define TJA1100_COMM_STAT_ESD_ERR                     0x0100
#define TJA1100_COMM_STAT_SQI                         0x00E0
#define TJA1100_COMM_STAT_SQI_WORSE_THAN_CLASS_A      0x0000
#define TJA1100_COMM_STAT_SQI_CLASS_A                 0x0020
#define TJA1100_COMM_STAT_SQI_CLASS_B                 0x0040
#define TJA1100_COMM_STAT_SQI_CLASS_C                 0x0060
#define TJA1100_COMM_STAT_SQI_CLASS_D                 0x0080
#define TJA1100_COMM_STAT_SQI_CLASS_E                 0x00A0
#define TJA1100_COMM_STAT_SQI_CLASS_F                 0x00C0
#define TJA1100_COMM_STAT_SQI_CLASS_G                 0x00E0
#define TJA1100_COMM_STAT_RECEIVE_ERR                 0x0010
#define TJA1100_COMM_STAT_TRANSMIT_ERR                0x0008
#define TJA1100_COMM_STAT_PHY_STATE                   0x0007
#define TJA1100_COMM_STAT_PHY_STATE_IDLE              0x0000
#define TJA1100_COMM_STAT_PHY_STATE_INITIALIZING      0x0001
#define TJA1100_COMM_STAT_PHY_STATE_CONFIGURED        0x0002
#define TJA1100_COMM_STAT_PHY_STATE_OFFLINE           0x0003
#define TJA1100_COMM_STAT_PHY_STATE_ACTIVE            0x0004
#define TJA1100_COMM_STAT_PHY_STATE_ISOLATE           0x0005
#define TJA1100_COMM_STAT_PHY_STATE_CABLE_TEST        0x0006
#define TJA1100_COMM_STAT_PHY_STATE_TEST_MODE         0x0007

//General status register
#define TJA1100_GENERAL_STAT_INT_STATUS               0x8000
#define TJA1100_GENERAL_STAT_PLL_LOCKED               0x4000
#define TJA1100_GENERAL_STAT_LOCAL_WU                 0x2000
#define TJA1100_GENERAL_STAT_REMOTE_WU                0x1000
#define TJA1100_GENERAL_STAT_DATA_DET_WU              0x0800
#define TJA1100_GENERAL_STAT_EN_STATUS                0x0400
#define TJA1100_GENERAL_STAT_RESET_STATUS             0x0200
#define TJA1100_GENERAL_STAT_LINKFAIL_CNT             0x00F8

//External status register
#define TJA1100_EXTERNAL_STAT_UV_VDDA_3V3             0x4000
#define TJA1100_EXTERNAL_STAT_UV_VDDD_1V8             0x2000
#define TJA1100_EXTERNAL_STAT_UV_VDDA_1V8             0x1000
#define TJA1100_EXTERNAL_STAT_UV_VDDIO                0x0800
#define TJA1100_EXTERNAL_STAT_TEMP_HIGH               0x0400
#define TJA1100_EXTERNAL_STAT_TEMP_WARN               0x0200
#define TJA1100_EXTERNAL_STAT_SHORT_DETECT            0x0100
#define TJA1100_EXTERNAL_STAT_OPEN_DETECT             0x0080
#define TJA1100_EXTERNAL_STAT_POLARITY_DETECT         0x0040
#define TJA1100_EXTERNAL_STAT_INTERLEAVE_DETECT       0x0020

//Link-fail counter register
#define TJA1100_LINK_FAIL_COUNTER_LOC_RCVR_CNT        0xFF00
#define TJA1100_LINK_FAIL_COUNTER_REM_RCVR_CNT        0x00FF

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//TJA1100 Ethernet PHY driver
extern const PhyDriver tja1100PhyDriver;

//TJA1100 related functions
error_t tja1100Init(NetInterface *interface);

void tja1100Tick(NetInterface *interface);

void tja1100EnableIrq(NetInterface *interface);
void tja1100DisableIrq(NetInterface *interface);

void tja1100EventHandler(NetInterface *interface);

void tja1100WritePhyReg(NetInterface *interface, uint8_t address,
   uint16_t data);

uint16_t tja1100ReadPhyReg(NetInterface *interface, uint8_t address);

void tja1100DumpPhyReg(NetInterface *interface);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
