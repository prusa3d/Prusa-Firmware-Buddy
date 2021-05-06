/**
 * @file mcf5225x_eth_driver.h
 * @brief Coldfire V2 MCF5225x Ethernet MAC driver
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

#ifndef _MCF5225X_ETH_DRIVER_H
#define _MCF5225X_ETH_DRIVER_H

//Number of TX buffers
#ifndef MCF5225X_ETH_TX_BUFFER_COUNT
   #define MCF5225X_ETH_TX_BUFFER_COUNT 2
#elif (MCF5225X_ETH_TX_BUFFER_COUNT < 1)
   #error MCF5225X_ETH_TX_BUFFER_COUNT parameter is not valid
#endif

//TX buffer size
#ifndef MCF5225X_ETH_TX_BUFFER_SIZE
   #define MCF5225X_ETH_TX_BUFFER_SIZE 1536
#elif (MCF5225X_ETH_TX_BUFFER_SIZE != 1536)
   #error MCF5225X_ETH_TX_BUFFER_SIZE parameter is not valid
#endif

//Number of RX buffers
#ifndef MCF5225X_ETH_RX_BUFFER_COUNT
   #define MCF5225X_ETH_RX_BUFFER_COUNT 4
#elif (MCF5225X_ETH_RX_BUFFER_COUNT < 1)
   #error MCF5225X_ETH_RX_BUFFER_COUNT parameter is not valid
#endif

//RX buffer size
#ifndef MCF5225X_ETH_RX_BUFFER_SIZE
   #define MCF5225X_ETH_RX_BUFFER_SIZE 1536
#elif (MCF5225X_ETH_RX_BUFFER_SIZE != 1536)
   #error MCF5225X_ETH_RX_BUFFER_SIZE parameter is not valid
#endif

//Ethernet interrupt level
#ifndef MCF5225X_ETH_IRQ_LEVEL
   #define MCF5225X_ETH_IRQ_LEVEL 4
#elif (MCF5225X_ETH_IRQ_LEVEL < 0)
   #error MCF5225X_ETH_IRQ_LEVEL parameter is not valid
#endif

//Ethernet interrupt priority
#ifndef MCF5225X_ETH_IRQ_PRIORITY
   #define MCF5225X_ETH_IRQ_PRIORITY 1
#elif (MCF5225X_ETH_IRQ_PRIORITY < 0)
   #error MCF5225X_ETH_IRQ_PRIORITY parameter is not valid
#endif

//Align to 16-byte boundary
#define FEC_ALIGN16(p) ((void *) ((((uint32_t) (p)) + 15) & 0xFFFFFFF0))

//Transmit buffer descriptor
#define FEC_TX_BD_R   0x8000
#define FEC_TX_BD_TO1 0x4000
#define FEC_TX_BD_W   0x2000
#define FEC_TX_BD_TO2 0x1000
#define FEC_TX_BD_L   0x0800
#define FEC_TX_BD_TC  0x0400
#define FEC_TX_BD_ABC 0x0200

//Receive buffer descriptor
#define FEC_RX_BD_E   0x8000
#define FEC_RX_BD_RO1 0x4000
#define FEC_RX_BD_W   0x2000
#define FEC_RX_BD_RO2 0x1000
#define FEC_RX_BD_L   0x0800
#define FEC_RX_BD_M   0x0100
#define FEC_RX_BD_BC  0x0080
#define FEC_RX_BD_MC  0x0040
#define FEC_RX_BD_LG  0x0020
#define FEC_RX_BD_NO  0x0010
#define FEC_RX_BD_CR  0x0004
#define FEC_RX_BD_OV  0x0002
#define FEC_RX_BD_TR  0x0001

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Transmit buffer descriptor
 **/

typedef struct
{
   uint16_t status;
   uint16_t length;
   uint32_t address;
} Mcf5225xTxBufferDesc;


/**
 * @brief Receive buffer descriptor
 **/

typedef struct
{
   uint16_t status;
   uint16_t length;
   uint32_t address;
} Mcf5225xRxBufferDesc;


//MCF5225x Ethernet MAC driver
extern const NicDriver mcf5225xEthDriver;

//MCF5225x Ethernet MAC related functions
error_t mcf5225xEthInit(NetInterface *interface);
void mcf5225xEthInitGpio(NetInterface *interface);
void mcf5225xEthInitBufferDesc(NetInterface *interface);

void mcf5225xEthTick(NetInterface *interface);

void mcf5225xEthEnableIrq(NetInterface *interface);
void mcf5225xEthDisableIrq(NetInterface *interface);
void mcf5225xEthEventHandler(NetInterface *interface);

error_t mcf5225xEthSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t mcf5225xEthReceivePacket(NetInterface *interface);

error_t mcf5225xEthUpdateMacAddrFilter(NetInterface *interface);
error_t mcf5225xEthUpdateMacConfig(NetInterface *interface);

void mcf5225xEthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data);

uint16_t mcf5225xEthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr);

uint32_t mcf5225xEthCalcCrc(const void *data, size_t length);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
