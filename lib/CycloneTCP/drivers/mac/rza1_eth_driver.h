/**
 * @file rza1_eth_driver.h
 * @brief Renesas RZ/A1 Ethernet MAC driver
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

#ifndef _RZA1_ETH_DRIVER_H
#define _RZA1_ETH_DRIVER_H

//Dependencies
#include "core/nic.h"

//Number of TX buffers
#ifndef RZA1_ETH_TX_BUFFER_COUNT
   #define RZA1_ETH_TX_BUFFER_COUNT 8
#elif (RZA1_ETH_TX_BUFFER_COUNT < 1)
   #error RZA1_ETH_TX_BUFFER_COUNT parameter is not valid
#endif

//TX buffer size
#ifndef RZA1_ETH_TX_BUFFER_SIZE
   #define RZA1_ETH_TX_BUFFER_SIZE 1536
#elif (RZA1_ETH_TX_BUFFER_SIZE != 1536)
   #error RZA1_ETH_TX_BUFFER_SIZE parameter is not valid
#endif

//Number of RX buffers
#ifndef RZA1_ETH_RX_BUFFER_COUNT
   #define RZA1_ETH_RX_BUFFER_COUNT 8
#elif (RZA1_ETH_RX_BUFFER_COUNT < 1)
   #error RZA1_ETH_RX_BUFFER_COUNT parameter is not valid
#endif

//RX buffer size
#ifndef RZA1_ETH_RX_BUFFER_SIZE
   #define RZA1_ETH_RX_BUFFER_SIZE 1536
#elif (RZA1_ETH_RX_BUFFER_SIZE != 1536)
   #error RZA1_ETH_RX_BUFFER_SIZE parameter is not valid
#endif

//Ethernet interrupt priority
#ifndef RZA1_ETH_IRQ_PRIORITY
   #define RZA1_ETH_IRQ_PRIORITY 25
#elif (RZA1_ETH_IRQ_PRIORITY < 0)
   #error RZA1_ETH_IRQ_PRIORITY parameter is not valid
#endif

//Name of the section where to place DMA buffers
#ifndef RZA1_ETH_RAM_SECTION
   #define RZA1_ETH_RAM_SECTION ".BSS_DMAC_SAMPLE_INTERNAL_RAM"
#endif

//ARSTR register
#define ETHER_ARSTR_ARST         0x00000001

//ECMR0 register
#define ETH_ECMR0_TRCCM          0x04000000
#define ETH_ECMR0_RCSC           0x00800000
#define ETH_ECMR0_DPAD           0x00200000
#define ETH_ECMR0_RZPF           0x00100000
#define ETH_ECMR0_ZPF            0x00080000
#define ETH_ECMR0_PFR            0x00040000
#define ETH_ECMR0_RXF            0x00020000
#define ETH_ECMR0_TXF            0x00010000
#define ETH_ECMR0_MCT            0x00002000
#define ETH_ECMR0_RE             0x00000040
#define ETH_ECMR0_TE             0x00000020
#define ETH_ECMR0_DM             0x00000002
#define ETH_ECMR0_PRM            0x00000001

//PIR0 register
#define ETHER_PIR0_MDI           0x00000008
#define ETHER_PIR0_MDO           0x00000004
#define ETHER_PIR0_MMD           0x00000002
#define ETHER_PIR0_MDC           0x00000001

//TSU_ADSBSY register
#define ETHER_TSU_ADSBSY_ADSBSY  0x00000001

//EDSR0 register
#define ETHER_EDSR0_ENT          0x00000002
#define ETHER_EDSR0_ENR          0x00000001

//EDMR0 register
#define ETHER_EDMR0_DE           0x00000040
#define ETHER_EDMR0_DL           0x00000030
#define ETHER_EDMR0_SWRT         0x00000002
#define ETHER_EDMR0_SWRR         0x00000001

#define ETHER_EDMR0_DL_16        0x00000000
#define ETHER_EDMR0_DL_32        0x00000010
#define ETHER_EDMR0_DL_64        0x00000020

//EDTRR0 register
#define ETHER_EDTRR0_TR          0x00000003

//EDRRR0 register
#define ETHER_EDRRR0_RR          0x00000001

//EESR0 register
#define ETHER_EESR0_TWB          0xC0000000
#define ETHER_EESR0_TC1          0x20000000
#define ETHER_EESR0_TUC          0x10000000
#define ETHER_EESR0_ROC          0x08000000
#define ETHER_EESR0_TABT         0x04000000
#define ETHER_EESR0_RABT         0x02000000
#define ETHER_EESR0_RFCOF        0x01000000
#define ETHER_EESR0_ECI          0x00400000
#define ETHER_EESR0_TC0          0x00200000
#define ETHER_EESR0_TDE          0x00100000
#define ETHER_EESR0_TFUF         0x00080000
#define ETHER_EESR0_FR           0x00040000
#define ETHER_EESR0_RDE          0x00020000
#define ETHER_EESR0_RFOF         0x00010000
#define ETHER_EESR0_RMAF         0x00000080
#define ETHER_EESR0_RRF          0x00000010
#define ETHER_EESR0_RTLF         0x00000008
#define ETHER_EESR0_RTSF         0x00000004
#define ETHER_EESR0_PRE          0x00000002
#define ETHER_EESR0_CERF         0x00000001

//EESIPR0 register
#define ETHER_EESIPR0_TWBIP      0xC0000000
#define ETHER_EESIPR0_TC1IP      0x20000000
#define ETHER_EESIPR0_TUCIP      0x10000000
#define ETHER_EESIPR0_ROCIP      0x08000000
#define ETHER_EESIPR0_TABTIP     0x04000000
#define ETHER_EESIPR0_RABTIP     0x02000000
#define ETHER_EESIPR0_RFCOFIP    0x01000000
#define ETHER_EESIPR0_ECIIP      0x00400000
#define ETHER_EESIPR0_TC0IP      0x00200000
#define ETHER_EESIPR0_TDEIP      0x00100000
#define ETHER_EESIPR0_TFUFIP     0x00080000
#define ETHER_EESIPR0_FRIP       0x00040000
#define ETHER_EESIPR0_RDEIP      0x00020000
#define ETHER_EESIPR0_RFOFIP     0x00010000
#define ETHER_EESIPR0_RMAFIP     0x00000080
#define ETHER_EESIPR0_RRFIP      0x00000010
#define ETHER_EESIPR0_RTLFIP     0x00000008
#define ETHER_EESIPR0_RTSFIP     0x00000004
#define ETHER_EESIPR0_PREIP      0x00000002
#define ETHER_EESIPR0_CERFIP     0x00000001

//TDFFR0 register
#define ETHER_TDFFR_TDLF         0x00000001

//RDFFR0 register
#define ETHER_RDFFR0_RDLF        0x00000001

//FDR0 register
#define ETHER_FDR0_TFD           0x00000700
#define ETHER_FDR0_RFD           0X0000001F

#define ETHER_FDR0_TFD_2048      0x00000700
#define ETHER_FDR0_RFD_2048      0x00000007

//RMCR0 register
#define ETHER_RMCR0_RNC          0x00000001

//FCFTR register
#define ETHER_FCFTR0_RFF         0x001F0000
#define ETHER_FCFTR0_RFD         0x000000FF

#define ETHER_FCFTR0_RFF_8       0x00070000
#define ETHER_FCFTR0_RFD_2048    0x00000007

//Transmit DMA descriptor flags
#define ETHER_TD0_TACT           0x80000000
#define ETHER_TD0_TDLE           0x40000000
#define ETHER_TD0_TFP_SOF        0x20000000
#define ETHER_TD0_TFP_EOF        0x10000000
#define ETHER_TD0_TFE            0x08000000
#define ETHER_TD0_TWBI           0x04000000
#define ETHER_TD0_TFS_MASK       0x00000300
#define ETHER_TD0_TFS_TUC        0x00000200
#define ETHER_TD0_TFS_TABT       0x00000100
#define ETHER_TD1_TDL            0xFFFF0000
#define ETHER_TD2_TBA            0xFFFFFFFF

//Receive DMA descriptor flags
#define ETHER_RD0_RACT           0x80000000
#define ETHER_RD0_RDLE           0x40000000
#define ETHER_RD0_RFP_SOF        0x20000000
#define ETHER_RD0_RFP_EOF        0x10000000
#define ETHER_RD0_RFE            0x08000000
#define ETHER_RD0_RCSE           0x04000000
#define ETHER_RD0_RFS_MASK       0x02DF0000
#define ETHER_RD0_RFS_RFOF       0x02000000
#define ETHER_RD0_RFS_RMAF       0x00800000
#define ETHER_RD0_RFS_RUAF       0x00400000
#define ETHER_RD0_RFS_RRF        0x00100000
#define ETHER_RD0_RFS_RTLF       0x00080000
#define ETHER_RD0_RFS_RTSF       0x00040000
#define ETHER_RD0_RFS_PRE        0x00020000
#define ETHER_RD0_RFS_CERF       0x00010000
#define ETHER_RD0_RCS            0x0000FFFF
#define ETHER_RD1_RBL            0xFFFF0000
#define ETHER_RD1_RDL            0x0000FFFF
#define ETHER_RD2_RBA            0xFFFFFFFF

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Transmit DMA descriptor
 **/

typedef struct
{
   uint32_t td0;
   uint32_t td1;
   uint32_t td2;
   uint32_t padding;
} Rza1TxDmaDesc;


/**
 * @brief Receive DMA descriptor
 **/

typedef struct
{
   uint32_t rd0;
   uint32_t rd1;
   uint32_t rd2;
   uint32_t padding;
} Rza1RxDmaDesc;


//RZ/A1 Ethernet MAC driver
extern const NicDriver rza1EthDriver;

//RZ/A1 Ethernet MAC related functions
error_t rza1EthInit(NetInterface *interface);
void rza1EthInitGpio(NetInterface *interface);
void rza1EthInitDmaDesc(NetInterface *interface);

void rza1EthTick(NetInterface *interface);

void rza1EthEnableIrq(NetInterface *interface);
void rza1EthDisableIrq(NetInterface *interface);
void rza1EthIrqHandler(uint32_t intSense);
void rza1EthEventHandler(NetInterface *interface);

error_t rza1EthSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary);

error_t rza1EthReceivePacket(NetInterface *interface);

error_t rza1EthUpdateMacAddrFilter(NetInterface *interface);
error_t rza1EthUpdateMacConfig(NetInterface *interface);

void rza1EthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data);

uint16_t rza1EthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr);

void rza1EthWriteSmi(uint32_t data, uint_t length);
uint32_t rza1EthReadSmi(uint_t length);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
