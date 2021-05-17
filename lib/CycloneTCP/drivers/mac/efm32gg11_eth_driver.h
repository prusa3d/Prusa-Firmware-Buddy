/**
 * @file efm32gg11_eth_driver.h
 * @brief EFM32 Giant Gecko 11 Ethernet MAC driver
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

#ifndef _EFM32GG11_ETH_DRIVER_H
#define _EFM32GG11_ETH_DRIVER_H

//Number of TX buffers
#ifndef EFM32GG11_ETH_TX_BUFFER_COUNT
   #define EFM32GG11_ETH_TX_BUFFER_COUNT 2
#elif (EFM32GG11_ETH_TX_BUFFER_COUNT < 1)
   #error EFM32GG11_ETH_TX_BUFFER_COUNT parameter is not valid
#endif

//TX buffer size
#ifndef EFM32GG11_ETH_TX_BUFFER_SIZE
   #define EFM32GG11_ETH_TX_BUFFER_SIZE 1536
#elif (EFM32GG11_ETH_TX_BUFFER_SIZE != 1536)
   #error EFM32GG11_ETH_TX_BUFFER_SIZE parameter is not valid
#endif

//Number of RX buffers
#ifndef EFM32GG11_ETH_RX_BUFFER_COUNT
   #define EFM32GG11_ETH_RX_BUFFER_COUNT 48
#elif (EFM32GG11_ETH_RX_BUFFER_COUNT < 12)
   #error EFM32GG11_ETH_RX_BUFFER_COUNT parameter is not valid
#endif

//RX buffer size
#ifndef EFM32GG11_ETH_RX_BUFFER_SIZE
   #define EFM32GG11_ETH_RX_BUFFER_SIZE 128
#elif (EFM32GG11_ETH_RX_BUFFER_SIZE != 128)
   #error EFM32GG11_ETH_RX_BUFFER_SIZE parameter is not valid
#endif

//Interrupt priority grouping
#ifndef EFM32GG11_ETH_IRQ_PRIORITY_GROUPING
   #define EFM32GG11_ETH_IRQ_PRIORITY_GROUPING 4
#elif (EFM32GG11_ETH_IRQ_PRIORITY_GROUPING < 0)
   #error EFM32GG11_ETH_IRQ_PRIORITY_GROUPING parameter is not valid
#endif

//Ethernet interrupt group priority
#ifndef EFM32GG11_ETH_IRQ_GROUP_PRIORITY
   #define EFM32GG11_ETH_IRQ_GROUP_PRIORITY 6
#elif (EFM32GG11_ETH_IRQ_GROUP_PRIORITY < 0)
   #error EFM32GG11_ETH_IRQ_GROUP_PRIORITY parameter is not valid
#endif

//Ethernet interrupt subpriority
#ifndef EFM32GG11_ETH_IRQ_SUB_PRIORITY
   #define EFM32GG11_ETH_IRQ_SUB_PRIORITY 0
#elif (EFM32GG11_ETH_IRQ_SUB_PRIORITY < 0)
   #error EFM32GG11_ETH_IRQ_SUB_PRIORITY parameter is not valid
#endif

//TX buffer descriptor flags
#define ETH_TX_USED           0x80000000
#define ETH_TX_WRAP           0x40000000
#define ETH_TX_ERROR          0x20000000
#define ETH_TX_UNDERRUN       0x10000000
#define ETH_TX_EXHAUSTED      0x08000000
#define ETH_TX_NO_CRC         0x00010000
#define ETH_TX_LAST           0x00008000
#define ETH_TX_LENGTH         0x000007FF

//RX buffer descriptor flags
#define ETH_RX_ADDRESS        0xFFFFFFFC
#define ETH_RX_WRAP           0x00000002
#define ETH_RX_OWNERSHIP      0x00000001
#define ETH_RX_BROADCAST      0x80000000
#define ETH_RX_MULTICAST_HASH 0x40000000
#define ETH_RX_UNICAST_HASH   0x20000000
#define ETH_RX_EXT_ADDR       0x10000000
#define ETH_RX_SAR1           0x04000000
#define ETH_RX_SAR2           0x02000000
#define ETH_RX_SAR3           0x01000000
#define ETH_RX_SAR4           0x00800000
#define ETH_RX_TYPE_ID        0x00400000
#define ETH_RX_VLAN_TAG       0x00200000
#define ETH_RX_PRIORITY_TAG   0x00100000
#define ETH_RX_VLAN_PRIORITY  0x000E0000
#define ETH_RX_CFI            0x00010000
#define ETH_RX_EOF            0x00008000
#define ETH_RX_SOF            0x00004000
#define ETH_RX_OFFSET         0x00003000
#define ETH_RX_LENGTH         0x00000FFF

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Transmit buffer descriptor
 **/

typedef struct
{
   uint32_t address;
   uint32_t status;
} Efm32gg11TxBufferDesc;


/**
 * @brief Receive buffer descriptor
 **/

typedef struct
{
   uint32_t address;
   uint32_t status;
} Efm32gg11RxBufferDesc;


//EFM32GG11 Ethernet MAC driver
extern const NicDriver efm32gg11EthDriver;

//EFM32GG11 Ethernet MAC related functions
error_t efm32gg11EthInit(NetInterface *interface);
void efm32gg11EthInitGpio(NetInterface *interface);
void efm32gg11EthInitBufferDesc(NetInterface *interface);

void efm32gg11EthTick(NetInterface *interface);

void efm32gg11EthEnableIrq(NetInterface *interface);
void efm32gg11EthDisableIrq(NetInterface *interface);
void efm32gg11EthEventHandler(NetInterface *interface);

error_t efm32gg11EthSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t efm32gg11EthReceivePacket(NetInterface *interface);

error_t efm32gg11EthUpdateMacAddrFilter(NetInterface *interface);
error_t efm32gg11EthUpdateMacConfig(NetInterface *interface);

void efm32gg11EthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data);

uint16_t efm32gg11EthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
