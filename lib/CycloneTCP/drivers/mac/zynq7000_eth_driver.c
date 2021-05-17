/**
 * @file zynq7000_eth_driver.c
 * @brief Zynq-7000 Gigabit Ethernet MAC driver
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
#include <limits.h>
#include "xemacps_hw.h"
#include "xscugic.h"
#include "xil_misc_psreset_api.h"
#include "core/net.h"
#include "drivers/mac/zynq7000_eth_driver.h"
#include "debug.h"

//Underlying network interface
static NetInterface *nicDriverInterface;

//GIC instance
extern XScuGic ZYNQ7000_ETH_GIC_INSTANCE;

//IAR EWARM compiler?
#if defined(__ICCARM__)

//TX buffer
#pragma data_alignment = 8
#pragma location = ZYNQ7000_ETH_RAM_SECTION
static uint8_t txBuffer[ZYNQ7000_ETH_TX_BUFFER_COUNT][ZYNQ7000_ETH_TX_BUFFER_SIZE];
//RX buffer
#pragma data_alignment = 8
#pragma location = ZYNQ7000_ETH_RAM_SECTION
static uint8_t rxBuffer[ZYNQ7000_ETH_RX_BUFFER_COUNT][ZYNQ7000_ETH_RX_BUFFER_SIZE];
//TX buffer descriptors
#pragma data_alignment = 4
#pragma location = ZYNQ7000_ETH_RAM_SECTION
static Zynq7000TxBufferDesc txBufferDesc[ZYNQ7000_ETH_TX_BUFFER_COUNT];
//RX buffer descriptors
#pragma data_alignment = 4
#pragma location = ZYNQ7000_ETH_RAM_SECTION
static Zynq7000RxBufferDesc rxBufferDesc[ZYNQ7000_ETH_RX_BUFFER_COUNT];

//GCC compiler?
#else

//TX buffer
static uint8_t txBuffer[ZYNQ7000_ETH_TX_BUFFER_COUNT][ZYNQ7000_ETH_TX_BUFFER_SIZE]
   __attribute__((aligned(8), __section__(ZYNQ7000_ETH_RAM_SECTION)));
//RX buffer
static uint8_t rxBuffer[ZYNQ7000_ETH_RX_BUFFER_COUNT][ZYNQ7000_ETH_RX_BUFFER_SIZE]
   __attribute__((aligned(8), __section__(ZYNQ7000_ETH_RAM_SECTION)));
//TX buffer descriptors
static Zynq7000TxBufferDesc txBufferDesc[ZYNQ7000_ETH_TX_BUFFER_COUNT]
   __attribute__((aligned(4), __section__(ZYNQ7000_ETH_RAM_SECTION)));
//RX buffer descriptors
static Zynq7000RxBufferDesc rxBufferDesc[ZYNQ7000_ETH_RX_BUFFER_COUNT]
   __attribute__((aligned(4), __section__(ZYNQ7000_ETH_RAM_SECTION)));

#endif

//TX buffer index
static uint_t txBufferIndex;
//RX buffer index
static uint_t rxBufferIndex;


/**
 * @brief Zynq-7000 Ethernet MAC driver
 **/

const NicDriver zynq7000EthDriver =
{
   NIC_TYPE_ETHERNET,
   ETH_MTU,
   zynq7000EthInit,
   zynq7000EthTick,
   zynq7000EthEnableIrq,
   zynq7000EthDisableIrq,
   zynq7000EthEventHandler,
   zynq7000EthSendPacket,
   zynq7000EthUpdateMacAddrFilter,
   zynq7000EthUpdateMacConfig,
   zynq7000EthWritePhyReg,
   zynq7000EthReadPhyReg,
   TRUE,
   TRUE,
   TRUE,
   FALSE
};


/**
 * @brief Zynq-7000 Ethernet MAC initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t zynq7000EthInit(NetInterface *interface)
{
   error_t error;
   volatile uint32_t temp;

   //Debug message
   TRACE_INFO("Initializing Zynq-7000 Ethernet MAC...\r\n");

   //Save underlying network interface
   nicDriverInterface = interface;

   //Unlock SLCR
   XSLCR_UNLOCK = XSLCR_UNLOCK_KEY_VALUE;

   //Configure Ethernet controller reference clock
   temp = XSLCR_GEM0_CLK_CTRL_CLKACT_MASK;
   temp |= (XPAR_PS7_ETHERNET_0_ENET_SLCR_1000MBPS_DIV1 << 20) & XSLCR_GEM0_CLK_CTRL_DIV1_MASK;
   temp |= (XPAR_PS7_ETHERNET_0_ENET_SLCR_1000MBPS_DIV0 << 8) & XSLCR_GEM0_CLK_CTRL_DIV0_MASK;
   XSLCR_GEM0_CLK_CTRL = temp;

   //Enable Ethernet controller RX clock
   XSLCR_GEM0_RCLK_CTRL = XSLCR_GEM0_RCLK_CTRL_CLKACT_MASK;

   //Lock SLCR
   XSLCR_LOCK = XSLCR_LOCK_KEY_VALUE;

   //Clear network control register
   XEMACPS_NWCTRL = 0;
   //Clear statistics registers
   XEMACPS_NWCTRL |= XEMACPS_NWCTRL_STATCLR_MASK;

   //Configure MDC clock speed
   XEMACPS_NWCFG = (MDC_DIV_224 << XEMACPS_NWCFG_MDC_SHIFT_MASK) | XEMACPS_NWCFG_MDCCLKDIV_MASK;
   //Enable management port (MDC and MDIO)
   XEMACPS_NWCTRL |= XEMACPS_NWCTRL_MDEN_MASK;

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

   //Set the MAC address of the station
   XEMACPS_LADDR1L = interface->macAddr.w[0] | (interface->macAddr.w[1] << 16);
   XEMACPS_LADDR1H = interface->macAddr.w[2];

   //Configure the receive filter
   XEMACPS_NWCFG |= XEMACPS_NWCFG_UCASTHASHEN_MASK | XEMACPS_NWCFG_MCASTHASHEN_MASK;

   //Initialize hash table
   XEMACPS_HASHL = 0;
   XEMACPS_HASHH = 0;

   //Initialize buffer descriptors
   zynq7000EthInitBufferDesc(interface);

   //Set RX buffer size
   temp = ((ZYNQ7000_ETH_RX_BUFFER_SIZE / 64) << XEMACPS_DMACR_RXBUF_SHIFT) &
      XEMACPS_DMACR_RXBUF_MASK;

   //Use full configured addressable space for transmit and receive packet buffers
   temp |= XEMACPS_DMACR_TXSIZE_MASK | XEMACPS_DMACR_RXSIZE_MASK;
   //Select the burst length for DMA data operations
   temp |= XEMACPS_DMACR_INCR16_AHB_BURST;
   //Set DMA configuration register
   XEMACPS_DMACR = temp;

   //Clear transmit status register
   XEMACPS_TXSR = XEMACPS_TXSR_TXCOMPL_MASK | XEMACPS_TXSR_TXGO_MASK |
      XEMACPS_TXSR_ERROR_MASK;

   //Clear receive status register
   XEMACPS_RXSR = XEMACPS_RXSR_FRAMERX_MASK | XEMACPS_RXSR_ERROR_MASK;

   //First disable all interrupts
   XEMACPS_IDR = 0xFFFFFFFF;

   //Only the desired ones are enabled
   XEMACPS_IER = XEMACPS_IXR_HRESPNOK_MASK | XEMACPS_IXR_RXOVR_MASK |
      XEMACPS_IXR_TXCOMPL_MASK | XEMACPS_IXR_TXEXH_MASK | XEMACPS_IXR_RETRY_MASK |
      XEMACPS_IXR_URUN_MASK | XEMACPS_IXR_RXUSED_MASK | XEMACPS_IXR_FRAMERX_MASK;

   //Read interrupt status register to clear any pending interrupt
   temp = XEMACPS_ISR;

   //Register interrupt handler
   XScuGic_Connect(&ZYNQ7000_ETH_GIC_INSTANCE, XPS_GEM0_INT_ID,
      (Xil_InterruptHandler) zynq7000EthIrqHandler, interface);

   //Configure interrupt priority
   XScuGic_SetPriorityTriggerType(&ZYNQ7000_ETH_GIC_INSTANCE,
      XPS_GEM0_INT_ID, ZYNQ7000_ETH_IRQ_PRIORITY, 1);

   //Enable the transmitter and the receiver
   XEMACPS_NWCTRL |= XEMACPS_NWCTRL_TXEN_MASK | XEMACPS_NWCTRL_RXEN_MASK;

   //Accept any packets from the upper layer
   osSetEvent(&interface->nicTxEvent);

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief Initialize buffer descriptors
 * @param[in] interface Underlying network interface
 **/

void zynq7000EthInitBufferDesc(NetInterface *interface)
{
   uint_t i;
   uint32_t address;

   //Initialize TX buffer descriptors
   for(i = 0; i < ZYNQ7000_ETH_TX_BUFFER_COUNT; i++)
   {
      //Calculate the address of the current TX buffer
      address = (uint32_t) txBuffer[i];
      //Write the address to the descriptor entry
      txBufferDesc[i].address = address;
      //Initialize status field
      txBufferDesc[i].status = XEMACPS_TX_USED;
   }

   //Mark the last descriptor entry with the wrap flag
   txBufferDesc[i - 1].status |= XEMACPS_TX_WRAP;
   //Initialize TX buffer index
   txBufferIndex = 0;

   //Initialize RX buffer descriptors
   for(i = 0; i < ZYNQ7000_ETH_RX_BUFFER_COUNT; i++)
   {
      //Calculate the address of the current RX buffer
      address = (uint32_t) rxBuffer[i];
      //Write the address to the descriptor entry
      rxBufferDesc[i].address = address & XEMACPS_RX_ADDRESS;
      //Clear status field
      rxBufferDesc[i].status = 0;
   }

   //Mark the last descriptor entry with the wrap flag
   rxBufferDesc[i - 1].address |= XEMACPS_RX_WRAP;
   //Initialize RX buffer index
   rxBufferIndex = 0;

   //Start location of the TX descriptor list
   XEMACPS_TXQBASE = (uint32_t) txBufferDesc;
   //Start location of the RX descriptor list
   XEMACPS_RXQBASE = (uint32_t) rxBufferDesc;
}


/**
 * @brief Zynq-7000 Ethernet MAC timer handler
 *
 * This routine is periodically called by the TCP/IP stack to handle periodic
 * operations such as polling the link state
 *
 * @param[in] interface Underlying network interface
 **/

void zynq7000EthTick(NetInterface *interface)
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

void zynq7000EthEnableIrq(NetInterface *interface)
{
   //Enable Ethernet MAC interrupts
   XScuGic_Enable(&ZYNQ7000_ETH_GIC_INSTANCE, XPS_GEM0_INT_ID);

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

void zynq7000EthDisableIrq(NetInterface *interface)
{
   //Disable Ethernet MAC interrupts
   XScuGic_Disable(&ZYNQ7000_ETH_GIC_INSTANCE, XPS_GEM0_INT_ID);

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
 * @brief Zynq-7000 Ethernet MAC interrupt service routine
 * @param[in] interface Underlying network interface
 **/

void zynq7000EthIrqHandler(NetInterface *interface)
{
   bool_t flag;
   volatile uint32_t isr;
   volatile uint32_t tsr;
   volatile uint32_t rsr;

   //Interrupt service routine prologue
   osEnterIsr();

   //This flag will be set if a higher priority task must be woken
   flag = FALSE;

   //Each time the software reads XEMACPS_ISR, it has to check the
   //contents of XEMACPS_TXSR, XEMACPS_RXSR
   isr = XEMACPS_ISR;
   tsr = XEMACPS_TXSR;
   rsr = XEMACPS_RXSR;

   //Clear interrupt flags
   XEMACPS_ISR = isr;

   //Packet transmitted?
   if((tsr & (XEMACPS_TXSR_TXCOMPL_MASK | XEMACPS_TXSR_TXGO_MASK |
      XEMACPS_TXSR_ERROR_MASK)) != 0)
   {
      //Only clear TSR flags that are currently set
      XEMACPS_TXSR = tsr;

      //Check whether the TX buffer is available for writing
      if((txBufferDesc[txBufferIndex].status & XEMACPS_TX_USED) != 0)
      {
         //Notify the TCP/IP stack that the transmitter is ready to send
         flag |= osSetEventFromIsr(&nicDriverInterface->nicTxEvent);
      }
   }

   //Packet received?
   if((rsr & (XEMACPS_RXSR_FRAMERX_MASK | XEMACPS_RXSR_ERROR_MASK)) != 0)
   {
      //Set event flag
      nicDriverInterface->nicEvent = TRUE;
      //Notify the TCP/IP stack of the event
      flag |= osSetEventFromIsr(&netEvent);
   }

   //Flush packet if the receive buffer not available
   if((isr & XEMACPS_IXR_RXUSED_MASK) != 0)
   {
      XEMACPS_NWCTRL |= XEMACPS_NWCTRL_FLUSH_DPRAM_MASK;
   }

   //Interrupt service routine epilogue
   osExitIsr(flag);
}


/**
 * @brief Zynq-7000 Ethernet MAC event handler
 * @param[in] interface Underlying network interface
 **/

void zynq7000EthEventHandler(NetInterface *interface)
{
   error_t error;
   uint32_t rsr;

   //Read receive status
   rsr = XEMACPS_RXSR;

   //Packet received?
   if((rsr & (XEMACPS_RXSR_FRAMERX_MASK | XEMACPS_RXSR_ERROR_MASK)) != 0)
   {
      //Only clear RSR flags that are currently set
      XEMACPS_RXSR = rsr;

      //Process all pending packets
      do
      {
         //Read incoming packet
         error = zynq7000EthReceivePacket(interface);

         //No more data in the receive buffer?
      } while(error != ERROR_BUFFER_EMPTY);
   }
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

error_t zynq7000EthSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary)
{
   static uint8_t temp[ZYNQ7000_ETH_TX_BUFFER_SIZE];
   size_t length;

   //Retrieve the length of the packet
   length = netBufferGetLength(buffer) - offset;

   //Check the frame length
   if(length > ZYNQ7000_ETH_TX_BUFFER_SIZE)
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
      //Report an error
      return ERROR_INVALID_LENGTH;
   }

   //Make sure the current buffer is available for writing
   if((txBufferDesc[txBufferIndex].status & XEMACPS_TX_USED) == 0)
   {
      return ERROR_FAILURE;
   }

   //Copy user data to the transmit buffer
   netBufferRead(temp, buffer, offset, length);
   osMemcpy(txBuffer[txBufferIndex], temp, length);

   //Set the necessary flags in the descriptor entry
   if(txBufferIndex < (ZYNQ7000_ETH_TX_BUFFER_COUNT - 1))
   {
      //Write the status word
      txBufferDesc[txBufferIndex].status = XEMACPS_TX_LAST |
         (length & XEMACPS_TX_LENGTH);

      //Point to the next buffer
      txBufferIndex++;
   }
   else
   {
      //Write the status word
      txBufferDesc[txBufferIndex].status = XEMACPS_TX_WRAP | XEMACPS_TX_LAST |
         (length & XEMACPS_TX_LENGTH);

      //Wrap around
      txBufferIndex = 0;
   }

   //Set the STARTTX bit to initiate transmission
   XEMACPS_NWCTRL |= XEMACPS_NWCTRL_STARTTX_MASK;

   //Check whether the next buffer is available for writing
   if((txBufferDesc[txBufferIndex].status & XEMACPS_TX_USED) != 0)
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

error_t zynq7000EthReceivePacket(NetInterface *interface)
{
   static uint8_t temp[ETH_MAX_FRAME_SIZE];
   error_t error;
   uint_t i;
   uint_t j;
   uint_t sofIndex;
   uint_t eofIndex;
   size_t n;
   size_t size;
   size_t length;

   //Initialize SOF and EOF indices
   sofIndex = UINT_MAX;
   eofIndex = UINT_MAX;

   //Search for SOF and EOF flags
   for(i = 0; i < ZYNQ7000_ETH_RX_BUFFER_COUNT; i++)
   {
      //Point to the current entry
      j = rxBufferIndex + i;

      //Wrap around to the beginning of the buffer if necessary
      if(j >= ZYNQ7000_ETH_RX_BUFFER_COUNT)
      {
         j -= ZYNQ7000_ETH_RX_BUFFER_COUNT;
      }

      //No more entries to process?
      if((rxBufferDesc[j].address & XEMACPS_RX_OWNERSHIP) == 0)
      {
         //Stop processing
         break;
      }

      //A valid SOF has been found?
      if((rxBufferDesc[j].status & XEMACPS_RX_SOF) != 0)
      {
         //Save the position of the SOF
         sofIndex = i;
      }

      //A valid EOF has been found?
      if((rxBufferDesc[j].status & XEMACPS_RX_EOF) != 0 && sofIndex != UINT_MAX)
      {
         //Save the position of the EOF
         eofIndex = i;
         //Retrieve the length of the frame
         size = rxBufferDesc[j].status & XEMACPS_RX_LENGTH;
         //Limit the number of data to read
         size = MIN(size, ETH_MAX_FRAME_SIZE);
         //Stop processing since we have reached the end of the frame
         break;
      }
   }

   //Determine the number of entries to process
   if(eofIndex != UINT_MAX)
   {
      j = eofIndex + 1;
   }
   else if(sofIndex != UINT_MAX)
   {
      j = sofIndex;
   }
   else
   {
      j = i;
   }

   //Total number of bytes that have been copied from the receive buffer
   length = 0;

   //Process incoming frame
   for(i = 0; i < j; i++)
   {
      //Any data to copy from current buffer?
      if(eofIndex != UINT_MAX && i >= sofIndex && i <= eofIndex)
      {
         //Calculate the number of bytes to read at a time
         n = MIN(size, ZYNQ7000_ETH_RX_BUFFER_SIZE);
         //Copy data from receive buffer
         osMemcpy(temp + length, rxBuffer[rxBufferIndex], n);
         //Update byte counters
         length += n;
         size -= n;
      }

      //Mark the current buffer as free
      rxBufferDesc[rxBufferIndex].address &= ~XEMACPS_RX_OWNERSHIP;

      //Point to the following entry
      rxBufferIndex++;

      //Wrap around to the beginning of the buffer if necessary
      if(rxBufferIndex >= ZYNQ7000_ETH_RX_BUFFER_COUNT)
      {
         rxBufferIndex = 0;
      }
   }

   //Any packet to process?
   if(length > 0)
   {
      NetRxAncillary ancillary;

      //Additional options can be passed to the stack along with the packet
      ancillary = NET_DEFAULT_RX_ANCILLARY;

      //Pass the packet to the upper layer
      nicProcessPacket(interface, temp, length, &ancillary);
      //Valid packet received
      error = NO_ERROR;
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

error_t zynq7000EthUpdateMacAddrFilter(NetInterface *interface)
{
   uint_t i;
   uint_t k;
   uint8_t *p;
   uint32_t hashTable[2];
   MacFilterEntry *entry;

   //Debug message
   TRACE_DEBUG("Updating MAC filter...\r\n");

   //Set the MAC address of the station
   XEMACPS_LADDR1L = interface->macAddr.w[0] | (interface->macAddr.w[1] << 16);
   XEMACPS_LADDR1H = interface->macAddr.w[2];

   //Clear hash table
   hashTable[0] = 0;
   hashTable[1] = 0;

   //The MAC address filter contains the list of MAC addresses to accept
   //to accept when receiving an Ethernet frame
   for(i = 0; i < MAC_ADDR_FILTER_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->macAddrFilter[i];

      //Valid entry?
      if(entry->refCount > 0)
      {
         //Point to the MAC address
         p = entry->addr.b;

         //Apply the hash function
         k = (p[0] >> 6) ^ p[0];
         k ^= (p[1] >> 4) ^ (p[1] << 2);
         k ^= (p[2] >> 2) ^ (p[2] << 4);
         k ^= (p[3] >> 6) ^ p[3];
         k ^= (p[4] >> 4) ^ (p[4] << 2);
         k ^= (p[5] >> 2) ^ (p[5] << 4);

         //The hash value is reduced to a 6-bit index
         k &= 0x3F;

         //Update hash table contents
         hashTable[k / 32] |= (1 << (k % 32));
      }
   }

   //Write the hash table
   XEMACPS_HASHL = hashTable[0];
   XEMACPS_HASHH = hashTable[1];

   //Debug message
   TRACE_DEBUG("  HASHL = %08" PRIX32 "\r\n", XEMACPS_HASHL);
   TRACE_DEBUG("  HASHH = %08" PRIX32 "\r\n", XEMACPS_HASHH);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Adjust MAC configuration parameters for proper operation
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t zynq7000EthUpdateMacConfig(NetInterface *interface)
{
   uint32_t config;
   uint32_t clockCtrl;

   //Read network configuration register
   config = XEMACPS_NWCFG;

   //Read clock control register
   clockCtrl = XSLCR_GEM0_CLK_CTRL;
   clockCtrl &= ~(XSLCR_GEM0_CLK_CTRL_DIV1_MASK | XSLCR_GEM0_CLK_CTRL_DIV0_MASK);

   //1000BASE-T operation mode?
   if(interface->linkSpeed == NIC_LINK_SPEED_1GBPS)
   {
      //Update network configuration
      config |= XEMACPS_NWCFG_1000_MASK;
      config &= ~XEMACPS_NWCFG_100_MASK;

      //Update clock configuration
      clockCtrl |= (XPAR_PS7_ETHERNET_0_ENET_SLCR_1000MBPS_DIV1 << 20) & XSLCR_GEM0_CLK_CTRL_DIV1_MASK;
      clockCtrl |= (XPAR_PS7_ETHERNET_0_ENET_SLCR_1000MBPS_DIV0 << 8) & XSLCR_GEM0_CLK_CTRL_DIV0_MASK;
   }
   //100BASE-TX operation mode?
   else if(interface->linkSpeed == NIC_LINK_SPEED_100MBPS)
   {
      //Update network configuration
      config &= ~XEMACPS_NWCFG_1000_MASK;
      config |= XEMACPS_NWCFG_100_MASK;

      //Update clock configuration
      clockCtrl |= (XPAR_PS7_ETHERNET_0_ENET_SLCR_100MBPS_DIV1 << 20) & XSLCR_GEM0_CLK_CTRL_DIV1_MASK;
      clockCtrl |= (XPAR_PS7_ETHERNET_0_ENET_SLCR_100MBPS_DIV0 << 8) & XSLCR_GEM0_CLK_CTRL_DIV0_MASK;
   }
   //10BASE-T operation mode?
   else
   {
      //Update network configuration
      config &= ~XEMACPS_NWCFG_1000_MASK;
      config &= ~XEMACPS_NWCFG_100_MASK;

      //Update clock configuration
      clockCtrl |= (XPAR_PS7_ETHERNET_0_ENET_SLCR_10MBPS_DIV1 << 20) & XSLCR_GEM0_CLK_CTRL_DIV1_MASK;
      clockCtrl |= (XPAR_PS7_ETHERNET_0_ENET_SLCR_10MBPS_DIV0 << 8) & XSLCR_GEM0_CLK_CTRL_DIV0_MASK;
   }

   //Half-duplex or full-duplex mode?
   if(interface->duplexMode == NIC_FULL_DUPLEX_MODE)
   {
      config |= XEMACPS_NWCFG_FDEN_MASK;
   }
   else
   {
      config &= ~XEMACPS_NWCFG_FDEN_MASK;
   }

   //Write network configuration register
   XEMACPS_NWCFG = config;

   //Unlock SLCR
   XSLCR_UNLOCK = XSLCR_UNLOCK_KEY_VALUE;
   //Write clock control register
   XSLCR_GEM0_CLK_CTRL = clockCtrl;
   //Lock SLCR
   XSLCR_LOCK = XSLCR_LOCK_KEY_VALUE;

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

void zynq7000EthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data)
{
   uint32_t temp;

   //Valid opcode?
   if(opcode == SMI_OPCODE_WRITE)
   {
      //Set up a write operation
      temp = XEMACPS_PHYMNTNC_OP_MASK | XEMACPS_PHYMNTNC_OP_W_MASK;
      //PHY address
      temp |= (phyAddr << 23) & XEMACPS_PHYMNTNC_ADDR_MASK;
      //Register address
      temp |= (regAddr << 18) & XEMACPS_PHYMNTNC_REG_MASK;
      //Register value
      temp |= data & XEMACPS_PHYMNTNC_DATA_MASK;

      //Start a write operation
      XEMACPS_PHYMNTNC = temp;
      //Wait for the write to complete
      while((XEMACPS_NWSR & XEMACPS_NWSR_MDIOIDLE_MASK) == 0)
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

uint16_t zynq7000EthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr)
{
   uint16_t data;
   uint32_t temp;

   //Valid opcode?
   if(opcode == SMI_OPCODE_READ)
   {
      //Set up a read operation
      temp = XEMACPS_PHYMNTNC_OP_MASK | XEMACPS_PHYMNTNC_OP_R_MASK;
      //PHY address
      temp |= (phyAddr << 23) & XEMACPS_PHYMNTNC_ADDR_MASK;
      //Register address
      temp |= (regAddr << 18) & XEMACPS_PHYMNTNC_REG_MASK;

      //Start a read operation
      XEMACPS_PHYMNTNC = temp;
      //Wait for the read to complete
      while((XEMACPS_NWSR & XEMACPS_NWSR_MDIOIDLE_MASK) == 0)
      {
      }

      //Get register value
      data = XEMACPS_PHYMNTNC & XEMACPS_PHYMNTNC_DATA_MASK;
   }
   else
   {
      //The MAC peripheral only supports standard Clause 22 opcodes
      data = 0;
   }

   //Return the value of the PHY register
   return data;
}
