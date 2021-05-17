/**
 * @file lpc43xx_eth_driver.h
 * @brief LPC4300 Ethernet MAC driver
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

#ifndef _LPC43XX_ETH_DRIVER_H
#define _LPC43XX_ETH_DRIVER_H

//Dependencies
#include "core/nic.h"

//Number of TX buffers
#ifndef LPC43XX_ETH_TX_BUFFER_COUNT
   #define LPC43XX_ETH_TX_BUFFER_COUNT 3
#elif (LPC43XX_ETH_TX_BUFFER_COUNT < 1)
   #error LPC43XX_ETH_TX_BUFFER_COUNT parameter is not valid
#endif

//TX buffer size
#ifndef LPC43XX_ETH_TX_BUFFER_SIZE
   #define LPC43XX_ETH_TX_BUFFER_SIZE 1536
#elif (LPC43XX_ETH_TX_BUFFER_SIZE != 1536)
   #error LPC43XX_ETH_TX_BUFFER_SIZE parameter is not valid
#endif

//Number of RX buffers
#ifndef LPC43XX_ETH_RX_BUFFER_COUNT
   #define LPC43XX_ETH_RX_BUFFER_COUNT 6
#elif (LPC43XX_ETH_RX_BUFFER_COUNT < 1)
   #error LPC43XX_ETH_RX_BUFFER_COUNT parameter is not valid
#endif

//RX buffer size
#ifndef LPC43XX_ETH_RX_BUFFER_SIZE
   #define LPC43XX_ETH_RX_BUFFER_SIZE 1536
#elif (LPC43XX_ETH_RX_BUFFER_SIZE != 1536)
   #error LPC43XX_ETH_RX_BUFFER_SIZE parameter is not valid
#endif

//Interrupt priority grouping
#ifndef LPC43XX_ETH_IRQ_PRIORITY_GROUPING
   #define LPC43XX_ETH_IRQ_PRIORITY_GROUPING 4
#elif (LPC43XX_ETH_IRQ_PRIORITY_GROUPING < 0)
   #error LPC43XX_ETH_IRQ_PRIORITY_GROUPING parameter is not valid
#endif

//Ethernet interrupt group priority
#ifndef LPC43XX_ETH_IRQ_GROUP_PRIORITY
   #define LPC43XX_ETH_IRQ_GROUP_PRIORITY 6
#elif (LPC43XX_ETH_IRQ_GROUP_PRIORITY < 0)
   #error LPC43XX_ETH_IRQ_GROUP_PRIORITY parameter is not valid
#endif

//Ethernet interrupt subpriority
#ifndef LPC43XX_ETH_IRQ_SUB_PRIORITY
   #define LPC43XX_ETH_IRQ_SUB_PRIORITY 0
#elif (LPC43XX_ETH_IRQ_SUB_PRIORITY < 0)
   #error LPC43XX_ETH_IRQ_SUB_PRIORITY parameter is not valid
#endif

//CREG6 register
#define CREG6_ETHMODE_MII               (0 << CREG_CREG6_ETHMODE_Pos)
#define CREG6_ETHMODE_RMII              (4 << CREG_CREG6_ETHMODE_Pos)

//MAC_MII_ADDR register
#define ETHERNET_MAC_MII_ADDR_CR_DIV42  (0 << ETHERNET_MAC_MII_ADDR_CR_Pos)
#define ETHERNET_MAC_MII_ADDR_CR_DIV62  (1 << ETHERNET_MAC_MII_ADDR_CR_Pos)
#define ETHERNET_MAC_MII_ADDR_CR_DIV16  (2 << ETHERNET_MAC_MII_ADDR_CR_Pos)
#define ETHERNET_MAC_MII_ADDR_CR_DIV26  (3 << ETHERNET_MAC_MII_ADDR_CR_Pos)
#define ETHERNET_MAC_MII_ADDR_CR_DIV102 (4 << ETHERNET_MAC_MII_ADDR_CR_Pos)
#define ETHERNET_MAC_MII_ADDR_CR_DIV124 (5 << ETHERNET_MAC_MII_ADDR_CR_Pos)

//DMA_BUS_MODE register
#define ETHERNET_DMA_BUS_MODE_RPBL_1    (1 << ETHERNET_DMA_BUS_MODE_RPBL_Pos)
#define ETHERNET_DMA_BUS_MODE_RPBL_2    (2 << ETHERNET_DMA_BUS_MODE_RPBL_Pos)
#define ETHERNET_DMA_BUS_MODE_RPBL_4    (4 << ETHERNET_DMA_BUS_MODE_RPBL_Pos)
#define ETHERNET_DMA_BUS_MODE_RPBL_8    (8 << ETHERNET_DMA_BUS_MODE_RPBL_Pos)
#define ETHERNET_DMA_BUS_MODE_RPBL_16   (16 << ETHERNET_DMA_BUS_MODE_RPBL_Pos)
#define ETHERNET_DMA_BUS_MODE_RPBL_32   (32 << ETHERNET_DMA_BUS_MODE_RPBL_Pos)

#define ETHERNET_DMA_BUS_MODE_PR_1_1    (0 << ETHERNET_DMA_BUS_MODE_PR_Pos)
#define ETHERNET_DMA_BUS_MODE_PR_2_1    (1 << ETHERNET_DMA_BUS_MODE_PR_Pos)
#define ETHERNET_DMA_BUS_MODE_PR_3_1    (2 << ETHERNET_DMA_BUS_MODE_PR_Pos)
#define ETHERNET_DMA_BUS_MODE_PR_4_1    (3 << ETHERNET_DMA_BUS_MODE_PR_Pos)

#define ETHERNET_DMA_BUS_MODE_PBL_1     (1 << ETHERNET_DMA_BUS_MODE_PBL_Pos)
#define ETHERNET_DMA_BUS_MODE_PBL_2     (2 << ETHERNET_DMA_BUS_MODE_PBL_Pos)
#define ETHERNET_DMA_BUS_MODE_PBL_4     (4 << ETHERNET_DMA_BUS_MODE_PBL_Pos)
#define ETHERNET_DMA_BUS_MODE_PBL_8     (8 << ETHERNET_DMA_BUS_MODE_PBL_Pos)
#define ETHERNET_DMA_BUS_MODE_PBL_16    (16 << ETHERNET_DMA_BUS_MODE_PBL_Pos)
#define ETHERNET_DMA_BUS_MODE_PBL_32    (32 << ETHERNET_DMA_BUS_MODE_PBL_Pos)

//DMA_OP_MODE register
#define ETHERNET_DMA_OP_MODE_TTC_64     (0 << ETHERNET_DMA_OP_MODE_TTC_Pos)
#define ETHERNET_DMA_OP_MODE_TTC_128    (1 << ETHERNET_DMA_OP_MODE_TTC_Pos)
#define ETHERNET_DMA_OP_MODE_TTC_192    (2 << ETHERNET_DMA_OP_MODE_TTC_Pos)
#define ETHERNET_DMA_OP_MODE_TTC_256    (3 << ETHERNET_DMA_OP_MODE_TTC_Pos)
#define ETHERNET_DMA_OP_MODE_TTC_40     (4 << ETHERNET_DMA_OP_MODE_TTC_Pos)
#define ETHERNET_DMA_OP_MODE_TTC_32     (5 << ETHERNET_DMA_OP_MODE_TTC_Pos)
#define ETHERNET_DMA_OP_MODE_TTC_24     (6 << ETHERNET_DMA_OP_MODE_TTC_Pos)
#define ETHERNET_DMA_OP_MODE_TTC_16     (7 << ETHERNET_DMA_OP_MODE_TTC_Pos)

#define ETHERNET_DMA_OP_MODE_RTC_64     (0 << ETHERNET_DMA_OP_MODE_RTC_Pos)
#define ETHERNET_DMA_OP_MODE_RTC_32     (1 << ETHERNET_DMA_OP_MODE_RTC_Pos)
#define ETHERNET_DMA_OP_MODE_RTC_96     (2 << ETHERNET_DMA_OP_MODE_RTC_Pos)
#define ETHERNET_DMA_OP_MODE_RTC_128    (3 << ETHERNET_DMA_OP_MODE_RTC_Pos)

//Transmit DMA descriptor flags
#define ETH_TDES0_OWN        0x80000000
#define ETH_TDES0_IC         0x40000000
#define ETH_TDES0_LS         0x20000000
#define ETH_TDES0_FS         0x10000000
#define ETH_TDES0_DC         0x08000000
#define ETH_TDES0_DP         0x04000000
#define ETH_TDES0_TTSE       0x02000000
#define ETH_TDES0_TER        0x00200000
#define ETH_TDES0_TCH        0x00100000
#define ETH_TDES0_TTSS       0x00020000
#define ETH_TDES0_IHE        0x00010000
#define ETH_TDES0_ES         0x00008000
#define ETH_TDES0_JT         0x00004000
#define ETH_TDES0_FF         0x00002000
#define ETH_TDES0_IPE        0x00001000
#define ETH_TDES0_LCA        0x00000800
#define ETH_TDES0_NC         0x00000400
#define ETH_TDES0_LCO        0x00000200
#define ETH_TDES0_EC         0x00000100
#define ETH_TDES0_VF         0x00000080
#define ETH_TDES0_CC         0x00000078
#define ETH_TDES0_ED         0x00000004
#define ETH_TDES0_UF         0x00000002
#define ETH_TDES0_DB         0x00000001
#define ETH_TDES1_TBS2       0x1FFF0000
#define ETH_TDES1_TBS1       0x00001FFF
#define ETH_TDES2_B1ADD      0xFFFFFFFF
#define ETH_TDES3_B2ADD      0xFFFFFFFF
#define ETH_TDES6_TTSL       0xFFFFFFFF
#define ETH_TDES7_TTSH       0xFFFFFFFF

//Receive DMA descriptor flags
#define ETH_RDES0_OWN        0x80000000
#define ETH_RDES0_AFM        0x40000000
#define ETH_RDES0_FL         0x3FFF0000
#define ETH_RDES0_ES         0x00008000
#define ETH_RDES0_DE         0x00004000
#define ETH_RDES0_SAF        0x00002000
#define ETH_RDES0_LE         0x00001000
#define ETH_RDES0_OE         0x00000800
#define ETH_RDES0_VLAN       0x00000400
#define ETH_RDES0_FS         0x00000200
#define ETH_RDES0_LS         0x00000100
#define ETH_RDES0_TSA        0x00000080
#define ETH_RDES0_LCO        0x00000040
#define ETH_RDES0_FT         0x00000020
#define ETH_RDES0_RWT        0x00000010
#define ETH_RDES0_RE         0x00000008
#define ETH_RDES0_DBE        0x00000004
#define ETH_RDES0_CE         0x00000002
#define ETH_RDES0_ESA        0x00000001
#define ETH_RDES1_RBS2       0x1FFF0000
#define ETH_RDES1_RER        0x00008000
#define ETH_RDES1_RCH        0x00004000
#define ETH_RDES1_RBS1       0x00001FFF
#define ETH_RDES2_B1ADD      0xFFFFFFFF
#define ETH_RDES3_B2ADD      0xFFFFFFFF
#define ETH_RDES4_PTPVERSION 0x00002000
#define ETH_RDES4_PTPTYPE    0x00001000
#define ETH_RDES4_MT         0x00000F00
#define ETH_RDES4_IPV6       0x00000080
#define ETH_RDES4_IPV4       0x00000040
#define ETH_RDES6_RTSL       0xFFFFFFFF
#define ETH_RDES7_RTSH       0xFFFFFFFF

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Enhanced TX DMA descriptor
 **/

typedef struct
{
   uint32_t tdes0;
   uint32_t tdes1;
   uint32_t tdes2;
   uint32_t tdes3;
   uint32_t tdes4;
   uint32_t tdes5;
   uint32_t tdes6;
   uint32_t tdes7;
} Lpc43xxTxDmaDesc;


/**
 * @brief Enhanced RX DMA descriptor
 **/

typedef struct
{
   uint32_t rdes0;
   uint32_t rdes1;
   uint32_t rdes2;
   uint32_t rdes3;
   uint32_t rdes4;
   uint32_t rdes5;
   uint32_t rdes6;
   uint32_t rdes7;
} Lpc43xxRxDmaDesc;


//LPC43xx Ethernet MAC driver
extern const NicDriver lpc43xxEthDriver;

//LPC43xx Ethernet MAC related functions
error_t lpc43xxEthInit(NetInterface *interface);
void lpc43xxEthInitGpio(NetInterface *interface);
void lpc43xxEthInitDmaDesc(NetInterface *interface);

void lpc43xxEthTick(NetInterface *interface);

void lpc43xxEthEnableIrq(NetInterface *interface);
void lpc43xxEthDisableIrq(NetInterface *interface);
void lpc43xxEthEventHandler(NetInterface *interface);

error_t lpc43xxEthSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t lpc43xxEthReceivePacket(NetInterface *interface);

error_t lpc43xxEthUpdateMacAddrFilter(NetInterface *interface);
error_t lpc43xxEthUpdateMacConfig(NetInterface *interface);

void lpc43xxEthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data);

uint16_t lpc43xxEthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr);

uint32_t lpc43xxEthCalcCrc(const void *data, size_t length);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
