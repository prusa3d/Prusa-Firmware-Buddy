/**
 * @file msp432e4_eth_driver.h
 * @brief MSP432E4 Ethernet controller
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

#ifndef _MSP432E4_ETH_DRIVER_H
#define _MSP432E4_ETH_DRIVER_H

//Dependencies
#include "core/nic.h"

//Number of TX buffers
#ifndef MSP432E4_ETH_TX_BUFFER_COUNT
   #define MSP432E4_ETH_TX_BUFFER_COUNT 3
#elif (MSP432E4_ETH_TX_BUFFER_COUNT < 1)
   #error MSP432E4_ETH_TX_BUFFER_COUNT parameter is not valid
#endif

//TX buffer size
#ifndef MSP432E4_ETH_TX_BUFFER_SIZE
   #define MSP432E4_ETH_TX_BUFFER_SIZE 1536
#elif (MSP432E4_ETH_TX_BUFFER_SIZE != 1536)
   #error MSP432E4_ETH_TX_BUFFER_SIZE parameter is not valid
#endif

//Number of RX buffers
#ifndef MSP432E4_ETH_RX_BUFFER_COUNT
   #define MSP432E4_ETH_RX_BUFFER_COUNT 6
#elif (MSP432E4_ETH_RX_BUFFER_COUNT < 1)
   #error MSP432E4_ETH_RX_BUFFER_COUNT parameter is not valid
#endif

//RX buffer size
#ifndef MSP432E4_ETH_RX_BUFFER_SIZE
   #define MSP432E4_ETH_RX_BUFFER_SIZE 1536
#elif (MSP432E4_ETH_RX_BUFFER_SIZE != 1536)
   #error MSP432E4_ETH_RX_BUFFER_SIZE parameter is not valid
#endif

//Interrupt priority grouping
#ifndef MSP432E4_ETH_IRQ_PRIORITY_GROUPING
   #define MSP432E4_ETH_IRQ_PRIORITY_GROUPING 3
#elif (MSP432E4_ETH_IRQ_PRIORITY_GROUPING < 0)
   #error MSP432E4_ETH_IRQ_PRIORITY_GROUPING parameter is not valid
#endif

//Ethernet interrupt priority
#ifndef MSP432E4_ETH_IRQ_PRIORITY
   #define MSP432E4_ETH_IRQ_PRIORITY 192
#elif (MSP432E4_ETH_IRQ_PRIORITY < 0)
   #error MSP432E4_ETH_IRQ_PRIORITY parameter is not valid
#endif

//DMABUSMOD register
#define EMAC_DMABUSMOD_RPBL_1  (1 << EMAC_DMABUSMOD_RPBL_S)
#define EMAC_DMABUSMOD_RPBL_2  (2 << EMAC_DMABUSMOD_RPBL_S)
#define EMAC_DMABUSMOD_RPBL_4  (4 << EMAC_DMABUSMOD_RPBL_S)
#define EMAC_DMABUSMOD_RPBL_8  (8 << EMAC_DMABUSMOD_RPBL_S)
#define EMAC_DMABUSMOD_RPBL_16 (16 << EMAC_DMABUSMOD_RPBL_S)
#define EMAC_DMABUSMOD_RPBL_32 (32 << EMAC_DMABUSMOD_RPBL_S)

#define EMAC_DMABUSMOD_PR_1_1  (0 << EMAC_DMABUSMOD_PR_S)
#define EMAC_DMABUSMOD_PR_2_1  (1 << EMAC_DMABUSMOD_PR_S)
#define EMAC_DMABUSMOD_PR_3_1  (2 << EMAC_DMABUSMOD_PR_S)
#define EMAC_DMABUSMOD_PR_4_1  (3 << EMAC_DMABUSMOD_PR_S)

#define EMAC_DMABUSMOD_PBL_1   (1 << EMAC_DMABUSMOD_PBL_S)
#define EMAC_DMABUSMOD_PBL_2   (2 << EMAC_DMABUSMOD_PBL_S)
#define EMAC_DMABUSMOD_PBL_4   (4 << EMAC_DMABUSMOD_PBL_S)
#define EMAC_DMABUSMOD_PBL_8   (8 << EMAC_DMABUSMOD_PBL_S)
#define EMAC_DMABUSMOD_PBL_16  (16 << EMAC_DMABUSMOD_PBL_S)
#define EMAC_DMABUSMOD_PBL_32  (32 << EMAC_DMABUSMOD_PBL_S)

//Transmit DMA descriptor flags
#define EMAC_TDES0_OWN    0x80000000
#define EMAC_TDES0_IC     0x40000000
#define EMAC_TDES0_LS     0x20000000
#define EMAC_TDES0_FS     0x10000000
#define EMAC_TDES0_DC     0x08000000
#define EMAC_TDES0_DP     0x04000000
#define EMAC_TDES0_TTSE   0x02000000
#define EMAC_TDES0_CRCR   0x01000000
#define EMAC_TDES0_CIC    0x00C00000
#define EMAC_TDES0_TER    0x00200000
#define EMAC_TDES0_TCH    0x00100000
#define EMAC_TDES0_VLIC   0x000C0000
#define EMAC_TDES0_TTSS   0x00020000
#define EMAC_TDES0_IHE    0x00010000
#define EMAC_TDES0_ES     0x00008000
#define EMAC_TDES0_JT     0x00004000
#define EMAC_TDES0_FF     0x00002000
#define EMAC_TDES0_IPE    0x00001000
#define EMAC_TDES0_LCA    0x00000800
#define EMAC_TDES0_NC     0x00000400
#define EMAC_TDES0_LCO    0x00000200
#define EMAC_TDES0_EC     0x00000100
#define EMAC_TDES0_VF     0x00000080
#define EMAC_TDES0_CC     0x00000078
#define EMAC_TDES0_ED     0x00000004
#define EMAC_TDES0_UF     0x00000002
#define EMAC_TDES0_DB     0x00000001
#define EMAC_TDES1_SAIC   0xE0000000
#define EMAC_TDES1_TBS2   0x1FFF0000
#define EMAC_TDES1_TBS1   0x00001FFF
#define EMAC_TDES2_TBAP1  0xFFFFFFFF
#define EMAC_TDES3_TBAP2  0xFFFFFFFF
#define EMAC_TDES6_TTSL   0xFFFFFFFF
#define EMAC_TDES7_TTSH   0xFFFFFFFF

//Receive DMA descriptor flags
#define EMAC_RDES0_OWN    0x80000000
#define EMAC_RDES0_AFM    0x40000000
#define EMAC_RDES0_FL     0x3FFF0000
#define EMAC_RDES0_ES     0x00008000
#define EMAC_RDES0_DE     0x00004000
#define EMAC_RDES0_SAF    0x00002000
#define EMAC_RDES0_LE     0x00001000
#define EMAC_RDES0_OE     0x00000800
#define EMAC_RDES0_VLAN   0x00000400
#define EMAC_RDES0_FS     0x00000200
#define EMAC_RDES0_LS     0x00000100
#define EMAC_RDES0_TSA_GF 0x00000080
#define EMAC_RDES0_LCO    0x00000040
#define EMAC_RDES0_FT     0x00000020
#define EMAC_RDES0_RWT    0x00000010
#define EMAC_RDES0_RE     0x00000008
#define EMAC_RDES0_DBE    0x00000004
#define EMAC_RDES0_CE     0x00000002
#define EMAC_RDES0_ESA    0x00000001
#define EMAC_RDES1_DIC    0x80000000
#define EMAC_RDES1_RBS2   0x1FFF0000
#define EMAC_RDES1_RER    0x00008000
#define EMAC_RDES1_RCH    0x00004000
#define EMAC_RDES1_RBS1   0x00001FFF
#define EMAC_RDES2_RBAP1  0xFFFFFFFF
#define EMAC_RDES3_RBAP2  0xFFFFFFFF
#define EMAC_RDES4_TSD    0x00004000
#define EMAC_RDES4_PV     0x00002000
#define EMAC_RDES4_PFT    0x00001000
#define EMAC_RDES4_PMT    0x00000F00
#define EMAC_RDES4_IPV6PR 0x00000080
#define EMAC_RDES4_IPV4PR 0x00000040
#define EMAC_RDES4_IPCB   0x00000020
#define EMAC_RDES4_IPPE   0x00000010
#define EMAC_RDES4_IPHE   0x00000008
#define EMAC_RDES4_IPPT   0x00000007
#define EMAC_RDES6_RTSL   0xFFFFFFFF
#define EMAC_RDES7_RTSH   0xFFFFFFFF

#ifndef ti_sysbios_BIOS___VERS
   #define msp432e4EthIrqHandler EMAC0_IRQHandler
#endif

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
} Msp432e4TxDmaDesc;


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
} Msp432e4RxDmaDesc;


//MSP432E4 Ethernet MAC driver
extern const NicDriver msp432e4EthDriver;

//MSP432E4 Ethernet MAC related functions
error_t msp432e4EthInit(NetInterface *interface);
void msp432e4EthInitGpio(NetInterface *interface);
void msp432e4EthInitDmaDesc(NetInterface *interface);

void msp432e4EthTick(NetInterface *interface);

void msp432e4EthEnableIrq(NetInterface *interface);
void msp432e4EthDisableIrq(NetInterface *interface);
void msp432e4EthIrqHandler(void);
void msp432e4EthEventHandler(NetInterface *interface);

error_t msp432e4EthSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t msp432e4EthReceivePacket(NetInterface *interface);

error_t msp432e4EthUpdateMacAddrFilter(NetInterface *interface);
error_t msp432e4EthUpdateMacConfig(NetInterface *interface);

void msp432e4EthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data);

uint16_t msp432e4EthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr);

void msp432e4EthDumpPhyReg(void);

uint32_t msp432e4EthCalcCrc(const void *data, size_t length);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
