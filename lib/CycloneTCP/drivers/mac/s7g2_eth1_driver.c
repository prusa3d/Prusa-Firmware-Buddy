/**
 * @file s7g2_eth1_driver.c
 * @brief Renesas Synergy S7G2 Ethernet MAC driver (ETHERC0 instance)
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
#include "bsp_irq_cfg.h"
#include "s7g2.h"
#include "core/net.h"
#include "drivers/mac/s7g2_eth1_driver.h"
#include "debug.h"

//Underlying network interface
static NetInterface *nicDriverInterface;

//IAR EWARM compiler?
#if defined(__ICCARM__)

//Transmit buffer
#pragma data_alignment = 32
static uint8_t txBuffer[S7G2_ETH1_TX_BUFFER_COUNT][S7G2_ETH1_TX_BUFFER_SIZE];
//Receive buffer
#pragma data_alignment = 32
static uint8_t rxBuffer[S7G2_ETH1_RX_BUFFER_COUNT][S7G2_ETH1_RX_BUFFER_SIZE];
//Transmit DMA descriptors
#pragma data_alignment = 32
static S7g2TxDmaDesc txDmaDesc[S7G2_ETH1_TX_BUFFER_COUNT];
//Receive DMA descriptors
#pragma data_alignment = 32
static S7g2RxDmaDesc rxDmaDesc[S7G2_ETH1_RX_BUFFER_COUNT];

//ARM or GCC compiler?
#else

//Transmit buffer
static uint8_t txBuffer[S7G2_ETH1_TX_BUFFER_COUNT][S7G2_ETH1_TX_BUFFER_SIZE]
   __attribute__((aligned(32)));
//Receive buffer
static uint8_t rxBuffer[S7G2_ETH1_RX_BUFFER_COUNT][S7G2_ETH1_RX_BUFFER_SIZE]
   __attribute__((aligned(32)));
//Transmit DMA descriptors
static S7g2TxDmaDesc txDmaDesc[S7G2_ETH1_TX_BUFFER_COUNT]
   __attribute__((aligned(32)));
//Receive DMA descriptors
static S7g2RxDmaDesc rxDmaDesc[S7G2_ETH1_RX_BUFFER_COUNT]
   __attribute__((aligned(32)));

#endif

//Current transmit descriptor
static uint_t txIndex;
//Current receive descriptor
static uint_t rxIndex;


/**
 * @brief S7G2 Ethernet MAC driver (ETHERC0 instance)
 **/

const NicDriver s7g2Eth1Driver =
{
   NIC_TYPE_ETHERNET,
   ETH_MTU,
   s7g2Eth1Init,
   s7g2Eth1Tick,
   s7g2Eth1EnableIrq,
   s7g2Eth1DisableIrq,
   s7g2Eth1EventHandler,
   s7g2Eth1SendPacket,
   s7g2Eth1UpdateMacAddrFilter,
   s7g2Eth1UpdateMacConfig,
   s7g2Eth1WritePhyReg,
   s7g2Eth1ReadPhyReg,
   TRUE,
   TRUE,
   TRUE,
   TRUE
};


/**
 * @brief S7G2 Ethernet MAC initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t s7g2Eth1Init(NetInterface *interface)
{
   error_t error;

   //Debug message
   TRACE_INFO("Initializing S7G2 Ethernet MAC (ETHERC0)...\r\n");

   //Save underlying network interface
   nicDriverInterface = interface;

   //Disable protection
   R_SYSTEM->PRCR = 0xA50B;
   //Cancel EDMAC0 module stop state
   R_MSTP->MSTPCRB_b.MSTPB15 = 0;
   //Enable protection
   R_SYSTEM->PRCR = 0xA500;

   //GPIO configuration
   s7g2Eth1InitGpio(interface);

   //Reset EDMAC0 module
   R_EDMAC0->EDMR_b.SWR = 1;
   sleep(10);

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

   //Initialize DMA descriptor lists
   s7g2Eth1InitDmaDesc(interface);

   //Maximum frame length that can be accepted
   R_ETHERC0->RFLR = S7G2_ETH1_RX_BUFFER_SIZE;
   //Set default inter packet gap (96-bit time)
   R_ETHERC0->IPGR = 0x14;

   //Set the upper 32 bits of the MAC address
   R_ETHERC0->MAHR = (interface->macAddr.b[0] << 24) | (interface->macAddr.b[1] << 16) |
      (interface->macAddr.b[2] << 8) | interface->macAddr.b[3];

   //Set the lower 16 bits of the MAC address
   R_ETHERC0->MALR_b.MALR = (interface->macAddr.b[4] << 8) | interface->macAddr.b[5];

   //Set descriptor length (16 bytes)
   R_EDMAC0->EDMR_b.DL = 0;
   //Select little endian mode
   R_EDMAC0->EDMR_b.DE = 1;
   //Use store and forward mode
   R_EDMAC0->TFTR_b.TFT = 0;

   //Set transmit FIFO size (2048 bytes)
   R_EDMAC0->FDR_b.TFD = 7;
   //Set receive FIFO size (2048 bytes)
   R_EDMAC0->FDR_b.RFD = 7;

   //Enable continuous reception of multiple frames
   R_EDMAC0->RMCR_b.RNR = 1;

   //Accept transmit interrupt notifications
   R_EDMAC0->TRIMD_b.TIM = 0;
   R_EDMAC0->TRIMD_b.TIS = 1;

   //Disable all EDMAC interrupts
   R_EDMAC0->EESIPR = 0;
   //Enable only the desired EDMAC interrupts
   R_EDMAC0->EESIPR_b.TWBIP = 1;
   R_EDMAC0->EESIPR_b.FRIP = 1;

   //Set priority grouping (4 bits for pre-emption priority, no bits for subpriority)
   NVIC_SetPriorityGrouping(S7G2_ETH1_IRQ_PRIORITY_GROUPING);

   //Configure EDMAC interrupt priority
   NVIC_SetPriority(ETHER_EINT0_IRQn, NVIC_EncodePriority(S7G2_ETH1_IRQ_PRIORITY_GROUPING,
      S7G2_ETH1_IRQ_GROUP_PRIORITY, S7G2_ETH1_IRQ_SUB_PRIORITY));

   //Enable transmission and reception
   R_ETHERC0->ECMR_b.TE = 1;
   R_ETHERC0->ECMR_b.RE = 1;

   //Instruct the DMA to poll the receive descriptor list
   R_EDMAC0->EDRRR_b.RR = 1;

   //Accept any packets from the upper layer
   osSetEvent(&interface->nicTxEvent);

   //Successful initialization
   return NO_ERROR;
}


//SK-S7G2 evaluation board?
#if defined(USE_SK_S7G2)

/**
 * @brief GPIO configuration
 * @param[in] interface Underlying network interface
 **/

void s7g2Eth1InitGpio(NetInterface *interface)
{
}

#endif


/**
 * @brief Initialize DMA descriptor lists
 * @param[in] interface Underlying network interface
 **/

void s7g2Eth1InitDmaDesc(NetInterface *interface)
{
   uint_t i;

   //Initialize TX descriptors
   for(i = 0; i < S7G2_ETH1_TX_BUFFER_COUNT; i++)
   {
      //The descriptor is initially owned by the application
      txDmaDesc[i].td0 = 0;
      //Transmit buffer length
      txDmaDesc[i].td1 = 0;
      //Transmit buffer address
      txDmaDesc[i].td2 = (uint32_t) txBuffer[i];
      //Clear padding field
      txDmaDesc[i].padding = 0;
   }

   //Mark the last descriptor entry with the TDLE flag
   txDmaDesc[i - 1].td0 |= EDMAC_TD0_TDLE;
   //Initialize TX descriptor index
   txIndex = 0;

   //Initialize RX descriptors
   for(i = 0; i < S7G2_ETH1_RX_BUFFER_COUNT; i++)
   {
      //The descriptor is initially owned by the DMA
      rxDmaDesc[i].rd0 = EDMAC_RD0_RACT;
      //Receive buffer length
      rxDmaDesc[i].rd1 = (S7G2_ETH1_RX_BUFFER_SIZE << 16) & EDMAC_RD1_RBL;
      //Receive buffer address
      rxDmaDesc[i].rd2 = (uint32_t) rxBuffer[i];
      //Clear padding field
      rxDmaDesc[i].padding = 0;
   }

   //Mark the last descriptor entry with the RDLE flag
   rxDmaDesc[i - 1].rd0 |= EDMAC_RD0_RDLE;
   //Initialize RX descriptor index
   rxIndex = 0;

   //Start address of the TX descriptor list
   R_EDMAC0->TDLAR = (uint32_t) txDmaDesc;
   //Start address of the RX descriptor list
   R_EDMAC0->RDLAR = (uint32_t) rxDmaDesc;
}


/**
 * @brief S7G2 Ethernet MAC timer handler
 *
 * This routine is periodically called by the TCP/IP stack to handle periodic
 * operations such as polling the link state
 *
 * @param[in] interface Underlying network interface
 **/

void s7g2Eth1Tick(NetInterface *interface)
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

void s7g2Eth1EnableIrq(NetInterface *interface)
{
   //Enable Ethernet MAC interrupts
   NVIC_EnableIRQ(ETHER_EINT0_IRQn);

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

void s7g2Eth1DisableIrq(NetInterface *interface)
{
   //Disable Ethernet MAC interrupts
   NVIC_DisableIRQ(ETHER_EINT0_IRQn);

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
 * @brief S7G2 Ethernet MAC interrupt service routine
 **/

void ETHER_EINT0_IRQHandler(void)
{
   bool_t flag;
   uint32_t status;

   //Interrupt service routine prologue
   osEnterIsr();

   //This flag will be set if a higher priority task must be woken
   flag = FALSE;

   //Read interrupt status register
   status = R_EDMAC0->EESR;

   //Packet transmitted?
   if((status & EDMAC_EESR_TWB) != 0)
   {
      //Clear TWB interrupt flag
      R_EDMAC0->EESR = EDMAC_EESR_TWB;

      //Check whether the TX buffer is available for writing
      if((txDmaDesc[txIndex].td0 & EDMAC_TD0_TACT) == 0)
      {
         //Notify the TCP/IP stack that the transmitter is ready to send
         flag |= osSetEventFromIsr(&nicDriverInterface->nicTxEvent);
      }
   }

   //Packet received?
   if((status & EDMAC_EESR_FR) != 0)
   {
      //Disable FR interrupts
      R_EDMAC0->EESIPR_b.FRIP = 0;

      //Set event flag
      nicDriverInterface->nicEvent = TRUE;
      //Notify the TCP/IP stack of the event
      flag |= osSetEventFromIsr(&netEvent);
   }

   //Clear IR flag
   R_ICU->IELSRn_b[ETHER_EINT0_IRQn].IR = 0;

   //Interrupt service routine epilogue
   osExitIsr(flag);
}


/**
 * @brief S7G2 Ethernet MAC event handler
 * @param[in] interface Underlying network interface
 **/

void s7g2Eth1EventHandler(NetInterface *interface)
{
   error_t error;

   //Packet received?
   if((R_EDMAC0->EESR & EDMAC_EESR_FR) != 0)
   {
      //Clear FR interrupt flag
      R_EDMAC0->EESR = EDMAC_EESR_FR;

      //Process all pending packets
      do
      {
         //Read incoming packet
         error = s7g2Eth1ReceivePacket(interface);

         //No more data in the receive buffer?
      } while(error != ERROR_BUFFER_EMPTY);
   }

   //Re-enable EDMAC interrupts
   R_EDMAC0->EESIPR_b.TWBIP = 1;
   R_EDMAC0->EESIPR_b.FRIP = 1;
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

error_t s7g2Eth1SendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary)
{
   //Retrieve the length of the packet
   size_t length = netBufferGetLength(buffer) - offset;

   //Check the frame length
   if(length > S7G2_ETH1_TX_BUFFER_SIZE)
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
      //Report an error
      return ERROR_INVALID_LENGTH;
   }

   //Make sure the current buffer is available for writing
   if((txDmaDesc[txIndex].td0 & EDMAC_TD0_TACT) != 0)
   {
      return ERROR_FAILURE;
   }

   //Copy user data to the transmit buffer
   netBufferRead(txBuffer[txIndex], buffer, offset, length);

   //Write the number of bytes to send
   txDmaDesc[txIndex].td1 = (length << 16) & EDMAC_TD1_TBL;

   //Check current index
   if(txIndex < (S7G2_ETH1_TX_BUFFER_COUNT - 1))
   {
      //Give the ownership of the descriptor to the DMA engine
      txDmaDesc[txIndex].td0 = EDMAC_TD0_TACT | EDMAC_TD0_TFP_SOF |
         EDMAC_TD0_TFP_EOF | EDMAC_TD0_TWBI;

      //Point to the next descriptor
      txIndex++;
   }
   else
   {
      //Give the ownership of the descriptor to the DMA engine
      txDmaDesc[txIndex].td0 = EDMAC_TD0_TACT | EDMAC_TD0_TDLE |
         EDMAC_TD0_TFP_SOF | EDMAC_TD0_TFP_EOF | EDMAC_TD0_TWBI;

      //Wrap around
      txIndex = 0;
   }

   //Instruct the DMA to poll the transmit descriptor list
   R_EDMAC0->EDTRR_b.TR = 1;

   //Check whether the next buffer is available for writing
   if((txDmaDesc[txIndex].td0 & EDMAC_TD0_TACT) == 0)
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

error_t s7g2Eth1ReceivePacket(NetInterface *interface)
{
   error_t error;
   size_t n;
   NetRxAncillary ancillary;

   //Current buffer available for reading?
   if((rxDmaDesc[rxIndex].rd0 & EDMAC_RD0_RACT) == 0)
   {
      //SOF and EOF flags should be set
      if((rxDmaDesc[rxIndex].rd0 & EDMAC_RD0_RFP_SOF) != 0 &&
         (rxDmaDesc[rxIndex].rd0 & EDMAC_RD0_RFP_EOF) != 0)
      {
         //Make sure no error occurred
         if((rxDmaDesc[rxIndex].rd0 & (EDMAC_RD0_RFS_MASK & ~EDMAC_RD0_RFS_RMAF)) == 0)
         {
            //Retrieve the length of the frame
            n = rxDmaDesc[rxIndex].rd1 & EDMAC_RD1_RFL;
            //Limit the number of data to read
            n = MIN(n, S7G2_ETH1_RX_BUFFER_SIZE);

            //Additional options can be passed to the stack along with the packet
            ancillary = NET_DEFAULT_RX_ANCILLARY;

            //Pass the packet to the upper layer
            nicProcessPacket(interface, rxBuffer[rxIndex], n, &ancillary);

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

      //Check current index
      if(rxIndex < (S7G2_ETH1_RX_BUFFER_COUNT - 1))
      {
         //Give the ownership of the descriptor back to the DMA
         rxDmaDesc[rxIndex].rd0 = EDMAC_RD0_RACT;
         //Point to the next descriptor
         rxIndex++;
      }
      else
      {
         //Give the ownership of the descriptor back to the DMA
         rxDmaDesc[rxIndex].rd0 = EDMAC_RD0_RACT | EDMAC_RD0_RDLE;
         //Wrap around
         rxIndex = 0;
      }

      //Instruct the DMA to poll the receive descriptor list
      R_EDMAC0->EDRRR_b.RR = 1;
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

error_t s7g2Eth1UpdateMacAddrFilter(NetInterface *interface)
{
   uint_t i;
   bool_t acceptMulticast;

   //Debug message
   TRACE_DEBUG("Updating MAC filter...\r\n");

   //Set the upper 32 bits of the MAC address
   R_ETHERC0->MAHR = (interface->macAddr.b[0] << 24) | (interface->macAddr.b[1] << 16) |
      (interface->macAddr.b[2] << 8) | interface->macAddr.b[3];

   //Set the lower 16 bits of the MAC address
   R_ETHERC0->MALR_b.MALR = (interface->macAddr.b[4] << 8) | interface->macAddr.b[5];

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
      R_EDMAC0->EESR_b.RMAF = 1;
   }
   else
   {
      R_EDMAC0->EESR_b.RMAF = 0;
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Adjust MAC configuration parameters for proper operation
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t s7g2Eth1UpdateMacConfig(NetInterface *interface)
{
   //10BASE-T or 100BASE-TX operation mode?
   if(interface->linkSpeed == NIC_LINK_SPEED_100MBPS)
   {
      R_ETHERC0->ECMR_b.RTM = 1;
   }
   else
   {
      R_ETHERC0->ECMR_b.RTM = 0;
   }

   //Half-duplex or full-duplex mode?
   if(interface->duplexMode == NIC_FULL_DUPLEX_MODE)
   {
      R_ETHERC0->ECMR_b.DM = 1;
   }
   else
   {
      R_ETHERC0->ECMR_b.DM = 0;
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

void s7g2Eth1WritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data)
{
   //Synchronization pattern
   s7g2Eth1WriteSmi(SMI_SYNC, 32);
   //Start of frame
   s7g2Eth1WriteSmi(SMI_START, 2);
   //Set up a write operation
   s7g2Eth1WriteSmi(opcode, 2);
   //Write PHY address
   s7g2Eth1WriteSmi(phyAddr, 5);
   //Write register address
   s7g2Eth1WriteSmi(regAddr, 5);
   //Turnaround
   s7g2Eth1WriteSmi(SMI_TA, 2);
   //Write register value
   s7g2Eth1WriteSmi(data, 16);
   //Release MDIO
   s7g2Eth1ReadSmi(1);
}


/**
 * @brief Read PHY register
 * @param[in] opcode Access type (2 bits)
 * @param[in] phyAddr PHY address (5 bits)
 * @param[in] regAddr Register address (5 bits)
 * @return Register value
 **/

uint16_t s7g2Eth1ReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr)
{
   uint16_t data;

   //Synchronization pattern
   s7g2Eth1WriteSmi(SMI_SYNC, 32);
   //Start of frame
   s7g2Eth1WriteSmi(SMI_START, 2);
   //Set up a read operation
   s7g2Eth1WriteSmi(opcode, 2);
   //Write PHY address
   s7g2Eth1WriteSmi(phyAddr, 5);
   //Write register address
   s7g2Eth1WriteSmi(regAddr, 5);
   //Turnaround to avoid contention
   s7g2Eth1ReadSmi(1);
   //Read register value
   data = s7g2Eth1ReadSmi(16);
   //Force the PHY to release the MDIO pin
   s7g2Eth1ReadSmi(1);

   //Return PHY register contents
   return data;
}


/**
 * @brief SMI write operation
 * @param[in] data Raw data to be written
 * @param[in] length Number of bits to be written
 **/

void s7g2Eth1WriteSmi(uint32_t data, uint_t length)
{
   //Skip the most significant bits since they are meaningless
   data <<= 32 - length;

   //Configure MDIO as an output
   R_ETHERC0->PIR_b.MMD = 1;

   //Write the specified number of bits
   while(length--)
   {
      //Write MDIO
      if((data & 0x80000000) != 0)
      {
         R_ETHERC0->PIR_b.MDO = 1;
      }
      else
      {
         R_ETHERC0->PIR_b.MDO = 0;
      }

      //Assert MDC
      usleep(1);
      R_ETHERC0->PIR_b.MDC = 1;
      //Deassert MDC
      usleep(1);
      R_ETHERC0->PIR_b.MDC = 0;

      //Rotate data
      data <<= 1;
   }
}


/**
 * @brief SMI read operation
 * @param[in] length Number of bits to be read
 * @return Data resulting from the MDIO read operation
 **/

uint32_t s7g2Eth1ReadSmi(uint_t length)
{
   uint32_t data = 0;

   //Configure MDIO as an input
   R_ETHERC0->PIR_b.MMD = 0;

   //Read the specified number of bits
   while(length--)
   {
      //Rotate data
      data <<= 1;

      //Assert MDC
      R_ETHERC0->PIR_b.MDC = 1;
      usleep(1);
      //Deassert MDC
      R_ETHERC0->PIR_b.MDC = 0;
      usleep(1);

      //Check MDIO state
      if(R_ETHERC0->PIR_b.MDI)
      {
         data |= 0x01;
      }
   }

   //Return the received data
   return data;
}
