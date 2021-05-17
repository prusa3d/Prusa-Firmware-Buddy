/**
 * @file rx63n_eth_driver.c
 * @brief Renesas RX63N Ethernet MAC driver
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
#include <iorx63n.h>
#include <intrinsics.h>
#include "core/net.h"
#include "drivers/mac/rx63n_eth_driver.h"
#include "debug.h"

//Underlying network interface
static NetInterface *nicDriverInterface;

//IAR EWRX compiler?
#if defined(__ICCRX__)

//Transmit buffer
#pragma data_alignment = 32
static uint8_t txBuffer[RX63N_ETH_TX_BUFFER_COUNT][RX63N_ETH_TX_BUFFER_SIZE];
//Receive buffer
#pragma data_alignment = 32
static uint8_t rxBuffer[RX63N_ETH_RX_BUFFER_COUNT][RX63N_ETH_RX_BUFFER_SIZE];
//Transmit DMA descriptors
#pragma data_alignment = 32
static Rx63nTxDmaDesc txDmaDesc[RX63N_ETH_TX_BUFFER_COUNT];
//Receive DMA descriptors
#pragma data_alignment = 32
static Rx63nRxDmaDesc rxDmaDesc[RX63N_ETH_RX_BUFFER_COUNT];

//GCC compiler?
#else

//Transmit buffer
static uint8_t txBuffer[RX63N_ETH_TX_BUFFER_COUNT][RX63N_ETH_TX_BUFFER_SIZE]
   __attribute__((aligned(32)));
//Receive buffer
static uint8_t rxBuffer[RX63N_ETH_RX_BUFFER_COUNT][RX63N_ETH_RX_BUFFER_SIZE]
   __attribute__((aligned(32)));
//Transmit DMA descriptors
static Rx63nTxDmaDesc txDmaDesc[RX63N_ETH_TX_BUFFER_COUNT]
   __attribute__((aligned(32)));
//Receive DMA descriptors
static Rx63nRxDmaDesc rxDmaDesc[RX63N_ETH_RX_BUFFER_COUNT]
   __attribute__((aligned(32)));

#endif

//Current transmit descriptor
static uint_t txIndex;
//Current receive descriptor
static uint_t rxIndex;


/**
 * @brief RX63N Ethernet MAC driver
 **/

const NicDriver rx63nEthDriver =
{
   NIC_TYPE_ETHERNET,
   ETH_MTU,
   rx63nEthInit,
   rx63nEthTick,
   rx63nEthEnableIrq,
   rx63nEthDisableIrq,
   rx63nEthEventHandler,
   rx63nEthSendPacket,
   rx63nEthUpdateMacAddrFilter,
   rx63nEthUpdateMacConfig,
   rx63nEthWritePhyReg,
   rx63nEthReadPhyReg,
   TRUE,
   TRUE,
   TRUE,
   TRUE
};


/**
 * @brief RX63N Ethernet MAC initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t rx63nEthInit(NetInterface *interface)
{
   error_t error;

   //Debug message
   TRACE_INFO("Initializing RX63N Ethernet MAC...\r\n");

   //Save underlying network interface
   nicDriverInterface = interface;

   //Disable protection
   SYSTEM.PRCR.WORD = 0xA50B;
   //Cancel EDMAC module stop state
   MSTP(EDMAC) = 0;
   //Enable protection
   SYSTEM.PRCR.WORD = 0xA500;

   //GPIO configuration
   rx63nEthInitGpio(interface);

   //Reset EDMAC module
   EDMAC.EDMR.BIT.SWR = 1;
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
   rx63nEthInitDmaDesc(interface);

   //Maximum frame length that can be accepted
   ETHERC.RFLR.LONG = RX63N_ETH_RX_BUFFER_SIZE;
   //Set default inter packet gap (96-bit time)
   ETHERC.IPGR.LONG = 0x14;

   //Set the upper 32 bits of the MAC address
   ETHERC.MAHR = (interface->macAddr.b[0] << 24) | (interface->macAddr.b[1] << 16) |
      (interface->macAddr.b[2] << 8) | interface->macAddr.b[3];

   //Set the lower 16 bits of the MAC address
   ETHERC.MALR.BIT.MA = (interface->macAddr.b[4] << 8) | interface->macAddr.b[5];

   //Set descriptor length (16 bytes)
   EDMAC.EDMR.BIT.DL = 0;

#ifdef _CPU_BIG_ENDIAN
   //Select big endian mode
   EDMAC.EDMR.BIT.DE = 0;
#else
   //Select little endian mode
   EDMAC.EDMR.BIT.DE = 1;
#endif

   //Use store and forward mode
   EDMAC.TFTR.BIT.TFT = 0;

   //Set transmit FIFO size (2048 bytes)
   EDMAC.FDR.BIT.TFD = 7;
   //Set receive FIFO size (2048 bytes)
   EDMAC.FDR.BIT.RFD = 7;

   //Enable continuous reception of multiple frames
   EDMAC.RMCR.BIT.RNR = 1;

   //Accept transmit interrupt notifications
   EDMAC.TRIMD.BIT.TIM = 0;
   EDMAC.TRIMD.BIT.TIS = 1;

   //Disable all EDMAC interrupts
   EDMAC.EESIPR.LONG = 0;
   //Enable only the desired EDMAC interrupts
   EDMAC.EESIPR.BIT.TWBIP = 1;
   EDMAC.EESIPR.BIT.FRIP = 1;

   //Configure EDMAC interrupt priority
   IPR(ETHER, EINT) = RX63N_ETH_IRQ_PRIORITY;

   //Enable transmission and reception
   ETHERC.ECMR.BIT.TE = 1;
   ETHERC.ECMR.BIT.RE = 1;

   //Instruct the DMA to poll the receive descriptor list
   EDMAC.EDRRR.BIT.RR = 1;

   //Accept any packets from the upper layer
   osSetEvent(&interface->nicTxEvent);

   //Successful initialization
   return NO_ERROR;
}


//RDK-RX63N, RSK-RX63N or RSK-RX63N-256K evaluation board?
#if defined(USE_RDK_RX63N) || defined(USE_RSK_RX63N) || \
   defined(USE_RSK_RX63N_256K)

/**
 * @brief GPIO configuration
 * @param[in] interface Underlying network interface
 **/

void rx63nEthInitGpio(NetInterface *interface)
{
   //Unlock MPC registers
   MPC.PWPR.BIT.B0WI = 0;
   MPC.PWPR.BIT.PFSWE = 1;

#if defined(USE_RDK_RX63N)
   //Select RMII interface mode
   MPC.PFENET.BIT.PHYMODE = 0;

   //Configure ET_MDIO (PA3)
   PORTA.PMR.BIT.B3 = 1;
   MPC.PA3PFS.BYTE = 0x11;

   //Configure ET_MDC (PA4)
   PORTA.PMR.BIT.B4 = 1;
   MPC.PA4PFS.BYTE = 0x11;

   //Configure ET_LINKSTA (PA5)
   PORTA.PMR.BIT.B5 = 1;
   MPC.PA5PFS.BYTE = 0x11;

   //Configure RMII_RXD1 (PB0)
   PORTB.PMR.BIT.B0 = 1;
   MPC.PB0PFS.BYTE = 0x12;

   //Configure RMII_RXD0 (PB1)
   PORTB.PMR.BIT.B1 = 1;
   MPC.PB1PFS.BYTE = 0x12;

   //Configure REF50CK (PB2)
   PORTB.PMR.BIT.B2 = 1;
   MPC.PB2PFS.BYTE = 0x12;

   //Configure RMII_RX_ER (PB3)
   PORTB.PMR.BIT.B3 = 1;
   MPC.PB3PFS.BYTE = 0x12;

   //Configure RMII_TXD_EN (PB4)
   PORTB.PMR.BIT.B4 = 1;
   MPC.PB4PFS.BYTE = 0x12;

   //Configure RMII_TXD0 (PB5)
   PORTB.PMR.BIT.B5 = 1;
   MPC.PB5PFS.BYTE = 0x12;

   //Configure RMII_TXD1 (PB6)
   PORTB.PMR.BIT.B6 = 1;
   MPC.PB6PFS.BYTE = 0x12;

   //Configure RMII_CRS_DV (PB7)
   PORTB.PMR.BIT.B7 = 1;
   MPC.PB7PFS.BYTE = 0x12;

#elif defined(USE_RSK_RX63N) || defined(USE_RSK_RX63N_256K)
   //Select MII interface mode
   MPC.PFENET.BIT.PHYMODE = 1;

   //Configure ET_MDIO (P71)
   PORT7.PMR.BIT.B1 = 1;
   MPC.P71PFS.BYTE = 0x11;

   //Configure ET_MDC (P72)
   PORT7.PMR.BIT.B2 = 1;
   MPC.P72PFS.BYTE = 0x11;

   //Configure ET_ERXD1 (P74)
   PORT7.PMR.BIT.B4 = 1;
   MPC.P74PFS.BYTE = 0x11;

   //Configure ET_ERXD0 P75)
   PORT7.PMR.BIT.B5 = 1;
   MPC.P75PFS.BYTE = 0x11;

   //Configure ET_RX_CLK (P76)
   PORT7.PMR.BIT.B6 = 1;
   MPC.P76PFS.BYTE = 0x11;

   //Configure ET_RX_ER (P77)
   PORT7.PMR.BIT.B7 = 1;
   MPC.P77PFS.BYTE = 0x11;

   //Configure ET_TX_EN (P80)
   PORT8.PMR.BIT.B0 = 1;
   MPC.P80PFS.BYTE = 0x11;

   //Configure ET_ETXD0 (P81)
   PORT8.PMR.BIT.B1 = 1;
   MPC.P81PFS.BYTE = 0x11;

   //Configure ET_ETXD1 (P82)
   PORT8.PMR.BIT.B2 = 1;
   MPC.P82PFS.BYTE = 0x11;

   //Configure ET_CRS (P83)
   PORT8.PMR.BIT.B3 = 1;
   MPC.P83PFS.BYTE = 0x11;

   //Configure ET_ERXD3 (PC0)
   PORTC.PMR.BIT.B0 = 1;
   MPC.PC0PFS.BYTE = 0x11;

   //Configure ET_ERXD2 (PC1)
   PORTC.PMR.BIT.B1 = 1;
   MPC.PC1PFS.BYTE = 0x11;

   //Configure ET_RX_DV (PC2)
   PORTC.PMR.BIT.B2 = 1;
   MPC.PC2PFS.BYTE = 0x11;

   //Configure ET_TX_ER (PC3)
   PORTC.PMR.BIT.B3 = 1;
   MPC.PC3PFS.BYTE = 0x11;

   //Configure ET_TX_CLK (PC4)
   PORTC.PMR.BIT.B4 = 1;
   MPC.PC4PFS.BYTE = 0x11;

   //Configure ET_ETXD2 (PC5)
   PORTC.PMR.BIT.B5 = 1;
   MPC.PC5PFS.BYTE = 0x11;

   //Configure ET_ETXD3 (PC6)
   PORTC.PMR.BIT.B6 = 1;
   MPC.PC6PFS.BYTE = 0x11;

   //Configure ET_COL (PC7)
   PORTC.PMR.BIT.B7 = 1;
   MPC.PC7PFS.BYTE = 0x11;
#endif

   //Lock MPC registers
   MPC.PWPR.BIT.PFSWE = 0;
   MPC.PWPR.BIT.B0WI = 0;
}

#endif


/**
 * @brief Initialize DMA descriptor lists
 * @param[in] interface Underlying network interface
 **/

void rx63nEthInitDmaDesc(NetInterface *interface)
{
   uint_t i;

   //Initialize TX descriptors
   for(i = 0; i < RX63N_ETH_TX_BUFFER_COUNT; i++)
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
   for(i = 0; i < RX63N_ETH_RX_BUFFER_COUNT; i++)
   {
      //The descriptor is initially owned by the DMA
      rxDmaDesc[i].rd0 = EDMAC_RD0_RACT;
      //Receive buffer length
      rxDmaDesc[i].rd1 = (RX63N_ETH_RX_BUFFER_SIZE << 16) & EDMAC_RD1_RBL;
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
   EDMAC.TDLAR = txDmaDesc;
   //Start address of the RX descriptor list
   EDMAC.RDLAR = rxDmaDesc;
}


/**
 * @brief RX63N Ethernet MAC timer handler
 *
 * This routine is periodically called by the TCP/IP stack to handle periodic
 * operations such as polling the link state
 *
 * @param[in] interface Underlying network interface
 **/

void rx63nEthTick(NetInterface *interface)
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

void rx63nEthEnableIrq(NetInterface *interface)
{
   //Enable Ethernet MAC interrupts
   IEN(ETHER, EINT) = 1;

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

void rx63nEthDisableIrq(NetInterface *interface)
{
   //Disable Ethernet MAC interrupts
   IEN(ETHER, EINT) = 0;

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
 * @brief RX63N Ethernet MAC interrupt service routine
 **/

#pragma vector = VECT_ETHER_EINT
__interrupt void rx63nEthIrqHandler(void)
{
   bool_t flag;
   uint32_t status;

   //Allow nested interrupts
   __enable_interrupt();

   //This flag will be set if a higher priority task must be woken
   flag = FALSE;

   //Read interrupt status register
   status = EDMAC.EESR.LONG;

   //Packet transmitted?
   if((status & EDMAC_EESR_TWB) != 0)
   {
      //Clear TWB interrupt flag
      EDMAC.EESR.LONG = EDMAC_EESR_TWB;

      //Check whether the TX buffer is available for writing
      if((txDmaDesc[txIndex].td0 & EDMAC_TD0_TACT) == 0)
      {
         //Notify the TCP/IP stack that the transmitter is ready to send
         flag |= osSetEventFromIsr(&nicDriverInterface->nicTxEvent);
      }
   }

   //Packet received?
   if((status & EDMAC_EESR_FR) != 0)
   {
      //Disable FR interrupts
      EDMAC.EESIPR.BIT.FRIP = 0;

      //Set event flag
      nicDriverInterface->nicEvent = TRUE;
      //Notify the TCP/IP stack of the event
      flag |= osSetEventFromIsr(&netEvent);
   }

   //Interrupt service routine epilogue
   osExitIsr(flag);
}


/**
 * @brief RX63N Ethernet MAC event handler
 * @param[in] interface Underlying network interface
 **/

void rx63nEthEventHandler(NetInterface *interface)
{
   error_t error;

   //Packet received?
   if((EDMAC.EESR.LONG & EDMAC_EESR_FR) != 0)
   {
      //Clear FR interrupt flag
      EDMAC.EESR.LONG = EDMAC_EESR_FR;

      //Process all pending packets
      do
      {
         //Read incoming packet
         error = rx63nEthReceivePacket(interface);

         //No more data in the receive buffer?
      } while(error != ERROR_BUFFER_EMPTY);
   }

   //Re-enable EDMAC interrupts
   EDMAC.EESIPR.BIT.TWBIP = 1;
   EDMAC.EESIPR.BIT.FRIP = 1;
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

error_t rx63nEthSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary)
{
   //Retrieve the length of the packet
   size_t length = netBufferGetLength(buffer) - offset;

   //Check the frame length
   if(length > RX63N_ETH_TX_BUFFER_SIZE)
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
   if(txIndex < (RX63N_ETH_TX_BUFFER_COUNT - 1))
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
   EDMAC.EDTRR.BIT.TR = 1;

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

error_t rx63nEthReceivePacket(NetInterface *interface)
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
            n = MIN(n, RX63N_ETH_RX_BUFFER_SIZE);

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
      if(rxIndex < (RX63N_ETH_RX_BUFFER_COUNT - 1))
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
      EDMAC.EDRRR.BIT.RR = 1;
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

error_t rx63nEthUpdateMacAddrFilter(NetInterface *interface)
{
   uint_t i;
   bool_t acceptMulticast;

   //Debug message
   TRACE_DEBUG("Updating MAC filter...\r\n");

   //Set the upper 32 bits of the MAC address
   ETHERC.MAHR = (interface->macAddr.b[0] << 24) | (interface->macAddr.b[1] << 16) |
      (interface->macAddr.b[2] << 8) | interface->macAddr.b[3];

   //Set the lower 16 bits of the MAC address
   ETHERC.MALR.BIT.MA = (interface->macAddr.b[4] << 8) | interface->macAddr.b[5];

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
      EDMAC.EESR.BIT.RMAF = 1;
   }
   else
   {
      EDMAC.EESR.BIT.RMAF = 0;
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Adjust MAC configuration parameters for proper operation
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t rx63nEthUpdateMacConfig(NetInterface *interface)
{
   //10BASE-T or 100BASE-TX operation mode?
   if(interface->linkSpeed == NIC_LINK_SPEED_100MBPS)
   {
      ETHERC.ECMR.BIT.RTM = 1;
   }
   else
   {
      ETHERC.ECMR.BIT.RTM = 0;
   }

   //Half-duplex or full-duplex mode?
   if(interface->duplexMode == NIC_FULL_DUPLEX_MODE)
   {
      ETHERC.ECMR.BIT.DM = 1;
   }
   else
   {
      ETHERC.ECMR.BIT.DM = 0;
   }

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

void rx63nEthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data)
{
   //Synchronization pattern
   rx63nEthWriteSmi(SMI_SYNC, 32);
   //Start of frame
   rx63nEthWriteSmi(SMI_START, 2);
   //Set up a write operation
   rx63nEthWriteSmi(opcode, 2);
   //Write PHY address
   rx63nEthWriteSmi(phyAddr, 5);
   //Write register address
   rx63nEthWriteSmi(regAddr, 5);
   //Turnaround
   rx63nEthWriteSmi(SMI_TA, 2);
   //Write register value
   rx63nEthWriteSmi(data, 16);
   //Release MDIO
   rx63nEthReadSmi(1);
}


/**
 * @brief Read PHY register
 * @param[in] opcode Access type (2 bits)
 * @param[in] phyAddr PHY address (5 bits)
 * @param[in] regAddr Register address (5 bits)
 * @return Register value
 **/

uint16_t rx63nEthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr)
{
   uint16_t data;

   //Synchronization pattern
   rx63nEthWriteSmi(SMI_SYNC, 32);
   //Start of frame
   rx63nEthWriteSmi(SMI_START, 2);
   //Set up a read operation
   rx63nEthWriteSmi(opcode, 2);
   //Write PHY address
   rx63nEthWriteSmi(phyAddr, 5);
   //Write register address
   rx63nEthWriteSmi(regAddr, 5);
   //Turnaround to avoid contention
   rx63nEthReadSmi(1);
   //Read register value
   data = rx63nEthReadSmi(16);
   //Force the PHY to release the MDIO pin
   rx63nEthReadSmi(1);

   //Return PHY register contents
   return data;
}


/**
 * @brief SMI write operation
 * @param[in] data Raw data to be written
 * @param[in] length Number of bits to be written
 **/

void rx63nEthWriteSmi(uint32_t data, uint_t length)
{
   //Skip the most significant bits since they are meaningless
   data <<= 32 - length;

   //Configure MDIO as an output
   ETHERC.PIR.BIT.MMD = 1;

   //Write the specified number of bits
   while(length--)
   {
      //Write MDIO
      if((data & 0x80000000) != 0)
      {
         ETHERC.PIR.BIT.MDO = 1;
      }
      else
      {
         ETHERC.PIR.BIT.MDO = 0;
      }

      //Assert MDC
      usleep(1);
      ETHERC.PIR.BIT.MDC = 1;
      //Deassert MDC
      usleep(1);
      ETHERC.PIR.BIT.MDC = 0;

      //Rotate data
      data <<= 1;
   }
}


/**
 * @brief SMI read operation
 * @param[in] length Number of bits to be read
 * @return Data resulting from the MDIO read operation
 **/

uint32_t rx63nEthReadSmi(uint_t length)
{
   uint32_t data = 0;

   //Configure MDIO as an input
   ETHERC.PIR.BIT.MMD = 0;

   //Read the specified number of bits
   while(length--)
   {
      //Rotate data
      data <<= 1;

      //Assert MDC
      ETHERC.PIR.BIT.MDC = 1;
      usleep(1);
      //Deassert MDC
      ETHERC.PIR.BIT.MDC = 0;
      usleep(1);

      //Check MDIO state
      if(ETHERC.PIR.BIT.MDI)
      {
         data |= 0x01;
      }
   }

   //Return the received data
   return data;
}
