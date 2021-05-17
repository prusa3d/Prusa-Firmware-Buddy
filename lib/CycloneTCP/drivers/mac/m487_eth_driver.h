/**
 * @file m487_eth_driver.h
 * @brief Nuvoton M487 Ethernet MAC driver
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

#ifndef _M487_ETH_DRIVER_H
#define _M487_ETH_DRIVER_H

//Dependencies
#include "core/nic.h"

//Number of TX buffers
#ifndef M487_ETH_TX_BUFFER_COUNT
   #define M487_ETH_TX_BUFFER_COUNT 2
#elif (M487_ETH_TX_BUFFER_COUNT < 1)
   #error M487_ETH_TX_BUFFER_COUNT parameter is not valid
#endif

//TX buffer size
#ifndef M487_ETH_TX_BUFFER_SIZE
   #define M487_ETH_TX_BUFFER_SIZE 1536
#elif (M487_ETH_TX_BUFFER_SIZE != 1536)
   #error M487_ETH_TX_BUFFER_SIZE parameter is not valid
#endif

//Number of RX buffers
#ifndef M487_ETH_RX_BUFFER_COUNT
   #define M487_ETH_RX_BUFFER_COUNT 4
#elif (M487_ETH_RX_BUFFER_COUNT < 1)
   #error M487_ETH_RX_BUFFER_COUNT parameter is not valid
#endif

//RX buffer size
#ifndef M487_ETH_RX_BUFFER_SIZE
   #define M487_ETH_RX_BUFFER_SIZE 1536
#elif (M487_ETH_RX_BUFFER_SIZE != 1536)
   #error M487_ETH_RX_BUFFER_SIZE parameter is not valid
#endif

//Interrupt priority grouping
#ifndef M487_ETH_IRQ_PRIORITY_GROUPING
   #define M487_ETH_IRQ_PRIORITY_GROUPING 3
#elif (M487_ETH_IRQ_PRIORITY_GROUPING < 0)
   #error M487_ETH_IRQ_PRIORITY_GROUPING parameter is not valid
#endif

//Ethernet interrupt group priority
#ifndef M487_ETH_IRQ_GROUP_PRIORITY
   #define M487_ETH_IRQ_GROUP_PRIORITY 12
#elif (M487_ETH_IRQ_GROUP_PRIORITY < 0)
   #error M487_ETH_IRQ_GROUP_PRIORITY parameter is not valid
#endif

//Ethernet interrupt subpriority
#ifndef M487_ETH_IRQ_SUB_PRIORITY
   #define M487_ETH_IRQ_SUB_PRIORITY 0
#elif (M487_ETH_IRQ_SUB_PRIORITY < 0)
   #error M487_ETH_IRQ_SUB_PRIORITY parameter is not valid
#endif

//Transmit DMA descriptor flags
#define EMAC_TXDES0_OWNER    0x80000000
#define EMAC_TXDES0_TTSEN    0x00000008
#define EMAC_TXDES0_INTEN    0x00000004
#define EMAC_TXDES0_CRCAPP   0x00000002
#define EMAC_TXDES0_PADEN    0x00000001
#define EMAC_TXDES1_TXBSA    0xFFFFFFFF
#define EMAC_TXDES2_COLCNT   0xF0000000
#define EMAC_TXDES2_TTSAS    0x08000000
#define EMAC_TXDES2_SQE      0x04000000
#define EMAC_TXDES2_TXPAUSED 0x02000000
#define EMAC_TXDES2_TXHALT   0x01000000
#define EMAC_TXDES2_LCIF     0x00800000
#define EMAC_TXDES2_TXABTIF  0x00400000
#define EMAC_TXDES2_NCSIF    0x00200000
#define EMAC_TXDES2_EXDEFIF  0x00100000
#define EMAC_TXDES2_TXCPIF   0x00080000
#define EMAC_TXDES2_DEF      0x00020000
#define EMAC_TXDES2_TXIF     0x00010000
#define EMAC_TXDES2_TBC      0x0000FFFF
#define EMAC_TXDES2_NTXDSA   0xFFFFFFFF

//Receive DMA descriptor flags
#define EMAC_RXDES0_OWNER    0x80000000
#define EMAC_RXDES0_RTSAS    0x00800000
#define EMAC_RXDES0_RPIF     0x00400000
#define EMAC_RXDES0_ALIEIF   0x00200000
#define EMAC_RXDES0_RXGDIF   0x00100000
#define EMAC_RXDES0_LPIF     0x00080000
#define EMAC_RXDES0_CRCEIF   0x00020000
#define EMAC_RXDES0_RXIF     0x00010000
#define EMAC_RXDES0_RBC      0x0000FFFF
#define EMAC_RXDES1_RXBSA    0xFFFFFFFF
#define EMAC_RXDES3_NRXDSA   0xFFFFFFFF

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief TX DMA descriptor
 **/

typedef struct
{
   uint32_t txdes0;
   uint32_t txdes1;
   uint32_t txdes2;
   uint32_t txdes3;
} Nuc472TxDmaDesc;


/**
 * @brief RX DMA descriptor
 **/

typedef struct
{
   uint32_t rxdes0;
   uint32_t rxdes1;
   uint32_t rxdes2;
   uint32_t rxdes3;
} Nuc472RxDmaDesc;


//M487 Ethernet MAC driver
extern const NicDriver m487EthDriver;

//M487 Ethernet MAC related functions
error_t m487EthInit(NetInterface *interface);
void m487EthInitGpio(NetInterface *interface);
void m487EthInitDmaDesc(NetInterface *interface);

void m487EthTick(NetInterface *interface);

void m487EthEnableIrq(NetInterface *interface);
void m487EthDisableIrq(NetInterface *interface);
void m487EthEventHandler(NetInterface *interface);

error_t m487EthSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t m487EthReceivePacket(NetInterface *interface);

error_t m487EthUpdateMacAddrFilter(NetInterface *interface);
error_t m487EthUpdateMacConfig(NetInterface *interface);

void m487EthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data);

uint16_t m487EthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
