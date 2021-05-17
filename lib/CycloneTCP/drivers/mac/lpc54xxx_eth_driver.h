/**
 * @file lpc54xxx_eth_driver.h
 * @brief LPC540xx/LPC546xx Ethernet MAC driver
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

#ifndef _LPC54XXX_ETH_DRIVER_H
#define _LPC54XXX_ETH_DRIVER_H

//Dependencies
#include "core/nic.h"

//Number of TX buffers
#ifndef LPC54XXX_ETH_TX_BUFFER_COUNT
   #define LPC54XXX_ETH_TX_BUFFER_COUNT 3
#elif (LPC54XXX_ETH_TX_BUFFER_COUNT < 1)
   #error LPC54XXX_ETH_TX_BUFFER_COUNT parameter is not valid
#endif

//TX buffer size
#ifndef LPC54XXX_ETH_TX_BUFFER_SIZE
   #define LPC54XXX_ETH_TX_BUFFER_SIZE 1536
#elif (LPC54XXX_ETH_TX_BUFFER_SIZE != 1536)
   #error LPC54XXX_ETH_TX_BUFFER_SIZE parameter is not valid
#endif

//Number of RX buffers
#ifndef LPC54XXX_ETH_RX_BUFFER_COUNT
   #define LPC54XXX_ETH_RX_BUFFER_COUNT 6
#elif (LPC54XXX_ETH_RX_BUFFER_COUNT < 1)
   #error LPC54XXX_ETH_RX_BUFFER_COUNT parameter is not valid
#endif

//RX buffer size
#ifndef LPC54XXX_ETH_RX_BUFFER_SIZE
   #define LPC54XXX_ETH_RX_BUFFER_SIZE 1536
#elif (LPC54XXX_ETH_RX_BUFFER_SIZE != 1536)
   #error LPC54XXX_ETH_RX_BUFFER_SIZE parameter is not valid
#endif

//Interrupt priority grouping
#ifndef LPC54XXX_ETH_IRQ_PRIORITY_GROUPING
   #define LPC54XXX_ETH_IRQ_PRIORITY_GROUPING 4
#elif (LPC54XXX_ETH_IRQ_PRIORITY_GROUPING < 0)
   #error LPC54XXX_ETH_IRQ_PRIORITY_GROUPING parameter is not valid
#endif

//Ethernet interrupt group priority
#ifndef LPC54XXX_ETH_IRQ_GROUP_PRIORITY
   #define LPC54XXX_ETH_IRQ_GROUP_PRIORITY 6
#elif (LPC54XXX_ETH_IRQ_GROUP_PRIORITY < 0)
   #error LPC54XXX_ETH_IRQ_GROUP_PRIORITY parameter is not valid
#endif

//Ethernet interrupt subpriority
#ifndef LPC54XXX_ETH_IRQ_SUB_PRIORITY
   #define LPC54XXX_ETH_IRQ_SUB_PRIORITY 0
#elif (LPC54XXX_ETH_IRQ_SUB_PRIORITY < 0)
   #error LPC54XXX_ETH_IRQ_SUB_PRIORITY parameter is not valid
#endif

//Transmit normal descriptor (read format)
#define ENET_TDES0_BUF1AP        0xFFFFFFFF
#define ENET_TDES1_BUF2AP        0xFFFFFFFF
#define ENET_TDES2_IOC           0x80000000
#define ENET_TDES2_TTSE          0x40000000
#define ENET_TDES2_B2L           0x3FFF0000
#define ENET_TDES2_B1L           0x00003FFF
#define ENET_TDES3_OWN           0x80000000
#define ENET_TDES3_CTXT          0x40000000
#define ENET_TDES3_FD            0x20000000
#define ENET_TDES3_LD            0x10000000
#define ENET_TDES3_CPC           0x0C000000
#define ENET_TDES3_SLOTNUM       0x00780000
#define ENET_TDES3_CIC           0x00030000
#define ENET_TDES3_FL            0x00007FFF

//Transmit normal descriptor (write-back format)
#define ENET_TDES0_TTSL          0xFFFFFFFF
#define ENET_TDES1_TTSH          0xFFFFFFFF
#define ENET_TDES3_OWN           0x80000000
#define ENET_TDES3_CTXT          0x40000000
#define ENET_TDES3_FD            0x20000000
#define ENET_TDES3_LD            0x10000000
#define ENET_TDES3_TTSS          0x00020000
#define ENET_TDES3_ES            0x00008000
#define ENET_TDES3_JT            0x00004000
#define ENET_TDES3_FF            0x00002000
#define ENET_TDES3_PCE           0x00001000
#define ENET_TDES3_LOC           0x00000800
#define ENET_TDES3_NC            0x00000400
#define ENET_TDES3_LC            0x00000200
#define ENET_TDES3_EC            0x00000100
#define ENET_TDES3_CC            0x000000F0
#define ENET_TDES3_ED            0x00000008
#define ENET_TDES3_UF            0x00000004
#define ENET_TDES3_DB            0x00000002
#define ENET_TDES3_IHE           0x00000001

//Receive normal descriptor (read format)
#define ENET_RDES0_BUF1AP        0xFFFFFFFF
#define ENET_RDES2_BUF2AP        0xFFFFFFFF
#define ENET_RDES3_OWN           0x80000000
#define ENET_RDES3_IOC           0x40000000
#define ENET_RDES3_BUF2V         0x02000000
#define ENET_RDES3_BUF1V         0x01000000

//Receive normal descriptor (write-back format)
#define ENET_RDES1_OPC           0xFFFF0000
#define ENET_RDES1_TD            0x00008000
#define ENET_RDES1_TSA           0x00004000
#define ENET_RDES1_PV            0x00002000
#define ENET_RDES1_PFT           0x00001000
#define ENET_RDES1_PMT           0x00000F00
#define ENET_RDES1_IPCE          0x00000080
#define ENET_RDES1_IPCB          0x00000040
#define ENET_RDES1_IPV6          0x00000020
#define ENET_RDES1_IPV4          0x00000010
#define ENET_RDES1_IPHE          0x00000008
#define ENET_RDES1_PT            0x00000007
#define ENET_RDES2_MADRM         0x07F80000
#define ENET_RDES2_DAF           0x00020000
#define ENET_RDES2_SAF           0x00010000
#define ENET_RDES3_OWN           0x80000000
#define ENET_RDES3_CTXT          0x40000000
#define ENET_RDES3_FD            0x20000000
#define ENET_RDES3_LD            0x10000000
#define ENET_RDES3_RS2V          0x08000000
#define ENET_RDES3_RS1V          0x04000000
#define ENET_RDES3_RS0V          0x02000000
#define ENET_RDES3_CE            0x01000000
#define ENET_RDES3_GP            0x00800000
#define ENET_RDES3_RWT           0x00400000
#define ENET_RDES3_OE            0x00200000
#define ENET_RDES3_RE            0x00100000
#define ENET_RDES3_DE            0x00080000
#define ENET_RDES3_LT            0x00070000
#define ENET_RDES3_ES            0x00008000
#define ENET_RDES3_PL            0x00007FFF

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Transmit descriptor
 **/

typedef struct
{
   uint32_t tdes0;
   uint32_t tdes1;
   uint32_t tdes2;
   uint32_t tdes3;
} Lpc54xxxTxDmaDesc;


/**
 * @brief Receive descriptor
 **/

typedef struct
{
   uint32_t rdes0;
   uint32_t rdes1;
   uint32_t rdes2;
   uint32_t rdes3;
} Lpc54xxxRxDmaDesc;


//LPC54xxx Ethernet MAC driver
extern const NicDriver lpc54xxxEthDriver;

//LPC54xxx Ethernet MAC related functions
error_t lpc54xxxEthInit(NetInterface *interface);
void lpc54xxxEthInitGpio(NetInterface *interface);
void lpc54xxxEthInitDmaDesc(NetInterface *interface);

void lpc54xxxEthTick(NetInterface *interface);

void lpc54xxxEthEnableIrq(NetInterface *interface);
void lpc54xxxEthDisableIrq(NetInterface *interface);
void lpc54xxxEthEventHandler(NetInterface *interface);

error_t lpc54xxxEthSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t lpc54xxxEthReceivePacket(NetInterface *interface);

error_t lpc54xxxEthUpdateMacAddrFilter(NetInterface *interface);
error_t lpc54xxxEthUpdateMacConfig(NetInterface *interface);

void lpc54xxxEthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data);

uint16_t lpc54xxxEthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
