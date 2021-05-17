/**
 * @file enc624j600_driver.h
 * @brief ENC624J600/ENC424J600 Ethernet controller
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

#ifndef _ENC624J600_DRIVER_H
#define _ENC624J600_DRIVER_H

//RX buffer size
#ifndef ENC624J600_ETH_RX_BUFFER_SIZE
   #define ENC624J600_ETH_RX_BUFFER_SIZE 1536
#elif (ENC624J600_ETH_RX_BUFFER_SIZE != 1536)
   #error ENC624J600_ETH_RX_BUFFER_SIZE parameter is not valid
#endif

//Receive and transmit buffers
#define ENC624J600_TX_BUFFER_START 0x0000
#define ENC624J600_TX_BUFFER_STOP  0x17FE
#define ENC624J600_RX_BUFFER_START 0x1800
#define ENC624J600_RX_BUFFER_STOP  0x5FFE

//SPI command set
#define ENC624J600_CMD_B0SEL       0xC0 //Bank 0 Select
#define ENC624J600_CMD_B1SEL       0xC2 //Bank 1 Select
#define ENC624J600_CMD_B2SEL       0xC4 //Bank 2 Select
#define ENC624J600_CMD_B3SEL       0xC6 //Bank 3 Select
#define ENC624J600_CMD_SETETHRST   0xCA //System Reset
#define ENC624J600_CMD_FCDISABLE   0xE0 //Flow Control Disable
#define ENC624J600_CMD_FCSINGLE    0xE2 //Flow Control Single
#define ENC624J600_CMD_FCMULTIPLE  0xE4 //Flow Control Multiple
#define ENC624J600_CMD_FCCLEAR     0xE6 //Flow Control Clear
#define ENC624J600_CMD_SETPKTDEC   0xCC //Decrement Packet Counter
#define ENC624J600_CMD_DMASTOP     0xD2 //DMA Stop
#define ENC624J600_CMD_DMACKSUM    0xD8 //DMA Start Checksum
#define ENC624J600_CMD_DMACKSUMS   0xDA //DMA Start Checksum with Seed
#define ENC624J600_CMD_DMACOPY     0xDC //DMA Start Copy
#define ENC624J600_CMD_DMACOPYS    0xDE //DMA Start Copy and Checksum with Seed
#define ENC624J600_CMD_SETTXRTS    0xD4 //Request Packet Transmission
#define ENC624J600_CMD_ENABLERX    0xE8 //Enable RX
#define ENC624J600_CMD_DISABLERX   0xEA //Disable RX
#define ENC624J600_CMD_SETEIE      0xEC //Enable Interrupts
#define ENC624J600_CMD_CLREIE      0xEE //Disable Interrupts
#define ENC624J600_CMD_RBSEL       0xC8 //Read Bank Select
#define ENC624J600_CMD_WGPRDPT     0x60 //Write EGPRDPT
#define ENC624J600_CMD_RGPRDPT     0x62 //Read EGPRDPT
#define ENC624J600_CMD_WRXRDPT     0x64 //Write ERXRDPT
#define ENC624J600_CMD_RRXRDPT     0x66 //Read ERXRDPT
#define ENC624J600_CMD_WUDARDPT    0x68 //Write EUDARDPT
#define ENC624J600_CMD_RUDARDPT    0x6A //Read EUDARDPT
#define ENC624J600_CMD_WGPWRPT     0x6C //Write EGPWRPT
#define ENC624J600_CMD_RGPWRPT     0x6E //Read EGPWRPT
#define ENC624J600_CMD_WRXWRPT     0x70 //Write ERXWRPT
#define ENC624J600_CMD_RRXWRPT     0x72 //Read ERXWRPT
#define ENC624J600_CMD_WUDAWRPT    0x74 //Write EUDAWRPT
#define ENC624J600_CMD_RUDAWRPT    0x76 //Read EUDAWRPT
#define ENC624J600_CMD_RCR         0x00 //Read Control Register
#define ENC624J600_CMD_WCR         0x40 //Write Control Register
#define ENC624J600_CMD_RCRU        0x20 //Read Control Register Unbanked
#define ENC624J600_CMD_WCRU        0x22 //Write Control Register Unbanked
#define ENC624J600_CMD_BFS         0x80 //Bit Field Set
#define ENC624J600_CMD_BFC         0xA0 //Bit Field Clear
#define ENC624J600_CMD_BFSU        0x24 //Bit Field Set Unbanked
#define ENC624J600_CMD_BFCU        0x26 //Bit Field Clear Unbanked
#define ENC624J600_CMD_RGPDATA     0x28 //Read EGPDATA
#define ENC624J600_CMD_WGPDATA     0x2A //Write EGPDATA
#define ENC624J600_CMD_RRXDATA     0x2C //Read ERXDATA
#define ENC624J600_CMD_WRXDATA     0x2E //Write ERXDATA
#define ENC624J600_CMD_RUDADATA    0x30 //Read EUDADATA
#define ENC624J600_CMD_WUDADATA    0x32 //Write EUDADATA

//ENC624J600 registers
#define ENC624J600_ETXST                          0x00
#define ENC624J600_ETXLEN                         0x02
#define ENC624J600_ERXST                          0x04
#define ENC624J600_ERXTAIL                        0x06
#define ENC624J600_ERXHEAD                        0x08
#define ENC624J600_EDMAST                         0x0A
#define ENC624J600_EDMALEN                        0x0C
#define ENC624J600_EDMADST                        0x0E
#define ENC624J600_EDMACS                         0x10
#define ENC624J600_ETXSTAT                        0x12
#define ENC624J600_ETXWIRE                        0x14
#define ENC624J600_EUDAST                         0x16
#define ENC624J600_EUDAND                         0x18
#define ENC624J600_ESTAT                          0x1A
#define ENC624J600_EIR                            0x1C
#define ENC624J600_ECON1                          0x1E
#define ENC624J600_EHT1                           0x20
#define ENC624J600_EHT2                           0x22
#define ENC624J600_EHT3                           0x24
#define ENC624J600_EHT4                           0x26
#define ENC624J600_EPMM1                          0x28
#define ENC624J600_EPMM2                          0x2A
#define ENC624J600_EPMM3                          0x2C
#define ENC624J600_EPMM4                          0x2E
#define ENC624J600_EPMCS                          0x30
#define ENC624J600_EPMO                           0x32
#define ENC624J600_ERXFCON                        0x34
#define ENC624J600_MACON1                         0x40
#define ENC624J600_MACON2                         0x42
#define ENC624J600_MABBIPG                        0x44
#define ENC624J600_MAIPG                          0x46
#define ENC624J600_MACLCON                        0x48
#define ENC624J600_MAMXFL                         0x4A
#define ENC624J600_MICMD                          0x52
#define ENC624J600_MIREGADR                       0x54
#define ENC624J600_MAADR3                         0x60
#define ENC624J600_MAADR2                         0x62
#define ENC624J600_MAADR1                         0x64
#define ENC624J600_MIWR                           0x66
#define ENC624J600_MIRD                           0x68
#define ENC624J600_MISTAT                         0x6A
#define ENC624J600_EPAUS                          0x6C
#define ENC624J600_ECON2                          0x6E
#define ENC624J600_ERXWM                          0x70
#define ENC624J600_EIE                            0x72
#define ENC624J600_EIDLED                         0x74
#define ENC624J600_EGPDATA                        0x80
#define ENC624J600_ERXDATA                        0x82
#define ENC624J600_EUDADATA                       0x84
#define ENC624J600_EGPRDPT                        0x86
#define ENC624J600_EGPWRPT                        0x88
#define ENC624J600_ERXRDPT                        0x8A
#define ENC624J600_ERXWRPT                        0x8C
#define ENC624J600_EUDARDPT                       0x8E
#define ENC624J600_EUDAWRPT                       0x90

//ENC624J600 PHY registers
#define ENC624J600_PHCON1                         0x00
#define ENC624J600_PHSTAT1                        0x01
#define ENC624J600_PHANA                          0x04
#define ENC624J600_PHANLPA                        0x05
#define ENC624J600_PHANE                          0x06
#define ENC624J600_PHCON2                         0x11
#define ENC624J600_PHSTAT2                        0x1B
#define ENC624J600_PHSTAT3                        0x1F

//TX Start Address register
#define ENC624J600_ETXST_VAL                      0x7FFF

//TX Length register
#define ENC624J600_ETXLEN_VAL                     0x7FFF

//RX Buffer Start Address register
#define ENC624J600_ERXST_VAL                      0x7FFF

//RX Tail Pointer register
#define ENC624J600_ERXTAIL_VAL                    0x7FFF

//RX Head Pointer register
#define ENC624J600_ERXHEAD_VAL                    0x7FFF

//DMA Start Address register
#define ENC624J600_EDMAST_VAL                     0x7FFF

//DMA Length register
#define ENC624J600_EDMALEN_VAL                    0x7FFF

//DMA Destination Address register
#define ENC624J600_EDMADST_VAL                    0x7FFF

//Ethernet Transmit Status register
#define ENC624J600_ETXSTAT_R12                    0x1000
#define ENC624J600_ETXSTAT_R11                    0x0800
#define ENC624J600_ETXSTAT_LATECOL                0x0400
#define ENC624J600_ETXSTAT_MAXCOL                 0x0200
#define ENC624J600_ETXSTAT_EXDEFER                0x0100
#define ENC624J600_ETXSTAT_DEFER                  0x0080
#define ENC624J600_ETXSTAT_R6                     0x0040
#define ENC624J600_ETXSTAT_R5                     0x0020
#define ENC624J600_ETXSTAT_CRCBAD                 0x0010
#define ENC624J600_ETXSTAT_COLCNT                 0x000F

//User-Defined Area Start Pointer register
#define ENC624J600_EUDAST_VAL                     0x7FFF

//User-Defined Area End Pointer register
#define ENC624J600_EUDAND_VAL                     0x7FFF

//Ethernet Status register
#define ENC624J600_ESTAT_INT                      0x8000
#define ENC624J600_ESTAT_FCIDLE                   0x4000
#define ENC624J600_ESTAT_RXBUSY                   0x2000
#define ENC624J600_ESTAT_CLKRDY                   0x1000
#define ENC624J600_ESTAT_R11                      0x0800
#define ENC624J600_ESTAT_PHYDPX                   0x0400
#define ENC624J600_ESTAT_R9                       0x0200
#define ENC624J600_ESTAT_PHYLNK                   0x0100
#define ENC624J600_ESTAT_PKTCNT                   0x00FF

//Ethernet Interrupt Flag register
#define ENC624J600_EIR_CRYPTEN                    0x8000
#define ENC624J600_EIR_MODEXIF                    0x4000
#define ENC624J600_EIR_HASHIF                     0x2000
#define ENC624J600_EIR_AESIF                      0x1000
#define ENC624J600_EIR_LINKIF                     0x0800
#define ENC624J600_EIR_R10                        0x0400
#define ENC624J600_EIR_R9                         0x0200
#define ENC624J600_EIR_R8                         0x0100
#define ENC624J600_EIR_R7                         0x0080
#define ENC624J600_EIR_PKTIF                      0x0040
#define ENC624J600_EIR_DMAIF                      0x0020
#define ENC624J600_EIR_R4                         0x0010
#define ENC624J600_EIR_TXIF                       0x0008
#define ENC624J600_EIR_TXABTIF                    0x0004
#define ENC624J600_EIR_RXABTIF                    0x0002
#define ENC624J600_EIR_PCFULIF                    0x0001

//Ethernet Control 1 register
#define ENC624J600_ECON1_MODEXST                  0x8000
#define ENC624J600_ECON1_HASHEN                   0x4000
#define ENC624J600_ECON1_HASHOP                   0x2000
#define ENC624J600_ECON1_HASHLST                  0x1000
#define ENC624J600_ECON1_AESST                    0x0800
#define ENC624J600_ECON1_AESOP1                   0x0400
#define ENC624J600_ECON1_AESOP0                   0x0200
#define ENC624J600_ECON1_PKTDEC                   0x0100
#define ENC624J600_ECON1_FCOP1                    0x0080
#define ENC624J600_ECON1_FCOP0                    0x0040
#define ENC624J600_ECON1_DMAST                    0x0020
#define ENC624J600_ECON1_DMACPY                   0x0010
#define ENC624J600_ECON1_DMACSSD                  0x0008
#define ENC624J600_ECON1_DMANOCS                  0x0004
#define ENC624J600_ECON1_TXRTS                    0x0002
#define ENC624J600_ECON1_RXEN                     0x0001

//Ethernet RX Filter Control register
#define ENC624J600_ERXFCON_HTEN                   0x8000
#define ENC624J600_ERXFCON_MPEN                   0x4000
#define ENC624J600_ERXFCON_NOTPM                  0x1000
#define ENC624J600_ERXFCON_PMEN                   0x0F00
#define ENC624J600_ERXFCON_PMEN_DISABLED          0x0000
#define ENC624J600_ERXFCON_PMEN_CHECKSUM          0x0100
#define ENC624J600_ERXFCON_PMEN_UNICAST           0x0200
#define ENC624J600_ERXFCON_PMEN_NOT_UNICAST       0x0300
#define ENC624J600_ERXFCON_PMEN_MULTICAST         0x0400
#define ENC624J600_ERXFCON_PMEN_NOT_MULTICAST     0x0500
#define ENC624J600_ERXFCON_PMEN_BROADCAST         0x0600
#define ENC624J600_ERXFCON_PMEN_NOT_BROADCAST     0x0700
#define ENC624J600_ERXFCON_PMEN_HASH              0x0800
#define ENC624J600_ERXFCON_PMEN_MAGIC_PKT         0x0900
#define ENC624J600_ERXFCON_CRCEEN                 0x0080
#define ENC624J600_ERXFCON_CRCEN                  0x0040
#define ENC624J600_ERXFCON_RUNTEEN                0x0020
#define ENC624J600_ERXFCON_RUNTEN                 0x0010
#define ENC624J600_ERXFCON_UCEN                   0x0008
#define ENC624J600_ERXFCON_NOTMEEN                0x0004
#define ENC624J600_ERXFCON_MCEN                   0x0002
#define ENC624J600_ERXFCON_BCEN                   0x0001

//MAC Control 1 register
#define ENC624J600_MACON1_R15_14                  0xC000
#define ENC624J600_MACON1_R11_8                   0x0F00
#define ENC624J600_MACON1_LOOPBK                  0x0010
#define ENC624J600_MACON1_R3                      0x0008
#define ENC624J600_MACON1_R3_DEFAULT              0x0008
#define ENC624J600_MACON1_RXPAUS                  0x0004
#define ENC624J600_MACON1_PASSALL                 0x0002
#define ENC624J600_MACON1_R0                      0x0001
#define ENC624J600_MACON1_R0_DEFAULT              0x0001

//MAC Control 2 register
#define ENC624J600_MACON2_DEFER                   0x4000
#define ENC624J600_MACON2_BPEN                    0x2000
#define ENC624J600_MACON2_NOBKOFF                 0x1000
#define ENC624J600_MACON2_R9_8                    0x0300
#define ENC624J600_MACON2_PADCFG                  0x00E0
#define ENC624J600_MACON2_PADCFG_NO               0x0000
#define ENC624J600_MACON2_PADCFG_60_BYTES         0x0020
#define ENC624J600_MACON2_PADCFG_64_BYTES         0x0060
#define ENC624J600_MACON2_PADCFG_AUTO             0x00A0
#define ENC624J600_MACON2_TXCRCEN                 0x0010
#define ENC624J600_MACON2_PHDREN                  0x0008
#define ENC624J600_MACON2_HFRMEN                  0x0004
#define ENC624J600_MACON2_R1                      0x0002
#define ENC624J600_MACON2_R1_DEFAULT              0x0002
#define ENC624J600_MACON2_FULDPX                  0x0001

//MAC Back-To-Back Inter-Packet Gap register
#define ENC624J600_MABBIPG_BBIPG                  0x007F
#define ENC624J600_MABBIPG_BBIPG_DEFAULT_HD       0x0012
#define ENC624J600_MABBIPG_BBIPG_DEFAULT_FD       0x0015

//MAC Inter-Packet Gap register
#define ENC624J600_MAIPG_R14_8                    0x7F00
#define ENC624J600_MAIPG_R14_8_DEFAULT            0x0C00
#define ENC624J600_MAIPG_IPG                      0x007F
#define ENC624J600_MAIPG_IPG_DEFAULT              0x0012

//MAC Collision Control register
#define ENC624J600_MACLCON_R13_8                  0x3F00
#define ENC624J600_MACLCON_R13_8_DEFAULT          0x3700
#define ENC624J600_MACLCON_MAXRET                 0x000F

//MII Management Command register
#define ENC624J600_MICMD_MIISCAN                  0x0002
#define ENC624J600_MICMD_MIIRD                    0x0001

//MII Management Address register
#define ENC624J600_MIREGADR_R12_8                 0x1F00
#define ENC624J600_MIREGADR_R12_8_DEFAULT         0x0100
#define ENC624J600_MIREGADR_PHREG                 0x001F

//MII Management Status register
#define ENC624J600_MISTAT_R3                      0x0008
#define ENC624J600_MISTAT_NVALID                  0x0004
#define ENC624J600_MISTAT_SCAN                    0x0002
#define ENC624J600_MISTAT_BUSY                    0x0001

//Ethernet Control 2 register
#define ENC624J600_ECON2_ETHEN                    0x8000
#define ENC624J600_ECON2_STRCH                    0x4000
#define ENC624J600_ECON2_TXMAC                    0x2000
#define ENC624J600_ECON2_SHA1MD5                  0x1000
#define ENC624J600_ECON2_COCON                    0x0F00
#define ENC624J600_ECON2_COCON_NONE               0x0000
#define ENC624J600_ECON2_COCON_33_33_MHZ          0x0100
#define ENC624J600_ECON2_COCON_25_00_MHZ          0x0200
#define ENC624J600_ECON2_COCON_20_00_MHZ          0x0300
#define ENC624J600_ECON2_COCON_16_67_MHZ          0x0400
#define ENC624J600_ECON2_COCON_12_50_MHZ          0x0500
#define ENC624J600_ECON2_COCON_10_00_MHZ          0x0600
#define ENC624J600_ECON2_COCON_8_333_MHZ          0x0700
#define ENC624J600_ECON2_COCON_8_000_MHZ          0x0800
#define ENC624J600_ECON2_COCON_6_250_MHZ          0x0900
#define ENC624J600_ECON2_COCON_5_000_MHZ          0x0A00
#define ENC624J600_ECON2_COCON_4_000_MHZ          0x0B00
#define ENC624J600_ECON2_COCON_3_125_MHZ          0x0C00
#define ENC624J600_ECON2_COCON_100_KHZ            0x0E00
#define ENC624J600_ECON2_COCON_50_KHZ             0x0F00
#define ENC624J600_ECON2_AUTOFC                   0x0080
#define ENC624J600_ECON2_TXRST                    0x0040
#define ENC624J600_ECON2_RXRST                    0x0020
#define ENC624J600_ECON2_ETHRST                   0x0010
#define ENC624J600_ECON2_MODLEN                   0x000C
#define ENC624J600_ECON2_MODLEN_512_BITS          0x0000
#define ENC624J600_ECON2_MODLEN_768_BITS          0x0004
#define ENC624J600_ECON2_MODLEN_1024_BITS         0x0008
#define ENC624J600_ECON2_AESLEN                   0x0003
#define ENC624J600_ECON2_AESLEN_128_BITS          0x0000
#define ENC624J600_ECON2_AESLEN_192_BITS          0x0001
#define ENC624J600_ECON2_AESLEN_256_BITS          0x0002

//Receive Watermark register
#define ENC624J600_ERXWM_RXFWM                    0xFF00
#define ENC624J600_ERXWM_RXEWM                    0x00FF

//Ethernet Interrupt Enable register
#define ENC624J600_EIE_INTIE                      0x8000
#define ENC624J600_EIE_MODEXIE                    0x4000
#define ENC624J600_EIE_HASHIE                     0x2000
#define ENC624J600_EIE_AESIE                      0x1000
#define ENC624J600_EIE_LINKIE                     0x0800
#define ENC624J600_EIE_R10_7                      0x0780
#define ENC624J600_EIE_PKTIE                      0x0040
#define ENC624J600_EIE_DMAIE                      0x0020
#define ENC624J600_EIE_R4                         0x0010
#define ENC624J600_EIE_R4_DEFAULT                 0x0010
#define ENC624J600_EIE_TXIE                       0x0008
#define ENC624J600_EIE_TXABTIE                    0x0004
#define ENC624J600_EIE_RXABTIE                    0x0002
#define ENC624J600_EIE_PCFULIE                    0x0001

//Ethernet ID Status/LED Control register
#define ENC624J600_EIDLED_LACFG                   0xF000
#define ENC624J600_EIDLED_LACFG_OFF               0x0000
#define ENC624J600_EIDLED_LACFG_ON                0x1000
#define ENC624J600_EIDLED_LACFG_LINK              0x2000
#define ENC624J600_EIDLED_LACFG_COL               0x3000
#define ENC624J600_EIDLED_LACFG_TX                0x4000
#define ENC624J600_EIDLED_LACFG_RX                0x5000
#define ENC624J600_EIDLED_LACFG_TX_RX             0x6000
#define ENC624J600_EIDLED_LACFG_DUPLEX            0x7000
#define ENC624J600_EIDLED_LACFG_SPEED             0x8000
#define ENC624J600_EIDLED_LACFG_LINK_TX           0x9000
#define ENC624J600_EIDLED_LACFG_LINK_RX           0xA000
#define ENC624J600_EIDLED_LACFG_LINK_TX_RX        0xB000
#define ENC624J600_EIDLED_LACFG_LINK_COL          0xC000
#define ENC624J600_EIDLED_LACFG_LINK_DUPLEX_TX_RX 0xE000
#define ENC624J600_EIDLED_LACFG_LINK_SPEED_TX_RX  0xF000
#define ENC624J600_EIDLED_LBCFG                   0x0F00
#define ENC624J600_EIDLED_LBCFG_OFF               0x0000
#define ENC624J600_EIDLED_LBCFG_ON                0x0100
#define ENC624J600_EIDLED_LBCFG_LINK              0x0200
#define ENC624J600_EIDLED_LBCFG_COL               0x0300
#define ENC624J600_EIDLED_LBCFG_TX                0x0400
#define ENC624J600_EIDLED_LBCFG_RX                0x0500
#define ENC624J600_EIDLED_LBCFG_TX_RX             0x0600
#define ENC624J600_EIDLED_LBCFG_DUPLEX            0x0700
#define ENC624J600_EIDLED_LBCFG_SPEED             0x0800
#define ENC624J600_EIDLED_LBCFG_LINK_TX           0x0900
#define ENC624J600_EIDLED_LBCFG_LINK_RX           0x0A00
#define ENC624J600_EIDLED_LBCFG_LINK_TX_RX        0x0B00
#define ENC624J600_EIDLED_LBCFG_LINK_COL          0x0C00
#define ENC624J600_EIDLED_LBCFG_LINK_DUPLEX_TX_RX 0x0E00
#define ENC624J600_EIDLED_LBCFG_LINK_SPEED_TX_RX  0x0F00
#define ENC624J600_EIDLED_DEVID                   0x00E0
#define ENC624J600_EIDLED_DEVID_DEFAULT           0x0020
#define ENC624J600_EIDLED_REVID                   0x001F

//General Purpose Data Window register
#define ENC624J600_EGPDATA_R15_8                  0xFF00
#define ENC624J600_EGPDATA_VAL                    0x00FF

//Ethernet RX Data Window register
#define ENC624J600_ERXDATA_R15_8                  0xFF00
#define ENC624J600_ERXDATA_VAL                    0x00FF

//User-Defined Area Data Window register
#define ENC624J600_EUDADATA_R15_8                 0xFF00
#define ENC624J600_EUDADATA_VAL                   0x00FF

//General Purpose Window Read Pointer register
#define ENC624J600_EGPRDPT_VAL                    0x7FFF

//General Purpose Window Write Pointer register
#define ENC624J600_EGPWRPT_VAL                    0x7FFF

//RX Window Read Pointer register
#define ENC624J600_ERXRDPT_VAL                    0x7FFF

//RX Window Write Pointer register
#define ENC624J600_ERXWRPT_VAL                    0x7FFF

//UDA Window Read Pointer register
#define ENC624J600_EUDARDPT_VAL                   0x7FFF

//UDA Window Write Pointer register
#define ENC624J600_EUDAWRPT_VAL                   0x7FFF

//PHY Control 1 register
#define ENC624J600_PHCON1_PRST                    0x8000
#define ENC624J600_PHCON1_PLOOPBK                 0x4000
#define ENC624J600_PHCON1_SPD100                  0x2000
#define ENC624J600_PHCON1_ANEN                    0x1000
#define ENC624J600_PHCON1_PSLEEP                  0x0800
#define ENC624J600_PHCON1_RENEG                   0x0200
#define ENC624J600_PHCON1_PFULDPX                 0x0100

//PHY Status 1 register
#define ENC624J600_PHSTAT1_FULL100                0x4000
#define ENC624J600_PHSTAT1_HALF100                0x2000
#define ENC624J600_PHSTAT1_FULL10                 0x1000
#define ENC624J600_PHSTAT1_HALF10                 0x0800
#define ENC624J600_PHSTAT1_ANDONE                 0x0020
#define ENC624J600_PHSTAT1_LRFAULT                0x0010
#define ENC624J600_PHSTAT1_ANABLE                 0x0008
#define ENC624J600_PHSTAT1_LLSTAT                 0x0004
#define ENC624J600_PHSTAT1_EXTREGS                0x0001

//PHY Auto-Negotiation Advertisement register
#define ENC624J600_PHANA_ADNP                     0x8000
#define ENC624J600_PHANA_ADFAULT                  0x2000
#define ENC624J600_PHANA_ADPAUS1                  0x0800
#define ENC624J600_PHANA_ADPAUS0                  0x0400
#define ENC624J600_PHANA_AD100FD                  0x0100
#define ENC624J600_PHANA_AD100                    0x0080
#define ENC624J600_PHANA_AD10FD                   0x0040
#define ENC624J600_PHANA_AD10                     0x0020
#define ENC624J600_PHANA_ADIEEE                   0x001F
#define ENC624J600_PHANA_ADIEEE_DEFAULT           0x0001

//PHY Auto-Negotiation Link Partner Ability register
#define ENC624J600_PHANLPA_LPNP                   0x8000
#define ENC624J600_PHANLPA_LPACK                  0x4000
#define ENC624J600_PHANLPA_LPFAULT                0x2000
#define ENC624J600_PHANLPA_LPPAUS1                0x0800
#define ENC624J600_PHANLPA_LPPAUS0                0x0400
#define ENC624J600_PHANLPA_LP100T4                0x0200
#define ENC624J600_PHANLPA_LP100FD                0x0100
#define ENC624J600_PHANLPA_LP100                  0x0080
#define ENC624J600_PHANLPA_LP10FD                 0x0040
#define ENC624J600_PHANLPA_LP10                   0x0020
#define ENC624J600_PHANLPA_LPIEEE                 0x001F

//PHY Auto-Negotiation Expansion register
#define ENC624J600_PHANE_PDFLT                    0x0010
#define ENC624J600_PHANE_LPARCD                   0x0002
#define ENC624J600_PHANE_LPANABL                  0x0001

//PHY Control 2 register
#define ENC624J600_PHCON2_EDPWRDN                 0x2000
#define ENC624J600_PHCON2_EDTHRES                 0x0800
#define ENC624J600_PHCON2_FRCLNK                  0x0004
#define ENC624J600_PHCON2_EDSTAT                  0x0002

//PHY Status 2 register
#define ENC624J600_PHSTAT2_PLRITY                 0x0010

//PHY Status 3 register
#define ENC624J600_PHSTAT3_R6                     0x0040
#define ENC624J600_PHSTAT3_R6_DEFAULT             0x0040
#define ENC624J600_PHSTAT3_SPDDPX2                0x0010
#define ENC624J600_PHSTAT3_SPDDPX1                0x0008
#define ENC624J600_PHSTAT3_SPDDPX0                0x0004

//Receive status vector
#define ENC624J600_RSV_UNICAST_FILTER             0x00100000
#define ENC624J600_RSV_PATTERN_MATCH_FILTER       0x00080000
#define ENC624J600_RSV_MAGIC_PACKET_FILTER        0x00040000
#define ENC624J600_RSV_HASH_FILTER                0x00020000
#define ENC624J600_RSV_NOT_ME_FILTER              0x00010000
#define ENC624J600_RSV_RUNT_FILTER                0x00008000
#define ENC624J600_RSV_VLAN_TYPE                  0x00004000
#define ENC624J600_RSV_UNKNOWN_OPCODE             0x00002000
#define ENC624J600_RSV_PAUSE_CONTROL_FRAME        0x00001000
#define ENC624J600_RSV_CONTROL_FRAME              0x00000800
#define ENC624J600_RSV_DRIBBLE_NIBBLE             0x00000400
#define ENC624J600_RSV_BROADCAST_PACKET           0x00000200
#define ENC624J600_RSV_MULTICAST_PACKET           0x00000100
#define ENC624J600_RSV_RECEIVED_OK                0x00000080
#define ENC624J600_RSV_LENGTH_OUT_OF_RANGE        0x00000040
#define ENC624J600_RSV_LENGTH_CHECK_ERROR         0x00000020
#define ENC624J600_RSV_CRC_ERROR                  0x00000010
#define ENC624J600_RSV_CARRIER_EVENT              0x00000004
#define ENC624J600_RSV_PACKET_IGNORED             0x00000001

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief ENC624J600 driver context
 **/

typedef struct
{
   uint16_t nextPacket; ///<Next packet in the receive buffer
} Enc624j600Context;


//ENC624J600 driver
extern const NicDriver enc624j600Driver;

//ENC624J600 related functions
error_t enc624j600Init(NetInterface *interface);

void enc624j600Tick(NetInterface *interface);

void enc624j600EnableIrq(NetInterface *interface);
void enc624j600DisableIrq(NetInterface *interface);
bool_t enc624j600IrqHandler(NetInterface *interface);
void enc624j600EventHandler(NetInterface *interface);

error_t enc624j600SendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t enc624j600ReceivePacket(NetInterface *interface);

error_t enc624j600UpdateMacAddrFilter(NetInterface *interface);
void enc624j600UpdateMacConfig(NetInterface *interface);

error_t enc624j600SoftReset(NetInterface *interface);

void enc624j600WriteReg(NetInterface *interface, uint8_t address,
   uint16_t data);

uint16_t enc624j600ReadReg(NetInterface *interface, uint8_t address);

void enc624j600WritePhyReg(NetInterface *interface, uint8_t address,
   uint16_t data);

uint16_t enc624j600ReadPhyReg(NetInterface *interface, uint8_t address);

void enc624j600WriteBuffer(NetInterface *interface,
   uint8_t opcode, const NetBuffer *buffer, size_t offset);

void enc624j600ReadBuffer(NetInterface *interface,
   uint8_t opcode, uint8_t *data, size_t length);

void enc624j600SetBit(NetInterface *interface, uint8_t address,
   uint16_t mask);

void enc624j600ClearBit(NetInterface *interface, uint8_t address,
   uint16_t mask);

uint32_t enc624j600CalcCrc(const void *data, size_t length);

void enc624j600DumpReg(NetInterface *interface);
void enc624j600DumpPhyReg(NetInterface *interface);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
