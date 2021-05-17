/**
 * @file str912_eth_driver.c
 * @brief STR9 Ethernet MAC driver
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
#include "91x_lib.h"
#include "core/net.h"
#include "drivers/mac/str912_eth_driver.h"
#include "debug.h"

//Underlying network interface
static NetInterface *nicDriverInterface;

//IAR EWARM compiler?
#if defined(__ICCARM__)

//Transmit buffer
#pragma data_alignment = 4
static uint8_t txBuffer[STR912_ETH_TX_BUFFER_COUNT][STR912_ETH_TX_BUFFER_SIZE];
//Receive buffer
#pragma data_alignment = 4
static uint8_t rxBuffer[STR912_ETH_RX_BUFFER_COUNT][STR912_ETH_RX_BUFFER_SIZE];
//Transmit DMA descriptors
#pragma data_alignment = 4
static Str912TxDmaDesc txDmaDesc[STR912_ETH_TX_BUFFER_COUNT];
//Receive DMA descriptors
#pragma data_alignment = 4
static Str912RxDmaDesc rxDmaDesc[STR912_ETH_RX_BUFFER_COUNT];

//Keil MDK-ARM or GCC compiler?
#else

//Transmit buffer
static uint8_t txBuffer[STR912_ETH_TX_BUFFER_COUNT][STR912_ETH_TX_BUFFER_SIZE]
   __attribute__((aligned(4)));
//Receive buffer
static uint8_t rxBuffer[STR912_ETH_RX_BUFFER_COUNT][STR912_ETH_RX_BUFFER_SIZE]
   __attribute__((aligned(4)));
//Transmit DMA descriptors
static Str912TxDmaDesc txDmaDesc[STR912_ETH_TX_BUFFER_COUNT]
   __attribute__((aligned(4)));
//Receive DMA descriptors
static Str912RxDmaDesc rxDmaDesc[STR912_ETH_RX_BUFFER_COUNT]
   __attribute__((aligned(4)));

#endif

//Pointer to the current TX DMA descriptor
static Str912TxDmaDesc *txCurDmaDesc;
//Pointer to the current RX DMA descriptor
static Str912RxDmaDesc *rxCurDmaDesc;


/**
 * @brief STR912 Ethernet MAC driver
 **/

const NicDriver str912EthDriver =
{
   NIC_TYPE_ETHERNET,
   ETH_MTU,
   str912EthInit,
   str912EthTick,
   str912EthEnableIrq,
   str912EthDisableIrq,
   str912EthEventHandler,
   str912EthSendPacket,
   str912EthUpdateMacAddrFilter,
   str912EthUpdateMacConfig,
   str912EthWritePhyReg,
   str912EthReadPhyReg,
   TRUE,
   TRUE,
   TRUE,
   FALSE
};


/**
 * @brief STR912 Ethernet MAC initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t str912EthInit(NetInterface *interface)
{
   error_t error;

   //Debug message
   TRACE_INFO("Initializing STR912 Ethernet MAC...\r\n");

   //Save underlying network interface
   nicDriverInterface = interface;

   //GPIO configuration
   str912EthInitGpio(interface);

   //Enable Ethernet MAC clock
   SCU_AHBPeriphClockConfig(__ENET, ENABLE);

   //Reset Ethernet MAC peripheral
   SCU_AHBPeriphReset(__ENET, ENABLE);
   SCU_AHBPeriphReset(__ENET, DISABLE);

   //MAC DMA software reset
   ENET_DMA->SCR |= ENET_SCR_SRESET;
   ENET_DMA->SCR &= ~ENET_SCR_SRESET;

   //Use default MAC configuration
   ENET_MAC->MCR = ENET_MCR_AFM_1 | ENET_MCR_RVFF | ENET_MCR_BL_1 |
      ENET_MCR_DCE | ENET_MCR_RVBE;

   //Adjust HCLK divider depending on system clock frequency
   if(SCU_GetHCLKFreqValue() > 50000)
   {
      ENET_MAC->MCR |= ENET_MCR_PS_1;
   }

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
   ENET_MAC->MAL = interface->macAddr.w[0] | (interface->macAddr.w[1] << 16);
   ENET_MAC->MAH = interface->macAddr.w[2];

   //Initialize hash table
   ENET_MAC->MCLA = 0;
   ENET_MAC->MCHA = 0;

   //DMA configuration
   //ENET_DMA->SCR = 0;

   //Force a DMA abort
   ENET_DMA->TXSTR |= ENET_TXSTR_DMA_EN;
   ENET_DMA->RXSTR |= ENET_RXSTR_DMA_EN;

   //Set descriptor fetch delay
   ENET_DMA->TXSTR = ENET_TXSTR_DFETCH_DLY_DEFAULT | ENET_TXSTR_UNDER_RUN;
   ENET_DMA->RXSTR = ENET_RXSTR_DFETCH_DLY_DEFAULT;

   //Initialize DMA descriptor lists
   str912EthInitDmaDesc(interface);

   //Clear interrupt flags
   ENET_DMA->ISR = ENET_ISR_TX_CURR_DONE | ENET_ISR_RX_CURR_DONE;
   //Configure DMA interrupts as desired
   ENET_DMA->IER = ENET_IER_TX_CURR_DONE_EN | ENET_IER_RX_CURR_DONE_EN;

   //Configure Ethernet interrupt priority
   VIC_Config(ENET_ITLine, VIC_IRQ, STR912_ETH_IRQ_PRIORITY);

   //Enable MAC transmission and reception
   ENET_MAC->MCR |= ENET_MCR_TE | ENET_MCR_RE;
   //Instruct the DMA to poll the receive descriptor list
   ENET_DMA->RXSTR |= ENET_RXSTR_START_FETCH;

   //Accept any packets from the upper layer
   osSetEvent(&interface->nicTxEvent);

   //Successful initialization
   return NO_ERROR;
}


//STR-E912 evaluation board?
#if defined(USE_STR_E912)

/**
 * @brief GPIO configuration
 * @param[in] interface Underlying network interface
 **/

void str912EthInitGpio(NetInterface *interface)
{
   GPIO_InitTypeDef GPIO_InitStructure;

   //Enable GPIO clocks
   SCU_APBPeriphClockConfig(__GPIO0, ENABLE);
   SCU_APBPeriphClockConfig(__GPIO1, ENABLE);
   SCU_APBPeriphClockConfig(__GPIO5, ENABLE);

   //Enable MII_PHYCLK clock
   SCU_PHYCLKConfig(ENABLE);

   //Configure MII_TX_CLK (P0.0), MII_RXD0 (P0.2), MII_RXD1 (P0.3), MII_RXD2 (P0.4),
   //MII_RXD3 (P0.5), MII_RX_CLK (P0.6) and MII_RX_DV (P0.7)
   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_2 | GPIO_Pin_3 |
      GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;

   GPIO_InitStructure.GPIO_Direction = GPIO_PinInput;
   GPIO_InitStructure.GPIO_IPInputConnected = GPIO_IPInputConnected_Disable;
   GPIO_InitStructure.GPIO_Alternate = GPIO_InputAlt1;
   GPIO_Init(GPIO0, &GPIO_InitStructure);

   //Configure MII_RX_ER (P1.0), MII_COL (P1.5) and MII_CRS (P1.6)
   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_5 | GPIO_Pin_6;
   GPIO_InitStructure.GPIO_Direction = GPIO_PinInput;
   GPIO_InitStructure.GPIO_IPInputConnected = GPIO_IPInputConnected_Disable;
   GPIO_InitStructure.GPIO_Alternate = GPIO_InputAlt1;
   GPIO_Init(GPIO1, &GPIO_InitStructure);

   //Configure MII_TXD0 (P1.1), MII_TXD1 (P1.2), MII_TXD2 (P1.3),
   //MII_TXD3 (P1.4) and MII_MDC (P1.7)
   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2 |
      GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_7;

   GPIO_InitStructure.GPIO_Direction = GPIO_PinOutput;
   GPIO_InitStructure.GPIO_Type = GPIO_Type_PushPull;
   GPIO_InitStructure.GPIO_IPInputConnected = GPIO_IPInputConnected_Disable;
   GPIO_InitStructure.GPIO_Alternate = GPIO_OutputAlt2;
   GPIO_Init(GPIO1, &GPIO_InitStructure);

   //Configure MII_PHYCLK (P5.2) and MII_TX_EN (P5.3)
   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
   GPIO_InitStructure.GPIO_Direction = GPIO_PinOutput;
   GPIO_InitStructure.GPIO_Type = GPIO_Type_PushPull;
   GPIO_InitStructure.GPIO_IPInputConnected = GPIO_IPInputConnected_Disable;
   GPIO_InitStructure.GPIO_Alternate = GPIO_OutputAlt2;
   GPIO_Init(GPIO5, &GPIO_InitStructure);
}

#endif


/**
 * @brief Initialize DMA descriptor lists
 * @param[in] interface Underlying network interface
 **/

void str912EthInitDmaDesc(NetInterface *interface)
{
   uint_t i;

   //Initialize TX DMA descriptor list
   for(i = 0; i < STR912_ETH_TX_BUFFER_COUNT; i++)
   {
      //Control word
      txDmaDesc[i].ctrl = ENET_TDES_CTRL_NXT_EN;
      //Transmit buffer address
      txDmaDesc[i].start = (uint32_t) txBuffer[i];
      //Next descriptor address
      txDmaDesc[i].next = (uint32_t) &txDmaDesc[i + 1] | ENET_TDES_NEXT_NPOL_EN;
      //Status word
      txDmaDesc[i].status = 0;
   }

   //The last descriptor is chained to the first entry
   txDmaDesc[i - 1].next = (uint32_t) &txDmaDesc[0] | ENET_TDES_NEXT_NPOL_EN;
   //Point to the very first descriptor
   txCurDmaDesc = &txDmaDesc[0];

   //Initialize RX DMA descriptor list
   for(i = 0; i < STR912_ETH_RX_BUFFER_COUNT; i++)
   {
      //Control word
      rxDmaDesc[i].ctrl = ENET_RDES_CTRL_NXT_EN | STR912_ETH_RX_BUFFER_SIZE;
      //Receive buffer address
      rxDmaDesc[i].start = (uint32_t) rxBuffer[i];
      //Next descriptor address
      rxDmaDesc[i].next = (uint32_t) &rxDmaDesc[i + 1] | ENET_RDES_NEXT_NPOL_EN;
      //Status word
      rxDmaDesc[i].status = ENET_RDES_STATUS_VALID;
   }

   //The last descriptor is chained to the first entry
   rxDmaDesc[i - 1].next = (uint32_t) &rxDmaDesc[0] | ENET_RDES_NEXT_NPOL_EN;
   //Point to the very first descriptor
   rxCurDmaDesc = &rxDmaDesc[0];

   //Start location of the TX descriptor list
   ENET_DMA->TXNDAR = (uint32_t) txDmaDesc | ENET_TDES_NEXT_NPOL_EN;
   //Start location of the RX descriptor list
   ENET_DMA->RXNDAR = (uint32_t) rxDmaDesc | ENET_RDES_NEXT_NPOL_EN;
}


/**
 * @brief STR912 Ethernet MAC timer handler
 *
 * This routine is periodically called by the TCP/IP stack to handle periodic
 * operations such as polling the link state
 *
 * @param[in] interface Underlying network interface
 **/

void str912EthTick(NetInterface *interface)
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

void str912EthEnableIrq(NetInterface *interface)
{
   //Enable Ethernet MAC interrupts
   VIC_ITCmd(ENET_ITLine, ENABLE);

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

void str912EthDisableIrq(NetInterface *interface)
{
   //Disable Ethernet MAC interrupts
   VIC_ITCmd(ENET_ITLine, DISABLE);

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
 * @brief STR912 Ethernet MAC interrupt service routine
 **/

void ENET_IRQHandler(void)
{
   bool_t flag;
   uint32_t status;

   //Interrupt service routine prologue
   osEnterIsr();

   //This flag will be set if a higher priority task must be woken
   flag = FALSE;

   //Read DMA status register
   status = ENET_DMA->ISR;

   //Packet transmitted?
   if((status & ENET_ISR_TX_CURR_DONE) != 0)
   {
      //Clear TX_CURR_DONE interrupt flag
      ENET_DMA->ISR = ENET_ISR_TX_CURR_DONE;

      //Check whether the TX buffer is available for writing
      if((txCurDmaDesc->status & ENET_TDES_STATUS_VALID) == 0)
      {
         //Notify the TCP/IP stack that the transmitter is ready to send
         flag |= osSetEventFromIsr(&nicDriverInterface->nicTxEvent);
      }
   }

   //Packet received?
   if((status & ENET_ISR_RX_CURR_DONE) != 0)
   {
      //Disable RX_CURR_DONE interrupt
      ENET_DMA->IER &= ~ENET_IER_RX_CURR_DONE_EN;

      //Set event flag
      nicDriverInterface->nicEvent = TRUE;
      //Notify the TCP/IP stack of the event
      flag |= osSetEventFromIsr(&netEvent);
   }

   //Interrupt service routine epilogue
   osExitIsr(flag);
}


/**
 * @brief STR912 Ethernet MAC event handler
 * @param[in] interface Underlying network interface
 **/

void str912EthEventHandler(NetInterface *interface)
{
   error_t error;

   //Packet received?
   if((ENET_DMA->ISR & ENET_ISR_RX_CURR_DONE) != 0)
   {
      //Clear interrupt flag
      ENET_DMA->ISR = ENET_ISR_RX_CURR_DONE;

      //Process all pending packets
      do
      {
         //Read incoming packet
         error = str912EthReceivePacket(interface);

         //No more data in the receive buffer?
      } while(error != ERROR_BUFFER_EMPTY);
   }

   //Re-enable DMA interrupts
   ENET_DMA->IER = ENET_IER_TX_CURR_DONE_EN | ENET_IER_RX_CURR_DONE_EN;
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

error_t str912EthSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary)
{
   size_t length;

   //Retrieve the length of the packet
   length = netBufferGetLength(buffer) - offset;

   //Check the frame length
   if(length > STR912_ETH_TX_BUFFER_SIZE)
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
      //Report an error
      return ERROR_INVALID_LENGTH;
   }

   //Make sure the current buffer is available for writing
   if((txCurDmaDesc->status & ENET_TDES_STATUS_VALID) != 0)
   {
      return ERROR_FAILURE;
   }

   //Copy user data to the transmit buffer
   netBufferRead((uint8_t *) (txCurDmaDesc->start & ENET_TDES_START_ADDR),
      buffer, offset, length);

   //Write the number of bytes to send
   txCurDmaDesc->ctrl = ENET_TDES_CTRL_NXT_EN | length;
   //Give the ownership of the descriptor to the DMA
   txCurDmaDesc->status = ENET_TDES_STATUS_VALID;

   //Instruct the DMA to poll the transmit descriptor list
   ENET_DMA->TXSTR |= ENET_TXSTR_START_FETCH;

   //Point to the next descriptor in the list
   txCurDmaDesc = (Str912TxDmaDesc *) (txCurDmaDesc->next & ENET_TDES_NEXT_ADDR);

   //Check whether the next buffer is available for writing
   if((txCurDmaDesc->status & ENET_TDES_STATUS_VALID) == 0)
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

error_t str912EthReceivePacket(NetInterface *interface)
{
   error_t error;
   size_t n;
   uint8_t *p;
   NetRxAncillary ancillary;

   //Current buffer available for reading?
   if((rxCurDmaDesc->status & ENET_RDES_STATUS_VALID) == 0)
   {
      //Make sure no error occurred
      if((rxCurDmaDesc->status & ENET_RDES_STATUS_ERROR) == 0)
      {
         //Point to the received frame
         p = (uint8_t *) (rxCurDmaDesc->start & ENET_RDES_START_ADDR);

         //Retrieve the length of the frame
         n = rxCurDmaDesc->status & ENET_RDES_STATUS_FL;
         //Limit the number of data to read
         n = MIN(n, STR912_ETH_RX_BUFFER_SIZE);

         //Additional options can be passed to the stack along with the packet
         ancillary = NET_DEFAULT_RX_ANCILLARY;

         //Pass the packet to the upper layer
         nicProcessPacket(interface, p, n, &ancillary);

         //Valid packet received
         error = NO_ERROR;
      }
      else
      {
         //The received packet contains an error
         error = ERROR_INVALID_PACKET;
      }

      //Give the ownership of the descriptor back to the DMA
      rxCurDmaDesc->status = ENET_RDES_STATUS_VALID;
      //Point to the next descriptor in the list
      rxCurDmaDesc = (Str912RxDmaDesc *) (rxCurDmaDesc->next & ENET_RDES_NEXT_ADDR);
   }
   else
   {
      //No more data in the receive buffer
      error = ERROR_BUFFER_EMPTY;
   }

   //Instruct the DMA to poll the receive descriptor list
   ENET_DMA->RXSTR |= ENET_RXSTR_START_FETCH;

   //Return status code
   return error;
}


/**
 * @brief Configure MAC address filtering
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t str912EthUpdateMacAddrFilter(NetInterface *interface)
{
   uint_t i;
   uint_t k;
   uint32_t crc;
   uint32_t hashTable[2];
   MacFilterEntry *entry;

   //Debug message
   TRACE_DEBUG("Updating MAC filter...\r\n");

   //Set the MAC address of the station
   ENET_MAC->MAL = interface->macAddr.w[0] | (interface->macAddr.w[1] << 16);
   ENET_MAC->MAH = interface->macAddr.w[2];

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
         crc = str912EthCalcCrc(&entry->addr, sizeof(MacAddr));

         //The upper 6 bits in the CRC register are used to index the
         //contents of the hash table
         k = (crc >> 26) & 0x3F;

         //Update hash table contents
         hashTable[k / 32] |= (1 << (k % 32));
      }
   }

   //Write the hash table
   ENET_MAC->MCLA = hashTable[0];
   ENET_MAC->MCHA = hashTable[1];

   //Debug message
   TRACE_DEBUG("  ENET_MCLA = %08" PRIX32 "\r\n", ENET_MAC->MCLA);
   TRACE_DEBUG("  ENET_MCHA = %08" PRIX32 "\r\n", ENET_MAC->MCHA);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Adjust MAC configuration parameters for proper operation
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t str912EthUpdateMacConfig(NetInterface *interface)
{
   uint32_t config;

   //Read current MAC configuration
   config = ENET_MAC->MCR;

   //Half-duplex or full-duplex mode?
   if(interface->duplexMode == NIC_FULL_DUPLEX_MODE)
   {
      //Full-duplex mode
      config |= ENET_MCR_FDM;
      //Enable the reception path during transmission
      config &= ~ENET_MCR_DRO;
   }
   else
   {
      //Half-duplex mode
      config &= ~ENET_MCR_FDM;
      //Disable the reception path during transmission
      config |= ENET_MCR_DRO;
   }

   //Update MAC configuration register
   ENET_MAC->MCR = config;

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

void str912EthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data)
{
   uint32_t temp;

   //Valid opcode?
   if(opcode == SMI_OPCODE_WRITE)
   {
      //Set up a write operation
      temp = ENET_MIIA_WR | ENET_MIIA_BUSY;
      //PHY address
      temp |= (phyAddr << 11) & ENET_MIIA_PADDR;
      //Register address
      temp |= (regAddr << 6) & ENET_MIIA_RADDR;

      //Data to be written in the PHY register
      ENET_MAC->MIID = data & ENET_MIID_RDATA;

      //Start a write operation
      ENET_MAC->MIIA = temp;
      //Wait for the write to complete
      while((ENET_MAC->MIIA & ENET_MIIA_BUSY) != 0)
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

uint16_t str912EthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr)
{
   uint16_t data;
   uint32_t temp;

   //Valid opcode?
   if(opcode == SMI_OPCODE_READ)
   {
      //Set up a read operation
      temp = ENET_MIIA_BUSY;
      //PHY address
      temp |= (phyAddr << 11) & ENET_MIIA_PADDR;
      //Register address
      temp |= (regAddr << 6) & ENET_MIIA_RADDR;

      //Start a read operation
      ENET_MAC->MIIA = temp;
      //Wait for the read to complete
      while((ENET_MAC->MIIA & ENET_MIIA_BUSY) != 0)
      {
      }

      //Get register value
      data = ENET_MAC->MIID & ENET_MIID_RDATA;
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

uint32_t str912EthCalcCrc(const void *data, size_t length)
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
      //The message is processed bit by bit
      for(j = 0; j < 8; j++)
      {
         //Update CRC value
         if((((crc >> 31) ^ (p[i] >> j)) & 0x01) != 0)
         {
            crc = (crc << 1) ^ 0x04C11DB7;
         }
         else
         {
            crc = crc << 1;
         }
      }
   }

   //Return CRC value
   return crc;
}
