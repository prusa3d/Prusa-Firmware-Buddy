/**
 * @file xmc4800_eth_driver.c
 * @brief Infineon XMC4800 Ethernet MAC driver
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
#include "xmc4800.h"
#include "core/net.h"
#include "drivers/mac/xmc4800_eth_driver.h"
#include "debug.h"

//Underlying network interface
static NetInterface *nicDriverInterface;

//IAR EWARM compiler?
#if defined(__ICCARM__)

//Transmit buffer
#pragma data_alignment = 4
#pragma location = XMC4800_ETH_RAM_SECTION
static uint8_t txBuffer[XMC4800_ETH_TX_BUFFER_COUNT][XMC4800_ETH_TX_BUFFER_SIZE];
//Receive buffer
#pragma data_alignment = 4
#pragma location = XMC4800_ETH_RAM_SECTION
static uint8_t rxBuffer[XMC4800_ETH_RX_BUFFER_COUNT][XMC4800_ETH_RX_BUFFER_SIZE];
//Transmit DMA descriptors
#pragma data_alignment = 4
#pragma location = XMC4800_ETH_RAM_SECTION
static Xmc4800TxDmaDesc txDmaDesc[XMC4800_ETH_TX_BUFFER_COUNT];
//Receive DMA descriptors
#pragma data_alignment = 4
#pragma location = XMC4800_ETH_RAM_SECTION
static Xmc4800RxDmaDesc rxDmaDesc[XMC4800_ETH_RX_BUFFER_COUNT];

//Keil MDK-ARM or GCC compiler?
#else

//Transmit buffer
static uint8_t txBuffer[XMC4800_ETH_TX_BUFFER_COUNT][XMC4800_ETH_TX_BUFFER_SIZE]
   __attribute__((aligned(4), __section__(XMC4800_ETH_RAM_SECTION)));
//Receive buffer
static uint8_t rxBuffer[XMC4800_ETH_RX_BUFFER_COUNT][XMC4800_ETH_RX_BUFFER_SIZE]
   __attribute__((aligned(4), __section__(XMC4800_ETH_RAM_SECTION)));
//Transmit DMA descriptors
static Xmc4800TxDmaDesc txDmaDesc[XMC4800_ETH_TX_BUFFER_COUNT]
   __attribute__((aligned(4), __section__(XMC4800_ETH_RAM_SECTION)));
//Receive DMA descriptors
static Xmc4800RxDmaDesc rxDmaDesc[XMC4800_ETH_RX_BUFFER_COUNT]
   __attribute__((aligned(4), __section__(XMC4800_ETH_RAM_SECTION)));

#endif

//Pointer to the current TX DMA descriptor
static Xmc4800TxDmaDesc *txCurDmaDesc;
//Pointer to the current RX DMA descriptor
static Xmc4800RxDmaDesc *rxCurDmaDesc;


/**
 * @brief XMC4800 Ethernet MAC driver
 **/

const NicDriver xmc4800EthDriver =
{
   NIC_TYPE_ETHERNET,
   ETH_MTU,
   xmc4800EthInit,
   xmc4800EthTick,
   xmc4800EthEnableIrq,
   xmc4800EthDisableIrq,
   xmc4800EthEventHandler,
   xmc4800EthSendPacket,
   xmc4800EthUpdateMacAddrFilter,
   xmc4800EthUpdateMacConfig,
   xmc4800EthWritePhyReg,
   xmc4800EthReadPhyReg,
   TRUE,
   TRUE,
   TRUE,
   FALSE
};


/**
 * @brief XMC4800 Ethernet MAC initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t xmc4800EthInit(NetInterface *interface)
{
   error_t error;

   //Debug message
   TRACE_INFO("Initializing XMC4800 Ethernet MAC...\r\n");

   //Save underlying network interface
   nicDriverInterface = interface;

   //Disable parity error trap
   SCU_PARITY->PETE = 0;
   //Disable unaligned access trap
   PPB->CCR &= ~PPB_CCR_UNALIGN_TRP_Msk;

   //Enable ETH0 peripheral clock
   SCU_CLK->CLKSET = SCU_CLK_CLKSET_ETH0CEN_Msk;

   //GPIO configuration
   xmc4800EthInitGpio(interface);

   //Reset ETH0 peripheral
   SCU_RESET->PRSET2 = SCU_RESET_PRSET2_ETH0RS_Msk;
   SCU_RESET->PRCLR2 = SCU_RESET_PRCLR2_ETH0RS_Msk;

   //Reset DMA controller
   ETH0->BUS_MODE |= ETH_BUS_MODE_SWR_Msk;
   //Wait for the reset to complete
   while((ETH0->BUS_MODE & ETH_BUS_MODE_SWR_Msk) != 0)
   {
   }

   //Adjust MDC clock range depending on ETH clock frequency
   ETH0->GMII_ADDRESS = ETH_GMII_ADDRESS_CR_DIV62;

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

   //Use default MAC configuration
   ETH0->MAC_CONFIGURATION = ETH_MAC_CONFIGURATION_RESERVED15_Msk |
      ETH_MAC_CONFIGURATION_DO_Msk;

   //Set the MAC address of the station
   ETH0->MAC_ADDRESS0_LOW = interface->macAddr.w[0] | (interface->macAddr.w[1] << 16);
   ETH0->MAC_ADDRESS0_HIGH = interface->macAddr.w[2];

   //The MAC supports 3 additional addresses for unicast perfect filtering
   ETH0->MAC_ADDRESS1_LOW = 0;
   ETH0->MAC_ADDRESS1_HIGH = 0;
   ETH0->MAC_ADDRESS2_LOW = 0;
   ETH0->MAC_ADDRESS2_HIGH = 0;
   ETH0->MAC_ADDRESS3_LOW = 0;
   ETH0->MAC_ADDRESS3_HIGH = 0;

   //Initialize hash table
   ETH0->HASH_TABLE_LOW = 0;
   ETH0->HASH_TABLE_HIGH = 0;

   //Configure the receive filter
   ETH0->MAC_FRAME_FILTER = ETH_MAC_FRAME_FILTER_HPF_Msk | ETH_MAC_FRAME_FILTER_HMC_Msk;
   //Disable flow control
   ETH0->FLOW_CONTROL = 0;
   //Enable store and forward mode
   ETH0->OPERATION_MODE = ETH_OPERATION_MODE_RSF_Msk | ETH_OPERATION_MODE_TSF_Msk;

   //Configure DMA bus mode
   ETH0->BUS_MODE = ETH_BUS_MODE_AAL_Msk | ETH_BUS_MODE_USP_Msk |
      ETH_BUS_MODE_RPBL_1 | ETH_BUS_MODE_PR_1_1 | ETH_BUS_MODE_PBL_1;

   //Initialize DMA descriptor lists
   xmc4800EthInitDmaDesc(interface);

   //Prevent interrupts from being generated when statistic counters reach
   //half their maximum value
   ETH0->MMC_TRANSMIT_INTERRUPT_MASK = 0xFFFFFFFF;
   ETH0->MMC_RECEIVE_INTERRUPT_MASK = 0xFFFFFFFF;
   ETH0->MMC_IPC_RECEIVE_INTERRUPT_MASK = 0xFFFFFFFF;

   //Disable MAC interrupts
   ETH0->INTERRUPT_MASK = ETH_INTERRUPT_MASK_TSIM_Msk | ETH_INTERRUPT_MASK_PMTIM_Msk;

   //Enable the desired DMA interrupts
   ETH0->INTERRUPT_ENABLE = ETH_INTERRUPT_ENABLE_NIE_Msk |
      ETH_INTERRUPT_ENABLE_RIE_Msk | ETH_INTERRUPT_ENABLE_TIE_Msk;

   //Set priority grouping (6 bits for pre-emption priority, no bits for subpriority)
   NVIC_SetPriorityGrouping(XMC4800_ETH_IRQ_PRIORITY_GROUPING);

   //Configure Ethernet interrupt priority
   NVIC_SetPriority(ETH0_0_IRQn, NVIC_EncodePriority(XMC4800_ETH_IRQ_PRIORITY_GROUPING,
      XMC4800_ETH_IRQ_GROUP_PRIORITY, XMC4800_ETH_IRQ_SUB_PRIORITY));

   //Enable MAC transmission and reception
   ETH0->MAC_CONFIGURATION |= ETH_MAC_CONFIGURATION_TE_Msk | ETH_MAC_CONFIGURATION_RE_Msk;
   //Enable DMA transmission and reception
   ETH0->OPERATION_MODE |= ETH_OPERATION_MODE_ST_Msk | ETH_OPERATION_MODE_SR_Msk;

   //Accept any packets from the upper layer
   osSetEvent(&interface->nicTxEvent);

   //Successful initialization
   return NO_ERROR;
}


//XMC4800 Relax EtherCAT Kit?
#if defined(USE_KIT_XMC4800_RELAX_ECAT)

/**
 * @brief GPIO configuration
 * @param[in] interface Underlying network interface
 **/

void xmc4800EthInitGpio(NetInterface *interface)
{
   uint32_t temp;

   //Configure ETH0.MDIO (P2.0), ETH0.RXD0A (P2.2) and ETH0.RXD1A (P2.3)
   temp = PORT2->IOCR0;
   temp &= ~(PORT2_IOCR0_PC0_Msk | PORT2_IOCR0_PC2_Msk | PORT2_IOCR0_PC3_Msk);
   temp |= (0UL << PORT2_IOCR0_PC0_Pos) | (0UL << PORT2_IOCR0_PC2_Pos) | (0UL << PORT2_IOCR0_PC3_Pos);
   PORT2->IOCR0 = temp;

   //Configure ETH0.RXERA (P2.4), ETH0.TX_EN (P2.5) and ETH0.MDC (P2.7)
   temp = PORT2->IOCR4;
   temp &= ~(PORT2_IOCR4_PC4_Msk | PORT2_IOCR4_PC5_Msk | PORT2_IOCR4_PC7_Msk);
   temp |= (0UL << PORT2_IOCR4_PC4_Pos) | (17UL << PORT2_IOCR4_PC5_Pos) | (17UL << PORT2_IOCR4_PC7_Pos);
   PORT2->IOCR4 = temp;

   //Configure ETH0.TXD0 (P2.8) and ETH0.TXD1 (P2.9)
   temp = PORT2->IOCR8;
   temp &= ~(PORT2_IOCR8_PC8_Msk | PORT2_IOCR8_PC9_Msk);
   temp |= (17UL << PORT2_IOCR8_PC8_Pos) | (17UL << PORT2_IOCR8_PC9_Pos);
   PORT2->IOCR8 = temp;

   //Configure ETH0.CLK_RMIIC (P15.8) and ETH0.CRS_DVC (P15.9)
   temp = PORT15->IOCR8;
   temp &= ~(PORT15_IOCR8_PC8_Msk | PORT15_IOCR8_PC9_Msk);
   temp |= (0UL << PORT15_IOCR8_PC8_Pos) | (0UL << PORT15_IOCR8_PC9_Pos);
   PORT15->IOCR8 = temp;

   //Assign ETH_MDIO (P2.0) to HW0
   temp = PORT2->HWSEL & ~PORT2_HWSEL_HW0_Msk;
   PORT2->HWSEL = temp | (1UL << PORT2_HWSEL_HW0_Pos);

   //Select output driver strength for ETH0.TX_EN (P2.5)
   temp = PORT2->PDR0;
   temp &= ~PORT2_PDR0_PD5_Msk;
   temp |= (0UL << PORT2_PDR0_PD5_Pos);
   PORT2->PDR0 = temp;

   //Select output driver strength for ETH0.TXD0 (P2.8) and ETH0.TXD1 (P2.9)
   temp = PORT2->PDR1;
   temp &= ~(PORT2_PDR1_PD8_Msk | PORT2_PDR1_PD9_Msk);
   temp |= (0UL << PORT2_PDR1_PD8_Pos) | (0UL << PORT2_PDR1_PD9_Pos);
   PORT2->PDR1 = temp;

   //Use ETH0.CLK_RMIIC (P15.8) and ETH0.CRS_DVC (P15.9) as digital inputs
   PORT15->PDISC &= ~(PORT15_PDISC_PDIS8_Msk | PORT15_PDISC_PDIS9_Msk);

   //Select RMII operation mode
   ETH0_CON->CON = ETH_CON_INFSEL_Msk | ETH_CON_MDIO_B | ETH_CON_RXER_A |
      ETH_CON_CRS_DV_C | ETH_CON_CLK_RMII_C | ETH_CON_RXD1_A | ETH_CON_RXD0_A;
}

#endif


/**
 * @brief Initialize DMA descriptor lists
 * @param[in] interface Underlying network interface
 **/

void xmc4800EthInitDmaDesc(NetInterface *interface)
{
   uint_t i;

   //Initialize TX DMA descriptor list
   for(i = 0; i < XMC4800_ETH_TX_BUFFER_COUNT; i++)
   {
      //Use chain structure rather than ring structure
      txDmaDesc[i].tdes0 = ETH_TDES0_IC | ETH_TDES0_TCH;
      //Initialize transmit buffer size
      txDmaDesc[i].tdes1 = 0;
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
   for(i = 0; i < XMC4800_ETH_RX_BUFFER_COUNT; i++)
   {
      //The descriptor is initially owned by the DMA
      rxDmaDesc[i].rdes0 = ETH_RDES0_OWN;
      //Use chain structure rather than ring structure
      rxDmaDesc[i].rdes1 = ETH_RDES1_RCH | (XMC4800_ETH_RX_BUFFER_SIZE & ETH_RDES1_RBS1);
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
   ETH0->TRANSMIT_DESCRIPTOR_LIST_ADDRESS = (uint32_t) txDmaDesc;
   //Start location of the RX descriptor list
   ETH0->RECEIVE_DESCRIPTOR_LIST_ADDRESS = (uint32_t) rxDmaDesc;
}


/**
 * @brief XMC4800 Ethernet MAC timer handler
 *
 * This routine is periodically called by the TCP/IP stack to handle periodic
 * operations such as polling the link state
 *
 * @param[in] interface Underlying network interface
 **/

void xmc4800EthTick(NetInterface *interface)
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

void xmc4800EthEnableIrq(NetInterface *interface)
{
   //Enable Ethernet MAC interrupts
   NVIC_EnableIRQ(ETH0_0_IRQn);

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

void xmc4800EthDisableIrq(NetInterface *interface)
{
   //Disable Ethernet MAC interrupts
   NVIC_DisableIRQ(ETH0_0_IRQn);

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
 * @brief XMC4800 Ethernet MAC interrupt service routine
 **/

void ETH0_0_IRQHandler(void)
{
   bool_t flag;
   uint32_t status;

   //Interrupt service routine prologue
   osEnterIsr();

   //This flag will be set if a higher priority task must be woken
   flag = FALSE;

   //Read DMA status register
   status = ETH0->STATUS;

   //Packet transmitted?
   if((status & ETH_STATUS_TI_Msk) != 0)
   {
      //Clear TI interrupt flag
      ETH0->STATUS = ETH_STATUS_TI_Msk;

      //Check whether the TX buffer is available for writing
      if((txCurDmaDesc->tdes0 & ETH_TDES0_OWN) == 0)
      {
         //Notify the TCP/IP stack that the transmitter is ready to send
         flag |= osSetEventFromIsr(&nicDriverInterface->nicTxEvent);
      }
   }

   //Packet received?
   if((status & ETH_STATUS_RI_Msk) != 0)
   {
      //Disable RIE interrupt
      ETH0->INTERRUPT_ENABLE &= ~ETH_INTERRUPT_ENABLE_RIE_Msk;

      //Set event flag
      nicDriverInterface->nicEvent = TRUE;
      //Notify the TCP/IP stack of the event
      flag |= osSetEventFromIsr(&netEvent);
   }

   //Clear NIS interrupt flag
   ETH0->STATUS = ETH_STATUS_NIS_Msk;

   //Interrupt service routine epilogue
   osExitIsr(flag);
}


/**
 * @brief XMC4800 Ethernet MAC event handler
 * @param[in] interface Underlying network interface
 **/

void xmc4800EthEventHandler(NetInterface *interface)
{
   error_t error;

   //Packet received?
   if((ETH0->STATUS & ETH_STATUS_RI_Msk) != 0)
   {
      //Clear interrupt flag
      ETH0->STATUS = ETH_STATUS_RI_Msk;

      //Process all pending packets
      do
      {
         //Read incoming packet
         error = xmc4800EthReceivePacket(interface);

         //No more data in the receive buffer?
      } while(error != ERROR_BUFFER_EMPTY);
   }

   //Re-enable DMA interrupts
   ETH0->INTERRUPT_ENABLE = ETH_INTERRUPT_ENABLE_NIE_Msk |
      ETH_INTERRUPT_ENABLE_RIE_Msk | ETH_INTERRUPT_ENABLE_TIE_Msk;
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

error_t xmc4800EthSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary)
{
   size_t length;

   //Retrieve the length of the packet
   length = netBufferGetLength(buffer) - offset;

   //Check the frame length
   if(length > XMC4800_ETH_TX_BUFFER_SIZE)
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
      //Report an error
      return ERROR_INVALID_LENGTH;
   }

   //Make sure the current buffer is available for writing
   if((txCurDmaDesc->tdes0 & ETH_TDES0_OWN) != 0)
   {
      return ERROR_FAILURE;
   }

   //Copy user data to the transmit buffer
   netBufferRead((uint8_t *) txCurDmaDesc->tdes2, buffer, offset, length);

   //Write the number of bytes to send
   txCurDmaDesc->tdes1 = length & ETH_TDES1_TBS1;
   //Set LS and FS flags as the data fits in a single buffer
   txCurDmaDesc->tdes0 |= ETH_TDES0_LS | ETH_TDES0_FS;
   //Give the ownership of the descriptor to the DMA
   txCurDmaDesc->tdes0 |= ETH_TDES0_OWN;

   //Clear TU flag to resume processing
   ETH0->STATUS = ETH_STATUS_TU_Msk;
   //Instruct the DMA to poll the transmit descriptor list
   ETH0->TRANSMIT_POLL_DEMAND = 0;

   //Point to the next descriptor in the list
   txCurDmaDesc = (Xmc4800TxDmaDesc *) txCurDmaDesc->tdes3;

   //Check whether the next buffer is available for writing
   if((txCurDmaDesc->tdes0 & ETH_TDES0_OWN) == 0)
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

error_t xmc4800EthReceivePacket(NetInterface *interface)
{
   error_t error;
   size_t n;
   NetRxAncillary ancillary;

   //Current buffer available for reading?
   if((rxCurDmaDesc->rdes0 & ETH_RDES0_OWN) == 0)
   {
      //FS and LS flags should be set
      if((rxCurDmaDesc->rdes0 & ETH_RDES0_FS) != 0 &&
         (rxCurDmaDesc->rdes0 & ETH_RDES0_LS) != 0)
      {
         //Make sure no error occurred
         if((rxCurDmaDesc->rdes0 & ETH_RDES0_ES) == 0)
         {
            //Retrieve the length of the frame
            n = (rxCurDmaDesc->rdes0 & ETH_RDES0_FL) >> 16;
            //Limit the number of data to read
            n = MIN(n, XMC4800_ETH_RX_BUFFER_SIZE);

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
      rxCurDmaDesc->rdes0 = ETH_RDES0_OWN;
      //Point to the next descriptor in the list
      rxCurDmaDesc = (Xmc4800RxDmaDesc *) rxCurDmaDesc->rdes3;
   }
   else
   {
      //No more data in the receive buffer
      error = ERROR_BUFFER_EMPTY;
   }

   //Clear RU flag to resume processing
   ETH0->STATUS = ETH_STATUS_RU_Msk;
   //Instruct the DMA to poll the receive descriptor list
   ETH0->RECEIVE_POLL_DEMAND = 0;

   //Return status code
   return error;
}


/**
 * @brief Configure MAC address filtering
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t xmc4800EthUpdateMacAddrFilter(NetInterface *interface)
{
   uint_t i;
   uint_t j;
   uint_t k;
   uint32_t crc;
   uint32_t hashTable[2];
   MacAddr unicastMacAddr[3];
   MacFilterEntry *entry;

   //Debug message
   TRACE_DEBUG("Updating MAC filter...\r\n");

   //Set the MAC address of the station
   ETH0->MAC_ADDRESS0_LOW = interface->macAddr.w[0] | (interface->macAddr.w[1] << 16);
   ETH0->MAC_ADDRESS0_HIGH = interface->macAddr.w[2];

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
            //Compute CRC over the current MAC address
            crc = xmc4800EthCalcCrc(&entry->addr, sizeof(MacAddr));

            //The upper 6 bits in the CRC register are used to index the
            //contents of the hash table
            k = (crc >> 26) & 0x3F;

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
      //When the AE bit is set, the entry is used for perfect filtering
      ETH0->MAC_ADDRESS1_LOW = unicastMacAddr[0].w[0] | (unicastMacAddr[0].w[1] << 16);
      ETH0->MAC_ADDRESS1_HIGH = unicastMacAddr[0].w[2] | ETH_MAC_ADDRESS1_HIGH_AE_Msk;
   }
   else
   {
      //When the AE bit is cleared, the entry is ignored
      ETH0->MAC_ADDRESS1_LOW = 0;
      ETH0->MAC_ADDRESS1_HIGH = 0;
   }

   //Configure the second unicast address filter
   if(j >= 2)
   {
      //When the AE bit is set, the entry is used for perfect filtering
      ETH0->MAC_ADDRESS2_LOW = unicastMacAddr[1].w[0] | (unicastMacAddr[1].w[1] << 16);
      ETH0->MAC_ADDRESS2_HIGH = unicastMacAddr[1].w[2] | ETH_MAC_ADDRESS2_HIGH_AE_Msk;
   }
   else
   {
      //When the AE bit is cleared, the entry is ignored
      ETH0->MAC_ADDRESS2_LOW = 0;
      ETH0->MAC_ADDRESS2_HIGH = 0;
   }

   //Configure the third unicast address filter
   if(j >= 3)
   {
      //When the AE bit is set, the entry is used for perfect filtering
      ETH0->MAC_ADDRESS3_LOW = unicastMacAddr[2].w[0] | (unicastMacAddr[2].w[1] << 16);
      ETH0->MAC_ADDRESS3_HIGH = unicastMacAddr[2].w[2] | ETH_MAC_ADDRESS3_HIGH_AE_Msk;
   }
   else
   {
      //When the AE bit is cleared, the entry is ignored
      ETH0->MAC_ADDRESS3_LOW = 0;
      ETH0->MAC_ADDRESS3_HIGH = 0;
   }

   //Configure the multicast address filter
   ETH0->HASH_TABLE_LOW = hashTable[0];
   ETH0->HASH_TABLE_HIGH = hashTable[1];

   //Debug message
   TRACE_DEBUG("  HASH_TABLE_LOW = %08" PRIX32 "\r\n", ETH0->HASH_TABLE_LOW);
   TRACE_DEBUG("  HASH_TABLE_HIGH = %08" PRIX32 "\r\n", ETH0->HASH_TABLE_HIGH);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Adjust MAC configuration parameters for proper operation
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t xmc4800EthUpdateMacConfig(NetInterface *interface)
{
   uint32_t config;

   //Read current MAC configuration
   config = ETH0->MAC_CONFIGURATION;

   //10BASE-T or 100BASE-TX operation mode?
   if(interface->linkSpeed == NIC_LINK_SPEED_100MBPS)
   {
      config |= ETH_MAC_CONFIGURATION_FES_Msk;
   }
   else
   {
      config &= ~ETH_MAC_CONFIGURATION_FES_Msk;
   }

   //Half-duplex or full-duplex mode?
   if(interface->duplexMode == NIC_FULL_DUPLEX_MODE)
   {
      config |= ETH_MAC_CONFIGURATION_DM_Msk;
   }
   else
   {
      config &= ~ETH_MAC_CONFIGURATION_DM_Msk;
   }

   //Update MAC configuration register
   ETH0->MAC_CONFIGURATION = config;

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

void xmc4800EthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data)
{
   uint32_t temp;

   //Valid opcode?
   if(opcode == SMI_OPCODE_WRITE)
   {
      //Take care not to alter MDC clock configuration
      temp = ETH0->GMII_ADDRESS & ETH_GMII_ADDRESS_CR_Msk;
      //Set up a write operation
      temp |= ETH_GMII_ADDRESS_MW_Msk | ETH_GMII_ADDRESS_MB_Msk;
      //PHY address
      temp |= (phyAddr << ETH_GMII_ADDRESS_PA_Pos) & ETH_GMII_ADDRESS_PA_Msk;
      //Register address
      temp |= (regAddr << ETH_GMII_ADDRESS_MR_Pos) & ETH_GMII_ADDRESS_MR_Msk;

      //Data to be written in the PHY register
      ETH0->GMII_DATA = data & ETH_GMII_DATA_MD_Msk;

      //Start a write operation
      ETH0->GMII_ADDRESS = temp;
      //Wait for the write to complete
      while((ETH0->GMII_ADDRESS & ETH_GMII_ADDRESS_MB_Msk) != 0)
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

uint16_t xmc4800EthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr)
{
   uint16_t data;
   uint32_t temp;

   //Valid opcode?
   if(opcode == SMI_OPCODE_READ)
   {
      //Take care not to alter MDC clock configuration
      temp = ETH0->GMII_ADDRESS & ETH_GMII_ADDRESS_CR_Msk;
      //Set up a read operation
      temp |= ETH_GMII_ADDRESS_MB_Msk;
      //PHY address
      temp |= (phyAddr << ETH_GMII_ADDRESS_PA_Pos) & ETH_GMII_ADDRESS_PA_Msk;
      //Register address
      temp |= (regAddr << ETH_GMII_ADDRESS_MR_Pos) & ETH_GMII_ADDRESS_MR_Msk;

      //Start a read operation
      ETH0->GMII_ADDRESS = temp;
      //Wait for the read to complete
      while((ETH0->GMII_ADDRESS & ETH_GMII_ADDRESS_MB_Msk) != 0)
      {
      }

      //Get register value
      data = ETH0->GMII_DATA & ETH_GMII_DATA_MD_Msk;
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

uint32_t xmc4800EthCalcCrc(const void *data, size_t length)
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
   return ~crc;
}
