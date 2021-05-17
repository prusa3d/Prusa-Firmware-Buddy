/**
 * @file avr32_eth_driver.h
 * @brief AVR32 Ethernet MAC driver
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

#ifndef _AVR32_ETH_DRIVER_H
#define _AVR32_ETH_DRIVER_H

//Number of TX buffers
#ifndef AVR32_ETH_TX_BUFFER_COUNT
   #define AVR32_ETH_TX_BUFFER_COUNT 2
#elif (AVR32_ETH_TX_BUFFER_COUNT < 1)
   #error AVR32_ETH_TX_BUFFER_COUNT parameter is not valid
#endif

//TX buffer size
#ifndef AVR32_ETH_TX_BUFFER_SIZE
   #define AVR32_ETH_TX_BUFFER_SIZE 1536
#elif (AVR32_ETH_TX_BUFFER_SIZE != 1536)
   #error AVR32_ETH_TX_BUFFER_SIZE parameter is not valid
#endif

//Number of RX buffers
#ifndef AVR32_ETH_RX_BUFFER_COUNT
   #define AVR32_ETH_RX_BUFFER_COUNT 48
#elif (AVR32_ETH_RX_BUFFER_COUNT < 12)
   #error AVR32_ETH_RX_BUFFER_COUNT parameter is not valid
#endif

//RX buffer size
#ifndef AVR32_ETH_RX_BUFFER_SIZE
   #define AVR32_ETH_RX_BUFFER_SIZE 128
#elif (AVR32_ETH_RX_BUFFER_SIZE != 128)
   #error AVR32_ETH_RX_BUFFER_SIZE parameter is not valid
#endif

//Ethernet interrupt priority
#ifndef AVR32_ETH_IRQ_PRIORITY
   #define AVR32_ETH_IRQ_PRIORITY 2
#elif (AVR32_ETH_IRQ_PRIORITY < 0 || AVR32_ETH_IRQ_PRIORITY > 3)
   #error AVR32_ETH_IRQ_PRIORITY parameter is not valid
#endif

//RMII pin definition
#define MACB_RMII_EREFCK_MASK (1 << (AVR32_MACB_TX_CLK_0_PIN - 32))
#define MACB_RMII_ETXEN_MASK  (1 << (AVR32_MACB_TX_EN_0_PIN - 32))
#define MACB_RMII_ETX0_MASK   (1 << (AVR32_MACB_TXD_0_PIN - 32))
#define MACB_RMII_ETX1_MASK   (1 << (AVR32_MACB_TXD_1_PIN - 32))
#define MACB_RMII_ERX0_MASK   (1 << (AVR32_MACB_RXD_0_PIN - 32))
#define MACB_RMII_ERX1_MASK   (1 << (AVR32_MACB_RXD_1_PIN - 32))
#define MACB_RMII_ERXER_MASK  (1 << (AVR32_MACB_RX_ER_0_PIN - 32))
#define MACB_RMII_ECRSDV_MASK (1 << (AVR32_MACB_RX_DV_0_PIN - 32))
#define MACB_RMII_MDC_MASK    (1 << (AVR32_MACB_MDC_0_PIN - 32))
#define MACB_RMII_MDIO_MASK   (1 << (AVR32_MACB_MDIO_0_PIN - 32))

//RMII signals
#define MACB_RMII_MASK (MACB_RMII_EREFCK_MASK | MACB_RMII_ETXEN_MASK | \
   MACB_RMII_ETX0_MASK | MACB_RMII_ETX1_MASK | MACB_RMII_ERX0_MASK | MACB_RMII_ERX1_MASK | \
   MACB_RMII_ERXER_MASK | MACB_RMII_ECRSDV_MASK | MACB_RMII_MDC_MASK | MACB_RMII_MDIO_MASK)

//PHY maintenance register (MAN)
#define MACB_MAN_SOF_01  (1 << AVR32_MACB_MAN_SOF_OFFSET)
#define MACB_MAN_RW_01   (1 << AVR32_MACB_MAN_RW_OFFSET)
#define MACB_MAN_RW_10   (2 << AVR32_MACB_MAN_RW_OFFSET)
#define MACB_MAN_CODE_10 (2 << AVR32_MACB_MAN_CODE_OFFSET)

//TX buffer descriptor flags
#define MACB_TX_USED           0x80000000
#define MACB_TX_WRAP           0x40000000
#define MACB_TX_ERROR          0x20000000
#define MACB_TX_UNDERRUN       0x10000000
#define MACB_TX_EXHAUSTED      0x08000000
#define MACB_TX_NO_CRC         0x00010000
#define MACB_TX_LAST           0x00008000
#define MACB_TX_LENGTH         0x000007FF

//RX buffer descriptor flags
#define MACB_RX_ADDRESS        0xFFFFFFFC
#define MACB_RX_WRAP           0x00000002
#define MACB_RX_OWNERSHIP      0x00000001
#define MACB_RX_BROADCAST      0x80000000
#define MACB_RX_MULTICAST_HASH 0x40000000
#define MACB_RX_UNICAST_HASH   0x20000000
#define MACB_RX_EXT_ADDR       0x10000000
#define MACB_RX_SAR1           0x04000000
#define MACB_RX_SAR2           0x02000000
#define MACB_RX_SAR3           0x01000000
#define MACB_RX_SAR4           0x00800000
#define MACB_RX_TYPE_ID        0x00400000
#define MACB_RX_VLAN_TAG       0x00200000
#define MACB_RX_PRIORITY_TAG   0x00100000
#define MACB_RX_VLAN_PRIORITY  0x000E0000
#define MACB_RX_CFI            0x00010000
#define MACB_RX_EOF            0x00008000
#define MACB_RX_SOF            0x00004000
#define MACB_RX_OFFSET         0x00003000
#define MACB_RX_LENGTH         0x00000FFF

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
} Avr32TxBufferDesc;


/**
 * @brief Receive buffer descriptor
 **/

typedef struct
{
   uint32_t address;
   uint32_t status;
} Avr32RxBufferDesc;


//AVR32 Ethernet MAC driver
extern const NicDriver avr32EthDriver;

//AVR32 Ethernet MAC related functions
error_t avr32EthInit(NetInterface *interface);
void avr32EthInitGpio(NetInterface *interface);
void avr32EthInitBufferDesc(NetInterface *interface);

void avr32EthTick(NetInterface *interface);

void avr32EthEnableIrq(NetInterface *interface);
void avr32EthDisableIrq(NetInterface *interface);
void avr32EthIrqWrapper(void);
bool_t avr32EthIrqHandler(void);
void avr32EthEventHandler(NetInterface *interface);

error_t avr32EthSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t avr32EthReceivePacket(NetInterface *interface);

error_t avr32EthUpdateMacAddrFilter(NetInterface *interface);
error_t avr32EthUpdateMacConfig(NetInterface *interface);

void avr32EthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data);

uint16_t avr32EthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
