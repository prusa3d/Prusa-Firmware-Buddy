/**
 * @file tms570_eth_driver.c
 * @brief TMS570 Ethernet MAC driver
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
#include "hl_hw_reg_access.h"
#include "hl_hw_emac.h"
#include "hl_hw_emac_ctrl.h"
#include "hl_hw_mdio.h"
#include "hl_gio.h"
#include "hl_sys_vim.h"
#include "core/net.h"
#include "drivers/mac/tms570_eth_driver.h"
#include "debug.h"

//MDIO input clock frequency
#define MDIO_INPUT_CLK 75000000
//MDIO output clock frequency
#define MDIO_OUTPUT_CLK 1000000

//Underlying network interface
static NetInterface *nicDriverInterface;

//IAR EWARM compiler?
#if defined(__ICCARM__)

//Transmit buffer
#pragma data_alignment = 4
#pragma location = TMS570_ETH_RAM_SECTION
static uint8_t txBuffer[TMS570_ETH_TX_BUFFER_COUNT][TMS570_ETH_TX_BUFFER_SIZE];
//Receive buffer
#pragma data_alignment = 4
#pragma location = TMS570_ETH_RAM_SECTION
static uint8_t rxBuffer[TMS570_ETH_RX_BUFFER_COUNT][TMS570_ETH_RX_BUFFER_SIZE];
//Transmit buffer descriptors
#pragma data_alignment = 4
#pragma location = TMS570_ETH_RAM_CPPI_SECTION
static Tms570TxBufferDesc txBufferDesc[TMS570_ETH_TX_BUFFER_COUNT];
//Receive buffer descriptors
#pragma data_alignment = 4
#pragma location = TMS570_ETH_RAM_CPPI_SECTION
static Tms570RxBufferDesc rxBufferDesc[TMS570_ETH_RX_BUFFER_COUNT];

//GCC compiler?
#else

//Transmit buffer
static uint8_t txBuffer[TMS570_ETH_TX_BUFFER_COUNT][TMS570_ETH_TX_BUFFER_SIZE]
   __attribute__((aligned(4), __section__(TMS570_ETH_RAM_SECTION)));
//Receive buffer
static uint8_t rxBuffer[TMS570_ETH_RX_BUFFER_COUNT][TMS570_ETH_RX_BUFFER_SIZE]
   __attribute__((aligned(4), __section__(TMS570_ETH_RAM_SECTION)));
//Transmit buffer descriptors
static Tms570TxBufferDesc txBufferDesc[TMS570_ETH_TX_BUFFER_COUNT]
   __attribute__((aligned(4), __section__(TMS570_ETH_RAM_CPPI_SECTION)));
//Receive buffer descriptors
static Tms570RxBufferDesc rxBufferDesc[TMS570_ETH_RX_BUFFER_COUNT]
   __attribute__((aligned(4), __section__(TMS570_ETH_RAM_CPPI_SECTION)));

#endif

//Pointer to the current TX buffer descriptor
static Tms570TxBufferDesc *txCurBufferDesc;
//Pointer to the current RX buffer descriptor
static Tms570RxBufferDesc *rxCurBufferDesc;


/**
 * @brief TMS570 Ethernet MAC driver
 **/

const NicDriver tms570EthDriver =
{
   NIC_TYPE_ETHERNET,
   ETH_MTU,
   tms570EthInit,
   tms570EthTick,
   tms570EthEnableIrq,
   tms570EthDisableIrq,
   tms570EthEventHandler,
   tms570EthSendPacket,
   tms570EthUpdateMacAddrFilter,
   tms570EthUpdateMacConfig,
   tms570EthWritePhyReg,
   tms570EthReadPhyReg,
   FALSE,
   TRUE,
   TRUE,
   FALSE
};


/**
 * @brief TMS570 Ethernet MAC initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t tms570EthInit(NetInterface *interface)
{
   error_t error;
   uint_t channel;
   uint32_t temp;

   //Debug message
   TRACE_INFO("Initializing TMS570 Ethernet MAC...\r\n");

   //Save underlying network interface
   nicDriverInterface = interface;

   //Select the interface mode (MII/RMII) and configure pin muxing
   tms570EthInitGpio(interface);

   //Reset the EMAC control module
   EMAC_CTRL_SOFTRESET_R = EMAC_SOFTRESET_SOFTRESET;
   //Wait for the reset to complete
   while((EMAC_CTRL_SOFTRESET_R & EMAC_SOFTRESET_SOFTRESET) != 0)
   {
   }

   //Reset the EMAC module
   EMAC_SOFTRESET_R = EMAC_SOFTRESET_SOFTRESET;
   //Wait for the reset to complete
   while((EMAC_SOFTRESET_R & EMAC_SOFTRESET_SOFTRESET) != 0)
   {
   }

   //Calculate the MDC clock divider to be used
   temp = (MDIO_INPUT_CLK / MDIO_OUTPUT_CLK) - 1;

   //Initialize MDIO interface
   MDIO_CONTROL_R = MDIO_CONTROL_ENABLE |
      MDIO_CONTROL_FAULTENB | (temp & MDIO_CONTROL_CLKDIV);

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

   //Clear the control registers
   EMAC_MACCONTROL_R = 0;
   EMAC_RXCONTROL_R = 0;
   EMAC_TXCONTROL_R = 0;

   //Initialize all 16 header descriptor pointer registers to 0
   for(channel = EMAC_CH0; channel <= EMAC_CH7; channel++)
   {
      //TX head descriptor pointer
      EMAC_TXHDP_R(channel) = 0;
      //TX completion pointer
      EMAC_TXCP_R(channel) = 0;
      //RX head descriptor pointer
      EMAC_RXHDP_R(channel) = 0;
      //RX completion pointer
      EMAC_RXCP_R(channel) = 0;
   }

   //Set the upper 32 bits of the source MAC address
   EMAC_MACSRCADDRHI_R = interface->macAddr.b[0] |
      (interface->macAddr.b[1] << 8) |
      (interface->macAddr.b[2] << 16) |
      (interface->macAddr.b[3] << 24);

   //Set the lower 16 bits of the source MAC address
   EMAC_MACSRCADDRLO_R = interface->macAddr.b[4] |
      (interface->macAddr.b[5] << 8);

   //Write the channel number to the MAC index register
   EMAC_MACINDEX_R = EMAC_CH0;

   //Set the upper 32 bits of the destination MAC address
   EMAC_MACADDRHI_R = interface->macAddr.b[0] |
      (interface->macAddr.b[1] << 8) |
      (interface->macAddr.b[2] << 16) |
      (interface->macAddr.b[3] << 24);

   //Set the lower 16 bits of the destination MAC address
   temp = interface->macAddr.b[4] |
      (interface->macAddr.b[5] << 8);

   //Use the current MAC address to match incoming packet addresses
   EMAC_MACADDRLO_R = EMAC_MACADDRLO_VALID | EMAC_MACADDRLO_MATCHFILT |
      (EMAC_CH0 << EMAC_MACADDRLO_CHANNEL_SHIFT) | temp;

   //Be sure to program all eight MAC address registers, whether the
   //receive channel is to be enabled or not
   for(channel = EMAC_CH1; channel <= EMAC_CH7; channel++)
   {
      //Write the channel number to the MAC index register
      EMAC_MACINDEX_R = channel;
      //The MAC address is not valid
      EMAC_MACADDRLO_R = (channel << EMAC_MACADDRLO_CHANNEL_SHIFT);
   }

   //Clear the MAC address hash registers
   EMAC_MACHASH1_R = 0;
   EMAC_MACHASH2_R = 0;

   //The RX buffer offset must be initialized to zero
   EMAC_RXBUFFEROFFSET_R = 0;

   //Clear all unicast channels
   EMAC_RXUNICASTCLEAR_R = 0xFF;

   //Accept unicast frames
   EMAC_RXUNICASTSET_R |= (1 << EMAC_CH0);

   //Received CRC is transferred to memory for all channels
   EMAC_RXMBPENABLE_R = EMAC_RXMBPENABLE_RXPASSCRC;

   //Accept broadcast frames
   EMAC_RXMBPENABLE_R |= EMAC_RXMBPENABLE_RXBROADEN |
      (EMAC_CH0 << EMAC_RXMBPENABLE_RXBROADCH_SHIFT);

   //Accept hash matching multicast frames
   EMAC_RXMBPENABLE_R |= EMAC_RXMBPENABLE_RXMULTEN |
      (EMAC_CH0 << EMAC_RXMBPENABLE_RXMULTCH_SHIFT);

   //Register TX interrupt handler
   vimChannelMap(77, TMS570_ETH_TX_IRQ_CHANNEL,
      (t_isrFuncPTR) tms570EthTxIrqHandler);

   //Register RX interrupt handler
   vimChannelMap(79, TMS570_ETH_RX_IRQ_CHANNEL,
      (t_isrFuncPTR) tms570EthRxIrqHandler);

   //Clear all unused channel interrupt bits
   EMAC_TXINTMASKCLEAR_R = 0xFF;
   EMAC_RXINTMASKCLEAR_R = 0xFF;

   //Enable the receive and transmit channel interrupt bits
   EMAC_TXINTMASKSET_R = (1 << EMAC_CH0);
   EMAC_RXINTMASKSET_R = (1 << EMAC_CH0);

   //Configure TX and RX buffer descriptors
   tms570EthInitBufferDesc(interface);

   //Write the RX DMA head descriptor pointer
   EMAC_RXHDP_R(EMAC_CH0) = (uint32_t) rxCurBufferDesc;

   //Enable the receive and transmit DMA controllers
   EMAC_TXCONTROL_R = EMAC_TXCONTROL_TXEN;
   EMAC_RXCONTROL_R = EMAC_RXCONTROL_RXEN;

   //Enable TX and RX
   EMAC_MACCONTROL_R = EMAC_MACCONTROL_GMIIEN;

   //Enable TX and RX completion interrupts
   EMAC_CTRL_CnTXEN_R(EMAC_CORE0) |= (1 << EMAC_CH0);
   EMAC_CTRL_CnRXEN_R(EMAC_CORE0) |= (1 << EMAC_CH0);

   //Accept any packets from the upper layer
   osSetEvent(&interface->nicTxEvent);

   //Successful initialization
   return NO_ERROR;
}


//LAUNCHXL2-570LC43 evaluation board?
#if defined(USE_LAUNCHXL2_570LC43)

/**
 * @brief GPIO configuration
 * @param[in] interface Underlying network interface
 **/

void tms570EthInitGpio(NetInterface *interface)
{
   //Configure PHY_INT (PA_3) as an input
   gioPORTA->DIR &= ~(1 << 3);
   gioPORTA->PSL |= (1 << 3);
   gioPORTA->PULDIS &= ~(1 << 3);

   //Configure PHY_RST (PA_4) as an output
   gioPORTA->DIR |= (1 << 4);
   gioPORTA->PDR &= ~(1 << 4);

   //Reset PHY transceiver
   gioPORTA->DCLR = (1 << 4);
   sleep(10);
   gioPORTA->DSET = (1 << 4);
   sleep(10);
}

#endif


/**
 * @brief Initialize buffer descriptor lists
 * @param[in] interface Underlying network interface
 **/

void tms570EthInitBufferDesc(NetInterface *interface)
{
   uint_t i;
   uint_t nextIndex;
   uint_t prevIndex;

   //Initialize TX buffer descriptor list
   for(i = 0; i < TMS570_ETH_TX_BUFFER_COUNT; i++)
   {
      //Index of the next buffer
      nextIndex = (i + 1) % TMS570_ETH_TX_BUFFER_COUNT;
      //Index of the previous buffer
      prevIndex = (i + TMS570_ETH_TX_BUFFER_COUNT - 1) % TMS570_ETH_TX_BUFFER_COUNT;

      //Next descriptor pointer
      txBufferDesc[i].word0 = HTOLE32((uint32_t) NULL);
      //Buffer pointer
      txBufferDesc[i].word1 = htole32((uint32_t) txBuffer[i]);
      //Buffer offset and buffer length
      txBufferDesc[i].word2 = HTOLE32(0);
      //Status flags and packet length
      txBufferDesc[i].word3 = HTOLE32(0);

      //Form a doubly linked list
      txBufferDesc[i].next = &txBufferDesc[nextIndex];
      txBufferDesc[i].prev = &txBufferDesc[prevIndex];
   }

   //Point to the very first descriptor
   txCurBufferDesc = &txBufferDesc[0];

   //Mark the end of the queue
   txCurBufferDesc->prev->word3 = HTOLE32(EMAC_TX_WORD3_SOP |
      EMAC_TX_WORD3_EOP | EMAC_TX_WORD3_EOQ);

   //Initialize RX buffer descriptor list
   for(i = 0; i < TMS570_ETH_RX_BUFFER_COUNT; i++)
   {
      //Index of the next buffer
      nextIndex = (i + 1) % TMS570_ETH_RX_BUFFER_COUNT;
      //Index of the previous buffer
      prevIndex = (i + TMS570_ETH_RX_BUFFER_COUNT - 1) % TMS570_ETH_RX_BUFFER_COUNT;

      //Next descriptor pointer
      rxBufferDesc[i].word0 = htole32((uint32_t) &rxBufferDesc[nextIndex]);
      //Buffer pointer
      rxBufferDesc[i].word1 = htole32((uint32_t) rxBuffer[i]);
      //Buffer offset and buffer length
      rxBufferDesc[i].word2 = HTOLE32(TMS570_ETH_RX_BUFFER_SIZE);
      //Status flags and packet length
      rxBufferDesc[i].word3 = HTOLE32(EMAC_RX_WORD3_OWNER);

      //Form a doubly linked list
      rxBufferDesc[i].next = &rxBufferDesc[nextIndex];
      rxBufferDesc[i].prev = &rxBufferDesc[prevIndex];
   }

   //Point to the very first descriptor
   rxCurBufferDesc = &rxBufferDesc[0];

   //Mark the end of the queue
   rxCurBufferDesc->prev->word0 = HTOLE32((uint32_t) NULL);
}


/**
 * @brief TMS570 Ethernet MAC timer handler
 *
 * This routine is periodically called by the TCP/IP stack to handle periodic
 * operations such as polling the link state
 *
 * @param[in] interface Underlying network interface
 **/

void tms570EthTick(NetInterface *interface)
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

   //Misqueued buffer condition?
   if(letoh32(rxCurBufferDesc->word3) & EMAC_RX_WORD3_OWNER)
   {
      if(EMAC_RXHDP_R(EMAC_CH0) == 0)
      {
         //The host acts on the misqueued buffer condition by writing the added
         //buffer descriptor address to the appropriate RX DMA head descriptor
         //pointer
         EMAC_RXHDP_R(EMAC_CH0) = (uint32_t) rxCurBufferDesc;
      }
   }
}


/**
 * @brief Enable interrupts
 * @param[in] interface Underlying network interface
 **/

void tms570EthEnableIrq(NetInterface *interface)
{
   //Enable Ethernet MAC interrupts
   vimEnableInterrupt(TMS570_ETH_TX_IRQ_CHANNEL, SYS_IRQ);
   vimEnableInterrupt(TMS570_ETH_RX_IRQ_CHANNEL, SYS_IRQ);


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

void tms570EthDisableIrq(NetInterface *interface)
{
   //Disable Ethernet MAC interrupts
   vimDisableInterrupt(TMS570_ETH_TX_IRQ_CHANNEL);
   vimDisableInterrupt(TMS570_ETH_RX_IRQ_CHANNEL);


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
 * @brief Ethernet MAC transmit interrupt
 **/

__irq __arm void tms570EthTxIrqHandler(void)
{
   bool_t flag;
   uint32_t status;
   uint32_t temp;
   Tms570TxBufferDesc *p;

   //Interrupt service routine prologue
   osEnterIsr();

   //This flag will be set if a higher priority task must be woken
   flag = FALSE;

   //Read the C0TXSTAT register to determine which channels caused the interrupt
   status = EMAC_CTRL_C0TXSTAT_R;

   //Packet transmitted on channel 0?
   if(status & (1 << EMAC_CH0))
   {
      //Point to the buffer descriptor
      p = (Tms570TxBufferDesc *) EMAC_TXCP_R(EMAC_CH0);

      //Read the status flags
      temp = letoh32(p->word3) & (EMAC_TX_WORD3_SOP | EMAC_TX_WORD3_EOP |
         EMAC_TX_WORD3_OWNER | EMAC_TX_WORD3_EOQ);

      //Misqueued buffer condition?
      if(temp == (EMAC_TX_WORD3_SOP | EMAC_TX_WORD3_EOP | EMAC_TX_WORD3_EOQ))
      {
         //Check whether the next descriptor pointer is non-zero
         if(letoh32(p->word0) != 0)
         {
            //The host corrects the misqueued buffer condition by writing the
            //misqueued packet’s buffer descriptor address to the appropriate
            //TX DMA head descriptor pointer
            EMAC_TXHDP_R(EMAC_CH0) = letoh32((uint32_t) p->word0);
         }
      }

      //Write the TX completion pointer
      EMAC_TXCP_R(EMAC_CH0) = (uint32_t) p;

      //Check whether the TX buffer is available for writing
      if((letoh32(txCurBufferDesc->word3) & EMAC_TX_WORD3_OWNER) == 0)
      {
         //Notify the TCP/IP stack that the transmitter is ready to send
         flag |= osSetEventFromIsr(&nicDriverInterface->nicTxEvent);
      }
   }

   //Write the DMA end of interrupt vector
   EMAC_MACEOIVECTOR_R = EMAC_MACEOIVECTOR_C0TX;

   //Interrupt service routine epilogue
   osExitIsr(flag);
}


/**
 * @brief Ethernet MAC receive interrupt
 **/

__irq __arm void tms570EthRxIrqHandler(void)
{
   bool_t flag;
   uint32_t status;

   //Interrupt service routine prologue
   osEnterIsr();

   //This flag will be set if a higher priority task must be woken
   flag = FALSE;

   //Read the C0RXSTAT register to determine which channels caused the interrupt
   status = EMAC_CTRL_C0RXSTAT_R;

   //Packet received on channel 0?
   if(status & (1 << EMAC_CH0))
   {
      //Disable RX interrupts
      EMAC_CTRL_CnRXEN_R(EMAC_CORE0) &= ~(1 << EMAC_CH0);

      //Set event flag
      nicDriverInterface->nicEvent = TRUE;
      //Notify the TCP/IP stack of the event
      flag |= osSetEventFromIsr(&netEvent);
   }

   //Write the DMA end of interrupt vector
   EMAC_MACEOIVECTOR_R = EMAC_MACEOIVECTOR_C0RX;

   //Interrupt service routine epilogue
   osExitIsr(flag);
}


/**
 * @brief TMS570 Ethernet MAC event handler
 * @param[in] interface Underlying network interface
 **/

void tms570EthEventHandler(NetInterface *interface)
{
   error_t error;

   //Process all pending packets
   do
   {
      //Read incoming packet
      error = tms570EthReceivePacket(interface);

      //No more data in the receive buffer?
   } while(error != ERROR_BUFFER_EMPTY);

   //Re-enable RX interrupts
   EMAC_CTRL_CnRXEN_R(EMAC_CORE0) |= (1 << EMAC_CH0);
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

error_t tms570EthSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary)
{
   size_t length;
   uint32_t temp;

   //Retrieve the length of the packet
   length = netBufferGetLength(buffer) - offset;

   //Check the frame length
   if(length > TMS570_ETH_TX_BUFFER_SIZE)
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
      //Report an error
      return ERROR_INVALID_LENGTH;
   }

   //Make sure the current buffer is available for writing
   if(letoh32(txCurBufferDesc->word3) & EMAC_TX_WORD3_OWNER)
   {
      return ERROR_FAILURE;
   }

   //Mark the end of the queue with a NULL pointer
   txCurBufferDesc->word0 = HTOLE32((uint32_t) NULL);

   //Copy user data to the transmit buffer
   netBufferRead((uint8_t *) letoh32(txCurBufferDesc->word1), buffer, offset, length);

   //Set the length of the buffer
   txCurBufferDesc->word2 = htole32(length & EMAC_TX_WORD2_BUFFER_LENGTH);

   //Give the ownership of the descriptor to the DMA
   txCurBufferDesc->word3 = htole32(EMAC_TX_WORD3_SOP | EMAC_TX_WORD3_EOP |
      EMAC_TX_WORD3_OWNER | (length & EMAC_TX_WORD3_PACKET_LENGTH));

   //Link the current descriptor to the previous descriptor
   txCurBufferDesc->prev->word0 = htole32((uint32_t) txCurBufferDesc);

   //Read the status flags of the previous descriptor
   temp = letoh32(txCurBufferDesc->prev->word3) & (EMAC_TX_WORD3_SOP |
      EMAC_TX_WORD3_EOP | EMAC_TX_WORD3_OWNER | EMAC_TX_WORD3_EOQ);

   //Misqueued buffer condition?
   if(temp == (EMAC_TX_WORD3_SOP | EMAC_TX_WORD3_EOP | EMAC_TX_WORD3_EOQ))
   {
      //Clear the misqueued buffer condition
      txCurBufferDesc->prev->word3 = HTOLE32(0);

      //The host corrects the misqueued buffer condition by writing the
      //misqueued packet’s buffer descriptor address to the appropriate
      //TX DMA head descriptor pointer
      EMAC_TXHDP_R(EMAC_CH0) = (uint32_t) txCurBufferDesc;
   }

   //Point to the next descriptor in the list
   txCurBufferDesc = txCurBufferDesc->next;

   //Check whether the next buffer is available for writing
   if((letoh32(txCurBufferDesc->word3) & EMAC_TX_WORD3_OWNER) == 0)
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
   }

   //Data successfully written
   return NO_ERROR;
}


/**
 * @brief Receive a packet
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t tms570EthReceivePacket(NetInterface *interface)
{
   static uint8_t buffer[TMS570_ETH_RX_BUFFER_SIZE];
   error_t error;
   size_t n;
   uint32_t temp;

   //Current buffer available for reading?
   if((letoh32(rxCurBufferDesc->word3) & EMAC_RX_WORD3_OWNER) == 0)
   {
      //SOP and EOP flags should be set
      if((letoh32(rxCurBufferDesc->word3) & EMAC_RX_WORD3_SOP) != 0 &&
         (letoh32(rxCurBufferDesc->word3) & EMAC_RX_WORD3_EOP) != 0)
      {
         //Make sure no error occurred
         if((letoh32(rxCurBufferDesc->word3) & EMAC_RX_WORD3_ERROR_MASK) == 0)
         {
            //Retrieve the length of the frame
            n = letoh32(rxCurBufferDesc->word3) & EMAC_RX_WORD3_PACKET_LENGTH;
            //Limit the number of data to read
            n = MIN(n, TMS570_ETH_RX_BUFFER_SIZE);

            //Copy data from the receive buffer
            osMemcpy(buffer, (uint8_t *) letoh32(rxCurBufferDesc->word1), n);

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

      //Mark the end of the queue with a NULL pointer
      rxCurBufferDesc->word0 = HTOLE32((uint32_t) NULL);
      //Restore the length of the buffer
      rxCurBufferDesc->word2 = HTOLE32(TMS570_ETH_RX_BUFFER_SIZE);
      //Give the ownership of the descriptor back to the DMA
      rxCurBufferDesc->word3 = HTOLE32(EMAC_RX_WORD3_OWNER);

      //Link the current descriptor to the previous descriptor
      rxCurBufferDesc->prev->word0 = htole32((uint32_t) rxCurBufferDesc);

      //Read the status flags of the previous descriptor
      temp = letoh32(rxCurBufferDesc->prev->word3) & (EMAC_RX_WORD3_SOP |
         EMAC_RX_WORD3_EOP | EMAC_RX_WORD3_OWNER | EMAC_RX_WORD3_EOQ);

      //Misqueued buffer condition?
      if(temp == (EMAC_RX_WORD3_SOP | EMAC_RX_WORD3_EOP | EMAC_RX_WORD3_EOQ))
      {
         //The host acts on the misqueued buffer condition by writing the added
         //buffer descriptor address to the appropriate RX DMA head descriptor
         //pointer
         EMAC_RXHDP_R(EMAC_CH0) = (uint32_t) rxCurBufferDesc;
      }

      //Write the RX completion pointer
      EMAC_RXCP_R(EMAC_CH0) = (uint32_t) rxCurBufferDesc;

      //Point to the next descriptor in the list
      rxCurBufferDesc = rxCurBufferDesc->next;
   }
   else
   {
      //No more data in the receive buffer
      error = ERROR_BUFFER_EMPTY;
   }

   //Check whether a valid packet has been received
   if(!error)
   {
      NetRxAncillary ancillary;

      //Additional options can be passed to the stack along with the packet
      ancillary = NET_DEFAULT_RX_ANCILLARY;

      //Pass the packet to the upper layer
      nicProcessPacket(interface, buffer, n, &ancillary);
   }

   //Return status code
   return error;
}


/**
 * @brief Configure MAC address filtering
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t tms570EthUpdateMacAddrFilter(NetInterface *interface)
{
   uint_t i;
   uint_t k;
   uint8_t *p;
   uint32_t hashTable[2];
   MacFilterEntry *entry;

   //Debug message
   TRACE_DEBUG("Updating MAC filter...\r\n");

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
         //Point to the MAC address
         p = entry->addr.b;

         //Apply the hash function
         k = (p[0] >> 2) ^ (p[0] << 4);
         k ^= (p[1] >> 4) ^ (p[1] << 2);
         k ^= (p[2] >> 6) ^ p[2];
         k ^= (p[3] >> 2) ^ (p[3] << 4);
         k ^= (p[4] >> 4) ^ (p[4] << 2);
         k ^= (p[5] >> 6) ^ p[5];

         //The hash value is reduced to a 6-bit index
         k &= 0x3F;

         //Update hash table contents
         hashTable[k / 32] |= (1 << (k % 32));
      }
   }

   //Write the hash table
   EMAC_MACHASH1_R = hashTable[0];
   EMAC_MACHASH2_R = hashTable[1];

   //Debug message
   TRACE_DEBUG("  MACHASH1 = %08" PRIX32 "\r\n", EMAC_MACHASH1_R);
   TRACE_DEBUG("  MACHASH2 = %08" PRIX32 "\r\n", EMAC_MACHASH2_R);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Adjust MAC configuration parameters for proper operation
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t tms570EthUpdateMacConfig(NetInterface *interface)
{
   uint32_t config;

   //Read MAC control register
   config = EMAC_MACCONTROL_R;

   //100BASE-TX or 10BASE-T operation mode?
   if(interface->linkSpeed == NIC_LINK_SPEED_100MBPS)
   {
      config |= EMAC_MACCONTROL_RMIISPEED;
   }
   else
   {
      config &= ~EMAC_MACCONTROL_RMIISPEED;
   }

   //Half-duplex or full-duplex mode?
   if(interface->duplexMode == NIC_FULL_DUPLEX_MODE)
   {
      config |= EMAC_MACCONTROL_FULLDUPLEX;
   }
   else
   {
      config &= ~EMAC_MACCONTROL_FULLDUPLEX;
   }

   //Update MAC control register
   EMAC_MACCONTROL_R = config;

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

void tms570EthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data)
{
   uint32_t temp;

   //Valid opcode?
   if(opcode == SMI_OPCODE_WRITE)
   {
      //Set up a write operation
      temp = MDIO_USERACCESS0_GO | MDIO_USERACCESS0_WRITE;
      //PHY address
      temp |= (phyAddr << MDIO_USERACCESS0_PHYADR_SHIFT) & MDIO_USERACCESS0_PHYADR;
      //Register address
      temp |= (regAddr << MDIO_USERACCESS0_REGADR_SHIFT) & MDIO_USERACCESS0_REGADR;
      //Register value
      temp |= data & MDIO_USERACCESS0_DATA;

      //Start a write operation
      MDIO_USERACCESS0_R = temp;
      //Wait for the write to complete
      while((MDIO_USERACCESS0_R & MDIO_USERACCESS0_GO) != 0)
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

uint16_t tms570EthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr)
{
   uint16_t data;
   uint32_t temp;

   //Valid opcode?
   if(opcode == SMI_OPCODE_READ)
   {
      //Set up a read operation
      temp = MDIO_USERACCESS0_GO | MDIO_USERACCESS0_READ;
      //PHY address
      temp |= (phyAddr << MDIO_USERACCESS0_PHYADR_SHIFT) & MDIO_USERACCESS0_PHYADR;
      //Register address
      temp |= (regAddr << MDIO_USERACCESS0_REGADR_SHIFT) & MDIO_USERACCESS0_REGADR;

      //Start a read operation
      MDIO_USERACCESS0_R = temp;
      //Wait for the read to complete
      while((MDIO_USERACCESS0_R & MDIO_USERACCESS0_GO) != 0)
      {
      }

      //Get register value
      data = MDIO_USERACCESS0_R & MDIO_USERACCESS0_DATA;
   }
   else
   {
      //The MAC peripheral only supports standard Clause 22 opcodes
      data = 0;
   }

   //Return the value of the PHY register
   return data;
}
