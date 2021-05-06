/**
 * @file s32k148_eth_driver.h
 * @brief NXP S32K148 Ethernet MAC driver
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

#ifndef _S32K148_ETH_DRIVER_H
#define _S32K148_ETH_DRIVER_H

//Number of TX buffers
#ifndef S32K148_ETH_TX_BUFFER_COUNT
   #define S32K148_ETH_TX_BUFFER_COUNT 3
#elif (S32K148_ETH_TX_BUFFER_COUNT < 1)
   #error S32K148_ETH_TX_BUFFER_COUNT parameter is not valid
#endif

//TX buffer size
#ifndef S32K148_ETH_TX_BUFFER_SIZE
   #define S32K148_ETH_TX_BUFFER_SIZE 1536
#elif (S32K148_ETH_TX_BUFFER_SIZE != 1536)
   #error S32K148_ETH_TX_BUFFER_SIZE parameter is not valid
#endif

//Number of RX buffers
#ifndef S32K148_ETH_RX_BUFFER_COUNT
   #define S32K148_ETH_RX_BUFFER_COUNT 6
#elif (S32K148_ETH_RX_BUFFER_COUNT < 1)
   #error S32K148_ETH_RX_BUFFER_COUNT parameter is not valid
#endif

//RX buffer size
#ifndef S32K148_ETH_RX_BUFFER_SIZE
   #define S32K148_ETH_RX_BUFFER_SIZE 1536
#elif (S32K148_ETH_RX_BUFFER_SIZE != 1536)
   #error S32K148_ETH_RX_BUFFER_SIZE parameter is not valid
#endif

//Interrupt priority grouping
#ifndef S32K148_ETH_IRQ_PRIORITY_GROUPING
   #define S32K148_ETH_IRQ_PRIORITY_GROUPING 3
#elif (S32K148_ETH_IRQ_PRIORITY_GROUPING < 0)
   #error S32K148_ETH_IRQ_PRIORITY_GROUPING parameter is not valid
#endif

//Ethernet interrupt group priority
#ifndef S32K148_ETH_IRQ_GROUP_PRIORITY
   #define S32K148_ETH_IRQ_GROUP_PRIORITY 12
#elif (S32K148_ETH_IRQ_GROUP_PRIORITY < 0)
   #error S32K148_ETH_IRQ_GROUP_PRIORITY parameter is not valid
#endif

//Ethernet interrupt subpriority
#ifndef S32K148_ETH_IRQ_SUB_PRIORITY
   #define S32K148_ETH_IRQ_SUB_PRIORITY 0
#elif (S32K148_ETH_IRQ_SUB_PRIORITY < 0)
   #error S32K148_ETH_IRQ_SUB_PRIORITY parameter is not valid
#endif

//Enhanced transmit buffer descriptor
#define ENET_TBD0_R                0x80000000
#define ENET_TBD0_TO1              0x40000000
#define ENET_TBD0_W                0x20000000
#define ENET_TBD0_TO2              0x10000000
#define ENET_TBD0_L                0x08000000
#define ENET_TBD0_TC               0x04000000
#define ENET_TBD0_DATA_LENGTH      0x0000FFFF
#define ENET_TBD1_DATA_POINTER     0xFFFFFFFF
#define ENET_TBD2_INT              0x40000000
#define ENET_TBD2_TS               0x20000000
#define ENET_TBD2_PINS             0x10000000
#define ENET_TBD2_IINS             0x08000000
#define ENET_TBD2_TXE              0x00008000
#define ENET_TBD2_UE               0x00002000
#define ENET_TBD2_EE               0x00001000
#define ENET_TBD2_FE               0x00000800
#define ENET_TBD2_LCE              0x00000400
#define ENET_TBD2_OE               0x00000200
#define ENET_TBD2_TSE              0x00000100
#define ENET_TBD4_BDU              0x80000000
#define ENET_TBD5_TIMESTAMP        0xFFFFFFFF

//Enhanced receive buffer descriptor
#define ENET_RBD0_E                0x80000000
#define ENET_RBD0_RO1              0x40000000
#define ENET_RBD0_W                0x20000000
#define ENET_RBD0_RO2              0x10000000
#define ENET_RBD0_L                0x08000000
#define ENET_RBD0_M                0x01000000
#define ENET_RBD0_BC               0x00800000
#define ENET_RBD0_MC               0x00400000
#define ENET_RBD0_LG               0x00200000
#define ENET_RBD0_NO               0x00100000
#define ENET_RBD0_CR               0x00040000
#define ENET_RBD0_OV               0x00020000
#define ENET_RBD0_TR               0x00010000
#define ENET_RBD0_DATA_LENGTH      0x0000FFFF
#define ENET_RBD1_DATA_POINTER     0xFFFFFFFF
#define ENET_RBD2_ME               0x80000000
#define ENET_RBD2_PE               0x04000000
#define ENET_RBD2_CE               0x02000000
#define ENET_RBD2_UC               0x01000000
#define ENET_RBD2_INT              0x00800000
#define ENET_RBD2_VPCP             0x0000E000
#define ENET_RBD2_ICE              0x00000020
#define ENET_RBD2_PCR              0x00000010
#define ENET_RBD2_VLAN             0x00000004
#define ENET_RBD2_IPV6             0x00000002
#define ENET_RBD2_FRAG             0x00000001
#define ENET_RBD3_HEADER_LENGTH    0xF8000000
#define ENET_RBD3_PROTOCOL_TYPE    0x00FF0000
#define ENET_RBD3_PAYLOAD_CHECKSUM 0x0000FFFF
#define ENET_RBD4_BDU              0x80000000
#define ENET_RBD5_TIMESTAMP        0xFFFFFFFF

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//S32K148 Ethernet MAC driver
extern const NicDriver s32k148EthDriver;

//S32K148 Ethernet MAC related functions
error_t s32k148EthInit(NetInterface *interface);
void s32k148EthInitGpio(NetInterface *interface);
void s32k148EthInitBufferDesc(NetInterface *interface);

void s32k148EthTick(NetInterface *interface);

void s32k148EthEnableIrq(NetInterface *interface);
void s32k148EthDisableIrq(NetInterface *interface);
void s32k148EthEventHandler(NetInterface *interface);

error_t s32k148EthSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t s32k148EthReceivePacket(NetInterface *interface);

error_t s32k148EthUpdateMacAddrFilter(NetInterface *interface);
error_t s32k148EthUpdateMacConfig(NetInterface *interface);

void s32k148EthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data);

uint16_t s32k148EthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr);

uint32_t s32k148EthCalcCrc(const void *data, size_t length);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
