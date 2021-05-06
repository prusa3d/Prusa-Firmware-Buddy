/**
 * @file enc28j60_driver.h
 * @brief ENC28J60 Ethernet controller
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

#ifndef _ENC28J60_DRIVER_H
#define _ENC28J60_DRIVER_H

//Full-duplex support
#ifndef ENC28J60_FULL_DUPLEX_SUPPORT
   #define ENC28J60_FULL_DUPLEX_SUPPORT ENABLED
#elif (ENC28J60_FULL_DUPLEX_SUPPORT != ENABLED && ENC28J60_FULL_DUPLEX_SUPPORT != DISABLED)
   #error ENC28J60_FULL_DUPLEX_SUPPORT parameter is not valid
#endif

//RX buffer size
#ifndef ENC28J60_ETH_RX_BUFFER_SIZE
   #define ENC28J60_ETH_RX_BUFFER_SIZE 1536
#elif (ENC28J60_ETH_RX_BUFFER_SIZE != 1536)
   #error ENC28J60_ETH_RX_BUFFER_SIZE parameter is not valid
#endif

//Receive and transmit buffers
#define ENC28J60_RX_BUFFER_START 0x0000
#define ENC28J60_RX_BUFFER_STOP  0x17FF
#define ENC28J60_TX_BUFFER_START 0x1800
#define ENC28J60_TX_BUFFER_STOP  0x1FFF

//SPI command set
#define ENC28J60_CMD_RCR 0x00
#define ENC28J60_CMD_RBM 0x3A
#define ENC28J60_CMD_WCR 0x40
#define ENC28J60_CMD_WBM 0x7A
#define ENC28J60_CMD_BFS 0x80
#define ENC28J60_CMD_BFC 0xA0
#define ENC28J60_CMD_SRC 0xFF

//ENC28J60 register types
#define ETH_REG_TYPE 0x0000
#define MAC_REG_TYPE 0x1000
#define MII_REG_TYPE 0x2000
#define PHY_REG_TYPE 0x3000

//ENC28J60 banks
#define BANK_0 0x0000
#define BANK_1 0x0100
#define BANK_2 0x0200
#define BANK_3 0x0300

//Related masks
#define REG_TYPE_MASK 0xF000
#define REG_BANK_MASK 0x0F00
#define REG_ADDR_MASK 0x001F

//ENC28J60 registers
#define ENC28J60_ERDPTL                      (ETH_REG_TYPE | BANK_0 | 0x00)
#define ENC28J60_ERDPTH                      (ETH_REG_TYPE | BANK_0 | 0x01)
#define ENC28J60_EWRPTL                      (ETH_REG_TYPE | BANK_0 | 0x02)
#define ENC28J60_EWRPTH                      (ETH_REG_TYPE | BANK_0 | 0x03)
#define ENC28J60_ETXSTL                      (ETH_REG_TYPE | BANK_0 | 0x04)
#define ENC28J60_ETXSTH                      (ETH_REG_TYPE | BANK_0 | 0x05)
#define ENC28J60_ETXNDL                      (ETH_REG_TYPE | BANK_0 | 0x06)
#define ENC28J60_ETXNDH                      (ETH_REG_TYPE | BANK_0 | 0x07)
#define ENC28J60_ERXSTL                      (ETH_REG_TYPE | BANK_0 | 0x08)
#define ENC28J60_ERXSTH                      (ETH_REG_TYPE | BANK_0 | 0x09)
#define ENC28J60_ERXNDL                      (ETH_REG_TYPE | BANK_0 | 0x0A)
#define ENC28J60_ERXNDH                      (ETH_REG_TYPE | BANK_0 | 0x0B)
#define ENC28J60_ERXRDPTL                    (ETH_REG_TYPE | BANK_0 | 0x0C)
#define ENC28J60_ERXRDPTH                    (ETH_REG_TYPE | BANK_0 | 0x0D)
#define ENC28J60_ERXWRPTL                    (ETH_REG_TYPE | BANK_0 | 0x0E)
#define ENC28J60_ERXWRPTH                    (ETH_REG_TYPE | BANK_0 | 0x0F)
#define ENC28J60_EDMASTL                     (ETH_REG_TYPE | BANK_0 | 0x10)
#define ENC28J60_EDMASTH                     (ETH_REG_TYPE | BANK_0 | 0x11)
#define ENC28J60_EDMANDL                     (ETH_REG_TYPE | BANK_0 | 0x12)
#define ENC28J60_EDMANDH                     (ETH_REG_TYPE | BANK_0 | 0x13)
#define ENC28J60_EDMADSTL                    (ETH_REG_TYPE | BANK_0 | 0x14)
#define ENC28J60_EDMADSTH                    (ETH_REG_TYPE | BANK_0 | 0x15)
#define ENC28J60_EDMACSL                     (ETH_REG_TYPE | BANK_0 | 0x16)
#define ENC28J60_EDMACSH                     (ETH_REG_TYPE | BANK_0 | 0x17)
#define ENC28J60_EIE                         (ETH_REG_TYPE | BANK_0 | 0x1B)
#define ENC28J60_EIR                         (ETH_REG_TYPE | BANK_0 | 0x1C)
#define ENC28J60_ESTAT                       (ETH_REG_TYPE | BANK_0 | 0x1D)
#define ENC28J60_ECON2                       (ETH_REG_TYPE | BANK_0 | 0x1E)
#define ENC28J60_ECON1                       (ETH_REG_TYPE | BANK_0 | 0x1F)
#define ENC28J60_EHT0                        (ETH_REG_TYPE | BANK_1 | 0x00)
#define ENC28J60_EHT1                        (ETH_REG_TYPE | BANK_1 | 0x01)
#define ENC28J60_EHT2                        (ETH_REG_TYPE | BANK_1 | 0x02)
#define ENC28J60_EHT3                        (ETH_REG_TYPE | BANK_1 | 0x03)
#define ENC28J60_EHT4                        (ETH_REG_TYPE | BANK_1 | 0x04)
#define ENC28J60_EHT5                        (ETH_REG_TYPE | BANK_1 | 0x05)
#define ENC28J60_EHT6                        (ETH_REG_TYPE | BANK_1 | 0x06)
#define ENC28J60_EHT7                        (ETH_REG_TYPE | BANK_1 | 0x07)
#define ENC28J60_EPMM0                       (ETH_REG_TYPE | BANK_1 | 0x08)
#define ENC28J60_EPMM1                       (ETH_REG_TYPE | BANK_1 | 0x09)
#define ENC28J60_EPMM2                       (ETH_REG_TYPE | BANK_1 | 0x0A)
#define ENC28J60_EPMM3                       (ETH_REG_TYPE | BANK_1 | 0x0B)
#define ENC28J60_EPMM4                       (ETH_REG_TYPE | BANK_1 | 0x0C)
#define ENC28J60_EPMM5                       (ETH_REG_TYPE | BANK_1 | 0x0D)
#define ENC28J60_EPMM6                       (ETH_REG_TYPE | BANK_1 | 0x0E)
#define ENC28J60_EPMM7                       (ETH_REG_TYPE | BANK_1 | 0x0F)
#define ENC28J60_EPMCSL                      (ETH_REG_TYPE | BANK_1 | 0x10)
#define ENC28J60_EPMCSH                      (ETH_REG_TYPE | BANK_1 | 0x11)
#define ENC28J60_EPMOL                       (ETH_REG_TYPE | BANK_1 | 0x14)
#define ENC28J60_EPMOH                       (ETH_REG_TYPE | BANK_1 | 0x15)
#define ENC28J60_EWOLIE                      (ETH_REG_TYPE | BANK_1 | 0x16)
#define ENC28J60_EWOLIR                      (ETH_REG_TYPE | BANK_1 | 0x17)
#define ENC28J60_ERXFCON                     (ETH_REG_TYPE | BANK_1 | 0x18)
#define ENC28J60_EPKTCNT                     (ETH_REG_TYPE | BANK_1 | 0x19)
#define ENC28J60_MACON1                      (MAC_REG_TYPE | BANK_2 | 0x00)
#define ENC28J60_MACON2                      (MAC_REG_TYPE | BANK_2 | 0x01)
#define ENC28J60_MACON3                      (MAC_REG_TYPE | BANK_2 | 0x02)
#define ENC28J60_MACON4                      (MAC_REG_TYPE | BANK_2 | 0x03)
#define ENC28J60_MABBIPG                     (MAC_REG_TYPE | BANK_2 | 0x04)
#define ENC28J60_MAIPGL                      (MAC_REG_TYPE | BANK_2 | 0x06)
#define ENC28J60_MAIPGH                      (MAC_REG_TYPE | BANK_2 | 0x07)
#define ENC28J60_MACLCON1                    (MAC_REG_TYPE | BANK_2 | 0x08)
#define ENC28J60_MACLCON2                    (MAC_REG_TYPE | BANK_2 | 0x09)
#define ENC28J60_MAMXFLL                     (MAC_REG_TYPE | BANK_2 | 0x0A)
#define ENC28J60_MAMXFLH                     (MAC_REG_TYPE | BANK_2 | 0x0B)
#define ENC28J60_MAPHSUP                     (MAC_REG_TYPE | BANK_2 | 0x0D)
#define ENC28J60_MICON                       (MII_REG_TYPE | BANK_2 | 0x11)
#define ENC28J60_MICMD                       (MII_REG_TYPE | BANK_2 | 0x12)
#define ENC28J60_MIREGADR                    (MII_REG_TYPE | BANK_2 | 0x14)
#define ENC28J60_MIWRL                       (MII_REG_TYPE | BANK_2 | 0x16)
#define ENC28J60_MIWRH                       (MII_REG_TYPE | BANK_2 | 0x17)
#define ENC28J60_MIRDL                       (MII_REG_TYPE | BANK_2 | 0x18)
#define ENC28J60_MIRDH                       (MII_REG_TYPE | BANK_2 | 0x19)
#define ENC28J60_MAADR1                      (MAC_REG_TYPE | BANK_3 | 0x00)
#define ENC28J60_MAADR0                      (MAC_REG_TYPE | BANK_3 | 0x01)
#define ENC28J60_MAADR3                      (MAC_REG_TYPE | BANK_3 | 0x02)
#define ENC28J60_MAADR2                      (MAC_REG_TYPE | BANK_3 | 0x03)
#define ENC28J60_MAADR5                      (MAC_REG_TYPE | BANK_3 | 0x04)
#define ENC28J60_MAADR4                      (MAC_REG_TYPE | BANK_3 | 0x05)
#define ENC28J60_EBSTSD                      (ETH_REG_TYPE | BANK_3 | 0x06)
#define ENC28J60_EBSTCON                     (ETH_REG_TYPE | BANK_3 | 0x07)
#define ENC28J60_EBSTCSL                     (ETH_REG_TYPE | BANK_3 | 0x08)
#define ENC28J60_EBSTCSH                     (ETH_REG_TYPE | BANK_3 | 0x09)
#define ENC28J60_MISTAT                      (MII_REG_TYPE | BANK_3 | 0x0A)
#define ENC28J60_EREVID                      (ETH_REG_TYPE | BANK_3 | 0x12)
#define ENC28J60_ECOCON                      (ETH_REG_TYPE | BANK_3 | 0x15)
#define ENC28J60_EFLOCON                     (ETH_REG_TYPE | BANK_3 | 0x17)
#define ENC28J60_EPAUSL                      (ETH_REG_TYPE | BANK_3 | 0x18)
#define ENC28J60_EPAUSH                      (ETH_REG_TYPE | BANK_3 | 0x19)

//ENC28J60 PHY registers
#define ENC28J60_PHCON1                      (PHY_REG_TYPE | 0x00)
#define ENC28J60_PHSTAT1                     (PHY_REG_TYPE | 0x01)
#define ENC28J60_PHID1                       (PHY_REG_TYPE | 0x02)
#define ENC28J60_PHID2                       (PHY_REG_TYPE | 0x03)
#define ENC28J60_PHCON2                      (PHY_REG_TYPE | 0x10)
#define ENC28J60_PHSTAT2                     (PHY_REG_TYPE | 0x11)
#define ENC28J60_PHIE                        (PHY_REG_TYPE | 0x12)
#define ENC28J60_PHIR                        (PHY_REG_TYPE | 0x13)
#define ENC28J60_PHLCON                      (PHY_REG_TYPE | 0x14)

//Ethernet Interrupt Enable register
#define ENC28J60_EIE_INTIE                   0x80
#define ENC28J60_EIE_PKTIE                   0x40
#define ENC28J60_EIE_DMAIE                   0x20
#define ENC28J60_EIE_LINKIE                  0x10
#define ENC28J60_EIE_TXIE                    0x08
#define ENC28J60_EIE_WOLIE                   0x04
#define ENC28J60_EIE_TXERIE                  0x02
#define ENC28J60_EIE_RXERIE                  0x01

//Ethernet Interrupt Request register
#define ENC28J60_EIR_PKTIF                   0x40
#define ENC28J60_EIR_DMAIF                   0x20
#define ENC28J60_EIR_LINKIF                  0x10
#define ENC28J60_EIR_TXIF                    0x08
#define ENC28J60_EIR_WOLIF                   0x04
#define ENC28J60_EIR_TXERIF                  0x02
#define ENC28J60_EIR_RXERIF                  0x01

//Ethernet Status register
#define ENC28J60_ESTAT_INT                   0x80
#define ENC28J60_ESTAT_R6                    0x40
#define ENC28J60_ESTAT_R5                    0x20
#define ENC28J60_ESTAT_LATECOL               0x10
#define ENC28J60_ESTAT_RXBUSY                0x04
#define ENC28J60_ESTAT_TXABRT                0x02
#define ENC28J60_ESTAT_CLKRDY                0x01

//Ethernet Control 2 register
#define ENC28J60_ECON2_AUTOINC               0x80
#define ENC28J60_ECON2_PKTDEC                0x40
#define ENC28J60_ECON2_PWRSV                 0x20
#define ENC28J60_ECON2_VRPS                  0x08

//Ethernet Control 1 register
#define ENC28J60_ECON1_TXRST                 0x80
#define ENC28J60_ECON1_RXRST                 0x40
#define ENC28J60_ECON1_DMAST                 0x20
#define ENC28J60_ECON1_CSUMEN                0x10
#define ENC28J60_ECON1_TXRTS                 0x08
#define ENC28J60_ECON1_RXEN                  0x04
#define ENC28J60_ECON1_BSEL1                 0x02
#define ENC28J60_ECON1_BSEL0                 0x01

//Ethernet Wake-Up On LAN Interrupt Enable register
#define ENC28J60_EWOLIE_UCWOLIE              0x80
#define ENC28J60_EWOLIE_AWOLIE               0x40
#define ENC28J60_EWOLIE_PMWOLIE              0x10
#define ENC28J60_EWOLIE_MPWOLIE              0x08
#define ENC28J60_EWOLIE_HTWOLIE              0x04
#define ENC28J60_EWOLIE_MCWOLIE              0x02
#define ENC28J60_EWOLIE_BCWOLIE              0x01

//Ethernet Wake-Up On LAN Interrupt Request register
#define ENC28J60_EWOLIR_UCWOLIF              0x80
#define ENC28J60_EWOLIR_AWOLIF               0x40
#define ENC28J60_EWOLIR_PMWOLIF              0x10
#define ENC28J60_EWOLIR_MPWOLIF              0x08
#define ENC28J60_EWOLIR_HTWOLIF              0x04
#define ENC28J60_EWOLIR_MCWOLIF              0x02
#define ENC28J60_EWOLIR_BCWOLIF              0x01

//Receive Filter Control register
#define ENC28J60_ERXFCON_UCEN                0x80
#define ENC28J60_ERXFCON_ANDOR               0x40
#define ENC28J60_ERXFCON_CRCEN               0x20
#define ENC28J60_ERXFCON_PMEN                0x10
#define ENC28J60_ERXFCON_MPEN                0x08
#define ENC28J60_ERXFCON_HTEN                0x04
#define ENC28J60_ERXFCON_MCEN                0x02
#define ENC28J60_ERXFCON_BCEN                0x01

//MAC Control 1 register
#define ENC28J60_MACON1_LOOPBK               0x10
#define ENC28J60_MACON1_TXPAUS               0x08
#define ENC28J60_MACON1_RXPAUS               0x04
#define ENC28J60_MACON1_PASSALL              0x02
#define ENC28J60_MACON1_MARXEN               0x01

//MAC Control 2 register
#define ENC28J60_MACON2_MARST                0x80
#define ENC28J60_MACON2_RNDRST               0x40
#define ENC28J60_MACON2_MARXRST              0x08
#define ENC28J60_MACON2_RFUNRST              0x04
#define ENC28J60_MACON2_MATXRST              0x02
#define ENC28J60_MACON2_TFUNRST              0x01

//MAC Control 3 register
#define ENC28J60_MACON3_PADCFG               0xE0
#define ENC28J60_MACON3_PADCFG_NO            0x00
#define ENC28J60_MACON3_PADCFG_60_BYTES      0x20
#define ENC28J60_MACON3_PADCFG_64_BYTES      0x60
#define ENC28J60_MACON3_PADCFG_AUTO          0xA0
#define ENC28J60_MACON3_TXCRCEN              0x10
#define ENC28J60_MACON3_PHDRLEN              0x08
#define ENC28J60_MACON3_HFRMEN               0x04
#define ENC28J60_MACON3_FRMLNEN              0x02
#define ENC28J60_MACON3_FULDPX               0x01

//MAC Control 4 register
#define ENC28J60_MACON4_DEFER                0x40
#define ENC28J60_MACON4_BPEN                 0x20
#define ENC28J60_MACON4_NOBKOFF              0x10
#define ENC28J60_MACON4_LONGPRE              0x02
#define ENC28J60_MACON4_PUREPRE              0x01

//Back-to-Back Inter-Packet Gap register
#define ENC28J60_MABBIPG_DEFAULT_HD          0x12
#define ENC28J60_MABBIPG_DEFAULT_FD          0x15

//Non-Back-to-Back Inter-Packet Gap Low Byte register
#define ENC28J60_MAIPGL_DEFAULT              0x12

//Non-Back-to-Back Inter-Packet Gap High Byte register
#define ENC28J60_MAIPGH_DEFAULT              0x0C

//Retransmission Maximum register
#define ENC28J60_MACLCON1_RETMAX             0x0F

//Collision Window register
#define ENC28J60_MACLCON2_COLWIN             0x3F
#define ENC28J60_MACLCON2_COLWIN_DEFAULT     0x37

//MAC-PHY Support register
#define ENC28J60_MAPHSUP_RSTINTFC            0x80
#define ENC28J60_MAPHSUP_R4                  0x10
#define ENC28J60_MAPHSUP_RSTRMII             0x08
#define ENC28J60_MAPHSUP_R0                  0x01

//MII Control register
#define ENC28J60_MICON_RSTMII                0x80

//MII Command register
#define ENC28J60_MICMD_MIISCAN               0x02
#define ENC28J60_MICMD_MIIRD                 0x01

//MII Register Addres register
#define ENC28J60_MIREGADR_VAL                0x1F

//Self-Test Control register
#define ENC28J60_EBSTCON_PSV                 0xE0
#define ENC28J60_EBSTCON_PSEL                0x10
#define ENC28J60_EBSTCON_TMSEL               0x0C
#define ENC28J60_EBSTCON_TMSEL_RANDOM        0x00
#define ENC28J60_EBSTCON_TMSEL_ADDR          0x04
#define ENC28J60_EBSTCON_TMSEL_PATTERN_SHIFT 0x08
#define ENC28J60_EBSTCON_TMSEL_RACE_MODE     0x0C
#define ENC28J60_EBSTCON_TME                 0x02
#define ENC28J60_EBSTCON_BISTST              0x01

//MII Status register
#define ENC28J60_MISTAT_R3                   0x08
#define ENC28J60_MISTAT_NVALID               0x04
#define ENC28J60_MISTAT_SCAN                 0x02
#define ENC28J60_MISTAT_BUSY                 0x01

//Ethernet Revision ID register
#define ENC28J60_EREVID_REV                  0x1F
#define ENC28J60_EREVID_REV_B1               0x02
#define ENC28J60_EREVID_REV_B4               0x04
#define ENC28J60_EREVID_REV_B5               0x05
#define ENC28J60_EREVID_REV_B7               0x06

//Clock Output Control register
#define ENC28J60_ECOCON_COCON                0x07
#define ENC28J60_ECOCON_COCON_DISABLED       0x00
#define ENC28J60_ECOCON_COCON_DIV1           0x01
#define ENC28J60_ECOCON_COCON_DIV2           0x02
#define ENC28J60_ECOCON_COCON_DIV3           0x03
#define ENC28J60_ECOCON_COCON_DIV4           0x04
#define ENC28J60_ECOCON_COCON_DIV8           0x05

//Ethernet Flow Control register
#define ENC28J60_EFLOCON_FULDPXS             0x04
#define ENC28J60_EFLOCON_FCEN                0x03
#define ENC28J60_EFLOCON_FCEN_OFF            0x00
#define ENC28J60_EFLOCON_FCEN_ON_HD          0x01
#define ENC28J60_EFLOCON_FCEN_ON_FD          0x02
#define ENC28J60_EFLOCON_FCEN_SEND_PAUSE     0x03

//PHY Control 1 register
#define ENC28J60_PHCON1_PRST                 0x8000
#define ENC28J60_PHCON1_PLOOPBK              0x4000
#define ENC28J60_PHCON1_PPWRSV               0x0800
#define ENC28J60_PHCON1_PDPXMD               0x0100

//Physical Layer Status 1 register
#define ENC28J60_PHSTAT1_PFDPX               0x1000
#define ENC28J60_PHSTAT1_PHDPX               0x0800
#define ENC28J60_PHSTAT1_LLSTAT              0x0004
#define ENC28J60_PHSTAT1_JBRSTAT             0x0002

//PHY Identifier 1 register
#define ENC28J60_PHID1_PIDH                  0xFFFF
#define ENC28J60_PHID1_PIDH_DEFAULT          0x0083

//PHY Identifier 2 register
#define ENC28J60_PHID2_PIDL                  0xFC00
#define ENC28J60_PHID2_PIDL_DEFAULT          0x1400
#define ENC28J60_PHID2_PPN                   0x03F0
#define ENC28J60_PHID2_PPN_DEFAULT           0x0000
#define ENC28J60_PHID2_PREV                  0x000F

//PHY Control 2 register
#define ENC28J60_PHCON2_FRCLNK               0x4000
#define ENC28J60_PHCON2_TXDIS                0x2000
#define ENC28J60_PHCON2_JABBER               0x0400
#define ENC28J60_PHCON2_HDLDIS               0x0100

//Physical Layer Status 2 register
#define ENC28J60_PHSTAT2_TXSTAT              0x2000
#define ENC28J60_PHSTAT2_RXSTAT              0x1000
#define ENC28J60_PHSTAT2_COLSTAT             0x0800
#define ENC28J60_PHSTAT2_LSTAT               0x0400
#define ENC28J60_PHSTAT2_DPXSTAT             0x0200
#define ENC28J60_PHSTAT2_PLRITY              0x0010

//PHY Interrupt Enable register
#define ENC28J60_PHIE_PLNKIE                 0x0010
#define ENC28J60_PHIE_PGEIE                  0x0002

//PHY Interrupt Request register
#define ENC28J60_PHIR_PLNKIF                 0x0010
#define ENC28J60_PHIR_PGIF                   0x0004

//PHY Module LED Control register
#define ENC28J60_PHLCON_LACFG                0x0F00
#define ENC28J60_PHLCON_LACFG_TX             0x0100
#define ENC28J60_PHLCON_LACFG_RX             0x0200
#define ENC28J60_PHLCON_LACFG_COL            0x0300
#define ENC28J60_PHLCON_LACFG_LINK           0x0400
#define ENC28J60_PHLCON_LACFG_DUPLEX         0x0500
#define ENC28J60_PHLCON_LACFG_TX_RX          0x0700
#define ENC28J60_PHLCON_LACFG_ON             0x0800
#define ENC28J60_PHLCON_LACFG_OFF            0x0900
#define ENC28J60_PHLCON_LACFG_BLINK_FAST     0x0A00
#define ENC28J60_PHLCON_LACFG_BLINK_SLOW     0x0B00
#define ENC28J60_PHLCON_LACFG_LINK_RX        0x0C00
#define ENC28J60_PHLCON_LACFG_LINK_TX_RX     0x0D00
#define ENC28J60_PHLCON_LACFG_DUPLEX_COL     0x0E00
#define ENC28J60_PHLCON_LBCFG                0x00F0
#define ENC28J60_PHLCON_LBCFG_TX             0x0010
#define ENC28J60_PHLCON_LBCFG_RX             0x0020
#define ENC28J60_PHLCON_LBCFG_COL            0x0030
#define ENC28J60_PHLCON_LBCFG_LINK           0x0040
#define ENC28J60_PHLCON_LBCFG_DUPLEX         0x0050
#define ENC28J60_PHLCON_LBCFG_TX_RX          0x0070
#define ENC28J60_PHLCON_LBCFG_ON             0x0080
#define ENC28J60_PHLCON_LBCFG_OFF            0x0090
#define ENC28J60_PHLCON_LBCFG_BLINK_FAST     0x00A0
#define ENC28J60_PHLCON_LBCFG_BLINK_SLOW     0x00B0
#define ENC28J60_PHLCON_LBCFG_LINK_RX        0x00C0
#define ENC28J60_PHLCON_LBCFG_LINK_TX_RX     0x00D0
#define ENC28J60_PHLCON_LBCFG_DUPLEX_COL     0x00E0
#define ENC28J60_PHLCON_LFRQ                 0x000C
#define ENC28J60_PHLCON_LFRQ_40_MS           0x0000
#define ENC28J60_PHLCON_LFRQ_73_MS           0x0004
#define ENC28J60_PHLCON_LFRQ_139_MS          0x0008
#define ENC28J60_PHLCON_STRCH                0x0002

//Per-packet control byte
#define ENC28J60_TX_CTRL_PHUGEEN             0x08
#define ENC28J60_TX_CTRL_PPADEN              0x04
#define ENC28J60_TX_CTRL_PCRCEN              0x02
#define ENC28J60_TX_CTRL_POVERRIDE           0x01

//Receive status vector
#define ENC28J60_RSV_VLAN_TYPE               0x4000
#define ENC28J60_RSV_UNKNOWN_OPCODE          0x2000
#define ENC28J60_RSV_PAUSE_CONTROL_FRAME     0x1000
#define ENC28J60_RSV_CONTROL_FRAME           0x0800
#define ENC28J60_RSV_DRIBBLE_NIBBLE          0x0400
#define ENC28J60_RSV_BROADCAST_PACKET        0x0200
#define ENC28J60_RSV_MULTICAST_PACKET        0x0100
#define ENC28J60_RSV_RECEIVED_OK             0x0080
#define ENC28J60_RSV_LENGTH_OUT_OF_RANGE     0x0040
#define ENC28J60_RSV_LENGTH_CHECK_ERROR      0x0020
#define ENC28J60_RSV_CRC_ERROR               0x0010
#define ENC28J60_RSV_CARRIER_EVENT           0x0004
#define ENC28J60_RSV_DROP_EVENT              0x0001

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief ENC28J60 driver context
 **/

typedef struct
{
   uint16_t currentBank; ///<Current bank
   uint16_t nextPacket;  ///<Next packet in the receive buffer
} Enc28j60Context;


//ENC28J60 driver
extern const NicDriver enc28j60Driver;

//ENC28J60 related functions
error_t enc28j60Init(NetInterface *interface);

void enc28j60Tick(NetInterface *interface);

void enc28j60EnableIrq(NetInterface *interface);
void enc28j60DisableIrq(NetInterface *interface);
bool_t enc28j60IrqHandler(NetInterface *interface);
void enc28j60EventHandler(NetInterface *interface);

error_t enc28j60SendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t enc28j60ReceivePacket(NetInterface *interface);

error_t enc28j60UpdateMacAddrFilter(NetInterface *interface);

void enc28j60SoftReset(NetInterface *interface);
void enc28j60SelectBank(NetInterface *interface, uint16_t address);

void enc28j60WriteReg(NetInterface *interface, uint16_t address, uint8_t data);
uint8_t enc28j60ReadReg(NetInterface *interface, uint16_t address);

void enc28j60WritePhyReg(NetInterface *interface, uint16_t address,
   uint16_t data);

uint16_t enc28j60ReadPhyReg(NetInterface *interface, uint16_t address);

void enc28j60WriteBuffer(NetInterface *interface,
   const NetBuffer *buffer, size_t offset);

void enc28j60ReadBuffer(NetInterface *interface,
   uint8_t *data, size_t length);

void enc28j60SetBit(NetInterface *interface, uint16_t address, uint16_t mask);
void enc28j60ClearBit(NetInterface *interface, uint16_t address, uint16_t mask);

uint32_t enc28j60CalcCrc(const void *data, size_t length);

void enc28j60DumpReg(NetInterface *interface);
void enc28j60DumpPhyReg(NetInterface *interface);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
