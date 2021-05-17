/**
 * @file mpfsxxx_eth1_driver.c
 * @brief PolarFire SoC Gigabit Ethernet MAC driver (MAC0 instance)
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
#include "mpfs_hal/mss_plic.h"
#include "mpfs_hal/mss_sysreg.h"
#include "drivers/mss_mac/mac_registers.h"
#include "drivers/mss_mac/pse_mac_regs.h"
#include "core/net.h"
#include "drivers/mac/mpfsxxx_eth1_driver.h"
#include "debug.h"

//Underlying network interface
static NetInterface *nicDriverInterface;

//TX buffer
static uint8_t txBuffer[MPFSXXX_ETH1_TX_BUFFER_COUNT][MPFSXXX_ETH1_TX_BUFFER_SIZE]
   __attribute__((aligned(8), __section__(MPFSXXX_ETH1_RAM_SECTION)));
//RX buffer
static uint8_t rxBuffer[MPFSXXX_ETH1_RX_BUFFER_COUNT][MPFSXXX_ETH1_RX_BUFFER_SIZE]
   __attribute__((aligned(8), __section__(MPFSXXX_ETH1_RAM_SECTION)));
//TX buffer descriptors
static MpfsxxxEth1TxBufferDesc txBufferDesc[MPFSXXX_ETH1_TX_BUFFER_COUNT]
   __attribute__((aligned(8), __section__(MPFSXXX_ETH1_RAM_SECTION)));
//RX buffer descriptors
static MpfsxxxEth1RxBufferDesc rxBufferDesc[MPFSXXX_ETH1_RX_BUFFER_COUNT]
   __attribute__((aligned(8), __section__(MPFSXXX_ETH1_RAM_SECTION)));

//Dummy TX buffer
static uint8_t dummyTxBuffer[MPFSXXX_ETH1_DUMMY_BUFFER_COUNT][MPFSXXX_ETH1_DUMMY_BUFFER_SIZE]
   __attribute__((aligned(8), __section__(MPFSXXX_ETH1_RAM_SECTION)));
//Dummy RX buffer
static uint8_t dummyRxBuffer[MPFSXXX_ETH1_DUMMY_BUFFER_COUNT][MPFSXXX_ETH1_DUMMY_BUFFER_SIZE]
   __attribute__((aligned(8), __section__(MPFSXXX_ETH1_RAM_SECTION)));
//Dummy TX buffer descriptors
static MpfsxxxEth1TxBufferDesc dummyTxBufferDesc[MPFSXXX_ETH1_DUMMY_BUFFER_COUNT]
   __attribute__((aligned(8), __section__(MPFSXXX_ETH1_RAM_SECTION)));
//Dummy RX buffer descriptors
static MpfsxxxEth1RxBufferDesc dummyRxBufferDesc[MPFSXXX_ETH1_DUMMY_BUFFER_COUNT]
   __attribute__((aligned(8), __section__(MPFSXXX_ETH1_RAM_SECTION)));

//TX buffer index
static uint_t txBufferIndex;
//RX buffer index
static uint_t rxBufferIndex;


/**
 * @brief MPFSxxx Ethernet MAC driver (MAC0 instance)
 **/

const NicDriver mpfsxxxEth1Driver =
{
   NIC_TYPE_ETHERNET,
   ETH_MTU,
   mpfsxxxEth1Init,
   mpfsxxxEth1Tick,
   mpfsxxxEth1EnableIrq,
   mpfsxxxEth1DisableIrq,
   mpfsxxxEth1EventHandler,
   mpfsxxxEth1SendPacket,
   mpfsxxxEth1UpdateMacAddrFilter,
   mpfsxxxEth1UpdateMacConfig,
   mpfsxxxEth1WritePhyReg,
   mpfsxxxEth1ReadPhyReg,
   TRUE,
   TRUE,
   TRUE,
   FALSE
};


/**
 * @brief MPFSxxx Ethernet MAC initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t mpfsxxxEth1Init(NetInterface *interface)
{
   error_t error;
   volatile uint32_t temp;

   //Debug message
   TRACE_INFO("Initializing MPFSxxx Ethernet MAC (MAC0)...\r\n");

   //Save underlying network interface
   nicDriverInterface = interface;

   //Enable MAC0 peripheral clock
   SYSREG->SUBBLK_CLOCK_CR |= 2U;

   //Reset MAC0 peripheral
   SYSREG->SOFT_RESET_CR |= 2U;
   SYSREG->SOFT_RESET_CR &= ~2U;

   //Disable transmit and receive circuits
   MAC0->NETWORK_CONTROL = 0;

   //GPIO configuration
   mpfsxxxEth1InitGpio(interface);

   //Configure MDC clock speed
   MAC0->NETWORK_CONFIG = (1 << GEM_DATA_BUS_WIDTH_SHIFT) |
      (2 << GEM_MDC_CLOCK_DIVISOR_SHIFT);

   //Enable management port (MDC and MDIO)
   MAC0->NETWORK_CONTROL |= GEM_MAN_PORT_EN;

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
   MAC0->SPEC_ADD1_BOTTOM = interface->macAddr.w[0] | (interface->macAddr.w[1] << 16);
   MAC0->SPEC_ADD1_TOP = interface->macAddr.w[2];

   //The MAC supports 3 additional addresses for unicast perfect filtering
   MAC0->SPEC_ADD2_BOTTOM = 0;
   MAC0->SPEC_ADD3_BOTTOM = 0;
   MAC0->SPEC_ADD4_BOTTOM = 0;

   //Initialize hash table
   MAC0->HASH_BOTTOM = 0;
   MAC0->HASH_TOP = 0;

   //Configure the receive filter
   MAC0->NETWORK_CONFIG |= GEM_RECEIVE_1536_BYTE_FRAMES | GEM_MULTICAST_HASH_ENABLE;

   //Set RX buffer size
   temp = ((MPFSXXX_ETH1_RX_BUFFER_SIZE / 64) << GEM_RX_BUF_SIZE_SHIFT) &
      GEM_RX_BUF_SIZE;

   //Select 64-bit bus width
   temp |= GEM_DMA_ADDR_BUS_WIDTH_1;
   //Use extended buffer descriptors
   temp |= GEM_TX_BD_EXTENDED_MODE_EN | GEM_RX_BD_EXTENDED_MODE_EN;
   //Use full configured addressable space for transmit and receive packet buffers
   temp |= GEM_TX_PBUF_SIZE | GEM_RX_PBUF_SIZE;

   //DMA configuration
   MAC0->DMA_CONFIG = temp;
   MAC0->DMA_RXBUF_SIZE_Q1 = MPFSXXX_ETH1_DUMMY_BUFFER_SIZE / 64;
   MAC0->DMA_RXBUF_SIZE_Q1 = MPFSXXX_ETH1_DUMMY_BUFFER_SIZE / 64;
   MAC0->DMA_RXBUF_SIZE_Q1 = MPFSXXX_ETH1_DUMMY_BUFFER_SIZE / 64;

   //Initialize buffer descriptors
   mpfsxxxEth1InitBufferDesc(interface);

   //Clear transmit status register
   MAC0->TRANSMIT_STATUS = GEM_TX_RESP_NOT_OK | GEM_STAT_TRANSMIT_UNDER_RUN |
      GEM_STAT_TRANSMIT_COMPLETE | GEM_STAT_AMBA_ERROR | GEM_TRANSMIT_GO |
      GEM_RETRY_LIMIT_EXCEEDED | GEM_COLLISION_OCCURRED | GEM_USED_BIT_READ;

   //Clear receive status register
   MAC0->RECEIVE_STATUS = GEM_RX_RESP_NOT_OK | GEM_RECEIVE_OVERRUN |
      GEM_FRAME_RECEIVED | GEM_BUFFER_NOT_AVAILABLE;

   //First disable all interrupts
   MAC0->INT_DISABLE = 0xFFFFFFFF;
   MAC0->INT_Q1_DISABLE = 0xFFFFFFFF;
   MAC0->INT_Q2_DISABLE = 0xFFFFFFFF;
   MAC0->INT_Q3_DISABLE = 0xFFFFFFFF;

   //Only the desired ones are enabled
   MAC0->INT_ENABLE = GEM_RESP_NOT_OK_INT |
      GEM_RECEIVE_OVERRUN_INT | GEM_TRANSMIT_COMPLETE | GEM_AMBA_ERROR |
      GEM_RETRY_LIMIT_EXCEEDED_OR_LATE_COLLISION | GEM_TRANSMIT_UNDER_RUN |
      GEM_RX_USED_BIT_READ | GEM_RECEIVE_COMPLETE;

   //Read interrupt status register to clear any pending interrupt
   temp = MAC0->INT_STATUS;

   //Configure interrupt priority
   PLIC_SetPriority(MAC0_INT_PLIC, MPFSXXX_ETH1_IRQ_PRIORITY);

   //Enable the transmitter and the receiver
   MAC0->NETWORK_CONTROL |= GEM_ENABLE_TRANSMIT | GEM_ENABLE_RECEIVE;

   //Accept any packets from the upper layer
   osSetEvent(&interface->nicTxEvent);

   //Successful initialization
   return NO_ERROR;
}


//MPFS-ICICLE-KIT-ES evaluation board?
#if defined(USE_MPFS_ICICLE_KIT_ES)

/**
 * @brief GPIO configuration
 * @param[in] interface Underlying network interface
 **/

void mpfsxxxEth1InitGpio(NetInterface *interface)
{
   //Select SGMII operation mode
}

#endif


/**
 * @brief Initialize buffer descriptors
 * @param[in] interface Underlying network interface
 **/

void mpfsxxxEth1InitBufferDesc(NetInterface *interface)
{
   uint_t i;
   uint64_t address;

   //Initialize TX buffer descriptors
   for(i = 0; i < MPFSXXX_ETH1_TX_BUFFER_COUNT; i++)
   {
      //Calculate the address of the current TX buffer
      address = (uint64_t) txBuffer[i];

      //Write the address to the descriptor entry
      txBufferDesc[i].addrLow = (uint32_t) address;
      txBufferDesc[i].addrHigh = (uint32_t) (address >> 32);

      //Initialize status field
      txBufferDesc[i].status = MAC_TX_USED;

      //Clear unused fields
      txBufferDesc[i].reserved = 0;
      txBufferDesc[i].nanoSeconds = 0;
      txBufferDesc[i].seconds = 0;
   }

   //Mark the last descriptor entry with the wrap flag
   txBufferDesc[i - 1].status |= MAC_TX_WRAP;
   //Initialize TX buffer index
   txBufferIndex = 0;

   //Initialize RX buffer descriptors
   for(i = 0; i < MPFSXXX_ETH1_RX_BUFFER_COUNT; i++)
   {
      //Calculate the address of the current RX buffer
      address = (uint64_t) rxBuffer[i];

      //Write the address to the descriptor entry
      rxBufferDesc[i].addrLow = (uint32_t) address & MAC_RX_ADDRESS;
      rxBufferDesc[i].addrHigh = (uint32_t) (address >> 32);

      //Clear status field
      rxBufferDesc[i].status = 0;

      //Clear unused fields
      rxBufferDesc[i].reserved = 0;
      rxBufferDesc[i].nanoSeconds = 0;
      rxBufferDesc[i].seconds = 0;
   }

   //Mark the last descriptor entry with the wrap flag
   rxBufferDesc[i - 1].addrLow |= MAC_RX_WRAP;
   //Initialize RX buffer index
   rxBufferIndex = 0;

   //Initialize dummy TX buffer descriptors
   for(i = 0; i < MPFSXXX_ETH1_DUMMY_BUFFER_COUNT; i++)
   {
      //Calculate the address of the current TX buffer
      address = (uint64_t) dummyTxBuffer[i];

      //Write the address to the descriptor entry
      dummyTxBufferDesc[i].addrLow = (uint32_t) address;
      dummyTxBufferDesc[i].addrHigh = (uint32_t) (address >> 32);

      //Initialize status field
      dummyTxBufferDesc[i].status = MAC_TX_USED;

      //Clear unused fields
      dummyTxBufferDesc[i].reserved = 0;
      dummyTxBufferDesc[i].nanoSeconds = 0;
      dummyTxBufferDesc[i].seconds = 0;
   }

   //Mark the last descriptor entry with the wrap flag
   dummyTxBufferDesc[i - 1].status |= MAC_TX_WRAP;

   //Initialize dummy RX buffer descriptors
   for(i = 0; i < MPFSXXX_ETH1_DUMMY_BUFFER_COUNT; i++)
   {
      //Calculate the address of the current RX buffer
      address = (uint64_t) dummyRxBuffer[i];

      //Write the address to the descriptor entry
      dummyRxBufferDesc[i].addrLow = ((uint32_t) address & MAC_RX_ADDRESS) | MAC_RX_OWNERSHIP;
      dummyRxBufferDesc[i].addrHigh = (uint32_t) (address >> 32);

      //Clear status field
      dummyRxBufferDesc[i].status = 0;

      //Clear unused fields
      dummyRxBufferDesc[i].reserved = 0;
      dummyRxBufferDesc[i].nanoSeconds = 0;
      dummyRxBufferDesc[i].seconds = 0;
   }

   //Mark the last descriptor entry with the wrap flag
   dummyRxBufferDesc[i - 1].addrLow |= MAC_RX_WRAP;

   //Start location of the TX descriptor list
   MAC0->TRANSMIT_Q_PTR = (uint32_t) ((uint64_t) txBufferDesc);
   MAC0->UPPER_TX_Q_BASE_ADDR = (uint32_t) ((uint64_t) txBufferDesc >> 32);

   MAC0->TRANSMIT_Q1_PTR = (uint32_t) ((uint64_t) dummyTxBufferDesc) | 1;
   MAC0->TRANSMIT_Q2_PTR = (uint32_t) ((uint64_t) dummyTxBufferDesc) | 1;
   MAC0->TRANSMIT_Q3_PTR = (uint32_t) ((uint64_t) dummyTxBufferDesc) | 1;

   //Start location of the RX descriptor list
   MAC0->RECEIVE_Q_PTR = (uint32_t) ((uint64_t) rxBufferDesc);
   MAC0->UPPER_RX_Q_BASE_ADDR = (uint32_t) ((uint64_t) rxBufferDesc >> 32);

   MAC0->RECEIVE_Q1_PTR = (uint32_t) ((uint64_t) dummyRxBufferDesc) | 1;
   MAC0->RECEIVE_Q2_PTR = (uint32_t) ((uint64_t) dummyRxBufferDesc) | 1;
   MAC0->RECEIVE_Q3_PTR = (uint32_t) ((uint64_t) dummyRxBufferDesc) | 1;
}


/**
 * @brief MPFSxxx Ethernet MAC timer handler
 *
 * This routine is periodically called by the TCP/IP stack to handle periodic
 * operations such as polling the link state
 *
 * @param[in] interface Underlying network interface
 **/

void mpfsxxxEth1Tick(NetInterface *interface)
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

void mpfsxxxEth1EnableIrq(NetInterface *interface)
{
   //Enable Ethernet MAC interrupts
   PLIC_EnableIRQ(MAC0_INT_PLIC);


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

void mpfsxxxEth1DisableIrq(NetInterface *interface)
{
   //Disable Ethernet MAC interrupts
   PLIC_DisableIRQ(MAC0_INT_PLIC);


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
 * @brief MPFSxxx Ethernet MAC interrupt service routine
 **/

uint8_t mac0_int_plic_IRQHandler(void)
{
   bool_t flag;
   volatile uint32_t isr;
   volatile uint32_t tsr;
   volatile uint32_t rsr;

   //Interrupt service routine prologue
   osEnterIsr();

   //This flag will be set if a higher priority task must be woken
   flag = FALSE;

   //Each time the software reads INT_STATUS, it has to check the contents
   //of TRANSMIT_STATUS, RECEIVE_STATUS and NETWORK_STATUS
   isr = MAC0->INT_Q1_STATUS;
   isr = MAC0->INT_Q2_STATUS;
   isr = MAC0->INT_Q3_STATUS;
   isr = MAC0->INT_STATUS;
   tsr = MAC0->TRANSMIT_STATUS;
   rsr = MAC0->RECEIVE_STATUS;

   //Packet transmitted?
   if((tsr & (GEM_TX_RESP_NOT_OK | GEM_STAT_TRANSMIT_UNDER_RUN |
      GEM_STAT_TRANSMIT_COMPLETE | GEM_STAT_AMBA_ERROR | GEM_TRANSMIT_GO |
      GEM_RETRY_LIMIT_EXCEEDED | GEM_COLLISION_OCCURRED | GEM_USED_BIT_READ)) != 0)
   {
      //Only clear TRANSMIT_STATUS flags that are currently set
      MAC0->TRANSMIT_STATUS = tsr;

      //Check whether the TX buffer is available for writing
      if((txBufferDesc[txBufferIndex].status & MAC_TX_USED) != 0)
      {
         //Notify the TCP/IP stack that the transmitter is ready to send
         flag |= osSetEventFromIsr(&nicDriverInterface->nicTxEvent);
      }
   }

   //Packet received?
   if((rsr & (GEM_RX_RESP_NOT_OK | GEM_RECEIVE_OVERRUN | GEM_FRAME_RECEIVED |
      GEM_BUFFER_NOT_AVAILABLE)) != 0)
   {
      //Set event flag
      nicDriverInterface->nicEvent = TRUE;
      //Notify the TCP/IP stack of the event
      flag |= osSetEventFromIsr(&netEvent);
   }

   //Interrupt service routine epilogue
   osExitIsr(flag);

   //Return from IRQ handler
   return EXT_IRQ_KEEP_ENABLED;
}


/**
 * @brief MPFSxxx Ethernet MAC event handler
 * @param[in] interface Underlying network interface
 **/

void mpfsxxxEth1EventHandler(NetInterface *interface)
{
   error_t error;
   uint32_t rsr;

   //Read receive status
   rsr = MAC0->RECEIVE_STATUS;

   //Packet received?
   if((rsr & (GEM_RX_RESP_NOT_OK | GEM_RECEIVE_OVERRUN | GEM_FRAME_RECEIVED |
      GEM_BUFFER_NOT_AVAILABLE)) != 0)
   {
      //Only clear RECEIVE_STATUS flags that are currently set
      MAC0->RECEIVE_STATUS = rsr;

      //Process all pending packets
      do
      {
         //Read incoming packet
         error = mpfsxxxEth1ReceivePacket(interface);

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

error_t mpfsxxxEth1SendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary)
{
   size_t length;

   //Retrieve the length of the packet
   length = netBufferGetLength(buffer) - offset;

   //Check the frame length
   if(length > MPFSXXX_ETH1_TX_BUFFER_SIZE)
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
      //Report an error
      return ERROR_INVALID_LENGTH;
   }

   //Make sure the current buffer is available for writing
   if((txBufferDesc[txBufferIndex].status & MAC_TX_USED) == 0)
   {
      return ERROR_FAILURE;
   }

   //Copy user data to the transmit buffer
   netBufferRead(txBuffer[txBufferIndex], buffer, offset, length);

   //Set the necessary flags in the descriptor entry
   if(txBufferIndex < (MPFSXXX_ETH1_TX_BUFFER_COUNT - 1))
   {
      //Write the status word
      txBufferDesc[txBufferIndex].status = MAC_TX_LAST |
         (length & MAC_TX_LENGTH);

      //Point to the next buffer
      txBufferIndex++;
   }
   else
   {
      //Write the status word
      txBufferDesc[txBufferIndex].status = MAC_TX_WRAP | MAC_TX_LAST |
         (length & MAC_TX_LENGTH);

      //Wrap around
      txBufferIndex = 0;
   }

   //Set the TRANSMIT_START bit to initiate transmission
   MAC0->NETWORK_CONTROL |= GEM_TRANSMIT_START;

   //Check whether the next buffer is available for writing
   if((txBufferDesc[txBufferIndex].status & MAC_TX_USED) != 0)
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

error_t mpfsxxxEth1ReceivePacket(NetInterface *interface)
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
   for(i = 0; i < MPFSXXX_ETH1_RX_BUFFER_COUNT; i++)
   {
      //Point to the current entry
      j = rxBufferIndex + i;

      //Wrap around to the beginning of the buffer if necessary
      if(j >= MPFSXXX_ETH1_RX_BUFFER_COUNT)
      {
         j -= MPFSXXX_ETH1_RX_BUFFER_COUNT;
      }

      //No more entries to process?
      if((rxBufferDesc[j].addrLow & MAC_RX_OWNERSHIP) == 0)
      {
         //Stop processing
         break;
      }

      //A valid SOF has been found?
      if((rxBufferDesc[j].status & MAC_RX_SOF) != 0)
      {
         //Save the position of the SOF
         sofIndex = i;
      }

      //A valid EOF has been found?
      if((rxBufferDesc[j].status & MAC_RX_EOF) != 0 && sofIndex != UINT_MAX)
      {
         //Save the position of the EOF
         eofIndex = i;
         //Retrieve the length of the frame
         size = rxBufferDesc[j].status & MAC_RX_LENGTH;
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
         n = MIN(size, MPFSXXX_ETH1_RX_BUFFER_SIZE);
         //Copy data from receive buffer
         osMemcpy(temp + length, rxBuffer[rxBufferIndex], n);
         //Update byte counters
         length += n;
         size -= n;
      }

      //Mark the current buffer as free
      rxBufferDesc[rxBufferIndex].addrLow &= ~MAC_RX_OWNERSHIP;

      //Point to the following entry
      rxBufferIndex++;

      //Wrap around to the beginning of the buffer if necessary
      if(rxBufferIndex >= MPFSXXX_ETH1_RX_BUFFER_COUNT)
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

error_t mpfsxxxEth1UpdateMacAddrFilter(NetInterface *interface)
{
   uint_t i;
   uint_t j;
   uint_t k;
   uint8_t *p;
   uint32_t hashTable[2];
   MacAddr unicastMacAddr[3];
   MacFilterEntry *entry;

   //Debug message
   TRACE_DEBUG("Updating MAC filter...\r\n");

   //Set the MAC address of the station
   MAC0->SPEC_ADD1_BOTTOM = interface->macAddr.w[0] | (interface->macAddr.w[1] << 16);
   MAC0->SPEC_ADD1_TOP = interface->macAddr.w[2];

   //The MAC supports 3 additional addresses for unicast perfect filtering
   unicastMacAddr[0] = MAC_UNSPECIFIED_ADDR;
   unicastMacAddr[1] = MAC_UNSPECIFIED_ADDR;
   unicastMacAddr[2] = MAC_UNSPECIFIED_ADDR;

   //The hash table is used for multicast address filtering
   hashTable[0] = 0;
   hashTable[1] = 0;

   //The MAC address filter contains the list of MAC addresses to accept
   //when receiving an Ethernet frame
   for(i = 0, j = 0; i < MAC_ADDR_FILTER_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->macAddrFilter[i];

      //Valid entry?
      if(entry->refCount > 0)
      {
         //Multicast address?
         if(macIsMulticastAddr(&entry->addr))
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
         else
         {
            //Up to 3 additional MAC addresses can be specified
            if(j < 3)
            {
               //Save the unicast address
               unicastMacAddr[j] = entry->addr;
            }
            else
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

            //Increment the number of unicast addresses
            j++;
         }
      }
   }

   //Configure the first unicast address filter
   if(j >= 1)
   {
      //The address is activated when SAT register is written
      MAC0->SPEC_ADD2_BOTTOM = unicastMacAddr[0].w[0] | (unicastMacAddr[0].w[1] << 16);
      MAC0->SPEC_ADD2_TOP = unicastMacAddr[0].w[2];
   }
   else
   {
      //The address is deactivated when SAB register is written
      MAC0->SPEC_ADD2_BOTTOM = 0;
   }

   //Configure the second unicast address filter
   if(j >= 2)
   {
      //The address is activated when SAT register is written
      MAC0->SPEC_ADD3_BOTTOM = unicastMacAddr[1].w[0] | (unicastMacAddr[1].w[1] << 16);
      MAC0->SPEC_ADD3_TOP = unicastMacAddr[1].w[2];
   }
   else
   {
      //The address is deactivated when SAB register is written
      MAC0->SPEC_ADD3_BOTTOM = 0;
   }

   //Configure the third unicast address filter
   if(j >= 3)
   {
      //The address is activated when SAT register is written
      MAC0->SPEC_ADD4_BOTTOM = unicastMacAddr[2].w[0] | (unicastMacAddr[2].w[1] << 16);
      MAC0->SPEC_ADD4_TOP = unicastMacAddr[2].w[2];
   }
   else
   {
      //The address is deactivated when SAB register is written
      MAC0->SPEC_ADD4_BOTTOM = 0;
   }

   //The perfect MAC filter supports only 3 unicast addresses
   if(j >= 4)
   {
      MAC0->NETWORK_CONFIG |= GEM_UNICAST_HASH_ENABLE;
   }
   else
   {
      MAC0->NETWORK_CONFIG &= ~GEM_UNICAST_HASH_ENABLE;
   }

   //Configure the multicast address filter
   MAC0->HASH_BOTTOM = hashTable[0];
   MAC0->HASH_TOP = hashTable[1];

   //Debug message
   TRACE_DEBUG("  HASH_BOTTOM = %08" PRIX32 "\r\n", MAC0->HASH_BOTTOM);
   TRACE_DEBUG("  HASH_TOP = %08" PRIX32 "\r\n", MAC0->HASH_TOP);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Adjust MAC configuration parameters for proper operation
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t mpfsxxxEth1UpdateMacConfig(NetInterface *interface)
{
   uint32_t config;

   //Read network configuration register
   config = MAC0->NETWORK_CONFIG;

   //1000BASE-T operation mode?
   if(interface->linkSpeed == NIC_LINK_SPEED_1GBPS)
   {
      config |= GEM_GIGABIT_MODE_ENABLE;
      config &= ~GEM_SPEED;
   }
   //100BASE-TX operation mode?
   else if(interface->linkSpeed == NIC_LINK_SPEED_100MBPS)
   {
      config &= ~GEM_GIGABIT_MODE_ENABLE;
      config |= GEM_SPEED;
   }
   //10BASE-T operation mode?
   else
   {
      config &= ~GEM_GIGABIT_MODE_ENABLE;
      config &= ~GEM_SPEED;
   }

   //Half-duplex or full-duplex mode?
   if(interface->duplexMode == NIC_FULL_DUPLEX_MODE)
   {
      config |= GEM_FULL_DUPLEX;
   }
   else
   {
      config &= ~GEM_FULL_DUPLEX;
   }

   //Write configuration value back to NCFGR register
   MAC0->NETWORK_CONFIG = config;

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

void mpfsxxxEth1WritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data)
{
   uint32_t temp;

   //Valid opcode?
   if(opcode == SMI_OPCODE_WRITE)
   {
      //Set up a write operation
      temp = GEM_WRITE1 | (GEM_PHY_OP_CL22_WRITE << GEM_OPERATION_SHIFT) |
         (2 << GEM_WRITE10_SHIFT);

      //PHY address
      temp |= (phyAddr << GEM_PHY_ADDRESS_SHIFT) & GEM_PHY_ADDRESS;
      //Register address
      temp |= (regAddr << GEM_REGISTER_ADDRESS_SHIFT) & GEM_REGISTER_ADDRESS;
      //Register value
      temp |= data;

      //Start a write operation
      MAC0->PHY_MANAGEMENT = temp;
      //Wait for the write to complete
      while((MAC0->NETWORK_STATUS & GEM_MAN_DONE) == 0)
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

uint16_t mpfsxxxEth1ReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr)
{
   uint16_t data;
   uint32_t temp;

   //Valid opcode?
   if(opcode == SMI_OPCODE_READ)
   {
      //Set up a read operation
      temp = GEM_WRITE1 | (GEM_PHY_OP_CL22_READ << GEM_OPERATION_SHIFT) |
         (2 << GEM_WRITE10_SHIFT);

      //PHY address
      temp |= (phyAddr << GEM_PHY_ADDRESS_SHIFT) & GEM_PHY_ADDRESS;
      //Register address
      temp |= (regAddr << GEM_REGISTER_ADDRESS_SHIFT) & GEM_REGISTER_ADDRESS;

      //Start a read operation
      MAC0->PHY_MANAGEMENT = temp;
      //Wait for the read to complete
      while((MAC0->NETWORK_STATUS & GEM_MAN_DONE) == 0)
      {
      }

      //Get register value
      data = (uint16_t) MAC0->PHY_MANAGEMENT;
   }
   else
   {
      //The MAC peripheral only supports standard Clause 22 opcodes
      data = 0;
   }

   //Return the value of the PHY register
   return data;
}
