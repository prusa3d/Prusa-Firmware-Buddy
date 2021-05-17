/**
 * @file lm3s_eth_driver.c
 * @brief Luminary Stellaris LM3S Ethernet controller
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

//LM3S6965 device?
#if defined(LM3S6965)
   #include "lm3s6965.h"
//LM3S9B92 device?
#elif defined(LM3S9B92)
   #include "lm3s9b92.h"
#endif

//Dependencies
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "core/net.h"
#include "drivers/mac/lm3s_eth_driver.h"
#include "debug.h"

//Underlying network interface
static NetInterface *nicDriverInterface;

//IAR EWARM compiler?
#if defined(__ICCARM__)

//Transmit buffer
#pragma data_alignment = 4
static uint8_t txBuffer[ETH_MAX_FRAME_SIZE + 2];
//Receive buffer
#pragma data_alignment = 4
static uint8_t rxBuffer[ETH_MAX_FRAME_SIZE];

//Keil MDK-ARM or GCC compiler?
#else

//Transmit buffer
static uint8_t txBuffer[ETH_MAX_FRAME_SIZE + 2] __attribute__((aligned(4)));
//Receive buffer
static uint8_t rxBuffer[ETH_MAX_FRAME_SIZE] __attribute__((aligned(4)));

#endif


/**
 * @brief Stellaris LM3S Ethernet driver
 **/

const NicDriver lm3sEthDriver =
{
   NIC_TYPE_ETHERNET,
   ETH_MTU,
   lm3sEthInit,
   lm3sEthTick,
   lm3sEthEnableIrq,
   lm3sEthDisableIrq,
   lm3sEthEventHandler,
   lm3sEthSendPacket,
   lm3sEthUpdateMacAddrFilter,
   NULL,
   NULL,
   NULL,
   TRUE,
   TRUE,
   TRUE,
   FALSE
};


/**
 * @brief Stellaris LM3S Ethernet controller initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t lm3sEthInit(NetInterface *interface)
{
   uint_t div;

   //Debug message
   TRACE_INFO("Initializing Stellaris LM3S Ethernet controller...\r\n");

   //Save underlying network interface
   nicDriverInterface = interface;

   //Enable Ethernet controller clock
   SysCtlPeripheralEnable(SYSCTL_PERIPH_ETH);
   //Reset Ethernet controller
   SysCtlPeripheralReset(SYSCTL_PERIPH_ETH);

   //GPIO configuration
   lm3sEthInitGpio(interface);

   //The MDC clock frequency cannot exceed 2.5MHz
   div = SysCtlClockGet() / (2 * 2500000) - 1;
   //Adjust MDC clock frequency
   MAC_MDV_R = div & MAC_MDV_DIV_M;

   //Reset PHY transceiver
   lm3sEthWritePhyReg(PHY_MR0, PHY_MR0_RESET);
   //Wait for the reset to complete
   while(lm3sEthReadPhyReg(PHY_MR0) & PHY_MR0_RESET)
   {
   }

   //Dump PHY registers for debugging purpose
   lm3sEthDumpPhyReg();

   //Configure LED0 and LED1
   lm3sEthWritePhyReg(PHY_MR23, PHY_MR23_LED0_RXTX | PHY_MR23_LED1_LINK);

   //Set the MAC address of the station
   MAC_IA0_R = interface->macAddr.w[0] | (interface->macAddr.w[1] << 16);
   MAC_IA1_R = interface->macAddr.w[2];

   //Enable automatic CRC generation and packet padding
   MAC_TCTL_R = MAC_TCTL_DUPLEX | MAC_TCTL_CRC | MAC_TCTL_PADEN;
   //Flush the receive FIFO and enable CRC verification
   MAC_RCTL_R = MAC_RCTL_RSTFIFO | MAC_RCTL_BADCRC;

   //Configure Ethernet interrupts
   MAC_IM_R = MAC_IM_PHYINTM | MAC_IM_TXEMPM | MAC_IM_RXINTM;
   //Configure PHY interrupts
   lm3sEthWritePhyReg(PHY_MR17, PHY_MR17_LSCHG_IE);

   //Set priority grouping (3 bits for pre-emption priority, no bits for subpriority)
   IntPriorityGroupingSet(LM3S_ETH_IRQ_PRIORITY_GROUPING);
   //Configure Ethernet interrupt priority
   IntPrioritySet(INT_ETH, LM3S_ETH_IRQ_PRIORITY);

   //Enable transmitter
   MAC_TCTL_R |= MAC_TCTL_TXEN;
   //Enable receiver
   MAC_RCTL_R |= MAC_RCTL_RXEN;

   //Accept any packets from the upper layer
   osSetEvent(&interface->nicTxEvent);

   //Successful initialization
   return NO_ERROR;
}


//EK-LM3S6965 evaluation board?
#if defined(USE_EK_LM3S6965)

/**
 * @brief GPIO configuration
 * @param[in] interface Underlying network interface
 **/

void lm3sEthInitGpio(NetInterface *interface)
{
   //Enable GPIO clock
   SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

   //Configure status LEDs
   GPIOPinTypeEthernetLED(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3);
}

#endif


/**
 * @brief Stellaris LM3S Ethernet timer handler
 *
 * This routine is periodically called by the TCP/IP stack to handle periodic
 * operations such as polling the link state
 *
 * @param[in] interface Underlying network interface
 **/

void lm3sEthTick(NetInterface *interface)
{
}


/**
 * @brief Enable interrupts
 * @param[in] interface Underlying network interface
 **/

void lm3sEthEnableIrq(NetInterface *interface)
{
   //Enable Ethernet interrupts
   IntEnable(INT_ETH);
}


/**
 * @brief Disable interrupts
 * @param[in] interface Underlying network interface
 **/

void lm3sEthDisableIrq(NetInterface *interface)
{
   //Disable Ethernet interrupts
   IntDisable(INT_ETH);
}


/**
 * @brief Stellaris LM3S Ethernet interrupt service routine
 **/

void ETH_IRQHandler(void)
{
   bool_t flag;
   uint32_t status;

   //Interrupt service routine prologue
   osEnterIsr();

   //This flag will be set if a higher priority task must be woken
   flag = FALSE;

   //Read interrupt status register
   status = MAC_RIS_R;

   //PHY interrupt?
   if((status & MAC_RIS_PHYINT) != 0)
   {
      //Disable PHYINT interrupt
      MAC_IM_R &= ~MAC_IM_PHYINTM;

      //Set event flag
      nicDriverInterface->nicEvent = TRUE;
      //Notify the TCP/IP stack of the event
      flag |= osSetEventFromIsr(&netEvent);
   }

   //Transmit FIFO empty?
   if((status & MAC_RIS_TXEMP) != 0)
   {
      //Acknowledge TXEMP interrupt
      MAC_IACK_R = MAC_IACK_TXEMP;

      //Check whether the transmit FIFO is available for writing
      if((MAC_TR_R & MAC_TR_NEWTX) == 0)
      {
         //Notify the TCP/IP stack that the transmitter is ready to send
         flag |= osSetEventFromIsr(&nicDriverInterface->nicTxEvent);
      }
   }

   //Packet received?
   if((status & MAC_RIS_RXINT) != 0)
   {
      //Disable RXINT interrupt
      MAC_IM_R &= ~MAC_IM_RXINTM;

      //Set event flag
      nicDriverInterface->nicEvent = TRUE;
      //Notify the TCP/IP stack of the event
      flag |= osSetEventFromIsr(&netEvent);
   }

   //Interrupt service routine epilogue
   osExitIsr(flag);
}


/**
 * @brief Stellaris LM3S Ethernet event handler
 * @param[in] interface Underlying network interface
 **/

void lm3sEthEventHandler(NetInterface *interface)
{
   uint32_t status;
   uint16_t value;

   //Read interrupt status register
   status = MAC_RIS_R;

   //PHY interrupt?
   if((status & MAC_RIS_PHYINT) != 0)
   {
      //Acknowledge PHYINT interrupt
      MAC_IACK_R = MAC_IACK_PHYINT;
      //Read PHY interrupt status register
      value = lm3sEthReadPhyReg(PHY_MR17);

      //Check whether the link state has changed
      if((value & PHY_MR17_LSCHG_IE) != 0)
      {
         //Read PHY status register
         value = lm3sEthReadPhyReg(PHY_MR1);

         //Check link state
         if((value & PHY_MR1_LINK) != 0)
         {
            //Read PHY diagnostic register
            value = lm3sEthReadPhyReg(PHY_MR18);

            //Get current speed
            if((value & PHY_MR18_RATE) != 0)
            {
               //100BASE-TX operation
               interface->linkSpeed = NIC_LINK_SPEED_100MBPS;
            }
            else
            {
               //10BASE-T operation
               interface->linkSpeed = NIC_LINK_SPEED_10MBPS;
            }

            //Get current duplex mode
            if((value & PHY_MR18_DPLX) != 0)
            {
               //Full-Duplex mode
               interface->duplexMode = NIC_FULL_DUPLEX_MODE;
               //Update MAC configuration
               MAC_TCTL_R |= MAC_TCTL_DUPLEX;
            }
            else
            {
               //Half-Duplex mode
               interface->duplexMode = NIC_HALF_DUPLEX_MODE;
               //Update MAC configuration
               MAC_TCTL_R &= ~MAC_TCTL_DUPLEX;
            }

            //Update link state
            interface->linkState = TRUE;
         }
         else
         {
            //Update link state
            interface->linkState = FALSE;
         }

         //Process link state change event
         nicNotifyLinkChange(interface);
      }
   }

   //Packet received?
   if((status & MAC_RIS_RXINT) != 0)
   {
      //Acknowledge RXINT interrupt
      MAC_IACK_R = MAC_IACK_RXINT;

      //Process all the pending packets
      while((MAC_NP_R & MAC_NP_NPR_M) != 0)
      {
         //Read incoming packet
         lm3sEthReceivePacket(interface);
      }
   }

   //Re-enable Ethernet interrupts
   MAC_IM_R = MAC_IM_PHYINTM | MAC_IM_TXEMPM | MAC_IM_RXINTM;
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

error_t lm3sEthSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary)
{
   size_t i;
   size_t length;
   uint32_t *p;

   //Retrieve the length of the packet
   length = netBufferGetLength(buffer) - offset;

   //Check the frame length
   if(length < sizeof(EthHeader) || length > ETH_MAX_FRAME_SIZE)
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
      //Report an error
      return ERROR_INVALID_LENGTH;
   }

   //Make sure the transmit FIFO is available for writing
   if((MAC_TR_R & MAC_TR_NEWTX) != 0)
   {
      return ERROR_FAILURE;
   }

   //Copy user data
   netBufferRead(txBuffer + 2, buffer, offset, length);

   //The packet is preceded by a 16-bit length field
   txBuffer[0] = LSB(length - sizeof(EthHeader));
   txBuffer[1] = MSB(length - sizeof(EthHeader));

   //Point to the beginning of the packet
   p = (uint32_t *) txBuffer;
   //Compute the length of the packet in 32-bit words
   length = (length + 5) / 4;

   //Copy packet to transmit FIFO
   for(i = 0; i < length; i++)
   {
      MAC_DATA_R = p[i];
   }

   //Start transmitting
   MAC_TR_R = MAC_TR_NEWTX;

   //Data successfully written
   return NO_ERROR;
}


/**
 * @brief Receive a packet
 * @return Error code
 **/

error_t lm3sEthReceivePacket(NetInterface *interface)
{
   error_t error;
   size_t i;
   size_t n;
   size_t length;
   uint32_t data;
   uint16_t *p;

   //Make sure the FIFO is not empty
   if((MAC_NP_R & MAC_NP_NPR_M) != 0)
   {
      //Read the first word
      data = MAC_DATA_R;
      //Retrieve the total length of the packet
      length = data & 0xFFFF;

      //Make sure the length field is valid
      if(length > 2)
      {
         //Point to the beginning of the buffer
         p = (uint16_t *) rxBuffer;

         //Retrieve the length of the frame
         length -= 2;
         //Limit the number of data to be read
         n = MIN(length, ETH_MAX_FRAME_SIZE);

         //Copy the first half word
         if(n > 0)
         {
            *(p++) = (uint16_t) (data >> 16);
         }

         //Copy data from receive FIFO
         for(i = 2; i < n; i += 4)
         {
            //Read a 32-bit word from the FIFO
            data = MAC_DATA_R;
            //Write the 32-bit to the receive buffer
            *(p++) = (uint16_t) data;
            *(p++) = (uint16_t) (data >> 16);
         }

         //Skip the remaining bytes
         while(i < length)
         {
            //Read a 32-bit word from the FIFO
            data = MAC_DATA_R;
            //Increment byte counter
            i += 4;
         }

         //Valid packet received
         error = NO_ERROR;
      }
      else
      {
         //Disable receiver
         MAC_RCTL_R &= ~MAC_RCTL_RXEN;
         //Flush the receive FIFO
         MAC_RCTL_R |= MAC_RCTL_RSTFIFO;
         //Re-enable receiver
         MAC_RCTL_R |= MAC_RCTL_RXEN;

         //The packet is not valid
         error = ERROR_INVALID_PACKET;
      }
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
      nicProcessPacket(interface, rxBuffer, n, &ancillary);
   }

   //Return status code
   return error;
}


/**
 * @brief Configure MAC address filtering
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t lm3sEthUpdateMacAddrFilter(NetInterface *interface)
{
   uint_t i;
   bool_t acceptMulticast;

   //Debug message
   TRACE_DEBUG("Updating MAC filter...\r\n");

   //Set the MAC address of the station
   MAC_IA0_R = interface->macAddr.w[0] | (interface->macAddr.w[1] << 16);
   MAC_IA1_R = interface->macAddr.w[2];

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
      MAC_RCTL_R |= MAC_RCTL_AMUL;
   }
   else
   {
      MAC_RCTL_R &= ~MAC_RCTL_AMUL;
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Write PHY register
 * @param[in] address PHY register address
 * @param[in] data Register value
 **/

void lm3sEthWritePhyReg(uint8_t address, uint16_t data)
{
   //Data to be written in the PHY register
   MAC_MTXD_R = data & MAC_MTXD_MDTX_M;
   //Start a write operation
   MAC_MCTL_R = (address << 3) | MAC_MCTL_WRITE | MAC_MCTL_START;

   //Wait for the write to complete
   while((MAC_MCTL_R & MAC_MCTL_START) != 0)
   {
   }
}


/**
 * @brief Read PHY register
 * @param[in] address PHY register address
 * @return Register value
 **/

uint16_t lm3sEthReadPhyReg(uint8_t address)
{
   //Start a read operation
   MAC_MCTL_R = (address << 3) | MAC_MCTL_START;

   //Wait for the read to complete
   while((MAC_MCTL_R & MAC_MCTL_START) != 0)
   {
   }

   //Return PHY register contents
   return MAC_MRXD_R & MAC_MRXD_MDRX_M;
}


/**
 * @brief Dump PHY registers for debugging purpose
 **/

void lm3sEthDumpPhyReg(void)
{
   uint8_t i;

   //Loop through PHY registers
   for(i = 0; i < 32; i++)
   {
      //Display current PHY register
      TRACE_DEBUG("%02" PRIu8 ": 0x%04" PRIX16 "\r\n", i,
         lm3sEthReadPhyReg(i));
   }

   //Terminate with a line feed
   TRACE_DEBUG("\r\n");
}
