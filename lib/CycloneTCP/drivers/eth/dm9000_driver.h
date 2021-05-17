/**
 * @file dm9000_driver.h
 * @brief DM9000A/B Ethernet controller
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

#ifndef _DM9000_DRIVER_H
#define _DM9000_DRIVER_H

//Dependencies
#include "core/ethernet.h"

//Loopback mode
#ifndef DM9000_LOOPBACK_MODE
   #define DM9000_LOOPBACK_MODE DISABLED
#elif (DM9000_LOOPBACK_MODE != ENABLED && DM9000_LOOPBACK_MODE != DISABLED)
   #error DM9000_LOOPBACK_MODE parameter is not valid
#endif

//TX buffer size
#ifndef DM9000_ETH_TX_BUFFER_SIZE
   #define DM9000_ETH_TX_BUFFER_SIZE 1536
#elif (DM9000_ETH_TX_BUFFER_SIZE != 1536)
   #error DM9000_ETH_TX_BUFFER_SIZE parameter is not valid
#endif

//RX buffer size
#ifndef DM9000_ETH_RX_BUFFER_SIZE
   #define DM9000_ETH_RX_BUFFER_SIZE 1536
#elif (DM9000_ETH_RX_BUFFER_SIZE != 1536)
   #error DM9000_ETH_RX_BUFFER_SIZE parameter is not valid
#endif

//DM9000 index register
#ifndef DM9000_INDEX_REG
   #define DM9000_INDEX_REG *((volatile uint16_t *) 0x30000000)
#endif

//DM9000 data register
#ifndef DM9000_DATA_REG
   #define DM9000_DATA_REG *((volatile uint16_t *) 0x30001000)
#endif

//DM9000 identifiers
#define DM9000_VID ((DM9000_VIDH_DEFAULT << 8) | DM9000_VIDL_DEFAULT)
#define DM9000_PID ((DM9000_PIDH_DEFAULT << 8) | DM9000_PIDL_DEFAULT)

//DM9000 registers
#define DM9000_NCR                      0x00
#define DM9000_NSR                      0x01
#define DM9000_TCR                      0x02
#define DM9000_TSR1                     0x03
#define DM9000_TSR2                     0x04
#define DM9000_RCR                      0x05
#define DM9000_RSR                      0x06
#define DM9000_ROCR                     0x07
#define DM9000_BPTR                     0x08
#define DM9000_FCTR                     0x09
#define DM9000_FCR                      0x0A
#define DM9000_EPCR                     0x0B
#define DM9000_EPAR                     0x0C
#define DM9000_EPDRL                    0x0D
#define DM9000_EPDRH                    0x0E
#define DM9000_WCR                      0x0F
#define DM9000_PAR0                     0x10
#define DM9000_PAR1                     0x11
#define DM9000_PAR2                     0x12
#define DM9000_PAR3                     0x13
#define DM9000_PAR4                     0x14
#define DM9000_PAR5                     0x15
#define DM9000_MAR0                     0x16
#define DM9000_MAR1                     0x17
#define DM9000_MAR2                     0x18
#define DM9000_MAR3                     0x19
#define DM9000_MAR4                     0x1A
#define DM9000_MAR5                     0x1B
#define DM9000_MAR6                     0x1C
#define DM9000_MAR7                     0x1D
#define DM9000_GPCR                     0x1E
#define DM9000_GPR                      0x1F
#define DM9000_TRPAL                    0x22
#define DM9000_TRPAH                    0x23
#define DM9000_RWPAL                    0x24
#define DM9000_RWPAH                    0x25
#define DM9000_VIDL                     0x28
#define DM9000_VIDH                     0x29
#define DM9000_PIDL                     0x2A
#define DM9000_PIDH                     0x2B
#define DM9000_CHIPR                    0x2C
#define DM9000_TCR2                     0x2D
#define DM9000_OCR                      0x2E
#define DM9000_SMCR                     0x2F
#define DM9000_ETXCSR                   0x30
#define DM9000_TCSCR                    0x31
#define DM9000_RCSCSR                   0x32
#define DM9000_MPAR                     0x33
#define DM9000_LEDCR                    0x34
#define DM9000_BUSCR                    0x38
#define DM9000_INTCR                    0x39
#define DM9000_SCCR                     0x50
#define DM9000_RSCCR                    0x51
#define DM9000_MRCMDX                   0xF0
#define DM9000_MRCMDX1                  0xF1
#define DM9000_MRCMD                    0xF2
#define DM9000_MRRL                     0xF4
#define DM9000_MRRH                     0xF5
#define DM9000_MWCMDX                   0xF6
#define DM9000_MWCMD                    0xF8
#define DM9000_MWRL                     0xFA
#define DM9000_MWRH                     0xFB
#define DM9000_TXPLL                    0xFC
#define DM9000_TXPLH                    0xFD
#define DM9000_ISR                      0xFE
#define DM9000_IMR                      0xFF

//DM9000 PHY registers
#define DM9000_BMCR                     0x00
#define DM9000_BMSR                     0x01
#define DM9000_PHYIDR1                  0x02
#define DM9000_PHYIDR2                  0x03
#define DM9000_ANAR                     0x04
#define DM9000_ANLPAR                   0x05
#define DM9000_ANER                     0x06
#define DM9000_DSCR                     0x10
#define DM9000_DSCSR                    0x11
#define DM9000_10BTCSR                  0x12
#define DM9000_PWDOR                    0x13
#define DM9000_SCR                      0x14
#define DM9000_DSPCR                    0x1B
#define DM9000_PSCR                     0x1D

//Network Control register
#define DM9000_NCR_WAKEEN               0x40
#define DM9000_NCR_FCOL                 0x10
#define DM9000_NCR_FDX                  0x08
#define DM9000_NCR_LBK                  0x06
#define DM9000_NCR_LBK_NORMAL           0x00
#define DM9000_NCR_LBK_MAC              0x02
#define DM9000_NCR_LBK_PHY              0x04
#define DM9000_NCR_RST                  0x01

//Network Status register
#define DM9000_NSR_SPEED                0x80
#define DM9000_NSR_LINKST               0x40
#define DM9000_NSR_WAKEST               0x20
#define DM9000_NSR_TX2END               0x08
#define DM9000_NSR_TX1END               0x04
#define DM9000_NSR_RXOV                 0x02

//TX Control register
#define DM9000_TCR_TJDIS                0x40
#define DM9000_TCR_EXCECM               0x20
#define DM9000_TCR_PAD_DIS2             0x10
#define DM9000_TCR_CRC_DIS2             0x08
#define DM9000_TCR_PAD_DIS1             0x04
#define DM9000_TCR_CRC_DIS1             0x02
#define DM9000_TCR_TXREQ                0x01

//TX Status 1 register
#define DM9000_TSR1_TJTO                0x80
#define DM9000_TSR1_LC                  0x40
#define DM9000_TSR1_NC                  0x20
#define DM9000_TSR1_LCOL                0x10
#define DM9000_TSR1_COL                 0x08
#define DM9000_TSR1_EC                  0x04

//TX Status 2 register
#define DM9000_TSR2_TJTO                0x80
#define DM9000_TSR2_LC                  0x40
#define DM9000_TSR2_NC                  0x20
#define DM9000_TSR2_LCOL                0x10
#define DM9000_TSR2_COL                 0x08
#define DM9000_TSR2_EC                  0x04

//RX Control register
#define DM9000_RCR_WTDIS                0x40
#define DM9000_RCR_DIS_LONG             0x20
#define DM9000_RCR_DIS_CRC              0x10
#define DM9000_RCR_ALL                  0x08
#define DM9000_RCR_RUNT                 0x04
#define DM9000_RCR_PRMSC                0x02
#define DM9000_RCR_RXEN                 0x01

//RX Status register
#define DM9000_RSR_RF                   0x80
#define DM9000_RSR_MF                   0x40
#define DM9000_RSR_LCS                  0x20
#define DM9000_RSR_RWTO                 0x10
#define DM9000_RSR_PLE                  0x08
#define DM9000_RSR_AE                   0x04
#define DM9000_RSR_CE                   0x02
#define DM9000_RSR_FOE                  0x01

//Receive Overflow Counter register
#define DM9000_ROCR_RXFU                0x80
#define DM9000_ROCR_ROC                 0x7F

//Back Pressure Threshold register
#define DM9000_BPTR_BPHW                0xF0
#define DM9000_BPTR_JPT                 0x0F

//Flow Control Threshold register
#define DM9000_FCTR_HWOT                0xF0
#define DM9000_FCTR_LWOT                0x0F

//RX Flow Control register
#define DM9000_FCR_TXP0                 0x80
#define DM9000_FCR_TXPF                 0x40
#define DM9000_FCR_TXPEN                0x20
#define DM9000_FCR_BKPA                 0x10
#define DM9000_FCR_BKPM                 0x08
#define DM9000_FCR_RXPS                 0x04
#define DM9000_FCR_RXPCS                0x02
#define DM9000_FCR_FLCE                 0x01

//EEPROM & PHY Control register
#define DM9000_EPCR_REEP                0x20
#define DM9000_EPCR_WEP                 0x10
#define DM9000_EPCR_EPOS                0x08
#define DM9000_EPCR_ERPRR               0x04
#define DM9000_EPCR_ERPRW               0x02
#define DM9000_EPCR_ERRE                0x01

//EEPROM & PHY Address register
#define DM9000_EPAR_PHY_ADR             0xC0
#define DM9000_EPAR_EROA                0x3F

//Wake Up Control register
#define DM9000_WCR_LINKEN               0x20
#define DM9000_WCR_SAMPLEEN             0x10
#define DM9000_WCR_MAGICEN              0x08
#define DM9000_WCR_LINKST               0x04
#define DM9000_WCR_SAMPLEST             0x02
#define DM9000_WCR_MAGICST              0x01

//General Purpose Control register
#define DM9000_GPCR_GPC6                0x40
#define DM9000_GPCR_GPC5                0x20
#define DM9000_GPCR_GPC4                0x10
#define DM9000_GPCR_GPC3                0x08
#define DM9000_GPCR_GPC2                0x04
#define DM9000_GPCR_GPC1                0x02

//General Purpose register
#define DM9000_GPR_GPO6                 0x40
#define DM9000_GPR_GPO5                 0x20
#define DM9000_GPR_GPO4                 0x10
#define DM9000_GPR_GPIO3                0x08
#define DM9000_GPR_GPIO2                0x04
#define DM9000_GPR_GPIO1                0x02
#define DM9000_GPR_PHYPD                0x01

//Vendor ID Low Byte register
#define DM9000_VIDL_DEFAULT             0x46

//Vendor ID High Byte register
#define DM9000_VIDH_DEFAULT             0x0A

//Product ID Low Byte register
#define DM9000_PIDL_DEFAULT             0x00

//Product ID High Byte register
#define DM9000_PIDH_DEFAULT             0x90

//Chip Revision register
#define DM9000_CHIPR_REV_A              0x19
#define DM9000_CHIPR_REV_B              0x1A

//TX Control 2 register
#define DM9000_TCR2_LED                 0x80
#define DM9000_TCR2_RLCP                0x40
#define DM9000_TCR2_DTU                 0x20
#define DM9000_TCR2_ONEPM               0x10
#define DM9000_TCR2_IFGS                0x0F
#define DM9000_TCR2_IFGS_64_BIT         0x08
#define DM9000_TCR2_IFGS_72_BIT         0x09
#define DM9000_TCR2_IFGS_80_BIT         0x0A
#define DM9000_TCR2_IFGS_88_BIT         0x0B
#define DM9000_TCR2_IFGS_96_BIT         0x0C
#define DM9000_TCR2_IFGS_104_BIT        0x0D
#define DM9000_TCR2_IFGS_112_BIT        0x0E
#define DM9000_TCR2_IFGS_120_BIT        0x0F

//Operation Control register
#define DM9000_OCR_SCC                  0xC0
#define DM9000_OCR_SCC_50MHZ            0x00
#define DM9000_OCR_SCC_20MHZ            0x40
#define DM9000_OCR_SCC_100MHZ           0x80
#define DM9000_OCR_SOE                  0x10
#define DM9000_OCR_SCS                  0x08
#define DM9000_OCR_PHYOP                0x07

//Special Mode Control register
#define DM9000_SMCR_SM_EN               0x80
#define DM9000_SMCR_FLC                 0x04
#define DM9000_SMCR_FB1                 0x02
#define DM9000_SMCR_FB0                 0x01

//Early Transmit Control/Status register
#define DM9000_ETXCSR_ETE               0x80
#define DM9000_ETXCSR_ETS2              0x40
#define DM9000_ETXCSR_ETS1              0x20
#define DM9000_ETXCSR_ETT               0x03
#define DM9000_ETXCSR_ETT_12_5_PERCENT  0x00
#define DM9000_ETXCSR_ETT_25_PERCENT    0x01
#define DM9000_ETXCSR_ETT_50_PERCENT    0x02
#define DM9000_ETXCSR_ETT_75_PERCENT    0x03

//Transmit Check Sum Control register
#define DM9000_TCSCR_UDPCSE             0x04
#define DM9000_TCSCR_TCPCSE             0x02
#define DM9000_TCSCR_IPCSE              0x01

//Receive Check Sum Control Status register
#define DM9000_RCSCSR_UDPS              0x80
#define DM9000_RCSCSR_TCPS              0x40
#define DM9000_RCSCSR_IPS               0x20
#define DM9000_RCSCSR_UDPP              0x10
#define DM9000_RCSCSR_TCPP              0x08
#define DM9000_RCSCSR_IPP               0x04
#define DM9000_RCSCSR_RCSEN             0x02
#define DM9000_RCSCSR_DCSE              0x01

//MII PHY Address register
#define DM9000_MPAR_ADR_EN              0x80
#define DM9000_MPAR_EPHYADR             0x1F

//LED Pin Control register
#define DM9000_LEDCR_GPIO               0x02
#define DM9000_LEDCR_MII                0x01

//Processor Bus Control register
#define DM9000_BUSCR_CURR               0x60
#define DM9000_BUSCR_CURR_2MA           0x00
#define DM9000_BUSCR_CURR_4MA           0x20
#define DM9000_BUSCR_CURR_6MA           0x40
#define DM9000_BUSCR_CURR_8MA           0x60
#define DM9000_BUSCR_EST                0x08
#define DM9000_BUSCR_IOW_SPIKE          0x02
#define DM9000_BUSCR_IOR_SPIKE          0x01

//INT Pin Control register
#define DM9000_INTCR_INT_TYPE           0x02
#define DM9000_INTCR_INT_TYPE_DIRECT    0x00
#define DM9000_INTCR_INT_TYPE_OC        0x02
#define DM9000_INTCR_INT_POL            0x01
#define DM9000_INTCR_INT_POL_HIGH       0x00
#define DM9000_INTCR_INT_POL_LOW        0x01

//System Clock Turn On Control register
#define DM9000_SCCR_DIS_CLK             0x01

//Interrupt Status register
#define DM9000_ISR_IOMODE               0x80
#define DM9000_ISR_IOMODE_16_BIT        0x00
#define DM9000_ISR_IOMODE_8_BIT         0x80
#define DM9000_ISR_LNKCHG               0x20
#define DM9000_ISR_UDRUN                0x10
#define DM9000_ISR_ROO                  0x08
#define DM9000_ISR_ROS                  0x04
#define DM9000_ISR_PT                   0x02
#define DM9000_ISR_PR                   0x01

//Interrupt Mask register
#define DM9000_IMR_PAR                  0x80
#define DM9000_IMR_LNKCHGI              0x20
#define DM9000_IMR_UDRUNI               0x10
#define DM9000_IMR_ROOI                 0x08
#define DM9000_IMR_ROI                  0x04
#define DM9000_IMR_PTI                  0x02
#define DM9000_IMR_PRI                  0x01

//Basic Mode Control register
#define DM9000_BMCR_RST                 0x8000
#define DM9000_BMCR_LOOPBACK            0x4000
#define DM9000_BMCR_SPEED_SEL           0x2000
#define DM9000_BMCR_AN_EN               0x1000
#define DM9000_BMCR_POWER_DOWN          0x0800
#define DM9000_BMCR_ISOLATE             0x0400
#define DM9000_BMCR_RESTART_AN          0x0200
#define DM9000_BMCR_DUPLEX_MODE         0x0100
#define DM9000_BMCR_COL_TEST            0x0080

//Basic Mode Status register
#define DM9000_BMSR_100BT4              0x8000
#define DM9000_BMSR_100BTX_FD           0x4000
#define DM9000_BMSR_100BTX_HD           0x2000
#define DM9000_BMSR_10BT_FD             0x1000
#define DM9000_BMSR_10BT_HD             0x0800
#define DM9000_BMSR_MF_PREAMBLE_SUPPR   0x0040
#define DM9000_BMSR_AN_COMPLETE         0x0020
#define DM9000_BMSR_REMOTE_FAULT        0x0010
#define DM9000_BMSR_AN_CAPABLE          0x0008
#define DM9000_BMSR_LINK_STATUS         0x0004
#define DM9000_BMSR_JABBER_DETECT       0x0002
#define DM9000_BMSR_EXTENDED_CAPABLE    0x0001

//PHY ID Identifier 1 register
#define DM9000_PHYIDR1_OUI_MSB          0xFFFF
#define DM9000_PHYIDR1_OUI_MSB_DEFAULT  0x0181

//PHY ID Identifier 2 register
#define DM9000_PHYIDR2_OUI_LSB          0xFC00
#define DM9000_PHYIDR2_OUI_LSB_DEFAULT  0xB800
#define DM9000_PHYIDR2_VNDR_MDL         0x03F0
#define DM9000_PHYIDR2_VNDR_MDL_DEFAULT 0x0070
#define DM9000_PHYIDR2_MDL_REV          0x000F
#define DM9000_PHYIDR2_MDL_REV_DEFAULT  0x0000

//Auto-Negotiation Advertisement register
#define DM9000_ANAR_NP                  0x8000
#define DM9000_ANAR_ACK                 0x4000
#define DM9000_ANAR_RF                  0x2000
#define DM9000_ANAR_FCS                 0x0400
#define DM9000_ANAR_100BT4              0x0200
#define DM9000_ANAR_100BTX_FD           0x0100
#define DM9000_ANAR_100BTX_HD           0x0080
#define DM9000_ANAR_10BT_FD             0x0040
#define DM9000_ANAR_10BT_HD             0x0020
#define DM9000_ANAR_SELECTOR            0x001F
#define DM9000_ANAR_SELECTOR_DEFAULT    0x0001

//Auto-Negotiation Link Partner Ability register
#define DM9000_ANLPAR_NP                0x8000
#define DM9000_ANLPAR_ACK               0x4000
#define DM9000_ANLPAR_RF                0x2000
#define DM9000_ANLPAR_FCS               0x0400
#define DM9000_ANLPAR_100BT4            0x0200
#define DM9000_ANLPAR_100BTX_FD         0x0100
#define DM9000_ANLPAR_100BTX_HD         0x0080
#define DM9000_ANLPAR_10BT_FD           0x0040
#define DM9000_ANLPAR_10BT_HD           0x0020
#define DM9000_ANLPAR_SELECTOR          0x001F
#define DM9000_ANLPAR_SELECTOR_DEFAULT  0x0001

//Auto-Negotiation Expansion register
#define DM9000_ANER_PDF                 0x0010
#define DM9000_ANER_LP_NP_ABLE          0x0008
#define DM9000_ANER_NP_ABLE             0x0004
#define DM9000_ANER_PAGE_RX             0x0002
#define DM9000_ANER_LP_AN_ABLE          0x0001

//Davicom Specified Configuration register
#define DM9000_DSCR_BP_4B5B             0x8000
#define DM9000_DSCR_BP_SCR              0x4000
#define DM9000_DSCR_BP_ALIGN            0x2000
#define DM9000_DSCR_BP_ADPOK            0x1000
#define DM9000_DSCR_TX_FX               0x0400
#define DM9000_DSCR_F_LINK_100          0x0080
#define DM9000_DSCR_SPLED_CTL           0x0040
#define DM9000_DSCR_COLLED_CTL          0x0020
#define DM9000_DSCR_RPDCTR_EN           0x0010
#define DM9000_DSCR_SMRST               0x0008
#define DM9000_DSCR_MFPSC               0x0004
#define DM9000_DSCR_SLEEP               0x0002
#define DM9000_DSCR_RLOUT               0x0001

//Davicom Specified Configuration/Status register
#define DM9000_DSCSR_100FDX             0x8000
#define DM9000_DSCSR_100HDX             0x4000
#define DM9000_DSCSR_10FDX              0x2000
#define DM9000_DSCSR_10HDX              0x1000
#define DM9000_DSCSR_PHYADR             0x01F0
#define DM9000_DSCSR_ANMB               0x000F

//10BASE-T Configuration/Status register
#define DM9000_10BTCSR_LP_EN            0x4000
#define DM9000_10BTCSR_HBE              0x2000
#define DM9000_10BTCSR_SQUELCH          0x1000
#define DM9000_10BTCSR_JABEN            0x0800
#define DM9000_10BTCSR_POLR             0x0001

//Power Down Control register
#define DM9000_PWDOR_PD10DRV            0x0100
#define DM9000_PWDOR_PD100DL            0x0080
#define DM9000_PWDOR_PDCHIP             0x0040
#define DM9000_PWDOR_PDCOM              0x0020
#define DM9000_PWDOR_PDAEQ              0x0010
#define DM9000_PWDOR_PDDRV              0x0008
#define DM9000_PWDOR_PDEDI              0x0004
#define DM9000_PWDOR_PDEDO              0x0002
#define DM9000_PWDOR_PD10               0x0001

//Specified Configuration register
#define DM9000_SCR_TSTSE1               0x8000
#define DM9000_SCR_TSTSE2               0x4000
#define DM9000_SCR_FORCE_TXSD           0x2000
#define DM9000_SCR_FORCE_FEF            0x1000
#define DM9000_SCR_PREAMBLEX            0x0800
#define DM9000_SCR_TX10M_PWR            0x0400
#define DM9000_SCR_NWAY_PWR             0x0200
#define DM9000_SCR_MDIX_CNTL            0x0080
#define DM9000_SCR_AUTONEG_LPBK         0x0040
#define DM9000_SCR_MDIX_FIX             0x0020
#define DM9000_SCR_MDIX_DOWN            0x0010
#define DM9000_SCR_MONSEL1              0x0008
#define DM9000_SCR_MONSEL0              0x0004
#define DM9000_SCR_PD_VALUE             0x0001

//DSP Control register
#define DM9000_DSPCR_DSP                0xFFFF

//Power Saving Control register
#define DM9000_PSCR_PREAMBLEX           0x0800
#define DM9000_PSCR_AMPLITUDE           0x0400
#define DM9000_PSCR_TX_PWR              0x0200

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief DM9000 driver context
 **/

typedef struct
{
   uint_t queuedPackets; ///<Number of packets in transmission buffer
} Dm9000Context;


//DM9000 driver
extern const NicDriver dm9000Driver;

//DM9000 related functions
error_t dm9000Init(NetInterface *interface);

void dm9000Tick(NetInterface *interface);

void dm9000EnableIrq(NetInterface *interface);
void dm9000DisableIrq(NetInterface *interface);
bool_t dm9000IrqHandler(NetInterface *interface);
void dm9000EventHandler(NetInterface *interface);

error_t dm9000SendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t dm9000ReceivePacket(NetInterface *interface);

error_t dm9000UpdateMacAddrFilter(NetInterface *interface);

void dm9000WriteReg(uint8_t address, uint8_t data);
uint8_t dm9000ReadReg(uint8_t address);

void dm9000WritePhyReg(uint8_t address, uint16_t data);
uint16_t dm9000ReadPhyReg(uint8_t address);

uint32_t dm9000CalcCrc(const void *data, size_t length);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
