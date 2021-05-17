/**
 * @file lan8770_driver.h
 * @brief LAN8770 Ethernet PHY driver
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

#ifndef _LAN8770_DRIVER_H
#define _LAN8770_DRIVER_H

//Dependencies
#include "core/nic.h"

//PHY address
#ifndef LAN8770_PHY_ADDR
   #define LAN8770_PHY_ADDR 4
#elif (LAN8770_PHY_ADDR < 0 || LAN8770_PHY_ADDR > 31)
   #error LAN8770_PHY_ADDR parameter is not valid
#endif

//LAN8770 PHY registers
#define LAN8770_BASIC_CONTROL                          0x00
#define LAN8770_BASIC_STATUS                           0x01
#define LAN8770_PHY_ID1                                0x02
#define LAN8770_PHY_ID2                                0x03
#define LAN8770_MASTER_SLAVE_CONTROL                   0x09
#define LAN8770_MASTER_SLAVE_STATUS                    0x0A
#define LAN8770_MDIO_CONTROL2                          0x10
#define LAN8770_MODE_STATUS                            0x11
#define LAN8770_EXT_REG_CTL                            0x14
#define LAN8770_EXT_REG_RD_DATA                        0x15
#define LAN8770_EXT_REG_WR_DATA                        0x16
#define LAN8770_PCS_CONTROL                            0x17
#define LAN8770_INTERRUPT_SOURCE                       0x18
#define LAN8770_INTERRUPT_MASK                         0x19
#define LAN8770_POWER_DOWN_CONTROL                     0x1A
#define LAN8770_PCS_RX_ERR_CNT_STS                     0x1E

//LAN8770 MISC registers (bank 1)
#define LAN8770_REV_ID                                 0x01, 0x01
#define LAN8770_INTERRUPT2_SOURCE                      0x01, 0x08
#define LAN8770_INTERRUPT2_MASK                        0x01, 0x09
#define LAN8770_CTRL_0                                 0x01, 0x10
#define LAN8770_CTRL_1                                 0x01, 0x11
#define LAN8770_CTRL_2                                 0x01, 0x15
#define LAN8770_CTRL_3                                 0x01, 0x17
#define LAN8770_DCQ_ERR_MAX                            0x01, 0x1A
#define LAN8770_WKP_COM_CTL0                           0x01, 0x20
#define LAN8770_WKP_COM_CTL1                           0x01, 0x21
#define LAN8770_WKP_DBG_STS                            0x01, 0x22
#define LAN8770_WKP_PRT_CTL                            0x01, 0x24

//LAN8770 PCS registers (bank 2)
#define LAN8770_SLEEP_WAKE_DET                         0x02, 0x20

//LAN8770 AFE registers (bank 3)
#define LAN8770_AFE_PORT_CFG1                          0x03, 0x0B

//LAN8770 DSP registers (bank 4)
#define LAN8770_COEF_CLK_PWR_DN_CFG                    0x04, 0x04
#define LAN8770_COEF_RW_CTL_CFG                        0x04, 0x0D
#define LAN8770_DCQ_CONFIG1                            0x04, 0x2E
#define LAN8770_DCQ_CONFIG2                            0x04, 0x4A
#define LAN8770_SQI_SQU_MEAN_LSB                       0x04, 0x82
#define LAN8770_SQI_SQU_MEAN_MSB                       0x04, 0x83
#define LAN8770_SQI_TBL0                               0x04, 0xBA
#define LAN8770_SQI_TBL1                               0x04, 0xBB
#define LAN8770_SQI_TBL2                               0x04, 0xBC
#define LAN8770_SQI_TBL3                               0x04, 0xBD
#define LAN8770_SQI_TBL4                               0x04, 0xBE
#define LAN8770_SQI_TBL5                               0x04, 0xBF
#define LAN8770_SQI_TBL6                               0x04, 0xC0
#define LAN8770_DCQ_MSE                                0x04, 0xC1
#define LAN8770_DCQ_MSE_WC                             0x04, 0xC2
#define LAN8770_DCQ_SQI                                0x04, 0xC3
#define LAN8770_DCQ_PMSE                               0x04, 0xC4
#define LAN8770_BER_RATE_CNT                           0x04, 0xF3
#define LAN8770_BER_RATE_WIN_TOG                       0x04, 0xF4

//Basic Control register
#define LAN8770_BASIC_CONTROL_RESET                    0x8000
#define LAN8770_BASIC_CONTROL_LOOPBACK                 0x4000
#define LAN8770_BASIC_CONTROL_SPEED_SEL_LSB            0x2000
#define LAN8770_BASIC_CONTROL_AN_EN                    0x1000
#define LAN8770_BASIC_CONTROL_POWER_DOWN               0x0800
#define LAN8770_BASIC_CONTROL_ISOLATE                  0x0400
#define LAN8770_BASIC_CONTROL_RESTART_AN               0x0200
#define LAN8770_BASIC_CONTROL_DUPLEX_MODE              0x0100
#define LAN8770_BASIC_CONTROL_COL_TEST                 0x0080
#define LAN8770_BASIC_CONTROL_SPEED_SEL_MSB            0x0040
#define LAN8770_BASIC_CONTROL_UNIDIRECTIONAL_EN        0x0020

//Basic Status register
#define LAN8770_BASIC_STATUS_100BT4                    0x8000
#define LAN8770_BASIC_STATUS_100BTX_FD                 0x4000
#define LAN8770_BASIC_STATUS_100BTX_HD                 0x2000
#define LAN8770_BASIC_STATUS_10BT_FD                   0x1000
#define LAN8770_BASIC_STATUS_10BT_HD                   0x0800
#define LAN8770_BASIC_STATUS_100BT2_FD                 0x0400
#define LAN8770_BASIC_STATUS_100BT2_HD                 0x0200
#define LAN8770_BASIC_STATUS_EXTENDED_STATUS           0x0100
#define LAN8770_BASIC_STATUS_UNIDIRECTIONAL_EN         0x0080
#define LAN8770_BASIC_STATUS_MF_PREAMBLE_SUPPR         0x0040
#define LAN8770_BASIC_STATUS_AN_COMPLETE               0x0020
#define LAN8770_BASIC_STATUS_REMOTE_FAULT              0x0010
#define LAN8770_BASIC_STATUS_AN_CAPABLE                0x0008
#define LAN8770_BASIC_STATUS_LINK_STATUS               0x0004
#define LAN8770_BASIC_STATUS_JABBER_DETECT             0x0002
#define LAN8770_BASIC_STATUS_EXTENDED_CAPABLE          0x0001

//PHY Identifier 1 register
#define LAN8770_PHY_ID1_PHY_ID_MSB                     0xFFFF
#define LAN8770_PHY_ID1_PHY_ID_MSB_DEFAULT             0x0007

//PHY Identifier 2 register
#define LAN8770_PHY_ID2_PHY_ID_LSB                     0xFC00
#define LAN8770_PHY_ID2_PHY_ID_LSB_DEFAULT             0xC000
#define LAN8770_PHY_ID2_MODEL_NUM                      0x03F0
#define LAN8770_PHY_ID2_MODEL_NUM_DEFAULT              0x0150
#define LAN8770_PHY_ID2_REVISION_NUM                   0x000F

//Master/Slave Control register
#define LAN8770_MASTER_SLAVE_CONTROL_TEST_MODE         0xE000
#define LAN8770_MASTER_SLAVE_CONTROL_MS_MAN_CONF_EN    0x1000
#define LAN8770_MASTER_SLAVE_CONTROL_MS_MAN_CONF_VAL   0x0800
#define LAN8770_MASTER_SLAVE_CONTROL_PORT_TYPE         0x0400
#define LAN8770_MASTER_SLAVE_CONTROL_1000BT_FD         0x0200
#define LAN8770_MASTER_SLAVE_CONTROL_1000BT_HD         0x0100

//Master/Slave Status register
#define LAN8770_MASTER_SLAVE_STATUS_MS_CONF_FAULT      0x8000
#define LAN8770_MASTER_SLAVE_STATUS_MS_CONF_RES        0x4000
#define LAN8770_MASTER_SLAVE_STATUS_LOC_RCVR_STATUS    0x2000
#define LAN8770_MASTER_SLAVE_STATUS_REM_RCVR_STATUS    0x1000
#define LAN8770_MASTER_SLAVE_STATUS_LP_1000BT_FD       0x0800
#define LAN8770_MASTER_SLAVE_STATUS_LP_1000BT_HD       0x0400
#define LAN8770_MASTER_SLAVE_STATUS_IDLE_ERR_COUNT     0x00FF

//MDIO Control 2 register
#define LAN8770_MDIO_CONTROL2_WAKE_REQ                 0x2000
#define LAN8770_MDIO_CONTROL2_SLEEP_REQ                0x1000

//Mode Status register
#define LAN8770_MODE_STATUS_ENERGY_STATUS              0x0040
#define LAN8770_MODE_STATUS_DSCR_LOCK_STATUS           0x0008
#define LAN8770_MODE_STATUS_LINK_UP                    0x0001

//External Register Control register
#define LAN8770_EXT_REG_CTL_READ_CONTROL               0x1000
#define LAN8770_EXT_REG_CTL_WRITE_CONTROL              0x0800
#define LAN8770_EXT_REG_CTL_REGISTER_BANK              0x0700
#define LAN8770_EXT_REG_CTL_REGISTER_BANK_MISC         0x0100
#define LAN8770_EXT_REG_CTL_REGISTER_BANK_PCS          0x0200
#define LAN8770_EXT_REG_CTL_REGISTER_BANK_AFE          0x0300
#define LAN8770_EXT_REG_CTL_REGISTER_BANK_DSP          0x0400
#define LAN8770_EXT_REG_CTL_REGISTER_ADDR              0x00FF

//PCS Control register
#define LAN8770_PCS_CONTROL_POL_FLIP_RX_CTRL           0x0200
#define LAN8770_PCS_CONTROL_POL_FLIP_TX_CTRL           0x0100
#define LAN8770_PCS_CONTROL_POL_FLIP_MAN_CTRL          0x0080

//Interrupt Source register
#define LAN8770_INTERRUPT_SOURCE_SOFT                  0x8000
#define LAN8770_INTERRUPT_SOURCE_WAKE_IN               0x4000
#define LAN8770_INTERRUPT_SOURCE_BER_TOGGLE            0x2000
#define LAN8770_INTERRUPT_SOURCE_DCQ_ERROR             0x1000
#define LAN8770_INTERRUPT_SOURCE_OVER_TEMP_ERROR       0x0800
#define LAN8770_INTERRUPT_SOURCE_RECEIVE_WAKE          0x0400
#define LAN8770_INTERRUPT_SOURCE_ENERGY_OFF            0x0040
#define LAN8770_INTERRUPT_SOURCE_RECEIVE_LPS           0x0020
#define LAN8770_INTERRUPT_SOURCE_JABBER_DETECT         0x0008
#define LAN8770_INTERRUPT_SOURCE_LINK_UP               0x0004
#define LAN8770_INTERRUPT_SOURCE_LINK_DOWN             0x0002
#define LAN8770_INTERRUPT_SOURCE_ENERGY_ON             0x0001

//Interrupt Mask register
#define LAN8770_INTERRUPT_MASK_SOFT                    0x8000
#define LAN8770_INTERRUPT_MASK_WAKE_IN                 0x4000
#define LAN8770_INTERRUPT_MASK_BER_TOGGLE              0x2000
#define LAN8770_INTERRUPT_MASK_DCQ_ERROR               0x1000
#define LAN8770_INTERRUPT_MASK_OVER_TEMP_ERROR         0x0800
#define LAN8770_INTERRUPT_MASK_RECEIVE_WAKE            0x0400
#define LAN8770_INTERRUPT_MASK_ENERGY_OFF              0x0040
#define LAN8770_INTERRUPT_MASK_RECEIVE_LPS             0x0020
#define LAN8770_INTERRUPT_MASK_JABBER_DETECT           0x0008
#define LAN8770_INTERRUPT_MASK_LINK_UP                 0x0004
#define LAN8770_INTERRUPT_MASK_LINK_DOWN               0x0002
#define LAN8770_INTERRUPT_MASK_ENERGY_ON               0x0001

//Power Down Control register
#define LAN8770_POWER_DOWN_CONTROL_HARD_INIT_SEQ_EN    0x0100

//PCS Receive Error Count Status register
#define LAN8770_PCS_RX_ERR_CNT_STS_PCS_RX_ERR_CNT      0xFFFF

//Revision and ID register
#define LAN8770_REV_ID_REVISION                        0x00FF

//Interrupt 2 Source register
#define LAN8770_INTERRUPT2_SOURCE_POR_RDY_STS          0x0800
#define LAN8770_INTERRUPT2_SOURCE_COMM_READY           0x0400
#define LAN8770_INTERRUPT2_SOURCE_REM_RCVR_STS_DOWN    0x0080
#define LAN8770_INTERRUPT2_SOURCE_REM_RCVR_STS_UP      0x0040
#define LAN8770_INTERRUPT2_SOURCE_LOC_RCVR_STS_DOWN    0x0020
#define LAN8770_INTERRUPT2_SOURCE_LOC_RCVR_STS_UP      0x0010
#define LAN8770_INTERRUPT2_SOURCE_DSP_CBL_DIAG_DONE    0x0008
#define LAN8770_INTERRUPT2_SOURCE_HW_INIT_DONE         0x0004

//Interrupt 2 Mask register
#define LAN8770_INTERRUPT2_MASK_POR_RDY                0x0800
#define LAN8770_INTERRUPT2_MASK_COMM_READY             0x0400
#define LAN8770_INTERRUPT2_MASK_REM_RCVR_STS_DOWN      0x0080
#define LAN8770_INTERRUPT2_MASK_REM_RCVR_STS_UP        0x0040
#define LAN8770_INTERRUPT2_MASK_LOC_RCVR_STS_DOWN      0x0020
#define LAN8770_INTERRUPT2_MASK_LOC_RCVR_STS_UP        0x0010
#define LAN8770_INTERRUPT2_MASK_DSP_CBL_DIAG           0x0008
#define LAN8770_INTERRUPT2_MASK_HW_INIT_DONE           0x0004

//Control 0 register
#define LAN8770_CTRL_0_REG_OFF_CONF_STRAP              0x0400
#define LAN8770_CTRL_0_PHYAD1_CONF_STRAP               0x0200
#define LAN8770_CTRL_0_PHYAD0_CONF_STRAP               0x0100
#define LAN8770_CTRL_0_MODE3_CONF_STRAP                0x0080
#define LAN8770_CTRL_0_MODE2_CONF_STRAP                0x0040
#define LAN8770_CTRL_0_MODE1_CONF_STRAP                0x0020
#define LAN8770_CTRL_0_MODE0_CONF_STRAP                0x0010
#define LAN8770_CTRL_0_LED_EN                          0x0008
#define LAN8770_CTRL_0_LED_MODE                        0x0007
#define LAN8770_CTRL_0_LED_MODE_LINK_UP                0x0000
#define LAN8770_CTRL_0_LED_MODE_LINK_UP_REM_STATUS     0x0001
#define LAN8770_CTRL_0_LED_MODE_LINK_UP_LOC_STATUS     0x0002

//Control 1 register
#define LAN8770_CTRL_1_CLK125_EDGE_MODE                0x8000
#define LAN8770_CTRL_1_RGMII_TXC_DELAY_EN              0x4000
#define LAN8770_CTRL_1_RGMII_RXC_DELAY_EN              0x2000
#define LAN8770_CTRL_1_BROADCAST_ADDR_EN               0x1000
#define LAN8770_CTRL_1_MISC_DRIVER                     0x0C00
#define LAN8770_CTRL_1_MISC_DRIVER_2MA                 0x0000
#define LAN8770_CTRL_1_MISC_DRIVER_4MA                 0x0400
#define LAN8770_CTRL_1_MISC_DRIVER_8MA                 0x0800
#define LAN8770_CTRL_1_MISC_DRIVER_10MA                0x0C00
#define LAN8770_CTRL_1_MII_DRIVER                      0x0300
#define LAN8770_CTRL_1_MII_DRIVER_2MA                  0x0000
#define LAN8770_CTRL_1_MII_DRIVER_4MA                  0x0100
#define LAN8770_CTRL_1_MII_DRIVER_8MA                  0x0200
#define LAN8770_CTRL_1_MII_DRIVER_10MA                 0x0300
#define LAN8770_CTRL_1_MII_EXT_LOOPBACK_EN             0x0008
#define LAN8770_CTRL_1_RMII_EXT_LOOPBACK_EN            0x0004
#define LAN8770_CTRL_1_PMA_LOOPBACK_EN                 0x0002

//Control 2 register
#define LAN8770_CTRL_2_CLK125_EN                       0x8000
#define LAN8770_CTRL_2_XMII_MODE                       0x0380
#define LAN8770_CTRL_2_XMII_MODE_MII                   0x0000
#define LAN8770_CTRL_2_XMII_MODE_RMII_REFCLK_IN        0x0080
#define LAN8770_CTRL_2_XMII_MODE_RMII_REFCLK_OUT       0x0100
#define LAN8770_CTRL_2_XMII_MODE_REVERSE_MII           0x0180
#define LAN8770_CTRL_2_XMII_MODE_RGMII                 0x0200
#define LAN8770_CTRL_2_DCQ_INT_SEL                     0x0007
#define LAN8770_CTRL_2_DCQ_INT_SEL_DCQ_SQI_METHOD_B    0x0000
#define LAN8770_CTRL_2_DCQ_INT_SEL_DCQ_MSE             0x0001
#define LAN8770_CTRL_2_DCQ_INT_SEL_DCQ_SQI_METHOD_A    0x0002
#define LAN8770_CTRL_2_DCQ_INT_SEL_DCQ_PMSE            0x0003

//Control 3 register
#define LAN8770_CTRL_3_OVER_TEMP_DETECT_STATUS         0x0200
#define LAN8770_CTRL_3_TSD_ADJ                         0x001C
#define LAN8770_CTRL_3_OVER_TEMP_EN                    0x0001

//Wakeup Common Control 0 register
#define LAN8770_WKP_COM_CTL0_VBAT_COMMON_REG_WRITE     0x8000
#define LAN8770_WKP_COM_CTL0_RING_OSC_STATUS           0x0040
#define LAN8770_WKP_COM_CTL0_RING_OSC_EN               0x0020
#define LAN8770_WKP_COM_CTL0_WAKE_DEBOUNCE_UNITS       0x0008
#define LAN8770_WKP_COM_CTL0_SLEEP_EN                  0x0004
#define LAN8770_WKP_COM_CTL0_INH_EN                    0x0002
#define LAN8770_WKP_COM_CTL0_WAKE_IN_EN                0x0001

//Wakeup Common Control 1 register
#define LAN8770_WKP_COM_CTL1_WAKE_IN_DEBOUNCE_VALUE    0xFF00

//Wakeup Debug Status register
#define LAN8770_WKP_DBG_STS_TC10_STATE                 0x0007
#define LAN8770_WKP_DBG_STS_TC10_STATE_START           0x0000
#define LAN8770_WKP_DBG_STS_TC10_STATE_NORMAL          0x0001
#define LAN8770_WKP_DBG_STS_TC10_STATE_SLEEP_ACK       0x0002
#define LAN8770_WKP_DBG_STS_TC10_STATE_SLEEP_REQ       0x0003
#define LAN8770_WKP_DBG_STS_TC10_STATE_SLEEP_FAIL      0x0004
#define LAN8770_WKP_DBG_STS_TC10_STATE_SLEEP_SILENT    0x0005
#define LAN8770_WKP_DBG_STS_TC10_STATE_SLEEP           0x0006

//Wakeup Port Control register
#define LAN8770_WKP_PRT_CTL_VBAT_PORT_REG_WRITE        0x8000
#define LAN8770_WKP_PRT_CTL_WUP_AUTO_FORWARD_EN        0x1000
#define LAN8770_WKP_PRT_CTL_WAKEUP_DEBOUNCE_UNITS      0x0800
#define LAN8770_WKP_PRT_CTL_SD_TUNE                    0x0600
#define LAN8770_WKP_PRT_CTL_MDI_WAKE_EN                0x0100
#define LAN8770_WKP_PRT_CTL_WUP_DEBOUNCE_VALUE         0x00FF

//Sleep Wake Detect register
#define LAN8770_SLEEP_WAKE_DET_WUR_DETECT_LEN          0xFF00
#define LAN8770_SLEEP_WAKE_DET_WUR_DETECT_LEN_DEFAULT  0x2800
#define LAN8770_SLEEP_WAKE_DET_LPS_DETECT_LEN          0x00FF
#define LAN8770_SLEEP_WAKE_DET_LPS_DETECT_LEN_DEFAULT  0x003C

//AFE Port Configuration 1 register
#define LAN8770_AFE_PORT_CFG1_TX_PD                    0x0020
#define LAN8770_AFE_PORT_CFG1_TX_AMP                   0x001E
#define LAN8770_AFE_PORT_CFG1_TX_AMP_DEFAULT           0x000A
#define LAN8770_AFE_PORT_CFG1_TX_SRC                   0x0001

//Coefficient Clock Power Down Configuration register
#define LAN8770_COEF_CLK_PWR_DN_CFG_COEF_CLK_PWR_DN_EN 0x0001

//Coefficient Control Configuration register
#define LAN8770_COEF_RW_CTL_CFG_TAP_EN                 0x0020
#define LAN8770_COEF_RW_CTL_CFG_TAP_DIS                0x0010
#define LAN8770_COEF_RW_CTL_CFG_OVERRIDE_COEF_DIS      0x0008
#define LAN8770_COEF_RW_CTL_CFG_OVERRIDE_COEF_EN       0x0004
#define LAN8770_COEF_RW_CTL_CFG_POKE_COEF_EN           0x0002
#define LAN8770_COEF_RW_CTL_CFG_PEAK_COEF_EN           0x0001

// DCQ				Configuration 1 register
#define LAN8770_DCQ_CONFIG1_PMSE_SCALING_FACTOR        0xC000
#define LAN8770_DCQ_CONFIG1_PMSE_KP_FACTOR             0x3C00
#define LAN8770_DCQ_CONFIG1_MSE_SCALING_FACTOR         0x0300
#define LAN8770_DCQ_CONFIG1_DCQ_RESET                  0x0080
#define LAN8770_DCQ_CONFIG1_SQI_METHOD_B_MODE_SEL      0x0040
#define LAN8770_DCQ_CONFIG1_SQI_METHOD_B_EN            0x0020
#define LAN8770_DCQ_CONFIG1_SQI_KP                     0x001F

// DCQ				Configuration 2 register
#define LAN8770_DCQ_CONFIG2_DCQ_PMSE_EN                0x0002
#define LAN8770_DCQ_CONFIG2_DCQ_MSE_SQI_METHOD_A_EN    0x0001

//SQI Method A Table 0 register
#define LAN8770_SQI_TBL0_SQI_METHOD_A_VALUE            0x01FF

//SQI Method A Table 1 register
#define LAN8770_SQI_TBL1_SQI_METHOD_A_VALUE            0x01FF

//SQI Method A Table 2 register
#define LAN8770_SQI_TBL2_SQI_METHOD_A_VALUE            0x01FF

//SQI Method A Table 3 register
#define LAN8770_SQI_TBL3_SQI_METHOD_A_VALUE            0x01FF

//SQI Method A Table 4 register
#define LAN8770_SQI_TBL4_SQI_METHOD_A_VALUE            0x01FF

//SQI Method A Table 5 register
#define LAN8770_SQI_TBL5_SQI_METHOD_A_VALUE            0x01FF

//SQI Method A Table 6 register
#define LAN8770_SQI_TBL6_SQI_METHOD_A_VALUE            0x01FF

//DCQ Mean Square Error register
#define LAN8770_DCQ_MSE_MSE_VALUE_VALID                0x0200
#define LAN8770_DCQ_MSE_MSE_VALUE                      0x01FF

//DCQ Mean Square Error Worst Case register
#define LAN8770_DCQ_MSE_WC_MSE_WORST_CASE_VALUE_VALID  0x0200
#define LAN8770_DCQ_MSE_WC_MSE_WORST_CASE_VALUE        0x01FF

//DCQ SQI Method A register
#define LAN8770_DCQ_SQI_DCQ_SQI_METHOD_A_WORST_CASE    0x00E0
#define LAN8770_DCQ_SQI_DCQ_SQI_METHOD_A_VALUE         0x000E

//DCQ Peak MSE register
#define LAN8770_DCQ_PMSE_PEAK_MSE_WORST_CASE           0xFF00
#define LAN8770_DCQ_PMSE_PEAK_MSE_VALUE                0x00FF

//DCQ Peak MSE register
#define LAN8770_BER_RATE_WIN_TOG_BER_WINDOW_TOGGLE     0x0001

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//LAN8770 Ethernet PHY driver
extern const PhyDriver lan8770PhyDriver;

//LAN8770 related functions
error_t lan8770Init(NetInterface *interface);

void lan8770Tick(NetInterface *interface);

void lan8770EnableIrq(NetInterface *interface);
void lan8770DisableIrq(NetInterface *interface);

void lan8770EventHandler(NetInterface *interface);

void lan8770WritePhyReg(NetInterface *interface, uint8_t address,
   uint16_t data);

uint16_t lan8770ReadPhyReg(NetInterface *interface, uint8_t address);

void lan8770DumpPhyReg(NetInterface *interface);

void lan8770WriteExtReg(NetInterface *interface, uint8_t bank,
   uint8_t addr, uint16_t data);

uint16_t lan8770ReadExtReg(NetInterface *interface, uint8_t bank,
   uint8_t addr);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
