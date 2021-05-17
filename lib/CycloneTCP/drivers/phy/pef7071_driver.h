/**
 * @file pef7071_driver.h
 * @brief XWAY PHY11G (PEF7071) Gigabit Ethernet PHY driver
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

#ifndef _PEF7071_DRIVER_H
#define _PEF7071_DRIVER_H

//Dependencies
#include "core/nic.h"

//PHY address
#ifndef PEF7071_PHY_ADDR
   #define PEF7071_PHY_ADDR 0
#elif (PEF7071_PHY_ADDR < 0 || PEF7071_PHY_ADDR > 31)
   #error PEF7071_PHY_ADDR parameter is not valid
#endif

//PEF7071 PHY registers
#define PEF7071_CTRL                        0x00
#define PEF7071_STAT                        0x01
#define PEF7071_PHYID1                      0x02
#define PEF7071_PHYID2                      0x03
#define PEF7071_AN_ADV                      0x04
#define PEF7071_AN_LPA                      0x05
#define PEF7071_AN_EXP                      0x06
#define PEF7071_AN_NPTX                     0x07
#define PEF7071_AN_NPRX                     0x08
#define PEF7071_GCTRL                       0x09
#define PEF7071_GSTAT                       0x0A
#define PEF7071_RES11                       0x0B
#define PEF7071_RES12                       0x0C
#define PEF7071_MMDCTRL                     0x0D
#define PEF7071_MMDDATA                     0x0E
#define PEF7071_XSTAT                       0x0F
#define PEF7071_PHYPHYPERF                  0x10
#define PEF7071_PHYSTAT1                    0x11
#define PEF7071_PHYSTAT2                    0x12
#define PEF7071_PHYCTL1                     0x13
#define PEF7071_PHYCTL2                     0x14
#define PEF7071_ERRCNT                      0x15
#define PEF7071_EECTRL                      0x16
#define PEF7071_MIICTRL                     0x17
#define PEF7071_MIISTAT                     0x18
#define PEF7071_IMASK                       0x19
#define PEF7071_ISTAT                       0x1A
#define PEF7071_LED                         0x1B
#define PEF7071_TPGCTRL                     0x1C
#define PEF7071_TPGDATA                     0x1D
#define PEF7071_FWV                         0x1E
#define PEF7071_RES1F                       0x1F

//PEF7071 MMD registers
#define PEF7071_EEE_CTRL1                   0x03, 0x0000
#define PEF7071_EEE_STAT1                   0x03, 0x0001
#define PEF7071_EEE_CAP                     0x03, 0x0014
#define PEF7071_EEE_WAKERR                  0x03, 0x0016
#define PEF7071_ANEGEEE_AN_ADV              0x07, 0x003C
#define PEF7071_EEE_AN_LPADV                0x07, 0x003D
#define PEF7071_EEPROM                      0x1E, 0x0000
#define PEF7071_LEDCH                       0x1F, 0x01E0
#define PEF7071_LEDCL                       0x1F, 0x01E1
#define PEF7071_LED0H                       0x1F, 0x01E2
#define PEF7071_LED0L                       0x1F, 0x01E3
#define PEF7071_LED1H                       0x1F, 0x01E4
#define PEF7071_LED1L                       0x1F, 0x01E5
#define PEF7071_LED2H                       0x1F, 0x01E6
#define PEF7071_LED2L                       0x1F, 0x01E7
#define PEF7071_EEE_RXERR_LINK_FAIL_H       0x1F, 0x01EA
#define PEF7071_EEE_RXERR_LINK_FAIL_L       0x1F, 0x01EB
#define PEF7071_MII2CTRL                    0x1F, 0x01EC
#define PEF7071_LEG_LPI_CFG0                0x1F, 0x01ED
#define PEF7071_LEG_LPI_CFG1                0x1F, 0x01EE
#define PEF7071_WOLCTRL                     0x1F, 0x0781
#define PEF7071_WOLAD0                      0x1F, 0x0783
#define PEF7071_WOLAD1                      0x1F, 0x0784
#define PEF7071_WOLAD2                      0x1F, 0x0785
#define PEF7071_WOLAD3                      0x1F, 0x0786
#define PEF7071_WOLAD4                      0x1F, 0x0787
#define PEF7071_WOLAD5                      0x1F, 0x0788
#define PEF7071_WOLPW0                      0x1F, 0x0789
#define PEF7071_WOLPW1                      0x1F, 0x078A
#define PEF7071_WOLPW2                      0x1F, 0x078B
#define PEF7071_WOLPW3                      0x1F, 0x078C
#define PEF7071_WOLPW4                      0x1F, 0x078D
#define PEF7071_WOLPW5                      0x1F, 0x078E
#define PEF7071_LEG_LPI_CFG2                0x1F, 0x0EB5
#define PEF7071_LEG_LPI_CFG3                0x1F, 0x0EB7

//Control register
#define PEF7071_CTRL_RST                    0x8000
#define PEF7071_CTRL_LB                     0x4000
#define PEF7071_CTRL_SSL                    0x2000
#define PEF7071_CTRL_ANEN                   0x1000
#define PEF7071_CTRL_PD                     0x0800
#define PEF7071_CTRL_ISOL                   0x0400
#define PEF7071_CTRL_ANRS                   0x0200
#define PEF7071_CTRL_DPLX                   0x0100
#define PEF7071_CTRL_COL                    0x0080
#define PEF7071_CTRL_SSM                    0x0040

//Status register
#define PEF7071_STAT_CBT4                   0x8000
#define PEF7071_STAT_CBTXF                  0x4000
#define PEF7071_STAT_CBTXH                  0x2000
#define PEF7071_STAT_XBTF                   0x1000
#define PEF7071_STAT_XBTH                   0x0800
#define PEF7071_STAT_CBT2F                  0x0400
#define PEF7071_STAT_CBT2H                  0x0200
#define PEF7071_STAT_EXT                    0x0100
#define PEF7071_STAT_MFPS                   0x0040
#define PEF7071_STAT_ANOK                   0x0020
#define PEF7071_STAT_RF                     0x0010
#define PEF7071_STAT_ANAB                   0x0008
#define PEF7071_STAT_LS                     0x0004
#define PEF7071_STAT_JD                     0x0002
#define PEF7071_STAT_XCAP                   0x0001

//PHY Identifier 1 register
#define PEF7071_PHYID1_OUI_MSB              0xFFFF
#define PEF7071_PHYID1_OUI_MSB_DEFAULT      0x0000

//PHY Identifier 2 register
#define PEF7071_PHYID2_OUI_LSB              0xFC00
#define PEF7071_PHYID2_OUI_LSB_DEFAULT      0x0000
#define PEF7071_PHYID2_LDN                  0x03F0
#define PEF7071_PHYID2_LDN_DEFAULT          0x0000
#define PEF7071_PHYID2_LDRN                 0x000F

//Auto-Negotiation Advertisement register
#define PEF7071_AN_ADV_NP                   0x8000
#define PEF7071_AN_ADV_RF                   0x2000
#define PEF7071_AN_ADV_TAF                  0x1FE0
#define PEF7071_AN_ADV_TAF_XBT_HDX          0x0020
#define PEF7071_AN_ADV_TAF_XBT_FDX          0x0040
#define PEF7071_AN_ADV_TAF_DBT_HDX          0x0080
#define PEF7071_AN_ADV_TAF_DBT_FDX          0x0100
#define PEF7071_AN_ADV_TAF_DBT4             0x0200
#define PEF7071_AN_ADV_TAF_PS_SYM           0x0400
#define PEF7071_AN_ADV_TAF_PS_ASYM          0x0800
#define PEF7071_AN_ADV_TAF_RES              0x1000
#define PEF7071_AN_ADV_SF                   0x001F
#define PEF7071_AN_ADV_SF_DEFAULT           0x0001

//Auto-Negotiation Link-Partner Ability register
#define PEF7071_AN_LPA_NP                   0x8000
#define PEF7071_AN_LPA_ACK                  0x4000
#define PEF7071_AN_LPA_RF                   0x2000
#define PEF7071_AN_LPA_TAF                  0x1FE0
#define PEF7071_AN_LPA_TAF_XBT_HDX          0x0020
#define PEF7071_AN_LPA_TAF_XBT_FDX          0x0040
#define PEF7071_AN_LPA_TAF_DBT_HDX          0x0080
#define PEF7071_AN_LPA_TAF_DBT_FDX          0x0100
#define PEF7071_AN_LPA_TAF_DBT4             0x0200
#define PEF7071_AN_LPA_TAF_PS_SYM           0x0400
#define PEF7071_AN_LPA_TAF_PS_ASYM          0x0800
#define PEF7071_AN_LPA_TAF_RES              0x1000
#define PEF7071_AN_LPA_SF                   0x001F
#define PEF7071_AN_LPA_SF_DEFAULT           0x0001

//Auto-Negotiation Expansion register
#define PEF7071_AN_EXP_RESD                 0xFFE0
#define PEF7071_AN_EXP_PDF                  0x0010
#define PEF7071_AN_EXP_LPNPC                0x0008
#define PEF7071_AN_EXP_NPC                  0x0004
#define PEF7071_AN_EXP_PR                   0x0002
#define PEF7071_AN_EXP_LPANC                0x0001

//Auto-Negotiation Next-Page Transmit register
#define PEF7071_AN_NPTX_NP                  0x8000
#define PEF7071_AN_NPTX_MP                  0x2000
#define PEF7071_AN_NPTX_ACK2                0x1000
#define PEF7071_AN_NPTX_TOGG                0x0800
#define PEF7071_AN_NPTX_MCF                 0x07FF

//Auto-Negotiation Link-Partner Received Next-Page register
#define PEF7071_AN_NPRX_NP                  0x8000
#define PEF7071_AN_NPRX_ACK                 0x4000
#define PEF7071_AN_NPRX_MP                  0x2000
#define PEF7071_AN_NPRX_ACK2                0x1000
#define PEF7071_AN_NPRX_TOGG                0x0800
#define PEF7071_AN_NPRX_MCF                 0x07FF

//Gigabit Control register
#define PEF7071_GCTRL_TM                    0xE000
#define PEF7071_GCTRL_MSEN                  0x1000
#define PEF7071_GCTRL_MS                    0x0800
#define PEF7071_GCTRL_MSPT                  0x0400
#define PEF7071_GCTRL_MBTFD                 0x0200
#define PEF7071_GCTRL_MBTHD                 0x0100

//Gigabit Status register
#define PEF7071_GSTAT_MSFAULT               0x8000
#define PEF7071_GSTAT_MSRES                 0x4000
#define PEF7071_GSTAT_LRXSTAT               0x2000
#define PEF7071_GSTAT_RRXSTAT               0x1000
#define PEF7071_GSTAT_MBTFD                 0x0800
#define PEF7071_GSTAT_MBTHD                 0x0400
#define PEF7071_GSTAT_IEC                   0x00FF

//MMD Access Control register
#define PEF7071_MMDCTRL_ACTYPE              0xC000
#define PEF7071_MMDCTRL_ACTYPE_ADDRESS      0x0000
#define PEF7071_MMDCTRL_ACTYPE_DATA         0x4000
#define PEF7071_MMDCTRL_ACTYPE_DATA_PI      0x8000
#define PEF7071_MMDCTRL_ACTYPE_DATA_PIWR    0xC000
#define PEF7071_MMDCTRL_RESH                0x3F00
#define PEF7071_MMDCTRL_RESL                0x00E0
#define PEF7071_MMDCTRL_DEVAD               0x001F

//MMD Access Data register
#define PEF7071_MMDDATA_ADDR_DATA           0xFFFF

//Extended Status register
#define PEF7071_XSTAT_MBXF                  0x8000
#define PEF7071_XSTAT_MBXH                  0x4000
#define PEF7071_XSTAT_MBTF                  0x2000
#define PEF7071_XSTAT_MBTH                  0x1000
#define PEF7071_XSTAT_RESH                  0x0F00
#define PEF7071_XSTAT_RESL                  0x00FF

//Physical Layer Performance Status register
#define PEF7071_PHYPHYPERF_FREQ             0xFF00
#define PEF7071_PHYPHYPERF_SNR              0x00F0
#define PEF7071_PHYPHYPERF_LEN              0x000F

//Physical Layer Status 1 register
#define PEF7071_PHYSTAT1_RESH               0xFE00
#define PEF7071_PHYSTAT1_LSADS              0x0100
#define PEF7071_PHYSTAT1_POLD               0x0080
#define PEF7071_PHYSTAT1_POLC               0x0040
#define PEF7071_PHYSTAT1_POLB               0x0020
#define PEF7071_PHYSTAT1_POLA               0x0010
#define PEF7071_PHYSTAT1_MDICD              0x0008
#define PEF7071_PHYSTAT1_MDIAB              0x0004
#define PEF7071_PHYSTAT1_RESL               0x0003

//Physical Layer Status 2 register
#define PEF7071_PHYSTAT2_RESD               0x8000
#define PEF7071_PHYSTAT2_SKEWD              0x7000
#define PEF7071_PHYSTAT2_RESC               0x0800
#define PEF7071_PHYSTAT2_SKEWC              0x0700
#define PEF7071_PHYSTAT2_RESB               0x0080
#define PEF7071_PHYSTAT2_SKEWB              0x0070
#define PEF7071_PHYSTAT2_RESA               0x0008
#define PEF7071_PHYSTAT2_SKEWA              0x0007

//Physical Layer Control 1 register
#define PEF7071_PHYCTL1_TLOOP               0xE000
#define PEF7071_PHYCTL1_TXOFF               0x1000
#define PEF7071_PHYCTL1_TXADJ               0x0F00
#define PEF7071_PHYCTL1_POLD                0x0080
#define PEF7071_PHYCTL1_POLC                0x0040
#define PEF7071_PHYCTL1_POLB                0x0020
#define PEF7071_PHYCTL1_POLA                0x0010
#define PEF7071_PHYCTL1_MDICD               0x0008
#define PEF7071_PHYCTL1_MDIAB               0x0004
#define PEF7071_PHYCTL1_TXEEE10             0x0002
#define PEF7071_PHYCTL1_AMDIX               0x0001

//Physical Layer Control 2 register
#define PEF7071_PHYCTL2_LSADS               0xC000
#define PEF7071_PHYCTL2_LSADS_OFF           0x0000
#define PEF7071_PHYCTL2_LSADS_ADS2          0x4000
#define PEF7071_PHYCTL2_LSADS_ADS3          0x8000
#define PEF7071_PHYCTL2_LSADS_ADS4          0xC000
#define PEF7071_PHYCTL2_RESH                0x3800
#define PEF7071_PHYCTL2_CLKSEL              0x0400
#define PEF7071_PHYCTL2_CLKSEL_CLK25M       0x0000
#define PEF7071_PHYCTL2_CLKSEL_CLK125M      0x0400
#define PEF7071_PHYCTL2_SDETP               0x0200
#define PEF7071_PHYCTL2_SDETP_LOWACTIVE     0x0000
#define PEF7071_PHYCTL2_SDETP_HIGHACTIVE    0x0200
#define PEF7071_PHYCTL2_STICKY              0x0100
#define PEF7071_PHYCTL2_RESL                0x00F0
#define PEF7071_PHYCTL2_ADCR                0x0008
#define PEF7071_PHYCTL2_ADCR_DEFAULT        0x0000
#define PEF7071_PHYCTL2_ADCR_BOOST          0x0008
#define PEF7071_PHYCTL2_PSCL                0x0004
#define PEF7071_PHYCTL2_ANPD                0x0002
#define PEF7071_PHYCTL2_LPI                 0x0001

//Error Counter register
#define PEF7071_ERRCNT_SEL                  0x0F00
#define PEF7071_ERRCNT_SEL_RXERR            0x0000
#define PEF7071_ERRCNT_SEL_RXACT            0x0100
#define PEF7071_ERRCNT_SEL_ESDERR           0x0200
#define PEF7071_ERRCNT_SEL_SSDERR           0x0300
#define PEF7071_ERRCNT_SEL_TXERR            0x0400
#define PEF7071_ERRCNT_SEL_TXACT            0x0500
#define PEF7071_ERRCNT_SEL_COL              0x0600
#define PEF7071_ERRCNT_COUNT                0x00FF

//EEPROM Control register
#define PEF7071_EECTRL_EESCAN               0x8000
#define PEF7071_EECTRL_EEAF                 0x4000
#define PEF7071_EECTRL_CSRDET               0x2000
#define PEF7071_EECTRL_EEDET                0x1000
#define PEF7071_EECTRL_SIZE                 0x0F00
#define PEF7071_EECTRL_SIZE_SIZE1K          0x0000
#define PEF7071_EECTRL_SIZE_SIZE2K          0x0100
#define PEF7071_EECTRL_SIZE_SIZE4K          0x0200
#define PEF7071_EECTRL_SIZE_SIZE8K          0x0300
#define PEF7071_EECTRL_SIZE_SIZE16K         0x0400
#define PEF7071_EECTRL_SIZE_SIZE32K         0x0500
#define PEF7071_EECTRL_SIZE_SIZE64K         0x0600
#define PEF7071_EECTRL_SIZE_SIZE128K        0x0700
#define PEF7071_EECTRL_SIZE_SIZE256K        0x0800
#define PEF7071_EECTRL_SIZE_SIZE512K        0x0900
#define PEF7071_EECTRL_SIZE_SIZE1024K       0x0A00
#define PEF7071_EECTRL_ADRMODE              0x0080
#define PEF7071_EECTRL_ADRMODE_MODE11       0x0000
#define PEF7071_EECTRL_ADRMODE_MODE16       0x0080
#define PEF7071_EECTRL_DADR                 0x0070
#define PEF7071_EECTRL_SPEED                0x000C
#define PEF7071_EECTRL_SPEED_FRQ_100KHZ     0x0000
#define PEF7071_EECTRL_SPEED_FRQ_400KHZ     0x0004
#define PEF7071_EECTRL_SPEED_FRQ_1_0MHZ     0x0008
#define PEF7071_EECTRL_SPEED_FRQ_3_4MHZ     0x000C
#define PEF7071_EECTRL_RDWR                 0x0002
#define PEF7071_EECTRL_EXEC                 0x0001

//Media-Independent Interface Control register
#define PEF7071_MIICTRL_RXCOFF              0x8000
#define PEF7071_MIICTRL_RXSKEW              0x7000
#define PEF7071_MIICTRL_RXSKEW_SKEW_0N0     0x0000
#define PEF7071_MIICTRL_RXSKEW_SKEW_0N5     0x1000
#define PEF7071_MIICTRL_RXSKEW_SKEW_1N0     0x2000
#define PEF7071_MIICTRL_RXSKEW_SKEW_1N5     0x3000
#define PEF7071_MIICTRL_RXSKEW_SKEW_2N0     0x4000
#define PEF7071_MIICTRL_RXSKEW_SKEW_2N5     0x5000
#define PEF7071_MIICTRL_RXSKEW_SKEW_3N0     0x6000
#define PEF7071_MIICTRL_RXSKEW_SKEW_3N5     0x7000
#define PEF7071_MIICTRL_V25_33              0x0800
#define PEF7071_MIICTRL_TXSKEW              0x0700
#define PEF7071_MIICTRL_TXSKEW_SKEW_0N0     0x0000
#define PEF7071_MIICTRL_TXSKEW_SKEW_0N5     0x0100
#define PEF7071_MIICTRL_TXSKEW_SKEW_1N0     0x0200
#define PEF7071_MIICTRL_TXSKEW_SKEW_1N5     0x0300
#define PEF7071_MIICTRL_TXSKEW_SKEW_2N0     0x0400
#define PEF7071_MIICTRL_TXSKEW_SKEW_2N5     0x0500
#define PEF7071_MIICTRL_TXSKEW_SKEW_3N0     0x0600
#define PEF7071_MIICTRL_TXSKEW_SKEW_3N5     0x0700
#define PEF7071_MIICTRL_CRS                 0x00C0
#define PEF7071_MIICTRL_FLOW                0x0030
#define PEF7071_MIICTRL_FLOW_COPPER         0x0000
#define PEF7071_MIICTRL_FLOW_CONVERTER      0x0030
#define PEF7071_MIICTRL_MODE                0x000F
#define PEF7071_MIICTRL_MODE_RGMII          0x0000
#define PEF7071_MIICTRL_MODE_SGMII          0x0001
#define PEF7071_MIICTRL_MODE_RMII           0x0002
#define PEF7071_MIICTRL_MODE_RTBI           0x0003
#define PEF7071_MIICTRL_MODE_GMII           0x0004
#define PEF7071_MIICTRL_MODE_TBI            0x0005
#define PEF7071_MIICTRL_MODE_SGMIINC        0x0006
#define PEF7071_MIICTRL_MODE_TEST           0x000F
#define PEF7071_MIICTRL_MODE_CONV_X2T1000   0x0000
#define PEF7071_MIICTRL_MODE_CONV_X2T1000A  0x0001

//Media-Independent Interface Status register
#define PEF7071_MIISTAT_RESH                0xFF00
#define PEF7071_MIISTAT_PHY                 0x00C0
#define PEF7071_MIISTAT_PHY_TP              0x0000
#define PEF7071_MIISTAT_PHY_FIBER           0x0040
#define PEF7071_MIISTAT_PHY_MII2            0x0080
#define PEF7071_MIISTAT_PHY_SGMII           0x00C0
#define PEF7071_MIISTAT_PS                  0x0030
#define PEF7071_MIISTAT_PS_NONE             0x0000
#define PEF7071_MIISTAT_PS_TX               0x0010
#define PEF7071_MIISTAT_PS_RX               0x0020
#define PEF7071_MIISTAT_PS_TXRX             0x0030
#define PEF7071_MIISTAT_DPX                 0x0008
#define PEF7071_MIISTAT_EEE                 0x0004
#define PEF7071_MIISTAT_EEE_OFF             0x0000
#define PEF7071_MIISTAT_EEE_ON              0x0004
#define PEF7071_MIISTAT_SPEED               0x0003
#define PEF7071_MIISTAT_SPEED_TEN           0x0000
#define PEF7071_MIISTAT_SPEED_FAST          0x0001
#define PEF7071_MIISTAT_SPEED_GIGA          0x0002
#define PEF7071_MIISTAT_SPEED_RES           0x0003

//Interrupt Mask register
#define PEF7071_IMASK_WOL                   0x8000
#define PEF7071_IMASK_MSRE                  0x4000
#define PEF7071_IMASK_NPRX                  0x2000
#define PEF7071_IMASK_NPTX                  0x1000
#define PEF7071_IMASK_ANE                   0x0800
#define PEF7071_IMASK_ANC                   0x0400
#define PEF7071_IMASK_RESH                  0x0300
#define PEF7071_IMASK_RESL                  0x00C0
#define PEF7071_IMASK_ADSC                  0x0020
#define PEF7071_IMASK_MDIPC                 0x0010
#define PEF7071_IMASK_MDIXC                 0x0008
#define PEF7071_IMASK_DXMC                  0x0004
#define PEF7071_IMASK_LSPC                  0x0002
#define PEF7071_IMASK_LSTC                  0x0001

//Interrupt Status register
#define PEF7071_ISTAT_WOL                   0x8000
#define PEF7071_ISTAT_MSRE                  0x4000
#define PEF7071_ISTAT_NPRX                  0x2000
#define PEF7071_ISTAT_NPTX                  0x1000
#define PEF7071_ISTAT_ANE                   0x0800
#define PEF7071_ISTAT_ANC                   0x0400
#define PEF7071_ISTAT_RESH                  0x0300
#define PEF7071_ISTAT_RESL                  0x00C0
#define PEF7071_ISTAT_ADSC                  0x0020
#define PEF7071_ISTAT_MDIPC                 0x0010
#define PEF7071_ISTAT_MDIXC                 0x0008
#define PEF7071_ISTAT_DXMC                  0x0004
#define PEF7071_ISTAT_LSPC                  0x0002
#define PEF7071_ISTAT_LSTC                  0x0001

//LED Control register
#define PEF7071_LED_RESH                    0xF000
#define PEF7071_LED_LED3EN                  0x0800
#define PEF7071_LED_LED2EN                  0x0400
#define PEF7071_LED_LED1EN                  0x0200
#define PEF7071_LED_LED0EN                  0x0100
#define PEF7071_LED_RESL                    0x00F0
#define PEF7071_LED_LED3DA                  0x0008
#define PEF7071_LED_LED3DA_OFF              0x0000
#define PEF7071_LED_LED3DA_ON               0x0008
#define PEF7071_LED_LED2DA                  0x0004
#define PEF7071_LED_LED2DA_OFF              0x0000
#define PEF7071_LED_LED2DA_ON               0x0004
#define PEF7071_LED_LED1DA                  0x0002
#define PEF7071_LED_LED1DA_OFF              0x0000
#define PEF7071_LED_LED1DA_ON               0x0002
#define PEF7071_LED_LED0DA                  0x0001
#define PEF7071_LED_LED0DA_OFF              0x0000
#define PEF7071_LED_LED0DA_ON               0x0001

//Test-Packet Generator Control register
#define PEF7071_TPGCTRL_RESH1               0xC000
#define PEF7071_TPGCTRL_MODE                0x2000
#define PEF7071_TPGCTRL_MODE_BURST          0x0000
#define PEF7071_TPGCTRL_MODE_SINGLE         0x2000
#define PEF7071_TPGCTRL_RESH0               0x1000
#define PEF7071_TPGCTRL_IPGL                0x0C00
#define PEF7071_TPGCTRL_IPGL_BT48           0x0000
#define PEF7071_TPGCTRL_IPGL_BT96           0x0400
#define PEF7071_TPGCTRL_IPGL_BT960          0x0800
#define PEF7071_TPGCTRL_IPGL_BT9600         0x0C00
#define PEF7071_TPGCTRL_TYPE                0x0300
#define PEF7071_TPGCTRL_TYPE_RANDOM         0x0000
#define PEF7071_TPGCTRL_TYPE_BYTEINC        0x0100
#define PEF7071_TPGCTRL_TYPE_PREDEF         0x0200
#define PEF7071_TPGCTRL_RESL1               0x0080
#define PEF7071_TPGCTRL_SIZE                0x0070
#define PEF7071_TPGCTRL_SIZE_B64            0x0000
#define PEF7071_TPGCTRL_SIZE_B128           0x0010
#define PEF7071_TPGCTRL_SIZE_B256           0x0020
#define PEF7071_TPGCTRL_SIZE_B512           0x0030
#define PEF7071_TPGCTRL_SIZE_B1024          0x0040
#define PEF7071_TPGCTRL_SIZE_B1518          0x0050
#define PEF7071_TPGCTRL_SIZE_B9600          0x0060
#define PEF7071_TPGCTRL_RESL0               0x000C
#define PEF7071_TPGCTRL_START               0x0002
#define PEF7071_TPGCTRL_EN                  0x0001

//Test-Packet Generator Data register
#define PEF7071_TPGDATA_DA                  0xF000
#define PEF7071_TPGDATA_SA                  0x0F00
#define PEF7071_TPGDATA_DATA                0x00FF

//Firmware Version register
#define PEF7071_FWV_REL                     0x8000
#define PEF7071_FWV_REL_TEST                0x0000
#define PEF7071_FWV_REL_RELEASE             0x8000
#define PEF7071_FWV_MAJOR                   0x7F00
#define PEF7071_FWV_MINOR                   0x00FF

//EEE Control 1 register
#define PEF7071_EEE_CTRL1_RXCKST            0x0400

//EEE Status 1 register
#define PEF7071_EEE_STAT1_TXLPI_RCVD        0x0800
#define PEF7071_EEE_STAT1_TXLPI_IND         0x0200
#define PEF7071_EEE_STAT1_RXLPI_IND         0x0100
#define PEF7071_EEE_STAT1_TXCKST            0x0040

//EEE Capability register
#define PEF7071_EEE_CAP_EEE_10GBKR          0x0040
#define PEF7071_EEE_CAP_EEE_10GBKX4         0x0020
#define PEF7071_EEE_CAP_EEE_1000BKX         0x0010
#define PEF7071_EEE_CAP_EEE_10GBT           0x0008
#define PEF7071_EEE_CAP_EEE_1000BT          0x0004
#define PEF7071_EEE_CAP_EEE_100BTX          0x0002

//EEE Wake Time Fault Count register
#define PEF7071_EEE_WAKERR_ERRCNT           0xFFFF

//EEE Auto-Negotiation Advertisement register
#define PEF7071_ANEGEEE_AN_ADV_EEE_10GBKR   0x0040
#define PEF7071_ANEGEEE_AN_ADV_EEE_10GBKX4  0x0020
#define PEF7071_ANEGEEE_AN_ADV_EEE_1000BKX  0x0010
#define PEF7071_ANEGEEE_AN_ADV_EEE_10GBT    0x0008
#define PEF7071_ANEGEEE_AN_ADV_EEE_1000BT   0x0004
#define PEF7071_ANEGEEE_AN_ADV_EEE_100BTX   0x0002

//EEE Auto-Negotiation Link-Partner Advertisement register
#define PEF7071_EEE_AN_LPADV_EEE_10GBKR     0x0040
#define PEF7071_EEE_AN_LPADV_EEE_10GBKX4    0x0020
#define PEF7071_EEE_AN_LPADV_EEE_1000BKX    0x0010
#define PEF7071_EEE_AN_LPADV_EEE_10GBT      0x0008
#define PEF7071_EEE_AN_LPADV_EEE_1000BT     0x0004
#define PEF7071_EEE_AN_LPADV_EEE_100BTX     0x0002

//EEPROM Content register
#define PEF7071_EEPROM_DATA                 0x00FF

//LED Configuration H register
#define PEF7071_LEDCH_FBF                   0x00C0
#define PEF7071_LEDCH_FBF_F02HZ             0x0000
#define PEF7071_LEDCH_FBF_F04HZ             0x0040
#define PEF7071_LEDCH_FBF_F08HZ             0x0080
#define PEF7071_LEDCH_FBF_F16HZ             0x00C0
#define PEF7071_LEDCH_SBF                   0x0030
#define PEF7071_LEDCH_SBF_F02HZ             0x0000
#define PEF7071_LEDCH_SBF_F04HZ             0x0010
#define PEF7071_LEDCH_SBF_F08HZ             0x0020
#define PEF7071_LEDCH_SBF_F16HZ             0x0030
#define PEF7071_LEDCH_NACS                  0x0007
#define PEF7071_LEDCH_NACS_NONE             0x0000
#define PEF7071_LEDCH_NACS_LINK             0x0001
#define PEF7071_LEDCH_NACS_PDOWN            0x0002
#define PEF7071_LEDCH_NACS_EEE              0x0003
#define PEF7071_LEDCH_NACS_ANEG             0x0004
#define PEF7071_LEDCH_NACS_ABIST            0x0005
#define PEF7071_LEDCH_NACS_CDIAG            0x0006
#define PEF7071_LEDCH_NACS_TEST             0x0007

//LED Configuration L register
#define PEF7071_LEDCL_SCAN                  0x0070
#define PEF7071_LEDCL_SCAN_NONE             0x0000
#define PEF7071_LEDCL_SCAN_LINK             0x0010
#define PEF7071_LEDCL_SCAN_PDOWN            0x0020
#define PEF7071_LEDCL_SCAN_EEE              0x0030
#define PEF7071_LEDCL_SCAN_ANEG             0x0040
#define PEF7071_LEDCL_SCAN_ABIST            0x0050
#define PEF7071_LEDCL_SCAN_CDIAG            0x0060
#define PEF7071_LEDCL_SCAN_TEST             0x0070
#define PEF7071_LEDCL_CBLINK                0x0007
#define PEF7071_LEDCL_CBLINK_NONE           0x0000
#define PEF7071_LEDCL_CBLINK_LINK           0x0001
#define PEF7071_LEDCL_CBLINK_PDOWN          0x0002
#define PEF7071_LEDCL_CBLINK_EEE            0x0003
#define PEF7071_LEDCL_CBLINK_ANEG           0x0004
#define PEF7071_LEDCL_CBLINK_ABIST          0x0005
#define PEF7071_LEDCL_CBLINK_CDIAG          0x0006
#define PEF7071_LEDCL_CBLINK_TEST           0x0007

//Configuration for LED Pin 0 H register
#define PEF7071_LED0H_CON                   0x00F0
#define PEF7071_LED0H_CON_NONE              0x0000
#define PEF7071_LED0H_CON_LINK10            0x0010
#define PEF7071_LED0H_CON_LINK100           0x0020
#define PEF7071_LED0H_CON_LINK10X           0x0030
#define PEF7071_LED0H_CON_LINK1000          0x0040
#define PEF7071_LED0H_CON_LINK10_0          0x0050
#define PEF7071_LED0H_CON_LINK100X          0x0060
#define PEF7071_LED0H_CON_LINK10XX          0x0070
#define PEF7071_LED0H_CON_PDOWN             0x0080
#define PEF7071_LED0H_CON_EEE               0x0090
#define PEF7071_LED0H_CON_ANEG              0x00A0
#define PEF7071_LED0H_CON_ABIST             0x00B0
#define PEF7071_LED0H_CON_CDIAG             0x00C0
#define PEF7071_LED0H_CON_COPPER            0x00D0
#define PEF7071_LED0H_CON_FIBER             0x00E0
#define PEF7071_LED0H_BLINKF                0x000F
#define PEF7071_LED0H_BLINKF_NONE           0x0000
#define PEF7071_LED0H_BLINKF_LINK10         0x0001
#define PEF7071_LED0H_BLINKF_LINK100        0x0002
#define PEF7071_LED0H_BLINKF_LINK10X        0x0003
#define PEF7071_LED0H_BLINKF_LINK1000       0x0004
#define PEF7071_LED0H_BLINKF_LINK10_0       0x0005
#define PEF7071_LED0H_BLINKF_LINK100X       0x0006
#define PEF7071_LED0H_BLINKF_LINK10XX       0x0007
#define PEF7071_LED0H_BLINKF_PDOWN          0x0008
#define PEF7071_LED0H_BLINKF_EEE            0x0009
#define PEF7071_LED0H_BLINKF_ANEG           0x000A
#define PEF7071_LED0H_BLINKF_ABIST          0x000B
#define PEF7071_LED0H_BLINKF_CDIAG          0x000C

//Configuration for LED Pin 0 L register
#define PEF7071_LED0L_BLINKS                0x00F0
#define PEF7071_LED0L_BLINKS_NONE           0x0000
#define PEF7071_LED0L_BLINKS_LINK10         0x0010
#define PEF7071_LED0L_BLINKS_LINK100        0x0020
#define PEF7071_LED0L_BLINKS_LINK10X        0x0030
#define PEF7071_LED0L_BLINKS_LINK1000       0x0040
#define PEF7071_LED0L_BLINKS_LINK10_0       0x0050
#define PEF7071_LED0L_BLINKS_LINK100X       0x0060
#define PEF7071_LED0L_BLINKS_LINK10XX       0x0070
#define PEF7071_LED0L_BLINKS_PDOWN          0x0080
#define PEF7071_LED0L_BLINKS_EEE            0x0090
#define PEF7071_LED0L_BLINKS_ANEG           0x00A0
#define PEF7071_LED0L_BLINKS_ABIST          0x00B0
#define PEF7071_LED0L_BLINKS_CDIAG          0x00C0
#define PEF7071_LED0L_PULSE                 0x000F
#define PEF7071_LED0L_PULSE_NONE            0x0000
#define PEF7071_LED0L_PULSE_TXACT           0x0001
#define PEF7071_LED0L_PULSE_RXACT           0x0002
#define PEF7071_LED0L_PULSE_COL             0x0004

//Configuration for LED Pin 1 H register
#define PEF7071_LED1H_CON                   0x00F0
#define PEF7071_LED1H_CON_NONE              0x0000
#define PEF7071_LED1H_CON_LINK10            0x0010
#define PEF7071_LED1H_CON_LINK100           0x0020
#define PEF7071_LED1H_CON_LINK10X           0x0030
#define PEF7071_LED1H_CON_LINK1000          0x0040
#define PEF7071_LED1H_CON_LINK10_0          0x0050
#define PEF7071_LED1H_CON_LINK100X          0x0060
#define PEF7071_LED1H_CON_LINK10XX          0x0070
#define PEF7071_LED1H_CON_PDOWN             0x0080
#define PEF7071_LED1H_CON_EEE               0x0090
#define PEF7071_LED1H_CON_ANEG              0x00A0
#define PEF7071_LED1H_CON_ABIST             0x00B0
#define PEF7071_LED1H_CON_CDIAG             0x00C0
#define PEF7071_LED1H_CON_COPPER            0x00D0
#define PEF7071_LED1H_CON_FIBER             0x00E0
#define PEF7071_LED1H_BLINKF                0x000F
#define PEF7071_LED1H_BLINKF_NONE           0x0000
#define PEF7071_LED1H_BLINKF_LINK10         0x0001
#define PEF7071_LED1H_BLINKF_LINK100        0x0002
#define PEF7071_LED1H_BLINKF_LINK10X        0x0003
#define PEF7071_LED1H_BLINKF_LINK1000       0x0004
#define PEF7071_LED1H_BLINKF_LINK10_0       0x0005
#define PEF7071_LED1H_BLINKF_LINK100X       0x0006
#define PEF7071_LED1H_BLINKF_LINK10XX       0x0007
#define PEF7071_LED1H_BLINKF_PDOWN          0x0008
#define PEF7071_LED1H_BLINKF_EEE            0x0009
#define PEF7071_LED1H_BLINKF_ANEG           0x000A
#define PEF7071_LED1H_BLINKF_ABIST          0x000B
#define PEF7071_LED1H_BLINKF_CDIAG          0x000C

//Configuration for LED Pin 1 L register
#define PEF7071_LED1L_BLINKS                0x00F0
#define PEF7071_LED1L_BLINKS_NONE           0x0000
#define PEF7071_LED1L_BLINKS_LINK10         0x0010
#define PEF7071_LED1L_BLINKS_LINK100        0x0020
#define PEF7071_LED1L_BLINKS_LINK10X        0x0030
#define PEF7071_LED1L_BLINKS_LINK1000       0x0040
#define PEF7071_LED1L_BLINKS_LINK10_0       0x0050
#define PEF7071_LED1L_BLINKS_LINK100X       0x0060
#define PEF7071_LED1L_BLINKS_LINK10XX       0x0070
#define PEF7071_LED1L_BLINKS_PDOWN          0x0080
#define PEF7071_LED1L_BLINKS_EEE            0x0090
#define PEF7071_LED1L_BLINKS_ANEG           0x00A0
#define PEF7071_LED1L_BLINKS_ABIST          0x00B0
#define PEF7071_LED1L_BLINKS_CDIAG          0x00C0
#define PEF7071_LED1L_PULSE                 0x000F
#define PEF7071_LED1L_PULSE_NONE            0x0000
#define PEF7071_LED1L_PULSE_TXACT           0x0001
#define PEF7071_LED1L_PULSE_RXACT           0x0002
#define PEF7071_LED1L_PULSE_COL             0x0004

//Configuration for LED Pin 2 H register
#define PEF7071_LED2H_CON                   0x00F0
#define PEF7071_LED2H_CON_NONE              0x0000
#define PEF7071_LED2H_CON_LINK10            0x0010
#define PEF7071_LED2H_CON_LINK100           0x0020
#define PEF7071_LED2H_CON_LINK10X           0x0030
#define PEF7071_LED2H_CON_LINK1000          0x0040
#define PEF7071_LED2H_CON_LINK10_0          0x0050
#define PEF7071_LED2H_CON_LINK100X          0x0060
#define PEF7071_LED2H_CON_LINK10XX          0x0070
#define PEF7071_LED2H_CON_PDOWN             0x0080
#define PEF7071_LED2H_CON_EEE               0x0090
#define PEF7071_LED2H_CON_ANEG              0x00A0
#define PEF7071_LED2H_CON_ABIST             0x00B0
#define PEF7071_LED2H_CON_CDIAG             0x00C0
#define PEF7071_LED2H_CON_COPPER            0x00D0
#define PEF7071_LED2H_CON_FIBER             0x00E0
#define PEF7071_LED2H_BLINKF                0x000F
#define PEF7071_LED2H_BLINKF_NONE           0x0000
#define PEF7071_LED2H_BLINKF_LINK10         0x0001
#define PEF7071_LED2H_BLINKF_LINK100        0x0002
#define PEF7071_LED2H_BLINKF_LINK10X        0x0003
#define PEF7071_LED2H_BLINKF_LINK1000       0x0004
#define PEF7071_LED2H_BLINKF_LINK10_0       0x0005
#define PEF7071_LED2H_BLINKF_LINK100X       0x0006
#define PEF7071_LED2H_BLINKF_LINK10XX       0x0007
#define PEF7071_LED2H_BLINKF_PDOWN          0x0008
#define PEF7071_LED2H_BLINKF_EEE            0x0009
#define PEF7071_LED2H_BLINKF_ANEG           0x000A
#define PEF7071_LED2H_BLINKF_ABIST          0x000B
#define PEF7071_LED2H_BLINKF_CDIAG          0x000C

//Configuration for LED Pin 2 L register
#define PEF7071_LED2L_BLINKS                0x00F0
#define PEF7071_LED2L_BLINKS_NONE           0x0000
#define PEF7071_LED2L_BLINKS_LINK10         0x0010
#define PEF7071_LED2L_BLINKS_LINK100        0x0020
#define PEF7071_LED2L_BLINKS_LINK10X        0x0030
#define PEF7071_LED2L_BLINKS_LINK1000       0x0040
#define PEF7071_LED2L_BLINKS_LINK10_0       0x0050
#define PEF7071_LED2L_BLINKS_LINK100X       0x0060
#define PEF7071_LED2L_BLINKS_LINK10XX       0x0070
#define PEF7071_LED2L_BLINKS_PDOWN          0x0080
#define PEF7071_LED2L_BLINKS_EEE            0x0090
#define PEF7071_LED2L_BLINKS_ANEG           0x00A0
#define PEF7071_LED2L_BLINKS_ABIST          0x00B0
#define PEF7071_LED2L_BLINKS_CDIAG          0x00C0
#define PEF7071_LED2L_PULSE                 0x000F
#define PEF7071_LED2L_PULSE_NONE            0x0000
#define PEF7071_LED2L_PULSE_TXACT           0x0001
#define PEF7071_LED2L_PULSE_RXACT           0x0002
#define PEF7071_LED2L_PULSE_COL             0x0004

//EEE Link-Fail Counter H register
#define PEF7071_EEE_RXERR_LINK_FAIL_H_VAL   0x00FF

//EEE Link-Fail Counter L register
#define PEF7071_EEE_RXERR_LINK_FAIL_L_VAL   0x00FF

//MII2 Control register
#define PEF7071_MII2CTRL_RXSKEW             0x0070
#define PEF7071_MII2CTRL_RXSKEW_SKEW_0N0    0x0000
#define PEF7071_MII2CTRL_RXSKEW_SKEW_0N5    0x0010
#define PEF7071_MII2CTRL_RXSKEW_SKEW_1N0    0x0020
#define PEF7071_MII2CTRL_RXSKEW_SKEW_1N5    0x0030
#define PEF7071_MII2CTRL_RXSKEW_SKEW_2N0    0x0040
#define PEF7071_MII2CTRL_RXSKEW_SKEW_2N5    0x0050
#define PEF7071_MII2CTRL_RXSKEW_SKEW_3N0    0x0060
#define PEF7071_MII2CTRL_RXSKEW_SKEW_3N5    0x0070
#define PEF7071_MII2CTRL_TXSKEW             0x0007
#define PEF7071_MII2CTRL_TXSKEW_SKEW_0N0    0x0000
#define PEF7071_MII2CTRL_TXSKEW_SKEW_0N5    0x0001
#define PEF7071_MII2CTRL_TXSKEW_SKEW_1N0    0x0002
#define PEF7071_MII2CTRL_TXSKEW_SKEW_1N5    0x0003
#define PEF7071_MII2CTRL_TXSKEW_SKEW_2N0    0x0004
#define PEF7071_MII2CTRL_TXSKEW_SKEW_2N5    0x0005
#define PEF7071_MII2CTRL_TXSKEW_SKEW_3N0    0x0006
#define PEF7071_MII2CTRL_TXSKEW_SKEW_3N5    0x0007

//Legacy LPI Configuration 0 register
#define PEF7071_LEG_LPI_CFG0_HOLDOFF_100BT  0x00FF

//Legacy LPI Configuration 1 register
#define PEF7071_LEG_LPI_CFG1_HOLDOFF_1000BT 0x00FF

//Wake-On-LAN Control register
#define PEF7071_WOLCTRL_SPWD_EN             0x0004
#define PEF7071_WOLCTRL_RES                 0x0002
#define PEF7071_WOLCTRL_EN                  0x0001

//Wake-On-LAN Address Byte 0 register
#define PEF7071_WOLAD0_VAL                  0x00FF

//Wake-On-LAN Address Byte 1 register
#define PEF7071_WOLAD1_VAL                  0x00FF

//Wake-On-LAN Address Byte 2 register
#define PEF7071_WOLAD2_VAL                  0x00FF

//Wake-On-LAN Address Byte 3 register
#define PEF7071_WOLAD3_VAL                  0x00FF

//Wake-On-LAN Address Byte 4 register
#define PEF7071_WOLAD4_VAL                  0x00FF

//Wake-On-LAN Address Byte 5 register
#define PEF7071_WOLAD5_VAL                  0x00FF

//Wake-On-LAN SecureON Password Byte 0 register
#define PEF7071_WOLPW0_VAL                  0x00FF

//Wake-On-LAN SecureON Password Byte 1 register
#define PEF7071_WOLPW1_VAL                  0x00FF

//Wake-On-LAN SecureON Password Byte 2 register
#define PEF7071_WOLPW2_VAL                  0x00FF

//Wake-On-LAN SecureON Password Byte 3 register
#define PEF7071_WOLPW3_VAL                  0x00FF

//Wake-On-LAN SecureON Password Byte 4 register
#define PEF7071_WOLPW4_VAL                  0x00FF

//Wake-On-LAN SecureON Password Byte 5 register
#define PEF7071_WOLPW5_VAL                  0x00FF

//Legacy LPI Configuration 2 register
#define PEF7071_LEG_LPI_CFG2_IPG            0x00FF
#define PEF7071_LEG_LPI_CFG2_IPG_DEFAULT    0x000E

//Legacy LPI Configuration 3 register
#define PEF7071_LEG_LPI_CFG3_IDLE           0x00FF
#define PEF7071_LEG_LPI_CFG3_IDLE_DEFAULT   0x0040

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//PEF7071 Ethernet PHY driver
extern const PhyDriver pef7071PhyDriver;

//PEF7071 related functions
error_t pef7071Init(NetInterface *interface);

void pef7071Tick(NetInterface *interface);

void pef7071EnableIrq(NetInterface *interface);
void pef7071DisableIrq(NetInterface *interface);

void pef7071EventHandler(NetInterface *interface);

void pef7071WritePhyReg(NetInterface *interface, uint8_t address,
   uint16_t data);

uint16_t pef7071ReadPhyReg(NetInterface *interface, uint8_t address);

void pef7071DumpPhyReg(NetInterface *interface);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
