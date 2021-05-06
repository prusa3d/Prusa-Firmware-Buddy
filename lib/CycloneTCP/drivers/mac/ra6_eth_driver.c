/**
 * @file ra6_eth_driver.c
 * @brief Renesas RA6M2 / RA6M3 / RA6M4 Ethernet MAC driver
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

//Switch to the appropriate trace level
#define TRACE_LEVEL NIC_TRACE_LEVEL

//Dependencies
#include "bsp_api.h"
#include "core/net.h"
#include "drivers/mac/ra6_eth_driver.h"
#include "debug.h"

//Underlying network interface
static NetInterface *nicDriverInterface;

//IAR EWARM compiler?
#if defined(__ICCARM__)

//Transmit buffer
#pragma data_alignment = 32
static uint8_t txBuffer[RA6_ETH_TX_BUFFER_COUNT][RA6_ETH_TX_BUFFER_SIZE];
//Receive buffer
#pragma data_alignment = 32
static uint8_t rxBuffer[RA6_ETH_RX_BUFFER_COUNT][RA6_ETH_RX_BUFFER_SIZE];
//Transmit DMA descriptors
#pragma data_alignment = 32
static Ra6TxDmaDesc txDmaDesc[RA6_ETH_TX_BUFFER_COUNT];
//Receive DMA descriptors
#pragma data_alignment = 32
static Ra6RxDmaDesc rxDmaDesc[RA6_ETH_RX_BUFFER_COUNT];

//ARM or GCC compiler?
#else

//Transmit buffer
static uint8_t txBuffer[RA6_ETH_TX_BUFFER_COUNT][RA6_ETH_TX_BUFFER_SIZE]
   __attribute__((aligned(32)));
//Receive buffer
static uint8_t rxBuffer[RA6_ETH_RX_BUFFER_COUNT][RA6_ETH_RX_BUFFER_SIZE]
   __attribute__((aligned(32)));
//Transmit DMA descriptors
static Ra6TxDmaDesc txDmaDesc[RA6_ETH_TX_BUFFER_COUNT]
   __attribute__((aligned(32)));
//Receive DMA descriptors
static Ra6RxDmaDesc rxDmaDesc[RA6_ETH_RX_BUFFER_COUNT]
   __attribute__((aligned(32)));

#endif

//Current transmit descriptor
static uint_t txIndex;
//Current receive descriptor
static uint_t rxIndex;


/**
 * @brief RA6 Ethernet MAC driver
 **/

const NicDriver ra6EthDriver =
{
   NIC_TYPE_ETHERNET,
   ETH_MTU,
   ra6EthInit,
   ra6EthTick,
   ra6EthEnableIrq,
   ra6EthDisableIrq,
   ra6EthEventHandler,
   ra6EthSendPacket,
   ra6EthUpdateMacAddrFilter,
   ra6EthUpdateMacConfig,
   ra6EthWritePhyReg,
   ra6EthReadPhyReg,
   TRUE,
   TRUE,
   TRUE,
   TRUE
};


/**
 * @brief RA6 Ethernet MAC initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t ra6EthInit(NetInterface *interface)
{
   error_t error;

   //Debug message
   TRACE_INFO("Initializing RA6 Ethernet MAC...\r\n");

   //Save underlying network interface
   nicDriverInterface = interface;

   //Disable protection
   R_SYSTEM->PRCR = 0xA50B;
   //Cancel EDMAC0 module stop state
   R_MSTP->MSTPCRB &= ~R_MSTP_MSTPCRB_MSTPB15_Msk;
   //Enable protection
   R_SYSTEM->PRCR = 0xA500;

   //GPIO configuration
   ra6EthInitGpio(interface);

   //Reset EDMAC0 module
   R_ETHERC_EDMAC->EDMR |= R_ETHERC_EDMAC_EDMR_SWR_Msk;
   sleep(10);

   //Valid Ethernet PHY or switch driver?
   if(interface->phyDriver != NULL)
   {
      //Ethernet PHY initialization
      error = interface->phyDriver->init(interface);
   }
   else if(interface->switchDriver != NULL)
   {
      //Ethernet switch initialization
      error = interface->switchDriver->init(interface);
   }
   else
   {
      //The interface is not properly configured
      error = ERROR_FAILURE;
   }

   //Any error to report?
   if(error)
   {
      return error;
   }

   //Initialize DMA descriptor lists
   ra6EthInitDmaDesc(interface);

   //Maximum frame length that can be accepted
   R_ETHERC0->RFLR = RA6_ETH_RX_BUFFER_SIZE;
   //Set default inter packet gap (96-bit time)
   R_ETHERC0->IPGR = 0x14;

   //Set the upper 32 bits of the MAC address
   R_ETHERC0->MAHR = (interface->macAddr.b[0] << 24) | (interface->macAddr.b[1] << 16) |
      (interface->macAddr.b[2] << 8) | interface->macAddr.b[3];

   //Set the lower 16 bits of the MAC address
   R_ETHERC0->MALR = (interface->macAddr.b[4] << 8) | interface->macAddr.b[5];

   //Select little endian mode and set descriptor length (16 bytes)
   R_ETHERC_EDMAC->EDMR = R_ETHERC_EDMAC_EDMR_DE_Msk |
      (0 << R_ETHERC_EDMAC_EDMR_DL_Pos);

   //Use store and forward mode
   R_ETHERC_EDMAC->TFTR = 0;

   //Set transmit and receive FIFO size (2048 bytes)
   R_ETHERC_EDMAC->FDR = (7 << R_ETHERC_EDMAC_FDR_TFD_Pos) |
      (7 << R_ETHERC_EDMAC_FDR_RFD_Pos);

   //Enable continuous reception of multiple frames
   R_ETHERC_EDMAC->RMCR |= R_ETHERC_EDMAC_RMCR_RNR_Msk;

   //Select write-back complete interrupt mode
   R_ETHERC_EDMAC->TRIMD &= ~R_ETHERC_EDMAC_TRIMD_TIM_Msk;
   //Enable transmit Interrupts
   R_ETHERC_EDMAC->TRIMD |= R_ETHERC_EDMAC_TRIMD_TIS_Msk;

   //Enable the desired EDMAC interrupts
   R_ETHERC_EDMAC->EESIPR = R_ETHERC_EDMAC_EESIPR_TWBIP_Msk |
      R_ETHERC_EDMAC_EESIPR_FRIP_Msk;

   //Set priority grouping (4 bits for pre-emption priority, no bits for subpriority)
   NVIC_SetPriorityGrouping(RA6_ETH_IRQ_PRIORITY_GROUPING);

   //Configure EDMAC interrupt priority
   NVIC_SetPriority(EDMAC0_EINT_IRQn, NVIC_EncodePriority(RA6_ETH_IRQ_PRIORITY_GROUPING,
      RA6_ETH_IRQ_GROUP_PRIORITY, RA6_ETH_IRQ_SUB_PRIORITY));

   //Enable transmission and reception
   R_ETHERC0->ECMR |= R_ETHERC0_ECMR_TE_Msk | R_ETHERC0_ECMR_RE_Msk;

   //Instruct the DMA to poll the receive descriptor list
   R_ETHERC_EDMAC->EDRRR = R_ETHERC_EDMAC_EDRRR_RR_Msk;

   //Accept any packets from the upper layer
   osSetEvent(&interface->nicTxEvent);

   //Successful initialization
   return NO_ERROR;
}


//EK-RA6M3 or M13-RA6M3-EK evaluation board?
#if defined(USE_EK_RA6M3) || defined(USE_M13_RA6M3_EK)

/**
 * @brief GPIO configuration
 * @param[in] interface Underlying network interface
 **/

void ra6EthInitGpio(NetInterface *interface)
{
//EK-RA6M3 evaluation board?
#if defined(USE_EK_RA6M3)
   //Disable protection
   R_SYSTEM->PRCR = 0xA50B;
   //Disable VBATT channel 0 input (P4_2)
   R_SYSTEM->VBTICTLR &= ~R_SYSTEM_VBTICTLR_VCH0INEN_Msk;
   //Enable protection
   R_SYSTEM->PRCR = 0xA500;

   //Unlock PFS registers
   R_PMISC->PWPR &= ~R_PMISC_PWPR_B0WI_Msk;
   R_PMISC->PWPR |= R_PMISC_PWPR_PFSWE_Msk;

   //Select RMII interface mode
   R_PMISC->PFENET &= ~R_PMISC_PFENET_PHYMODE0_Msk;

   //Configure ET0_MDC (P4_1)
   R_PFS->PORT[4].PIN[1].PmnPFS = (23 << R_PFS_PORT_PIN_PmnPFS_PSEL_Pos) |
      R_PFS_PORT_PIN_PmnPFS_PMR_Msk | (1 << R_PFS_PORT_PIN_PmnPFS_DSCR_Pos);

   //Configure ET0_MDIO (P4_2)
   R_PFS->PORT[4].PIN[2].PmnPFS = (23 << R_PFS_PORT_PIN_PmnPFS_PSEL_Pos) |
      R_PFS_PORT_PIN_PmnPFS_PMR_Msk | (1 << R_PFS_PORT_PIN_PmnPFS_DSCR_Pos);

   //Configure RMII0_TXD_EN_B (P4_5)
   R_PFS->PORT[4].PIN[5].PmnPFS = (23 << R_PFS_PORT_PIN_PmnPFS_PSEL_Pos) |
      R_PFS_PORT_PIN_PmnPFS_PMR_Msk | (3 << R_PFS_PORT_PIN_PmnPFS_DSCR_Pos);

   //Configure RMII0_TXD1_B (P4_6)
   R_PFS->PORT[4].PIN[6].PmnPFS = (23 << R_PFS_PORT_PIN_PmnPFS_PSEL_Pos) |
      R_PFS_PORT_PIN_PmnPFS_PMR_Msk | (3 << R_PFS_PORT_PIN_PmnPFS_DSCR_Pos);

   //Configure RMII0_TXD0_B (P7_0)
   R_PFS->PORT[7].PIN[0].PmnPFS = (23 << R_PFS_PORT_PIN_PmnPFS_PSEL_Pos) |
      R_PFS_PORT_PIN_PmnPFS_PMR_Msk | (3 << R_PFS_PORT_PIN_PmnPFS_DSCR_Pos);

   //Configure REF50CK0_B (P7_1)
   R_PFS->PORT[7].PIN[1].PmnPFS = (23 << R_PFS_PORT_PIN_PmnPFS_PSEL_Pos) |
      R_PFS_PORT_PIN_PmnPFS_PMR_Msk | (3 << R_PFS_PORT_PIN_PmnPFS_DSCR_Pos);

   //Configure RMII0_RXD0_B (P7_2)
   R_PFS->PORT[7].PIN[2].PmnPFS = (23 << R_PFS_PORT_PIN_PmnPFS_PSEL_Pos) |
      R_PFS_PORT_PIN_PmnPFS_PMR_Msk | (3 << R_PFS_PORT_PIN_PmnPFS_DSCR_Pos);

   //Configure RMII0_RXD1_B (P7_3)
   R_PFS->PORT[7].PIN[3].PmnPFS = (23 << R_PFS_PORT_PIN_PmnPFS_PSEL_Pos) |
      R_PFS_PORT_PIN_PmnPFS_PMR_Msk | (3 << R_PFS_PORT_PIN_PmnPFS_DSCR_Pos);

   //Configure RMII0_RX_ER_B (P7_4)
   R_PFS->PORT[7].PIN[4].PmnPFS = (23 << R_PFS_PORT_PIN_PmnPFS_PSEL_Pos) |
      R_PFS_PORT_PIN_PmnPFS_PMR_Msk | (3 << R_PFS_PORT_PIN_PmnPFS_DSCR_Pos);

   //Configure RMII0_CRS_DV_B (P7_5)
   R_PFS->PORT[7].PIN[5].PmnPFS = (23 << R_PFS_PORT_PIN_PmnPFS_PSEL_Pos) |
      R_PFS_PORT_PIN_PmnPFS_PMR_Msk | (3 << R_PFS_PORT_PIN_PmnPFS_DSCR_Pos);

   //Lock PFS registers
   R_PMISC->PWPR &= ~R_PMISC_PWPR_PFSWE_Msk;
   R_PMISC->PWPR |= R_PMISC_PWPR_B0WI_Msk;

//M13-RA6M3-EK evaluation board?
#elif defined(USE_M13_RA6M3_EK)
   //Disable protection
   R_SYSTEM->PRCR = 0xA50B;
   //Disable VBATT channel 0 input (P4_2)
   R_SYSTEM->VBTICTLR &= ~R_SYSTEM_VBTICTLR_VCH0INEN_Msk;
   //Enable protection
   R_SYSTEM->PRCR = 0xA500;

   //Unlock PFS registers
   R_PMISC->PWPR &= ~R_PMISC_PWPR_B0WI_Msk;
   R_PMISC->PWPR |= R_PMISC_PWPR_PFSWE_Msk;

   //Select RMII interface mode
   R_PMISC->PFENET &= ~R_PMISC_PFENET_PHYMODE0_Msk;

   //Configure ET0_MDC (P4_1)
   R_PFS->PORT[4].PIN[1].PmnPFS = (23 << R_PFS_PORT_PIN_PmnPFS_PSEL_Pos) |
      R_PFS_PORT_PIN_PmnPFS_PMR_Msk | (1 << R_PFS_PORT_PIN_PmnPFS_DSCR_Pos);

   //Configure ET0_MDIO (P4_2)
   R_PFS->PORT[4].PIN[2].PmnPFS = (23 << R_PFS_PORT_PIN_PmnPFS_PSEL_Pos) |
      R_PFS_PORT_PIN_PmnPFS_PMR_Msk | (1 << R_PFS_PORT_PIN_PmnPFS_DSCR_Pos);

   //Configure RMII0_CRS_DV_A (P4_8)
   R_PFS->PORT[4].PIN[8].PmnPFS = (23 << R_PFS_PORT_PIN_PmnPFS_PSEL_Pos) |
      R_PFS_PORT_PIN_PmnPFS_PMR_Msk | (3 << R_PFS_PORT_PIN_PmnPFS_DSCR_Pos);

   //Configure RMII0_RXD1_A (P4_10)
   R_PFS->PORT[4].PIN[10].PmnPFS = (23 << R_PFS_PORT_PIN_PmnPFS_PSEL_Pos) |
      R_PFS_PORT_PIN_PmnPFS_PMR_Msk | (3 << R_PFS_PORT_PIN_PmnPFS_DSCR_Pos);

   //Configure RMII0_RXD0_A (P4_11)
   R_PFS->PORT[4].PIN[11].PmnPFS = (23 << R_PFS_PORT_PIN_PmnPFS_PSEL_Pos) |
      R_PFS_PORT_PIN_PmnPFS_PMR_Msk | (3 << R_PFS_PORT_PIN_PmnPFS_DSCR_Pos);

   //Configure REF50CK0_A (P4_12)
   R_PFS->PORT[4].PIN[12].PmnPFS = (23 << R_PFS_PORT_PIN_PmnPFS_PSEL_Pos) |
      R_PFS_PORT_PIN_PmnPFS_PMR_Msk | (3 << R_PFS_PORT_PIN_PmnPFS_DSCR_Pos);

   //Configure RMII0_TXD0_A (P4_13)
   R_PFS->PORT[4].PIN[13].PmnPFS = (23 << R_PFS_PORT_PIN_PmnPFS_PSEL_Pos) |
      R_PFS_PORT_PIN_PmnPFS_PMR_Msk | (3 << R_PFS_PORT_PIN_PmnPFS_DSCR_Pos);

   //Configure RMII0_TXD1_A (P4_14)
   R_PFS->PORT[4].PIN[14].PmnPFS = (23 << R_PFS_PORT_PIN_PmnPFS_PSEL_Pos) |
      R_PFS_PORT_PIN_PmnPFS_PMR_Msk | (3 << R_PFS_PORT_PIN_PmnPFS_DSCR_Pos);

   //Configure RMII0_TXD_EN_A (P4_15)
   R_PFS->PORT[4].PIN[15].PmnPFS = (23 << R_PFS_PORT_PIN_PmnPFS_PSEL_Pos) |
      R_PFS_PORT_PIN_PmnPFS_PMR_Msk | (3 << R_PFS_PORT_PIN_PmnPFS_DSCR_Pos);

   //Lock PFS registers
   R_PMISC->PWPR &= ~R_PMISC_PWPR_PFSWE_Msk;
   R_PMISC->PWPR |= R_PMISC_PWPR_B0WI_Msk;
#endif
}

#endif


/**
 * @brief Initialize DMA descriptor lists
 * @param[in] interface Underlying network interface
 **/

void ra6EthInitDmaDesc(NetInterface *interface)
{
   uint_t i;

   //Initialize TX descriptors
   for(i = 0; i < RA6_ETH_TX_BUFFER_COUNT; i++)
   {
      //The descriptor is initially owned by the application
      txDmaDesc[i].td0 = 0;
      //Transmit buffer length
      txDmaDesc[i].td1 = 0;
      //Transmit buffer address
      txDmaDesc[i].td2 = (uint32_t) txBuffer[i];
      //Clear padding field
      txDmaDesc[i].padding = 0;
   }

   //Mark the last descriptor entry with the TDLE flag
   txDmaDesc[i - 1].td0 |= EDMAC_TD0_TDLE;
   //Initialize TX descriptor index
   txIndex = 0;

   //Initialize RX descriptors
   for(i = 0; i < RA6_ETH_RX_BUFFER_COUNT; i++)
   {
      //The descriptor is initially owned by the DMA
      rxDmaDesc[i].rd0 = EDMAC_RD0_RACT;
      //Receive buffer length
      rxDmaDesc[i].rd1 = (RA6_ETH_RX_BUFFER_SIZE << 16) & EDMAC_RD1_RBL;
      //Receive buffer address
      rxDmaDesc[i].rd2 = (uint32_t) rxBuffer[i];
      //Clear padding field
      rxDmaDesc[i].padding = 0;
   }

   //Mark the last descriptor entry with the RDLE flag
   rxDmaDesc[i - 1].rd0 |= EDMAC_RD0_RDLE;
   //Initialize RX descriptor index
   rxIndex = 0;

   //Start address of the TX descriptor list
   R_ETHERC_EDMAC->TDLAR = (uint32_t) txDmaDesc;
   //Start address of the RX descriptor list
   R_ETHERC_EDMAC->RDLAR = (uint32_t) rxDmaDesc;
}


/**
 * @brief RA6 Ethernet MAC timer handler
 *
 * This routine is periodically called by the TCP/IP stack to handle periodic
 * operations such as polling the link state
 *
 * @param[in] interface Underlying network interface
 **/

void ra6EthTick(NetInterface *interface)
{
   //Valid Ethernet PHY or switch driver?
   if(interface->phyDriver != NULL)
   {
      //Handle periodic operations
      interface->phyDriver->tick(interface);
   }
   else if(interface->switchDriver != NULL)
   {
      //Handle periodic operations
      interface->switchDriver->tick(interface);
   }
   else
   {
      //Just for sanity
   }
}


/**
 * @brief Enable interrupts
 * @param[in] interface Underlying network interface
 **/

void ra6EthEnableIrq(NetInterface *interface)
{
   //Enable Ethernet MAC interrupts
   NVIC_EnableIRQ(EDMAC0_EINT_IRQn);

   //Valid Ethernet PHY or switch driver?
   if(interface->phyDriver != NULL)
   {
      //Enable Ethernet PHY interrupts
      interface->phyDriver->enableIrq(interface);
   }
   else if(interface->switchDriver != NULL)
   {
      //Enable Ethernet switch interrupts
      interface->switchDriver->enableIrq(interface);
   }
   else
   {
      //Just for sanity
   }
}


/**
 * @brief Disable interrupts
 * @param[in] interface Underlying network interface
 **/

void ra6EthDisableIrq(NetInterface *interface)
{
   //Disable Ethernet MAC interrupts
   NVIC_DisableIRQ(EDMAC0_EINT_IRQn);

   //Valid Ethernet PHY or switch driver?
   if(interface->phyDriver != NULL)
   {
      //Disable Ethernet PHY interrupts
      interface->phyDriver->disableIrq(interface);
   }
   else if(interface->switchDriver != NULL)
   {
      //Disable Ethernet switch interrupts
      interface->switchDriver->disableIrq(interface);
   }
   else
   {
      //Just for sanity
   }
}


/**
 * @brief RA6 Ethernet MAC interrupt service routine
 **/

void EDMAC0_EINT_IRQHandler(void)
{
   bool_t flag;
   uint32_t status;

   //Interrupt service routine prologue
   osEnterIsr();

   //This flag will be set if a higher priority task must be woken
   flag = FALSE;

   //Read interrupt status register
   status = R_ETHERC_EDMAC->EESR;

   //Packet transmitted?
   if((status & R_ETHERC_EDMAC_EESR_TWB_Msk) != 0)
   {
      //Clear TWB interrupt flag
      R_ETHERC_EDMAC->EESR = R_ETHERC_EDMAC_EESR_TWB_Msk;

      //Check whether the TX buffer is available for writing
      if((txDmaDesc[txIndex].td0 & EDMAC_TD0_TACT) == 0)
      {
         //Notify the TCP/IP stack that the transmitter is ready to send
         flag |= osSetEventFromIsr(&nicDriverInterface->nicTxEvent);
      }
   }

   //Packet received?
   if((status & R_ETHERC_EDMAC_EESR_FR_Msk) != 0)
   {
      //Disable FR interrupts
      R_ETHERC_EDMAC->EESIPR &= ~R_ETHERC_EDMAC_EESIPR_FRIP_Msk;

      //Set event flag
      nicDriverInterface->nicEvent = TRUE;
      //Notify the TCP/IP stack of the event
      flag |= osSetEventFromIsr(&netEvent);
   }

   //Clear IR flag
   R_ICU->IELSR[EDMAC0_EINT_IRQn] &= ~R_ICU_IELSR_IR_Msk;

   //Interrupt service routine epilogue
   osExitIsr(flag);
}


/**
 * @brief RA6 Ethernet MAC event handler
 * @param[in] interface Underlying network interface
 **/

void ra6EthEventHandler(NetInterface *interface)
{
   error_t error;

   //Packet received?
   if((R_ETHERC_EDMAC->EESR & R_ETHERC_EDMAC_EESR_FR_Msk) != 0)
   {
      //Clear FR interrupt flag
      R_ETHERC_EDMAC->EESR = R_ETHERC_EDMAC_EESR_FR_Msk;

      //Process all pending packets
      do
      {
         //Read incoming packet
         error = ra6EthReceivePacket(interface);

         //No more data in the receive buffer?
      } while(error != ERROR_BUFFER_EMPTY);
   }

   //Re-enable EDMAC interrupts
   R_ETHERC_EDMAC->EESIPR = R_ETHERC_EDMAC_EESIPR_TWBIP_Msk |
      R_ETHERC_EDMAC_EESIPR_FRIP_Msk;
}


/**
 * @brief Send a packet
 * @param[in] interface Underlying network interface
 * @param[in] buffer Multi-part buffer containing the data to send
 * @param[in] offset Offset to the first data byte
 * @param[in] ancillary Additional options passed to the stack along with
 *   the packet
 * @return Error code
 **/

error_t ra6EthSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary)
{
   //Retrieve the length of the packet
   size_t length = netBufferGetLength(buffer) - offset;

   //Check the frame length
   if(length > RA6_ETH_TX_BUFFER_SIZE)
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
      //Report an error
      return ERROR_INVALID_LENGTH;
   }

   //Make sure the current buffer is available for writing
   if((txDmaDesc[txIndex].td0 & EDMAC_TD0_TACT) != 0)
   {
      return ERROR_FAILURE;
   }

   //Copy user data to the transmit buffer
   netBufferRead(txBuffer[txIndex], buffer, offset, length);

   //Write the number of bytes to send
   txDmaDesc[txIndex].td1 = (length << 16) & EDMAC_TD1_TBL;

   //Check current index
   if(txIndex < (RA6_ETH_TX_BUFFER_COUNT - 1))
   {
      //Give the ownership of the descriptor to the DMA engine
      txDmaDesc[txIndex].td0 = EDMAC_TD0_TACT | EDMAC_TD0_TFP_SOF |
         EDMAC_TD0_TFP_EOF | EDMAC_TD0_TWBI;

      //Point to the next descriptor
      txIndex++;
   }
   else
   {
      //Give the ownership of the descriptor to the DMA engine
      txDmaDesc[txIndex].td0 = EDMAC_TD0_TACT | EDMAC_TD0_TDLE |
         EDMAC_TD0_TFP_SOF | EDMAC_TD0_TFP_EOF | EDMAC_TD0_TWBI;

      //Wrap around
      txIndex = 0;
   }

   //Instruct the DMA to poll the transmit descriptor list
   R_ETHERC_EDMAC->EDTRR = R_ETHERC_EDMAC_EDTRR_TR_Msk;

   //Check whether the next buffer is available for writing
   if((txDmaDesc[txIndex].td0 & EDMAC_TD0_TACT) == 0)
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
   }

   //Successful write operation
   return NO_ERROR;
}


/**
 * @brief Receive a packet
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t ra6EthReceivePacket(NetInterface *interface)
{
   error_t error;
   size_t n;
   NetRxAncillary ancillary;

   //Current buffer available for reading?
   if((rxDmaDesc[rxIndex].rd0 & EDMAC_RD0_RACT) == 0)
   {
      //SOF and EOF flags should be set
      if((rxDmaDesc[rxIndex].rd0 & EDMAC_RD0_RFP_SOF) != 0 &&
         (rxDmaDesc[rxIndex].rd0 & EDMAC_RD0_RFP_EOF) != 0)
      {
         //Make sure no error occurred
         if((rxDmaDesc[rxIndex].rd0 & (EDMAC_RD0_RFS_MASK & ~EDMAC_RD0_RFS_RMAF)) == 0)
         {
            //Retrieve the length of the frame
            n = rxDmaDesc[rxIndex].rd1 & EDMAC_RD1_RFL;
            //Limit the number of data to read
            n = MIN(n, RA6_ETH_RX_BUFFER_SIZE);

            //Additional options can be passed to the stack along with the packet
            ancillary = NET_DEFAULT_RX_ANCILLARY;

            //Pass the packet to the upper layer
            nicProcessPacket(interface, rxBuffer[rxIndex], n, &ancillary);

            //Valid packet received
            error = NO_ERROR;
         }
         else
         {
            //The received packet contains an error
            error = ERROR_INVALID_PACKET;
         }
      }
      else
      {
         //The packet is not valid
         error = ERROR_INVALID_PACKET;
      }

      //Check current index
      if(rxIndex < (RA6_ETH_RX_BUFFER_COUNT - 1))
      {
         //Give the ownership of the descriptor back to the DMA
         rxDmaDesc[rxIndex].rd0 = EDMAC_RD0_RACT;
         //Point to the next descriptor
         rxIndex++;
      }
      else
      {
         //Give the ownership of the descriptor back to the DMA
         rxDmaDesc[rxIndex].rd0 = EDMAC_RD0_RACT | EDMAC_RD0_RDLE;
         //Wrap around
         rxIndex = 0;
      }

      //Instruct the DMA to poll the receive descriptor list
      R_ETHERC_EDMAC->EDRRR = R_ETHERC_EDMAC_EDRRR_RR_Msk;
   }
   else
   {
      //No more data in the receive buffer
      error = ERROR_BUFFER_EMPTY;
   }

   //Return status code
   return error;
}


/**
 * @brief Configure MAC address filtering
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t ra6EthUpdateMacAddrFilter(NetInterface *interface)
{
   uint_t i;
   bool_t acceptMulticast;

   //Debug message
   TRACE_DEBUG("Updating MAC filter...\r\n");

   //Set the upper 32 bits of the MAC address
   R_ETHERC0->MAHR = (interface->macAddr.b[0] << 24) | (interface->macAddr.b[1] << 16) |
      (interface->macAddr.b[2] << 8) | interface->macAddr.b[3];

   //Set the lower 16 bits of the MAC address
   R_ETHERC0->MALR = (interface->macAddr.b[4] << 8) | interface->macAddr.b[5];

   //This flag will be set if multicast addresses should be accepted
   acceptMulticast = FALSE;

   //The MAC address filter contains the list of MAC addresses to accept
   //when receiving an Ethernet frame
   for(i = 0; i < MAC_ADDR_FILTER_SIZE; i++)
   {
      //Valid entry?
      if(interface->macAddrFilter[i].refCount > 0)
      {
         //Accept multicast addresses
         acceptMulticast = TRUE;
         //We are done
         break;
      }
   }

   //Enable the reception of multicast frames if necessary
   if(acceptMulticast)
   {
      R_ETHERC_EDMAC->EESR |= R_ETHERC_EDMAC_EESR_RMAF_Msk;
   }
   else
   {
      R_ETHERC_EDMAC->EESR &= ~R_ETHERC_EDMAC_EESR_RMAF_Msk;
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Adjust MAC configuration parameters for proper operation
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t ra6EthUpdateMacConfig(NetInterface *interface)
{
   uint32_t mode;

   //Read ETHERC mode register
   mode = R_ETHERC0->ECMR;

   //10BASE-T or 100BASE-TX operation mode?
   if(interface->linkSpeed == NIC_LINK_SPEED_100MBPS)
   {
      mode |= R_ETHERC0_ECMR_RTM_Msk;
   }
   else
   {
      mode &= ~R_ETHERC0_ECMR_RTM_Msk;
   }

   //Half-duplex or full-duplex mode?
   if(interface->duplexMode == NIC_FULL_DUPLEX_MODE)
   {
      mode |= R_ETHERC0_ECMR_DM_Msk;
   }
   else
   {
      mode &= ~R_ETHERC0_ECMR_DM_Msk;
   }

   //Update ETHERC mode register
   R_ETHERC0->ECMR = mode;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Write PHY register
 * @param[in] opcode Access type (2 bits)
 * @param[in] phyAddr PHY address (5 bits)
 * @param[in] regAddr Register address (5 bits)
 * @param[in] data Register value
 **/

void ra6EthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data)
{
   //Synchronization pattern
   ra6EthWriteSmi(SMI_SYNC, 32);
   //Start of frame
   ra6EthWriteSmi(SMI_START, 2);
   //Set up a write operation
   ra6EthWriteSmi(opcode, 2);
   //Write PHY address
   ra6EthWriteSmi(phyAddr, 5);
   //Write register address
   ra6EthWriteSmi(regAddr, 5);
   //Turnaround
   ra6EthWriteSmi(SMI_TA, 2);
   //Write register value
   ra6EthWriteSmi(data, 16);
   //Release MDIO
   ra6EthReadSmi(1);
}


/**
 * @brief Read PHY register
 * @param[in] opcode Access type (2 bits)
 * @param[in] phyAddr PHY address (5 bits)
 * @param[in] regAddr Register address (5 bits)
 * @return Register value
 **/

uint16_t ra6EthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr)
{
   uint16_t data;

   //Synchronization pattern
   ra6EthWriteSmi(SMI_SYNC, 32);
   //Start of frame
   ra6EthWriteSmi(SMI_START, 2);
   //Set up a read operation
   ra6EthWriteSmi(opcode, 2);
   //Write PHY address
   ra6EthWriteSmi(phyAddr, 5);
   //Write register address
   ra6EthWriteSmi(regAddr, 5);
   //Turnaround to avoid contention
   ra6EthReadSmi(1);
   //Read register value
   data = ra6EthReadSmi(16);
   //Force the PHY to release the MDIO pin
   ra6EthReadSmi(1);

   //Return PHY register contents
   return data;
}


/**
 * @brief SMI write operation
 * @param[in] data Raw data to be written
 * @param[in] length Number of bits to be written
 **/

void ra6EthWriteSmi(uint32_t data, uint_t length)
{
   //Skip the most significant bits since they are meaningless
   data <<= 32 - length;

   //Configure MDIO as an output
   R_ETHERC0->PIR |= R_ETHERC0_PIR_MMD_Msk;

   //Write the specified number of bits
   while(length--)
   {
      //Write MDIO
      if((data & 0x80000000) != 0)
      {
         R_ETHERC0->PIR |= R_ETHERC0_PIR_MDO_Msk;
      }
      else
      {
         R_ETHERC0->PIR &= ~R_ETHERC0_PIR_MDO_Msk;
      }

      //Assert MDC
      usleep(1);
      R_ETHERC0->PIR |= R_ETHERC0_PIR_MDC_Msk;
      //Deassert MDC
      usleep(1);
      R_ETHERC0->PIR &= ~R_ETHERC0_PIR_MDC_Msk;

      //Rotate data
      data <<= 1;
   }
}


/**
 * @brief SMI read operation
 * @param[in] length Number of bits to be read
 * @return Data resulting from the MDIO read operation
 **/

uint32_t ra6EthReadSmi(uint_t length)
{
   uint32_t data = 0;

   //Configure MDIO as an input
   R_ETHERC0->PIR &= ~R_ETHERC0_PIR_MMD_Msk;

   //Read the specified number of bits
   while(length--)
   {
      //Rotate data
      data <<= 1;

      //Assert MDC
      R_ETHERC0->PIR |= R_ETHERC0_PIR_MDC_Msk;
      usleep(1);
      //Deassert MDC
      R_ETHERC0->PIR &= ~R_ETHERC0_PIR_MDC_Msk;
      usleep(1);

      //Check MDIO state
      if((R_ETHERC0->PIR & R_ETHERC0_PIR_MDI_Msk) != 0)
      {
         data |= 0x01;
      }
   }

   //Return the received data
   return data;
}
