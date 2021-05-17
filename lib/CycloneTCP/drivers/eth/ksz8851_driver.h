/**
 * @file ksz8851_driver.h
 * @brief KSZ8851 Ethernet controller
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

#ifndef _KSZ8851_DRIVER_H
#define _KSZ8851_DRIVER_H

//SPI interface support
#ifndef KSZ8851_SPI_SUPPORT
   #define KSZ8851_SPI_SUPPORT ENABLED
#elif (KSZ8851_SPI_SUPPORT != ENABLED && KSZ8851_SPI_SUPPORT != DISABLED)
   #error KSZ8851_SPI_SUPPORT parameter is not valid
#endif

//TX buffer size
#ifndef KSZ8851_ETH_TX_BUFFER_SIZE
   #define KSZ8851_ETH_TX_BUFFER_SIZE 1536
#elif (KSZ8851_ETH_TX_BUFFER_SIZE != 1536)
   #error KSZ8851_ETH_TX_BUFFER_SIZE parameter is not valid
#endif

//RX buffer size
#ifndef KSZ8851_ETH_RX_BUFFER_SIZE
   #define KSZ8851_ETH_RX_BUFFER_SIZE 1536
#elif (KSZ8851_ETH_RX_BUFFER_SIZE != 1536)
   #error KSZ8851_ETH_RX_BUFFER_SIZE parameter is not valid
#endif

//KSZ8851 data register
#ifndef KSZ8851_DATA_REG
   #define KSZ8851_DATA_REG *((volatile uint16_t *) 0x60000000)
#endif

//KSZ8851 command register
#ifndef KSZ8851_CMD_REG
   #define KSZ8851_CMD_REG *((volatile uint16_t *) 0x60000004)
#endif

//SPI command set
#define KSZ8851_CMD_RD_REG  0x00
#define KSZ8851_CMD_WR_REG  0x40
#define KSZ8851_CMD_RD_FIFO 0x80
#define KSZ8851_CMD_WR_FIFO 0xC0

//Byte enable bits
#if (KSZ8851_SPI_SUPPORT == ENABLED)
   #define KSZ8851_CMD_B0 0x04
   #define KSZ8851_CMD_B1 0x08
   #define KSZ8851_CMD_B2 0x10
   #define KSZ8851_CMD_B3 0x20
#else
   #define KSZ8851_CMD_B0 0x1000
   #define KSZ8851_CMD_B1 0x2000
   #define KSZ8851_CMD_B2 0x4000
   #define KSZ8851_CMD_B3 0x8000
#endif

//KSZ8851 registers
#define KSZ8851_CCR                                   0x08
#define KSZ8851_MARL                                  0x10
#define KSZ8851_MARM                                  0x12
#define KSZ8851_MARH                                  0x14
#define KSZ8851_OBCR                                  0x20
#define KSZ8851_EEPCR                                 0x22
#define KSZ8851_MBIR                                  0x24
#define KSZ8851_GRR                                   0x26
#define KSZ8851_WFCR                                  0x2A
#define KSZ8851_WF0CRC0                               0x30
#define KSZ8851_WF0CRC1                               0x32
#define KSZ8851_WF0BM0                                0x34
#define KSZ8851_WF0BM1                                0x36
#define KSZ8851_WF0BM2                                0x38
#define KSZ8851_WF0BM3                                0x3A
#define KSZ8851_WF1CRC0                               0x40
#define KSZ8851_WF1CRC1                               0x42
#define KSZ8851_WF1BM0                                0x44
#define KSZ8851_WF1BM1                                0x46
#define KSZ8851_WF1BM2                                0x48
#define KSZ8851_WF1BM3                                0x4A
#define KSZ8851_WF2CRC0                               0x50
#define KSZ8851_WF2CRC1                               0x52
#define KSZ8851_WF2BM0                                0x54
#define KSZ8851_WF2BM1                                0x56
#define KSZ8851_WF2BM2                                0x58
#define KSZ8851_WF2BM3                                0x5A
#define KSZ8851_WF3CRC0                               0x60
#define KSZ8851_WF3CRC1                               0x62
#define KSZ8851_WF3BM0                                0x64
#define KSZ8851_WF3BM1                                0x66
#define KSZ8851_WF3BM2                                0x68
#define KSZ8851_WF3BM3                                0x6A
#define KSZ8851_TXCR                                  0x70
#define KSZ8851_TXSR                                  0x72
#define KSZ8851_RXCR1                                 0x74
#define KSZ8851_RXCR2                                 0x76
#define KSZ8851_TXMIR                                 0x78
#define KSZ8851_RXFHSR                                0x7C
#define KSZ8851_RXFHBCR                               0x7E
#define KSZ8851_TXQCR                                 0x80
#define KSZ8851_RXQCR                                 0x82
#define KSZ8851_TXFDPR                                0x84
#define KSZ8851_RXFDPR                                0x86
#define KSZ8851_RXDTTR                                0x8C
#define KSZ8851_RXDBCTR                               0x8E
#define KSZ8851_IER                                   0x90
#define KSZ8851_ISR                                   0x92
#define KSZ8851_RXFCTR                                0x9C
#define KSZ8851_TXNTFSR                               0x9E
#define KSZ8851_MAHTR0                                0xA0
#define KSZ8851_MAHTR1                                0xA2
#define KSZ8851_MAHTR2                                0xA4
#define KSZ8851_MAHTR3                                0xA6
#define KSZ8851_FCLWR                                 0xB0
#define KSZ8851_FCHWR                                 0xB2
#define KSZ8851_FCOWR                                 0xB4
#define KSZ8851_CIDER                                 0xC0
#define KSZ8851_CGCR                                  0xC6
#define KSZ8851_IACR                                  0xC8
#define KSZ8851_IADLR                                 0xD0
#define KSZ8851_IADHR                                 0xD2
#define KSZ8851_PMECR                                 0xD4
#define KSZ8851_GSWUTR                                0xD6
#define KSZ8851_PHYRR                                 0xD8
#define KSZ8851_P1MBCR                                0xE4
#define KSZ8851_P1MBSR                                0xE6
#define KSZ8851_PHY1ILR                               0xE8
#define KSZ8851_PHY1IHR                               0xEA
#define KSZ8851_P1ANAR                                0xEC
#define KSZ8851_P1ANLPR                               0xEE
#define KSZ8851_P1SCLMD                               0xF4
#define KSZ8851_P1CR                                  0xF6
#define KSZ8851_P1SR                                  0xF8

//Chip Configuration register
#define KSZ8851_CCR_BUS_ENDIAN_MODE                   0x0400
#define KSZ8851_CCR_EEPROM_PRESENCE                   0x0200
#define KSZ8851_CCR_SPI_BUS_MODE                      0x0100
#define KSZ8851_CCR_8_BIT_DATA_BUS                    0x0080
#define KSZ8851_CCR_16_BIT_DATA_BUS                   0x0040
#define KSZ8851_CCR_32_BIT_DATA_BUS                   0x0020
#define KSZ8851_CCR_SHARED_BUS_MODE                   0x0010
#define KSZ8851_CCR_128_PIN_PACKAGE                   0x0008
#define KSZ8851_CCR_48_PIN_PACKAGE                    0x0002
#define KSZ8851_CCR_32_PIN_PACKAGE                    0x0001

//On-Chip Bus Control register
#define KSZ8851_OBCR_OUT_PIN_DRIVE_STRENGTH           0x0040
#define KSZ8851_OBCR_BUS_CLK_SEL                      0x0004
#define KSZ8851_OBCR_BUS_CLK_DIV                      0x0003
#define KSZ8851_OBCR_BUS_CLK_DIV_1                    0x0000
#define KSZ8851_OBCR_BUS_CLK_DIV_2                    0x0001
#define KSZ8851_OBCR_BUS_CLK_DIV_3                    0x0002

//EEPROM Control register
#define KSZ8851_EEPCR_EESRWA                          0x0020
#define KSZ8851_EEPCR_EESA                            0x0010
#define KSZ8851_EEPCR_EESB                            0x0008
#define KSZ8851_EEPCR_EECB                            0x0007
#define KSZ8851_EEPCR_EECB_EED_IO                     0x0004
#define KSZ8851_EEPCR_EECB_EESK                       0x0002
#define KSZ8851_EEPCR_EECB_EECS                       0x0001

//Memory BIST Info register
#define KSZ8851_MBIR_TXMBF                            0x1000
#define KSZ8851_MBIR_TXMBFA                           0x0800
#define KSZ8851_MBIR_TXMBFC                           0x0700
#define KSZ8851_MBIR_RXMBF                            0x0010
#define KSZ8851_MBIR_RXMBFA                           0x0008
#define KSZ8851_MBIR_RXMBFC                           0x0007

//Global Reset register
#define KSZ8851_GRR_QMU_MODULE_SOFT_RESET             0x0002
#define KSZ8851_GRR_GLOBAL_SOFT_RESET                 0x0001

//Wakeup Frame Control register
#define KSZ8851_WFCR_MPRXE                            0x0080
#define KSZ8851_WFCR_WF3E                             0x0008
#define KSZ8851_WFCR_WF2E                             0x0004
#define KSZ8851_WFCR_WF1E                             0x0002
#define KSZ8851_WFCR_WF0E                             0x0001

//Transmit Control register
#define KSZ8851_TXCR_TCGICMP                          0x0100
#define KSZ8851_TXCR_TCGTCP                           0x0040
#define KSZ8851_TXCR_TCGIP                            0x0020
#define KSZ8851_TXCR_FTXQ                             0x0010
#define KSZ8851_TXCR_TXFCE                            0x0008
#define KSZ8851_TXCR_TXPE                             0x0004
#define KSZ8851_TXCR_TXCE                             0x0002
#define KSZ8851_TXCR_TXE                              0x0001

//Transmit Status register
#define KSZ8851_TXSR_TXLC                             0x2000
#define KSZ8851_TXSR_TXMC                             0x1000
#define KSZ8851_TXSR_TXFID                            0x003F

//Receive Control 1 register
#define KSZ8851_RXCR1_FRXQ                            0x8000
#define KSZ8851_RXCR1_RXUDPFCC                        0x4000
#define KSZ8851_RXCR1_RXTCPFCC                        0x2000
#define KSZ8851_RXCR1_RXIPFCC                         0x1000
#define KSZ8851_RXCR1_RXPAFMA                         0x0800
#define KSZ8851_RXCR1_RXFCE                           0x0400
#define KSZ8851_RXCR1_RXEFE                           0x0200
#define KSZ8851_RXCR1_RXMAFMA                         0x0100
#define KSZ8851_RXCR1_RXBE                            0x0080
#define KSZ8851_RXCR1_RXME                            0x0040
#define KSZ8851_RXCR1_RXUE                            0x0020
#define KSZ8851_RXCR1_RXAE                            0x0010
#define KSZ8851_RXCR1_RXINVF                          0x0002
#define KSZ8851_RXCR1_RXE                             0x0001

//Receive Control 2 register
#define KSZ8851_RXCR2_SRDBL                           0x00E0
#define KSZ8851_RXCR2_SRDBL_4_BYTES                   0x0000
#define KSZ8851_RXCR2_SRDBL_8_BYTES                   0x0020
#define KSZ8851_RXCR2_SRDBL_16_BYTES                  0x0040
#define KSZ8851_RXCR2_SRDBL_32_BYTES                  0x0060
#define KSZ8851_RXCR2_SRDBL_SINGLE_FRAME              0x0080
#define KSZ8851_RXCR2_IUFFP                           0x0010
#define KSZ8851_RXCR2_RXIUFCEZ                        0x0008
#define KSZ8851_RXCR2_UDPLFE                          0x0004
#define KSZ8851_RXCR2_RXICMPFCC                       0x0002
#define KSZ8851_RXCR2_RXSAF                           0x0001

//TXQ Memory Information register
#define KSZ8851_TXMIR_TXMA                            0x1FFF

//Receive Frame Header Status register
#define KSZ8851_RXFHSR_RXFV                           0x8000
#define KSZ8851_RXFHSR_RXICMPFCS                      0x2000
#define KSZ8851_RXFHSR_RXIPFCS                        0x1000
#define KSZ8851_RXFHSR_RXTCPFCS                       0x0800
#define KSZ8851_RXFHSR_RXUDPFCS                       0x0400
#define KSZ8851_RXFHSR_RXBF                           0x0080
#define KSZ8851_RXFHSR_RXMF                           0x0040
#define KSZ8851_RXFHSR_RXUF                           0x0020
#define KSZ8851_RXFHSR_RXMR                           0x0010
#define KSZ8851_RXFHSR_RXFT                           0x0008
#define KSZ8851_RXFHSR_RXFTL                          0x0004
#define KSZ8851_RXFHSR_RXRF                           0x0002
#define KSZ8851_RXFHSR_RXCE                           0x0001

//Receive Frame Header Byte Count register
#define KSZ8851_RXFHBCR_RXBC                          0x0FFF

//TXQ Command register
#define KSZ8851_TXQCR_AETFE                           0x0004
#define KSZ8851_TXQCR_TXQMAM                          0x0002
#define KSZ8851_TXQCR_METFE                           0x0001

//RXQ Command register
#define KSZ8851_RXQCR_RXDTTS                          0x1000
#define KSZ8851_RXQCR_RXDBCTS                         0x0800
#define KSZ8851_RXQCR_RXFCTS                          0x0400
#define KSZ8851_RXQCR_RXIPHTOE                        0x0200
#define KSZ8851_RXQCR_RXDTTE                          0x0080
#define KSZ8851_RXQCR_RXDBCTE                         0x0040
#define KSZ8851_RXQCR_RXFCTE                          0x0020
#define KSZ8851_RXQCR_ADRFE                           0x0010
#define KSZ8851_RXQCR_SDA                             0x0008
#define KSZ8851_RXQCR_RRXEF                           0x0001

//TX Frame Data Pointer register
#define KSZ8851_TXFDPR_TXFPAI                         0x4000
#define KSZ8851_TXFDPR_TXFP                           0x07FF

//RX Frame Data Pointer register
#define KSZ8851_RXFDPR_RXFPAI                         0x4000
#define KSZ8851_RXFDPR_WST                            0x1000
#define KSZ8851_RXFDPR_EMS                            0x0800
#define KSZ8851_RXFDPR_RXFP                           0x07FF

//Interrupt Enable register
#define KSZ8851_IER_LCIE                              0x8000
#define KSZ8851_IER_TXIE                              0x4000
#define KSZ8851_IER_RXIE                              0x2000
#define KSZ8851_IER_RXOIE                             0x0800
#define KSZ8851_IER_TXPSIE                            0x0200
#define KSZ8851_IER_RXPSIE                            0x0100
#define KSZ8851_IER_TXSAIE                            0x0040
#define KSZ8851_IER_RXWFDIE                           0x0020
#define KSZ8851_IER_RXMPDIE                           0x0010
#define KSZ8851_IER_LDIE                              0x0008
#define KSZ8851_IER_EDIE                              0x0004
#define KSZ8851_IER_SPIBEIE                           0x0002
#define KSZ8851_IER_DEDIE                             0x0001

//Interrupt Status register
#define KSZ8851_ISR_LCIS                              0x8000
#define KSZ8851_ISR_TXIS                              0x4000
#define KSZ8851_ISR_RXIS                              0x2000
#define KSZ8851_ISR_RXOIS                             0x0800
#define KSZ8851_ISR_TXPSIS                            0x0200
#define KSZ8851_ISR_RXPSIS                            0x0100
#define KSZ8851_ISR_TXSAIS                            0x0040
#define KSZ8851_ISR_RXWFDIS                           0x0020
#define KSZ8851_ISR_RXMPDIS                           0x0010
#define KSZ8851_ISR_LDIS                              0x0008
#define KSZ8851_ISR_EDIS                              0x0004
#define KSZ8851_ISR_SPIBEIS                           0x0002

//RX Frame Count & Threshold register
#define KSZ8851_RXFCTR_RXFC                           0xFF00
#define KSZ8851_RXFCTR_RXFCT                          0x00FF

//Flow Control Low Watermark register
#define KSZ8851_FCLWR_FCLWC                           0x0FFF

//Flow Control High Watermark register
#define KSZ8851_FCHWR_FCHWC                           0x0FFF

//Flow Control Overrun Watermark register
#define KSZ8851_FCOWR_FCLWC                           0x0FFF

//Chip ID and Enable register
#define KSZ8851_CIDER_FAMILY_ID                       0xFF00
#define KSZ8851_CIDER_FAMILY_ID_DEFAULT               0x8800
#define KSZ8851_CIDER_CHIP_ID                         0x00F0
#define KSZ8851_CIDER_CHIP_ID_DEFAULT                 0x0070
#define KSZ8851_CIDER_REV_ID                          0x000E
#define KSZ8851_CIDER_REV_ID_A2                       0x0000
#define KSZ8851_CIDER_REV_ID_A3                       0x0002

//Chip Global Control register
#define KSZ8851_CGCR_LEDSEL0                          0x0200

//Indirect Access Control register
#define KSZ8851_IACR_READ_EN                          0x1000
#define KSZ8851_IACR_TABLE_SEL                        0x0C00
#define KSZ8851_IACR_TABLE_SEL_MIB_COUNTER            0x0C00
#define KSZ8851_IACR_INDIRECT_ADDR                    0x001F

//Power Management Event Control register
#define KSZ8851_PMECR_PME_DELAY_EN                    0x4000
#define KSZ8851_PMECR_PME_OUT_POLARITY                0x1000
#define KSZ8851_PMECR_WOL_TO_PME_OUT_EN               0x0F00
#define KSZ8851_PMECR_WOL_TO_PME_OUT_EN_WUP_FRAME     0x0800
#define KSZ8851_PMECR_WOL_TO_PME_OUT_EN_MAGIC_PKT     0x0400
#define KSZ8851_PMECR_WOL_TO_PME_OUT_EN_LINK_UP       0x0200
#define KSZ8851_PMECR_WOL_TO_PME_OUT_EN_ENERGY_DETECT 0x0100
#define KSZ8851_PMECR_AUTO_WUP_EN                     0x0080
#define KSZ8851_PMECR_WUP_TO_NORMAL_OP_MODE           0x0040
#define KSZ8851_PMECR_WUP_EVENT                       0x003C
#define KSZ8851_PMECR_WUP_EVENT_NONE                  0x0000
#define KSZ8851_PMECR_WUP_EVENT_ENERGY_DETECT         0x0004
#define KSZ8851_PMECR_WUP_EVENT_LINK_UP               0x0008
#define KSZ8851_PMECR_WUP_EVENT_MAGIC_PKT             0x0010
#define KSZ8851_PMECR_WUP_EVENT_WUP_FRAME             0x0020
#define KSZ8851_PMECR_PWR_MGMT_MODE                   0x0003
#define KSZ8851_PMECR_PWR_MGMT_MODE_NORMAL            0x0000
#define KSZ8851_PMECR_PWR_MGMT_MODE_ENERGY_DETECT     0x0001
#define KSZ8851_PMECR_PWR_MGMT_MODE_PWR_SAVING        0x0003

//Go-Sleep & Wake-Up Time register
#define KSZ8851_GSWUTR_WUP_TIME                       0xFF00
#define KSZ8851_GSWUTR_GO_SLEEP_TIME                  0x00FF

//PHY Reset register
#define KSZ8851_PHYRR_PHY_RESET                       0x0001

//PHY 1 MII Basic Control register
#define KSZ8851_P1MBCR_LOCAL_LOOPBACK                 0x4000
#define KSZ8851_P1MBCR_FORCE_100                      0x2000
#define KSZ8851_P1MBCR_AN_ENABLE                      0x1000
#define KSZ8851_P1MBCR_RESTART_AN                     0x0200
#define KSZ8851_P1MBCR_FORCE_FULL_DUPLEX              0x0100
#define KSZ8851_P1MBCR_HP_MDIX                        0x0020
#define KSZ8851_P1MBCR_FORCE_MDIX                     0x0010
#define KSZ8851_P1MBCR_DISABLE_MDIX                   0x0008
#define KSZ8851_P1MBCR_DISABLE_TRANSMIT               0x0002
#define KSZ8851_P1MBCR_DISABLE_LED                    0x0001

//PHY 1 MII Basic Status register
#define KSZ8851_P1MBSR_T4_CAPABLE                     0x8000
#define KSZ8851_P1MBSR_100_FULL_CAPABLE               0x4000
#define KSZ8851_P1MBSR_100_HALF_CAPABLE               0x2000
#define KSZ8851_P1MBSR_10_FULL_CAPABLE                0x1000
#define KSZ8851_P1MBSR_10_HALF_CAPABLE                0x0800
#define KSZ8851_P1MBSR_PREAMBLE_SUPPR                 0x0040
#define KSZ8851_P1MBSR_AN_COMPLETE                    0x0020
#define KSZ8851_P1MBSR_AN_CAPABLE                     0x0008
#define KSZ8851_P1MBSR_LINK_STATUS                    0x0004
#define KSZ8851_P1MBSR_JABBER_TEST                    0x0002
#define KSZ8851_P1MBSR_EXTENDED_CAPABLE               0x0001

//PHY 1 ID Low register
#define KSZ8851_PHY1ILR_DEFAULT                       0x1430

//PHY 1 ID High register
#define KSZ8851_PHY1IHR_DEFAULT                       0x0022

//PHY 1 Auto-Negotiation Advertisement register
#define KSZ8851_P1ANAR_NEXT_PAGE                      0x8000
#define KSZ8851_P1ANAR_REMOTE_FAULT                   0x2000
#define KSZ8851_P1ANAR_PAUSE                          0x0400
#define KSZ8851_P1ANAR_ADV_100_FULL                   0x0100
#define KSZ8851_P1ANAR_ADV_100_HALF                   0x0080
#define KSZ8851_P1ANAR_ADV_10_FULL                    0x0040
#define KSZ8851_P1ANAR_ADV_10_HALF                    0x0020
#define KSZ8851_P1ANAR_SELECTOR                       0x001F

//PHY 1 Auto-Negotiation Link Partner Ability register
#define KSZ8851_P1ANLPR_NEXT_PAGE                     0x8000
#define KSZ8851_P1ANLPR_LP_ACK                        0x4000
#define KSZ8851_P1ANLPR_REMOTE_FAULT                  0x2000
#define KSZ8851_P1ANLPR_PAUSE                         0x0400
#define KSZ8851_P1ANLPR_ADV_100_FULL                  0x0100
#define KSZ8851_P1ANLPR_ADV_100_HALF                  0x0080
#define KSZ8851_P1ANLPR_ADV_10_FULL                   0x0040
#define KSZ8851_P1ANLPR_ADV_10_HALF                   0x0020

//Port 1 PHY Special Control/Status & LinkMD register
#define KSZ8851_P1SCLMD_VCT_RESULT                    0x6000
#define KSZ8851_P1SCLMD_VCT_EN                        0x1000
#define KSZ8851_P1SCLMD_FORCE_LNK                     0x0800
#define KSZ8851_P1SCLMD_REMOTE_LOOPBACK               0x0200
#define KSZ8851_P1SCLMD_VCT_FAULT_COUNT               0x01FF

//Port 1 Control register
#define KSZ8851_P1CR_LED_OFF                          0x8000
#define KSZ8851_P1CR_TX_DISABLE                       0x4000
#define KSZ8851_P1CR_RESTART_AN                       0x2000
#define KSZ8851_P1CR_DISABLE_AUTO_MDIX                0x0400
#define KSZ8851_P1CR_FORCE_MDIX                       0x0200
#define KSZ8851_P1CR_AN_ENABLE                        0x0080
#define KSZ8851_P1CR_FORCE_SPEED                      0x0040
#define KSZ8851_P1CR_FORCE_DUPLEX                     0x0020
#define KSZ8851_P1CR_ADV_PAUSE                        0x0010
#define KSZ8851_P1CR_ADV_100_FULL                     0x0008
#define KSZ8851_P1CR_ADV_100_HALF                     0x0004
#define KSZ8851_P1CR_ADV_10_FULL                      0x0002
#define KSZ8851_P1CR_ADV_10_HALF                      0x0001

//Port 1 Status register
#define KSZ8851_P1SR_HP_MDIX                          0x8000
#define KSZ8851_P1SR_POLARITY_REVERSE                 0x2000
#define KSZ8851_P1SR_OPERATION_SPEED                  0x0400
#define KSZ8851_P1SR_OPERATION_DUPLEX                 0x0200
#define KSZ8851_P1SR_MDIX_STATUS                      0x0080
#define KSZ8851_P1SR_AN_DONE                          0x0040
#define KSZ8851_P1SR_LINK_GOOD                        0x0020
#define KSZ8851_P1SR_LP_PAUSE                         0x0010
#define KSZ8851_P1SR_LP_100_FULL                      0x0008
#define KSZ8851_P1SR_LP_100_HALF                      0x0004
#define KSZ8851_P1SR_LP_10_FULL                       0x0002
#define KSZ8851_P1SR_LP_10_HALF                       0x0001

//Transmit control word
#define KSZ8851_TX_CTRL_TXIC                          0x8000
#define KSZ8851_TX_CTRL_TXFID                         0x003F

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief TX packet header
 **/

typedef __start_packed struct
{
   uint16_t controlWord;
   uint16_t byteCount;
} __end_packed Ksz8851TxHeader;


/**
 * @brief RX packet header
 **/

typedef __start_packed struct
{
   uint16_t statusWord;
   uint16_t byteCount;
} __end_packed Ksz8851RxHeader;


/**
 * @brief KSZ8851 driver context
 **/

typedef struct
{
   uint16_t frameId; ///<Identify a frame and its associated status
} Ksz8851Context;


//KSZ8851 driver
extern const NicDriver ksz8851Driver;

//KSZ8851 related functions
error_t ksz8851Init(NetInterface *interface);

void ksz8851Tick(NetInterface *interface);

void ksz8851EnableIrq(NetInterface *interface);
void ksz8851DisableIrq(NetInterface *interface);
bool_t ksz8851IrqHandler(NetInterface *interface);
void ksz8851EventHandler(NetInterface *interface);

error_t ksz8851SendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t ksz8851ReceivePacket(NetInterface *interface);

error_t ksz8851UpdateMacAddrFilter(NetInterface *interface);

void ksz8851WriteReg(NetInterface *interface, uint8_t address, uint16_t data);
uint16_t ksz8851ReadReg(NetInterface *interface, uint8_t address);

void ksz8851WriteFifo(NetInterface *interface, const uint8_t *data,
   size_t length);

void ksz8851ReadFifo(NetInterface *interface, uint8_t *data, size_t length);

void ksz8851SetBit(NetInterface *interface, uint8_t address, uint16_t mask);
void ksz8851ClearBit(NetInterface *interface, uint8_t address, uint16_t mask);

uint32_t ksz8851CalcCrc(const void *data, size_t length);

void ksz8851DumpReg(NetInterface *interface);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
