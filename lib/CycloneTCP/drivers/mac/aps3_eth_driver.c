/**
 * @file aps3_eth_driver.c
 * @brief Cortus APS3 Ethernet MAC driver
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
#include <machine/sfradr.h>
#include <machine/sfradr_eth.h>
#include <machine/ethernet.h>
#include <machine/ic.h>
#undef _ETHERNET_H
#include "core/net.h"
#include "drivers/mac/aps3_eth_driver.h"
#include "debug.h"

//Transmit buffer
#define txBuffer ((uint8_t *) SFRADR_ETH_TX_MEM_BOTTOM_AD)
//Receive buffer
#define rxBuffer ((uint8_t *) SFRADR_ETH_RX_MEM_BOTTOM_AD)

//Transmit DMA descriptors
#define txDmaDesc ((Aps3TxDmaDesc *) (SFRADR_ETH_TX_MEM_BOTTOM_AD + \
   APS3_ETH_TX_BUFFER_COUNT * APS3_ETH_TX_BUFFER_SIZE))

//Receive DMA descriptors
#define rxDmaDesc ((Aps3RxDmaDesc *) (SFRADR_ETH_RX_MEM_BOTTOM_AD + \
   APS3_ETH_RX_BUFFER_COUNT * APS3_ETH_RX_BUFFER_SIZE))

//Underlying network interface
static NetInterface *nicDriverInterface;


/**
 * @brief Cortus APS3 Ethernet MAC driver
 **/

const NicDriver aps3EthDriver =
{
   NIC_TYPE_ETHERNET,
   ETH_MTU,
   aps3EthInit,
   aps3EthTick,
   aps3EthEnableIrq,
   aps3EthDisableIrq,
   aps3EthEventHandler,
   aps3EthSendPacket,
   aps3EthUpdateMacAddrFilter,
   aps3EthUpdateMacConfig,
   aps3EthWritePhyReg,
   aps3EthReadPhyReg,
   TRUE,
   TRUE,
   TRUE,
   FALSE
};


/**
 * @brief Cortus APS3 Ethernet MAC initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t aps3EthInit(NetInterface *interface)
{
   error_t error;

   //Debug message
   TRACE_INFO("Initializing Cortus APS3 Ethernet MAC...\r\n");

   //Save underlying network interface
   nicDriverInterface = interface;

   //Adjust MDC clock range
   eth_miim->miim_clock_divider = 32;

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

   //Reset Ethernet MAC peripheral
   eth_mac->sw_reset = 1;

   //Set the MAC address of the station
   eth_mac->addr_low = interface->macAddr.w[0] | (interface->macAddr.w[1] << 16);
   eth_mac->addr_high = interface->macAddr.w[2];

   //Initialize hash table
   eth_mac->hash_filter_low = 0;
   eth_mac->hash_filter_high = 0;

   //Configure the receive filter
   eth_mac->unicast = 1;
   eth_mac->multicast = 0;
   eth_mac->broadcast = 1;
   eth_mac->hash = 1;
   eth_mac->exact_addr = 1;

   //Default duplex mode
   eth_mac->full_duplex = 0;

   //Automatic padding and CRC generation
   eth_mac->no_padding = 0;
   eth_mac->crc_disable = 0;

   //Set the maximum frame length
   eth_mac->max_frame_size = APS3_ETH_RX_BUFFER_SIZE;

   //Set transmit and receive thresholds
   eth_tx->tx_threshold = 0;
   eth_rx->rx_threshold = 0;

   //Disable indefinite deferral
   eth_mac->indefinite_deferral = 0;
   //Number of attempts to transmit a frame before aborting
   eth_mac->max_deferral = 15;

   //Use default collision window (112 half-octets)
   eth_mac->collision_window = 111;
   //Maximum Number of Collisions
   eth_mac->max_collision = 15;

   //Automatic backoff on collision
   eth_mac->no_backoff = 0;

   //Use the default interframe gap (24 half-octets or 96 bits)
   eth_mac->interframe_gap = 23;

   //Initialize DMA descriptor lists
   aps3EthInitDmaDesc(interface);

   //Configure TX interrupts
   eth_tx->tx_irq_mask = TX_IRQ_MASK_MEMORY_AVAILABLE;
   //Configure RX interrupts
   eth_rx->rx_irq_mask = RX_IRQ_MASK_FRAME_READY;

   //Configure TX interrupt priority
   irq[IRQ_ETH_TX].ipl = APS3_ETH_IRQ_PRIORITY;
   //Configure RX interrupt priority
   irq[IRQ_ETH_RX].ipl = APS3_ETH_IRQ_PRIORITY;

   //Enable transmission and reception
   eth_tx->tx_enable = 1;
   eth_rx->rx_enable = 1;

   //Accept any packets from the upper layer
   osSetEvent(&interface->nicTxEvent);

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief Initialize DMA descriptor lists
 * @param[in] interface Underlying network interface
 **/

void aps3EthInitDmaDesc(NetInterface *interface)
{
   uint_t i;

   //Initialize TX DMA descriptor list
   for(i = 0; i < APS3_ETH_TX_BUFFER_COUNT; i++)
   {
      //Transmit buffer address
      txDmaDesc[i].addr = (uint32_t) txBuffer + (APS3_ETH_TX_BUFFER_SIZE * i);
      //Transmit buffer size
      txDmaDesc[i].size = 0;
      //Transmit status
      txDmaDesc[i].status = 0;
   }

   //Initialize RX DMA descriptor list
   for(i = 0; i < APS3_ETH_RX_BUFFER_COUNT; i++)
   {
      //Receive buffer address
      rxDmaDesc[i].addr = (uint32_t) rxBuffer + (APS3_ETH_RX_BUFFER_SIZE * i);
      //Receive buffer size
      rxDmaDesc[i].size = 0;
      //Receive status
      rxDmaDesc[i].status = 0;
   }

   //Start location of the TX descriptor list
   eth_tx->tx_desc_base_addr = (uint32_t) txDmaDesc;
   //Number of TX descriptors
   eth_tx->tx_desc_number = APS3_ETH_TX_BUFFER_COUNT - 1;

   //Start location of the RX descriptor list
   eth_rx->rx_desc_base_addr = (uint32_t) rxDmaDesc;
   //Number of RX descriptors
   eth_rx->rx_desc_number = APS3_ETH_RX_BUFFER_COUNT - 1;
}


/**
 * @brief Cortus APS3 Ethernet MAC timer handler
 *
 * This routine is periodically called by the TCP/IP stack to handle periodic
 * operations such as polling the link state
 *
 * @param[in] interface Underlying network interface
 **/

void aps3EthTick(NetInterface *interface)
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

void aps3EthEnableIrq(NetInterface *interface)
{
   //Enable Ethernet MAC interrupts
   irq[IRQ_ETH_TX].ien = 1;
   irq[IRQ_ETH_RX].ien = 1;


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

void aps3EthDisableIrq(NetInterface *interface)
{
   //Disable Ethernet MAC interrupts
   irq[IRQ_ETH_TX].ien = 0;
   irq[IRQ_ETH_RX].ien = 0;


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
 * @brief Ethernet MAC transmit interrupt service routine
 **/

void aps3EthTxIrqHandler(void)
{
   bool_t flag;

   //Interrupt service routine prologue
   osEnterIsr();

   //This flag will be set if a higher priority task must be woken
   flag = FALSE;

   //Check interrupt flag
   if((eth_tx->tx_status & TX_IRQ_MASK_MEMORY_AVAILABLE) != 0)
   {
      //Disable TX interrupts
      eth_tx->tx_irq_mask = 0;

      //Check whether the TX buffer is available for writing
      if(eth_tx->tx_desc_status == 0)
      {
         //Notify the TCP/IP stack that the transmitter is ready to send
         flag = osSetEventFromIsr(&nicDriverInterface->nicTxEvent);
      }
   }

   //Interrupt service routine epilogue
   osExitIsr(flag);
}


/**
 * @brief Ethernet MAC receive interrupt service routine
 **/

void aps3EthRxIrqHandler(void)
{
   bool_t flag;

   //Interrupt service routine prologue
   osEnterIsr();

   //This flag will be set if a higher priority task must be woken
   flag = FALSE;

   //Disable RX interrupts
   eth_rx->rx_irq_mask = 0;

   //Set event flag
   nicDriverInterface->nicEvent = TRUE;
   //Notify the TCP/IP stack of the event
   flag = osSetEventFromIsr(&netEvent);

   //Interrupt service routine epilogue
   osExitIsr(flag);
}


/**
 * @brief Cortus APS3 Ethernet MAC event handler
 * @param[in] interface Underlying network interface
 **/

void aps3EthEventHandler(NetInterface *interface)
{
   error_t error;

   //Packet received?
   if((eth_rx->rx_status & RX_IRQ_MASK_FRAME_READY) != 0)
   {
      //Process all pending packets
      do
      {
         //Read incoming packet
         error = aps3EthReceivePacket(interface);

         //No more data in the receive buffer?
      } while(error != ERROR_BUFFER_EMPTY);
   }

   //Re-enable RX interrupts
   eth_rx->rx_irq_mask = RX_IRQ_MASK_FRAME_READY;
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

error_t aps3EthSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary)
{
   uint_t i;
   size_t length;

   //Retrieve the length of the packet
   length = netBufferGetLength(buffer) - offset;

   //Check the frame length
   if(length > APS3_ETH_TX_BUFFER_SIZE)
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
      //Report an error
      return ERROR_INVALID_LENGTH;
   }

   //Make sure the current buffer is available for writing
   if(eth_tx->tx_desc_status)
   {
      //Re-enable TX interrupts
      eth_tx->tx_irq_mask = TX_IRQ_MASK_MEMORY_AVAILABLE;
      //Report an error
      return ERROR_FAILURE;
   }

   //Get the index of the current descriptor
   i = eth_tx->tx_desc_produce;

   //Copy user data to the transmit buffer
   netBufferRead((uint8_t *) txDmaDesc[i].addr, buffer, offset, length);
   //Write the number of bytes to send
   txDmaDesc[i].size = length;

   //Start transmission
   eth_tx->tx_sw_done = 1;

   //Check whether the next buffer is available for writing
   if(!eth_tx->tx_desc_status)
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
   }
   else
   {
      //Re-enable TX interrupts
      eth_tx->tx_irq_mask = TX_IRQ_MASK_MEMORY_AVAILABLE;
   }

   //Data successfully written
   return NO_ERROR;
}


/**
 * @brief Receive a packet
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t aps3EthReceivePacket(NetInterface *interface)
{
   error_t error;
   uint_t i;
   size_t n;
   NetRxAncillary ancillary;

   //Current buffer available for reading?
   if(eth_rx->rx_desc_status == 0)
   {
      //Point to the current descriptor
      i = eth_rx->rx_desc_consume;

      //Make sure no error occurred
      if((rxDmaDesc[i].status & RX_DESC_RECEIVE_ERROR) == 0)
      {
         //Retrieve the length of the frame
         n = rxDmaDesc[i].size;
         //Limit the number of data to read
         n = MIN(n, APS3_ETH_RX_BUFFER_SIZE);

         //Additional options can be passed to the stack along with the packet
         ancillary = NET_DEFAULT_RX_ANCILLARY;

         //Pass the packet to the upper layer
         nicProcessPacket(interface, (uint8_t *) rxDmaDesc[i].addr, n,
            &ancillary);

         //Valid packet received
         error = NO_ERROR;
      }
      else
      {
         //The received packet contains an error
         error = ERROR_INVALID_PACKET;
      }

      //The frame has been has been processed by the software
      //and is no longer needed
      eth_rx->rx_sw_done = 1;
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

error_t aps3EthUpdateMacAddrFilter(NetInterface *interface)
{
   uint_t i;
   uint_t k;
   uint32_t crc;
   uint32_t hashTable[2];
   MacFilterEntry *entry;

   //Debug message
   TRACE_DEBUG("Updating MAC filter...\r\n");

   //Set the MAC address of the station
   eth_mac->addr_low = interface->macAddr.w[0] | (interface->macAddr.w[1] << 16);
   eth_mac->addr_high = interface->macAddr.w[2];

   //Clear hash table
   hashTable[0] = 0;
   hashTable[1] = 0;

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
         crc = aps3EthCalcCrc(&entry->addr, sizeof(MacAddr));
         //Calculate the corresponding index in the table
         k = (crc >> 23) & 0x3F;
         //Update hash table contents
         hashTable[k / 32] |= (1 << (k % 32));
      }
   }

   //Disable transmission and reception
   eth_tx->tx_enable = 0;
   eth_rx->rx_enable = 0;

   //Write the hash table
   eth_mac->hash_filter_low = hashTable[0];
   eth_mac->hash_filter_high = hashTable[1];

   //Debug message
   TRACE_DEBUG("  hash_filter_low = %08" PRIX32 "\r\n", hashTable[0]);
   TRACE_DEBUG("  hash_filter_high = %08" PRIX32 "\r\n", hashTable[1]);

   //Re-enable transmission and reception
   eth_tx->tx_enable = 1;
   eth_rx->rx_enable = 1;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Adjust MAC configuration parameters for proper operation
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t aps3EthUpdateMacConfig(NetInterface *interface)
{
   //Disable transmission and reception
   eth_tx->tx_enable = 0;
   eth_rx->rx_enable = 0;

   //Half-duplex or full-duplex mode?
   if(interface->duplexMode == NIC_FULL_DUPLEX_MODE)
   {
      eth_mac->full_duplex = 1;
   }
   else
   {
      eth_mac->full_duplex = 0;
   }

   //Re-enable transmission and reception
   eth_tx->tx_enable = 1;
   eth_rx->rx_enable = 1;

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

void aps3EthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data)
{
   //Valid opcode?
   if(opcode == SMI_OPCODE_WRITE)
   {
      //Wait for the MII management module to be ready
      while(!eth_miim->miim_status)
      {
      }

      //PHY address
      eth_miim->miim_phy_addr = phyAddr;
      //Register address
      eth_miim->miim_phy_register_addr = regAddr;
      //Data to be written in the PHY register
      eth_miim->miim_data = data;

      //Start a write operation
      eth_miim->miim_read_write = 0;
      //Wait for the write to complete
      while(!eth_miim->miim_status)
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

uint16_t aps3EthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr)
{
   uint16_t data;

   //Valid opcode?
   if(opcode == SMI_OPCODE_READ)
   {
      //Wait for the MII management module to be ready
      while(!eth_miim->miim_status)
      {
      }

      //PHY address
      eth_miim->miim_phy_addr = phyAddr;
      //Register address
      eth_miim->miim_phy_register_addr = regAddr;

      //Start a read operation
      eth_miim->miim_read_write = 1;
      //Wait for the read to complete
      while(!eth_miim->miim_status)
      {
      }

      //Get register value
      data = eth_miim->miim_data;
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

uint32_t aps3EthCalcCrc(const void *data, size_t length)
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
         //Update CRC value
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
   return ~crc;
}
