/**
 * @file mcf5225x_eth_driver.c
 * @brief Coldfire V2 MCF5225x Ethernet MAC driver
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
#include "mcf52259.h"
#include "core/net.h"
#include "drivers/mac/mcf5225x_eth_driver.h"
#include "debug.h"

//Underlying network interface
static NetInterface *nicDriverInterface;

//TX buffer
static uint8_t txBuffer[MCF5225X_ETH_TX_BUFFER_COUNT][MCF5225X_ETH_TX_BUFFER_SIZE];
//RX buffer
static uint8_t rxBuffer[MCF5225X_ETH_RX_BUFFER_COUNT][MCF5225X_ETH_RX_BUFFER_SIZE];
//TX buffer descriptors
static Mcf5225xTxBufferDesc txBufferDesc[MCF5225X_ETH_TX_BUFFER_COUNT];
//RX buffer descriptors
static Mcf5225xRxBufferDesc rxBufferDesc[MCF5225X_ETH_RX_BUFFER_COUNT];

//TX buffer index
static uint_t txBufferIndex;
//RX buffer index
static uint_t rxBufferIndex;


/**
 * @brief MCF5225x Ethernet MAC driver
 **/

const NicDriver mcf5225xEthDriver =
{
   NIC_TYPE_ETHERNET,
   ETH_MTU,
   mcf5225xEthInit,
   mcf5225xEthTick,
   mcf5225xEthEnableIrq,
   mcf5225xEthDisableIrq,
   mcf5225xEthEventHandler,
   mcf5225xEthSendPacket,
   mcf5225xEthUpdateMacAddrFilter,
   mcf5225xEthUpdateMacConfig,
   mcf5225xEthWritePhyReg,
   mcf5225xEthReadPhyReg,
   TRUE,
   TRUE,
   TRUE,
   FALSE
};


/**
 * @brief MCF5225x Ethernet MAC initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t mcf5225xEthInit(NetInterface *interface)
{
   error_t error;
   uint_t i;
   uint32_t value;

   //Debug message
   TRACE_INFO("Initializing MCF5225x Ethernet MAC...\r\n");

   //Save underlying network interface
   nicDriverInterface = interface;

   //GPIO configuration
   mcf5225xEthInitGpio(interface);

   //Reset FEC module
   MCF_FEC_ECR = MCF_FEC_ECR_RESET;
   //Wait for the reset to complete
   while((MCF_FEC_ECR & MCF_FEC_ECR_RESET) != 0)
   {
   }

   //Receive control register
   MCF_FEC_RCR = MCF_FEC_RCR_MAX_FL(MCF5225X_ETH_RX_BUFFER_SIZE) |
      MCF_FEC_RCR_MII_MODE;

   //Transmit control register
   MCF_FEC_TCR = 0;
   //Configure MDC clock frequency
   MCF_FEC_MSCR = MCF_FEC_MSCR_MII_SPEED(19);

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

   //Set the MAC address of the station (upper 16 bits)
   value = interface->macAddr.b[5];
   value |= (interface->macAddr.b[4] << 8);
   MCF_FEC_PAUR = MCF_FEC_PAUR_PADDR2(value) | MCF_FEC_PAUR_TYPE(0x8808);

   //Set the MAC address of the station (lower 32 bits)
   value = interface->macAddr.b[3];
   value |= (interface->macAddr.b[2] << 8);
   value |= (interface->macAddr.b[1] << 16);
   value |= (interface->macAddr.b[0] << 24);
   MCF_FEC_PALR = MCF_FEC_PALR_PADDR1(value);

   //Hash table for unicast address filtering
   MCF_FEC_IALR = 0;
   MCF_FEC_IAUR = 0;
   //Hash table for multicast address filtering
   MCF_FEC_GALR = 0;
   MCF_FEC_GAUR = 0;

   //Initialize buffer descriptors
   mcf5225xEthInitBufferDesc(interface);

   //Clear any pending interrupts
   MCF_FEC_EIR = MCF_FEC_EIR_CLEAR_ALL;

   //Enable desired interrupts
   MCF_FEC_EIMR = MCF_FEC_EIMR_TXF | MCF_FEC_EIMR_TXB |
      MCF_FEC_EIMR_RXF | MCF_FEC_EIMR_RXB | MCF_FEC_EIMR_EBERR;

   //Set the priority of FEC interrupts
   for(i = 23; i <= 35; i++)
   {
      MCF_INTC0_ICR(i) = MCF_INTC_ICR_IL(MCF5225X_ETH_IRQ_LEVEL) |
         MCF_INTC_ICR_IP(MCF5225X_ETH_IRQ_PRIORITY);
   }

   //Enable Ethernet MAC
   MCF_FEC_ECR |= MCF_FEC_ECR_ETHER_EN;
   //Instruct the DMA to poll the receive descriptor list
   MCF_FEC_RDAR = MCF_FEC_RDAR_R_DES_ACTIVE;

   //Accept any packets from the upper layer
   osSetEvent(&interface->nicTxEvent);

   //Successful initialization
   return NO_ERROR;
}


//TWR-MCF5225X evaluation board?
#if defined(USE_TWR_MCF5225X)

/**
 * @brief GPIO configuration
 * @param[in] interface Underlying network interface
 **/

void mcf5225xEthInitGpio(NetInterface *interface)
{
   uint8_t temp;

   //Configure FEC_COL (PTI0), FEC_CRS (PTI1), FEC_RXCLK (PTI2), FEC_RXD0 (PTI3),
   //FEC_RXD1 (PTI4), FEC_RXD2 (PTI5), FEC_RXD3 (PTI6) and FEC_RXDV (PTI7)
   MCF_GPIO_PTIPAR |= MCF_GPIO_PTIPAR_FEC_COL_FEC_COL | MCF_GPIO_PTIPAR_FEC_CRS_FEC_CRS |
      MCF_GPIO_PTIPAR_FEC_RXCLK_FEC_RXCLK | MCF_GPIO_PTIPAR_FEC_RXD0_FEC_RXD0 |
      MCF_GPIO_PTIPAR_FEC_RXD1_FEC_RXD1 | MCF_GPIO_PTIPAR_FEC_RXD2_FEC_RXD2 |
      MCF_GPIO_PTIPAR_FEC_RXD3_FEC_RXD3 | MCF_GPIO_PTIPAR_FEC_RXDV_FEC_RXDV;

   //Configure FEC_RXER (PTJ0), FEC_TXCLK (PTJ1), FEC_TXD0 (PTJ2), FEC_TXD1 (PTJ3)
   //FEC_TXD2 (PTJ4), FEC_TXD3 (PTJ5), FEC_TXEN (PTJ6) and FEC_TXER (PTJ7)
   MCF_GPIO_PTJPAR |= MCF_GPIO_PTJPAR_FEC_RXER_FEC_RXER | MCF_GPIO_PTJPAR_FEC_TXCLK_FEC_TXCLK |
      MCF_GPIO_PTJPAR_FEC_TXD0_FEC_TXD0 | MCF_GPIO_PTJPAR_FEC_TXD1_FEC_TXD1 |
      MCF_GPIO_PTJPAR_FEC_TXD2_FEC_TXD2 | MCF_GPIO_PTJPAR_FEC_TXD3_FEC_TXD3 |
      MCF_GPIO_PTJPAR_FEC_TXEN_FEC_TXEN | MCF_GPIO_PTJPAR_FEC_TXER_FEC_TXER;

   //Configure FEC_MDIO (PNQ3)
   temp = MCF_GPIO_PNQPAR & ~MCF_GPIO_PNQPAR_PNQPAR3(3);
   MCF_GPIO_PNQPAR = temp | MCF_GPIO_PNQPAR_IRQ3_FEC_MDIO;

   //Configure FEC_MDC (PNQ5)
   temp = MCF_GPIO_PNQPAR & ~MCF_GPIO_PNQPAR_PNQPAR5(3);
   MCF_GPIO_PNQPAR = temp | MCF_GPIO_PNQPAR_IRQ5_FEC_MDC;

   //Reset PHY transceiver
   MCF_RCM_RCR |= MCF_RCM_RCR_FRCRSTOUT;
   sleep(10);
   MCF_RCM_RCR &= ~MCF_RCM_RCR_FRCRSTOUT;
   sleep(10);
}

#endif


/**
 * @brief Initialize buffer descriptors
 * @param[in] interface Underlying network interface
 **/

void mcf5225xEthInitBufferDesc(NetInterface *interface)
{
   uint_t i;

   //Initialize TX buffer descriptors
   for(i = 0; i < MCF5225X_ETH_TX_BUFFER_COUNT; i++)
   {
      //The descriptor is initially owned by the software
      txBufferDesc[i].status = 0;
      //Transmit buffer length
      txBufferDesc[i].length = 0;
      //Transmit buffer address
      txBufferDesc[i].address = (uint32_t) FEC_ALIGN16(txBuffer[i]);
   }

   //Mark the last descriptor entry with the wrap flag
   txBufferDesc[i - 1].status |= FEC_TX_BD_W;
   //Initialize TX buffer index
   txBufferIndex = 0;

   //Initialize RX buffer descriptors
   for(i = 0; i < MCF5225X_ETH_RX_BUFFER_COUNT; i++)
   {
      //The descriptor is initially owned by the DMA
      rxBufferDesc[i].status = FEC_RX_BD_E;
      //Receive buffer length
      rxBufferDesc[i].length = 0;
      //Receive buffer address
      rxBufferDesc[i].address = (uint32_t) FEC_ALIGN16(rxBuffer[i]);
   }

   //Mark the last descriptor entry with the wrap flag
   rxBufferDesc[i - 1].status |= FEC_RX_BD_W;
   //Initialize RX buffer index
   rxBufferIndex = 0;

   //Start location of the TX descriptor list
   MCF_FEC_ETSDR = (uint32_t) txBufferDesc;
   //Start location of the RX descriptor list
   MCF_FEC_ERDSR = (uint32_t) rxBufferDesc;
   //Maximum receive buffer size
   MCF_FEC_EMRBR = MCF5225X_ETH_RX_BUFFER_SIZE;
}


/**
 * @brief MCF5225x Ethernet MAC timer handler
 *
 * This routine is periodically called by the TCP/IP stack to handle periodic
 * operations such as polling the link state
 *
 * @param[in] interface Underlying network interface
 **/

void mcf5225xEthTick(NetInterface *interface)
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

void mcf5225xEthEnableIrq(NetInterface *interface)
{
   //Enable Ethernet MAC interrupts
   MCF_INTC0_IMRL &= ~(MCF_INTC_IMRL_INT_MASK23 |
      MCF_INTC_IMRL_INT_MASK24 | MCF_INTC_IMRL_INT_MASK25 |
      MCF_INTC_IMRL_INT_MASK26 | MCF_INTC_IMRL_INT_MASK27 |
      MCF_INTC_IMRL_INT_MASK28 | MCF_INTC_IMRL_INT_MASK29 |
      MCF_INTC_IMRL_INT_MASK30 | MCF_INTC_IMRL_INT_MASK31);

   MCF_INTC0_IMRH &= ~(MCF_INTC_IMRH_INT_MASK33 |
      MCF_INTC_IMRH_INT_MASK34 | MCF_INTC_IMRH_INT_MASK35);


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

void mcf5225xEthDisableIrq(NetInterface *interface)
{
   //Disable Ethernet MAC interrupts
   MCF_INTC0_IMRL |= MCF_INTC_IMRL_INT_MASK23 |
      MCF_INTC_IMRL_INT_MASK24 | MCF_INTC_IMRL_INT_MASK25 |
      MCF_INTC_IMRL_INT_MASK26 | MCF_INTC_IMRL_INT_MASK27 |
      MCF_INTC_IMRL_INT_MASK28 | MCF_INTC_IMRL_INT_MASK29 |
      MCF_INTC_IMRL_INT_MASK30 | MCF_INTC_IMRL_INT_MASK31;

   MCF_INTC0_IMRH |= MCF_INTC_IMRH_INT_MASK33 |
      MCF_INTC_IMRH_INT_MASK34 | MCF_INTC_IMRH_INT_MASK35;


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
 * @brief Ethernet MAC interrupt
 **/

__declspec(interrupt) void mcf5225xEthIrqHandler(void)
{
   bool_t flag;
   uint32_t events;

   //Interrupt service routine prologue
   osEnterIsr();

   //This flag will be set if a higher priority task must be woken
   flag = FALSE;
   //Read interrupt event register
   events = MCF_FEC_EIR;

   //Packet transmitted?
   if((events & (MCF_FEC_EIR_TXF | MCF_FEC_EIR_TXB)) != 0)
   {
      //Clear TXF and TXB interrupt flags
      MCF_FEC_EIR = MCF_FEC_EIR_TXF | MCF_FEC_EIR_TXB;

      //Check whether the TX buffer is available for writing
      if((txBufferDesc[txBufferIndex].status & FEC_TX_BD_R) == 0)
      {
         //Notify the TCP/IP stack that the transmitter is ready to send
         flag |= osSetEventFromIsr(&nicDriverInterface->nicTxEvent);
      }

      //Instruct the DMA to poll the transmit descriptor list
      MCF_FEC_TDAR = MCF_FEC_TDAR_X_DES_ACTIVE;
   }

   //Packet received?
   if((events & (MCF_FEC_EIR_RXF | MCF_FEC_EIR_RXB)) != 0)
   {
      //Disable RXF and RXB interrupts
      MCF_FEC_EIMR &= ~(MCF_FEC_EIMR_RXF | MCF_FEC_EIMR_RXB);

      //Set event flag
      nicDriverInterface->nicEvent = TRUE;
      //Notify the TCP/IP stack of the event
      flag |= osSetEventFromIsr(&netEvent);
   }

   //System bus error?
   if((events & MCF_FEC_EIR_EBERR) != 0)
   {
      //Disable EBERR interrupt
      MCF_FEC_EIMR &= ~MCF_FEC_EIMR_EBERR;

      //Set event flag
      nicDriverInterface->nicEvent = TRUE;
      //Notify the TCP/IP stack of the event
      flag |= osSetEventFromIsr(&netEvent);
   }

   //Any other event?
   if((events & (MCF_FEC_EIR_HBERR | MCF_FEC_EIR_BABR | MCF_FEC_EIR_BABT |
      MCF_FEC_EIR_GRA | MCF_FEC_EIR_MII | MCF_FEC_EIR_LC | MCF_FEC_EIR_RL |
      MCF_FEC_EIR_UN)) != 0)
   {
      //Clear interrupt flags
      MCF_FEC_EIR = MCF_FEC_EIR_HBERR | MCF_FEC_EIR_BABR | MCF_FEC_EIR_BABT |
         MCF_FEC_EIR_GRA | MCF_FEC_EIR_MII | MCF_FEC_EIR_LC | MCF_FEC_EIR_RL |
         MCF_FEC_EIR_UN;
   }

   //Interrupt service routine epilogue
   osExitIsr(flag);
}


/**
 * @brief MCF5225x Ethernet MAC event handler
 * @param[in] interface Underlying network interface
 **/

void mcf5225xEthEventHandler(NetInterface *interface)
{
   error_t error;
   uint32_t status;

   //Read interrupt event register
   status = MCF_FEC_EIR;

   //Packet received?
   if((status & (MCF_FEC_EIR_RXF | MCF_FEC_EIR_RXB)) != 0)
   {
      //Clear RXF and RXB interrupt flag
      MCF_FEC_EIR = MCF_FEC_EIR_RXF | MCF_FEC_EIR_RXB;

      //Process all pending packets
      do
      {
         //Read incoming packet
         error = mcf5225xEthReceivePacket(interface);

         //No more data in the receive buffer?
      } while(error != ERROR_BUFFER_EMPTY);
   }

   //System bus error?
   if((status & MCF_FEC_EIR_EBERR) != 0)
   {
      //Clear EBERR interrupt flag
      MCF_FEC_EIR = MCF_FEC_EIR_EBERR;

      //Disable Ethernet MAC
      MCF_FEC_ECR &= ~MCF_FEC_ECR_ETHER_EN;
      //Reset buffer descriptors
      mcf5225xEthInitBufferDesc(interface);
      //Resume normal operation
      MCF_FEC_ECR |= MCF_FEC_ECR_ETHER_EN;
      //Instruct the DMA to poll the receive descriptor list
      MCF_FEC_RDAR = MCF_FEC_RDAR_R_DES_ACTIVE;
   }

   //Re-enable Ethernet MAC interrupts
   MCF_FEC_EIMR = MCF_FEC_EIMR_TXF | MCF_FEC_EIMR_TXB |
      MCF_FEC_EIMR_RXF | MCF_FEC_EIMR_RXB | MCF_FEC_EIMR_EBERR;
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

error_t mcf5225xEthSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary)
{
   size_t length;

   //Retrieve the length of the packet
   length = netBufferGetLength(buffer) - offset;

   //Check the frame length
   if(length > MCF5225X_ETH_TX_BUFFER_SIZE)
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
      //Report an error
      return ERROR_INVALID_LENGTH;
   }

   //Make sure the current buffer is available for writing
   if((txBufferDesc[txBufferIndex].status & FEC_TX_BD_R) != 0)
   {
      return ERROR_FAILURE;
   }

   //Copy user data to the transmit buffer
   netBufferRead(FEC_ALIGN16(txBuffer[txBufferIndex]), buffer, offset, length);

   //Set frame length
   txBufferDesc[txBufferIndex].length = length;

   //Check current index
   if(txBufferIndex < (MCF5225X_ETH_TX_BUFFER_COUNT - 1))
   {
      //Give the ownership of the descriptor to the DMA engine
      txBufferDesc[txBufferIndex].status = FEC_TX_BD_R |
         FEC_TX_BD_L | FEC_TX_BD_TC;

      //Point to the next buffer
      txBufferIndex++;
   }
   else
   {
      //Give the ownership of the descriptor to the DMA engine
      txBufferDesc[txBufferIndex].status = FEC_TX_BD_R |
         FEC_TX_BD_W | FEC_TX_BD_L | FEC_TX_BD_TC;

      //Wrap around
      txBufferIndex = 0;
   }

   //Instruct the DMA to poll the transmit descriptor list
   MCF_FEC_TDAR = MCF_FEC_TDAR_X_DES_ACTIVE;

   //Check whether the next buffer is available for writing
   if((txBufferDesc[txBufferIndex].status & FEC_TX_BD_R) == 0)
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Receive a packet
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t mcf5225xEthReceivePacket(NetInterface *interface)
{
   error_t error;
   size_t n;
   NetRxAncillary ancillary;

   //Current buffer available for reading?
   if((rxBufferDesc[rxBufferIndex].status & FEC_RX_BD_E) == 0)
   {
      //The frame should not span multiple buffers
      if((rxBufferDesc[rxBufferIndex].status & FEC_RX_BD_L) != 0)
      {
         //Check whether an error occurred
         if((rxBufferDesc[rxBufferIndex].status & (FEC_RX_BD_LG |
            FEC_RX_BD_NO | FEC_RX_BD_CR | FEC_RX_BD_OV | FEC_RX_BD_TR)) == 0)
         {
            //Retrieve the length of the frame
            n = rxBufferDesc[rxBufferIndex].length;
            //Limit the number of data to read
            n = MIN(n, MCF5225X_ETH_RX_BUFFER_SIZE);

            //Additional options can be passed to the stack along with the packet
            ancillary = NET_DEFAULT_RX_ANCILLARY;

            //Pass the packet to the upper layer
            nicProcessPacket(interface, FEC_ALIGN16(rxBuffer[rxBufferIndex]), n,
               &ancillary);

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
      if(rxBufferIndex < (MCF5225X_ETH_RX_BUFFER_COUNT - 1))
      {
         //Give the ownership of the descriptor back to the DMA engine
         rxBufferDesc[rxBufferIndex].status = FEC_RX_BD_E;
         //Point to the next buffer
         rxBufferIndex++;
      }
      else
      {
         //Give the ownership of the descriptor back to the DMA engine
         rxBufferDesc[rxBufferIndex].status = FEC_RX_BD_E | FEC_RX_BD_W;
         //Wrap around
         rxBufferIndex = 0;
      }

      //Instruct the DMA to poll the receive descriptor list
      MCF_FEC_RDAR = MCF_FEC_RDAR_R_DES_ACTIVE;
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

error_t mcf5225xEthUpdateMacAddrFilter(NetInterface *interface)
{
   uint_t i;
   uint_t k;
   uint32_t crc;
   uint32_t value;
   uint32_t unicastHashTable[2];
   uint32_t multicastHashTable[2];
   MacFilterEntry *entry;

   //Debug message
   TRACE_DEBUG("Updating MAC filter...\r\n");

   //Set the MAC address of the station (upper 16 bits)
   value = interface->macAddr.b[5];
   value |= (interface->macAddr.b[4] << 8);
   MCF_FEC_PAUR = MCF_FEC_PAUR_PADDR2(value) | MCF_FEC_PAUR_TYPE(0x8808);

   //Set the MAC address of the station (lower 32 bits)
   value = interface->macAddr.b[3];
   value |= (interface->macAddr.b[2] << 8);
   value |= (interface->macAddr.b[1] << 16);
   value |= (interface->macAddr.b[0] << 24);
   MCF_FEC_PALR = MCF_FEC_PALR_PADDR1(value);

   //Clear hash table (unicast address filtering)
   unicastHashTable[0] = 0;
   unicastHashTable[1] = 0;

   //Clear hash table (multicast address filtering)
   multicastHashTable[0] = 0;
   multicastHashTable[1] = 0;

   //The MAC address filter contains the list of MAC addresses to accept
   //when receiving an Ethernet frame
   for(i = 0; i < MAC_ADDR_FILTER_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->macAddrFilter[i];

      //Valid entry?
      if(entry->refCount > 0)
      {
         //Compute CRC over the current MAC address
         crc = mcf5225xEthCalcCrc(&entry->addr, sizeof(MacAddr));

         //The upper 6 bits in the CRC register are used to index the
         //contents of the hash table
         k = (crc >> 26) & 0x3F;

         //Multicast address?
         if(macIsMulticastAddr(&entry->addr))
         {
            //Update the multicast hash table
            multicastHashTable[k / 32] |= (1 << (k % 32));
         }
         else
         {
            //Update the unicast hash table
            unicastHashTable[k / 32] |= (1 << (k % 32));
         }
      }
   }

   //Write the hash table (unicast address filtering)
   MCF_FEC_IALR = unicastHashTable[0];
   MCF_FEC_IAUR = unicastHashTable[1];

   //Write the hash table (multicast address filtering)
   MCF_FEC_GALR = multicastHashTable[0];
   MCF_FEC_GAUR = multicastHashTable[1];

   //Debug message
   TRACE_DEBUG("  IALR = %08" PRIX32 "\r\n", MCF_FEC_IALR);
   TRACE_DEBUG("  IAUR = %08" PRIX32 "\r\n", MCF_FEC_IAUR);
   TRACE_DEBUG("  GALR = %08" PRIX32 "\r\n", MCF_FEC_GALR);
   TRACE_DEBUG("  GAUR = %08" PRIX32 "\r\n", MCF_FEC_GAUR);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Adjust MAC configuration parameters for proper operation
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t mcf5225xEthUpdateMacConfig(NetInterface *interface)
{
   //Disable Ethernet MAC while modifying configuration registers
   MCF_FEC_ECR &= ~MCF_FEC_ECR_ETHER_EN;

   //Half-duplex or full-duplex mode?
   if(interface->duplexMode == NIC_FULL_DUPLEX_MODE)
   {
      //Full-duplex mode
      MCF_FEC_TCR |= MCF_FEC_TCR_FDEN;
      //Receive path operates independently of transmit
      MCF_FEC_RCR &= ~MCF_FEC_RCR_DRT;
   }
   else
   {
      //Half-duplex mode
      MCF_FEC_TCR &= ~MCF_FEC_TCR_FDEN;
      //Disable reception of frames while transmitting
      MCF_FEC_RCR |= MCF_FEC_RCR_DRT;
   }

   //Reset buffer descriptors
   mcf5225xEthInitBufferDesc(interface);

   //Re-enable Ethernet MAC
   MCF_FEC_ECR |= MCF_FEC_ECR_ETHER_EN;
   //Instruct the DMA to poll the receive descriptor list
   MCF_FEC_RDAR = MCF_FEC_RDAR_R_DES_ACTIVE;

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

void mcf5225xEthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data)
{
   uint32_t temp;

   //Valid opcode?
   if(opcode == SMI_OPCODE_WRITE)
   {
      //Set up a write operation
      temp = MCF_FEC_MMFR_ST(1) | MCF_FEC_MMFR_OP(1) | MCF_FEC_MMFR_TA(2);
      //PHY address
      temp |= MCF_FEC_MMFR_PA(phyAddr);
      //Register address
      temp |= MCF_FEC_MMFR_RA(regAddr);
      //Register value
      temp |= MCF_FEC_MMFR_DATA(data);

      //Clear MII interrupt flag
      MCF_FEC_EIR = MCF_FEC_EIR_MII;
      //Start a write operation
      MCF_FEC_MMFR = temp;

      //Wait for the write to complete
      while((MCF_FEC_EIR & MCF_FEC_EIR_MII) == 0)
      {
      }
   }
   else
   {
      //The MAC peripheral only supports standard Clause 22 opcodes
   }
}


/**
 * @brief Read PHY register
 * @param[in] opcode Access type (2 bits)
 * @param[in] phyAddr PHY address (5 bits)
 * @param[in] regAddr Register address (5 bits)
 * @return Register value
 **/

uint16_t mcf5225xEthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr)
{
   uint16_t data;
   uint32_t temp;

   //Valid opcode?
   if(opcode == SMI_OPCODE_READ)
   {
      //Set up a read operation
      temp = MCF_FEC_MMFR_ST(1) | MCF_FEC_MMFR_OP(2) | MCF_FEC_MMFR_TA(2);
      //PHY address
      temp |= MCF_FEC_MMFR_PA(phyAddr);
      //Register address
      temp |= MCF_FEC_MMFR_RA(regAddr);

      //Clear MII interrupt flag
      MCF_FEC_EIR = MCF_FEC_EIR_MII;
      //Start a read operation
      MCF_FEC_MMFR = temp;

      //Wait for the read to complete
      while((MCF_FEC_EIR & MCF_FEC_EIR_MII) == 0)
      {
      }

      //Get register value
      data = MCF_FEC_MMFR;
   }
   else
   {
      //The MAC peripheral only supports standard Clause 22 opcodes
      data = 0;
   }

   //Return the value of the PHY register
   return data;
}


/**
 * @brief CRC calculation
 * @param[in] data Pointer to the data over which to calculate the CRC
 * @param[in] length Number of bytes to process
 * @return Resulting CRC value
 **/

uint32_t mcf5225xEthCalcCrc(const void *data, size_t length)
{
   uint_t i;
   uint_t j;
   uint32_t crc;
   const uint8_t *p;

   //Point to the data over which to calculate the CRC
   p = (uint8_t *) data;
   //CRC preset value
   crc = 0xFFFFFFFF;

   //Loop through data
   for(i = 0; i < length; i++)
   {
      //Update CRC value
      crc ^= p[i];
      //The message is processed bit by bit
      for(j = 0; j < 8; j++)
      {
         if((crc & 0x01) != 0)
         {
            crc = (crc >> 1) ^ 0xEDB88320;
         }
         else
         {
            crc = crc >> 1;
         }
      }
   }

   //Return CRC value
   return crc;
}
