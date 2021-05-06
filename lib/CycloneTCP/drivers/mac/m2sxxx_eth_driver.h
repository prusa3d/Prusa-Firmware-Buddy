/**
 * @file m2sxxx_eth_driver.h
 * @brief SmartFusion2 (M2Sxxx) Ethernet MAC driver
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

#ifndef _M2SXXX_ETH_DRIVER_H
#define _M2SXXX_ETH_DRIVER_H

//Dependencies
#include "core/nic.h"

//Number of TX buffers
#ifndef M2SXXX_ETH_TX_BUFFER_COUNT
   #define M2SXXX_ETH_TX_BUFFER_COUNT 2
#elif (M2SXXX_ETH_TX_BUFFER_COUNT < 1)
   #error M2SXXX_ETH_TX_BUFFER_COUNT parameter is not valid
#endif

//TX buffer size
#ifndef M2SXXX_ETH_TX_BUFFER_SIZE
   #define M2SXXX_ETH_TX_BUFFER_SIZE 1536
#elif (M2SXXX_ETH_TX_BUFFER_SIZE != 1536)
   #error M2SXXX_ETH_TX_BUFFER_SIZE parameter is not valid
#endif

//Number of RX buffers
#ifndef M2SXXX_ETH_RX_BUFFER_COUNT
   #define M2SXXX_ETH_RX_BUFFER_COUNT 4
#elif (M2SXXX_ETH_RX_BUFFER_COUNT < 1)
   #error M2SXXX_ETH_RX_BUFFER_COUNT parameter is not valid
#endif

//RX buffer size
#ifndef M2SXXX_ETH_RX_BUFFER_SIZE
   #define M2SXXX_ETH_RX_BUFFER_SIZE 1536
#elif (M2SXXX_ETH_RX_BUFFER_SIZE != 1536)
   #error M2SXXX_ETH_RX_BUFFER_SIZE parameter is not valid
#endif

//Interrupt priority grouping
#ifndef M2SXXX_ETH_IRQ_PRIORITY_GROUPING
   #define M2SXXX_ETH_IRQ_PRIORITY_GROUPING 3
#elif (M2SXXX_ETH_IRQ_PRIORITY_GROUPING < 0)
   #error M2SXXX_ETH_IRQ_PRIORITY_GROUPING parameter is not valid
#endif

//Ethernet interrupt group priority
#ifndef M2SXXX_ETH_IRQ_GROUP_PRIORITY
   #define M2SXXX_ETH_IRQ_GROUP_PRIORITY 12
#elif (M2SXXX_ETH_IRQ_GROUP_PRIORITY < 0)
   #error M2SXXX_ETH_IRQ_GROUP_PRIORITY parameter is not valid
#endif

//Ethernet interrupt subpriority
#ifndef M2SXXX_ETH_IRQ_SUB_PRIORITY
   #define M2SXXX_ETH_IRQ_SUB_PRIORITY 0
#elif (M2SXXX_ETH_IRQ_SUB_PRIORITY < 0)
   #error M2SXXX_ETH_IRQ_SUB_PRIORITY parameter is not valid
#endif

//EDAC_CR register
#define EDAC_CR_CAN_EDAC_EN               0x00000040
#define EDAC_CR_USB_EDAC_EN               0x00000020
#define EDAC_CR_MAC_EDAC_RX_EN            0x00000010
#define EDAC_CR_MAC_EDAC_TX_EN            0x00000008
#define EDAC_CR_ESRAM1_EDAC_EN            0x00000002
#define EDAC_CR_ESRAM0_EDAC_EN            0x00000001

//MAC_CR register
#define MAC_CR_RGMII_TXC_DELAY_SEL        0x000001E0
#define MAC_CR_ETH_PHY_MODE               0x0000001C
#define MAC_CR_ETH_LINE_SPEED             0x00000003

#define MAC_CR_ETH_PHY_MODE_RMII          0x00000000
#define MAC_CR_ETH_PHY_MODE_TBI           0x00000008
#define MAC_CR_ETH_PHY_MODE_MII           0x0000000C
#define MAC_CR_ETH_PHY_MODE_GMII          0x00000010

#define MAC_CR_ETH_LINE_SPEED_10MBPS      0x00000000
#define MAC_CR_ETH_LINE_SPEED_100MBPS     0x00000001
#define MAC_CR_ETH_LINE_SPEED_1000MBPS    0x00000002

//DMA_TX_CTRL register
#define DMA_TX_CTRL_TX_EN                 0x00000001

//DMA_TX_STATUS register
#define DMA_TX_STATUS_TX_PKT_COUNT        0x00FF0000
#define DMA_TX_STATUS_TX_BUS_ERROR        0x00000008
#define DMA_TX_STATUS_TX_UNDERRUN         0x00000002
#define DMA_TX_STATUS_TX_PKT_SENT         0x00000001

//DMA_RX_CTRL register
#define DMA_RX_CTRL_RX_EN                 0x00000001

//DMA_RX_STATUS register
#define DMA_RX_STATUS_RX_PKT_COUNT        0x00FF0000
#define DMA_RX_STATUS_RX_BUS_ERROR        0x00000008
#define DMA_RX_STATUS_RX_OVERFLOW         0x00000004
#define DMA_RX_STATUS_RX_PKT_RECEIVED     0x00000001

//DMA_IRQ_MASK register
#define DMA_IRQ_MASK_RX_BUS_ERROR         0x00000080
#define DMA_IRQ_MASK_RX_OVERFLOW          0x00000040
#define DMA_IRQ_MASK_RX_PKT_RECEIVED      0x00000010
#define DMA_IRQ_MASK_TX_BUS_ERROR         0x00000008
#define DMA_IRQ_MASK_TX_UNDERRUN          0x00000002
#define DMA_IRQ_MASK_TX_PKT_SENT          0x00000001

//DMA_IRQ register
#define DMA_IRQ_RX_BUS_ERROR              0x00000080
#define DMA_IRQ_RX_OVERFLOW               0x00000040
#define DMA_IRQ_RX_PKT_RECEIVED           0x00000010
#define DMA_IRQ_TX_BUS_ERROR              0x00000008
#define DMA_IRQ_TX_UNDERRUN               0x00000002
#define DMA_IRQ_TX_PKT_SENT               0x00000001

//CFG1 register
#define CFG1_SOFT_RESET                   0x80000000
#define CFG1_SIMULATION_RESET             0x40000000
#define CFG1_RESET_RX_MAC_CTRL            0x00080000
#define CFG1_RESET_TX_MAC_CTRL            0x00040000
#define CFG1_RESET_RX_FUNCTION            0x00020000
#define CFG1_RESET_TX_FUNCTION            0x00010000
#define CFG1_LOOP_BACK                    0x00000100
#define CFG1_RX_FLOW_CTRL_EN              0x00000020
#define CFG1_TX_FLOW_CTRL_EN              0x00000010
#define CFG1_SYNC_RX_EN                   0x00000008
#define CFG1_RX_EN                        0x00000004
#define CFG1_SYNC_TX_EN                   0x00000002
#define CFG1_TX_EN                        0x00000001

//CFG2 register
#define CFG2_PREAMBLE_LENGTH              0x0000F000
#define CFG2_INTERFACE_MODE               0x00000300
#define CFG2_HUGE FRAME_EN                0x00000020
#define CFG2_LENGTH_FIELD_CHECK           0x00000010
#define CFG2_PAD_CRC_EN                   0x00000004
#define CFG2_CRC_EN                       0x00000002
#define CFG2_FULL_DUPLEX                  0x00000001

#define CFG2_PREAMBLE_7                   0x00007000

#define CFG2_INTERFACE_MODE_NIBBLE        0x00000100
#define CFG2_INTERFACE_MODE_BYTE          0x00000200

//MII_CONFIG register
#define MII_CONFIG_CLKSEL_DIV4            0x00000000
#define MII_CONFIG_CLKSEL_DIV6            0x00000002
#define MII_CONFIG_CLKSEL_DIV8            0x00000003
#define MII_CONFIG_CLKSEL_DIV10           0x00000004
#define MII_CONFIG_CLKSEL_DIV14           0x00000005
#define MII_CONFIG_CLKSEL_DIV20           0x00000006
#define MII_CONFIG_CLKSEL_DIV28           0x00000007

//MII_COMMAND register
#define MII_COMMAND_SCAN                  0x00000002
#define MII_COMMAND_READ                  0x00000001

//MII_ADDRESS register
#define MII_ADDRESS_PHY_ADDR              0x00001F00
#define MII_ADDRESS_REG_ADDR              0x0000001F

#define MII_ADDRESS_PHY_ADDR_POS          8
#define MII_ADDRESS_REG_ADDR_POS          0

//MII_INDICATORS register
#define MII_INDICATORS_NOT_VALID          0x00000004
#define MII_INDICATORS_SCANNING           0x00000002
#define MII_INDICATORS_BUSY               0x00000001

//INTERFACE_CTRL register
#define INTERFACE_CTRL_RESET              0x80000000
#define INTERFACE_CTRL_TBI_MODE           0x08000000
#define INTERFACE_CTRL_GHD_MODE           0x04000000
#define INTERFACE_CTRL_LHD_MODE           0x02000000
#define INTERFACE_CTRL_PHY_MODE           0x01000000
#define INTERFACE_CTRL_RESET_PERMII       0x00800000
#define INTERFACE_CTRL_SPEED              0x00010000
#define INTERFACE_CTRL_RESET_PE100X       0x00008000
#define INTERFACE_CTRL_FORCE_QUIET        0x00000400
#define INTERFACE_CTRL_NO_CIPHER          0x00000200
#define INTERFACE_CTRL_DISABLE_LINK_FAIL  0x00000100
#define INTERFACE_CTRL_EN_JABBER_PROTECT  0x00000001

//FIFO_CFG0 register
#define FIFO_CFG0_STFENRPLY               0x00080000
#define FIFO_CFG0_FRFENRPLY               0x00040000
#define FIFO_CFG0_SRFENRPLY               0x00020000
#define FIFO_CFG0_WTMENRPLY               0x00010000
#define FIFO_CFG0_FTFENREQ                0x00001000
#define FIFO_CFG0_STFENREQ                0x00000800
#define FIFO_CFG0_FRFENREQ                0x00000400
#define FIFO_CFG0_SRFENREQ                0x00000200
#define FIFO_CFG0_WTMENREQ                0x00000100
#define FIFO_CFG0_HSTRSTFT                0x00000010
#define FIFO_CFG0_HSTRSTST                0x00000008
#define FIFO_CFG0_HSTRSTFR                0x00000004
#define FIFO_CFG0_HSTRSTSR                0x00000002
#define FIFO_CFG0_HSTRSTWT                0x00000001

//FIFO_CFG1 register
#define FIFO_CFG1_CFGSRTH                 0x0FFF0000
#define FIFO_CFG1_CFGXOFFRTX              0x0000FFFF

#define FIFO_CFG1_DEFAULT_VALUE           0x0FFF0000

//FIFO_CFG2 register
#define FIFO_CFG2_CFGHWM                  0x1FFF0000
#define FIFO_CFG2_CFGLWM                  0x00001FFF

#define FIFO_CFG2_DEFAULT_VALUE           0x04000180

//FIFO_CFG3 register
#define FIFO_CFG3_CFGHWMFT                0x0FFF0000
#define FIFO_CFG3_CFGFTTH                 0x00000FFF

#define FIFO_CFG3_DEFAULT_VALUE           0x0258FFFF

//FIFO_CFG4 register
#define FIFO_CFG4_HSTFLTRFRM              0x0003FFFF
#define FIFO_CFG4_RECEIVE_LONG_EVENT      0x00020000
#define FIFO_CFG4_VLAN                    0x00010000
#define FIFO_CFG4_CONTROL_NOT_PAUSE       0x00008000
#define FIFO_CFG4_CONTROL_PAUSE           0x00004000
#define FIFO_CFG4_CONTROL                 0x00002000
#define FIFO_CFG4_TRUNCATED               0x00001000
#define FIFO_CFG4_LONG_EVENT              0x00000800
#define FIFO_CFG4_DRIBBLE_NIBBLE          0x00000400
#define FIFO_CFG4_BROADCAST               0x00000200
#define FIFO_CFG4_MULTICAST               0x00000100
#define FIFO_CFG4_RECEPTION_OK            0x00000080
#define FIFO_CFG4_TYPE_ERROR              0x00000040
#define FIFO_CFG4_LENGTH_ERROR            0x00000020
#define FIFO_CFG4_INVALID_CRC             0x00000010
#define FIFO_CFG4_RECEIVE_ERROR           0x00000008
#define FIFO_CFG4_FALSE_CARRIER           0x00000004
#define FIFO_CFG4_RX_DV_EVENT             0x00000002
#define FIFO_CFG4_PRIOR_PKT_DROPPED       0x00000001

//FIFO_CFG5 register
#define FIFO_CFG5_CFGHDPLX                0x00400000
#define FIFO_CFG5_SRFULL                  0x00200000
#define FIFO_CFG5_HSTSRFULLCLR            0x00100000
#define FIFO_CFG5_CFGBYTMODE              0x00080000
#define FIFO_CFG5_HSTDRPLT64              0x00040000
#define FIFO_CFG5_HSTFLTRFRMDC            0x0003FFFF
#define FIFO_CFG5_RECEIVE_LONG_EVENT      0x00020000
#define FIFO_CFG5_VLAN                    0x00010000
#define FIFO_CFG5_CONTROL_NOT_PAUSE       0x00008000
#define FIFO_CFG5_CONTROL_PAUSE           0x00004000
#define FIFO_CFG5_CONTROL                 0x00002000
#define FIFO_CFG5_TRUNCATED               0x00001000
#define FIFO_CFG5_LONG_EVENT              0x00000800
#define FIFO_CFG5_DRIBBLE_NIBBLE          0x00000400
#define FIFO_CFG5_BROADCAST               0x00000200
#define FIFO_CFG5_MULTICAST               0x00000100
#define FIFO_CFG5_RECEPTION_OK            0x00000080
#define FIFO_CFG5_TYPE_ERROR              0x00000040
#define FIFO_CFG5_LENGTH_ERROR            0x00000020
#define FIFO_CFG5_INVALID_CRC             0x00000010
#define FIFO_CFG5_RECEIVE_ERROR           0x00000008
#define FIFO_CFG5_FALSE_CARRIER           0x00000004
#define FIFO_CFG5_RX_DV_EVENT             0x00000002
#define FIFO_CFG5_PRIOR_PKT_DROPPED       0x00000001

//DMA descriptor flags
#define DMA_DESC_EMPTY_FLAG               0x80000000
#define DMA_DESC_SIZE_MASK                0x00000FFF

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Transmit DMA descriptor
 **/

typedef struct
{
   uint32_t addr;
   uint32_t size;
   uint32_t next;
} M2sxxxTxDmaDesc;


/**
 * @brief Receive DMA descriptor
 **/

typedef struct
{
   uint32_t addr;
   uint32_t size;
   uint32_t next;
} M2sxxxRxDmaDesc;


//M2Sxxx Ethernet MAC driver
extern const NicDriver m2sxxxEthDriver;

//M2Sxxx Ethernet MAC related functions
error_t m2sxxxEthInit(NetInterface *interface);
void m2sxxxEthInitGpio(NetInterface *interface);
void m2sxxxEthInitDmaDesc(NetInterface *interface);

void m2sxxxEthTick(NetInterface *interface);

void m2sxxxEthEnableIrq(NetInterface *interface);
void m2sxxxEthDisableIrq(NetInterface *interface);
void m2sxxxEthEventHandler(NetInterface *interface);

error_t m2sxxxEthSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t m2sxxxEthReceivePacket(NetInterface *interface);

error_t m2sxxxEthUpdateMacAddrFilter(NetInterface *interface);
error_t m2sxxxEthUpdateMacConfig(NetInterface *interface);

void m2sxxxEthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data);

uint16_t m2sxxxEthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
