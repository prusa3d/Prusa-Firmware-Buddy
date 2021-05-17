/**
 * @file st802rt1a_driver.h
 * @brief ST802RT1A Ethernet PHY driver
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

#ifndef _ST802RT1A_DRIVER_H
#define _ST802RT1A_DRIVER_H

//Dependencies
#include "core/nic.h"

//PHY address
#define ST802RT1A_PHY_ADDR 1

//ST802RT1A PHY registers
#define ST802RT1A_CNTRL                    0x00
#define ST802RT1A_STATS                    0x01
#define ST802RT1A_PHYID1                   0x02
#define ST802RT1A_PHYID2                   0x03
#define ST802RT1A_LDADV                    0x04
#define ST802RT1A_LPADV                    0x05
#define ST802RT1A_ANEGX                    0x06
#define ST802RT1A_LDNPG                    0x07
#define ST802RT1A_LPNPG                    0x08
#define ST802RT1A_XCNTL                    0x10
#define ST802RT1A_XSTAT                    0x11
#define ST802RT1A_XRCNT                    0x12
#define ST802RT1A_XCCNT                    0x13
#define ST802RT1A_XDCNT                    0x14
#define ST802RT1A_AUXCS                    0x18
#define ST802RT1A_AUXSS                    0x19
#define ST802RT1A_AUXM2                    0x1B
#define ST802RT1A_TSTAT                    0x1C
#define ST802RT1A_AMPHY                    0x1E
#define ST802RT1A_BTEST                    0x1F

//Control register
#define ST802RT1A_CNTRL_RESET              0x8000
#define ST802RT1A_CNTRL_LOOPBACK           0x4000
#define ST802RT1A_CNTRL_SPEED_SEL          0x2000
#define ST802RT1A_CNTRL_AN_EN              0x1000
#define ST802RT1A_CNTRL_POWER_DOWN         0x0800
#define ST802RT1A_CNTRL_ISOLATE            0x0400
#define ST802RT1A_CNTRL_RESTART_AN         0x0200
#define ST802RT1A_CNTRL_DUPLEX_MODE        0x0100
#define ST802RT1A_CNTRL_COL_TEST           0x0080

//Status register
#define ST802RT1A_STATS_100BT4             0x8000
#define ST802RT1A_STATS_100BTX_FD          0x4000
#define ST802RT1A_STATS_100BTX_HD          0x2000
#define ST802RT1A_STATS_10BT_FD            0x1000
#define ST802RT1A_STATS_10BT_HD            0x0800
#define ST802RT1A_STATS_MF_PREAMBLE_SUPPR  0x0040
#define ST802RT1A_STATS_AN_COMPLETE        0x0020
#define ST802RT1A_STATS_REMOTE_FAULT       0x0010
#define ST802RT1A_STATS_AN_CAPABLE         0x0008
#define ST802RT1A_STATS_LINK_STATUS        0x0004
#define ST802RT1A_STATS_JABBER_DETECT      0x0002
#define ST802RT1A_STATS_EXTENDED_CAPABLE   0x0001

//PHY identifier Hi register
#define ST802RT1A_PHYID1_OUI_MSB           0xFFFF
#define ST802RT1A_PHYID1_OUI_MSB_DEFAULT   0x0203

//PHY identifier Lo register
#define ST802RT1A_PHYID2_OUI_LSB           0xFC00
#define ST802RT1A_PHYID2_OUI_LSB_DEFAULT   0x8400
#define ST802RT1A_PHYID2_MODEL_NUM         0x03F0
#define ST802RT1A_PHYID2_MODEL_NUM_DEFAULT 0x0060
#define ST802RT1A_PHYID2_REVISION_NUM      0x000F

//Auto-negotiation advertisement register
#define ST802RT1A_LDADV_NEXT_PAGE          0x8000
#define ST802RT1A_LDADV_REMOTE_FAULT       0x2000
#define ST802RT1A_LDADV_ASYM_PAUSE         0x0800
#define ST802RT1A_LDADV_PAUSE              0x0400
#define ST802RT1A_LDADV_100BT4             0x0200
#define ST802RT1A_LDADV_100BTX_FD          0x0100
#define ST802RT1A_LDADV_100BTX_HD          0x0080
#define ST802RT1A_LDADV_10BT_FD            0x0040
#define ST802RT1A_LDADV_10BT_HD            0x0020
#define ST802RT1A_LDADV_SELECTOR           0x001F
#define ST802RT1A_LDADV_SELECTOR_DEFAULT   0x0001

//Auto-negotiation link partner ability register
#define ST802RT1A_LPADV_NEXT_PAGE          0x8000
#define ST802RT1A_LPADV_ACK                0x4000
#define ST802RT1A_LPADV_REMOTE_FAULT       0x2000
#define ST802RT1A_LPADV_ASYM_PAUSE         0x0800
#define ST802RT1A_LPADV_PAUSE              0x0400
#define ST802RT1A_LPADV_100BT4             0x0200
#define ST802RT1A_LPADV_100BTX_FD          0x0100
#define ST802RT1A_LPADV_100BTX_HD          0x0080
#define ST802RT1A_LPADV_10BT_FD            0x0040
#define ST802RT1A_LPADV_10BT_HD            0x0020
#define ST802RT1A_LPADV_SELECTOR           0x001F
#define ST802RT1A_LPADV_SELECTOR_DEFAULT   0x0001

//Auto-negotiation expansion register
#define ST802RT1A_ANEGX_PAR_DETECT_FAULT   0x0010
#define ST802RT1A_ANEGX_LP_NEXT_PAGE_ABLE  0x0008
#define ST802RT1A_ANEGX_NEXT_PAGE_ABLE     0x0004
#define ST802RT1A_ANEGX_PAGE_RECEIVED      0x0002
#define ST802RT1A_ANEGX_LP_AN_ABLE         0x0001

//Auto-negotiation next page transmit register
#define ST802RT1A_LDNPG_NEXT_PAGE          0x8000
#define ST802RT1A_LDNPG_MSG_PAGE           0x2000
#define ST802RT1A_LDNPG_ACK2               0x1000
#define ST802RT1A_LDNPG_TOGGLE             0x0800
#define ST802RT1A_LDNPG_MESSAGE            0x07FF

//Auto-negotiation link partner received next page register
#define ST802RT1A_LPNPG_NEXT_PAGE          0x8000
#define ST802RT1A_LPNPG_ACK                0x4000
#define ST802RT1A_LPNPG_MSG_PAGE           0x2000
#define ST802RT1A_LPNPG_ACK2               0x1000
#define ST802RT1A_LPNPG_TOGGLE             0x0800
#define ST802RT1A_LPNPG_MESSAGE            0x07FF

//RMII-TEST control register
#define ST802RT1A_XCNTL_MII_EN             0x0200
#define ST802RT1A_XCNTL_FEF_EN             0x0020
#define ST802RT1A_XCNTL_FIFO_EXTENDED      0x0004
#define ST802RT1A_XCNTL_RMII_OOBS          0x0002

//Receiver configuration information and interrupt status register
#define ST802RT1A_XSTAT_FX_MODE            0x0400
#define ST802RT1A_XSTAT_SPEED              0x0200
#define ST802RT1A_XSTAT_DUPLEX             0x0100
#define ST802RT1A_XSTAT_PAUSE              0x0080
#define ST802RT1A_XSTAT_AN_CMPL_INT        0x0040
#define ST802RT1A_XSTAT_REM_FLT_DET_INT    0x0020
#define ST802RT1A_XSTAT_LK_DWN_INT         0x0010
#define ST802RT1A_XSTAT_AN_ACK_DET_INT     0x0008
#define ST802RT1A_XSTAT_PD_FLT_INT         0x0004
#define ST802RT1A_XSTAT_PG_RCVD_INT        0x0002
#define ST802RT1A_XSTAT_RX_FULL_INT        0x0001

//Receiver event interrupts register
#define ST802RT1A_XRCNT_INT_OE_N           0x0100
#define ST802RT1A_XRCNT_INT_EN             0x0080
#define ST802RT1A_XRCNT_AN_CMPL_EN         0x0040
#define ST802RT1A_XRCNT_REM_FLT_DET_EN     0x0020
#define ST802RT1A_XRCNT_LK_DWN_EN          0x0010
#define ST802RT1A_XRCNT_AN_ACK_DET_EN      0x0008
#define ST802RT1A_XRCNT_PD_FLT_EN          0x0004
#define ST802RT1A_XRCNT_PG_RCVD_EN         0x0002
#define ST802RT1A_XRCNT_RX_FULL_EN         0x0001

//100Base-TX control register
#define ST802RT1A_XCCNT_RX_ERR_COUNTER_DIS 0x2000
#define ST802RT1A_XCCNT_AN_COMPLETE        0x1000
#define ST802RT1A_XCCNT_DC_REST_EN         0x0100
#define ST802RT1A_XCCNT_NRZ_CONV_EN        0x0080
#define ST802RT1A_XCCNT_TX_ISOLATE         0x0020
#define ST802RT1A_XCCNT_CMODE              0x001C
#define ST802RT1A_XCCNT_CMODE_AN           0x0000
#define ST802RT1A_XCCNT_CMODE_10BT_HD      0x0004
#define ST802RT1A_XCCNT_CMODE_100BTX_HD    0x0008
#define ST802RT1A_XCCNT_CMODE_10BT_FD      0x0014
#define ST802RT1A_XCCNT_CMODE_100BTX_FD    0x0018
#define ST802RT1A_XCCNT_CMODE_ISOLATE      0x001C
#define ST802RT1A_XCCNT_MLT3_DIS           0x0002
#define ST802RT1A_XCCNT_SCRAMBLER_DIS      0x0001

//Receiver mode control register
#define ST802RT1A_XDCNT_PHY_ADDR           0x00F8
#define ST802RT1A_XDCNT_PREAMBLE_SUPPR     0x0002

//Auxiliary control register
#define ST802RT1A_AUXCS_JABBER_DIS         0x8000
#define ST802RT1A_AUXCS_MDIO_POWER_SAVING  0x0010

//Auxiliary status register
#define ST802RT1A_AUXSS_AN_COMPLETE        0x8000
#define ST802RT1A_AUXSS_AN_ACK             0x4000
#define ST802RT1A_AUXSS_AN_DETECT          0x2000
#define ST802RT1A_AUXSS_LP_AN_ABLE_DETECT  0x1000
#define ST802RT1A_AUXSS_AN_PAUSE           0x0800
#define ST802RT1A_AUXSS_AN_HCD             0x0700
#define ST802RT1A_AUXSS_PAR_DET_FAULT      0x0080
#define ST802RT1A_AUXSS_REMOTE_FAULT       0x0040
#define ST802RT1A_AUXSS_PAGE_RECEIVED      0x0020
#define ST802RT1A_AUXSS_LP_AN_ABLE         0x0010
#define ST802RT1A_AUXSS_SP100              0x0008
#define ST802RT1A_AUXSS_LINK_STATUS        0x0004
#define ST802RT1A_AUXSS_AN_EN              0x0002
#define ST802RT1A_AUXSS_JABBER_DETECT      0x0001

//Auxiliary mode 2 register
#define ST802RT1A_AUXM2_LED_MODE           0x0200
#define ST802RT1A_AUXM2_10BT_ECHO_DIS      0x0080
#define ST802RT1A_AUXM2_MI_SQE_DIS         0x0008

//10Base-T error and general status register
#define ST802RT1A_TSTAT_MDIX_STATUS        0x2000
#define ST802RT1A_TSTAT_MDIX_SWAP          0x1000
#define ST802RT1A_TSTAT_MDIX_DIS           0x0800
#define ST802RT1A_TSTAT_JABBER_DETECT      0x0200
#define ST802RT1A_TSTAT_POLARITY_CHANGED   0x0100

//Auxiliary PHY register
#define ST802RT1A_AMPHY_HCD_100BTX_FD      0x8000
#define ST802RT1A_AMPHY_HCD_100BT4         0x4000
#define ST802RT1A_AMPHY_HCD_100BTX_HD      0x2000
#define ST802RT1A_AMPHY_HCD_10BT_FD        0x1000
#define ST802RT1A_AMPHY_HCD_10BT_HD        0x0800
#define ST802RT1A_AMPHY_AN_RESTART         0x0100
#define ST802RT1A_AMPHY_AN_COMPLETE        0x0080
#define ST802RT1A_AMPHY_AN_ACK_COMPLETE    0x0040
#define ST802RT1A_AMPHY_AN_ACK             0x0020
#define ST802RT1A_AMPHY_AN_ABLE            0x0010
#define ST802RT1A_AMPHY_SUPER_ISOLATE      0x0008

//Shadow Registers enable register
#define ST802RT1A_BTEST_SHADOW_REG_EN      0x0080

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//ST802RT1A Ethernet PHY driver
extern const PhyDriver st802rt1aPhyDriver;

//ST802RT1A related functions
error_t st802rt1aInit(NetInterface *interface);

void st802rt1aTick(NetInterface *interface);

void st802rt1aEnableIrq(NetInterface *interface);
void st802rt1aDisableIrq(NetInterface *interface);

void st802rt1aEventHandler(NetInterface *interface);

void st802rt1aWritePhyReg(NetInterface *interface, uint8_t address,
   uint16_t data);

uint16_t st802rt1aReadPhyReg(NetInterface *interface, uint8_t address);

void st802rt1aDumpPhyReg(NetInterface *interface);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
