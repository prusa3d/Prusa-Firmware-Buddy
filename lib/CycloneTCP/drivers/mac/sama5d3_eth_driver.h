/**
 * @file sama5d3_eth_driver.h
 * @brief SAMA5D3 Ethernet MAC driver (EMAC instance)
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

#ifndef _SAMA5D3_ETH_DRIVER_H
#define _SAMA5D3_ETH_DRIVER_H

//Number of TX buffers
#ifndef SAMA5D3_ETH_TX_BUFFER_COUNT
   #define SAMA5D3_ETH_TX_BUFFER_COUNT 4
#elif (SAMA5D3_ETH_TX_BUFFER_COUNT < 1)
   #error SAMA5D3_ETH_TX_BUFFER_COUNT parameter is not valid
#endif

//TX buffer size
#ifndef SAMA5D3_ETH_TX_BUFFER_SIZE
   #define SAMA5D3_ETH_TX_BUFFER_SIZE 1536
#elif (SAMA5D3_ETH_TX_BUFFER_SIZE != 1536)
   #error SAMA5D3_ETH_TX_BUFFER_SIZE parameter is not valid
#endif

//Number of RX buffers
#ifndef SAMA5D3_ETH_RX_BUFFER_COUNT
   #define SAMA5D3_ETH_RX_BUFFER_COUNT 96
#elif (SAMA5D3_ETH_RX_BUFFER_COUNT < 12)
   #error SAMA5D3_ETH_RX_BUFFER_COUNT parameter is not valid
#endif

//RX buffer size
#ifndef SAMA5D3_ETH_RX_BUFFER_SIZE
   #define SAMA5D3_ETH_RX_BUFFER_SIZE 128
#elif (SAMA5D3_ETH_RX_BUFFER_SIZE != 128)
   #error SAMA5D3_ETH_RX_BUFFER_SIZE parameter is not valid
#endif

//Ethernet interrupt priority
#ifndef SAMA5D3_ETH_IRQ_PRIORITY
   #define SAMA5D3_ETH_IRQ_PRIORITY 0
#elif (SAMA5D3_ETH_IRQ_PRIORITY < 0)
   #error SAMA5D3_ETH_IRQ_PRIORITY parameter is not valid
#endif

//Name of the section where to place DMA buffers
#ifndef SAMA5D3_ETH_RAM_SECTION
   #define SAMA5D3_ETH_RAM_SECTION ".ram_no_cache"
#endif

//RMII signals
#define EMAC_RMII_MASK (PIO_PC9A_EMDIO | PIO_PC8A_EMDC | \
   PIO_PC7A_EREFCK | PIO_PC6A_ERXER | PIO_PC5A_ECRSDV | PIO_PC4A_ETXEN | \
   PIO_PC3A_ERX1 | PIO_PC2A_ERX0 | PIO_PC1A_ETX1 | PIO_PC0A_ETX0)

//TX buffer descriptor flags
#define EMAC_TX_USED           0x80000000
#define EMAC_TX_WRAP           0x40000000
#define EMAC_TX_ERROR          0x20000000
#define EMAC_TX_UNDERRUN       0x10000000
#define EMAC_TX_EXHAUSTED      0x08000000
#define EMAC_TX_NO_CRC         0x00010000
#define EMAC_TX_LAST           0x00008000
#define EMAC_TX_LENGTH         0x000007FF

//RX buffer descriptor flags
#define EMAC_RX_ADDRESS        0xFFFFFFFC
#define EMAC_RX_WRAP           0x00000002
#define EMAC_RX_OWNERSHIP      0x00000001
#define EMAC_RX_BROADCAST      0x80000000
#define EMAC_RX_MULTICAST_HASH 0x40000000
#define EMAC_RX_UNICAST_HASH   0x20000000
#define EMAC_RX_EXT_ADDR       0x10000000
#define EMAC_RX_SAR1           0x04000000
#define EMAC_RX_SAR2           0x02000000
#define EMAC_RX_SAR3           0x01000000
#define EMAC_RX_SAR4           0x00800000
#define EMAC_RX_TYPE_ID        0x00400000
#define EMAC_RX_VLAN_TAG       0x00200000
#define EMAC_RX_PRIORITY_TAG   0x00100000
#define EMAC_RX_VLAN_PRIORITY  0x000E0000
#define EMAC_RX_CFI            0x00010000
#define EMAC_RX_EOF            0x00008000
#define EMAC_RX_SOF            0x00004000
#define EMAC_RX_OFFSET         0x00003000
#define EMAC_RX_LENGTH         0x00000FFF

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
} Sama5d3EthTxBufferDesc;


/**
 * @brief Receive buffer descriptor
 **/

typedef struct
{
   uint32_t address;
   uint32_t status;
} Sama5d3EthRxBufferDesc;


//SAMA5D3 Ethernet MAC driver (EMAC instance)
extern const NicDriver sama5d3EthDriver;

//SAMA5D3 Ethernet MAC related functions
error_t sama5d3EthInit(NetInterface *interface);
void sama5d3EthInitGpio(NetInterface *interface);
void sama5d3EthInitBufferDesc(NetInterface *interface);

void sama5d3EthTick(NetInterface *interface);

void sama5d3EthEnableIrq(NetInterface *interface);
void sama5d3EthDisableIrq(NetInterface *interface);
void sama5d3EthIrqHandler(void);
void sama5d3EthEventHandler(NetInterface *interface);

error_t sama5d3EthSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t sama5d3EthReceivePacket(NetInterface *interface);

error_t sama5d3EthUpdateMacAddrFilter(NetInterface *interface);
error_t sama5d3EthUpdateMacConfig(NetInterface *interface);

void sama5d3EthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data);

uint16_t sama5d3EthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
