/**
 * @file aps3_eth_driver.h
 * @brief Cortus APS3 Ethernet MAC driver
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

#ifndef _APS3_ETH_DRIVER_H
#define _APS3_ETH_DRIVER_H

//Dependencies
#include "core/nic.h"

//Number of TX buffers
#ifndef APS3_ETH_TX_BUFFER_COUNT
   #define APS3_ETH_TX_BUFFER_COUNT 4
#elif (APS3_ETH_TX_BUFFER_COUNT < 1)
   #error APS3_ETH_TX_BUFFER_COUNT parameter is not valid
#endif

//TX buffer size
#ifndef APS3_ETH_TX_BUFFER_SIZE
   #define APS3_ETH_TX_BUFFER_SIZE 1536
#elif (APS3_ETH_TX_BUFFER_SIZE != 1536)
   #error APS3_ETH_TX_BUFFER_SIZE parameter is not valid
#endif

//Number of RX buffers
#ifndef APS3_ETH_RX_BUFFER_COUNT
   #define APS3_ETH_RX_BUFFER_COUNT 4
#elif (APS3_ETH_RX_BUFFER_COUNT < 1)
   #error APS3_ETH_RX_BUFFER_COUNT parameter is not valid
#endif

//RX buffer size
#ifndef APS3_ETH_RX_BUFFER_SIZE
   #define APS3_ETH_RX_BUFFER_SIZE 1536
#elif (APS3_ETH_RX_BUFFER_SIZE != 1536)
   #error APS3_ETH_RX_BUFFER_SIZE parameter is not valid
#endif

//Ethernet interrupt priority
#ifndef APS3_ETH_IRQ_PRIORITY
   #define APS3_ETH_IRQ_PRIORITY 0
#elif (APS3_ETH_IRQ_PRIORITY < 0)
   #error APS3_ETH_IRQ_PRIORITY parameter is not valid
#endif

//tx_irq_mask register
#define TX_IRQ_MASK_TRANSMIT_ERROR      0x0001
#define TX_IRQ_MASK_EXCESSIVE_DEFERRAL  0x0002
#define TX_IRQ_MASK_EXCESSIVE_COLLISION 0x0004
#define TX_IRQ_MASK_LATE_COLLISION      0x0008
#define TX_IRQ_MASK_FRAME_TOO_LONG      0x0010
#define TX_IRQ_MASK_MEMORY_ERROR        0x0020
#define TX_IRQ_MASK_FRAME_SENT          0x0040
#define TX_IRQ_MASK_MEMORY_AVAILABLE    0x0080
#define TX_IRQ_MASK_THRESHOLD_REACHED   0x0100
#define TX_IRQ_MASK_MEMORY_EMPTY        0x0200

//rx_irq_mask register
#define RX_IRQ_MASK_RECEIVE_ERROR       0x0001
#define RX_IRQ_MASK_LENGTH_FIELD_ERROR  0x0002
#define RX_IRQ_MASK_FRAME_TOO_LONG      0x0004
#define RX_IRQ_MASK_SHORT_FRAME         0x0008
#define RX_IRQ_MASK_ODD_NIBBLE_COUNT    0x0010
#define RX_IRQ_MASK_INVALID_ADDRESS     0x0020
#define RX_IRQ_MASK_PHY_ERROR           0x0040
#define RX_IRQ_MASK_CRC_ERROR           0x0080
#define RX_IRQ_MASK_MEMORY_ERROR        0x0100
#define RX_IRQ_MASK_WAKEUP_ON_LAN       0x0200
#define RX_IRQ_MASK_FRAME_READY         0x0400
#define RX_IRQ_MASK_THRESHOLD_REACHED   0x0800
#define RX_IRQ_MASK_FRAME_OVERFLOW      0x1000

//Transmit DMA descriptor flags
#define TX_DESC_TRANSMIT_ERROR          0x0001
#define TX_DESC_EXCESSIVE_DEFERRAL      0x0002
#define TX_DESC_EXCESSIVE_COLLISION     0x0003
#define TX_DESC_LATE_COLLISION          0x0004
#define TX_DESC_FRAME_TOO_LONG          0x0010
#define TX_DESC_MEMORY_ERROR            0x0020

//Receive DMA descriptor flags
#define RX_DESC_RECEIVE_ERROR           0x0001
#define RX_DESC_LENGTH_FIELD_ERROR      0x0002
#define RX_DESC_FRAME_TOO_LONG          0x0003
#define RX_DESC_SHORT_FRAME             0x0004
#define RX_DESC_ODD_NIBBLE_COUNT        0x0010
#define RX_DESC_INVALID_ADDRESS         0x0020
#define RX_DESC_PHY_ERROR               0x0040
#define RX_DESC_CRC_ERROR               0x0080
#define RX_DESC_MEMORY_ERROR            0x0100

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief TX DMA descriptor
 **/

typedef struct
{
   uint32_t addr;
   uint32_t size : 16;
   uint32_t status : 16;
} Aps3TxDmaDesc;


/**
 * @brief RX DMA descriptor
 **/

typedef struct
{
   uint32_t addr;
   uint32_t size : 16;
   uint32_t status : 16;
} Aps3RxDmaDesc;


//Cortus APS3 Ethernet MAC driver
extern const NicDriver aps3EthDriver;

//Cortus APS3 Ethernet MAC related functions
error_t aps3EthInit(NetInterface *interface);
void aps3EthInitDmaDesc(NetInterface *interface);

void aps3EthTick(NetInterface *interface);

void aps3EthEnableIrq(NetInterface *interface);
void aps3EthDisableIrq(NetInterface *interface);

void aps3EthTxIrqHandler(void) __attribute__((noinline));
void aps3EthRxIrqHandler(void) __attribute__((noinline));

void aps3EthEventHandler(NetInterface *interface);

error_t aps3EthSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t aps3EthReceivePacket(NetInterface *interface);

error_t aps3EthUpdateMacAddrFilter(NetInterface *interface);
error_t aps3EthUpdateMacConfig(NetInterface *interface);

void aps3EthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data);

uint16_t aps3EthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr);

uint32_t aps3EthCalcCrc(const void *data, size_t length);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
