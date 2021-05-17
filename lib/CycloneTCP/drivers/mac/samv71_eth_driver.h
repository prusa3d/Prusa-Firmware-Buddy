/**
 * @file samv71_eth_driver.h
 * @brief SAMV71 Ethernet MAC driver
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

#ifndef _SAMV71_ETH_DRIVER_H
#define _SAMV71_ETH_DRIVER_H

//Number of TX buffers
#ifndef SAMV71_ETH_TX_BUFFER_COUNT
   #define SAMV71_ETH_TX_BUFFER_COUNT 4
#elif (SAMV71_ETH_TX_BUFFER_COUNT < 1)
   #error SAMV71_ETH_TX_BUFFER_COUNT parameter is not valid
#endif

//TX buffer size
#ifndef SAMV71_ETH_TX_BUFFER_SIZE
   #define SAMV71_ETH_TX_BUFFER_SIZE 1536
#elif (SAMV71_ETH_TX_BUFFER_SIZE != 1536)
   #error SAMV71_ETH_TX_BUFFER_SIZE parameter is not valid
#endif

//Number of RX buffers
#ifndef SAMV71_ETH_RX_BUFFER_COUNT
   #define SAMV71_ETH_RX_BUFFER_COUNT 96
#elif (SAMV71_ETH_RX_BUFFER_COUNT < 12)
   #error SAMV71_ETH_RX_BUFFER_COUNT parameter is not valid
#endif

//RX buffer size
#ifndef SAMV71_ETH_RX_BUFFER_SIZE
   #define SAMV71_ETH_RX_BUFFER_SIZE 128
#elif (SAMV71_ETH_RX_BUFFER_SIZE != 128)
   #error SAMV71_ETH_RX_BUFFER_SIZE parameter is not valid
#endif

//Number of dummy buffers
#ifndef SAMV71_ETH_DUMMY_BUFFER_COUNT
   #define SAMV71_ETH_DUMMY_BUFFER_COUNT 2
#elif (SAMV71_ETH_DUMMY_BUFFER_COUNT < 1)
   #error SAMV71_ETH_DUMMY_BUFFER_COUNT parameter is not valid
#endif

//Dummy buffer size
#ifndef SAMV71_ETH_DUMMY_BUFFER_SIZE
   #define SAMV71_ETH_DUMMY_BUFFER_SIZE 128
#elif (SAMV71_ETH_DUMMY_BUFFER_SIZE != 128)
   #error SAMV71_ETH_DUMMY_BUFFER_SIZE parameter is not valid
#endif

//Interrupt priority grouping
#ifndef SAMV71_ETH_IRQ_PRIORITY_GROUPING
   #define SAMV71_ETH_IRQ_PRIORITY_GROUPING 4
#elif (SAMV71_ETH_IRQ_PRIORITY_GROUPING < 0)
   #error SAMV71_ETH_IRQ_PRIORITY_GROUPING parameter is not valid
#endif

//Ethernet interrupt group priority
#ifndef SAMV71_ETH_IRQ_GROUP_PRIORITY
   #define SAMV71_ETH_IRQ_GROUP_PRIORITY 6
#elif (SAMV71_ETH_IRQ_GROUP_PRIORITY < 0)
   #error SAMV71_ETH_IRQ_GROUP_PRIORITY parameter is not valid
#endif

//Ethernet interrupt subpriority
#ifndef SAMV71_ETH_IRQ_SUB_PRIORITY
   #define SAMV71_ETH_IRQ_SUB_PRIORITY 0
#elif (SAMV71_ETH_IRQ_SUB_PRIORITY < 0)
   #error SAMV71_ETH_IRQ_SUB_PRIORITY parameter is not valid
#endif

//Name of the section where to place DMA buffers
#ifndef SAMV71_ETH_RAM_SECTION
   #define SAMV71_ETH_RAM_SECTION ".ram_no_cache"
#endif

//Backward compatibility
#ifndef PIO_PD9A_GMDIO
   #define PIO_PD9A_GMDIO PIO_PD9A_GMAC_GMDIO
   #define PIO_PD8A_GMDC  PIO_PD8A_GMAC_GMDC
   #define PIO_PD7A_GRXER PIO_PD7A_GMAC_GRXER
   #define PIO_PD6A_GRX1  PIO_PD6A_GMAC_GRX1
   #define PIO_PD5A_GRX0  PIO_PD5A_GMAC_GRX0
   #define PIO_PD4A_GRXDV PIO_PD4A_GMAC_GRXDV
   #define PIO_PD3A_GTX1  PIO_PD3A_GMAC_GTX1
   #define PIO_PD2A_GTX0  PIO_PD2A_GMAC_GTX0
   #define PIO_PD1A_GTXEN PIO_PD1A_GMAC_GTXEN
   #define PIO_PD0A_GTXCK PIO_PD0A_GMAC_GTXCK
   #define GMAC_SA        GmacSa
#endif

//RMII signals
#define GMAC_RMII_MASK (PIO_PD9A_GMDIO | PIO_PD8A_GMDC | \
   PIO_PD7A_GRXER | PIO_PD6A_GRX1 | PIO_PD5A_GRX0 | PIO_PD4A_GRXDV | \
   PIO_PD3A_GTX1 | PIO_PD2A_GTX0 | PIO_PD1A_GTXEN | PIO_PD0A_GTXCK)

//TX buffer descriptor flags
#define GMAC_TX_USED           0x80000000
#define GMAC_TX_WRAP           0x40000000
#define GMAC_TX_RLE_ERROR      0x20000000
#define GMAC_TX_UNDERRUN_ERROR 0x10000000
#define GMAC_TX_AHB_ERROR      0x08000000
#define GMAC_TX_LATE_COL_ERROR 0x04000000
#define GMAC_TX_CHECKSUM_ERROR 0x00700000
#define GMAC_TX_NO_CRC         0x00010000
#define GMAC_TX_LAST           0x00008000
#define GMAC_TX_LENGTH         0x00003FFF

//RX buffer descriptor flags
#define GMAC_RX_ADDRESS        0xFFFFFFFC
#define GMAC_RX_WRAP           0x00000002
#define GMAC_RX_OWNERSHIP      0x00000001
#define GMAC_RX_BROADCAST      0x80000000
#define GMAC_RX_MULTICAST_HASH 0x40000000
#define GMAC_RX_UNICAST_HASH   0x20000000
#define GMAC_RX_SAR            0x08000000
#define GMAC_RX_SAR_MASK       0x06000000
#define GMAC_RX_TYPE_ID        0x01000000
#define GMAC_RX_SNAP           0x01000000
#define GMAC_RX_TYPE_ID_MASK   0x00C00000
#define GMAC_RX_CHECKSUM_VALID 0x00C00000
#define GMAC_RX_VLAN_TAG       0x00200000
#define GMAC_RX_PRIORITY_TAG   0x00100000
#define GMAC_RX_VLAN_PRIORITY  0x000E0000
#define GMAC_RX_CFI            0x00010000
#define GMAC_RX_EOF            0x00008000
#define GMAC_RX_SOF            0x00004000
#define GMAC_RX_LENGTH_MSB     0x00002000
#define GMAC_RX_BAD_FCS        0x00002000
#define GMAC_RX_LENGTH         0x00001FFF

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
} Samv71TxBufferDesc;


/**
 * @brief Receive buffer descriptor
 **/

typedef struct
{
   uint32_t address;
   uint32_t status;
} Samv71RxBufferDesc;


//SAMV71 Ethernet MAC driver
extern const NicDriver samv71EthDriver;

//SAMV71 Ethernet MAC related functions
error_t samv71EthInit(NetInterface *interface);
void samv71EthInitGpio(NetInterface *interface);
void samv71EthInitBufferDesc(NetInterface *interface);

void samv71EthTick(NetInterface *interface);

void samv71EthEnableIrq(NetInterface *interface);
void samv71EthDisableIrq(NetInterface *interface);
void samv71EthEventHandler(NetInterface *interface);

error_t samv71EthSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t samv71EthReceivePacket(NetInterface *interface);

error_t samv71EthUpdateMacAddrFilter(NetInterface *interface);
error_t samv71EthUpdateMacConfig(NetInterface *interface);

void samv71EthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data);

uint16_t samv71EthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
