/**
 * @file s5d9_eth_driver.h
 * @brief Renesas Synergy S5D9 Ethernet MAC driver
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

#ifndef _S5D9_ETH_DRIVER_H
#define _S5D9_ETH_DRIVER_H

//Dependencies
#include "core/nic.h"

//Number of TX buffers
#ifndef S5D9_ETH_TX_BUFFER_COUNT
   #define S5D9_ETH_TX_BUFFER_COUNT 3
#elif (S5D9_ETH_TX_BUFFER_COUNT < 1)
   #error S5D9_ETH_TX_BUFFER_COUNT parameter is not valid
#endif

//TX buffer size
#ifndef S5D9_ETH_TX_BUFFER_SIZE
   #define S5D9_ETH_TX_BUFFER_SIZE 1536
#elif (S5D9_ETH_TX_BUFFER_SIZE != 1536)
   #error S5D9_ETH_TX_BUFFER_SIZE parameter is not valid
#endif

//Number of RX buffers
#ifndef S5D9_ETH_RX_BUFFER_COUNT
   #define S5D9_ETH_RX_BUFFER_COUNT 6
#elif (S5D9_ETH_RX_BUFFER_COUNT < 1)
   #error S5D9_ETH_RX_BUFFER_COUNT parameter is not valid
#endif

//RX buffer size
#ifndef S5D9_ETH_RX_BUFFER_SIZE
   #define S5D9_ETH_RX_BUFFER_SIZE 1536
#elif (S5D9_ETH_RX_BUFFER_SIZE != 1536)
   #error S5D9_ETH_RX_BUFFER_SIZE parameter is not valid
#endif

//Interrupt priority grouping
#ifndef S5D9_ETH_IRQ_PRIORITY_GROUPING
   #define S5D9_ETH_IRQ_PRIORITY_GROUPING 3
#elif (S5D9_ETH_IRQ_PRIORITY_GROUPING < 0)
   #error S5D9_ETH_IRQ_PRIORITY_GROUPING parameter is not valid
#endif

//Ethernet interrupt group priority
#ifndef S5D9_ETH_IRQ_GROUP_PRIORITY
   #define S5D9_ETH_IRQ_GROUP_PRIORITY 12
#elif (S5D9_ETH_IRQ_GROUP_PRIORITY < 0)
   #error S5D9_ETH_IRQ_GROUP_PRIORITY parameter is not valid
#endif

//Ethernet interrupt subpriority
#ifndef S5D9_ETH_IRQ_SUB_PRIORITY
   #define S5D9_ETH_IRQ_SUB_PRIORITY 0
#elif (S5D9_ETH_IRQ_SUB_PRIORITY < 0)
   #error S5D9_ETH_IRQ_SUB_PRIORITY parameter is not valid
#endif

//EESR register
#define EDMAC_EESR_TWB     0x40000000
#define EDMAC_EESR_TABT    0x04000000
#define EDMAC_EESR_RABT    0x02000000
#define EDMAC_EESR_RFCOF   0x01000000
#define EDMAC_EESR_ADE     0x00800000
#define EDMAC_EESR_ECI     0x00400000
#define EDMAC_EESR_TC      0x00200000
#define EDMAC_EESR_TDE     0x00100000
#define EDMAC_EESR_TFUF    0x00080000
#define EDMAC_EESR_FR      0x00040000
#define EDMAC_EESR_RDE     0x00020000
#define EDMAC_EESR_RFOF    0x00010000
#define EDMAC_EESR_CND     0x00000800
#define EDMAC_EESR_DLC     0x00000400
#define EDMAC_EESR_CD      0x00000200
#define EDMAC_EESR_TRO     0x00000100
#define EDMAC_EESR_RMAF    0x00000080
#define EDMAC_EESR_RRF     0x00000010
#define EDMAC_EESR_RTLF    0x00000008
#define EDMAC_EESR_RTSF    0x00000004
#define EDMAC_EESR_PRE     0x00000002
#define EDMAC_EESR_CERF    0x00000001

//Transmit DMA descriptor flags
#define EDMAC_TD0_TACT     0x80000000
#define EDMAC_TD0_TDLE     0x40000000
#define EDMAC_TD0_TFP_SOF  0x20000000
#define EDMAC_TD0_TFP_EOF  0x10000000
#define EDMAC_TD0_TFE      0x08000000
#define EDMAC_TD0_TWBI     0x04000000
#define EDMAC_TD0_TFS_MASK 0x0000010F
#define EDMAC_TD0_TFS_TABT 0x00000100
#define EDMAC_TD0_TFS_CND  0x00000008
#define EDMAC_TD0_TFS_DLC  0x00000004
#define EDMAC_TD0_TFS_CD   0x00000002
#define EDMAC_TD0_TFS_TRO  0x00000001
#define EDMAC_TD1_TBL      0xFFFF0000
#define EDMAC_TD2_TBA      0xFFFFFFFF

//Receive DMA descriptor flags
#define EDMAC_RD0_RACT     0x80000000
#define EDMAC_RD0_RDLE     0x40000000
#define EDMAC_RD0_RFP_SOF  0x20000000
#define EDMAC_RD0_RFP_EOF  0x10000000
#define EDMAC_RD0_RFE      0x08000000
#define EDMAC_RD0_RFS_MASK 0x0000039F
#define EDMAC_RD0_RFS_RFOF 0x00000200
#define EDMAC_RD0_RFS_RABT 0x00000100
#define EDMAC_RD0_RFS_RMAF 0x00000080
#define EDMAC_RD0_RFS_RRF  0x00000010
#define EDMAC_RD0_RFS_RTLF 0x00000008
#define EDMAC_RD0_RFS_RTSF 0x00000004
#define EDMAC_RD0_RFS_PRE  0x00000002
#define EDMAC_RD0_RFS_CERF 0x00000001
#define EDMAC_RD1_RBL      0xFFFF0000
#define EDMAC_RD1_RFL      0x0000FFFF
#define EDMAC_RD2_RBA      0xFFFFFFFF

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Transmit DMA descriptor
 **/

typedef struct
{
   uint32_t td0;
   uint32_t td1;
   uint32_t td2;
   uint32_t padding;
} S5d9TxDmaDesc;


/**
 * @brief Receive DMA descriptor
 **/

typedef struct
{
   uint32_t rd0;
   uint32_t rd1;
   uint32_t rd2;
   uint32_t padding;
} S5d9RxDmaDesc;


//S5D9 Ethernet MAC driver
extern const NicDriver s5d9EthDriver;

//S5D9 Ethernet MAC related functions
error_t s5d9EthInit(NetInterface *interface);
void s5d9EthInitGpio(NetInterface *interface);
void s5d9EthInitDmaDesc(NetInterface *interface);

void s5d9EthTick(NetInterface *interface);

void s5d9EthEnableIrq(NetInterface *interface);
void s5d9EthDisableIrq(NetInterface *interface);
void s5d9EthEventHandler(NetInterface *interface);

error_t s5d9EthSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t s5d9EthReceivePacket(NetInterface *interface);

error_t s5d9EthUpdateMacAddrFilter(NetInterface *interface);
error_t s5d9EthUpdateMacConfig(NetInterface *interface);

void s5d9EthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data);

uint16_t s5d9EthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr);

void s5d9EthWriteSmi(uint32_t data, uint_t length);
uint32_t s5d9EthReadSmi(uint_t length);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
