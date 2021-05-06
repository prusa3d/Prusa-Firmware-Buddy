/**
 * @file efm32gg11_eth_driver.c
 * @brief EFM32 Giant Gecko 11 Ethernet MAC driver
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
#include <limits.h>
#include "em_device.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "core/net.h"
#include "drivers/mac/efm32gg11_eth_driver.h"
#include "debug.h"

//Underlying network interface
static NetInterface *nicDriverInterface;

//IAR EWARM compiler?
#if defined(__ICCARM__)

//TX buffer
#pragma data_alignment = 8
static uint8_t txBuffer[EFM32GG11_ETH_TX_BUFFER_COUNT][EFM32GG11_ETH_TX_BUFFER_SIZE];
//RX buffer
#pragma data_alignment = 8
static uint8_t rxBuffer[EFM32GG11_ETH_RX_BUFFER_COUNT][EFM32GG11_ETH_RX_BUFFER_SIZE];
//TX buffer descriptors
#pragma data_alignment = 4
static Efm32gg11TxBufferDesc txBufferDesc[EFM32GG11_ETH_TX_BUFFER_COUNT];
//RX buffer descriptors
#pragma data_alignment = 4
static Efm32gg11RxBufferDesc rxBufferDesc[EFM32GG11_ETH_RX_BUFFER_COUNT];

//Keil MDK-ARM or GCC compiler?
#else

//TX buffer
static uint8_t txBuffer[EFM32GG11_ETH_TX_BUFFER_COUNT][EFM32GG11_ETH_TX_BUFFER_SIZE]
   __attribute__((aligned(8)));
//RX buffer
static uint8_t rxBuffer[EFM32GG11_ETH_RX_BUFFER_COUNT][EFM32GG11_ETH_RX_BUFFER_SIZE]
   __attribute__((aligned(8)));
//TX buffer descriptors
static Efm32gg11TxBufferDesc txBufferDesc[EFM32GG11_ETH_TX_BUFFER_COUNT]
   __attribute__((aligned(4)));
//RX buffer descriptors
static Efm32gg11RxBufferDesc rxBufferDesc[EFM32GG11_ETH_RX_BUFFER_COUNT]
   __attribute__((aligned(4)));

#endif

//TX buffer index
static uint_t txBufferIndex;
//RX buffer index
static uint_t rxBufferIndex;


/**
 * @brief EFM32GG11 Ethernet MAC driver
 **/

const NicDriver efm32gg11EthDriver =
{
   NIC_TYPE_ETHERNET,
   ETH_MTU,
   efm32gg11EthInit,
   efm32gg11EthTick,
   efm32gg11EthEnableIrq,
   efm32gg11EthDisableIrq,
   efm32gg11EthEventHandler,
   efm32gg11EthSendPacket,
   efm32gg11EthUpdateMacAddrFilter,
   efm32gg11EthUpdateMacConfig,
   efm32gg11EthWritePhyReg,
   efm32gg11EthReadPhyReg,
   TRUE,
   TRUE,
   TRUE,
   FALSE
};


/**
 * @brief EFM32GG11 Ethernet MAC initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t efm32gg11EthInit(NetInterface *interface)
{
   error_t error;
   volatile uint32_t status;

   //Debug message
   TRACE_INFO("Initializing EFM32GG11 Ethernet MAC...\r\n");

   //Save underlying network interface
   nicDriverInterface = interface;

   //Enable high-frequency peripheral clock
   CMU_ClockEnable(cmuClock_HFPER, true);
   //Enable Ethernet peripheral clock
   CMU_ClockEnable(cmuClock_ETH, true);

   //Disable transmit and receive circuits
   ETH->NETWORKCTRL = 0;

   //GPIO configuration
   efm32gg11EthInitGpio(interface);

   //Configure MDC clock speed
   ETH->NETWORKCFG = (4 << _ETH_NETWORKCFG_MDCCLKDIV_SHIFT) &
      _ETH_NETWORKCFG_MDCCLKDIV_MASK;

   //Enable management port (MDC and MDIO)
   ETH->NETWORKCTRL |= _ETH_NETWORKCTRL_MANPORTEN_MASK;

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
   ETH->SPECADDR1BOTTOM = interface->macAddr.w[0] | (interface->macAddr.w[1] << 16);
   ETH->SPECADDR1TOP = interface->macAddr.w[2];

   //The MAC supports 3 additional addresses for unicast perfect filtering
   ETH->SPECADDR2BOTTOM = 0;
   ETH->SPECADDR3BOTTOM = 0;
   ETH->SPECADDR4BOTTOM = 0;

   //Initialize hash table
   ETH->HASHBOTTOM = 0;
   ETH->HASHTOP = 0;

   //Configure the receive filter
   ETH->NETWORKCFG |= _ETH_NETWORKCFG_RX1536BYTEFRAMES_MASK |
      _ETH_NETWORKCFG_MULTICASTHASHEN_MASK;

   //Initialize buffer descriptors
   efm32gg11EthInitBufferDesc(interface);

   //Clear transmit status register
   ETH->TXSTATUS = _ETH_TXSTATUS_TXUNDERRUN_MASK |
      _ETH_TXSTATUS_TXCMPLT_MASK | _ETH_TXSTATUS_AMBAERR_MASK |
      _ETH_TXSTATUS_TXGO_MASK | _ETH_TXSTATUS_RETRYLMTEXCD_MASK |
      _ETH_TXSTATUS_COLOCCRD_MASK | _ETH_TXSTATUS_USEDBITREAD_MASK;

   //Clear receive status register
   ETH->RXSTATUS = _ETH_RXSTATUS_RXOVERRUN_MASK | _ETH_RXSTATUS_FRMRX_MASK |
      _ETH_RXSTATUS_BUFFNOTAVAIL_MASK;

   //First disable all interrupts
   ETH->IENC = 0xFFFFFFFF;

   //Only the desired ones are enabled
   ETH->IENS = _ETH_IENS_RXOVERRUN_MASK |
      _ETH_IENS_TXCMPLT_MASK | _ETH_IENS_AMBAERR_MASK |
      _ETH_IENS_RTRYLMTORLATECOL_MASK | _ETH_IENS_TXUNDERRUN_MASK |
      _ETH_IENS_RXUSEDBITREAD_MASK | _ETH_IENS_RXCMPLT_MASK;

   //Read ETH_IFCR register to clear any pending interrupt
   status = ETH->IFCR;

   //Set priority grouping (3 bits for pre-emption priority, no bits for subpriority)
   NVIC_SetPriorityGrouping(EFM32GG11_ETH_IRQ_PRIORITY_GROUPING);

   //Configure Ethernet interrupt priority
   NVIC_SetPriority(ETH_IRQn, NVIC_EncodePriority(EFM32GG11_ETH_IRQ_PRIORITY_GROUPING,
      EFM32GG11_ETH_IRQ_GROUP_PRIORITY, EFM32GG11_ETH_IRQ_SUB_PRIORITY));

   //Enable the transmitter and the receiver
   ETH->NETWORKCTRL |= _ETH_NETWORKCTRL_ENBTX_MASK | _ETH_NETWORKCTRL_ENBRX_MASK;

   //Accept any packets from the upper layer
   osSetEvent(&interface->nicTxEvent);

   //Successful initialization
   return NO_ERROR;
}


//EFM32 Giant Gecko 11 Starter Kit?
#if defined(USE_EFM32_GIANT_GECKO_11_SK)

/**
 * @brief GPIO configuration
 * @param[in] interface Underlying network interface
 **/

void efm32gg11EthInitGpio(NetInterface *interface)
{
   uint32_t temp;

   //Enable GPIO clock
   CMU_ClockEnable(cmuClock_GPIO, true);
   //Enable external oscillator
   CMU_OscillatorEnable(cmuOsc_HFXO, true, true);

   //Select CMU_CLK2 clock source
   CMU->CTRL |= CMU_CTRL_CLKOUTSEL2_HFXO;

   //Configure CMU_CLK2 (PD10)
   GPIO_PinModeSet((GPIO_Port_TypeDef) AF_CMU_CLK2_PORT(5),
      AF_CMU_CLK2_PIN(5), gpioModePushPull, 0);

   //Remap CMU_CLK2 pin
   temp = CMU->ROUTELOC0 & ~_CMU_ROUTELOC0_CLKOUT2LOC_MASK;
   CMU->ROUTELOC0 = temp | CMU_ROUTELOC0_CLKOUT2LOC_LOC5;

   //Enable CMU_CLK2 pin
   CMU->ROUTEPEN |= CMU_ROUTEPEN_CLKOUT2PEN;

   //Select RMII operation mode and enable transceiver clock
   ETH->CTRL = ETH_CTRL_GBLCLKEN | ETH_CTRL_MIISEL_RMII;

   //Configure ETH_RMII_TXD0 (PF7)
   GPIO_PinModeSet((GPIO_Port_TypeDef) AF_ETH_RMIITXD0_PORT(1),
      AF_ETH_RMIITXD0_PIN(1), gpioModePushPull, 0);

   //Configure ETH_RMII_TXD1 (PF6)
   GPIO_PinModeSet((GPIO_Port_TypeDef) AF_ETH_RMIITXD1_PORT(1),
      AF_ETH_RMIITXD1_PIN(1), gpioModePushPull, 0);

   //Configure ETH_RMII_TXEN (PF8)
   GPIO_PinModeSet((GPIO_Port_TypeDef) AF_ETH_RMIITXEN_PORT(1),
      AF_ETH_RMIITXEN_PIN(1), gpioModePushPull, 0);

   //Configure ETH_RMII_RXD0 (PD9)
   GPIO_PinModeSet((GPIO_Port_TypeDef) AF_ETH_RMIIRXD0_PORT(1),
      AF_ETH_RMIIRXD0_PIN(1), gpioModeInput, 0);

   //Configure ETH_RMII_RXD1 (PF9)
   GPIO_PinModeSet((GPIO_Port_TypeDef) AF_ETH_RMIIRXD1_PORT(1),
      AF_ETH_RMIIRXD1_PIN(1), gpioModeInput, 0);

   //Configure ETH_RMII_CRSDV (PD11)
   GPIO_PinModeSet((GPIO_Port_TypeDef) AF_ETH_RMIICRSDV_PORT(1),
      AF_ETH_RMIICRSDV_PIN(1), gpioModeInput, 0);

   //Configure ETH_RMII_RXER (PD12)
   GPIO_PinModeSet((GPIO_Port_TypeDef) AF_ETH_RMIIRXER_PORT(1),
      AF_ETH_RMIIRXER_PIN(1), gpioModeInput, 0);

   //Configure ETH_MDIO (PD13)
   GPIO_PinModeSet((GPIO_Port_TypeDef) AF_ETH_MDIO_PORT(1),
      AF_ETH_MDIO_PIN(1), gpioModePushPull, 0);

   //Configure ETH_MDC (PD14)
   GPIO_PinModeSet((GPIO_Port_TypeDef) AF_ETH_MDC_PORT(1),
      AF_ETH_MDC_PIN(1), gpioModePushPull, 0);

   //Remap RMII pins
   temp = ETH->ROUTELOC1 & ~(_ETH_ROUTELOC1_RMIILOC_MASK & _ETH_ROUTELOC1_MDIOLOC_MASK);
   ETH->ROUTELOC1 = temp | (ETH_ROUTELOC1_RMIILOC_LOC1 | ETH_ROUTELOC1_MDIOLOC_LOC1);

   //Enable RMII pins
   ETH->ROUTEPEN  = ETH_ROUTEPEN_RMIIPEN | ETH_ROUTEPEN_MDIOPEN;

   //Configure ETH_PWR_ENABLE (PI10)
   GPIO_PinModeSet(gpioPortI, 10, gpioModePushPull, 0);
   //Configure ETH_RESET_N (PH7)
   GPIO_PinModeSet(gpioPortH, 7, gpioModePushPull, 0);
   //Configure ETH_INTRP (PG15)
   GPIO_PinModeSet(gpioPortG, 15, gpioModeInput, 0);

   //Power on PHY transceiver
   GPIO_PinOutSet(gpioPortI, 10);
   sleep(10);

   //Reset PHY transceiver (hard reset)
   GPIO_PinOutClear(gpioPortH, 7);
   sleep(10);
   GPIO_PinOutSet(gpioPortH, 7);
   sleep(10);
}

#endif


/**
 * @brief Initialize buffer descriptors
 * @param[in] interface Underlying network interface
 **/

void efm32gg11EthInitBufferDesc(NetInterface *interface)
{
   uint_t i;
   uint32_t address;

   //Initialize TX buffer descriptors
   for(i = 0; i < EFM32GG11_ETH_TX_BUFFER_COUNT; i++)
   {
      //Calculate the address of the current TX buffer
      address = (uint32_t) txBuffer[i];
      //Write the address to the descriptor entry
      txBufferDesc[i].address = address;
      //Initialize status field
      txBufferDesc[i].status = ETH_TX_USED;
   }

   //Mark the last descriptor entry with the wrap flag
   txBufferDesc[i - 1].status |= ETH_TX_WRAP;
   //Initialize TX buffer index
   txBufferIndex = 0;

   //Initialize RX buffer descriptors
   for(i = 0; i < EFM32GG11_ETH_RX_BUFFER_COUNT; i++)
   {
      //Calculate the address of the current RX buffer
      address = (uint32_t) rxBuffer[i];
      //Write the address to the descriptor entry
      rxBufferDesc[i].address = address & ETH_RX_ADDRESS;
      //Clear status field
      rxBufferDesc[i].status = 0;
   }

   //Mark the last descriptor entry with the wrap flag
   rxBufferDesc[i - 1].address |= ETH_RX_WRAP;
   //Initialize RX buffer index
   rxBufferIndex = 0;

   //Start location of the TX descriptor list
   ETH->TXQPTR = (uint32_t) txBufferDesc;
   //Start location of the RX descriptor list
   ETH->RXQPTR = (uint32_t) rxBufferDesc;
}


/**
 * @brief EFM32GG11 Ethernet MAC timer handler
 *
 * This routine is periodically called by the TCP/IP stack to handle periodic
 * operations such as polling the link state
 *
 * @param[in] interface Underlying network interface
 **/

void efm32gg11EthTick(NetInterface *interface)
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

void efm32gg11EthEnableIrq(NetInterface *interface)
{
   //Enable Ethernet MAC interrupts
   NVIC_EnableIRQ(ETH_IRQn);

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

void efm32gg11EthDisableIrq(NetInterface *interface)
{
   //Disable Ethernet MAC interrupts
   NVIC_DisableIRQ(ETH_IRQn);

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
 * @brief EFM32GG11 Ethernet MAC interrupt service routine
 **/

void ETH_IRQHandler(void)
{
   bool_t flag;
   volatile uint32_t isr;
   volatile uint32_t tsr;
   volatile uint32_t rsr;

   //Interrupt service routine prologue
   osEnterIsr();

   //This flag will be set if a higher priority task must be woken
   flag = FALSE;

   //Each time the software reads ETH_IFCR, it has to check the contents
   //of ETH_TXSTATUS, ETH_RXSTATUS and ETH_NETWORKSTATUS
   isr = ETH->IFCR;
   tsr = ETH->TXSTATUS;
   rsr = ETH->RXSTATUS;

   //Clear interrupt flags
   ETH->IFCR = isr;

   //Packet transmitted?
   if((tsr & (_ETH_TXSTATUS_TXUNDERRUN_MASK | _ETH_TXSTATUS_TXCMPLT_MASK |
      _ETH_TXSTATUS_AMBAERR_MASK | _ETH_TXSTATUS_TXGO_MASK |
      _ETH_TXSTATUS_RETRYLMTEXCD_MASK | _ETH_TXSTATUS_COLOCCRD_MASK |
      _ETH_TXSTATUS_USEDBITREAD_MASK)) != 0)
   {
      //Only clear TXSTATUS flags that are currently set
      ETH->TXSTATUS = tsr;

      //Check whether the TX buffer is available for writing
      if((txBufferDesc[txBufferIndex].status & ETH_TX_USED) != 0)
      {
         //Notify the TCP/IP stack that the transmitter is ready to send
         flag |= osSetEventFromIsr(&nicDriverInterface->nicTxEvent);
      }
   }

   //Packet received?
   if((rsr & (_ETH_RXSTATUS_RXOVERRUN_MASK | _ETH_RXSTATUS_FRMRX_MASK |
      _ETH_RXSTATUS_BUFFNOTAVAIL_MASK)) != 0)
   {
      //Set event flag
      nicDriverInterface->nicEvent = TRUE;
      //Notify the TCP/IP stack of the event
      flag |= osSetEventFromIsr(&netEvent);
   }

   //Interrupt service routine epilogue
   osExitIsr(flag);
}


/**
 * @brief EFM32GG11 Ethernet MAC event handler
 * @param[in] interface Underlying network interface
 **/

void efm32gg11EthEventHandler(NetInterface *interface)
{
   error_t error;
   uint32_t rsr;

   //Read receive status
   rsr = ETH->RXSTATUS;

   //Packet received?
   if((rsr & (_ETH_RXSTATUS_RXOVERRUN_MASK | _ETH_RXSTATUS_FRMRX_MASK |
      _ETH_RXSTATUS_BUFFNOTAVAIL_MASK)) != 0)
   {
      //Only clear RXSTATUS flags that are currently set
      ETH->RXSTATUS = rsr;

      //Process all pending packets
      do
      {
         //Read incoming packet
         error = efm32gg11EthReceivePacket(interface);

         //No more data in the receive buffer?
      } while(error != ERROR_BUFFER_EMPTY);
   }
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

error_t efm32gg11EthSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary)
{
   size_t length;

   //Retrieve the length of the packet
   length = netBufferGetLength(buffer) - offset;

   //Check the frame length
   if(length > EFM32GG11_ETH_TX_BUFFER_SIZE)
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
      //Report an error
      return ERROR_INVALID_LENGTH;
   }

   //Make sure the current buffer is available for writing
   if((txBufferDesc[txBufferIndex].status & ETH_TX_USED) == 0)
   {
      return ERROR_FAILURE;
   }

   //Copy user data to the transmit buffer
   netBufferRead(txBuffer[txBufferIndex], buffer, offset, length);

   //Set the necessary flags in the descriptor entry
   if(txBufferIndex < (EFM32GG11_ETH_TX_BUFFER_COUNT - 1))
   {
      //Write the status word
      txBufferDesc[txBufferIndex].status = ETH_TX_LAST |
         (length & ETH_TX_LENGTH);

      //Point to the next buffer
      txBufferIndex++;
   }
   else
   {
      //Write the status word
      txBufferDesc[txBufferIndex].status = ETH_TX_WRAP | ETH_TX_LAST |
         (length & ETH_TX_LENGTH);

      //Wrap around
      txBufferIndex = 0;
   }

   //Set the TSTART bit to initiate transmission
   ETH->NETWORKCTRL |= _ETH_NETWORKCTRL_TXSTRT_MASK;

   //Check whether the next buffer is available for writing
   if((txBufferDesc[txBufferIndex].status & ETH_TX_USED) != 0)
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Receive a packet
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t efm32gg11EthReceivePacket(NetInterface *interface)
{
   static uint8_t temp[ETH_MAX_FRAME_SIZE];
   error_t error;
   uint_t i;
   uint_t j;
   uint_t sofIndex;
   uint_t eofIndex;
   size_t n;
   size_t size;
   size_t length;

   //Initialize SOF and EOF indices
   sofIndex = UINT_MAX;
   eofIndex = UINT_MAX;

   //Search for SOF and EOF flags
   for(i = 0; i < EFM32GG11_ETH_RX_BUFFER_COUNT; i++)
   {
      //Point to the current entry
      j = rxBufferIndex + i;

      //Wrap around to the beginning of the buffer if necessary
      if(j >= EFM32GG11_ETH_RX_BUFFER_COUNT)
      {
         j -= EFM32GG11_ETH_RX_BUFFER_COUNT;
      }

      //No more entries to process?
      if((rxBufferDesc[j].address & ETH_RX_OWNERSHIP) == 0)
      {
         //Stop processing
         break;
      }

      //A valid SOF has been found?
      if((rxBufferDesc[j].status & ETH_RX_SOF) != 0)
      {
         //Save the position of the SOF
         sofIndex = i;
      }

      //A valid EOF has been found?
      if((rxBufferDesc[j].status & ETH_RX_EOF) != 0 && sofIndex != UINT_MAX)
      {
         //Save the position of the EOF
         eofIndex = i;
         //Retrieve the length of the frame
         size = rxBufferDesc[j].status & ETH_RX_LENGTH;
         //Limit the number of data to read
         size = MIN(size, ETH_MAX_FRAME_SIZE);
         //Stop processing since we have reached the end of the frame
         break;
      }
   }

   //Determine the number of entries to process
   if(eofIndex != UINT_MAX)
   {
      j = eofIndex + 1;
   }
   else if(sofIndex != UINT_MAX)
   {
      j = sofIndex;
   }
   else
   {
      j = i;
   }

   //Total number of bytes that have been copied from the receive buffer
   length = 0;

   //Process incoming frame
   for(i = 0; i < j; i++)
   {
      //Any data to copy from current buffer?
      if(eofIndex != UINT_MAX && i >= sofIndex && i <= eofIndex)
      {
         //Calculate the number of bytes to read at a time
         n = MIN(size, EFM32GG11_ETH_RX_BUFFER_SIZE);
         //Copy data from receive buffer
         osMemcpy(temp + length, rxBuffer[rxBufferIndex], n);
         //Update byte counters
         length += n;
         size -= n;
      }

      //Mark the current buffer as free
      rxBufferDesc[rxBufferIndex].address &= ~ETH_RX_OWNERSHIP;

      //Point to the following entry
      rxBufferIndex++;

      //Wrap around to the beginning of the buffer if necessary
      if(rxBufferIndex >= EFM32GG11_ETH_RX_BUFFER_COUNT)
      {
         rxBufferIndex = 0;
      }
   }

   //Any packet to process?
   if(length > 0)
   {
      NetRxAncillary ancillary;

      //Additional options can be passed to the stack along with the packet
      ancillary = NET_DEFAULT_RX_ANCILLARY;

      //Pass the packet to the upper layer
      nicProcessPacket(interface, temp, length, &ancillary);
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

error_t efm32gg11EthUpdateMacAddrFilter(NetInterface *interface)
{
   uint_t i;
   uint_t j;
   uint_t k;
   uint8_t *p;
   uint32_t hashTable[2];
   MacAddr unicastMacAddr[3];
   MacFilterEntry *entry;

   //Debug message
   TRACE_DEBUG("Updating MAC filter...\r\n");

   //Set the MAC address of the station
   ETH->SPECADDR1BOTTOM = interface->macAddr.w[0] | (interface->macAddr.w[1] << 16);
   ETH->SPECADDR1TOP = interface->macAddr.w[2];

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
            //Point to the MAC address
            p = entry->addr.b;

            //Apply the hash function
            k = (p[0] >> 6) ^ p[0];
            k ^= (p[1] >> 4) ^ (p[1] << 2);
            k ^= (p[2] >> 2) ^ (p[2] << 4);
            k ^= (p[3] >> 6) ^ p[3];
            k ^= (p[4] >> 4) ^ (p[4] << 2);
            k ^= (p[5] >> 2) ^ (p[5] << 4);

            //The hash value is reduced to a 6-bit index
            k &= 0x3F;

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
      //The address is activated when SAT register is written
      ETH->SPECADDR2BOTTOM = unicastMacAddr[0].w[0] | (unicastMacAddr[0].w[1] << 16);
      ETH->SPECADDR2TOP = unicastMacAddr[0].w[2];
   }
   else
   {
      //The address is deactivated when SAB register is written
      ETH->SPECADDR2BOTTOM = 0;
   }

   //Configure the second unicast address filter
   if(j >= 2)
   {
      //The address is activated when SAT register is written
      ETH->SPECADDR3BOTTOM = unicastMacAddr[1].w[0] | (unicastMacAddr[1].w[1] << 16);
      ETH->SPECADDR3TOP = unicastMacAddr[1].w[2];
   }
   else
   {
      //The address is deactivated when SAB register is written
      ETH->SPECADDR3BOTTOM = 0;
   }

   //Configure the third unicast address filter
   if(j >= 3)
   {
      //The address is activated when SAT register is written
      ETH->SPECADDR4BOTTOM = unicastMacAddr[2].w[0] | (unicastMacAddr[2].w[1] << 16);
      ETH->SPECADDR4TOP = unicastMacAddr[2].w[2];
   }
   else
   {
      //The address is deactivated when SAB register is written
      ETH->SPECADDR4BOTTOM = 0;
   }

   //Configure the multicast address filter
   ETH->HASHBOTTOM = hashTable[0];
   ETH->HASHTOP = hashTable[1];

   //Debug message
   TRACE_DEBUG("  HASHBOTTOM = %08" PRIX32 "\r\n", ETH->HASHBOTTOM);
   TRACE_DEBUG("  HASHTOP = %08" PRIX32 "\r\n", ETH->HASHTOP);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Adjust MAC configuration parameters for proper operation
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t efm32gg11EthUpdateMacConfig(NetInterface *interface)
{
   uint32_t config;

   //Read network configuration register
   config = ETH->NETWORKCFG;

   //10BASE-T or 100BASE-TX operation mode?
   if(interface->linkSpeed == NIC_LINK_SPEED_100MBPS)
   {
      config |= _ETH_NETWORKCFG_SPEED_MASK;
   }
   else
   {
      config &= ~_ETH_NETWORKCFG_SPEED_MASK;
   }

   //Half-duplex or full-duplex mode?
   if(interface->duplexMode == NIC_FULL_DUPLEX_MODE)
   {
      config |= _ETH_NETWORKCFG_FULLDUPLEX_MASK;
   }
   else
   {
      config &= ~_ETH_NETWORKCFG_FULLDUPLEX_MASK;
   }

   //Write configuration value back to ETH_NETWORKCFG register
   ETH->NETWORKCFG = config;

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

void efm32gg11EthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data)
{
   uint32_t temp;

   //Valid opcode?
   if(opcode == SMI_OPCODE_WRITE)
   {
      //Set up a write operation
      temp = _ETH_PHYMNGMNT_WRITE1_MASK;
      temp |= (1 << _ETH_PHYMNGMNT_OPERATION_SHIFT) & _ETH_PHYMNGMNT_OPERATION_MASK;
      temp |= (2 << _ETH_PHYMNGMNT_WRITE10_SHIFT) & _ETH_PHYMNGMNT_WRITE10_MASK;

      //PHY address
      temp |= (phyAddr << _ETH_PHYMNGMNT_PHYADDR_SHIFT) & _ETH_PHYMNGMNT_PHYADDR_MASK;
      //Register address
      temp |= (regAddr << _ETH_PHYMNGMNT_REGADDR_SHIFT) & _ETH_PHYMNGMNT_REGADDR_MASK;
      //Register value
      temp |= data & _ETH_PHYMNGMNT_PHYRWDATA_MASK;

      //Start a write operation
      ETH->PHYMNGMNT = temp;
      //Wait for the write to complete
      while((ETH->NETWORKSTATUS & _ETH_NETWORKSTATUS_MANDONE_MASK) == 0)
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

uint16_t efm32gg11EthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr)
{
   uint16_t data;
   uint32_t temp;

   //Valid opcode?
   if(opcode == SMI_OPCODE_READ)
   {
      //Set up a read operation
      temp = _ETH_PHYMNGMNT_WRITE1_MASK;
      temp |= (2 << _ETH_PHYMNGMNT_OPERATION_SHIFT) & _ETH_PHYMNGMNT_OPERATION_MASK;
      temp |= (2 << _ETH_PHYMNGMNT_WRITE10_SHIFT) & _ETH_PHYMNGMNT_WRITE10_MASK;

      //PHY address
      temp |= (phyAddr << _ETH_PHYMNGMNT_PHYADDR_SHIFT) & _ETH_PHYMNGMNT_PHYADDR_MASK;
      //Register address
      temp |= (regAddr << _ETH_PHYMNGMNT_REGADDR_SHIFT) & _ETH_PHYMNGMNT_REGADDR_MASK;

      //Start a read operation
      ETH->PHYMNGMNT = temp;
      //Wait for the read to complete
      while((ETH->NETWORKSTATUS & _ETH_NETWORKSTATUS_MANDONE_MASK) == 0)
      {
      }

      //Get register value
      data = ETH->PHYMNGMNT & _ETH_PHYMNGMNT_PHYRWDATA_MASK;
   }
   else
   {
      //The MAC peripheral only supports standard Clause 22 opcodes
      data = 0;
   }

   //Return the value of the PHY register
   return data;
}
