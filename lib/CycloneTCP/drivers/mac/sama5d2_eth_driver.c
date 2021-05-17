/**
 * @file sama5d2_eth_driver.c
 * @brief SAMA5D2 Ethernet MAC driver
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
#include "chip.h"
#include "peripherals/aic.h"
#include "peripherals/pio.h"
#include "core/net.h"
#include "drivers/mac/sama5d2_eth_driver.h"
#include "debug.h"

//Underlying network interface
static NetInterface *nicDriverInterface;

//IAR EWARM compiler?
#if defined(__ICCARM__)

//TX buffer
#pragma data_alignment = 8
#pragma location = SAMA5D2_ETH_RAM_SECTION
static uint8_t txBuffer[SAMA5D2_ETH_TX_BUFFER_COUNT][SAMA5D2_ETH_TX_BUFFER_SIZE];
//RX buffer
#pragma data_alignment = 8
#pragma location = SAMA5D2_ETH_RAM_SECTION
static uint8_t rxBuffer[SAMA5D2_ETH_RX_BUFFER_COUNT][SAMA5D2_ETH_RX_BUFFER_SIZE];
//TX buffer descriptors
#pragma data_alignment = 4
#pragma location = SAMA5D2_ETH_RAM_SECTION
static Sama5d2TxBufferDesc txBufferDesc[SAMA5D2_ETH_TX_BUFFER_COUNT];
//RX buffer descriptors
#pragma data_alignment = 4
#pragma location = SAMA5D2_ETH_RAM_SECTION
static Sama5d2RxBufferDesc rxBufferDesc[SAMA5D2_ETH_RX_BUFFER_COUNT];

//Dummy TX buffer
#pragma data_alignment = 8
#pragma location = SAMA5D2_ETH_RAM_SECTION
static uint8_t dummyTxBuffer[SAMA5D2_ETH_DUMMY_BUFFER_COUNT][SAMA5D2_ETH_DUMMY_BUFFER_SIZE];
//Dummy RX buffer
#pragma data_alignment = 8
#pragma location = SAMA5D2_ETH_RAM_SECTION
static uint8_t dummyRxBuffer[SAMA5D2_ETH_DUMMY_BUFFER_COUNT][SAMA5D2_ETH_DUMMY_BUFFER_SIZE];
//Dummy TX buffer descriptors
#pragma data_alignment = 4
#pragma location = SAMA5D2_ETH_RAM_SECTION
static Sama5d2TxBufferDesc dummyTxBufferDesc[SAMA5D2_ETH_DUMMY_BUFFER_COUNT];
//Dummy RX buffer descriptors
#pragma data_alignment = 4
#pragma location = SAMA5D2_ETH_RAM_SECTION
static Sama5d2RxBufferDesc dummyRxBufferDesc[SAMA5D2_ETH_DUMMY_BUFFER_COUNT];

//GCC compiler?
#else

//TX buffer
static uint8_t txBuffer[SAMA5D2_ETH_TX_BUFFER_COUNT][SAMA5D2_ETH_TX_BUFFER_SIZE]
   __attribute__((aligned(8), __section__(SAMA5D2_ETH_RAM_SECTION)));
//RX buffer
static uint8_t rxBuffer[SAMA5D2_ETH_RX_BUFFER_COUNT][SAMA5D2_ETH_RX_BUFFER_SIZE]
   __attribute__((aligned(8), __section__(SAMA5D2_ETH_RAM_SECTION)));
//TX buffer descriptors
static Sama5d2TxBufferDesc txBufferDesc[SAMA5D2_ETH_TX_BUFFER_COUNT]
   __attribute__((aligned(4), __section__(SAMA5D2_ETH_RAM_SECTION)));
//RX buffer descriptors
static Sama5d2RxBufferDesc rxBufferDesc[SAMA5D2_ETH_RX_BUFFER_COUNT]
   __attribute__((aligned(4), __section__(SAMA5D2_ETH_RAM_SECTION)));

//Dummy TX buffer
static uint8_t dummyTxBuffer[SAMA5D2_ETH_DUMMY_BUFFER_COUNT][SAMA5D2_ETH_DUMMY_BUFFER_SIZE]
   __attribute__((aligned(8), __section__(SAMA5D2_ETH_RAM_SECTION)));
//Dummy RX buffer
static uint8_t dummyRxBuffer[SAMA5D2_ETH_DUMMY_BUFFER_COUNT][SAMA5D2_ETH_DUMMY_BUFFER_SIZE]
   __attribute__((aligned(8), __section__(SAMA5D2_ETH_RAM_SECTION)));
//Dummy TX buffer descriptors
static Sama5d2TxBufferDesc dummyTxBufferDesc[SAMA5D2_ETH_DUMMY_BUFFER_COUNT]
   __attribute__((aligned(4), __section__(SAMA5D2_ETH_RAM_SECTION)));
//Dummy RX buffer descriptors
static Sama5d2RxBufferDesc dummyRxBufferDesc[SAMA5D2_ETH_DUMMY_BUFFER_COUNT]
   __attribute__((aligned(4), __section__(SAMA5D2_ETH_RAM_SECTION)));

#endif

//TX buffer index
static uint_t txBufferIndex;
//RX buffer index
static uint_t rxBufferIndex;


/**
 * @brief SAMA5D2 Ethernet MAC driver
 **/

const NicDriver sama5d2EthDriver =
{
   NIC_TYPE_ETHERNET,
   ETH_MTU,
   sama5d2EthInit,
   sama5d2EthTick,
   sama5d2EthEnableIrq,
   sama5d2EthDisableIrq,
   sama5d2EthEventHandler,
   sama5d2EthSendPacket,
   sama5d2EthUpdateMacAddrFilter,
   sama5d2EthUpdateMacConfig,
   sama5d2EthWritePhyReg,
   sama5d2EthReadPhyReg,
   TRUE,
   TRUE,
   TRUE,
   FALSE
};


/**
 * @brief SAMA5D2 Ethernet MAC initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t sama5d2EthInit(NetInterface *interface)
{
   error_t error;
   volatile uint32_t status;

   //Debug message
   TRACE_INFO("Initializing SAMA5D2 Ethernet MAC...\r\n");

   //Save underlying network interface
   nicDriverInterface = interface;

   //Enable GMAC peripheral clock
   PMC->PMC_PCER0 = (1 << ID_GMAC0);

   //Disable transmit and receive circuits
   GMAC0->GMAC_NCR = 0;

   //GPIO configuration
   sama5d2EthInitGpio(interface);

   //Configure MDC clock speed
   GMAC0->GMAC_NCFGR = GMAC_NCFGR_CLK_MCK_96;
   //Enable management port (MDC and MDIO)
   GMAC0->GMAC_NCR |= GMAC_NCR_MPE;

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
   GMAC0->GMAC_SA[0].GMAC_SAB = interface->macAddr.w[0] | (interface->macAddr.w[1] << 16);
   GMAC0->GMAC_SA[0].GMAC_SAT = interface->macAddr.w[2];

   //The MAC supports 3 additional addresses for unicast perfect filtering
   GMAC0->GMAC_SA[1].GMAC_SAB = 0;
   GMAC0->GMAC_SA[2].GMAC_SAB = 0;
   GMAC0->GMAC_SA[3].GMAC_SAB = 0;

   //Initialize hash table
   GMAC0->GMAC_HRB = 0;
   GMAC0->GMAC_HRT = 0;

   //Configure the receive filter
   GMAC0->GMAC_NCFGR |= GMAC_NCFGR_MAXFS | GMAC_NCFGR_MTIHEN;

   //DMA configuration
   GMAC0->GMAC_DCFGR = GMAC_DCFGR_DRBS(SAMA5D2_ETH_RX_BUFFER_SIZE / 64) |
      GMAC_DCFGR_TXPBMS | GMAC_DCFGR_RXBMS_FULL | GMAC_DCFGR_FBLDO_INCR4;

   GMAC0->GMAC_RBSRPQ[0] = GMAC_RBSRPQ_RBS(SAMA5D2_ETH_DUMMY_BUFFER_SIZE / 64);
   GMAC0->GMAC_RBSRPQ[1] = GMAC_RBSRPQ_RBS(SAMA5D2_ETH_DUMMY_BUFFER_SIZE / 64);

   //Initialize buffer descriptors
   sama5d2EthInitBufferDesc(interface);

   //Clear transmit status register
   GMAC0->GMAC_TSR = GMAC_TSR_HRESP | GMAC_TSR_TXCOMP | GMAC_TSR_TFC |
      GMAC_TSR_TXGO | GMAC_TSR_RLE | GMAC_TSR_COL | GMAC_TSR_UBR;
   //Clear receive status register
   GMAC0->GMAC_RSR = GMAC_RSR_HNO | GMAC_RSR_RXOVR | GMAC_RSR_REC | GMAC_RSR_BNA;

   //First disable all GMAC interrupts
   GMAC0->GMAC_IDR = 0xFFFFFFFF;
   GMAC0->GMAC_IDRPQ[0] = 0xFFFFFFFF;
   GMAC0->GMAC_IDRPQ[1] = 0xFFFFFFFF;

   //Only the desired ones are enabled
   GMAC0->GMAC_IER = GMAC_IER_HRESP | GMAC_IER_ROVR | GMAC_IER_TCOMP | GMAC_IER_TFC |
      GMAC_IER_RLEX | GMAC_IER_TUR | GMAC_IER_RXUBR | GMAC_IER_RCOMP;

   //Read GMAC ISR register to clear any pending interrupt
   status = GMAC0->GMAC_ISR;

   //Register interrupt handler
   aic_set_source_vector(ID_GMAC0, sama5d2EthIrqHandler);

   //Configure interrupt priority
   aic_configure(ID_GMAC0, AIC_SMR_SRCTYPE_INT_LEVEL_SENSITIVE |
      AIC_SMR_PRIOR(SAMA5D2_ETH_IRQ_PRIORITY));

   //Enable the GMAC to transmit and receive data
   GMAC0->GMAC_NCR |= GMAC_NCR_TXEN | GMAC_NCR_RXEN;

   //Accept any packets from the upper layer
   osSetEvent(&interface->nicTxEvent);

   //Successful initialization
   return NO_ERROR;
}


//SAMA5D2-Xplained-Ultra evaluation board?
#if defined(CONFIG_BOARD_SAMA5D2_XPLAINED)

/**
 * @brief GPIO configuration
 * @param[in] interface Underlying network interface
 **/

void sama5d2EthInitGpio(NetInterface *interface)
{
   struct _pin rmiiPins[] = PINS_GMAC_RMII_IOS3;

   //Configure RMII pins
   pio_configure(rmiiPins, arraysize(rmiiPins));

   //Select RMII operation mode
   GMAC0->GMAC_UR = GMAC_UR_RMII;
}

#endif


/**
 * @brief Initialize buffer descriptors
 * @param[in] interface Underlying network interface
 **/

void sama5d2EthInitBufferDesc(NetInterface *interface)
{
   uint_t i;
   uint32_t address;

   //Initialize TX buffer descriptors
   for(i = 0; i < SAMA5D2_ETH_TX_BUFFER_COUNT; i++)
   {
      //Calculate the address of the current TX buffer
      address = (uint32_t) txBuffer[i];
      //Write the address to the descriptor entry
      txBufferDesc[i].address = address;
      //Initialize status field
      txBufferDesc[i].status = GMAC_TX_USED;
   }

   //Mark the last descriptor entry with the wrap flag
   txBufferDesc[i - 1].status |= GMAC_TX_WRAP;
   //Initialize TX buffer index
   txBufferIndex = 0;

   //Initialize RX buffer descriptors
   for(i = 0; i < SAMA5D2_ETH_RX_BUFFER_COUNT; i++)
   {
      //Calculate the address of the current RX buffer
      address = (uint32_t) rxBuffer[i];
      //Write the address to the descriptor entry
      rxBufferDesc[i].address = address & GMAC_RX_ADDRESS;
      //Clear status field
      rxBufferDesc[i].status = 0;
   }

   //Mark the last descriptor entry with the wrap flag
   rxBufferDesc[i - 1].address |= GMAC_RX_WRAP;
   //Initialize RX buffer index
   rxBufferIndex = 0;

   //Initialize dummy TX buffer descriptors
   for(i = 0; i < SAMA5D2_ETH_DUMMY_BUFFER_COUNT; i++)
   {
      //Calculate the address of the current TX buffer
      address = (uint32_t) dummyTxBuffer[i];
      //Write the address to the descriptor entry
      dummyTxBufferDesc[i].address = address;
      //Initialize status field
      dummyTxBufferDesc[i].status = GMAC_TX_USED;
   }

   //Mark the last descriptor entry with the wrap flag
   dummyTxBufferDesc[i - 1].status |= GMAC_TX_WRAP;

   //Initialize dummy RX buffer descriptors
   for(i = 0; i < SAMA5D2_ETH_DUMMY_BUFFER_COUNT; i++)
   {
      //Calculate the address of the current RX buffer
      address = (uint32_t) dummyRxBuffer[i];
      //Write the address to the descriptor entry
      dummyRxBufferDesc[i].address = (address & GMAC_RX_ADDRESS) | GMAC_RX_OWNERSHIP;
      //Clear status field
      dummyRxBufferDesc[i].status = 0;
   }

   //Mark the last descriptor entry with the wrap flag
   dummyRxBufferDesc[i - 1].address |= GMAC_RX_WRAP;

   //Start location of the TX descriptor list
   GMAC0->GMAC_TBQB = (uint32_t) txBufferDesc;
   GMAC0->GMAC_TBQBAPQ[0] = (uint32_t) dummyTxBufferDesc;
   GMAC0->GMAC_TBQBAPQ[1] = (uint32_t) dummyTxBufferDesc;

   //Start location of the RX descriptor list
   GMAC0->GMAC_RBQB = (uint32_t) rxBufferDesc;
   GMAC0->GMAC_RBQBAPQ[0] = (uint32_t) dummyRxBufferDesc;
   GMAC0->GMAC_RBQBAPQ[1] = (uint32_t) dummyRxBufferDesc;
}


/**
 * @brief SAMA5D2 Ethernet MAC timer handler
 *
 * This routine is periodically called by the TCP/IP stack to handle periodic
 * operations such as polling the link state
 *
 * @param[in] interface Underlying network interface
 **/

void sama5d2EthTick(NetInterface *interface)
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

void sama5d2EthEnableIrq(NetInterface *interface)
{
   //Enable Ethernet MAC interrupts
   aic_enable(ID_GMAC0);

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

void sama5d2EthDisableIrq(NetInterface *interface)
{
   //Disable Ethernet MAC interrupts
   aic_disable(ID_GMAC0);

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
 * @brief SAMA5D2 Ethernet MAC interrupt service routine
 **/

void sama5d2EthIrqHandler(void)
{
   bool_t flag;
   volatile uint32_t isr;
   volatile uint32_t tsr;
   volatile uint32_t rsr;

   //Interrupt service routine prologue
   osEnterIsr();

   //This flag will be set if a higher priority task must be woken
   flag = FALSE;

   //Each time the software reads GMAC_ISR, it has to check the
   //contents of GMAC_TSR, GMAC_RSR and GMAC_NSR
   isr = GMAC0->GMAC_ISRPQ[0];
   isr = GMAC0->GMAC_ISRPQ[1];
   isr = GMAC0->GMAC_ISR;
   tsr = GMAC0->GMAC_TSR;
   rsr = GMAC0->GMAC_RSR;

   //Packet transmitted?
   if((tsr & (GMAC_TSR_HRESP | GMAC_TSR_TXCOMP | GMAC_TSR_TFC |
      GMAC_TSR_TXGO | GMAC_TSR_RLE | GMAC_TSR_COL | GMAC_TSR_UBR)) != 0)
   {
      //Only clear TSR flags that are currently set
      GMAC0->GMAC_TSR = tsr;

      //Check whether the TX buffer is available for writing
      if((txBufferDesc[txBufferIndex].status & GMAC_TX_USED) != 0)
      {
         //Notify the TCP/IP stack that the transmitter is ready to send
         flag |= osSetEventFromIsr(&nicDriverInterface->nicTxEvent);
      }
   }

   //Packet received?
   if((rsr & (GMAC_RSR_HNO | GMAC_RSR_RXOVR | GMAC_RSR_REC | GMAC_RSR_BNA)) != 0)
   {
      //Set event flag
      nicDriverInterface->nicEvent = TRUE;
      //Notify the TCP/IP stack of the event
      flag |= osSetEventFromIsr(&netEvent);
   }

   //Write AIC_EOICR register before exiting
   AIC->AIC_EOICR = 0;

   //Interrupt service routine epilogue
   osExitIsr(flag);
}


/**
 * @brief SAMA5D2 Ethernet MAC event handler
 * @param[in] interface Underlying network interface
 **/

void sama5d2EthEventHandler(NetInterface *interface)
{
   error_t error;
   uint32_t rsr;

   //Read receive status
   rsr = GMAC0->GMAC_RSR;

   //Packet received?
   if((rsr & (GMAC_RSR_HNO | GMAC_RSR_RXOVR | GMAC_RSR_REC | GMAC_RSR_BNA)) != 0)
   {
      //Only clear RSR flags that are currently set
      GMAC0->GMAC_RSR = rsr;

      //Process all pending packets
      do
      {
         //Read incoming packet
         error = sama5d2EthReceivePacket(interface);

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

error_t sama5d2EthSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary)
{
   size_t length;

   //Retrieve the length of the packet
   length = netBufferGetLength(buffer) - offset;

   //Check the frame length
   if(length > SAMA5D2_ETH_TX_BUFFER_SIZE)
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
      //Report an error
      return ERROR_INVALID_LENGTH;
   }

   //Make sure the current buffer is available for writing
   if((txBufferDesc[txBufferIndex].status & GMAC_TX_USED) == 0)
   {
      return ERROR_FAILURE;
   }

   //Copy user data to the transmit buffer
   netBufferRead(txBuffer[txBufferIndex], buffer, offset, length);

   //Set the necessary flags in the descriptor entry
   if(txBufferIndex < (SAMA5D2_ETH_TX_BUFFER_COUNT - 1))
   {
      //Write the status word
      txBufferDesc[txBufferIndex].status = GMAC_TX_LAST |
         (length & GMAC_TX_LENGTH);

      //Point to the next buffer
      txBufferIndex++;
   }
   else
   {
      //Write the status word
      txBufferDesc[txBufferIndex].status = GMAC_TX_WRAP | GMAC_TX_LAST |
         (length & GMAC_TX_LENGTH);

      //Wrap around
      txBufferIndex = 0;
   }

   //Data synchronization barrier
   __DSB();

   //Set the TSTART bit to initiate transmission
   GMAC0->GMAC_NCR |= GMAC_NCR_TSTART;

   //Check whether the next buffer is available for writing
   if((txBufferDesc[txBufferIndex].status & GMAC_TX_USED) != 0)
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

error_t sama5d2EthReceivePacket(NetInterface *interface)
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
   for(i = 0; i < SAMA5D2_ETH_RX_BUFFER_COUNT; i++)
   {
      //Point to the current entry
      j = rxBufferIndex + i;

      //Wrap around to the beginning of the buffer if necessary
      if(j >= SAMA5D2_ETH_RX_BUFFER_COUNT)
      {
         j -= SAMA5D2_ETH_RX_BUFFER_COUNT;
      }

      //No more entries to process?
      if((rxBufferDesc[j].address & GMAC_RX_OWNERSHIP) == 0)
      {
         //Stop processing
         break;
      }

      //A valid SOF has been found?
      if((rxBufferDesc[j].status & GMAC_RX_SOF) != 0)
      {
         //Save the position of the SOF
         sofIndex = i;
      }

      //A valid EOF has been found?
      if((rxBufferDesc[j].status & GMAC_RX_EOF) != 0 && sofIndex != UINT_MAX)
      {
         //Save the position of the EOF
         eofIndex = i;
         //Retrieve the length of the frame
         size = rxBufferDesc[j].status & GMAC_RX_LENGTH;
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
         n = MIN(size, SAMA5D2_ETH_RX_BUFFER_SIZE);
         //Copy data from receive buffer
         osMemcpy(temp + length, rxBuffer[rxBufferIndex], n);
         //Update byte counters
         length += n;
         size -= n;
      }

      //Mark the current buffer as free
      rxBufferDesc[rxBufferIndex].address &= ~GMAC_RX_OWNERSHIP;

      //Point to the following entry
      rxBufferIndex++;

      //Wrap around to the beginning of the buffer if necessary
      if(rxBufferIndex >= SAMA5D2_ETH_RX_BUFFER_COUNT)
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

error_t sama5d2EthUpdateMacAddrFilter(NetInterface *interface)
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
   GMAC0->GMAC_SA[0].GMAC_SAB = interface->macAddr.w[0] | (interface->macAddr.w[1] << 16);
   GMAC0->GMAC_SA[0].GMAC_SAT = interface->macAddr.w[2];

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
               unicastMacAddr[j++] = entry->addr;
            }
         }
      }
   }

   //Configure the first unicast address filter
   if(j >= 1)
   {
      //The address is activated when SAT register is written
      GMAC0->GMAC_SA[1].GMAC_SAB = unicastMacAddr[0].w[0] | (unicastMacAddr[0].w[1] << 16);
      GMAC0->GMAC_SA[1].GMAC_SAT = unicastMacAddr[0].w[2];
   }
   else
   {
      //The address is deactivated when SAB register is written
      GMAC0->GMAC_SA[1].GMAC_SAB = 0;
   }

   //Configure the second unicast address filter
   if(j >= 2)
   {
      //The address is activated when SAT register is written
      GMAC0->GMAC_SA[2].GMAC_SAB = unicastMacAddr[1].w[0] | (unicastMacAddr[1].w[1] << 16);
      GMAC0->GMAC_SA[2].GMAC_SAT = unicastMacAddr[1].w[2];
   }
   else
   {
      //The address is deactivated when SAB register is written
      GMAC0->GMAC_SA[2].GMAC_SAB = 0;
   }

   //Configure the third unicast address filter
   if(j >= 3)
   {
      //The address is activated when SAT register is written
      GMAC0->GMAC_SA[3].GMAC_SAB = unicastMacAddr[2].w[0] | (unicastMacAddr[2].w[1] << 16);
      GMAC0->GMAC_SA[3].GMAC_SAT = unicastMacAddr[2].w[2];
   }
   else
   {
      //The address is deactivated when SAB register is written
      GMAC0->GMAC_SA[3].GMAC_SAB = 0;
   }

   //Configure the multicast address filter
   GMAC0->GMAC_HRB = hashTable[0];
   GMAC0->GMAC_HRT = hashTable[1];

   //Debug message
   TRACE_DEBUG("  HRB = %08" PRIX32 "\r\n", GMAC0->GMAC_HRB);
   TRACE_DEBUG("  HRT = %08" PRIX32 "\r\n", GMAC0->GMAC_HRT);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Adjust MAC configuration parameters for proper operation
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t sama5d2EthUpdateMacConfig(NetInterface *interface)
{
   uint32_t config;

   //Read network configuration register
   config = GMAC0->GMAC_NCFGR;

   //10BASE-T or 100BASE-TX operation mode?
   if(interface->linkSpeed == NIC_LINK_SPEED_100MBPS)
   {
      config |= GMAC_NCFGR_SPD;
   }
   else
   {
      config &= ~GMAC_NCFGR_SPD;
   }

   //Half-duplex or full-duplex mode?
   if(interface->duplexMode == NIC_FULL_DUPLEX_MODE)
   {
      config |= GMAC_NCFGR_FD;
   }
   else
   {
      config &= ~GMAC_NCFGR_FD;
   }

   //Write configuration value back to NCFGR register
   GMAC0->GMAC_NCFGR = config;

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

void sama5d2EthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data)
{
   uint32_t temp;

   //Valid opcode?
   if(opcode == SMI_OPCODE_WRITE)
   {
      //Set up a write operation
      temp = GMAC_MAN_CLTTO | GMAC_MAN_OP(1) | GMAC_MAN_WTN(2);
      //PHY address
      temp |= GMAC_MAN_PHYA(phyAddr);
      //Register address
      temp |= GMAC_MAN_REGA(regAddr);
      //Register value
      temp |= GMAC_MAN_DATA(data);

      //Start a write operation
      GMAC0->GMAC_MAN = temp;
      //Wait for the write to complete
      while((GMAC0->GMAC_NSR & GMAC_NSR_IDLE) == 0)
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

uint16_t sama5d2EthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr)
{
   uint16_t data;
   uint32_t temp;

   //Valid opcode?
   if(opcode == SMI_OPCODE_READ)
   {
      //Set up a read operation
      temp = GMAC_MAN_CLTTO | GMAC_MAN_OP(2) | GMAC_MAN_WTN(2);
      //PHY address
      temp |= GMAC_MAN_PHYA(phyAddr);
      //Register address
      temp |= GMAC_MAN_REGA(regAddr);

      //Start a read operation
      GMAC0->GMAC_MAN = temp;
      //Wait for the read to complete
      while((GMAC0->GMAC_NSR & GMAC_NSR_IDLE) == 0)
      {
      }

      //Get register value
      data = GMAC0->GMAC_MAN & GMAC_MAN_DATA_Msk;
   }
   else
   {
      //The MAC peripheral only supports standard Clause 22 opcodes
      data = 0;
   }

   //Return the value of the PHY register
   return data;
}
