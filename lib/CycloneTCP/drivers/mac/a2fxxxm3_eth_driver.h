/**
 * @file a2fxxxm3_eth_driver.h
 * @brief SmartFusion (A2FxxxM3) Ethernet MAC driver
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

#ifndef _A2FXXXM3_ETH_DRIVER_H
#define _A2FXXXM3_ETH_DRIVER_H

//Dependencies
#include "core/nic.h"

//Number of TX buffers
#ifndef A2FXXXM3_ETH_TX_BUFFER_COUNT
   #define A2FXXXM3_ETH_TX_BUFFER_COUNT 2
#elif (A2FXXXM3_ETH_TX_BUFFER_COUNT < 1)
   #error A2FXXXM3_ETH_TX_BUFFER_COUNT parameter is not valid
#endif

//TX buffer size
#ifndef A2FXXXM3_ETH_TX_BUFFER_SIZE
   #define A2FXXXM3_ETH_TX_BUFFER_SIZE 1536
#elif (A2FXXXM3_ETH_TX_BUFFER_SIZE != 1536)
   #error A2FXXXM3_ETH_TX_BUFFER_SIZE parameter is not valid
#endif

//Number of RX buffers
#ifndef A2FXXXM3_ETH_RX_BUFFER_COUNT
   #define A2FXXXM3_ETH_RX_BUFFER_COUNT 4
#elif (A2FXXXM3_ETH_RX_BUFFER_COUNT < 1)
   #error A2FXXXM3_ETH_RX_BUFFER_COUNT parameter is not valid
#endif

//RX buffer size
#ifndef A2FXXXM3_ETH_RX_BUFFER_SIZE
   #define A2FXXXM3_ETH_RX_BUFFER_SIZE 1536
#elif (A2FXXXM3_ETH_RX_BUFFER_SIZE != 1536)
   #error A2FXXXM3_ETH_RX_BUFFER_SIZE parameter is not valid
#endif

//Interrupt priority grouping
#ifndef A2FXXXM3_ETH_IRQ_PRIORITY_GROUPING
   #define A2FXXXM3_ETH_IRQ_PRIORITY_GROUPING 2
#elif (A2FXXXM3_ETH_IRQ_PRIORITY_GROUPING < 0)
   #error A2FXXXM3_ETH_IRQ_PRIORITY_GROUPING parameter is not valid
#endif

//Ethernet interrupt group priority
#ifndef A2FXXXM3_ETH_IRQ_GROUP_PRIORITY
   #define A2FXXXM3_ETH_IRQ_GROUP_PRIORITY 24
#elif (A2FXXXM3_ETH_IRQ_GROUP_PRIORITY < 0)
   #error A2FXXXM3_ETH_IRQ_GROUP_PRIORITY parameter is not valid
#endif

//Ethernet interrupt subpriority
#ifndef A2FXXXM3_ETH_IRQ_SUB_PRIORITY
   #define A2FXXXM3_ETH_IRQ_SUB_PRIORITY 0
#elif (A2FXXXM3_ETH_IRQ_SUB_PRIORITY < 0)
   #error A2FXXXM3_ETH_IRQ_SUB_PRIORITY parameter is not valid
#endif

//MDEN bit definition
#ifndef CSR9_MDEN_MASK
   #define CSR9_MDEN_MASK CSR9_MII_MASK
#endif

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
} A2fxxxm3TxDmaDesc;


/**
 * @brief Receive DMA descriptor
 **/

typedef struct
{
   uint32_t rdes0;
   uint32_t rdes1;
   uint32_t rdes2;
   uint32_t rdes3;
} A2fxxxm3RxDmaDesc;


/**
 * @brief Hash table setup frame
 **/

typedef struct
{
   uint32_t hashFilter[32];  //0-127
   uint32_t reserved1[7];    //128-155
   uint32_t physicalAddr[3]; //156-167
   uint32_t reserved2[6];    //168-191
} A2fxxxm3HashTableSetupFrame;


//A2FxxxM3 Ethernet MAC driver
extern const NicDriver a2fxxxm3EthDriver;

//A2FxxxM3 Ethernet MAC related functions
error_t a2fxxxm3EthInit(NetInterface *interface);
void a2fxxxm3EthInitDmaDesc(NetInterface *interface);

void a2fxxxm3EthTick(NetInterface *interface);

void a2fxxxm3EthEnableIrq(NetInterface *interface);
void a2fxxxm3EthDisableIrq(NetInterface *interface);
void a2fxxxm3EthEventHandler(NetInterface *interface);

error_t a2fxxxm3EthSendSetup(NetInterface *interface);

error_t a2fxxxm3EthSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t a2fxxxm3EthReceivePacket(NetInterface *interface);

error_t a2fxxxm3EthUpdateMacAddrFilter(NetInterface *interface);
error_t a2fxxxm3EthUpdateMacConfig(NetInterface *interface);

void a2fxxxm3EthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data);

uint16_t a2fxxxm3EthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr);

void a2fxxxm3EthWriteSmi(uint32_t data, uint_t length);
uint32_t a2fxxxm3EthReadSmi(uint_t length);

uint32_t a2fxxxm3EthCalcCrc(const void *data, size_t length);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
