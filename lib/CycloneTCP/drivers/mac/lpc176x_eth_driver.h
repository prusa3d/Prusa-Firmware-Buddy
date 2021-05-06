/**
 * @file lpc176x_eth_driver.h
 * @brief LPC1764/66/67/68/69 Ethernet MAC driver
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

#ifndef _LPC176X_ETH_DRIVER_H
#define _LPC176X_ETH_DRIVER_H

//Dependencies
#include "core/nic.h"

//Number of TX buffers
#ifndef LPC176X_ETH_TX_BUFFER_COUNT
   #define LPC176X_ETH_TX_BUFFER_COUNT 2
#elif (LPC176X_ETH_TX_BUFFER_COUNT < 1)
   #error LPC176X_ETH_TX_BUFFER_COUNT parameter is not valid
#endif

//TX buffer size
#ifndef LPC176X_ETH_TX_BUFFER_SIZE
   #define LPC176X_ETH_TX_BUFFER_SIZE 1536
#elif (LPC176X_ETH_TX_BUFFER_SIZE != 1536)
   #error LPC176X_ETH_TX_BUFFER_SIZE parameter is not valid
#endif

//Number of RX buffers
#ifndef LPC176X_ETH_RX_BUFFER_COUNT
   #define LPC176X_ETH_RX_BUFFER_COUNT 4
#elif (LPC176X_ETH_RX_BUFFER_COUNT < 1)
   #error LPC176X_ETH_RX_BUFFER_COUNT parameter is not valid
#endif

//RX buffer size
#ifndef LPC176X_ETH_RX_BUFFER_SIZE
   #define LPC176X_ETH_RX_BUFFER_SIZE 1536
#elif (LPC176X_ETH_RX_BUFFER_SIZE != 1536)
   #error LPC176X_ETH_RX_BUFFER_SIZE parameter is not valid
#endif

//Interrupt priority grouping
#ifndef LPC176X_ETH_IRQ_PRIORITY_GROUPING
   #define LPC176X_ETH_IRQ_PRIORITY_GROUPING 2
#elif (LPC176X_ETH_IRQ_PRIORITY_GROUPING < 0)
   #error LPC176X_ETH_IRQ_PRIORITY_GROUPING parameter is not valid
#endif

//Ethernet interrupt group priority
#ifndef LPC176X_ETH_IRQ_GROUP_PRIORITY
   #define LPC176X_ETH_IRQ_GROUP_PRIORITY 24
#elif (LPC176X_ETH_IRQ_GROUP_PRIORITY < 0)
   #error LPC176X_ETH_IRQ_GROUP_PRIORITY parameter is not valid
#endif

//Ethernet interrupt subpriority
#ifndef LPC176X_ETH_IRQ_SUB_PRIORITY
   #define LPC176X_ETH_IRQ_SUB_PRIORITY 0
#elif (LPC176X_ETH_IRQ_SUB_PRIORITY < 0)
   #error LPC176X_ETH_IRQ_SUB_PRIORITY parameter is not valid
#endif

//MAC1 register
#define MAC1_SOFT_RESET                0x00008000
#define MAC1_SIMULATION_RESET          0x00004000
#define MAC1_RESET_MCS_RX              0x00000800
#define MAC1_RESET_RX                  0x00000400
#define MAC1_RESET_MCS_TX              0x00000200
#define MAC1_RESET_TX                  0x00000100
#define MAC1_LOOPBACK                  0x00000010
#define MAC1_TX_FLOW_CONTROL           0x00000008
#define MAC1_RX_FLOW_CONTROL           0x00000004
#define MAC1_PASS_ALL_FRAMES           0x00000002
#define MAC1_RECEIVE_ENABLE            0x00000001

//MAC2 register
#define MAC2_EXCESS_DEFER              0x00004000
#define MAC2_BACK_PRESSURE_NO_BACKOFF  0x00002000
#define MAC2_NO_BACKOFF                0x00001000
#define MAC2_LONG_PREAMBLE_ENFORCEMENT 0x00000200
#define MAC2_PURE_PREAMBLE_ENFORCEMENT 0x00000100
#define MAC2_AUTO_DETECT_PAD_ENABLE    0x00000080
#define MAC2_VLAN_PAD_ENABLE           0x00000040
#define MAC2_PAD_CRC_ENABLE            0x00000020
#define MAC2_CRC_ENABLE                0x00000010
#define MAC2_DELAYED_CRC               0x00000008
#define MAC2_HUGE_FRAME_ENABLE         0x00000004
#define MAC2_FRAME_LENGTH_CHECKING     0x00000002
#define MAC2_FULL_DUPLEX               0x00000001

//IPGT register
#define IPGT_BACK_TO_BACK_IPG          0x0000007F
#define IPGT_HALF_DUPLEX               0x00000012
#define IPGT_FULL_DUPLEX               0x00000015

//IPGR register
#define IPGR_NON_BACK_TO_BACK_IPG1     0x00007F00
#define IPGR_NON_BACK_TO_BACK_IPG2     0x0000007F
#define IPGR_DEFAULT_VALUE             0x00000C12

//CLRT register
#define CLRT_COLLISION_WINDOW          0x00003F00
#define CLRT_RETRANSMISSION_MAXIMUM    0x00003F00
#define CLRT_DEFAULT_VALUE             0x0000370F

//MAXF register
#define MAXF_MAXIMUM_FRAME_LENGTH      0x0000FFFF

//SUPP register
#define SUPP_SPEED                     0x00000100

//TEST register
#define TEST_BACKPRESSURE              0x00000004
#define TEST_PAUSE                     0x00000002
#define TEST_SHORTCUT_PAUSE_QUANTA     0x00000001

//MCFG register
#define MCFG_RESET_MII_MGMT            0x00008000
#define MCFG_CLOCK SELECT              0x0000003C
#define MCFG_SUPPRESS_PREAMBLE         0x00000002
#define MCFG_SCAN_INCREMENT            0x00000001

#define MCFG_CLOCK_SELECT_DIV4         0x00000000
#define MCFG_CLOCK_SELECT_DIV6         0x00000008
#define MCFG_CLOCK_SELECT_DIV8         0x0000000C
#define MCFG_CLOCK_SELECT_DIV10        0x00000010
#define MCFG_CLOCK_SELECT_DIV14        0x00000014
#define MCFG_CLOCK_SELECT_DIV20        0x00000018
#define MCFG_CLOCK_SELECT_DIV28        0x0000001C
#define MCFG_CLOCK_SELECT_DIV36        0x00000020
#define MCFG_CLOCK_SELECT_DIV40        0x00000024
#define MCFG_CLOCK_SELECT_DIV44        0x00000028
#define MCFG_CLOCK_SELECT_DIV48        0x0000002C
#define MCFG_CLOCK_SELECT_DIV52        0x00000030
#define MCFG_CLOCK_SELECT_DIV56        0x00000034
#define MCFG_CLOCK_SELECT_DIV60        0x00000038
#define MCFG_CLOCK_SELECT_DIV64        0x0000003C

//MCMD register
#define MCMD_SCAN                      0x00000002
#define MCMD_READ                      0x00000001

//MADR register
#define MADR_PHY_ADDRESS               0x00001F00
#define MADR_REGISTER_ADDRESS          0x0000001F

//MWTD register
#define MWTD_WRITE_DATA                0x0000FFFF

//MRDD register
#define MRDD_READ_DATA                 0x0000FFFF

//MIND register
#define MIND_MII_LINK_FAIL             0x00000008
#define MIND_NOT_VALID                 0x00000004
#define MIND_SCANNING                  0x00000002
#define MIND_BUSY                      0x00000001

//Command register
#define COMMAND_FULL_DUPLEX            0x00000400
#define COMMAND_RMII                   0x00000200
#define COMMAND_TX_FLOW_CONTROL        0x00000100
#define COMMAND_PASS_RX_FILTER         0x00000080
#define COMMAND_PASS_RUNT_FRAME        0x00000040
#define COMMAND_RX_RESET               0x00000020
#define COMMAND_TX_RESET               0x00000010
#define COMMAND_REG_RESET              0x00000008
#define COMMAND_TX_ENABLE              0x00000002
#define COMMAND_RX_ENABLE              0x00000001

//Status register
#define STATUS_TX                      0x00000002
#define STATUS_RX                      0x00000001

//TSV0 register
#define TSV0_VLAN                      0x80000000
#define TSV0_BACKPRESSURE              0x40000000
#define TSV0_PAUSE                     0x20000000
#define TSV0_CONTROL_FRAME             0x10000000
#define TSV0_TOTAL_BYTES               0x0FFFF000
#define TSV0_UNDERRUN                  0x00000800
#define TSV0_GIANT                     0x00000400
#define TSV0_LATE_COLLISION            0x00000200
#define TSV0_EXCESSIVE_COLLISION       0x00000100
#define TSV0_EXCESSIVE_DEFER           0x00000080
#define TSV0_PACKET_DEFER              0x00000040
#define TSV0_BROADCAST                 0x00000020
#define TSV0_MULTICAST                 0x00000010
#define TSV0_DONE                      0x00000008
#define TSV0_LENGTH_OUT_OF_RANGE       0x00000004
#define TSV0_LENGTH_CHECK_ERROR        0x00000002
#define TSV0_CRC_ERROR                 0x00000001

//TSV1 register
#define TSV1_TRANSMIT_COLLISION_COUNT  0x000F0000
#define TSV1_TRANSMIT_BYTE_COUNT       0x0000FFFF

//RSV register
#define RSV_VLAN                       0x40000000
#define RSV_UNSUPPORTED_OPCODE         0x20000000
#define RSV_PAUSE                      0x10000000
#define RSV_CONTROL_FRAME              0x08000000
#define RSV_DRIBBLE_NIBBLE             0x04000000
#define RSV_BROADCAST                  0x02000000
#define RSV_MULTICAST                  0x01000000
#define RSV_RECEIVE_OK                 0x00800000
#define RSV_LENGTH_OUT_OF_RANGE        0x00400000
#define RSV_LENGTH_CHECK_ERROR         0x00200000
#define RSV_CRC_ERROR                  0x00100000
#define RSV_RECEIVE_CODE_VIOLATION     0x00080000
#define RSV_CARRIER_EVENT_PREV_SEEN    0x00040000
#define RSV_RXDV_EVENT_PREV_SEEN       0x00020000
#define RSV_PACKET_PREVIOUSLY_IGNORED  0x00010000
#define RSV_RECEIVED_BYTE_COUNT        0x0000FFFF

//FlowControlCounter register
#define FCC_PAUSE_TIMER                0xFFFF0000
#define FCC_MIRROR_COUNTER             0x0000FFFF

//FlowControlStatus register
#define FCS_MIRROR_COUNTER_CURRENT     0x0000FFFF

//RxFilterCtrl register
#define RFC_RX_FILTER_EN_WOL           0x00002000
#define RFC_MAGIC_PACKET_EN_WOL        0x00001000
#define RFC_ACCEPT_PERFECT_EN          0x00000020
#define RFC_ACCEPT_MULTICAST_HASH_EN   0x00000010
#define RFC_ACCEPT_UNICAST_HASH_EN     0x00000008
#define RFC_ACCEPT_MULTICAST_EN        0x00000004
#define RFC_ACCEPT_BROADCAST_EN        0x00000002
#define RFC_ACCEPT_UNICAST_EN          0x00000001

//RxFilterWoLStatus and RxFilterWoLClear registers
#define RFWS_MAGIC_PACKET_WOL          0x00000100
#define RFWS_RX_FILTER_WOL             0x00000080
#define RFWS_ACCEPT_PERFECT_WOL        0x00000020
#define RFWS_ACCEPT_MULTICAST_HASH_WOL 0x00000010
#define RFWS_ACCEPT_UNICAST_HASH_WOL   0x00000008
#define RFWS_ACCEPT_MULTICAST_WOL      0x00000004
#define RFWS_ACCEPT_BROADCAST_WOL      0x00000002
#define RFWS_ACCEPT_UNICAST_WOL        0x00000001

//IntStatus, IntEnable, IntClear and IntSet registers
#define INT_WAKEUP                     0x00002000
#define INT_SOFT_INT                   0x00001000
#define INT_TX_DONE                    0x00000080
#define INT_TX_FINISHED                0x00000040
#define INT_TX_ERROR                   0x00000020
#define INT_TX_UNDERRUN                0x00000010
#define INT_RX_DONE                    0x00000008
#define INT_RX_FINISHED                0x00000004
#define INT_RX_ERROR                   0x00000002
#define INT_RX_OVERRUN                 0x00000001

//Transmit descriptor control word
#define TX_CTRL_INTERRUPT              0x80000000
#define TX_CTRL_LAST                   0x40000000
#define TX_CTRL_CRC                    0x20000000
#define TX_CTRL_PAD                    0x10000000
#define TX_CTRL_HUGE                   0x08000000
#define TX_CTRL_OVERRIDE               0x04000000
#define TX_CTRL_SIZE                   0x000007FF

//Transmit status information word
#define TX_STATUS_ERROR                0x80000000
#define TX_STATUS_NO_DESCRIPTOR        0x40000000
#define TX_STATUS_UNDERRUN             0x20000000
#define TX_STATUS_LATE_COLLISION       0x10000000
#define TX_STATUS_EXCESSIVE_COLLISION  0x08000000
#define TX_STATUS_EXCESSIVE_DEFER      0x04000000
#define TX_STATUS_DEFER                0x02000000
#define TX_STATUS_COLLISION_COUNT      0x01E00000

//Receive descriptor control word
#define RX_CTRL_INTERRUPT              0x80000000
#define RX_CTRL_SIZE                   0x000007FF

//Receive status information word
#define RX_STATUS_ERROR                0x80000000
#define RX_STATUS_LAST_FLAG            0x40000000
#define RX_STATUS_NO_DESCRIPTOR        0x20000000
#define RX_STATUS_OVERRUN              0x10000000
#define RX_STATUS_ALIGNMENT_ERROR      0x08000000
#define RX_STATUS_RANGE_ERROR          0x04000000
#define RX_STATUS_LENGTH_ERROR         0x02000000
#define RX_STATUS_SYMBOL_ERROR         0x01000000
#define RX_STATUS_CRC_ERROR            0x00800000
#define RX_STATUS_BROADCAST            0x00400000
#define RX_STATUS_MULTICAST            0x00200000
#define RX_STATUS_FAIL_FILTER          0x00100000
#define RX_STATUS_VLAN                 0x00080000
#define RX_STATUS_CONTROL_FRAME        0x00040000
#define RX_STATUS_SIZE                 0x000007FF

//Receive status HashCRC word
#define RX_HASH_CRC_DA                 0x001FF000
#define RX_HASH_CRC_SA                 0x000001FF

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Transmit descriptor
 **/

typedef struct
{
   uint32_t packet;
   uint32_t control;
} Lpc176xTxDesc;


/**
 * @brief Transmit status
 **/

typedef struct
{
   uint32_t info;
} Lpc176xTxStatus;


/**
 * @brief Receive descriptor
 **/

typedef struct
{
   uint32_t packet;
   uint32_t control;
} Lpc176xRxDesc;


/**
 * @brief Receive status
 **/

typedef struct
{
   uint32_t info;
   uint32_t hashCrc;
} Lpc176xRxStatus;


//LPC176x Ethernet MAC driver
extern const NicDriver lpc176xEthDriver;

//LPC176x Ethernet MAC related functions
error_t lpc176xEthInit(NetInterface *interface);
void lpc176xEthInitGpio(NetInterface *interface);
void lpc176xEthInitDesc(NetInterface *interface);

void lpc176xEthTick(NetInterface *interface);

void lpc176xEthEnableIrq(NetInterface *interface);
void lpc176xEthDisableIrq(NetInterface *interface);
void lpc176xEthEventHandler(NetInterface *interface);

error_t lpc176xEthSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t lpc176xEthReceivePacket(NetInterface *interface);

error_t lpc176xEthUpdateMacAddrFilter(NetInterface *interface);
error_t lpc176xEthUpdateMacConfig(NetInterface *interface);

void lpc176xEthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data);

uint16_t lpc176xEthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr);

uint32_t lpc176xEthCalcCrc(const void *data, size_t length);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
