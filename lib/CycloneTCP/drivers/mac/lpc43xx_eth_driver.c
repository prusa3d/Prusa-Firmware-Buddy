/**
 * @file lpc43xx_eth_driver.c
 * @brief LPC4300 Ethernet MAC driver
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
#include "lpc43xx.h"
#include "core/net.h"
#include "drivers/mac/lpc43xx_eth_driver.h"
#include "debug.h"

//Underlying network interface
static NetInterface *nicDriverInterface;

//IAR EWARM compiler?
#if defined(__ICCARM__)

//Transmit buffer
#pragma data_alignment = 4
static uint8_t txBuffer[LPC43XX_ETH_TX_BUFFER_COUNT][LPC43XX_ETH_TX_BUFFER_SIZE];
//Receive buffer
#pragma data_alignment = 4
static uint8_t rxBuffer[LPC43XX_ETH_RX_BUFFER_COUNT][LPC43XX_ETH_RX_BUFFER_SIZE];
//Transmit DMA descriptors
#pragma data_alignment = 4
static Lpc43xxTxDmaDesc txDmaDesc[LPC43XX_ETH_TX_BUFFER_COUNT];
//Receive DMA descriptors
#pragma data_alignment = 4
static Lpc43xxRxDmaDesc rxDmaDesc[LPC43XX_ETH_RX_BUFFER_COUNT];

//ARM or GCC compiler?
#else

//Transmit buffer
static uint8_t txBuffer[LPC43XX_ETH_TX_BUFFER_COUNT][LPC43XX_ETH_TX_BUFFER_SIZE]
   __attribute__((aligned(4)));
//Receive buffer
static uint8_t rxBuffer[LPC43XX_ETH_RX_BUFFER_COUNT][LPC43XX_ETH_RX_BUFFER_SIZE]
   __attribute__((aligned(4)));
//Transmit DMA descriptors
static Lpc43xxTxDmaDesc txDmaDesc[LPC43XX_ETH_TX_BUFFER_COUNT]
   __attribute__((aligned(4)));
//Receive DMA descriptors
static Lpc43xxRxDmaDesc rxDmaDesc[LPC43XX_ETH_RX_BUFFER_COUNT]
   __attribute__((aligned(4)));

#endif

//Pointer to the current TX DMA descriptor
static Lpc43xxTxDmaDesc *txCurDmaDesc;
//Pointer to the current RX DMA descriptor
static Lpc43xxRxDmaDesc *rxCurDmaDesc;


/**
 * @brief LPC43xx Ethernet MAC driver
 **/

const NicDriver lpc43xxEthDriver =
{
   NIC_TYPE_ETHERNET,
   ETH_MTU,
   lpc43xxEthInit,
   lpc43xxEthTick,
   lpc43xxEthEnableIrq,
   lpc43xxEthDisableIrq,
   lpc43xxEthEventHandler,
   lpc43xxEthSendPacket,
   lpc43xxEthUpdateMacAddrFilter,
   lpc43xxEthUpdateMacConfig,
   lpc43xxEthWritePhyReg,
   lpc43xxEthReadPhyReg,
   TRUE,
   TRUE,
   TRUE,
   FALSE
};


/**
 * @brief LPC43xx Ethernet MAC initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t lpc43xxEthInit(NetInterface *interface)
{
   error_t error;

   //Debug message
   TRACE_INFO("Initializing LPC43xx Ethernet MAC...\r\n");

   //Save underlying network interface
   nicDriverInterface = interface;

   //Enable Ethernet peripheral clock
   LPC_CCU1->CLK_M4_ETHERNET_CFG |= CCU1_CLK_M4_ETHERNET_CFG_RUN_Msk;
   //Wait for completion
   while((LPC_CCU1->CLK_M4_ETHERNET_STAT & CCU1_CLK_M4_ETHERNET_STAT_RUN_Msk) == 0)
   {
   }

   //Reset DMA
   LPC_RGU->RESET_EXT_STAT19 |= RGU_RESET_EXT_STAT19_MASTER_RESET_Msk;
   LPC_RGU->RESET_EXT_STAT19 &= ~RGU_RESET_EXT_STAT19_MASTER_RESET_Msk;

   //Reset Ethernet peripheral
   LPC_RGU->RESET_EXT_STAT22 |= RGU_RESET_EXT_STAT22_MASTER_RESET_Msk;
   LPC_RGU->RESET_EXT_STAT22 &= ~RGU_RESET_EXT_STAT22_MASTER_RESET_Msk;

   //GPIO configuration
   lpc43xxEthInitGpio(interface);

   //Reset Ethernet peripheral
   LPC_RGU->RESET_CTRL0 = RGU_RESET_CTRL0_ETHERNET_RST_Msk;
   //Wait for the reset to complete
   while((LPC_RGU->RESET_ACTIVE_STATUS0 & RGU_RESET_ACTIVE_STATUS0_ETHERNET_RST_Msk) == 0)
   {
   }

   //Perform a software reset
   LPC_ETHERNET->DMA_BUS_MODE |= ETHERNET_DMA_BUS_MODE_SWR_Msk;
   //Wait for the reset to complete
   while((LPC_ETHERNET->DMA_BUS_MODE & ETHERNET_DMA_BUS_MODE_SWR_Msk) != 0)
   {
   }

   //Adjust MDC clock range
   LPC_ETHERNET->MAC_MII_ADDR = ETHERNET_MAC_MII_ADDR_CR_DIV62;

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
   LPC_ETHERNET->MAC_CONFIG = ETHERNET_MAC_CONFIG_DO_Msk;

   //Set the MAC address of the station
   LPC_ETHERNET->MAC_ADDR0_LOW = interface->macAddr.w[0] | (interface->macAddr.w[1] << 16);
   LPC_ETHERNET->MAC_ADDR0_HIGH = interface->macAddr.w[2];

   //Initialize hash table
   LPC_ETHERNET->MAC_HASHTABLE_LOW = 0;
   LPC_ETHERNET->MAC_HASHTABLE_HIGH = 0;

   //Configure the receive filter
   LPC_ETHERNET->MAC_FRAME_FILTER = ETHERNET_MAC_FRAME_FILTER_HPF_Msk |
      ETHERNET_MAC_FRAME_FILTER_HMC_Msk;

   //Disable flow control
   LPC_ETHERNET->MAC_FLOW_CTRL = 0;
   //Set the threshold level of the transmit and receive FIFOs
   LPC_ETHERNET->DMA_OP_MODE = ETHERNET_DMA_OP_MODE_TTC_64 | ETHERNET_DMA_OP_MODE_RTC_32;

   //Configure DMA bus mode
   LPC_ETHERNET->DMA_BUS_MODE = ETHERNET_DMA_BUS_MODE_AAL_Msk | ETHERNET_DMA_BUS_MODE_USP_Msk |
      ETHERNET_DMA_BUS_MODE_RPBL_1 | ETHERNET_DMA_BUS_MODE_PR_1_1 |
      ETHERNET_DMA_BUS_MODE_PBL_1 | ETHERNET_DMA_BUS_MODE_ATDS_Msk;

   //Initialize DMA descriptor lists
   lpc43xxEthInitDmaDesc(interface);

   //Disable MAC interrupts
   LPC_ETHERNET->MAC_INTR_MASK = ETHERNET_MAC_INTR_MASK_TSIM_Msk |
      ETHERNET_MAC_INTR_MASK_PMTIM_Msk;

   //Enable the desired DMA interrupts
   LPC_ETHERNET->DMA_INT_EN = ETHERNET_DMA_INT_EN_NIE_Msk |
      ETHERNET_DMA_INT_EN_AIE_Msk | ETHERNET_DMA_INT_EN_RIE_Msk |
      ETHERNET_DMA_INT_EN_OVE_Msk | ETHERNET_DMA_INT_EN_TIE_Msk |
      ETHERNET_DMA_INT_EN_UNE_Msk;

   //Set priority grouping (3 bits for pre-emption priority, no bits for subpriority)
   NVIC_SetPriorityGrouping(LPC43XX_ETH_IRQ_PRIORITY_GROUPING);

   //Configure Ethernet interrupt priority
   NVIC_SetPriority(ETHERNET_IRQn, NVIC_EncodePriority(LPC43XX_ETH_IRQ_PRIORITY_GROUPING,
      LPC43XX_ETH_IRQ_GROUP_PRIORITY, LPC43XX_ETH_IRQ_SUB_PRIORITY));

   //Enable MAC transmission and reception
   LPC_ETHERNET->MAC_CONFIG |= ETHERNET_MAC_CONFIG_TE_Msk | ETHERNET_MAC_CONFIG_RE_Msk;
   //Enable DMA transmission and reception
   LPC_ETHERNET->DMA_OP_MODE |= ETHERNET_DMA_OP_MODE_ST_Msk | ETHERNET_DMA_OP_MODE_SR_Msk;

   //Accept any packets from the upper layer
   osSetEvent(&interface->nicTxEvent);

   //Successful initialization
   return NO_ERROR;
}


//LPC4330-Xplorer or LPCXpresso4337 evaluation board?
#if defined(USE_LPC4330_XPLORER) || defined(USE_LPCXPRESSO_4337)

/**
 * @brief GPIO configuration
 * @param[in] interface Underlying network interface
 **/

void lpc43xxEthInitGpio(NetInterface *interface)
{
   //Enable GPIO peripheral clock
   LPC_CCU1->CLK_M4_GPIO_CFG |= CCU1_CLK_M4_GPIO_CFG_RUN_Msk;
   //Wait for completion
   while((LPC_CCU1->CLK_M4_GPIO_STAT & CCU1_CLK_M4_GPIO_STAT_RUN_Msk) == 0)
   {
   }

   //Select RMII operation mode
   LPC_CREG->CREG6 &= ~CREG_CREG6_ETHMODE_Msk;
   LPC_CREG->CREG6 |= CREG6_ETHMODE_RMII;

   //Configure P0.0 (ENET_RXD1)
   LPC_SCU->SFSP0_0 = SCU_SFSP0_0_EZI_Msk | SCU_SFSP0_0_EHS_Msk | (2 & SCU_SFSP0_0_MODE_Msk);
   //Configure P0.1 (ENET_TX_EN)
   LPC_SCU->SFSP0_1 = SCU_SFSP0_1_EHS_Msk | (6 & SCU_SFSP0_1_MODE_Msk);

   //Configure P1.15 (ENET_RXD0)
   LPC_SCU->SFSP1_15 = SCU_SFSP1_15_EZI_Msk | SCU_SFSP1_15_EHS_Msk | (3 & SCU_SFSP1_15_MODE_Msk);
   //Configure P1.16 (ENET_RX_DV)
   LPC_SCU->SFSP1_16 = SCU_SFSP1_16_EZI_Msk | SCU_SFSP1_16_EHS_Msk | (7 & SCU_SFSP1_16_MODE_Msk);
   //Configure P1.17 (ENET_MDIO)
   LPC_SCU->SFSP1_17 = SCU_SFSP1_17_EZI_Msk | (3 & SCU_SFSP1_17_MODE_Msk);
   //Configure P1.18 (ENET_TXD0)
   LPC_SCU->SFSP1_18 = SCU_SFSP1_18_EHS_Msk | (3 & SCU_SFSP1_18_MODE_Msk);
   //Configure P1.19 (ENET_REF_CLK)
   LPC_SCU->SFSP1_19 = SCU_SFSP1_19_EZI_Msk | SCU_SFSP1_19_EHS_Msk | (0 & SCU_SFSP1_19_MODE_Msk);
   //Configure P1.20 (ENET_TXD1)
   LPC_SCU->SFSP1_20 = SCU_SFSP1_20_EHS_Msk | (3 & SCU_SFSP1_20_MODE_Msk);

   //Configure P2.0 (ENET_MDC)
   LPC_SCU->SFSP2_0 = (7 & SCU_SFSP2_0_MODE_Msk);
}

#endif


/**
 * @brief Initialize DMA descriptor lists
 * @param[in] interface Underlying network interface
 **/

void lpc43xxEthInitDmaDesc(NetInterface *interface)
{
   uint_t i;

   //Initialize TX DMA descriptor list
   for(i = 0; i < LPC43XX_ETH_TX_BUFFER_COUNT; i++)
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
   for(i = 0; i < LPC43XX_ETH_RX_BUFFER_COUNT; i++)
   {
      //The descriptor is initially owned by the DMA
      rxDmaDesc[i].rdes0 = ETH_RDES0_OWN;
      //Use chain structure rather than ring structure
      rxDmaDesc[i].rdes1 = ETH_RDES1_RCH | (LPC43XX_ETH_RX_BUFFER_SIZE & ETH_RDES1_RBS1);
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
   LPC_ETHERNET->DMA_TRANS_DES_ADDR = (uint32_t) txDmaDesc;
   //Start location of the RX descriptor list
   LPC_ETHERNET->DMA_REC_DES_ADDR = (uint32_t) rxDmaDesc;
}


/**
 * @brief LPC43xx Ethernet MAC timer handler
 *
 * This routine is periodically called by the TCP/IP stack to handle periodic
 * operations such as polling the link state
 *
 * @param[in] interface Underlying network interface
 **/

void lpc43xxEthTick(NetInterface *interface)
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

void lpc43xxEthEnableIrq(NetInterface *interface)
{
   //Enable Ethernet MAC interrupts
   NVIC_EnableIRQ(ETHERNET_IRQn);

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

void lpc43xxEthDisableIrq(NetInterface *interface)
{
   //Disable Ethernet MAC interrupts
   NVIC_DisableIRQ(ETHERNET_IRQn);

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
 * @brief LPC43xx Ethernet MAC interrupt service routine
 **/

void ETHERNET_IRQHandler(void)
{
   bool_t flag;
   uint32_t status;

   //Interrupt service routine prologue
   osEnterIsr();

   //This flag will be set if a higher priority task must be woken
   flag = FALSE;

   //Read DMA status register
   status = LPC_ETHERNET->DMA_STAT;

   //Packet transmitted?
   if((status & (ETHERNET_DMA_STAT_TI_Msk | ETHERNET_DMA_STAT_UNF_Msk)) != 0)
   {
      //Clear TI and UNF interrupt flags
      LPC_ETHERNET->DMA_STAT = ETHERNET_DMA_STAT_TI_Msk | ETHERNET_DMA_STAT_UNF_Msk;

      //Check whether the TX buffer is available for writing
      if((txCurDmaDesc->tdes0 & ETH_TDES0_OWN) == 0)
      {
         //Notify the TCP/IP stack that the transmitter is ready to send
         flag |= osSetEventFromIsr(&nicDriverInterface->nicTxEvent);
      }
   }

   //Packet received?
   if((status & (ETHERNET_DMA_STAT_RI_Msk | ETHERNET_DMA_STAT_OVF_Msk)) != 0)
   {
      //Disable RIE and OVE interrupts
      LPC_ETHERNET->DMA_INT_EN &= ~(ETHERNET_DMA_INT_EN_RIE_Msk |
         ETHERNET_DMA_INT_EN_OVE_Msk);

      //Set event flag
      nicDriverInterface->nicEvent = TRUE;
      //Notify the TCP/IP stack of the event
      flag |= osSetEventFromIsr(&netEvent);
   }

   //Clear NIS and AIS interrupt flags
   LPC_ETHERNET->DMA_STAT = ETHERNET_DMA_STAT_NIS_Msk | ETHERNET_DMA_STAT_AIE_Msk;

   //Interrupt service routine epilogue
   osExitIsr(flag);
}


/**
 * @brief LPC43xx Ethernet MAC event handler
 * @param[in] interface Underlying network interface
 **/

void lpc43xxEthEventHandler(NetInterface *interface)
{
   error_t error;

   //Packet received?
   if((LPC_ETHERNET->DMA_STAT & (ETHERNET_DMA_STAT_RI_Msk | ETHERNET_DMA_STAT_OVF_Msk)) != 0)
   {
      //Clear RI and OVF interrupt flags
      LPC_ETHERNET->DMA_STAT = ETHERNET_DMA_STAT_RI_Msk | ETHERNET_DMA_STAT_OVF_Msk;

      //Process all pending packets
      do
      {
         //Read incoming packet
         error = lpc43xxEthReceivePacket(interface);

         //No more data in the receive buffer?
      } while(error != ERROR_BUFFER_EMPTY);
   }

   //Re-enable DMA interrupts
   LPC_ETHERNET->DMA_INT_EN = ETHERNET_DMA_INT_EN_NIE_Msk |
      ETHERNET_DMA_INT_EN_AIE_Msk | ETHERNET_DMA_INT_EN_RIE_Msk |
      ETHERNET_DMA_INT_EN_OVE_Msk | ETHERNET_DMA_INT_EN_TIE_Msk |
      ETHERNET_DMA_INT_EN_UNE_Msk;
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

error_t lpc43xxEthSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary)
{
   size_t length;

   //Retrieve the length of the packet
   length = netBufferGetLength(buffer) - offset;

   //Check the frame length
   if(length > LPC43XX_ETH_TX_BUFFER_SIZE)
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
   LPC_ETHERNET->DMA_STAT = ETHERNET_DMA_STAT_TU_Msk;
   //Instruct the DMA to poll the transmit descriptor list
   LPC_ETHERNET->DMA_TRANS_POLL_DEMAND = 0;

   //Point to the next descriptor in the list
   txCurDmaDesc = (Lpc43xxTxDmaDesc *) txCurDmaDesc->tdes3;

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

error_t lpc43xxEthReceivePacket(NetInterface *interface)
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
            n = MIN(n, LPC43XX_ETH_RX_BUFFER_SIZE);

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
      rxCurDmaDesc = (Lpc43xxRxDmaDesc *) rxCurDmaDesc->rdes3;
   }
   else
   {
      //No more data in the receive buffer
      error = ERROR_BUFFER_EMPTY;
   }

   //Clear RU flag to resume processing
   LPC_ETHERNET->DMA_STAT = ETHERNET_DMA_STAT_RU_Msk;
   //Instruct the DMA to poll the receive descriptor list
   LPC_ETHERNET->DMA_REC_POLL_DEMAND = 0;

   //Return status code
   return error;
}


/**
 * @brief Configure MAC address filtering
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t lpc43xxEthUpdateMacAddrFilter(NetInterface *interface)
{
   uint_t i;
   uint_t k;
   uint32_t crc;
   uint32_t hashTable[2];
   MacFilterEntry *entry;

   //Debug message
   TRACE_DEBUG("Updating MAC filter...\r\n");

   //Set the MAC address of the station
   LPC_ETHERNET->MAC_ADDR0_LOW = interface->macAddr.w[0] | (interface->macAddr.w[1] << 16);
   LPC_ETHERNET->MAC_ADDR0_HIGH = interface->macAddr.w[2];

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
         crc = lpc43xxEthCalcCrc(&entry->addr, sizeof(MacAddr));

         //The upper 6 bits in the CRC register are used to index the
         //contents of the hash table
         k = (crc >> 26) & 0x3F;

         //Update hash table contents
         hashTable[k / 32] |= (1 << (k % 32));
      }
   }

   //Write the hash table
   LPC_ETHERNET->MAC_HASHTABLE_LOW = hashTable[0];
   LPC_ETHERNET->MAC_HASHTABLE_HIGH = hashTable[1];

   //Debug message
   TRACE_DEBUG("  MAC_HASHTABLE_LOW = %08" PRIX32 "\r\n", LPC_ETHERNET->MAC_HASHTABLE_LOW);
   TRACE_DEBUG("  MAC_HASHTABLE_HIGH = %08" PRIX32 "\r\n", LPC_ETHERNET->MAC_HASHTABLE_HIGH);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Adjust MAC configuration parameters for proper operation
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t lpc43xxEthUpdateMacConfig(NetInterface *interface)
{
   uint32_t config;

   //Read current MAC configuration
   config = LPC_ETHERNET->MAC_CONFIG;

   //10BASE-T or 100BASE-TX operation mode?
   if(interface->linkSpeed == NIC_LINK_SPEED_100MBPS)
   {
      config |= ETHERNET_MAC_CONFIG_FES_Msk;
   }
   else
   {
      config &= ~ETHERNET_MAC_CONFIG_FES_Msk;
   }

   //Half-duplex or full-duplex mode?
   if(interface->duplexMode == NIC_FULL_DUPLEX_MODE)
   {
      config |= ETHERNET_MAC_CONFIG_DM_Msk;
   }
   else
   {
      config &= ~ETHERNET_MAC_CONFIG_DM_Msk;
   }

   //Update MAC configuration register
   LPC_ETHERNET->MAC_CONFIG = config;

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

void lpc43xxEthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data)
{
   uint32_t temp;

   //Valid opcode?
   if(opcode == SMI_OPCODE_WRITE)
   {
      //Take care not to alter MDC clock configuration
      temp = LPC_ETHERNET->MAC_MII_ADDR & ETHERNET_MAC_MII_ADDR_CR_Msk;
      //Set up a write operation
      temp |= ETHERNET_MAC_MII_ADDR_W_Msk | ETHERNET_MAC_MII_ADDR_GB_Msk;
      //PHY address
      temp |= (phyAddr << ETHERNET_MAC_MII_ADDR_PA_Pos) & ETHERNET_MAC_MII_ADDR_PA_Msk;
      //Register address
      temp |= (regAddr << ETHERNET_MAC_MII_ADDR_GR_Pos) & ETHERNET_MAC_MII_ADDR_GR_Msk;

      //Data to be written in the PHY register
      LPC_ETHERNET->MAC_MII_DATA = data & ETHERNET_MAC_MII_DATA_GD_Msk;

      //Start a write operation
      LPC_ETHERNET->MAC_MII_ADDR = temp;
      //Wait for the write to complete
      while((LPC_ETHERNET->MAC_MII_ADDR & ETHERNET_MAC_MII_ADDR_GB_Msk) != 0)
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

uint16_t lpc43xxEthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr)
{
   uint16_t data;
   uint32_t temp;

   //Valid opcode?
   if(opcode == SMI_OPCODE_READ)
   {
      //Take care not to alter MDC clock configuration
      temp = LPC_ETHERNET->MAC_MII_ADDR & ETHERNET_MAC_MII_ADDR_CR_Msk;
      //Set up a read operation
      temp |= ETHERNET_MAC_MII_ADDR_GB_Msk;
      //PHY address
      temp |= (phyAddr << ETHERNET_MAC_MII_ADDR_PA_Pos) & ETHERNET_MAC_MII_ADDR_PA_Msk;
      //Register address
      temp |= (regAddr << ETHERNET_MAC_MII_ADDR_GR_Pos) & ETHERNET_MAC_MII_ADDR_GR_Msk;

      //Start a read operation
      LPC_ETHERNET->MAC_MII_ADDR = temp;
      //Wait for the read to complete
      while((LPC_ETHERNET->MAC_MII_ADDR & ETHERNET_MAC_MII_ADDR_GB_Msk) != 0)
      {
      }

      //Get register value
      data = LPC_ETHERNET->MAC_MII_DATA & ETHERNET_MAC_MII_DATA_GD_Msk;
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

uint32_t lpc43xxEthCalcCrc(const void *data, size_t length)
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
