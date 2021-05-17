/**
 * @file fm4_eth_driver.c
 * @brief Cypress FM4 Ethernet MAC driver
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
#include "mcu.h"
#include "core/net.h"
#include "drivers/mac/fm4_eth_driver.h"
#include "debug.h"

//Underlying network interface
static NetInterface *nicDriverInterface;

//IAR EWARM compiler?
#if defined(__ICCARM__)

//Transmit buffer
#pragma data_alignment = 4
static uint8_t txBuffer[FM4_ETH_TX_BUFFER_COUNT][FM4_ETH_TX_BUFFER_SIZE];
//Receive buffer
#pragma data_alignment = 4
static uint8_t rxBuffer[FM4_ETH_RX_BUFFER_COUNT][FM4_ETH_RX_BUFFER_SIZE];
//Transmit DMA descriptors
#pragma data_alignment = 4
static Fm4TxDmaDesc txDmaDesc[FM4_ETH_TX_BUFFER_COUNT];
//Receive DMA descriptors
#pragma data_alignment = 4
static Fm4RxDmaDesc rxDmaDesc[FM4_ETH_RX_BUFFER_COUNT];

//Keil MDK-ARM or GCC compiler?
#else

//Transmit buffer
static uint8_t txBuffer[FM4_ETH_TX_BUFFER_COUNT][FM4_ETH_TX_BUFFER_SIZE]
   __attribute__((aligned(4)));
//Receive buffer
static uint8_t rxBuffer[FM4_ETH_RX_BUFFER_COUNT][FM4_ETH_RX_BUFFER_SIZE]
   __attribute__((aligned(4)));
//Transmit DMA descriptors
static Fm4TxDmaDesc txDmaDesc[FM4_ETH_TX_BUFFER_COUNT]
   __attribute__((aligned(4)));
//Receive DMA descriptors
static Fm4RxDmaDesc rxDmaDesc[FM4_ETH_RX_BUFFER_COUNT]
   __attribute__((aligned(4)));

#endif

//Pointer to the current TX DMA descriptor
static Fm4TxDmaDesc *txCurDmaDesc;
//Pointer to the current RX DMA descriptor
static Fm4RxDmaDesc *rxCurDmaDesc;


/**
 * @brief FM4 Ethernet MAC driver
 **/

const NicDriver fm4EthDriver =
{
   NIC_TYPE_ETHERNET,
   ETH_MTU,
   fm4EthInit,
   fm4EthTick,
   fm4EthEnableIrq,
   fm4EthDisableIrq,
   fm4EthEventHandler,
   fm4EthSendPacket,
   fm4EthUpdateMacAddrFilter,
   fm4EthUpdateMacConfig,
   fm4EthWritePhyReg,
   fm4EthReadPhyReg,
   TRUE,
   TRUE,
   TRUE,
   FALSE
};


/**
 * @brief FM4 Ethernet MAC initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t fm4EthInit(NetInterface *interface)
{
   error_t error;

   //Debug message
   TRACE_INFO("Initializing FM4 Ethernet MAC...\r\n");

   //Save underlying network interface
   nicDriverInterface = interface;

   //GPIO configuration
   fm4EthInitGpio(interface);

   //Enable Ethernet MAC clock
   FM4_ETHERNET_CONTROL->ETH_CLKG_f.MACEN = 1;

   //Reset Ethernet MAC peripheral
   FM4_ETHERNET_CONTROL->ETH_MODE_f.RST0 = 1;
   FM4_ETHERNET_CONTROL->ETH_MODE_f.RST0 = 0;

   //Perform a software reset
   FM4_ETHERNET_MAC0->BMR_f.SWR = 1;
   //Wait for the reset to complete
   while(FM4_ETHERNET_MAC0->BMR_f.SWR)
   {
   }

   //Ensure that on-going AHB transactions are complete
   while(FM4_ETHERNET_MAC0->AHBSR_f.AHBS)
   {
   }

   //Adjust MDC clock range depending on HCLK frequency
   FM4_ETHERNET_MAC0->GAR_f.CR = 5;

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
   FM4_ETHERNET_MAC0->MCR = 0;
   FM4_ETHERNET_MAC0->MCR_f.PS = 1;
   FM4_ETHERNET_MAC0->MCR_f.DO = 1;

   //Set the MAC address of the station
   FM4_ETHERNET_MAC0->MAR0L = interface->macAddr.w[0] | (interface->macAddr.w[1] << 16);
   FM4_ETHERNET_MAC0->MAR0H = interface->macAddr.w[2];

   //Initialize hash table
   FM4_ETHERNET_MAC0->MHTRL = 0;
   FM4_ETHERNET_MAC0->MHTRH = 0;

   //Configure the receive filter
   FM4_ETHERNET_MAC0->MFFR = 0;
   FM4_ETHERNET_MAC0->MFFR_f.HPF = 1;
   FM4_ETHERNET_MAC0->MFFR_f.HMC = 1;

   //Disable flow control
   FM4_ETHERNET_MAC0->FCR = 0;

   //Enable store and forward mode
   FM4_ETHERNET_MAC0->OMR = 0;
   FM4_ETHERNET_MAC0->OMR_f.RSF = 1;
   FM4_ETHERNET_MAC0->OMR_f.TSF = 1;

   //Configure DMA bus mode
   FM4_ETHERNET_MAC0->BMR = 0;
   FM4_ETHERNET_MAC0->BMR_f.TXPR = 0;
   FM4_ETHERNET_MAC0->BMR_f.MB = 1;
   FM4_ETHERNET_MAC0->BMR_f.AAL = 0;
   FM4_ETHERNET_MAC0->BMR_f._8XPBL = 0;
   FM4_ETHERNET_MAC0->BMR_f.USP = 1;
   FM4_ETHERNET_MAC0->BMR_f.RPBL = 1;
   FM4_ETHERNET_MAC0->BMR_f.FB = 0;
   FM4_ETHERNET_MAC0->BMR_f.PR = 0;
   FM4_ETHERNET_MAC0->BMR_f.PBL = 1;
   FM4_ETHERNET_MAC0->BMR_f.ATDS = 1;
   FM4_ETHERNET_MAC0->BMR_f.DSL = 0;
   FM4_ETHERNET_MAC0->BMR_f.DA = 0;

   //Initialize DMA descriptor lists
   fm4EthInitDmaDesc(interface);

   //Prevent interrupts from being generated when statistic counters reach
   //half their maximum value
   FM4_ETHERNET_MAC0->MMC_INTR_MASK_TX = 0xFFFFFFFF;
   FM4_ETHERNET_MAC0->MMC_INTR_MASK_RX = 0xFFFFFFFF;
   FM4_ETHERNET_MAC0->MMC_IPC_INTR_MASK_RX = 0xFFFFFFFF;

   //Disable MAC interrupts
   bFM4_ETHERNET_MAC0_IMR_LPIIM = 1;
   bFM4_ETHERNET_MAC0_IMR_TSIM = 1;
   bFM4_ETHERNET_MAC0_IMR_PIM = 1;
   bFM4_ETHERNET_MAC0_IMR_RGIM = 1;

   //Enable the desired DMA interrupts
   bFM4_ETHERNET_MAC0_IER_TIE = 1;
   bFM4_ETHERNET_MAC0_IER_RIE = 1;
   bFM4_ETHERNET_MAC0_IER_NIE = 1;

   //Set priority grouping (4 bits for pre-emption priority, no bits for subpriority)
   NVIC_SetPriorityGrouping(FM4_ETH_IRQ_PRIORITY_GROUPING);

   //Configure Ethernet interrupt priority
   NVIC_SetPriority(ETHER0_IRQn, NVIC_EncodePriority(FM4_ETH_IRQ_PRIORITY_GROUPING,
      FM4_ETH_IRQ_GROUP_PRIORITY, FM4_ETH_IRQ_SUB_PRIORITY));

   //Enable MAC transmission and reception
   bFM4_ETHERNET_MAC0_MCR_TE = 1;
   bFM4_ETHERNET_MAC0_MCR_RE = 1;

   //Enable DMA transmission and reception
   bFM4_ETHERNET_MAC0_OMR_ST = 1;
   bFM4_ETHERNET_MAC0_OMR_SR = 1;

   //Accept any packets from the upper layer
   osSetEvent(&interface->nicTxEvent);

   //Successful initialization
   return NO_ERROR;
}


//SK-FM4-176L-S6E2CC-ETH or SK-FM4-176L-S6E2GM evaluation board?
#if defined(USE_SK_FM4_176L_S6E2CC_ETH) || defined(USE_SK_FM4_176L_S6E2GM)

/**
 * @brief GPIO configuration
 * @param[in] interface Underlying network interface
 **/

void fm4EthInitGpio(NetInterface *interface)
{
   //Select MII interface mode
   FM4_ETHERNET_CONTROL->ETH_MODE_f.IFMODE = 0;

   //Configure E_RXER (PC0)
   FM4_GPIO->PFRC_f.P0 = 1;
   //Configure E_RX03 (PC1)
   FM4_GPIO->PFRC_f.P1 = 1;
   //Configure E_RX02 (PC2)
   FM4_GPIO->PFRC_f.P2 = 1;
   //Configure E_RX01 (PC3)
   FM4_GPIO->PFRC_f.P3 = 1;
   //Configure E_RX00 (PC4)
   FM4_GPIO->PFRC_f.P4 = 1;
   //Configure E_RXDV (PC5)
   FM4_GPIO->PFRC_f.P5 = 1;
   //Configure E_MDIO (PC6)
   FM4_GPIO->PFRC_f.P6 = 1;
   //Configure E_MDC (PC7)
   FM4_GPIO->PFRC_f.P7 = 1;
   //Configure E_RXCK_REFCK (PC8)
   FM4_GPIO->PFRC_f.P8 = 1;
   //Configure E_COL (PC9)
   FM4_GPIO->PFRC_f.P9 = 1;
   //Configure E_CRS (PCA)
   FM4_GPIO->PFRC_f.PA = 1;
   //Configure E_TCK (PCC)
   FM4_GPIO->PFRC_f.PC = 1;
   //Configure E_TXER (PCD)
   FM4_GPIO->PFRC_f.PD = 1;
   //Configure E_TX03 (PCE)
   FM4_GPIO->PFRC_f.PE = 1;
   //Configure E_TX02 (PCF)
   FM4_GPIO->PFRC_f.PF = 1;
   //Configure E_TX01 (PD0)
   FM4_GPIO->PFRD_f.P0 = 1;
   //Configure E_TX00 (PD1)
   FM4_GPIO->PFRD_f.P1 = 1;
   //Configure E_TXEN (PD2)
   FM4_GPIO->PFRD_f.P2 = 1;

   //Peripheral assignment
   FM4_GPIO->EPFR14_f.E_TD0E = 1;
   FM4_GPIO->EPFR14_f.E_TD1E = 1;
   FM4_GPIO->EPFR14_f.E_TE0E = 1;
   FM4_GPIO->EPFR14_f.E_TE1E = 1;
   FM4_GPIO->EPFR14_f.E_MC0E = 1;
   FM4_GPIO->EPFR14_f.E_MC1B = 1;
   FM4_GPIO->EPFR14_f.E_MD0B = 1;
   FM4_GPIO->EPFR14_f.E_MD1B = 1;
   FM4_GPIO->EPFR14_f.E_SPLC = 1;

   //Configure PHY_RST as an output
   FM4_GPIO->PFR6_f.P5 = 0;
   FM4_GPIO->DDR6_f.P5 = 1;

   //Reset PHY transceiver
   FM4_GPIO->PDOR6_f.P5 = 0;
   sleep(10);
   FM4_GPIO->PDOR6_f.P5 = 1;
   sleep(10);
}

#endif


/**
 * @brief Initialize DMA descriptor lists
 * @param[in] interface Underlying network interface
 **/

void fm4EthInitDmaDesc(NetInterface *interface)
{
   uint_t i;

   //Initialize TX DMA descriptor list
   for(i = 0; i < FM4_ETH_TX_BUFFER_COUNT; i++)
   {
      //Use chain structure rather than ring structure
      txDmaDesc[i].tdes0 = ETH_TDES0_IC | ETH_TDES0_TCH;
      //Initialize transmit buffer size
      txDmaDesc[i].tdes1 = 0;
      //Transmit buffer address
      txDmaDesc[i].tdes2 = (uint32_t) txBuffer[i];
      //Next descriptor address
      txDmaDesc[i].tdes3 = (uint32_t) &txDmaDesc[i + 1];
      //Reserved fields
      txDmaDesc[i].tdes4 = 0;
      txDmaDesc[i].tdes5 = 0;
      //Transmit frame time stamp
      txDmaDesc[i].tdes6 = 0;
      txDmaDesc[i].tdes7 = 0;
   }

   //The last descriptor is chained to the first entry
   txDmaDesc[i - 1].tdes3 = (uint32_t) &txDmaDesc[0];
   //Point to the very first descriptor
   txCurDmaDesc = &txDmaDesc[0];

   //Initialize RX DMA descriptor list
   for(i = 0; i < FM4_ETH_RX_BUFFER_COUNT; i++)
   {
      //The descriptor is initially owned by the DMA
      rxDmaDesc[i].rdes0 = ETH_RDES0_OWN;
      //Use chain structure rather than ring structure
      rxDmaDesc[i].rdes1 = ETH_RDES1_RCH | (FM4_ETH_RX_BUFFER_SIZE & ETH_RDES1_RBS1);
      //Receive buffer address
      rxDmaDesc[i].rdes2 = (uint32_t) rxBuffer[i];
      //Next descriptor address
      rxDmaDesc[i].rdes3 = (uint32_t) &rxDmaDesc[i + 1];
      //Extended status
      rxDmaDesc[i].rdes4 = 0;
      //Reserved field
      rxDmaDesc[i].rdes5 = 0;
      //Receive frame time stamp
      rxDmaDesc[i].rdes6 = 0;
      rxDmaDesc[i].rdes7 = 0;
   }

   //The last descriptor is chained to the first entry
   rxDmaDesc[i - 1].rdes3 = (uint32_t) &rxDmaDesc[0];
   //Point to the very first descriptor
   rxCurDmaDesc = &rxDmaDesc[0];

   //Start location of the TX descriptor list
   FM4_ETHERNET_MAC0->TDLAR = (uint32_t) txDmaDesc;
   //Start location of the RX descriptor list
   FM4_ETHERNET_MAC0->RDLAR = (uint32_t) rxDmaDesc;
}


/**
 * @brief FM4 Ethernet MAC timer handler
 *
 * This routine is periodically called by the TCP/IP stack to handle periodic
 * operations such as polling the link state
 *
 * @param[in] interface Underlying network interface
 **/

void fm4EthTick(NetInterface *interface)
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

void fm4EthEnableIrq(NetInterface *interface)
{
   //Enable Ethernet MAC interrupts
   NVIC_EnableIRQ(ETHER0_IRQn);

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

void fm4EthDisableIrq(NetInterface *interface)
{
   //Disable Ethernet MAC interrupts
   NVIC_DisableIRQ(ETHER0_IRQn);

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
 * @brief FM4 Ethernet MAC interrupt service routine
 **/

void ETHER0_IRQHandler(void)
{
   bool_t flag;
   uint32_t status;

   //Interrupt service routine prologue
   osEnterIsr();

   //This flag will be set if a higher priority task must be woken
   flag = FALSE;

   //Read DMA status register
   status = FM4_ETHERNET_MAC0->SR;

   //Packet transmitted?
   if((status & ETH_SR_TI) != 0)
   {
      //Clear TI interrupt flag
      FM4_ETHERNET_MAC0->SR = ETH_SR_TI;

      //Check whether the TX buffer is available for writing
      if((txCurDmaDesc->tdes0 & ETH_TDES0_OWN) == 0)
      {
         //Notify the TCP/IP stack that the transmitter is ready to send
         flag |= osSetEventFromIsr(&nicDriverInterface->nicTxEvent);
      }
   }

   //Packet received?
   if((status & ETH_SR_RI) != 0)
   {
      //Clear RI interrupt flag
      FM4_ETHERNET_MAC0->SR = ETH_SR_RI;

      //Set event flag
      nicDriverInterface->nicEvent = TRUE;
      //Notify the TCP/IP stack of the event
      flag |= osSetEventFromIsr(&netEvent);
   }

   //Clear NIS interrupt flag
   FM4_ETHERNET_MAC0->SR = ETH_SR_NIS;

   //Interrupt service routine epilogue
   osExitIsr(flag);
}


/**
 * @brief FM4 Ethernet MAC event handler
 * @param[in] interface Underlying network interface
 **/

void fm4EthEventHandler(NetInterface *interface)
{
   error_t error;

   //Process all pending packets
   do
   {
      //Read incoming packet
      error = fm4EthReceivePacket(interface);

      //No more data in the receive buffer?
   } while(error != ERROR_BUFFER_EMPTY);
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

error_t fm4EthSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary)
{
   size_t length;

   //Retrieve the length of the packet
   length = netBufferGetLength(buffer) - offset;

   //Check the frame length
   if(length > FM4_ETH_TX_BUFFER_SIZE)
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
   FM4_ETHERNET_MAC0->SR = ETH_SR_TU;
   //Instruct the DMA to poll the transmit descriptor list
   FM4_ETHERNET_MAC0->TPDR = 0;

   //Point to the next descriptor in the list
   txCurDmaDesc = (Fm4TxDmaDesc *) txCurDmaDesc->tdes3;

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

error_t fm4EthReceivePacket(NetInterface *interface)
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
            n = MIN(n, FM4_ETH_RX_BUFFER_SIZE);

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
      rxCurDmaDesc = (Fm4RxDmaDesc *) rxCurDmaDesc->rdes3;
   }
   else
   {
      //No more data in the receive buffer
      error = ERROR_BUFFER_EMPTY;
   }

   //Clear RU flag to resume processing
   FM4_ETHERNET_MAC0->SR = ETH_SR_RU;
   //Instruct the DMA to poll the receive descriptor list
   FM4_ETHERNET_MAC0->RPDR = 0;

   //Return status code
   return error;
}


/**
 * @brief Configure MAC address filtering
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t fm4EthUpdateMacAddrFilter(NetInterface *interface)
{
   uint_t i;
   uint_t k;
   uint32_t crc;
   uint32_t hashTable[2];
   MacFilterEntry *entry;

   //Debug message
   TRACE_DEBUG("Updating MAC filter...\r\n");

   //Set the MAC address of the station
   FM4_ETHERNET_MAC0->MAR0L = interface->macAddr.w[0] | (interface->macAddr.w[1] << 16);
   FM4_ETHERNET_MAC0->MAR0H = interface->macAddr.w[2];

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
         crc = fm4EthCalcCrc(&entry->addr, sizeof(MacAddr));

         //The upper 6 bits in the CRC register are used to index the
         //contents of the hash table
         k = (crc >> 26) & 0x3F;

         //Update hash table contents
         hashTable[k / 32] |= (1 << (k % 32));
      }
   }

   //Write the hash table
   FM4_ETHERNET_MAC0->MHTRL = hashTable[0];
   FM4_ETHERNET_MAC0->MHTRH = hashTable[1];

   //Debug message
   TRACE_DEBUG("  MACHTLR = %08" PRIX32 "\r\n", FM4_ETHERNET_MAC0->MHTRL);
   TRACE_DEBUG("  MACHTHR = %08" PRIX32 "\r\n", FM4_ETHERNET_MAC0->MHTRH);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Adjust MAC configuration parameters for proper operation
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t fm4EthUpdateMacConfig(NetInterface *interface)
{
   stc_ethernet_mac_mcr_field_t config;

   //Read current MAC configuration
   config = FM4_ETHERNET_MAC0->MCR_f;

   //10BASE-T or 100BASE-TX operation mode?
   if(interface->linkSpeed == NIC_LINK_SPEED_100MBPS)
   {
      config.FES = 1;
   }
   else
   {
      config.FES = 0;
   }

   //Half-duplex or full-duplex mode?
   if(interface->duplexMode == NIC_FULL_DUPLEX_MODE)
   {
      config.DM = 1;
   }
   else
   {
      config.DM = 0;
   }

   //Update MAC configuration register
   FM4_ETHERNET_MAC0->MCR_f = config;

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

void fm4EthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data)
{
   //Valid opcode?
   if(opcode == SMI_OPCODE_WRITE)
   {
      //Set up a write operation
      FM4_ETHERNET_MAC0->GAR_f.GW = 1;
      //PHY address
      FM4_ETHERNET_MAC0->GAR_f.PA = phyAddr;
      //Register address
      FM4_ETHERNET_MAC0->GAR_f.GR = regAddr;

      //Data to be written in the PHY register
      FM4_ETHERNET_MAC0->GDR_f.GD = data;

      //Start a write operation
      FM4_ETHERNET_MAC0->GAR_f.GB = 1;
      //Wait for the write to complete
      while(FM4_ETHERNET_MAC0->GAR_f.GB)
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

uint16_t fm4EthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr)
{
   uint16_t data;

   //Valid opcode?
   if(opcode == SMI_OPCODE_READ)
   {
      //Set up a read operation
      FM4_ETHERNET_MAC0->GAR_f.GW = 0;
      //PHY address
      FM4_ETHERNET_MAC0->GAR_f.PA = phyAddr;
      //Register address
      FM4_ETHERNET_MAC0->GAR_f.GR = regAddr;

      //Start a read operation
      FM4_ETHERNET_MAC0->GAR_f.GB = 1;
      //Wait for the read to complete
      while(FM4_ETHERNET_MAC0->GAR_f.GB)
      {
      }

      //Get register value
      data = FM4_ETHERNET_MAC0->GDR_f.GD;
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

uint32_t fm4EthCalcCrc(const void *data, size_t length)
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
