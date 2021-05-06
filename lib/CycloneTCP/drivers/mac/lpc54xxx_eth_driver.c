/**
 * @file lpc54xxx_eth_driver.c
 * @brief LPC540xx/LPC546xx Ethernet MAC driver
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
#include "fsl_device_registers.h"
#include "fsl_power.h"
#include "fsl_reset.h"
#include "fsl_clock.h"
#include "fsl_iocon.h"
#include "fsl_gpio.h"
#include "core/net.h"
#include "drivers/mac/lpc54xxx_eth_driver.h"
#include "debug.h"

//Underlying network interface
static NetInterface *nicDriverInterface;

//IAR EWARM compiler?
#if defined(__ICCARM__)

//Transmit buffer
#pragma data_alignment = 4
static uint8_t txBuffer[LPC54XXX_ETH_TX_BUFFER_COUNT][LPC54XXX_ETH_TX_BUFFER_SIZE];
//Receive buffer
#pragma data_alignment = 4
static uint8_t rxBuffer[LPC54XXX_ETH_RX_BUFFER_COUNT][LPC54XXX_ETH_RX_BUFFER_SIZE];
//Transmit DMA descriptors
#pragma data_alignment = 4
static Lpc54xxxTxDmaDesc txDmaDesc[LPC54XXX_ETH_TX_BUFFER_COUNT];
//Receive DMA descriptors
#pragma data_alignment = 4
static Lpc54xxxRxDmaDesc rxDmaDesc[LPC54XXX_ETH_RX_BUFFER_COUNT];

//Keil MDK-ARM or GCC compiler?
#else

//Transmit buffer
static uint8_t txBuffer[LPC54XXX_ETH_TX_BUFFER_COUNT][LPC54XXX_ETH_TX_BUFFER_SIZE]
   __attribute__((aligned(4)));
//Receive buffer
static uint8_t rxBuffer[LPC54XXX_ETH_RX_BUFFER_COUNT][LPC54XXX_ETH_RX_BUFFER_SIZE]
   __attribute__((aligned(4)));
//Transmit DMA descriptors
static Lpc54xxxTxDmaDesc txDmaDesc[LPC54XXX_ETH_TX_BUFFER_COUNT]
   __attribute__((aligned(4)));
//Receive DMA descriptors
static Lpc54xxxRxDmaDesc rxDmaDesc[LPC54XXX_ETH_RX_BUFFER_COUNT]
   __attribute__((aligned(4)));

#endif

//Current transmit descriptor
static uint_t txIndex;
//Current receive descriptor
static uint_t rxIndex;


/**
 * @brief LPC54xxx Ethernet MAC driver
 **/

const NicDriver lpc54xxxEthDriver =
{
   NIC_TYPE_ETHERNET,
   ETH_MTU,
   lpc54xxxEthInit,
   lpc54xxxEthTick,
   lpc54xxxEthEnableIrq,
   lpc54xxxEthDisableIrq,
   lpc54xxxEthEventHandler,
   lpc54xxxEthSendPacket,
   lpc54xxxEthUpdateMacAddrFilter,
   lpc54xxxEthUpdateMacConfig,
   lpc54xxxEthWritePhyReg,
   lpc54xxxEthReadPhyReg,
   TRUE,
   TRUE,
   TRUE,
   FALSE
};


/**
 * @brief LPC54xxx Ethernet MAC initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t lpc54xxxEthInit(NetInterface *interface)
{
   error_t error;

   //Debug message
   TRACE_INFO("Initializing LPC54xxx Ethernet MAC...\r\n");

   //Save underlying network interface
   nicDriverInterface = interface;

   //Enable ENET peripheral clock
   CLOCK_EnableClock(kCLOCK_Eth);
   //Reset ENET module
   RESET_PeripheralReset(kETH_RST_SHIFT_RSTn);

   //GPIO configuration
   lpc54xxxEthInitGpio(interface);

   //Perform a software reset
   ENET->DMA_MODE |= ENET_DMA_MODE_SWR_MASK;
   //Wait for the reset to complete
   while((ENET->DMA_MODE & ENET_DMA_MODE_SWR_MASK) != 0)
   {
   }

   //Adjust MDC clock range depending on CSR frequency
   ENET->MAC_MDIO_ADDR = ENET_MAC_MDIO_ADDR_CR(4);

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
   ENET->MAC_CONFIG = ENET_MAC_CONFIG_PS_MASK | ENET_MAC_CONFIG_DO_MASK;

   //Set the MAC address of the station
   ENET->MAC_ADDR_LOW = interface->macAddr.w[0] | (interface->macAddr.w[1] << 16);
   ENET->MAC_ADDR_HIGH = interface->macAddr.w[2];

   //Configure the receive filter
   ENET->MAC_FRAME_FILTER = 0;

   //Disable flow control
   ENET->MAC_TX_FLOW_CTRL_Q[0] = 0;
   ENET->MAC_RX_FLOW_CTRL = 0;

   //Enable the first RX queue
   ENET->MAC_RXQ_CTRL[0] = ENET_MAC_RXQ_CTRL_RXQ0EN(1);

   //Configure DMA operating mode
   ENET->DMA_MODE = ENET_DMA_MODE_PR(0);
   //Configure system bus mode
   ENET->DMA_SYSBUS_MODE |= ENET_DMA_SYSBUS_MODE_AAL_MASK;

   //The DMA takes the descriptor table as contiguous
   ENET->DMA_CH[0].DMA_CHX_CTRL = ENET_DMA_CH_DMA_CHX_CTRL_DSL(0);
   //Configure TX features
   ENET->DMA_CH[0].DMA_CHX_TX_CTRL = ENET_DMA_CH_DMA_CHX_TX_CTRL_TxPBL(1);

   //Configure RX features
   ENET->DMA_CH[0].DMA_CHX_RX_CTRL = ENET_DMA_CH_DMA_CHX_RX_CTRL_RxPBL(1) |
      ENET_DMA_CH_DMA_CHX_RX_CTRL_RBSZ(LPC54XXX_ETH_RX_BUFFER_SIZE / 4);

   //Enable store and forward mode for transmission
   ENET->MTL_QUEUE[0].MTL_TXQX_OP_MODE |= ENET_MTL_QUEUE_MTL_TXQX_OP_MODE_TQS(7) |
      ENET_MTL_QUEUE_MTL_TXQX_OP_MODE_TXQEN(2) |
      ENET_MTL_QUEUE_MTL_TXQX_OP_MODE_TSF_MASK;

   //Enable store and forward mode for reception
   ENET->MTL_QUEUE[0].MTL_RXQX_OP_MODE |= ENET_MTL_QUEUE_MTL_RXQX_OP_MODE_RQS(7) |
      ENET_MTL_QUEUE_MTL_RXQX_OP_MODE_RSF_MASK;

   //Initialize DMA descriptor lists
   lpc54xxxEthInitDmaDesc(interface);

   //Disable MAC interrupts
   ENET->MAC_INTR_EN = 0;

   //Enable the desired DMA interrupts
   ENET->DMA_CH[0].DMA_CHX_INT_EN = ENET_DMA_CH_DMA_CHX_INT_EN_NIE_MASK |
      ENET_DMA_CH_DMA_CHX_INT_EN_RIE_MASK | ENET_DMA_CH_DMA_CHX_INT_EN_TIE_MASK;

   //Set priority grouping (3 bits for pre-emption priority, no bits for subpriority)
   NVIC_SetPriorityGrouping(LPC54XXX_ETH_IRQ_PRIORITY_GROUPING);

   //Configure Ethernet interrupt priority
   NVIC_SetPriority(ETHERNET_IRQn, NVIC_EncodePriority(LPC54XXX_ETH_IRQ_PRIORITY_GROUPING,
      LPC54XXX_ETH_IRQ_GROUP_PRIORITY, LPC54XXX_ETH_IRQ_SUB_PRIORITY));

   //Enable MAC transmission and reception
   ENET->MAC_CONFIG |= ENET_MAC_CONFIG_TE_MASK | ENET_MAC_CONFIG_RE_MASK;

   //Enable DMA transmission and reception
   ENET->DMA_CH[0].DMA_CHX_TX_CTRL |= ENET_DMA_CH_DMA_CHX_TX_CTRL_ST_MASK;
   ENET->DMA_CH[0].DMA_CHX_RX_CTRL |= ENET_DMA_CH_DMA_CHX_RX_CTRL_SR_MASK;

   //Accept any packets from the upper layer
   osSetEvent(&interface->nicTxEvent);

   //Successful initialization
   return NO_ERROR;
}


//LPCXpresso54S018, LPCXpresso54S018M, LPCXpresso54608, LPCXpresso54628 or
//LPC54018-IoT-Module evaluation board?
#if defined(USE_LPCXPRESSO_54S018) || defined(USE_LPCXPRESSO_54S018M) || \
   defined(USE_LPCXPRESSO_54608) || defined(USE_LPCXPRESSO_54628) || \
   defined(USE_LPC54018_IOT_MODULE)

/**
 * @brief GPIO configuration
 * @param[in] interface Underlying network interface
 **/

void lpc54xxxEthInitGpio(NetInterface *interface)
{
   gpio_pin_config_t pinConfig;

//LPCXpresso54S018, LPCXpresso54608, LPCXpresso54628 or LPC54018-IoT-Module
//evaluation board?
#if defined(USE_LPCXPRESSO_54S018) || defined(USE_LPCXPRESSO_54608) || \
   defined(USE_LPCXPRESSO_54628) || defined(USE_LPC54018_IOT_MODULE)
   //Select RMII interface mode
   SYSCON->ETHPHYSEL |= SYSCON_ETHPHYSEL_PHY_SEL_MASK;

   //Enable IOCON clock
   CLOCK_EnableClock(kCLOCK_Iocon);

   //Enable GPIO clocks
   CLOCK_EnableClock(kCLOCK_Gpio0);
   CLOCK_EnableClock(kCLOCK_Gpio2);
   CLOCK_EnableClock(kCLOCK_Gpio4);

   //Configure ENET_TXD1 (P0_17)
   IOCON_PinMuxSet(IOCON, 0, 17, IOCON_FUNC7 | IOCON_MODE_INACT |
      IOCON_DIGITAL_EN | IOCON_INPFILT_OFF);

   //Configure ENET_TXD0 (P4_8)
   IOCON_PinMuxSet(IOCON, 4, 8, IOCON_FUNC1 | IOCON_MODE_INACT |
      IOCON_DIGITAL_EN | IOCON_INPFILT_OFF);

   //Configure ENET_RX_DV (P4_10)
   IOCON_PinMuxSet(IOCON, 4, 10, IOCON_FUNC1 | IOCON_MODE_INACT |
      IOCON_DIGITAL_EN | IOCON_INPFILT_OFF);

   //Configure ENET_RXD0 (P4_11)
   IOCON_PinMuxSet(IOCON, 4, 11, IOCON_FUNC1 | IOCON_MODE_INACT |
      IOCON_DIGITAL_EN | IOCON_INPFILT_OFF);

   //Configure ENET_RXD1 (P4_12)
   IOCON_PinMuxSet(IOCON, 4, 12, IOCON_FUNC1 | IOCON_MODE_INACT |
      IOCON_DIGITAL_EN | IOCON_INPFILT_OFF);

   //Configure ENET_TX_EN (P4_13)
   IOCON_PinMuxSet(IOCON, 4, 13, IOCON_FUNC1 | IOCON_MODE_INACT |
      IOCON_DIGITAL_EN | IOCON_INPFILT_OFF);

   //Configure ENET_RX_CLK (P4_14)
   IOCON_PinMuxSet(IOCON, 4, 14, IOCON_FUNC1 | IOCON_MODE_INACT |
      IOCON_DIGITAL_EN | IOCON_INPFILT_OFF);

   //Configure ENET_MDC (P4_15)
   IOCON_PinMuxSet(IOCON, 4, 15, IOCON_FUNC1 | IOCON_MODE_INACT |
      IOCON_DIGITAL_EN | IOCON_INPFILT_OFF);

   //Configure ENET_MDIO (P4_16)
   IOCON_PinMuxSet(IOCON, 4, 16, IOCON_FUNC1 | IOCON_MODE_PULLUP |
      IOCON_DIGITAL_EN | IOCON_INPFILT_OFF);

   //Configure ENET_RST (P2_26) as an output
   pinConfig.pinDirection = kGPIO_DigitalOutput;
   pinConfig.outputLogic = 0;
   GPIO_PinInit(GPIO, 2, 26, &pinConfig);

   //Reset PHY transceiver (hard reset)
   GPIO_PinWrite(GPIO, 2, 26, 0);
   sleep(10);
   GPIO_PinWrite(GPIO, 2, 26, 1);
   sleep(10);

//LPCXpresso54S018M evaluation board?
#elif defined(USE_LPCXPRESSO_54S018M)
   //Select RMII interface mode
   SYSCON->ETHPHYSEL |= SYSCON_ETHPHYSEL_PHY_SEL_MASK;

   //Enable IOCON clock
   CLOCK_EnableClock(kCLOCK_Iocon);

   //Enable GPIO clocks
   CLOCK_EnableClock(kCLOCK_Gpio0);
   CLOCK_EnableClock(kCLOCK_Gpio1);
   CLOCK_EnableClock(kCLOCK_Gpio2);
   CLOCK_EnableClock(kCLOCK_Gpio4);

   //Configure ENET_TXD1 (P0_17)
   IOCON_PinMuxSet(IOCON, 0, 17, IOCON_FUNC7 | IOCON_MODE_INACT |
      IOCON_DIGITAL_EN | IOCON_INPFILT_OFF);

   //Configure ENET_MDC (P1_16)
   IOCON_PinMuxSet(IOCON, 1, 16, IOCON_FUNC1 | IOCON_MODE_INACT |
      IOCON_DIGITAL_EN | IOCON_INPFILT_OFF);

   //Configure ENET_MDIO (P1_23)
   IOCON_PinMuxSet(IOCON, 1, 23, IOCON_FUNC4 | IOCON_MODE_PULLUP |
      IOCON_DIGITAL_EN | IOCON_INPFILT_OFF);

   //Configure ENET_TXD0 (P4_8)
   IOCON_PinMuxSet(IOCON, 4, 8, IOCON_FUNC1 | IOCON_MODE_INACT |
      IOCON_DIGITAL_EN | IOCON_INPFILT_OFF);

   //Configure ENET_RX_DV (P4_10)
   IOCON_PinMuxSet(IOCON, 4, 10, IOCON_FUNC1 | IOCON_MODE_INACT |
      IOCON_DIGITAL_EN | IOCON_INPFILT_OFF);

   //Configure ENET_RXD0 (P4_11)
   IOCON_PinMuxSet(IOCON, 4, 11, IOCON_FUNC1 | IOCON_MODE_INACT |
      IOCON_DIGITAL_EN | IOCON_INPFILT_OFF);

   //Configure ENET_RXD1 (P4_12)
   IOCON_PinMuxSet(IOCON, 4, 12, IOCON_FUNC1 | IOCON_MODE_INACT |
      IOCON_DIGITAL_EN | IOCON_INPFILT_OFF);

   //Configure ENET_TX_EN (P4_13)
   IOCON_PinMuxSet(IOCON, 4, 13, IOCON_FUNC1 | IOCON_MODE_INACT |
      IOCON_DIGITAL_EN | IOCON_INPFILT_OFF);

   //Configure ENET_RX_CLK (P4_14)
   IOCON_PinMuxSet(IOCON, 4, 14, IOCON_FUNC1 | IOCON_MODE_INACT |
      IOCON_DIGITAL_EN | IOCON_INPFILT_OFF);

   //Configure ENET_RST (P2_26) as an output
   pinConfig.pinDirection = kGPIO_DigitalOutput;
   pinConfig.outputLogic = 0;
   GPIO_PinInit(GPIO, 2, 26, &pinConfig);

   //Reset PHY transceiver (hard reset)
   GPIO_PinWrite(GPIO, 2, 26, 0);
   sleep(10);
   GPIO_PinWrite(GPIO, 2, 26, 1);
   sleep(10);
#endif
}

#endif


/**
 * @brief Initialize DMA descriptor lists
 * @param[in] interface Underlying network interface
 **/

void lpc54xxxEthInitDmaDesc(NetInterface *interface)
{
   uint_t i;

   //Initialize TX DMA descriptor list
   for(i = 0; i < LPC54XXX_ETH_TX_BUFFER_COUNT; i++)
   {
      //The descriptor is initially owned by the application
      txDmaDesc[i].tdes0 = 0;
      txDmaDesc[i].tdes1 = 0;
      txDmaDesc[i].tdes2 = 0;
      txDmaDesc[i].tdes3 = 0;
   }

   //Initialize TX descriptor index
   txIndex = 0;

   //Initialize RX DMA descriptor list
   for(i = 0; i < LPC54XXX_ETH_RX_BUFFER_COUNT; i++)
   {
      //The descriptor is initially owned by the DMA
      rxDmaDesc[i].rdes0 = (uint32_t) rxBuffer[i];
      rxDmaDesc[i].rdes1 = 0;
      rxDmaDesc[i].rdes2 = 0;
      rxDmaDesc[i].rdes3 = ENET_RDES3_OWN | ENET_RDES3_IOC | ENET_RDES3_BUF1V;
   }

   //Initialize RX descriptor index
   rxIndex = 0;

   //Start location of the TX descriptor list
   ENET->DMA_CH[0].DMA_CHX_TXDESC_LIST_ADDR = (uint32_t) &txDmaDesc[0];
   //Length of the transmit descriptor ring
   ENET->DMA_CH[0].DMA_CHX_TXDESC_RING_LENGTH = LPC54XXX_ETH_TX_BUFFER_COUNT - 1;

   //Start location of the RX descriptor list
   ENET->DMA_CH[0].DMA_CHX_RXDESC_LIST_ADDR = (uint32_t) &rxDmaDesc[0];
   //Length of the receive descriptor ring
   ENET->DMA_CH[0].DMA_CHX_RXDESC_RING_LENGTH = LPC54XXX_ETH_RX_BUFFER_COUNT - 1;
}


/**
 * @brief LPC54xxx Ethernet MAC timer handler
 *
 * This routine is periodically called by the TCP/IP stack to handle periodic
 * operations such as polling the link state
 *
 * @param[in] interface Underlying network interface
 **/

void lpc54xxxEthTick(NetInterface *interface)
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

void lpc54xxxEthEnableIrq(NetInterface *interface)
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

void lpc54xxxEthDisableIrq(NetInterface *interface)
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
 * @brief LPC54xxx Ethernet MAC interrupt service routine
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
   status = ENET->DMA_CH[0].DMA_CHX_STAT;

   //Packet transmitted?
   if((status & ENET_DMA_CH_DMA_CHX_STAT_TI_MASK) != 0)
   {
      //Clear TI interrupt flag
      ENET->DMA_CH[0].DMA_CHX_STAT = ENET_DMA_CH_DMA_CHX_STAT_TI_MASK;

      //Check whether the TX buffer is available for writing
      if((txDmaDesc[txIndex].tdes3 & ENET_TDES3_OWN) == 0)
      {
         //Notify the TCP/IP stack that the transmitter is ready to send
         flag |= osSetEventFromIsr(&nicDriverInterface->nicTxEvent);
      }
   }

   //Packet received?
   if((status & ENET_DMA_CH_DMA_CHX_STAT_RI_MASK) != 0)
   {
      //Disable RIE interrupt
      ENET->DMA_CH[0].DMA_CHX_INT_EN &= ~ENET_DMA_CH_DMA_CHX_INT_EN_RIE_MASK;

      //Set event flag
      nicDriverInterface->nicEvent = TRUE;
      //Notify the TCP/IP stack of the event
      flag |= osSetEventFromIsr(&netEvent);
   }

   //Clear NIS interrupt flag
   ENET->DMA_CH[0].DMA_CHX_STAT = ENET_DMA_CH_DMA_CHX_STAT_NIS_MASK;

   //Interrupt service routine epilogue
   osExitIsr(flag);
}


/**
 * @brief LPC54xxx Ethernet MAC event handler
 * @param[in] interface Underlying network interface
 **/

void lpc54xxxEthEventHandler(NetInterface *interface)
{
   error_t error;

   //Packet received?
   if((ENET->DMA_CH[0].DMA_CHX_STAT & ENET_DMA_CH_DMA_CHX_STAT_RI_MASK) != 0)
   {
      //Clear interrupt flag
      ENET->DMA_CH[0].DMA_CHX_STAT = ENET_DMA_CH_DMA_CHX_STAT_RI_MASK;

      //Process all pending packets
      do
      {
         //Read incoming packet
         error = lpc54xxxEthReceivePacket(interface);

         //No more data in the receive buffer?
      } while(error != ERROR_BUFFER_EMPTY);
   }

   //Re-enable DMA interrupts
   ENET->DMA_CH[0].DMA_CHX_INT_EN = ENET_DMA_CH_DMA_CHX_INT_EN_NIE_MASK |
      ENET_DMA_CH_DMA_CHX_INT_EN_RIE_MASK | ENET_DMA_CH_DMA_CHX_INT_EN_TIE_MASK;
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

error_t lpc54xxxEthSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary)
{
   size_t length;

   //Retrieve the length of the packet
   length = netBufferGetLength(buffer) - offset;

   //Check the frame length
   if(length > LPC54XXX_ETH_TX_BUFFER_SIZE)
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
      //Report an error
      return ERROR_INVALID_LENGTH;
   }

   //Make sure the current buffer is available for writing
   if((txDmaDesc[txIndex].tdes3 & ENET_TDES3_OWN) != 0)
   {
      return ERROR_FAILURE;
   }

   //Copy user data to the transmit buffer
   netBufferRead(txBuffer[txIndex], buffer, offset, length);

   //Set the start address of the buffer
   txDmaDesc[txIndex].tdes0 = (uint32_t) txBuffer[txIndex];
   //Write the number of bytes to send
   txDmaDesc[txIndex].tdes2 = ENET_TDES2_IOC | (length & ENET_TDES2_B1L);
   //Give the ownership of the descriptor to the DMA
   txDmaDesc[txIndex].tdes3 = ENET_TDES3_OWN | ENET_TDES3_FD | ENET_TDES3_LD;

   //Clear TBU flag to resume processing
   ENET->DMA_CH[0].DMA_CHX_STAT = ENET_DMA_CH_DMA_CHX_STAT_TBU_MASK;
   //Instruct the DMA to poll the transmit descriptor list
   ENET->DMA_CH[0].DMA_CHX_TXDESC_TAIL_PTR = 0;

   //Increment index and wrap around if necessary
   if(++txIndex >= LPC54XXX_ETH_TX_BUFFER_COUNT)
   {
      txIndex = 0;
   }

   //Check whether the next buffer is available for writing
   if((txDmaDesc[txIndex].tdes3 & ENET_TDES3_OWN) == 0)
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

error_t lpc54xxxEthReceivePacket(NetInterface *interface)
{
   error_t error;
   size_t n;
   NetRxAncillary ancillary;

   //Current buffer available for reading?
   if((rxDmaDesc[rxIndex].rdes3 & ENET_RDES3_OWN) == 0)
   {
      //FD and LD flags should be set
      if((rxDmaDesc[rxIndex].rdes3 & ENET_RDES3_FD) != 0 &&
         (rxDmaDesc[rxIndex].rdes3 & ENET_RDES3_LD) != 0)
      {
         //Make sure no error occurred
         if((rxDmaDesc[rxIndex].rdes3 & ENET_RDES3_ES) == 0)
         {
            //Retrieve the length of the frame
            n = rxDmaDesc[rxIndex].rdes3 & ENET_RDES3_PL;
            //Limit the number of data to read
            n = MIN(n, LPC54XXX_ETH_RX_BUFFER_SIZE);

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

      //Set the start address of the buffer
      rxDmaDesc[rxIndex].rdes0 = (uint32_t) rxBuffer[rxIndex];
      //Give the ownership of the descriptor back to the DMA
      rxDmaDesc[rxIndex].rdes3 = ENET_RDES3_OWN | ENET_RDES3_IOC | ENET_RDES3_BUF1V;

      //Increment index and wrap around if necessary
      if(++rxIndex >= LPC54XXX_ETH_RX_BUFFER_COUNT)
      {
         rxIndex = 0;
      }
   }
   else
   {
      //No more data in the receive buffer
      error = ERROR_BUFFER_EMPTY;
   }

   //Clear RBU flag to resume processing
   ENET->DMA_CH[0].DMA_CHX_STAT = ENET_DMA_CH_DMA_CHX_STAT_RBU_MASK;
   //Instruct the DMA to poll the receive descriptor list
   ENET->DMA_CH[0].DMA_CHX_RXDESC_TAIL_PTR = 0;

   //Return status code
   return error;
}


/**
 * @brief Configure MAC address filtering
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t lpc54xxxEthUpdateMacAddrFilter(NetInterface *interface)
{
   uint_t i;
   bool_t acceptMulticast;

   //Debug message
   TRACE_DEBUG("Updating MAC filter...\r\n");

   //Set the MAC address of the station
   ENET->MAC_ADDR_LOW = interface->macAddr.w[0] | (interface->macAddr.w[1] << 16);
   ENET->MAC_ADDR_HIGH = interface->macAddr.w[2];

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
      ENET->MAC_FRAME_FILTER |= ENET_MAC_FRAME_FILTER_PM_MASK;
   }
   else
   {
      ENET->MAC_FRAME_FILTER &= ~ENET_MAC_FRAME_FILTER_PM_MASK;
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Adjust MAC configuration parameters for proper operation
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t lpc54xxxEthUpdateMacConfig(NetInterface *interface)
{
   uint32_t config;

   //Read current MAC configuration
   config = ENET->MAC_CONFIG;

   //10BASE-T or 100BASE-TX operation mode?
   if(interface->linkSpeed == NIC_LINK_SPEED_100MBPS)
   {
      config |= ENET_MAC_CONFIG_FES_MASK;
   }
   else
   {
      config &= ~ENET_MAC_CONFIG_FES_MASK;
   }

   //Half-duplex or full-duplex mode?
   if(interface->duplexMode == NIC_FULL_DUPLEX_MODE)
   {
      config |= ENET_MAC_CONFIG_DM_MASK;
   }
   else
   {
      config &= ~ENET_MAC_CONFIG_DM_MASK;
   }

   //Update MAC configuration register
   ENET->MAC_CONFIG = config;

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

void lpc54xxxEthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data)
{
   uint32_t temp;

   //Valid opcode?
   if(opcode == SMI_OPCODE_WRITE)
   {
      //Take care not to alter MDC clock configuration
      temp = ENET->MAC_MDIO_ADDR & ENET_MAC_MDIO_ADDR_CR_MASK;
      //Set up a write operation
      temp |= ENET_MAC_MDIO_ADDR_MOC(1) | ENET_MAC_MDIO_ADDR_MB_MASK;
      //PHY address
      temp |= ENET_MAC_MDIO_ADDR_PA(phyAddr);
      //Register address
      temp |= ENET_MAC_MDIO_ADDR_RDA(regAddr);

      //Data to be written in the PHY register
      ENET->MAC_MDIO_DATA = data & ENET_MAC_MDIO_DATA_MD_MASK;

      //Start a write operation
      ENET->MAC_MDIO_ADDR = temp;
      //Wait for the write to complete
      while((ENET->MAC_MDIO_ADDR & ENET_MAC_MDIO_ADDR_MB_MASK) != 0)
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

uint16_t lpc54xxxEthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr)
{
   uint16_t data;
   uint32_t temp;

   //Valid opcode?
   if(opcode == SMI_OPCODE_READ)
   {
      //Take care not to alter MDC clock configuration
      temp = ENET->MAC_MDIO_ADDR & ENET_MAC_MDIO_ADDR_CR_MASK;
      //Set up a read operation
      temp |= ENET_MAC_MDIO_ADDR_MOC(3) | ENET_MAC_MDIO_ADDR_MB_MASK;
      //PHY address
      temp |= ENET_MAC_MDIO_ADDR_PA(phyAddr);
      //Register address
      temp |= ENET_MAC_MDIO_ADDR_RDA(regAddr);

      //Start a read operation
      ENET->MAC_MDIO_ADDR = temp;
      //Wait for the read to complete
      while((ENET->MAC_MDIO_ADDR & ENET_MAC_MDIO_ADDR_MB_MASK) != 0)
      {
      }

      //Get register value
      data = ENET->MAC_MDIO_DATA & ENET_MAC_MDIO_DATA_MD_MASK;
   }
   else
   {
      //The MAC peripheral only supports standard Clause 22 opcodes
      data = 0;
   }

   //Return the value of the PHY register
   return data;
}
