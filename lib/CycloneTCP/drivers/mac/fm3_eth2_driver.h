/**
 * @file fm3_eth2_driver.h
 * @brief Cypress FM3 Ethernet MAC driver (ETHER1 instance)
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

#ifndef _FM3_ETH2_DRIVER_H
#define _FM3_ETH2_DRIVER_H

//Dependencies
#include "core/nic.h"

//Number of TX buffers
#ifndef FM3_ETH2_TX_BUFFER_COUNT
   #define FM3_ETH2_TX_BUFFER_COUNT 3
#elif (FM3_ETH2_TX_BUFFER_COUNT < 1)
   #error FM3_ETH2_TX_BUFFER_COUNT parameter is not valid
#endif

//TX buffer size
#ifndef FM3_ETH2_TX_BUFFER_SIZE
   #define FM3_ETH2_TX_BUFFER_SIZE 1536
#elif (FM3_ETH2_TX_BUFFER_SIZE != 1536)
   #error FM3_ETH2_TX_BUFFER_SIZE parameter is not valid
#endif

//Number of RX buffers
#ifndef FM3_ETH2_RX_BUFFER_COUNT
   #define FM3_ETH2_RX_BUFFER_COUNT 6
#elif (FM3_ETH2_RX_BUFFER_COUNT < 1)
   #error FM3_ETH2_RX_BUFFER_COUNT parameter is not valid
#endif

//RX buffer size
#ifndef FM3_ETH2_RX_BUFFER_SIZE
   #define FM3_ETH2_RX_BUFFER_SIZE 1536
#elif (FM3_ETH2_RX_BUFFER_SIZE != 1536)
   #error FM3_ETH2_RX_BUFFER_SIZE parameter is not valid
#endif

//Interrupt priority grouping
#ifndef FM3_ETH2_IRQ_PRIORITY_GROUPING
   #define FM3_ETH2_IRQ_PRIORITY_GROUPING 3
#elif (FM3_ETH2_IRQ_PRIORITY_GROUPING < 0)
   #error FM3_ETH2_IRQ_PRIORITY_GROUPING parameter is not valid
#endif

//Ethernet interrupt group priority
#ifndef FM3_ETH2_IRQ_GROUP_PRIORITY
   #define FM3_ETH2_IRQ_GROUP_PRIORITY 12
#elif (FM3_ETH2_IRQ_GROUP_PRIORITY < 0)
   #error FM3_ETH2_IRQ_GROUP_PRIORITY parameter is not valid
#endif

//Ethernet interrupt subpriority
#ifndef FM3_ETH2_IRQ_SUB_PRIORITY
   #define FM3_ETH2_IRQ_SUB_PRIORITY 0
#elif (FM3_ETH2_IRQ_SUB_PRIORITY < 0)
   #error FM3_ETH2_IRQ_SUB_PRIORITY parameter is not valid
#endif

//SR register
#define ETH_SR_GLPII   0x40000000
#define ETH_SR_TTI     0x20000000
#define ETH_SR_GPI     0x10000000
#define ETH_SR_GMI     0x08000000
#define ETH_SR_GLI     0x04000000
#define ETH_SR_EB      0x03800000
#define ETH_SR_TS      0x00700000
#define ETH_SR_RS      0x000E0000
#define ETH_SR_NIS     0x00010000
#define ETH_SR_AIS     0x00008000
#define ETH_SR_ERI     0x00004000
#define ETH_SR_FBI     0x00002000
#define ETH_SR_ETI     0x00000400
#define ETH_SR_RWT     0x00000200
#define ETH_SR_RPS     0x00000100
#define ETH_SR_RU      0x00000080
#define ETH_SR_RI      0x00000040
#define ETH_SR_UNF     0x00000020
#define ETH_SR_OVF     0x00000010
#define ETH_SR_TJT     0x00000008
#define ETH_SR_TU      0x00000004
#define ETH_SR_TPS     0x00000002
#define ETH_SR_TI      0x00000001

//Transmit DMA descriptor flags
#define ETH_TDES0_OWN  0x80000000
#define ETH_TDES0_IC   0x40000000
#define ETH_TDES0_LS   0x20000000
#define ETH_TDES0_FS   0x10000000
#define ETH_TDES0_DC   0x08000000
#define ETH_TDES0_DP   0x04000000
#define ETH_TDES0_TTSE 0x02000000
#define ETH_TDES0_CIC  0x00C00000
#define ETH_TDES0_TER  0x00200000
#define ETH_TDES0_TCH  0x00100000
#define ETH_TDES0_TTSS 0x00020000
#define ETH_TDES0_IHE  0x00010000
#define ETH_TDES0_ES   0x00008000
#define ETH_TDES0_JT   0x00004000
#define ETH_TDES0_FF   0x00002000
#define ETH_TDES0_IPE  0x00001000
#define ETH_TDES0_LCA  0x00000800
#define ETH_TDES0_NC   0x00000400
#define ETH_TDES0_LCO  0x00000200
#define ETH_TDES0_EC   0x00000100
#define ETH_TDES0_VF   0x00000080
#define ETH_TDES0_CC   0x00000078
#define ETH_TDES0_ED   0x00000004
#define ETH_TDES0_UF   0x00000002
#define ETH_TDES0_DB   0x00000001
#define ETH_TDES1_TBS2 0x1FFF0000
#define ETH_TDES1_TBS1 0x00001FFF
#define ETH_TDES2_B1AP 0xFFFFFFFF
#define ETH_TDES3_B2AP 0xFFFFFFFF
#define ETH_TDES6_TTSL 0xFFFFFFFF
#define ETH_TDES7_TTSH 0xFFFFFFFF

//Receive DMA descriptor flags
#define ETH_RDES0_OWN  0x80000000
#define ETH_RDES0_AFM  0x40000000
#define ETH_RDES0_FL   0x3FFF0000
#define ETH_RDES0_ES   0x00008000
#define ETH_RDES0_DE   0x00004000
#define ETH_RDES0_SAF  0x00002000
#define ETH_RDES0_LE   0x00001000
#define ETH_RDES0_OE   0x00000800
#define ETH_RDES0_VLAN 0x00000400
#define ETH_RDES0_FS   0x00000200
#define ETH_RDES0_LS   0x00000100
#define ETH_RDES0_TS   0x00000080
#define ETH_RDES0_LCO  0x00000040
#define ETH_RDES0_FT   0x00000020
#define ETH_RDES0_RWT  0x00000010
#define ETH_RDES0_RE   0x00000008
#define ETH_RDES0_DBE  0x00000004
#define ETH_RDES0_CE   0x00000002
#define ETH_RDES0_ESA  0x00000001
#define ETH_RDES1_DIC  0x80000000
#define ETH_RDES1_RBS2 0x1FFF0000
#define ETH_RDES1_RER  0x00008000
#define ETH_RDES1_RCH  0x00004000
#define ETH_RDES1_RBS1 0x00001FFF
#define ETH_RDES2_B1AP 0xFFFFFFFF
#define ETH_RDES3_B2AP 0xFFFFFFFF
#define ETH_RDES4_TD   0x00004000
#define ETH_RDES4_PV   0x00002000
#define ETH_RDES4_PFT  0x00001000
#define ETH_RDES4_MT   0x00000F00
#define ETH_RDES4_IP6R 0x00000080
#define ETH_RDES4_IP4R 0x00000040
#define ETH_RDES4_IPCB 0x00000020
#define ETH_RDES4_IPE  0x00000010
#define ETH_RDES4_IPHE 0x00000008
#define ETH_RDES4_IPT  0x00000007
#define ETH_RDES6_RTSL 0xFFFFFFFF
#define ETH_RDES7_RTSH 0xFFFFFFFF

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
} Fm3Eth2TxDmaDesc;


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
} Fm3Eth2RxDmaDesc;


//FM3 Ethernet MAC driver (ETHER1 instance)
extern const NicDriver fm3Eth2Driver;

//FM3 Ethernet MAC related functions
error_t fm3Eth2Init(NetInterface *interface);
void fm3Eth2InitGpio(NetInterface *interface);
void fm3Eth2InitDmaDesc(NetInterface *interface);

void fm3Eth2Tick(NetInterface *interface);

void fm3Eth2EnableIrq(NetInterface *interface);
void fm3Eth2DisableIrq(NetInterface *interface);
void fm3Eth2EventHandler(NetInterface *interface);

error_t fm3Eth2SendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t fm3Eth2ReceivePacket(NetInterface *interface);

error_t fm3Eth2UpdateMacAddrFilter(NetInterface *interface);
error_t fm3Eth2UpdateMacConfig(NetInterface *interface);

void fm3Eth2WritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data);

uint16_t fm3Eth2ReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr);

uint32_t fm3Eth2CalcCrc(const void *data, size_t length);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
