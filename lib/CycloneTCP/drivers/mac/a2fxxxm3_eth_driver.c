/**
 * @file a2fxxxm3_eth_driver.c
 * @brief SmartFusion (A2FxxxM3) Ethernet MAC driver
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
#include "a2fxxxm3.h"
#include "drivers/mss_ethernet_mac/mss_ethernet_mac_regs_driver.h"
#include "drivers/mss_ethernet_mac/mss_ethernet_mac_desc_driver.h"
#include "core/net.h"
#include "drivers/mac/a2fxxxm3_eth_driver.h"
#include "debug.h"

//Underlying network interface
static NetInterface *nicDriverInterface;

//IAR EWARM compiler?
#if defined(__ICCARM__)

//Transmit buffer
#pragma data_alignment = 4
static uint8_t txBuffer[A2FXXXM3_ETH_TX_BUFFER_COUNT][A2FXXXM3_ETH_TX_BUFFER_SIZE];
//Receive buffer
#pragma data_alignment = 4
static uint8_t rxBuffer[A2FXXXM3_ETH_RX_BUFFER_COUNT][A2FXXXM3_ETH_RX_BUFFER_SIZE];
//Transmit DMA descriptors
#pragma data_alignment = 4
static A2fxxxm3TxDmaDesc txDmaDesc[A2FXXXM3_ETH_TX_BUFFER_COUNT];
//Receive DMA descriptors
#pragma data_alignment = 4
static A2fxxxm3RxDmaDesc rxDmaDesc[A2FXXXM3_ETH_RX_BUFFER_COUNT];

//Keil MDK-ARM or GCC compiler?
#else

//Transmit buffer
static uint8_t txBuffer[A2FXXXM3_ETH_TX_BUFFER_COUNT][A2FXXXM3_ETH_TX_BUFFER_SIZE]
   __attribute__((aligned(4)));
//Receive buffer
static uint8_t rxBuffer[A2FXXXM3_ETH_RX_BUFFER_COUNT][A2FXXXM3_ETH_RX_BUFFER_SIZE]
   __attribute__((aligned(4)));
//Transmit DMA descriptors
static A2fxxxm3TxDmaDesc txDmaDesc[A2FXXXM3_ETH_TX_BUFFER_COUNT]
   __attribute__((aligned(4)));
//Receive DMA descriptors
static A2fxxxm3RxDmaDesc rxDmaDesc[A2FXXXM3_ETH_RX_BUFFER_COUNT]
   __attribute__((aligned(4)));

#endif

//Pointer to the current TX DMA descriptor
static A2fxxxm3TxDmaDesc *txCurDmaDesc;
//Pointer to the current RX DMA descriptor
static A2fxxxm3RxDmaDesc *rxCurDmaDesc;


/**
 * @brief A2FxxxM3 Ethernet MAC driver
 **/

const NicDriver a2fxxxm3EthDriver =
{
   NIC_TYPE_ETHERNET,
   ETH_MTU,
   a2fxxxm3EthInit,
   a2fxxxm3EthTick,
   a2fxxxm3EthEnableIrq,
   a2fxxxm3EthDisableIrq,
   a2fxxxm3EthEventHandler,
   a2fxxxm3EthSendPacket,
   a2fxxxm3EthUpdateMacAddrFilter,
   a2fxxxm3EthUpdateMacConfig,
   a2fxxxm3EthWritePhyReg,
   a2fxxxm3EthReadPhyReg,
   TRUE,
   TRUE,
   TRUE,
   FALSE
};


/**
 * @brief A2FxxxM3 Ethernet MAC initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t a2fxxxm3EthInit(NetInterface *interface)
{
   error_t error;

   //Debug message
   TRACE_INFO("Initializing A2FxxxM3 Ethernet MAC...\r\n");

   //Save underlying network interface
   nicDriverInterface = interface;

   //Perform a software reset
   MAC->CSR0 |= CSR0_SWR_MASK;
   //Wait for the reset to complete
   while((MAC->CSR0 & CSR0_SWR_MASK) != 0)
   {
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

   //Enable store and forward mode
   MAC->CSR6 |= CSR6_SF_MASK;

   //Initialize DMA descriptor lists
   a2fxxxm3EthInitDmaDesc(interface);

   //Enable the desired Ethernet interrupts
   MAC->CSR7 |= CSR7_NIE_MASK | CSR7_RIE_MASK | CSR7_TIE_MASK;

   //Set priority grouping (5 bits for pre-emption priority, no bits for subpriority)
   NVIC_SetPriorityGrouping(A2FXXXM3_ETH_IRQ_PRIORITY_GROUPING);

   //Configure Ethernet interrupt priority
   NVIC_SetPriority(EthernetMAC_IRQn, NVIC_EncodePriority(A2FXXXM3_ETH_IRQ_PRIORITY_GROUPING,
      A2FXXXM3_ETH_IRQ_GROUP_PRIORITY, A2FXXXM3_ETH_IRQ_SUB_PRIORITY));

   //Enable transmission and reception
   MAC->CSR6 |= CSR6_ST_MASK | CSR6_SR_MASK;

   //Set MAC address
   error = a2fxxxm3EthSendSetup(interface);
   //Any error to report?
   if(error)
   {
      return error;
   }

   //Accept any packets from the upper layer
   osSetEvent(&interface->nicTxEvent);

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief Initialize DMA descriptor lists
 * @param[in] interface Underlying network interface
 **/

void a2fxxxm3EthInitDmaDesc(NetInterface *interface)
{
   uint_t i;

   //Initialize TX DMA descriptor list
   for(i = 0; i < A2FXXXM3_ETH_TX_BUFFER_COUNT; i++)
   {
      //The descriptor is initially owned by the user
      txDmaDesc[i].tdes0 = 0;
      //Use chain structure rather than ring structure
      txDmaDesc[i].tdes1 = TDES1_TCH;
      //Transmit buffer address
      txDmaDesc[i].tdes2 = (uint32_t) txBuffer[i];
      //Next descriptor address
      txDmaDesc[i].tdes3 = (uint32_t) &txDmaDesc[i + 1];
   }

   //The last descriptor is chained to the first entry
   txDmaDesc[i - 1].tdes3 = (uint32_t) &txDmaDesc[0];
   //Point to the very first descriptor
   txCurDmaDesc = &txDmaDesc[0];

   //Initialize RX DMA descriptor list
   for(i = 0; i < A2FXXXM3_ETH_RX_BUFFER_COUNT; i++)
   {
      //The descriptor is initially owned by the DMA
      rxDmaDesc[i].rdes0 = RDES0_OWN;
      //Use chain structure rather than ring structure
      rxDmaDesc[i].rdes1 = RDES1_RCH | (A2FXXXM3_ETH_RX_BUFFER_SIZE & RDES1_RBS1_MASK);
      //Receive buffer address
      rxDmaDesc[i].rdes2 = (uint32_t) rxBuffer[i];
      //Next descriptor address
      rxDmaDesc[i].rdes3 = (uint32_t) &rxDmaDesc[i + 1];
   }

   //The last descriptor is chained to the first entry
   rxDmaDesc[i - 1].rdes3 = (uint32_t) &rxDmaDesc[0];
   //Point to the very first descriptor
   rxCurDmaDesc = &rxDmaDesc[0];

   //Start location of the TX descriptor list
   MAC->CSR4 = (uint32_t) txDmaDesc;
   //Start location of the RX descriptor list
   MAC->CSR3 = (uint32_t) rxDmaDesc;
}


/**
 * @brief A2FxxxM3 Ethernet MAC timer handler
 *
 * This routine is periodically called by the TCP/IP stack to handle periodic
 * operations such as polling the link state
 *
 * @param[in] interface Underlying network interface
 **/

void a2fxxxm3EthTick(NetInterface *interface)
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

void a2fxxxm3EthEnableIrq(NetInterface *interface)
{
   //Enable Ethernet MAC interrupts
   NVIC_EnableIRQ(EthernetMAC_IRQn);

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

void a2fxxxm3EthDisableIrq(NetInterface *interface)
{
   //Disable Ethernet MAC interrupts
   NVIC_DisableIRQ(EthernetMAC_IRQn);

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
 * @brief A2FxxxM3 Ethernet MAC interrupt service routine
 **/

void EthernetMAC_IRQHandler(void)
{
   bool_t flag;
   uint32_t status;

   //Interrupt service routine prologue
   osEnterIsr();

   //This flag will be set if a higher priority task must be woken
   flag = FALSE;

   //Read interrupt status register
   status = MAC->CSR5;

   //Packet transmitted?
   if((status & CSR5_TI_MASK) != 0)
   {
      //Clear TI interrupt flag
      MAC->CSR5 = CSR5_TI_MASK;

      //Check whether the TX buffer is available for writing
      if((txCurDmaDesc->tdes0 & TDES0_OWN) == 0)
      {
         //Notify the TCP/IP stack that the transmitter is ready to send
         flag |= osSetEventFromIsr(&nicDriverInterface->nicTxEvent);
      }
   }

   //Packet received?
   if((status & CSR5_RI_MASK) != 0)
   {
      //Disable RIE interrupt
      MAC->CSR7 &= ~CSR7_RIE_MASK;

      //Set event flag
      nicDriverInterface->nicEvent = TRUE;
      //Notify the TCP/IP stack of the event
      flag |= osSetEventFromIsr(&netEvent);
   }

   //Clear NIS interrupt flag
   MAC->CSR5 = CSR5_NIS_MASK;

   //Interrupt service routine epilogue
   osExitIsr(flag);
}


/**
 * @brief A2FxxxM3 Ethernet MAC event handler
 * @param[in] interface Underlying network interface
 **/

void a2fxxxm3EthEventHandler(NetInterface *interface)
{
   error_t error;

   //Packet received?
   if((MAC->CSR5 & CSR5_RI_MASK) != 0)
   {
      //Clear interrupt flag
      MAC->CSR5 = CSR5_RI_MASK;

      //Process all pending packets
      do
      {
         //Read incoming packet
         error = a2fxxxm3EthReceivePacket(interface);

         //No more data in the receive buffer?
      } while(error != ERROR_BUFFER_EMPTY);
   }

   //Re-enable Ethernet interrupts
   MAC->CSR7 |= CSR7_NIE_MASK | CSR7_RIE_MASK | CSR7_TIE_MASK;
}


/**
 * @brief Send a setup frame
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t a2fxxxm3EthSendSetup(NetInterface *interface)
{
   A2fxxxm3HashTableSetupFrame *setupFrame;

   //Make sure the current buffer is available for writing
   if((txCurDmaDesc->tdes0 & TDES0_OWN) != 0)
   {
      return ERROR_FAILURE;
   }

   //Point to the buffer where to format the setup frame
   setupFrame = (A2fxxxm3HashTableSetupFrame *) txCurDmaDesc->tdes2;

   //Clear contents
   osMemset(setupFrame, 0, sizeof(A2fxxxm3HashTableSetupFrame));

   //Set MAC address
   setupFrame->physicalAddr[0] = interface->macAddr.w[0];
   setupFrame->physicalAddr[1] = interface->macAddr.w[1];
   setupFrame->physicalAddr[2] = interface->macAddr.w[2];

   //Write the number of bytes to send
   txCurDmaDesc->tdes1 = sizeof(A2fxxxm3HashTableSetupFrame) & TDES1_TBS1_MASK;
   //The SET flag indicates that this is a setup frame descriptor
   txCurDmaDesc->tdes1 |= TDES1_SET | TDES1_TCH | TDES1_FT0;
   //Give the ownership of the descriptor to the DMA
   txCurDmaDesc->tdes0 |= TDES0_OWN;

   //Instruct the DMA to poll the transmit descriptor list
   MAC->CSR1 = 1;

   //Point to the next descriptor in the list
   txCurDmaDesc = (A2fxxxm3TxDmaDesc *) txCurDmaDesc->tdes3;

   //Data successfully written
   return NO_ERROR;
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

error_t a2fxxxm3EthSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary)
{
   size_t length;

   //Retrieve the length of the packet
   length = netBufferGetLength(buffer) - offset;

   //Check the frame length
   if(length > A2FXXXM3_ETH_TX_BUFFER_SIZE)
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
      //Report an error
      return ERROR_INVALID_LENGTH;
   }

   //Make sure the current buffer is available for writing
   if((txCurDmaDesc->tdes0 & TDES0_OWN) != 0)
   {
      return ERROR_FAILURE;
   }

   //Copy user data to the transmit buffer
   netBufferRead((uint8_t *) txCurDmaDesc->tdes2, buffer, offset, length);

   //Write the number of bytes to send
   txCurDmaDesc->tdes1 = length & TDES1_TBS1_MASK;
   //Set LS and FS flags as the data fits in a single buffer
   txCurDmaDesc->tdes1 |= TDES1_IC | TDES1_LS | TDES1_FS | TDES1_TCH;
   //Give the ownership of the descriptor to the DMA
   txCurDmaDesc->tdes0 |= TDES0_OWN;

   //Instruct the DMA to poll the transmit descriptor list
   MAC->CSR1 = 1;

   //Point to the next descriptor in the list
   txCurDmaDesc = (A2fxxxm3TxDmaDesc *) txCurDmaDesc->tdes3;

   //Check whether the next buffer is available for writing
   if((txCurDmaDesc->tdes0 & TDES0_OWN) == 0)
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

error_t a2fxxxm3EthReceivePacket(NetInterface *interface)
{
   error_t error;
   size_t n;
   NetRxAncillary ancillary;

   //Current buffer available for reading?
   if((rxCurDmaDesc->rdes0 & RDES0_OWN) == 0)
   {
      //FS and LS flags should be set
      if((rxCurDmaDesc->rdes0 & RDES0_FS) != 0 &&
         (rxCurDmaDesc->rdes0 & RDES0_LS) != 0)
      {
         //Make sure no error occurred
         if((rxCurDmaDesc->rdes0 & RDES0_ES) == 0)
         {
            //Retrieve the length of the frame
            n = (rxCurDmaDesc->rdes0 >> RDES0_FL_OFFSET) & RDES0_FL_MASK;
            //Limit the number of data to read
            n = MIN(n, A2FXXXM3_ETH_RX_BUFFER_SIZE);

            //Additional options can be passed to the stack along with the packet
            ancillary = NET_DEFAULT_RX_ANCILLARY;

            //Pass the packet to the upper layer
            nicProcessPacket(interface, (uint8_t *) rxCurDmaDesc->rdes2, n,
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

      //Give the ownership of the descriptor back to the DMA
      rxCurDmaDesc->rdes0 = RDES0_OWN;
      //Point to the next descriptor in the list
      rxCurDmaDesc = (A2fxxxm3RxDmaDesc *) rxCurDmaDesc->rdes3;
   }
   else
   {
      //No more data in the receive buffer
      error = ERROR_BUFFER_EMPTY;
   }

   //Instruct the DMA to poll the receive descriptor list
   MAC->CSR2 = 1;

   //Return status code
   return error;
}


/**
 * @brief Configure MAC address filtering
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t a2fxxxm3EthUpdateMacAddrFilter(NetInterface *interface)
{
   uint_t i;
   bool_t acceptMulticast;

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
      MAC->CSR6 |= CSR6_PM_MASK;
   }
   else
   {
      MAC->CSR6 &= ~CSR6_PM_MASK;
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Adjust MAC configuration parameters for proper operation
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t a2fxxxm3EthUpdateMacConfig(NetInterface *interface)
{
   //Stop transmission
   while(((MAC->CSR5 & CSR5_TS_MASK) >> CSR5_TS_SHIFT) != CSR5_TS_STOPPED)
      MAC->CSR6 &= ~CSR6_ST_MASK;

   //Stop reception
   while(((MAC->CSR5 & CSR5_RS_MASK) >> CSR5_RS_SHIFT) != CSR5_RS_STOPPED)
      MAC->CSR6 &= ~CSR6_SR_MASK;

   //10BASE-T or 100BASE-TX operation mode?
   if(interface->linkSpeed == NIC_LINK_SPEED_100MBPS)
   {
      MAC->CSR6 |= CSR6_TTM_MASK;
   }
   else
   {
      MAC->CSR6 &= ~CSR6_TTM_MASK;
   }

   //Half-duplex or full-duplex mode?
   if(interface->duplexMode == NIC_FULL_DUPLEX_MODE)
   {
      MAC->CSR6 |= CSR6_FD_MASK;
   }
   else
   {
      MAC->CSR6 &= ~CSR6_FD_MASK;
   }

   //Restart transmission and reception
   MAC->CSR6 |= CSR6_ST_MASK | CSR6_SR_MASK;

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

void a2fxxxm3EthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data)
{
   //Synchronization pattern
   a2fxxxm3EthWriteSmi(SMI_SYNC, 32);
   //Start of frame
   a2fxxxm3EthWriteSmi(SMI_START, 2);
   //Set up a write operation
   a2fxxxm3EthWriteSmi(opcode, 2);
   //Write PHY address
   a2fxxxm3EthWriteSmi(phyAddr, 5);
   //Write register address
   a2fxxxm3EthWriteSmi(regAddr, 5);
   //Turnaround
   a2fxxxm3EthWriteSmi(SMI_TA, 2);
   //Write register value
   a2fxxxm3EthWriteSmi(data, 16);
   //Release MDIO
   a2fxxxm3EthReadSmi(1);
}


/**
 * @brief Read PHY register
 * @param[in] opcode Access type (2 bits)
 * @param[in] phyAddr PHY address (5 bits)
 * @param[in] regAddr Register address (5 bits)
 * @return Register value
 **/

uint16_t a2fxxxm3EthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr)
{
   uint16_t data;

   //Synchronization pattern
   a2fxxxm3EthWriteSmi(SMI_SYNC, 32);
   //Start of frame
   a2fxxxm3EthWriteSmi(SMI_START, 2);
   //Set up a read operation
   a2fxxxm3EthWriteSmi(opcode, 2);
   //Write PHY address
   a2fxxxm3EthWriteSmi(phyAddr, 5);
   //Write register address
   a2fxxxm3EthWriteSmi(regAddr, 5);
   //Turnaround to avoid contention
   a2fxxxm3EthReadSmi(1);
   //Read register value
   data = a2fxxxm3EthReadSmi(16);
   //Force the PHY to release the MDIO pin
   a2fxxxm3EthReadSmi(1);

   //Return PHY register contents
   return data;
}


/**
 * @brief SMI write operation
 * @param[in] data Raw data to be written
 * @param[in] length Number of bits to be written
 **/

void a2fxxxm3EthWriteSmi(uint32_t data, uint_t length)
{
   //Skip the most significant bits since they are meaningless
   data <<= 32 - length;

   //Configure MDIO as an output
   MAC->CSR9 |= CSR9_MDEN_MASK;

   //Write the specified number of bits
   while(length--)
   {
      //Write MDIO
      if((data & 0x80000000) != 0)
      {
         MAC->CSR9 |= CSR9_MDO_MASK;
      }
      else
      {
         MAC->CSR9 &= ~CSR9_MDO_MASK;
      }

      //Assert MDC
      usleep(1);
      MAC->CSR9 |= CSR9_MDC_MASK;
      //Deassert MDC
      usleep(1);
      MAC->CSR9 &= ~CSR9_MDC_MASK;

      //Rotate data
      data <<= 1;
   }
}


/**
 * @brief SMI read operation
 * @param[in] length Number of bits to be read
 * @return Data resulting from the MDIO read operation
 **/

uint32_t a2fxxxm3EthReadSmi(uint_t length)
{
   uint32_t data = 0;

   //Configure MDIO as an input
   MAC->CSR9 &= ~CSR9_MDEN_MASK;

   //Read the specified number of bits
   while(length--)
   {
      //Rotate data
      data <<= 1;

      //Assert MDC
      MAC->CSR9 |= CSR9_MDC_MASK;
      usleep(1);
      //Deassert MDC
      MAC->CSR9 &= ~CSR9_MDC_MASK;
      usleep(1);

      //Check MDIO state
      if((MAC->CSR9 & CSR9_MDI_MASK) != 0)
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

uint32_t a2fxxxm3EthCalcCrc(const void *data, size_t length)
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
   return crc;
}
