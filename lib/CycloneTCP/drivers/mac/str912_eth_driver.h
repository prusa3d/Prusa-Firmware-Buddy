/**
 * @file str912_eth_driver.h
 * @brief STR9 Ethernet MAC driver
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

#ifndef _STR912_ETH_DRIVER_H
#define _STR912_ETH_DRIVER_H

//Dependencies
#include "core/nic.h"

//Number of TX buffers
#ifndef STR912_ETH_TX_BUFFER_COUNT
   #define STR912_ETH_TX_BUFFER_COUNT 2
#elif (STR912_ETH_TX_BUFFER_COUNT < 1)
   #error STR912_ETH_TX_BUFFER_COUNT parameter is not valid
#endif

//TX buffer size
#ifndef STR912_ETH_TX_BUFFER_SIZE
   #define STR912_ETH_TX_BUFFER_SIZE 1536
#elif (STR912_ETH_TX_BUFFER_SIZE != 1536)
   #error STR912_ETH_TX_BUFFER_SIZE parameter is not valid
#endif

//Number of RX buffers
#ifndef STR912_ETH_RX_BUFFER_COUNT
   #define STR912_ETH_RX_BUFFER_COUNT 4
#elif (STR912_ETH_RX_BUFFER_COUNT < 1)
   #error STR912_ETH_RX_BUFFER_COUNT parameter is not valid
#endif

//RX buffer size
#ifndef STR912_ETH_RX_BUFFER_SIZE
   #define STR912_ETH_RX_BUFFER_SIZE 1536
#elif (STR912_ETH_RX_BUFFER_SIZE != 1536)
   #error STR912_ETH_RX_BUFFER_SIZE parameter is not valid
#endif

//Ethernet interrupt priority
#ifndef STR912_ETH_IRQ_PRIORITY
   #define STR912_ETH_IRQ_PRIORITY 15
#elif (STR912_ETH_IRQ_PRIORITY < 0)
   #error STR912_ETH_IRQ_PRIORITY parameter is not valid
#endif

//ENET_SCR register
#define ENET_SCR_TX_FIFO_SIZE          0xF0000000
#define ENET_SCR_TX_IO_DATA_WIDTH      0x0C000000
#define ENET_SCR_TX_CHAN_STATUS        0x03000000
#define ENET_SCR_RX_FIFO_SIZE          0x00F00000
#define ENET_SCR_RX_IO_DATA_WIDTH      0x000C0000
#define ENET_SCR_RX_CHAN_STATUS        0x00030000
#define ENET_SCR_TX_MAX_BURST_SIZE     0x000000C0
#define ENET_SCR_RX_MAX_BURST_SIZE     0x00000030
#define ENET_SCR_LOOPB                 0x00000002
#define ENET_SCR_SRESET                0x00000001

//ENET_IER register
#define ENET_IER_TX_CURR_DONE_EN       0x80000000
#define ENET_IER_MAC_802_3_INT_EN      0x10000000
#define ENET_IER_TX_MERR_INT_EN        0x02000000
#define ENET_IER_TX_DONE_EN            0x00800000
#define ENET_IER_TX_NEXT_EN            0x00400000
#define ENET_IER_TX_TO_EN              0x00080000
#define ENET_IER_TX_ENTRY_EN           0x00040000
#define ENET_IER_TX_FULL_EN            0x00020000
#define ENET_IER_TX_EMPTY_EN           0x00010000
#define ENET_IER_RX_CURR_DONE_EN       0x00008000
#define ENET_IER_RX_MERR_INT_EN        0x00000200
#define ENET_IER_RX_DONE_EN            0x00000080
#define ENET_IER_RX_NEXT_EN            0x00000040
#define ENET_IER_PACKET_LOST_EN        0x00000020
#define ENET_IER_RX_TO_EN              0x00000008
#define ENET_IER_RX_ENTRY_EN           0x00000004
#define ENET_IER_RX_FULL_EN            0x00000002
#define ENET_IER_RX_EMPTY_EN           0x00000001

//ENET_ISR register
#define ENET_ISR_TX_CURR_DONE          0x80000000
#define ENET_ISR_MAC_802_3_INT         0x10000000
#define ENET_ISR_TX_MERR_INT           0x02000000
#define ENET_ISR_TX_DONE               0x00800000
#define ENET_ISR_TX_NEXT               0x00400000
#define ENET_ISR_TX_TO                 0x00080000
#define ENET_ISR_TX_ENTRY              0x00040000
#define ENET_ISR_TX_FULL               0x00020000
#define ENET_ISR_TX_EMPTY              0x00010000
#define ENET_ISR_RX_CURR_DONE          0x00008000
#define ENET_ISR_RX_MERR_INT           0x00000200
#define ENET_ISR_RX_DONE               0x00000080
#define ENET_ISR_RX_NEXT               0x00000040
#define ENET_ISR_PACKET_LOST           0x00000020
#define ENET_ISR_RX_TO                 0x00000008
#define ENET_ISR_RX_ENTRY              0x00000004
#define ENET_ISR_RX_FULL               0x00000002
#define ENET_ISR_RX_EMPTY              0x00000001

//ENET_CCR register
#define ENET_CCR_SEL_CLK               0x0000000C

#define ENET_CCR_SEL_CLK_0             0x00000000
#define ENET_CCR_SEL_CLK_1             0x00000004

//ENET_RXSTR register
#define ENET_RXSTR_DFETCH_DLY          0x00FFFF00
#define ENET_RXSTR_COLL_SEEN           0x00000080
#define ENET_RXSTR_RUNT_FRAME          0x00000040
#define ENET_RXSTR_FILTER_FAIL         0x00000020
#define ENET_RXSTR_START_FETCH         0x00000004
#define ENET_RXSTR_DMA_EN              0x00000001

#define ENET_RXSTR_DFETCH_DLY_DEFAULT  0x00800000

//ENET_TXSTR register
#define ENET_TXSTR_DFETCH_DLY          0x00FFFF00
#define ENET_TXSTR_UNDER_RUN           0x00000020
#define ENET_TXSTR_START_FETCH         0x00000004
#define ENET_TXSTR_DMA_EN              0x00000001

#define ENET_TXSTR_DFETCH_DLY_DEFAULT  0x00800000

//ENET_MCR register
#define ENET_MCR_RA                    0x80000000
#define ENET_MCR_EN                    0x40000000
#define ENET_MCR_PS                    0x03000000
#define ENET_MCR_DRO                   0x00800000
#define ENET_MCR_LM                    0x00600000
#define ENET_MCR_FDM                   0x00100000
#define ENET_MCR_AFM                   0x000E0000
#define ENET_MCR_PWF                   0x00010000
#define ENET_MCR_VFM                   0x00008000
#define ENET_MCR_ELC                   0x00001000
#define ENET_MCR_DBF                   0x00000800
#define ENET_MCR_DPR                   0x00000400
#define ENET_MCR_RVFF                  0x00000200
#define ENET_MCR_APR                   0x00000100
#define ENET_MCR_BL                    0x000000C0
#define ENET_MCR_DCE                   0x00000020
#define ENET_MCR_RVBE                  0x00000010
#define ENET_MCR_TE                    0x00000008
#define ENET_MCR_RE                    0x00000004
#define ENET_MCR_RCFA                  0x00000001

#define ENET_MCR_PS_0                  0x00000000
#define ENET_MCR_PS_1                  0x01000000

#define ENET_MCR_AFM_0                 0x00000000
#define ENET_MCR_AFM_1                 0x00020000
#define ENET_MCR_AFM_2                 0x00040000
#define ENET_MCR_AFM_3                 0x00060000
#define ENET_MCR_AFM_4                 0x00080000
#define ENET_MCR_AFM_5                 0x000A0000
#define ENET_MCR_AFM_6                 0x000C0000
#define ENET_MCR_AFM_7                 0x000E0000

#define ENET_MCR_BL_0                  0x00000000
#define ENET_MCR_BL_1                  0x00000040
#define ENET_MCR_BL_2                  0x00000080
#define ENET_MCR_BL_3                  0x000000C0

//ENET_MIIA register
#define ENET_MIIA_PADDR                0x0000F800
#define ENET_MIIA_RADDR                0x000007C0
#define ENET_MIIA_PR                   0x00000004
#define ENET_MIIA_WR                   0x00000002
#define ENET_MIIA_BUSY                 0x00000001

//ENET_MIID register
#define ENET_MIID_RDATA                0x0000FFFF

//TX DMA descriptor (control word)
#define ENET_TDES_CTRL_DLY_EN          0x00008000
#define ENET_TDES_CTRL_NXT_EN          0x00004000
#define ENET_TDES_CTRL_CONT_EN         0x00001000
#define ENET_TDES_CTRL_FL              0x00000FFF

//TX DMA descriptor (start address)
#define ENET_TDES_START_ADDR           0xFFFFFFFC
#define ENET_TDES_START_FIX_ADDR       0x00000002
#define ENET_TDES_START_WRAP_EN        0x00000001

//TX DMA descriptor (next descriptor address)
#define ENET_TDES_NEXT_ADDR            0xFFFFFFFC
#define ENET_TDES_NEXT_NPOL_EN         0x00000001

//TX DMA descriptor (status word)
#define ENET_TDES_STATUS_PR            0x80000000
#define ENET_TDES_STATUS_BC            0x7FFC0000
#define ENET_TDES_STATUS_VALID         0x00010000
#define ENET_TDES_STATUS_CC            0x00003C00
#define ENET_TDES_STATUS_LCO           0x00000200
#define ENET_TDES_STATUS_DEF           0x00000100
#define ENET_TDES_STATUS_UR            0x00000080
#define ENET_TDES_STATUS_EC            0x00000040
#define ENET_TDES_STATUS_LC            0x00000020
#define ENET_TDES_STATUS_ED            0x00000010
#define ENET_TDES_STATUS_LOC           0x00000008
#define ENET_TDES_STATUS_NC            0x00000004
#define ENET_TDES_STATUS_FA            0x00000001

//RX DMA descriptor (control word)
#define ENET_RDES_CTRL_DLY_EN          0x00008000
#define ENET_RDES_CTRL_NXT_EN          0x00004000
#define ENET_RDES_CTRL_CONT_EN         0x00001000
#define ENET_RDES_CTRL_FL              0x00000FFF

//RX DMA descriptor (start address)
#define ENET_RDES_START_ADDR           0xFFFFFFFC
#define ENET_RDES_START_FIX_ADDR       0x00000002
#define ENET_RDES_START_WRAP_EN        0x00000001

//RX DMA descriptor (next descriptor address)
#define ENET_RDES_NEXT_ADDR            0xFFFFFFFC
#define ENET_RDES_NEXT_NPOL_EN         0x00000001

//RX DMA descriptor (status word)
#define ENET_RDES_STATUS_FA            0x80000000
#define ENET_RDES_STATUS_PF            0x40000000
#define ENET_RDES_STATUS_FF            0x20000000
#define ENET_RDES_STATUS_BF            0x10000000
#define ENET_RDES_STATUS_MCF           0x08000000
#define ENET_RDES_STATUS_UCF           0x04000000
#define ENET_RDES_STATUS_CF            0x02000000
#define ENET_RDES_STATUS_LE            0x01000000
#define ENET_RDES_STATUS_VL2           0x00800000
#define ENET_RDES_STATUS_VL1           0x00400000
#define ENET_RDES_STATUS_CE            0x00200000
#define ENET_RDES_STATUS_EB            0x00100000
#define ENET_RDES_STATUS_ME            0x00080000
#define ENET_RDES_STATUS_FT            0x00040000
#define ENET_RDES_STATUS_LC            0x00020000
#define ENET_RDES_STATUS_VALID         0x00010000
#define ENET_RDES_STATUS_RF            0x00008000
#define ENET_RDES_STATUS_WT            0x00004000
#define ENET_RDES_STATUS_FCI           0x00002000
#define ENET_RDES_STATUS_OL            0x00001000
#define ENET_RDES_STATUS_FL            0x000007FF

//Error mask
#define ENET_RDES_STATUS_ERROR (ENET_RDES_STATUS_FA | \
   ENET_RDES_STATUS_LE | ENET_RDES_STATUS_CE | \
   ENET_RDES_STATUS_EB | ENET_RDES_STATUS_ME | \
   ENET_RDES_STATUS_LC | ENET_RDES_STATUS_RF | \
   ENET_RDES_STATUS_WT | ENET_RDES_STATUS_OL)

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Transmit DMA descriptor
 **/

typedef struct
{
   uint32_t ctrl;
   uint32_t start;
   uint32_t next;
   uint32_t status;
} Str912TxDmaDesc;


/**
 * @brief Receive DMA descriptor
 **/

typedef struct
{
   uint32_t ctrl;
   uint32_t start;
   uint32_t next;
   uint32_t status;
} Str912RxDmaDesc;


//STR912 Ethernet MAC driver
extern const NicDriver str912EthDriver;

//STR912 Ethernet MAC related functions
error_t str912EthInit(NetInterface *interface);
void str912EthInitGpio(NetInterface *interface);
void str912EthInitDmaDesc(NetInterface *interface);

void str912EthTick(NetInterface *interface);

void str912EthEnableIrq(NetInterface *interface);
void str912EthDisableIrq(NetInterface *interface);
void str912EthEventHandler(NetInterface *interface);

error_t str912EthSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t str912EthReceivePacket(NetInterface *interface);

error_t str912EthUpdateMacAddrFilter(NetInterface *interface);
error_t str912EthUpdateMacConfig(NetInterface *interface);

void str912EthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data);

uint16_t str912EthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr);

uint32_t str912EthCalcCrc(const void *data, size_t length);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
