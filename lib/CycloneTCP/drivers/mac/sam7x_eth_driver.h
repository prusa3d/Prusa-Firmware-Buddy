/**
 * @file sam7x_eth_driver.h
 * @brief AT91SAM7X Ethernet MAC driver
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

#ifndef _SAM7X_ETH_DRIVER_H
#define _SAM7X_ETH_DRIVER_H

//Number of TX buffers
#ifndef SAM7X_ETH_TX_BUFFER_COUNT
   #define SAM7X_ETH_TX_BUFFER_COUNT 2
#elif (SAM7X_ETH_TX_BUFFER_COUNT < 1)
   #error SAM7X_ETH_TX_BUFFER_COUNT parameter is not valid
#endif

//TX buffer size
#ifndef SAM7X_ETH_TX_BUFFER_SIZE
   #define SAM7X_ETH_TX_BUFFER_SIZE 1536
#elif (SAM7X_ETH_TX_BUFFER_SIZE != 1536)
   #error SAM7X_ETH_TX_BUFFER_SIZE parameter is not valid
#endif

//Number of RX buffers
#ifndef SAM7X_ETH_RX_BUFFER_COUNT
   #define SAM7X_ETH_RX_BUFFER_COUNT 48
#elif (SAM7X_ETH_RX_BUFFER_COUNT < 12)
   #error SAM7X_ETH_RX_BUFFER_COUNT parameter is not valid
#endif

//RX buffer size
#ifndef SAM7X_ETH_RX_BUFFER_SIZE
   #define SAM7X_ETH_RX_BUFFER_SIZE 128
#elif (SAM7X_ETH_RX_BUFFER_SIZE != 128)
   #error SAM7X_ETH_RX_BUFFER_SIZE parameter is not valid
#endif

//MII signals
#define AT91C_EMAC_MII_MASK (AT91C_PB17_ERXCK | AT91C_PB16_ECOL | \
   AT91C_PB15_ERXDV_ECRSDV | AT91C_PB14_ERX3 | AT91C_PB13_ERX2 | AT91C_PB12_ETXER | \
   AT91C_PB11_ETX3 | AT91C_PB10_ETX2 | AT91C_PB9_EMDIO | AT91C_PB8_EMDC | \
   AT91C_PB7_ERXER | AT91C_PB6_ERX1 | AT91C_PB5_ERX0 | AT91C_PB4_ECRS | \
   AT91C_PB3_ETX1 | AT91C_PB2_ETX0 | AT91C_PB1_ETXEN | AT91C_PB0_ETXCK_EREFCK)

//PHY maintenance register (EMAC_MAN)
#define AT91C_EMAC_SOF_01    (1 << 30)
#define AT91C_EMAC_RW_01     (1 << 28)
#define AT91C_EMAC_RW_10     (2 << 28)
#define AT91C_EMAC_CODE_10   (2 << 16)

//TX buffer descriptor flags
#define AT91C_EMAC_TX_USED           0x80000000
#define AT91C_EMAC_TX_WRAP           0x40000000
#define AT91C_EMAC_TX_ERROR          0x20000000
#define AT91C_EMAC_TX_UNDERRUN       0x10000000
#define AT91C_EMAC_TX_EXHAUSTED      0x08000000
#define AT91C_EMAC_TX_NO_CRC         0x00010000
#define AT91C_EMAC_TX_LAST           0x00008000
#define AT91C_EMAC_TX_LENGTH         0x000007FF

//RX buffer descriptor flags
#define AT91C_EMAC_RX_ADDRESS        0xFFFFFFFC
#define AT91C_EMAC_RX_WRAP           0x00000002
#define AT91C_EMAC_RX_OWNERSHIP      0x00000001
#define AT91C_EMAC_RX_BROADCAST      0x80000000
#define AT91C_EMAC_RX_MULTICAST_HASH 0x40000000
#define AT91C_EMAC_RX_UNICAST_HASH   0x20000000
#define AT91C_EMAC_RX_EXT_ADDR       0x10000000
#define AT91C_EMAC_RX_SAR1           0x04000000
#define AT91C_EMAC_RX_SAR2           0x02000000
#define AT91C_EMAC_RX_SAR3           0x01000000
#define AT91C_EMAC_RX_SAR4           0x00800000
#define AT91C_EMAC_RX_TYPE_ID        0x00400000
#define AT91C_EMAC_RX_VLAN_TAG       0x00200000
#define AT91C_EMAC_RX_PRIORITY_TAG   0x00100000
#define AT91C_EMAC_RX_VLAN_PRIORITY  0x000E0000
#define AT91C_EMAC_RX_CFI            0x00010000
#define AT91C_EMAC_RX_EOF            0x00008000
#define AT91C_EMAC_RX_SOF            0x00004000
#define AT91C_EMAC_RX_OFFSET         0x00003000
#define AT91C_EMAC_RX_LENGTH         0x00000FFF

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
} Sam7xTxBufferDesc;


/**
 * @brief Receive buffer descriptor
 **/

typedef struct
{
   uint32_t address;
   uint32_t status;
} Sam7xRxBufferDesc;


//SAM7X Ethernet MAC driver
extern const NicDriver sam7xEthDriver;

//SAM7X Ethernet MAC related functions
error_t sam7xEthInit(NetInterface *interface);
void sam7xEthInitGpio(NetInterface *interface);
void sam7xEthInitBufferDesc(NetInterface *interface);

void sam7xEthTick(NetInterface *interface);

void sam7xEthEnableIrq(NetInterface *interface);
void sam7xEthDisableIrq(NetInterface *interface);
void sam7xEthIrqHandler(void);
void sam7xEthEventHandler(NetInterface *interface);

error_t sam7xEthSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t sam7xEthReceivePacket(NetInterface *interface);

error_t sam7xEthUpdateMacAddrFilter(NetInterface *interface);
error_t sam7xEthUpdateMacConfig(NetInterface *interface);

void sam7xEthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data);

uint16_t sam7xEthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr);

//Wrapper for the interrupt service routine
void emacIrqWrapper(void);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
