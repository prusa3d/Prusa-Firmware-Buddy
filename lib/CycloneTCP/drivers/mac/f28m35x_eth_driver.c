/**
 * @file f28m35x_eth_driver.c
 * @brief F28M35x Ethernet MAC driver
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
#include "inc/hw_ethernet.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/sysctl.h"
#include "core/net.h"
#include "drivers/mac/f28m35x_eth_driver.h"
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

//GCC compiler?
#else

//Transmit buffer
static uint8_t txBuffer[ETH_MAX_FRAME_SIZE + 2] __attribute__((aligned(4)));
//Receive buffer
static uint8_t rxBuffer[ETH_MAX_FRAME_SIZE] __attribute__((aligned(4)));

#endif


/**
 * @brief F28M35x Ethernet MAC driver
 **/

const NicDriver f28m35xEthDriver =
{
   NIC_TYPE_ETHERNET,
   ETH_MTU,
   f28m35xEthInit,
   f28m35xEthTick,
   f28m35xEthEnableIrq,
   f28m35xEthDisableIrq,
   f28m35xEthEventHandler,
   f28m35xEthSendPacket,
   f28m35xEthUpdateMacAddrFilter,
   f28m35xEthUpdateMacConfig,
   f28m35xEthWritePhyReg,
   f28m35xEthReadPhyReg,
   TRUE,
   TRUE,
   TRUE,
   FALSE
};


/**
 * @brief F28M35x Ethernet MAC driver initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t f28m35xEthInit(NetInterface *interface)
{
   error_t error;
   uint_t div;
#ifdef ti_sysbios_BIOS___VERS
   Hwi_Params hwiParams;
#endif

   //Debug message
   TRACE_INFO("Initializing F28M35x Ethernet MAC driver...\r\n");

   //Save underlying network interface
   nicDriverInterface = interface;

   //Enable Ethernet controller clock
   SysCtlPeripheralEnable(SYSCTL_PERIPH_ETH);
   //Reset Ethernet controller
   SysCtlPeripheralReset(SYSCTL_PERIPH_ETH);

   //GPIO configuration
   f28m35xEthInitGpio(interface);

   //The MDC clock frequency cannot exceed 2.5MHz
   div = SysCtlClockGet(20000000) / (2 * 2500000) - 1;
   //Adjust MDC clock frequency
   MAC_MDV_R = div & MAC_MDV_DIV_M;

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
   MAC_IA0_R = interface->macAddr.w[0] | (interface->macAddr.w[1] << 16);
   MAC_IA1_R = interface->macAddr.w[2];

   //Enable automatic CRC generation and packet padding
   MAC_TCTL_R = MAC_TCTL_DUPLEX | MAC_TCTL_CRC | MAC_TCTL_PADEN;
   //Flush the receive FIFO and enable CRC verification
   MAC_RCTL_R = MAC_RCTL_RSTFIFO | MAC_RCTL_BADCRC;

   //Configure Ethernet interrupts
   MAC_IM_R = MAC_IM_TXEMPM | MAC_IM_RXINTM;

#ifdef ti_sysbios_BIOS___VERS
   //Configure Ethernet interrupt
   Hwi_Params_init(&hwiParams);
   hwiParams.enableInt = FALSE;
   hwiParams.priority = F28M35X_ETH_IRQ_PRIORITY;

   //Register interrupt handler
   Hwi_create(INT_ETH, (Hwi_FuncPtr) f28m35xEthIrqHandler, &hwiParams, NULL);
#else
   //Register interrupt handler
   IntRegister(INT_ETH, f28m35xEthIrqHandler);

   //Set priority grouping (3 bits for pre-emption priority, no bits for subpriority)
   IntPriorityGroupingSet(F28M35X_ETH_IRQ_PRIORITY_GROUPING);
   //Configure Ethernet interrupt priority
   IntPrioritySet(INT_ETH, F28M35X_ETH_IRQ_PRIORITY);
#endif

   //Enable transmitter
   MAC_TCTL_R |= MAC_TCTL_TXEN;
   //Enable receiver
   MAC_RCTL_R |= MAC_RCTL_RXEN;

   //Accept any packets from the upper layer
   osSetEvent(&interface->nicTxEvent);

   //Successful initialization
   return NO_ERROR;
}


//TMDXCNCDH52C1 evaluation board?
#if defined(USE_TMDXCNCDH52C1)

/**
 * @brief GPIO configuration
 * @param[in] interface Underlying network interface
 **/

void f28m35xEthInitGpio(NetInterface *interface)
{
   //Enable GPIO clocks
   SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
   SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
   SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
   SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
   SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOH);
   SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);

   //Configure MII_TXD3 (PC4)
   GPIODirModeSet(GPIO_PORTC_BASE, GPIO_PIN_4, GPIO_DIR_MODE_HW);
   GPIOPadConfigSet(GPIO_PORTC_BASE, GPIO_PIN_4, GPIO_PIN_TYPE_STD);
   GPIOPinConfigure(GPIO_PC4_MIITXD3);

   //Configure MII_MDIO (PE6)
   GPIODirModeSet(GPIO_PORTE_BASE, GPIO_PIN_6, GPIO_DIR_MODE_HW);
   GPIOPadConfigSet(GPIO_PORTE_BASE, GPIO_PIN_6, GPIO_PIN_TYPE_STD);
   GPIOPinConfigure(GPIO_PE6_MIIMDIO);

   //Configure MII_RXD3 (PF5)
   GPIODirModeSet(GPIO_PORTF_BASE, GPIO_PIN_5, GPIO_DIR_MODE_HW);
   GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_5, GPIO_PIN_TYPE_STD);
   GPIOPinConfigure(GPIO_PF5_MIIRXD3);

   //Configure MII_RXD2 (PG0)
   GPIODirModeSet(GPIO_PORTG_BASE, GPIO_PIN_0, GPIO_DIR_MODE_HW);
   GPIOPadConfigSet(GPIO_PORTG_BASE, GPIO_PIN_0, GPIO_PIN_TYPE_STD);
   GPIOPinConfigure(GPIO_PG0_MIIRXD2);

   //Configure MII_RXD1 (PG1)
   GPIODirModeSet(GPIO_PORTG_BASE, GPIO_PIN_1, GPIO_DIR_MODE_HW);
   GPIOPadConfigSet(GPIO_PORTG_BASE, GPIO_PIN_1, GPIO_PIN_TYPE_STD);
   GPIOPinConfigure(GPIO_PG1_MIIRXD1);

   //Configure MII_RXDV (PG3)
   GPIODirModeSet(GPIO_PORTG_BASE, GPIO_PIN_3, GPIO_DIR_MODE_HW);
   GPIOPadConfigSet(GPIO_PORTG_BASE, GPIO_PIN_3, GPIO_PIN_TYPE_STD);
   GPIOPinConfigure(GPIO_PG3_MIIRXDV);

   //Configure MII_TXER (PG7)
   GPIODirModeSet(GPIO_PORTG_BASE, GPIO_PIN_7, GPIO_DIR_MODE_HW);
   GPIOPadConfigSet(GPIO_PORTG_BASE, GPIO_PIN_7, GPIO_PIN_TYPE_STD);
   GPIOPinConfigure(GPIO_PG7_MIITXER);

   //Configure MII_RXD0 (PH1)
   GPIODirModeSet(GPIO_PORTH_BASE, GPIO_PIN_1, GPIO_DIR_MODE_HW);
   GPIOPadConfigSet(GPIO_PORTH_BASE, GPIO_PIN_1, GPIO_PIN_TYPE_STD);
   GPIOPinConfigure(GPIO_PH1_MIIRXD0);

   //Configure MII_TXD2 (PH3)
   GPIODirModeSet(GPIO_PORTH_BASE, GPIO_PIN_3, GPIO_DIR_MODE_HW);
   GPIOPadConfigSet(GPIO_PORTH_BASE, GPIO_PIN_3, GPIO_PIN_TYPE_STD);
   GPIOPinConfigure(GPIO_PH3_MIITXD2);

   //Configure MII_TXD1 (PH4)
   GPIODirModeSet(GPIO_PORTH_BASE, GPIO_PIN_4, GPIO_DIR_MODE_HW);
   GPIOPadConfigSet(GPIO_PORTH_BASE, GPIO_PIN_4, GPIO_PIN_TYPE_STD);
   GPIOPinConfigure(GPIO_PH4_MIITXD1);

   //Configure MII_TXD0 (PH5)
   GPIODirModeSet(GPIO_PORTH_BASE, GPIO_PIN_5, GPIO_DIR_MODE_HW);
   GPIOPadConfigSet(GPIO_PORTH_BASE, GPIO_PIN_5, GPIO_PIN_TYPE_STD);
   GPIOPinConfigure(GPIO_PH5_MIITXD0);

   //Configure MII_TXEN (PH6)
   GPIODirModeSet(GPIO_PORTH_BASE, GPIO_PIN_6, GPIO_DIR_MODE_HW);
   GPIOPadConfigSet(GPIO_PORTH_BASE, GPIO_PIN_6, GPIO_PIN_TYPE_STD);
   GPIOPinConfigure(GPIO_PH6_MIITXEN);

   //Configure MII_TXCK (PH7)
   GPIODirModeSet(GPIO_PORTH_BASE, GPIO_PIN_7, GPIO_DIR_MODE_HW);
   GPIOPadConfigSet(GPIO_PORTH_BASE, GPIO_PIN_7, GPIO_PIN_TYPE_STD);
   GPIOPinConfigure(GPIO_PH7_MIITXCK);

   //Configure MII_RXER (PJ0)
   GPIODirModeSet(GPIO_PORTJ_BASE, GPIO_PIN_0, GPIO_DIR_MODE_HW);
   GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_0, GPIO_PIN_TYPE_STD);
   GPIOPinConfigure(GPIO_PJ0_MIIRXER);

   //Configure MII_RXCK (PJ2)
   GPIODirModeSet(GPIO_PORTJ_BASE, GPIO_PIN_2, GPIO_DIR_MODE_HW);
   GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_2, GPIO_PIN_TYPE_STD);
   GPIOPinConfigure(GPIO_PJ2_MIIRXCK);

   //Configure MII_MDC (PJ3)
   GPIODirModeSet(GPIO_PORTJ_BASE, GPIO_PIN_3, GPIO_DIR_MODE_HW);
   GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_3, GPIO_PIN_TYPE_STD);
   GPIOPinConfigure(GPIO_PJ3_MIIMDC);

   //Configure MII_COL (PJ4)
   GPIODirModeSet(GPIO_PORTJ_BASE, GPIO_PIN_4, GPIO_DIR_MODE_HW);
   GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_4, GPIO_PIN_TYPE_STD);
   GPIOPinConfigure(GPIO_PJ4_MIICOL);

   //Configure MII_CRS (PJ5)
   GPIODirModeSet(GPIO_PORTJ_BASE, GPIO_PIN_5, GPIO_DIR_MODE_HW);
   GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_5, GPIO_PIN_TYPE_STD);
   GPIOPinConfigure(GPIO_PJ5_MIICRS);

   //Configure MII_PHYINTR (PJ6)
   GPIODirModeSet(GPIO_PORTJ_BASE, GPIO_PIN_6, GPIO_DIR_MODE_HW);
   GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_6, GPIO_PIN_TYPE_STD);
   GPIOPinConfigure(GPIO_PJ6_MIIPHYINTRn);

   //Configure MII_PHYRSTn (PJ7)
   GPIODirModeSet(GPIO_PORTJ_BASE, GPIO_PIN_7, GPIO_DIR_MODE_HW);
   GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_7, GPIO_PIN_TYPE_STD);
   GPIOPinConfigure(GPIO_PJ7_MIIPHYRSTn);
}

#endif


/**
 * @brief F28M35x Ethernet MAC timer handler
 *
 * This routine is periodically called by the TCP/IP stack to handle periodic
 * operations such as polling the link state
 *
 * @param[in] interface Underlying network interface
 **/

void f28m35xEthTick(NetInterface *interface)
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

void f28m35xEthEnableIrq(NetInterface *interface)
{
#ifdef ti_sysbios_BIOS___VERS
   //Enable Ethernet MAC interrupts
   Hwi_enableInterrupt(INT_ETH);
#else
   //Enable Ethernet MAC interrupts
   IntEnable(INT_ETH);
#endif


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

void f28m35xEthDisableIrq(NetInterface *interface)
{
#ifdef ti_sysbios_BIOS___VERS
   //Disable Ethernet MAC interrupts
   Hwi_disableInterrupt(INT_ETH);
#else
   //Disable Ethernet MAC interrupts
   IntDisable(INT_ETH);
#endif


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
 * @brief F28M35x Ethernet MAC interrupt service routine
 **/

void f28m35xEthIrqHandler(void)
{
   bool_t flag;
   uint32_t status;

   //Interrupt service routine prologue
   osEnterIsr();

   //This flag will be set if a higher priority task must be woken
   flag = FALSE;

   //Read interrupt status register
   status = MAC_RIS_R;

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
 * @brief F28M35x Ethernet MAC event handler
 * @param[in] interface Underlying network interface
 **/

void f28m35xEthEventHandler(NetInterface *interface)
{
   //Packet received?
   if((MAC_RIS_R & MAC_RIS_RXINT) != 0)
   {
      //Acknowledge RXINT interrupt
      MAC_IACK_R = MAC_IACK_RXINT;

      //Process all the pending packets
      while((MAC_NP_R & MAC_NP_NPR_M) != 0)
      {
         //Read incoming packet
         f28m35xEthReceivePacket(interface);
      }
   }

   //Re-enable Ethernet interrupts
   MAC_IM_R = MAC_IM_TXEMPM | MAC_IM_RXINTM;
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

error_t f28m35xEthSendPacket(NetInterface *interface,
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
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t f28m35xEthReceivePacket(NetInterface *interface)
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

error_t f28m35xEthUpdateMacAddrFilter(NetInterface *interface)
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
 * @brief Adjust MAC configuration parameters for proper operation
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t f28m35xEthUpdateMacConfig(NetInterface *interface)
{
   //Half-duplex or full-duplex mode?
   if(interface->duplexMode == NIC_FULL_DUPLEX_MODE)
   {
      MAC_TCTL_R |= MAC_TCTL_DUPLEX;
   }
   else
   {
      MAC_TCTL_R &= ~MAC_TCTL_DUPLEX;
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

void f28m35xEthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data)
{
   //Valid opcode?
   if(opcode == SMI_OPCODE_WRITE)
   {
      //Set PHY address
      MAC_MAR_R = phyAddr;
      //Data to be written in the PHY register
      MAC_MTXD_R = data & MAC_MTXD_MDTX_M;
      //Start a write operation
      MAC_MCTL_R = (regAddr << 3) | MAC_MCTL_WRITE | MAC_MCTL_START;

      //Wait for the write to complete
      while((MAC_MCTL_R & MAC_MCTL_START) != 0)
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

uint16_t f28m35xEthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr)
{
   uint16_t data;

   //Valid opcode?
   if(opcode == SMI_OPCODE_READ)
   {
      //Set PHY address
      MAC_MAR_R = phyAddr;
      //Start a read operation
      MAC_MCTL_R = (regAddr << 3) | MAC_MCTL_START;

      //Wait for the read to complete
      while((MAC_MCTL_R & MAC_MCTL_START) != 0)
      {
      }

      //Get register value
      data = MAC_MRXD_R & MAC_MRXD_MDRX_M;
   }
   else
   {
      //The MAC peripheral only supports standard Clause 22 opcodes
      data = 0;
   }

   //Return the value of the PHY register
   return data;
}
