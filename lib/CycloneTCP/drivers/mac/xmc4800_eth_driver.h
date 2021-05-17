/**
 * @file xmc4800_eth_driver.h
 * @brief Infineon XMC4800 Ethernet MAC driver
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

#ifndef _XMC4800_ETH_DRIVER_H
#define _XMC4800_ETH_DRIVER_H

//Dependencies
#include "core/nic.h"

//Number of TX buffers
#ifndef XMC4800_ETH_TX_BUFFER_COUNT
   #define XMC4800_ETH_TX_BUFFER_COUNT 3
#elif (XMC4800_ETH_TX_BUFFER_COUNT < 1)
   #error XMC4800_ETH_TX_BUFFER_COUNT parameter is not valid
#endif

//TX buffer size
#ifndef XMC4800_ETH_TX_BUFFER_SIZE
   #define XMC4800_ETH_TX_BUFFER_SIZE 1536
#elif (XMC4800_ETH_TX_BUFFER_SIZE != 1536)
   #error XMC4800_ETH_TX_BUFFER_SIZE parameter is not valid
#endif

//Number of RX buffers
#ifndef XMC4800_ETH_RX_BUFFER_COUNT
   #define XMC4800_ETH_RX_BUFFER_COUNT 6
#elif (XMC4800_ETH_RX_BUFFER_COUNT < 1)
   #error XMC4800_ETH_RX_BUFFER_COUNT parameter is not valid
#endif

//RX buffer size
#ifndef XMC4800_ETH_RX_BUFFER_SIZE
   #define XMC4800_ETH_RX_BUFFER_SIZE 1536
#elif (XMC4800_ETH_RX_BUFFER_SIZE != 1536)
   #error XMC4800_ETH_RX_BUFFER_SIZE parameter is not valid
#endif

//Interrupt priority grouping
#ifndef XMC4800_ETH_IRQ_PRIORITY_GROUPING
   #define XMC4800_ETH_IRQ_PRIORITY_GROUPING 1
#elif (XMC4800_ETH_IRQ_PRIORITY_GROUPING < 0)
   #error XMC4800_ETH_IRQ_PRIORITY_GROUPING parameter is not valid
#endif

//Ethernet interrupt group priority
#ifndef XMC4800_ETH_IRQ_GROUP_PRIORITY
   #define XMC4800_ETH_IRQ_GROUP_PRIORITY 48
#elif (XMC4800_ETH_IRQ_GROUP_PRIORITY < 0)
   #error XMC4800_ETH_IRQ_GROUP_PRIORITY parameter is not valid
#endif

//Ethernet interrupt subpriority
#ifndef XMC4800_ETH_IRQ_SUB_PRIORITY
   #define XMC4800_ETH_IRQ_SUB_PRIORITY 0
#elif (XMC4800_ETH_IRQ_SUB_PRIORITY < 0)
   #error XMC4800_ETH_IRQ_SUB_PRIORITY parameter is not valid
#endif

//Name of the section where to place DMA buffers
#ifndef XMC4800_ETH_RAM_SECTION
   #define XMC4800_ETH_RAM_SECTION "ETH_RAM"
#endif

//ETH0_CON register
#define ETH_CON_MDIO_A     (0 << ETH_CON_MDIO_Pos)
#define ETH_CON_MDIO_B     (1 << ETH_CON_MDIO_Pos)
#define ETH_CON_MDIO_C     (2 << ETH_CON_MDIO_Pos)
#define ETH_CON_MDIO_D     (3 << ETH_CON_MDIO_Pos)

#define ETH_CON_CLK_TX_A   (0 << ETH_CON_CLK_TX_Pos)
#define ETH_CON_CLK_TX_B   (1 << ETH_CON_CLK_TX_Pos)
#define ETH_CON_CLK_TX_C   (2 << ETH_CON_CLK_TX_Pos)
#define ETH_CON_CLK_TX_D   (3 << ETH_CON_CLK_TX_Pos)

#define ETH_CON_COL_A      (0 << ETH_CON_COL_Pos)
#define ETH_CON_COL_B      (1 << ETH_CON_COL_Pos)
#define ETH_CON_COL_C      (2 << ETH_CON_COL_Pos)
#define ETH_CON_COL_D      (3 << ETH_CON_COL_Pos)

#define ETH_CON_RXER_A     (0 << ETH_CON_RXER_Pos)
#define ETH_CON_RXER_B     (1 << ETH_CON_RXER_Pos)
#define ETH_CON_RXER_C     (2 << ETH_CON_RXER_Pos)
#define ETH_CON_RXER_D     (3 << ETH_CON_RXER_Pos)

#define ETH_CON_CRS_A      (0 << ETH_CON_CRS_Pos)
#define ETH_CON_CRS_B      (1 << ETH_CON_CRS_Pos)
#define ETH_CON_CRS_C      (2 << ETH_CON_CRS_Pos)
#define ETH_CON_CRS_D      (3 << ETH_CON_CRS_Pos)

#define ETH_CON_CRS_DV_A   (0 << ETH_CON_CRS_DV_Pos)
#define ETH_CON_CRS_DV_B   (1 << ETH_CON_CRS_DV_Pos)
#define ETH_CON_CRS_DV_C   (2 << ETH_CON_CRS_DV_Pos)
#define ETH_CON_CRS_DV_D   (3 << ETH_CON_CRS_DV_Pos)

#define ETH_CON_CLK_RMII_A (0 << ETH_CON_CLK_RMII_Pos)
#define ETH_CON_CLK_RMII_B (1 << ETH_CON_CLK_RMII_Pos)
#define ETH_CON_CLK_RMII_C (2 << ETH_CON_CLK_RMII_Pos)
#define ETH_CON_CLK_RMII_D (3 << ETH_CON_CLK_RMII_Pos)

#define ETH_CON_RXD3_A     (0 << ETH_CON_RXD3_Pos)
#define ETH_CON_RXD3_B     (1 << ETH_CON_RXD3_Pos)
#define ETH_CON_RXD3_C     (2 << ETH_CON_RXD3_Pos)
#define ETH_CON_RXD3_D     (3 << ETH_CON_RXD3_Pos)

#define ETH_CON_RXD2_A     (0 << ETH_CON_RXD2_Pos)
#define ETH_CON_RXD2_B     (1 << ETH_CON_RXD2_Pos)
#define ETH_CON_RXD2_C     (2 << ETH_CON_RXD2_Pos)
#define ETH_CON_RXD2_D     (3 << ETH_CON_RXD2_Pos)

#define ETH_CON_RXD1_A     (0 << ETH_CON_RXD1_Pos)
#define ETH_CON_RXD1_B     (1 << ETH_CON_RXD1_Pos)
#define ETH_CON_RXD1_C     (2 << ETH_CON_RXD1_Pos)
#define ETH_CON_RXD1_D     (3 << ETH_CON_RXD1_Pos)

#define ETH_CON_RXD0_A     (0 << ETH_CON_RXD0_Pos)
#define ETH_CON_RXD0_B     (1 << ETH_CON_RXD0_Pos)
#define ETH_CON_RXD0_C     (2 << ETH_CON_RXD0_Pos)
#define ETH_CON_RXD0_D     (3 << ETH_CON_RXD0_Pos)

//ETH0_MAC_CONFIGURATION register
#define ETH_MAC_CONFIGURATION_RESERVED15_Msk (1 << 15)

//ETH0_GMII_ADDRESS register
#define ETH_GMII_ADDRESS_CR_DIV42  (0 << ETH_GMII_ADDRESS_CR_Pos)
#define ETH_GMII_ADDRESS_CR_DIV62  (1 << ETH_GMII_ADDRESS_CR_Pos)
#define ETH_GMII_ADDRESS_CR_DIV16  (2 << ETH_GMII_ADDRESS_CR_Pos)
#define ETH_GMII_ADDRESS_CR_DIV26  (3 << ETH_GMII_ADDRESS_CR_Pos)
#define ETH_GMII_ADDRESS_CR_DIV102 (4 << ETH_GMII_ADDRESS_CR_Pos)
#define ETH_GMII_ADDRESS_CR_DIV124 (5 << ETH_GMII_ADDRESS_CR_Pos)

//ETH0_BUS_MODE register
#define ETH_BUS_MODE_RPBL_1  (1 << ETH_BUS_MODE_RPBL_Pos)
#define ETH_BUS_MODE_RPBL_2  (2 << ETH_BUS_MODE_RPBL_Pos)
#define ETH_BUS_MODE_RPBL_4  (4 << ETH_BUS_MODE_RPBL_Pos)
#define ETH_BUS_MODE_RPBL_8  (8 << ETH_BUS_MODE_RPBL_Pos)
#define ETH_BUS_MODE_RPBL_16 (16 << ETH_BUS_MODE_RPBL_Pos)
#define ETH_BUS_MODE_RPBL_32 (32 << ETH_BUS_MODE_RPBL_Pos)

#define ETH_BUS_MODE_PR_1_1  (0 << ETH_BUS_MODE_PR_Pos)
#define ETH_BUS_MODE_PR_2_1  (1 << ETH_BUS_MODE_PR_Pos)
#define ETH_BUS_MODE_PR_3_1  (2 << ETH_BUS_MODE_PR_Pos)
#define ETH_BUS_MODE_PR_4_1  (3 << ETH_BUS_MODE_PR_Pos)

#define ETH_BUS_MODE_PBL_1   (1 << ETH_BUS_MODE_PBL_Pos)
#define ETH_BUS_MODE_PBL_2   (2 << ETH_BUS_MODE_PBL_Pos)
#define ETH_BUS_MODE_PBL_4   (4 << ETH_BUS_MODE_PBL_Pos)
#define ETH_BUS_MODE_PBL_8   (8 << ETH_BUS_MODE_PBL_Pos)
#define ETH_BUS_MODE_PBL_16  (16 << ETH_BUS_MODE_PBL_Pos)
#define ETH_BUS_MODE_PBL_32  (32 << ETH_BUS_MODE_PBL_Pos)

//Transmit DMA descriptor flags
#define ETH_TDES0_OWN     0x80000000
#define ETH_TDES0_IC      0x40000000
#define ETH_TDES0_LS      0x20000000
#define ETH_TDES0_FS      0x10000000
#define ETH_TDES0_DC      0x08000000
#define ETH_TDES0_DP      0x04000000
#define ETH_TDES0_TTSE    0x02000000
#define ETH_TDES0_CIC     0x00C00000
#define ETH_TDES0_TER     0x00200000
#define ETH_TDES0_TCH     0x00100000
#define ETH_TDES0_TTSS    0x00020000
#define ETH_TDES0_IHE     0x00010000
#define ETH_TDES0_ES      0x00008000
#define ETH_TDES0_JT      0x00004000
#define ETH_TDES0_FF      0x00002000
#define ETH_TDES0_IPE     0x00001000
#define ETH_TDES0_LCA     0x00000800
#define ETH_TDES0_NC      0x00000400
#define ETH_TDES0_LCO     0x00000200
#define ETH_TDES0_EC      0x00000100
#define ETH_TDES0_VF      0x00000080
#define ETH_TDES0_CC      0x00000078
#define ETH_TDES0_ED      0x00000004
#define ETH_TDES0_UF      0x00000002
#define ETH_TDES0_DB      0x00000001
#define ETH_TDES1_TBS2    0x1FFF0000
#define ETH_TDES1_TBS1    0x00001FFF
#define ETH_TDES2_TBAP1   0xFFFFFFFF
#define ETH_TDES3_TBAP2   0xFFFFFFFF

//Receive DMA descriptor flags
#define ETH_RDES0_OWN     0x80000000
#define ETH_RDES0_AFM     0x40000000
#define ETH_RDES0_FL      0x3FFF0000
#define ETH_RDES0_ES      0x00008000
#define ETH_RDES0_DE      0x00004000
#define ETH_RDES0_SAF     0x00002000
#define ETH_RDES0_LE      0x00001000
#define ETH_RDES0_OE      0x00000800
#define ETH_RDES0_VLAN    0x00000400
#define ETH_RDES0_FS      0x00000200
#define ETH_RDES0_LS      0x00000100
#define ETH_RDES0_IPCE_GF 0x00000080
#define ETH_RDES0_LCO     0x00000040
#define ETH_RDES0_FT      0x00000020
#define ETH_RDES0_RWT     0x00000010
#define ETH_RDES0_RE      0x00000008
#define ETH_RDES0_DBE     0x00000004
#define ETH_RDES0_CE      0x00000002
#define ETH_RDES0_PCE     0x00000001
#define ETH_RDES1_DIC     0x80000000
#define ETH_RDES1_RBS2    0x1FFF0000
#define ETH_RDES1_RER     0x00008000
#define ETH_RDES1_RCH     0x00004000
#define ETH_RDES1_RBS1    0x00001FFF
#define ETH_RDES2_RBAP1   0xFFFFFFFF
#define ETH_RDES3_RBAP2   0xFFFFFFFF

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Transmit DMA descriptor
 **/

typedef struct
{
   uint32_t tdes0;
   uint32_t tdes1;
   uint32_t tdes2;
   uint32_t tdes3;
} Xmc4800TxDmaDesc;


/**
 * @brief Receive DMA descriptor
 **/

typedef struct
{
   uint32_t rdes0;
   uint32_t rdes1;
   uint32_t rdes2;
   uint32_t rdes3;
} Xmc4800RxDmaDesc;


//XMC4800 Ethernet MAC driver
extern const NicDriver xmc4800EthDriver;

//XMC4800 Ethernet MAC related functions
error_t xmc4800EthInit(NetInterface *interface);
void xmc4800EthInitGpio(NetInterface *interface);
void xmc4800EthInitDmaDesc(NetInterface *interface);

void xmc4800EthTick(NetInterface *interface);

void xmc4800EthEnableIrq(NetInterface *interface);
void xmc4800EthDisableIrq(NetInterface *interface);
void xmc4800EthEventHandler(NetInterface *interface);

error_t xmc4800EthSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t xmc4800EthReceivePacket(NetInterface *interface);

error_t xmc4800EthUpdateMacAddrFilter(NetInterface *interface);
error_t xmc4800EthUpdateMacConfig(NetInterface *interface);

void xmc4800EthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data);

uint16_t xmc4800EthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr);

uint32_t xmc4800EthCalcCrc(const void *data, size_t length);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
