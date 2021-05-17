/**
 * @file lpc175x_eth_driver.c
 * @brief LPC1758 Ethernet MAC driver
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
#include "lpc17xx.h"
#include "core/net.h"
#include "drivers/mac/lpc175x_eth_driver.h"
#include "debug.h"

//Underlying network interface
static NetInterface *nicDriverInterface;

//IAR EWARM compiler?
#if defined(__ICCARM__)

//Transmit buffer
#pragma data_alignment = 4
static uint8_t txBuffer[LPC175X_ETH_TX_BUFFER_COUNT][LPC175X_ETH_TX_BUFFER_SIZE];
//Receive buffer
#pragma data_alignment = 4
static uint8_t rxBuffer[LPC175X_ETH_RX_BUFFER_COUNT][LPC175X_ETH_RX_BUFFER_SIZE];
//Transmit descriptors
#pragma data_alignment = 4
static Lpc175xTxDesc txDesc[LPC175X_ETH_TX_BUFFER_COUNT];
//Transmit status array
#pragma data_alignment = 4
static Lpc175xTxStatus txStatus[LPC175X_ETH_TX_BUFFER_COUNT];
//Receive descriptors
#pragma data_alignment = 4
static Lpc175xRxDesc rxDesc[LPC175X_ETH_RX_BUFFER_COUNT];
//Receive status array
#pragma data_alignment = 8
static Lpc175xRxStatus rxStatus[LPC175X_ETH_RX_BUFFER_COUNT];

//Keil MDK-ARM or GCC compiler?
#else

//Transmit buffer
static uint8_t txBuffer[LPC175X_ETH_TX_BUFFER_COUNT][LPC175X_ETH_TX_BUFFER_SIZE]
   __attribute__((aligned(4)));
//Receive buffer
static uint8_t rxBuffer[LPC175X_ETH_RX_BUFFER_COUNT][LPC175X_ETH_RX_BUFFER_SIZE]
   __attribute__((aligned(4)));
//Transmit descriptors
static Lpc175xTxDesc txDesc[LPC175X_ETH_TX_BUFFER_COUNT]
   __attribute__((aligned(4)));
//Transmit status array
static Lpc175xTxStatus txStatus[LPC175X_ETH_TX_BUFFER_COUNT]
   __attribute__((aligned(4)));
//Receive descriptors
static Lpc175xRxDesc rxDesc[LPC175X_ETH_RX_BUFFER_COUNT]
   __attribute__((aligned(4)));
//Receive status array
static Lpc175xRxStatus rxStatus[LPC175X_ETH_RX_BUFFER_COUNT]
   __attribute__((aligned(8)));

#endif


/**
 * @brief LPC175x Ethernet MAC driver
 **/

const NicDriver lpc175xEthDriver =
{
   NIC_TYPE_ETHERNET,
   ETH_MTU,
   lpc175xEthInit,
   lpc175xEthTick,
   lpc175xEthEnableIrq,
   lpc175xEthDisableIrq,
   lpc175xEthEventHandler,
   lpc175xEthSendPacket,
   lpc175xEthUpdateMacAddrFilter,
   lpc175xEthUpdateMacConfig,
   lpc175xEthWritePhyReg,
   lpc175xEthReadPhyReg,
   TRUE,
   TRUE,
   TRUE,
   FALSE
};


/**
 * @brief LPC175x Ethernet MAC initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t lpc175xEthInit(NetInterface *interface)
{
   error_t error;

   //Debug message
   TRACE_INFO("Initializing LPC175x Ethernet MAC...\r\n");

   //Save underlying network interface
   nicDriverInterface = interface;

   //Power up EMAC controller
   LPC_SC->PCONP |= PCONP_PCENET;

   //GPIO configuration
   lpc175xEthInitGpio(interface);

   //Reset host registers, transmit datapath and receive datapath
   LPC_EMAC->Command = COMMAND_RX_RESET | COMMAND_TX_RESET | COMMAND_REG_RESET;

   //Reset EMAC controller
   LPC_EMAC->MAC1 = MAC1_SOFT_RESET | MAC1_SIMULATION_RESET |
      MAC1_RESET_MCS_RX | MAC1_RESET_RX | MAC1_RESET_MCS_TX | MAC1_RESET_TX;

   //Initialize MAC related registers
   LPC_EMAC->MAC1 = 0;
   LPC_EMAC->MAC2 = MAC2_PAD_CRC_ENABLE | MAC2_CRC_ENABLE;
   LPC_EMAC->IPGR = IPGR_DEFAULT_VALUE;
   LPC_EMAC->CLRT = CLRT_DEFAULT_VALUE;

   //Select RMII mode
   LPC_EMAC->Command = COMMAND_RMII;

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

   //Initialize TX and RX descriptor arrays
   lpc175xEthInitDesc(interface);

   //Set the MAC address of the station
   LPC_EMAC->SA0 = interface->macAddr.w[2];
   LPC_EMAC->SA1 = interface->macAddr.w[1];
   LPC_EMAC->SA2 = interface->macAddr.w[0];

   //Initialize hash table
   LPC_EMAC->HashFilterL = 0;
   LPC_EMAC->HashFilterH = 0;

   //Configure the receive filter
   LPC_EMAC->RxFilterCtrl = RFC_ACCEPT_PERFECT_EN |
      RFC_ACCEPT_MULTICAST_HASH_EN | RFC_ACCEPT_BROADCAST_EN;

   //Program the MAXF register with the maximum frame length to be accepted
   LPC_EMAC->MAXF = LPC175X_ETH_RX_BUFFER_SIZE;

   //Reset EMAC interrupt flags
   LPC_EMAC->IntClear  = 0xFFFF;
   //Enable desired EMAC interrupts
   LPC_EMAC->IntEnable = INT_TX_DONE | INT_RX_DONE;

   //Set priority grouping (5 bits for pre-emption priority, no bits for subpriority)
   NVIC_SetPriorityGrouping(LPC175X_ETH_IRQ_PRIORITY_GROUPING);

   //Configure Ethernet interrupt priority
   NVIC_SetPriority(ENET_IRQn, NVIC_EncodePriority(LPC175X_ETH_IRQ_PRIORITY_GROUPING,
      LPC175X_ETH_IRQ_GROUP_PRIORITY, LPC175X_ETH_IRQ_SUB_PRIORITY));

   //Enable transmission and reception
   LPC_EMAC->Command |= COMMAND_TX_ENABLE | COMMAND_RX_ENABLE;
   //Allow frames to be received
   LPC_EMAC->MAC1 |= MAC1_RECEIVE_ENABLE;

   //Accept any packets from the upper layer
   osSetEvent(&interface->nicTxEvent);

   //Successful initialization
   return NO_ERROR;
}


//MCB1758 evaluation board?
#if defined(USE_MCB1758)

/**
 * @brief GPIO configuration
 * @param[in] interface Underlying network interface
 **/

void lpc175xEthInitGpio(NetInterface *interface)
{
   //Configure P1.0 (ENET_TXD0), P1.1 (ENET_TXD1), P1.4 (ENET_TX_EN), P1.8 (ENET_CRS),
   //P1.9 (ENET_RXD0), P1.10 (ENET_RXD1), P1.14 (RX_ER) and P1.15 (ENET_REF_CLK)
   LPC_PINCON->PINSEL2 &= ~(PINSEL2_P1_0_MASK | PINSEL2_P1_1_MASK |
      PINSEL2_P1_4_MASK | PINSEL2_P1_8_MASK | PINSEL2_P1_9_MASK |
      PINSEL2_P1_10_MASK | PINSEL2_P1_14_MASK | PINSEL2_P1_15_MASK);

   LPC_PINCON->PINSEL2 |= PINSEL2_P1_0_ENET_TXD0 | PINSEL2_P1_1_ENET_TXD1 |
      PINSEL2_P1_4_ENET_TX_EN | PINSEL2_P1_8_ENET_CRS | PINSEL2_P1_9_ENET_RXD0 |
      PINSEL2_P1_10_ENET_RXD1 | PINSEL2_P1_14_ENET_RX_ER | PINSEL2_P1_15_ENET_REF_CLK;

   //Configure P2.8 (ENET_MDC) and P2.9 (ENET_MDIO) as GPIOs
   LPC_PINCON->PINSEL4 &= ~(PINSEL4_P2_8_MASK | PINSEL4_P2_9_MASK);
   LPC_PINCON->PINSEL4 |= PINSEL4_P2_8_GPIO | PINSEL4_P2_9_GPIO;

   //SMI software implementation to drive MDC and MDIO
   LPC175X_ETH_MDC_GPIO->FIOCLR = LPC175X_ETH_MDC_MASK;
   LPC175X_ETH_MDC_GPIO->FIODIR |= LPC175X_ETH_MDC_MASK;
   LPC175X_ETH_MDIO_GPIO->FIODIR &= ~LPC175X_ETH_MDIO_MASK;
}

#endif


/**
 * @brief Initialize TX and RX descriptors
 * @param[in] interface Underlying network interface
 **/

void lpc175xEthInitDesc(NetInterface *interface)
{
   uint_t i;

   //Initialize TX descriptors
   for(i = 0; i < LPC175X_ETH_TX_BUFFER_COUNT; i++)
   {
      //Base address of the buffer containing transmit data
      txDesc[i].packet = (uint32_t) txBuffer[i];
      //Transmit descriptor control word
      txDesc[i].control = 0;
      //Transmit status information word
      txStatus[i].info = 0;
   }

   //Initialize RX descriptors
   for(i = 0; i < LPC175X_ETH_RX_BUFFER_COUNT; i++)
   {
      //Base address of the buffer for storing receive data
      rxDesc[i].packet = (uint32_t) rxBuffer[i];
      //Receive descriptor control word
      rxDesc[i].control = RX_CTRL_INTERRUPT | (LPC175X_ETH_RX_BUFFER_SIZE - 1);
      //Receive status information word
      rxStatus[i].info = 0;
      //Receive status HashCRC word
      rxStatus[i].hashCrc = 0;
   }

   //Initialize EMAC transmit descriptor registers
   LPC_EMAC->TxDescriptor = (uint32_t) txDesc;
   LPC_EMAC->TxStatus = (uint32_t) txStatus;
   LPC_EMAC->TxDescriptorNumber = LPC175X_ETH_TX_BUFFER_COUNT - 1;
   LPC_EMAC->TxProduceIndex = 0;

   //Initialize EMAC receive descriptor registers
   LPC_EMAC->RxDescriptor = (uint32_t) rxDesc;
   LPC_EMAC->RxStatus = (uint32_t) rxStatus;
   LPC_EMAC->RxDescriptorNumber = LPC175X_ETH_RX_BUFFER_COUNT - 1;
   LPC_EMAC->RxConsumeIndex = 0;
}


/**
 * @brief LPC175x Ethernet MAC timer handler
 *
 * This routine is periodically called by the TCP/IP stack to handle periodic
 * operations such as polling the link state
 *
 * @param[in] interface Underlying network interface
 **/

void lpc175xEthTick(NetInterface *interface)
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

void lpc175xEthEnableIrq(NetInterface *interface)
{
   //Enable Ethernet MAC interrupts
   NVIC_EnableIRQ(ENET_IRQn);

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

void lpc175xEthDisableIrq(NetInterface *interface)
{
   //Disable Ethernet MAC interrupts
   NVIC_DisableIRQ(ENET_IRQn);

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
 * @brief LPC175x Ethernet MAC interrupt service routine
 **/

void ENET_IRQHandler(void)
{
   uint_t i;
   bool_t flag;
   uint32_t status;

   //Interrupt service routine prologue
   osEnterIsr();

   //This flag will be set if a higher priority task must be woken
   flag = FALSE;

   //Read interrupt status register
   status = LPC_EMAC->IntStatus;

   //Packet transmitted?
   if((status & INT_TX_DONE) != 0)
   {
      //Clear TxDone interrupt flag
      LPC_EMAC->IntClear = INT_TX_DONE;

      //Get the index of the next descriptor
      i = LPC_EMAC->TxProduceIndex + 1;

      //Wrap around if necessary
      if(i >= LPC175X_ETH_TX_BUFFER_COUNT)
      {
         i = 0;
      }

      //Check whether the TX buffer is available for writing
      if(i != LPC_EMAC->TxConsumeIndex)
      {
         //Notify the TCP/IP stack that the transmitter is ready to send
         flag |= osSetEventFromIsr(&nicDriverInterface->nicTxEvent);
      }
   }

   //Packet received?
   if((status & INT_RX_DONE) != 0)
   {
      //Disable RxDone interrupts
      LPC_EMAC->IntEnable &= ~INT_RX_DONE;

      //Set event flag
      nicDriverInterface->nicEvent = TRUE;
      //Notify the TCP/IP stack of the event
      flag |= osSetEventFromIsr(&netEvent);
   }

   //Interrupt service routine epilogue
   osExitIsr(flag);
}


/**
 * @brief LPC175x Ethernet MAC event handler
 * @param[in] interface Underlying network interface
 **/

void lpc175xEthEventHandler(NetInterface *interface)
{
   error_t error;

   //Packet received?
   if((LPC_EMAC->IntStatus & INT_RX_DONE) != 0)
   {
      //Clear RxDone interrupt flag
      LPC_EMAC->IntClear = INT_RX_DONE;

      //Process all pending packets
      do
      {
         //Read incoming packet
         error = lpc175xEthReceivePacket(interface);

         //No more data in the receive buffer?
      } while(error != ERROR_BUFFER_EMPTY);
   }

   //Re-enable TxDone and RxDone interrupts
   LPC_EMAC->IntEnable = INT_TX_DONE | INT_RX_DONE;
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

error_t lpc175xEthSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary)
{
   uint_t i;
   uint_t j;
   size_t length;

   //Retrieve the length of the packet
   length = netBufferGetLength(buffer) - offset;

   //Check the frame length
   if(!length)
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
      //We are done since the buffer is empty
      return NO_ERROR;
   }
   else if(length > LPC175X_ETH_TX_BUFFER_SIZE)
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
      //Report an error
      return ERROR_INVALID_LENGTH;
   }

   //Get the index of the current descriptor
   i = LPC_EMAC->TxProduceIndex;
   //Get the index of the next descriptor
   j = i + 1;

   //Wrap around if necessary
   if(j >= LPC175X_ETH_TX_BUFFER_COUNT)
   {
      j = 0;
   }

   //Check whether the transmit descriptor array is full
   if(j == LPC_EMAC->TxConsumeIndex)
   {
      return ERROR_FAILURE;
   }

   //Copy user data to the transmit buffer
   netBufferRead((uint8_t *) txDesc[i].packet, buffer, offset, length);

   //Write the transmit control word
   txDesc[i].control = TX_CTRL_INTERRUPT | TX_CTRL_LAST |
      TX_CTRL_CRC | TX_CTRL_PAD | ((length - 1) & TX_CTRL_SIZE);

   //Increment index and wrap around if necessary
   if(++i >= LPC175X_ETH_TX_BUFFER_COUNT)
   {
      i = 0;
   }

   //Save the resulting value
   LPC_EMAC->TxProduceIndex = i;

   //Get the index of the next descriptor
   j = i + 1;

   //Wrap around if necessary
   if(j >= LPC175X_ETH_TX_BUFFER_COUNT)
   {
      j = 0;
   }

   //Check whether the next buffer is available for writing
   if(j != LPC_EMAC->TxConsumeIndex)
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

error_t lpc175xEthReceivePacket(NetInterface *interface)
{
   error_t error;
   size_t n;
   uint_t i;
   NetRxAncillary ancillary;

   //Point to the current descriptor
   i = LPC_EMAC->RxConsumeIndex;

   //Current buffer available for reading?
   if(i != LPC_EMAC->RxProduceIndex)
   {
      //Retrieve the length of the frame
      n = (rxStatus[i].info & RX_STATUS_SIZE) + 1;
      //Limit the number of data to read
      n = MIN(n, LPC175X_ETH_RX_BUFFER_SIZE);

      //Additional options can be passed to the stack along with the packet
      ancillary = NET_DEFAULT_RX_ANCILLARY;

      //Pass the packet to the upper layer
      nicProcessPacket(interface, (uint8_t *) rxDesc[i].packet, n, &ancillary);

      //Increment index and wrap around if necessary
      if(++i >= LPC175X_ETH_RX_BUFFER_COUNT)
      {
         i = 0;
      }

      //Save the resulting value
      LPC_EMAC->RxConsumeIndex = i;

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

error_t lpc175xEthUpdateMacAddrFilter(NetInterface *interface)
{
   uint_t i;
   uint_t k;
   uint32_t crc;
   uint32_t hashTable[2];
   MacFilterEntry *entry;

   //Debug message
   TRACE_DEBUG("Updating MAC filter...\r\n");

   //Set the MAC address of the station
   LPC_EMAC->SA0 = interface->macAddr.w[2];
   LPC_EMAC->SA1 = interface->macAddr.w[1];
   LPC_EMAC->SA2 = interface->macAddr.w[0];

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
         crc = lpc175xEthCalcCrc(&entry->addr, sizeof(MacAddr));
         //Bits [28:23] are used to form the hash
         k = (crc >> 23) & 0x3F;
         //Update hash table contents
         hashTable[k / 32] |= (1 << (k % 32));
      }
   }

   //Write the hash table
   LPC_EMAC->HashFilterL = hashTable[0];
   LPC_EMAC->HashFilterH = hashTable[1];

   //Debug message
   TRACE_DEBUG("  HashFilterL = %08" PRIX32 "\r\n", LPC_EMAC->HashFilterL);
   TRACE_DEBUG("  HashFilterH = %08" PRIX32 "\r\n", LPC_EMAC->HashFilterH);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Adjust MAC configuration parameters for proper operation
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t lpc175xEthUpdateMacConfig(NetInterface *interface)
{
   //10BASE-T or 100BASE-TX operation mode?
   if(interface->linkSpeed == NIC_LINK_SPEED_100MBPS)
   {
      LPC_EMAC->SUPP = SUPP_SPEED;
   }
   else
   {
      LPC_EMAC->SUPP = 0;
   }

   //Half-duplex or full-duplex mode?
   if(interface->duplexMode == NIC_FULL_DUPLEX_MODE)
   {
      //The MAC operates in full-duplex mode
      LPC_EMAC->MAC2 |= MAC2_FULL_DUPLEX;
      LPC_EMAC->Command |= COMMAND_FULL_DUPLEX;
      //Configure Back-to-Back Inter-Packet Gap
      LPC_EMAC->IPGT = IPGT_FULL_DUPLEX;
   }
   else
   {
      //The MAC operates in half-duplex mode
      LPC_EMAC->MAC2 &= ~MAC2_FULL_DUPLEX;
      LPC_EMAC->Command &= ~COMMAND_FULL_DUPLEX;
      //Configure Back-to-Back Inter-Packet Gap
      LPC_EMAC->IPGT = IPGT_HALF_DUPLEX;
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

void lpc175xEthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data)
{
   //Synchronization pattern
   lpc175xEthWriteSmi(SMI_SYNC, 32);
   //Start of frame
   lpc175xEthWriteSmi(SMI_START, 2);
   //Set up a write operation
   lpc175xEthWriteSmi(opcode, 2);
   //Write PHY address
   lpc175xEthWriteSmi(phyAddr, 5);
   //Write register address
   lpc175xEthWriteSmi(regAddr, 5);
   //Turnaround
   lpc175xEthWriteSmi(SMI_TA, 2);
   //Write register value
   lpc175xEthWriteSmi(data, 16);
   //Release MDIO
   lpc175xEthReadSmi(1);
}


/**
 * @brief Read PHY register
 * @param[in] opcode Access type (2 bits)
 * @param[in] phyAddr PHY address (5 bits)
 * @param[in] regAddr Register address (5 bits)
 * @return Register value
 **/

uint16_t lpc175xEthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr)
{
   uint16_t data;

   //Synchronization pattern
   lpc175xEthWriteSmi(SMI_SYNC, 32);
   //Start of frame
   lpc175xEthWriteSmi(SMI_START, 2);
   //Set up a read operation
   lpc175xEthWriteSmi(opcode, 2);
   //Write PHY address
   lpc175xEthWriteSmi(phyAddr, 5);
   //Write register address
   lpc175xEthWriteSmi(regAddr, 5);
   //Turnaround to avoid contention
   lpc175xEthReadSmi(1);
   //Read register value
   data = lpc175xEthReadSmi(16);
   //Force the PHY to release the MDIO pin
   lpc175xEthReadSmi(1);

   //Return PHY register contents
   return data;
}


/**
 * @brief SMI write operation
 * @param[in] data Raw data to be written
 * @param[in] length Number of bits to be written
 **/

void lpc175xEthWriteSmi(uint32_t data, uint_t length)
{
   //Skip the most significant bits since they are meaningless
   data <<= 32 - length;

   //Configure MDIO as an output
   LPC175X_ETH_MDIO_GPIO->FIODIR |= LPC175X_ETH_MDIO_MASK;

   //Write the specified number of bits
   while(length--)
   {
      //Write MDIO
      if((data & 0x80000000) != 0)
      {
         LPC175X_ETH_MDIO_GPIO->FIOSET = LPC175X_ETH_MDIO_MASK;
      }
      else
      {
         LPC175X_ETH_MDIO_GPIO->FIOCLR = LPC175X_ETH_MDIO_MASK;
      }

      //Assert MDC
      usleep(1);
      LPC175X_ETH_MDC_GPIO->FIOSET = LPC175X_ETH_MDC_MASK;
      //Deassert MDC
      usleep(1);
      LPC175X_ETH_MDC_GPIO->FIOCLR = LPC175X_ETH_MDC_MASK;

      //Rotate data
      data <<= 1;
   }
}


/**
 * @brief SMI read operation
 * @param[in] length Number of bits to be read
 * @return Data resulting from the MDIO read operation
 **/

uint32_t lpc175xEthReadSmi(uint_t length)
{
   uint32_t data = 0;

   //Configure MDIO as an input
   LPC175X_ETH_MDIO_GPIO->FIODIR &= ~LPC175X_ETH_MDIO_MASK;

   //Read the specified number of bits
   while(length--)
   {
      //Rotate data
      data <<= 1;

      //Assert MDC
      LPC175X_ETH_MDC_GPIO->FIOSET = LPC175X_ETH_MDC_MASK;
      usleep(1);
      //Deassert MDC
      LPC175X_ETH_MDC_GPIO->FIOCLR = LPC175X_ETH_MDC_MASK;
      usleep(1);

      //Check MDIO state
      if((LPC175X_ETH_MDIO_GPIO->FIOPIN & LPC175X_ETH_MDIO_MASK) != 0)
      {
         data |= 0x01;
      }
   }

   //Return the received data
   return data;
}


/**
 * @brief CRC calculation
 * @param[in] data Pointer to the data over which to calculate the CRC
 * @param[in] length Number of bytes to process
 * @return Resulting CRC value
 **/

uint32_t lpc175xEthCalcCrc(const void *data, size_t length)
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
