/**
 * @file mpfsxxx_eth2_driver.h
 * @brief PolarFire SoC Gigabit Ethernet MAC driver (MAC1 instance)
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

#ifndef _MPFSXXX_ETH2_DRIVER_H
#define _MPFSXXX_ETH2_DRIVER_H

//Number of TX buffers
#ifndef MPFSXXX_ETH2_TX_BUFFER_COUNT
   #define MPFSXXX_ETH2_TX_BUFFER_COUNT 16
#elif (MPFSXXX_ETH2_TX_BUFFER_COUNT < 1)
   #error MPFSXXX_ETH2_TX_BUFFER_COUNT parameter is not valid
#endif

//TX buffer size
#ifndef MPFSXXX_ETH2_TX_BUFFER_SIZE
   #define MPFSXXX_ETH2_TX_BUFFER_SIZE 1536
#elif (MPFSXXX_ETH2_TX_BUFFER_SIZE != 1536)
   #error MPFSXXX_ETH2_TX_BUFFER_SIZE parameter is not valid
#endif

//Number of RX buffers
#ifndef MPFSXXX_ETH2_RX_BUFFER_COUNT
   #define MPFSXXX_ETH2_RX_BUFFER_COUNT 16
#elif (MPFSXXX_ETH2_RX_BUFFER_COUNT < 12)
   #error MPFSXXX_ETH2_RX_BUFFER_COUNT parameter is not valid
#endif

//RX buffer size
#ifndef MPFSXXX_ETH2_RX_BUFFER_SIZE
   #define MPFSXXX_ETH2_RX_BUFFER_SIZE 1536
#elif (MPFSXXX_ETH2_RX_BUFFER_SIZE != 1536)
   #error MPFSXXX_ETH2_RX_BUFFER_SIZE parameter is not valid
#endif

//Number of dummy buffers
#ifndef MPFSXXX_ETH2_DUMMY_BUFFER_COUNT
   #define MPFSXXX_ETH2_DUMMY_BUFFER_COUNT 2
#elif (MPFSXXX_ETH2_DUMMY_BUFFER_COUNT < 1)
   #error MPFSXXX_ETH2_DUMMY_BUFFER_COUNT parameter is not valid
#endif

//Dummy buffer size
#ifndef MPFSXXX_ETH2_DUMMY_BUFFER_SIZE
   #define MPFSXXX_ETH2_DUMMY_BUFFER_SIZE 128
#elif (MPFSXXX_ETH2_DUMMY_BUFFER_SIZE != 128)
   #error MPFSXXX_ETH2_DUMMY_BUFFER_SIZE parameter is not valid
#endif

//Ethernet interrupt priority
#ifndef MPFSXXX_ETH2_IRQ_PRIORITY
   #define MPFSXXX_ETH2_IRQ_PRIORITY 7
#elif (MPFSXXX_ETH2_IRQ_PRIORITY < 0)
   #error MPFSXXX_ETH2_IRQ_PRIORITY parameter is not valid
#endif

//Name of the section where to place DMA buffers
#ifndef MPFSXXX_ETH2_RAM_SECTION
   #define MPFSXXX_ETH2_RAM_SECTION ".ram_no_cache"
#endif

//MAC controller base address
#define MAC1 ((MAC_TypeDef *) 0x20112000)

//TX buffer descriptor flags
#define MAC_TX_USED           0x80000000
#define MAC_TX_WRAP           0x40000000
#define MAC_TX_RLE_ERROR      0x20000000
#define MAC_TX_UNDERRUN_ERROR 0x10000000
#define MAC_TX_AHB_ERROR      0x08000000
#define MAC_TX_LATE_COL_ERROR 0x04000000
#define MAC_TX_CHECKSUM_ERROR 0x00700000
#define MAC_TX_NO_CRC         0x00010000
#define MAC_TX_LAST           0x00008000
#define MAC_TX_LENGTH         0x00003FFF

//RX buffer descriptor flags
#define MAC_RX_ADDRESS        0xFFFFFFFC
#define MAC_RX_WRAP           0x00000002
#define MAC_RX_OWNERSHIP      0x00000001
#define MAC_RX_BROADCAST      0x80000000
#define MAC_RX_MULTICAST_HASH 0x40000000
#define MAC_RX_UNICAST_HASH   0x20000000
#define MAC_RX_SAR            0x08000000
#define MAC_RX_SAR_MASK       0x06000000
#define MAC_RX_TYPE_ID        0x01000000
#define MAC_RX_SNAP           0x01000000
#define MAC_RX_TYPE_ID_MASK   0x00C00000
#define MAC_RX_CHECKSUM_VALID 0x00C00000
#define MAC_RX_VLAN_TAG       0x00200000
#define MAC_RX_PRIORITY_TAG   0x00100000
#define MAC_RX_VLAN_PRIORITY  0x000E0000
#define MAC_RX_CFI            0x00010000
#define MAC_RX_EOF            0x00008000
#define MAC_RX_SOF            0x00004000
#define MAC_RX_LENGTH_MSB     0x00002000
#define MAC_RX_BAD_FCS        0x00002000
#define MAC_RX_LENGTH         0x00001FFF

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Transmit buffer descriptor
 **/

typedef struct
{
   uint32_t addrLow;
   uint32_t status;
   uint32_t addrHigh;
   uint32_t reserved;
   uint32_t nanoSeconds;
   uint32_t seconds;
} MpfsxxxEth2TxBufferDesc;


/**
 * @brief Receive buffer descriptor
 **/

typedef struct
{
   uint32_t addrLow;
   uint32_t status;
   uint32_t addrHigh;
   uint32_t reserved;
   uint32_t nanoSeconds;
   uint32_t seconds;
} MpfsxxxEth2RxBufferDesc;


//MPFSxxx Ethernet MAC driver (MAC1 instance)
extern const NicDriver mpfsxxxEth2Driver;

//MPFSxxx Ethernet MAC related functions
error_t mpfsxxxEth2Init(NetInterface *interface);
void mpfsxxxEth2InitGpio(NetInterface *interface);
void mpfsxxxEth2InitBufferDesc(NetInterface *interface);

void mpfsxxxEth2Tick(NetInterface *interface);

void mpfsxxxEth2EnableIrq(NetInterface *interface);
void mpfsxxxEth2DisableIrq(NetInterface *interface);
void mpfsxxxEth2EventHandler(NetInterface *interface);

error_t mpfsxxxEth2SendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t mpfsxxxEth2ReceivePacket(NetInterface *interface);

error_t mpfsxxxEth2UpdateMacAddrFilter(NetInterface *interface);
error_t mpfsxxxEth2UpdateMacConfig(NetInterface *interface);

void mpfsxxxEth2WritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data);

uint16_t mpfsxxxEth2ReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
