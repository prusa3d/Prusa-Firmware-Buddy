/**
 * @file stm32mp1xx_eth_driver.c
 * @brief STM32MP1 Gigabit Ethernet MAC driver
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
#include "stm32mp1xx.h"
#include "stm32mp1xx_hal.h"
#include "core/net.h"
#include "drivers/mac/stm32mp1xx_eth_driver.h"
#include "debug.h"

//Underlying network interface
static NetInterface *nicDriverInterface;

//IAR EWARM compiler?
#if defined(__ICCARM__)

//Transmit buffer
#pragma data_alignment = 4
static uint8_t txBuffer[STM32MP1XX_ETH_TX_BUFFER_COUNT][STM32MP1XX_ETH_TX_BUFFER_SIZE];
//Receive buffer
#pragma data_alignment = 4
static uint8_t rxBuffer[STM32MP1XX_ETH_RX_BUFFER_COUNT][STM32MP1XX_ETH_RX_BUFFER_SIZE];
//Transmit DMA descriptors
#pragma data_alignment = 8
static Stm32mp1xxTxDmaDesc txDmaDesc[STM32MP1XX_ETH_TX_BUFFER_COUNT];
//Receive DMA descriptors
#pragma data_alignment = 8
static Stm32mp1xxRxDmaDesc rxDmaDesc[STM32MP1XX_ETH_RX_BUFFER_COUNT];

//Keil MDK-ARM or GCC compiler?
#else

//Transmit buffer
static uint8_t txBuffer[STM32MP1XX_ETH_TX_BUFFER_COUNT][STM32MP1XX_ETH_TX_BUFFER_SIZE]
   __attribute__((aligned(4)));
//Receive buffer
static uint8_t rxBuffer[STM32MP1XX_ETH_RX_BUFFER_COUNT][STM32MP1XX_ETH_RX_BUFFER_SIZE]
   __attribute__((aligned(4)));
//Transmit DMA descriptors
static Stm32mp1xxTxDmaDesc txDmaDesc[STM32MP1XX_ETH_TX_BUFFER_COUNT]
   __attribute__((aligned(8)));
//Receive DMA descriptors
static Stm32mp1xxRxDmaDesc rxDmaDesc[STM32MP1XX_ETH_RX_BUFFER_COUNT]
   __attribute__((aligned(8)));

#endif

//Current transmit descriptor
static uint_t txIndex;
//Current receive descriptor
static uint_t rxIndex;


/**
 * @brief STM32MP1 Ethernet MAC driver
 **/

const NicDriver stm32mp1xxEthDriver =
{
   NIC_TYPE_ETHERNET,
   ETH_MTU,
   stm32mp1xxEthInit,
   stm32mp1xxEthTick,
   stm32mp1xxEthEnableIrq,
   stm32mp1xxEthDisableIrq,
   stm32mp1xxEthEventHandler,
   stm32mp1xxEthSendPacket,
   stm32mp1xxEthUpdateMacAddrFilter,
   stm32mp1xxEthUpdateMacConfig,
   stm32mp1xxEthWritePhyReg,
   stm32mp1xxEthReadPhyReg,
   TRUE,
   TRUE,
   TRUE,
   FALSE
};


/**
 * @brief STM32MP1 Ethernet MAC initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t stm32mp1xxEthInit(NetInterface *interface)
{
   error_t error;

   //Debug message
   TRACE_INFO("Initializing STM32MP1 Ethernet MAC...\r\n");

   //Save underlying network interface
   nicDriverInterface = interface;

   //GPIO configuration
   stm32mp1xxEthInitGpio(interface);

   //Enable Ethernet MAC clock
   __HAL_RCC_ETH1MAC_CLK_ENABLE();
   __HAL_RCC_ETH1TX_CLK_ENABLE();
   __HAL_RCC_ETH1RX_CLK_ENABLE();

   //Reset Ethernet MAC peripheral
   __HAL_RCC_ETH1MAC_FORCE_RESET();
   __HAL_RCC_ETH1MAC_RELEASE_RESET();

   //Perform a software reset
   ETH->DMAMR |= ETH_DMAMR_SWR;
   //Wait for the reset to complete
   while((ETH->DMAMR & ETH_DMAMR_SWR) != 0)
   {
   }

   //Adjust MDC clock range depending on HCLK frequency
   ETH->MACMDIOAR = ETH_MACMDIOAR_CR_Val(5);

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
   ETH->MACCR = ETH_MACCR_DO;

   //Set the MAC address of the station
   ETH->MACA0LR = interface->macAddr.w[0] | (interface->macAddr.w[1] << 16);
   ETH->MACA0HR = interface->macAddr.w[2];

   //The MAC supports 3 additional addresses for unicast perfect filtering
   ETH->MACA1LR = 0;
   ETH->MACA1HR = 0;
   ETH->MACA2LR = 0;
   ETH->MACA2HR = 0;
   ETH->MACA3LR = 0;
   ETH->MACA3HR = 0;

   //Initialize hash table
   ETH->MACHT0R = 0;
   ETH->MACHT1R = 0;

   //Configure the receive filter
   ETH->MACPFR = ETH_MACPFR_HPF | ETH_MACPFR_HMC;

   //Disable flow control
   ETH->MACQ0TXFCR = 0;
   ETH->MACRXFCR = 0;

   //Enable the first RX queue
   ETH->MACRXQC0R = ETH_MACRXQC0R_RXQ0EN_Val(1);

   //Configure DMA operating mode
   ETH->DMAMR = ETH_DMAMR_INTM_Val(0) | ETH_DMAMR_PR_Val(0);
   //Configure system bus mode
   ETH->DMASBMR |= ETH_DMASBMR_AAL;

   //The DMA takes the descriptor table as contiguous
   ETH->DMAC0CR = ETH_DMAC0CR_DSL_Val(0);
   //Configure TX features
   ETH->DMAC0TXCR = ETH_DMAC0TXCR_TXPBL_Val(1);

   //Configure RX features
   ETH->DMAC0RXCR = ETH_DMAC0RXCR_RXPBL_Val(1) |
      ETH_DMAC0RXCR_RBSZ_Val(STM32MP1XX_ETH_RX_BUFFER_SIZE);

   //Enable store and forward mode for transmission
   ETH->MTLTXQ0OMR = ETH_MTLTXQ0OMR_TQS_Val(7) | ETH_MTLTXQ0OMR_TXQEN_Val(2) |
      ETH_MTLTXQ0OMR_TSF;

   //Enable store and forward mode for reception
   ETH->MTLRXQ0OMR = ETH_MTLRXQ0OMR_RQS_Val(7) | ETH_MTLRXQ0OMR_RSF;

   //Initialize DMA descriptor lists
   stm32mp1xxEthInitDmaDesc(interface);

   //Prevent interrupts from being generated when the transmit statistic
   //counters reach half their maximum value
   ETH->MMCTXIMR = ETH_MMCTXIMR_TXLPITRCIM | ETH_MMCTXIMR_TXLPIUSCIM |
      ETH_MMCTXIMR_TXGPKTIM | ETH_MMCTXIMR_TXMCOLGPIM | ETH_MMCTXIMR_TXSCOLGPIM;

   //Prevent interrupts from being generated when the receive statistic
   //counters reach half their maximum value
   ETH->MMCRXIMR = ETH_MMCRXIMR_RXLPITRCIM | ETH_MMCRXIMR_RXLPIUSCIM |
      ETH_MMCRXIMR_RXUCGPIM | ETH_MMCRXIMR_RXALGNERPIM | ETH_MMCRXIMR_RXCRCERPIM;

   //Disable MAC interrupts
   ETH->MACIER = 0;
   //Enable the desired DMA interrupts
   ETH->DMAC0IER = ETH_DMAC0IER_NIE | ETH_DMAC0IER_RIE | ETH_DMAC0IER_TIE;

   //Set priority grouping (4 bits for pre-emption priority, no bits for subpriority)
   NVIC_SetPriorityGrouping(STM32MP1XX_ETH_IRQ_PRIORITY_GROUPING);

   //Configure Ethernet interrupt priority
   NVIC_SetPriority(ETH1_IRQn, NVIC_EncodePriority(STM32MP1XX_ETH_IRQ_PRIORITY_GROUPING,
      STM32MP1XX_ETH_IRQ_GROUP_PRIORITY, STM32MP1XX_ETH_IRQ_SUB_PRIORITY));

   //Enable MAC transmission and reception
   ETH->MACCR |= ETH_MACCR_TE | ETH_MACCR_RE;

   //Enable DMA transmission and reception
   ETH->DMAC0TXCR |= ETH_DMAC0TXCR_ST;
   ETH->DMAC0RXCR |= ETH_DMAC0RXCR_SR;

   //Accept any packets from the upper layer
   osSetEvent(&interface->nicTxEvent);

   //Successful initialization
   return NO_ERROR;
}


//STM32MP157A-EV1 or STM32MP157C-DK2 evaluation board?
#if defined(USE_STM32MP15XX_EVAL) || defined(USE_STM32MP15XX_DISCO)

/**
 * @brief GPIO configuration
 * @param[in] interface Underlying network interface
 **/

void stm32mp1xxEthInitGpio(NetInterface *interface)
{
   GPIO_InitTypeDef GPIO_InitStructure;

//STM32MP157A-EV1 evaluation board?
#if defined(USE_STM32MP15XX_EVAL)
   //Enable SYSCFG clock
   __HAL_RCC_SYSCFG_CLK_ENABLE();

   //Enable GPIO clocks
   __HAL_RCC_GPIOA_CLK_ENABLE();
   __HAL_RCC_GPIOB_CLK_ENABLE();
   __HAL_RCC_GPIOC_CLK_ENABLE();
   //__HAL_RCC_GPIOD_CLK_ENABLE();
   __HAL_RCC_GPIOE_CLK_ENABLE();
   __HAL_RCC_GPIOG_CLK_ENABLE();

   //Select RGMII interface mode
   HAL_SYSCFG_ETHInterfaceSelect(SYSCFG_ETH_RGMII);

   //Configure RGMII pins
   GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
   GPIO_InitStructure.Pull = GPIO_NOPULL;
   GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
   GPIO_InitStructure.Alternate = GPIO_AF11_ETH;

   //Configure ETH1_RGMII_RX_CLK (PA1), ETH1_MDIO (PA2) and
   //ETH1_RGMII_RX_CTL (PA7)
   GPIO_InitStructure.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_7;
   HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);

   //Configure ETH1_RGMII_RXD2 (PB0), ETH1_RGMII_RXD3 (PB1) and
   //ETH1_RGMII_TX_CTL (PB11)
   GPIO_InitStructure.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_11;
   HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);

   //Configure ETH1_MDC (PC1), ETH1_RGMII_TXD2 (PC2), ETH1_RGMII_RXD0 (PC4) and
   //ETH1_RGMII_RXD1 (PC5)
   GPIO_InitStructure.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_5;
   HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);

   //Configure ETH1_RGMII_TXD3 (PE2)
   GPIO_InitStructure.Pin = GPIO_PIN_2;
   HAL_GPIO_Init(GPIOE, &GPIO_InitStructure);

   //Configure ETH1_RGMII_GTX_CLK (PG4), ETH1_RGMII_CLK125 (PG5),
   //ETH1_RGMII_TXD0 (PG13) and ETH1_RMII_TXD1 (PG14)
   GPIO_InitStructure.Pin = GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_13 | GPIO_PIN_14;
   HAL_GPIO_Init(GPIOG, &GPIO_InitStructure);

   //Configure PHY_RST (PD10)
   //GPIO_InitStructure.Pin = GPIO_PIN_10;
   //GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
   //GPIO_InitStructure.Pull = GPIO_NOPULL;
   //GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_LOW;
   //HAL_GPIO_Init(GPIOD, &GPIO_InitStructure);

   //Reset PHY transceiver
   //HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10, GPIO_PIN_RESET);
   //sleep(10);
   //HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10, GPIO_PIN_SET);
   //sleep(10);
//STM32MP157C-DK2 evaluation board?
#elif defined(USE_STM32MP15XX_DISCO)
   //Enable SYSCFG clock
   __HAL_RCC_SYSCFG_CLK_ENABLE();

   //Enable GPIO clocks
   __HAL_RCC_GPIOA_CLK_ENABLE();
   __HAL_RCC_GPIOB_CLK_ENABLE();
   __HAL_RCC_GPIOC_CLK_ENABLE();
   __HAL_RCC_GPIOE_CLK_ENABLE();
   __HAL_RCC_GPIOG_CLK_ENABLE();

   //Select RGMII interface mode
   HAL_SYSCFG_ETHInterfaceSelect(SYSCFG_ETH_RGMII);

   //Configure RGMII pins
   GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
   GPIO_InitStructure.Pull = GPIO_NOPULL;
   GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
   GPIO_InitStructure.Alternate = GPIO_AF11_ETH;

   //Configure ETH1_RGMII_RX_CLK (PA1), ETH1_MDIO (PA2) and
   //ETH1_RGMII_RX_CTL (PA7)
   GPIO_InitStructure.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_7;
   HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);

   //Configure ETH1_RGMII_RXD2 (PB0), ETH1_RGMII_RXD3 (PB1) and
   //ETH1_RGMII_TX_CTL (PB11)
   GPIO_InitStructure.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_11;
   HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);

   //Configure ETH1_MDC (PC1), ETH1_RGMII_TXD2 (PC2), ETH1_RGMII_RXD0 (PC4) and
   //ETH1_RGMII_RXD1 (PC5)
   GPIO_InitStructure.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_4 | GPIO_PIN_5;
   HAL_GPIO_Init(GPIOC, &GPIO_InitStructure);

   //Configure ETH1_RGMII_TXD3 (PE2)
   GPIO_InitStructure.Pin = GPIO_PIN_2;
   HAL_GPIO_Init(GPIOE, &GPIO_InitStructure);

   //Configure ETH1_RGMII_GTX_CLK (PG4), ETH1_RGMII_CLK125 (PG5),
   //ETH1_RGMII_TXD0 (PG13) and ETH1_RMII_TXD1 (PG14)
   GPIO_InitStructure.Pin = GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_13 | GPIO_PIN_14;
   HAL_GPIO_Init(GPIOG, &GPIO_InitStructure);

   //Configure PHY_RST (PG0)
   GPIO_InitStructure.Pin = GPIO_PIN_0;
   GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
   GPIO_InitStructure.Pull = GPIO_NOPULL;
   GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_LOW;
   HAL_GPIO_Init(GPIOG, &GPIO_InitStructure);

   //Reset PHY transceiver
   HAL_GPIO_WritePin(GPIOG, GPIO_PIN_0, GPIO_PIN_RESET);
   sleep(10);
   HAL_GPIO_WritePin(GPIOG, GPIO_PIN_0, GPIO_PIN_SET);
   sleep(10);
#endif
}

#endif


/**
 * @brief Initialize DMA descriptor lists
 * @param[in] interface Underlying network interface
 **/

void stm32mp1xxEthInitDmaDesc(NetInterface *interface)
{
   uint_t i;

   //Initialize TX DMA descriptor list
   for(i = 0; i < STM32MP1XX_ETH_TX_BUFFER_COUNT; i++)
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
   for(i = 0; i < STM32MP1XX_ETH_RX_BUFFER_COUNT; i++)
   {
      //The descriptor is initially owned by the DMA
      rxDmaDesc[i].rdes0 = (uint32_t) rxBuffer[i];
      rxDmaDesc[i].rdes1 = 0;
      rxDmaDesc[i].rdes2 = 0;
      rxDmaDesc[i].rdes3 = ETH_RDES3_OWN | ETH_RDES3_IOC | ETH_RDES3_BUF1V;
   }

   //Initialize RX descriptor index
   rxIndex = 0;

   //Start location of the TX descriptor list
   ETH->DMAC0TXDLAR = (uint32_t) &txDmaDesc[0];
   //Length of the transmit descriptor ring
   ETH->DMAC0TXRLR = STM32MP1XX_ETH_TX_BUFFER_COUNT - 1;

   //Start location of the RX descriptor list
   ETH->DMAC0RXDLAR = (uint32_t) &rxDmaDesc[0];
   //Length of the receive descriptor ring
   ETH->DMAC0RXRLR = STM32MP1XX_ETH_RX_BUFFER_COUNT - 1;
}


/**
 * @brief STM32MP1 Ethernet MAC timer handler
 *
 * This routine is periodically called by the TCP/IP stack to handle periodic
 * operations such as polling the link state
 *
 * @param[in] interface Underlying network interface
 **/

void stm32mp1xxEthTick(NetInterface *interface)
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

void stm32mp1xxEthEnableIrq(NetInterface *interface)
{
   //Enable Ethernet MAC interrupts
   NVIC_EnableIRQ(ETH1_IRQn);

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

void stm32mp1xxEthDisableIrq(NetInterface *interface)
{
   //Disable Ethernet MAC interrupts
   NVIC_DisableIRQ(ETH1_IRQn);

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
 * @brief STM32MP1 Ethernet MAC interrupt service routine
 **/

void ETH1_IRQHandler(void)
{
   bool_t flag;
   uint32_t status;

   //Interrupt service routine prologue
   osEnterIsr();

   //This flag will be set if a higher priority task must be woken
   flag = FALSE;

   //Read DMA status register
   status = ETH->DMAC0SR;

   //Packet transmitted?
   if((status & ETH_DMAC0SR_TI) != 0)
   {
      //Clear TI interrupt flag
      ETH->DMAC0SR = ETH_DMAC0SR_TI;

      //Check whether the TX buffer is available for writing
      if((txDmaDesc[txIndex].tdes3 & ETH_TDES3_OWN) == 0)
      {
         //Notify the TCP/IP stack that the transmitter is ready to send
         flag |= osSetEventFromIsr(&nicDriverInterface->nicTxEvent);
      }
   }

   //Packet received?
   if((status & ETH_DMAC0SR_RI) != 0)
   {
      //Disable RIE interrupt
      ETH->DMAC0IER &= ~ETH_DMAC0IER_RIE;

      //Set event flag
      nicDriverInterface->nicEvent = TRUE;
      //Notify the TCP/IP stack of the event
      flag |= osSetEventFromIsr(&netEvent);
   }

   //Clear NIS interrupt flag
   ETH->DMAC0SR = ETH_DMAC0SR_NIS;

   //Interrupt service routine epilogue
   osExitIsr(flag);
}


/**
 * @brief STM32MP1 Ethernet MAC event handler
 * @param[in] interface Underlying network interface
 **/

void stm32mp1xxEthEventHandler(NetInterface *interface)
{
   error_t error;

   //Packet received?
   if((ETH->DMAC0SR & ETH_DMAC0SR_RI) != 0)
   {
      //Clear interrupt flag
      ETH->DMAC0SR = ETH_DMAC0SR_RI;

      //Process all pending packets
      do
      {
         //Read incoming packet
         error = stm32mp1xxEthReceivePacket(interface);

         //No more data in the receive buffer?
      } while(error != ERROR_BUFFER_EMPTY);
   }

   //Re-enable DMA interrupts
   ETH->DMAC0IER = ETH_DMAC0IER_NIE | ETH_DMAC0IER_RIE | ETH_DMAC0IER_TIE;
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

error_t stm32mp1xxEthSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary)
{
   size_t length;

   //Retrieve the length of the packet
   length = netBufferGetLength(buffer) - offset;

   //Check the frame length
   if(length > STM32MP1XX_ETH_TX_BUFFER_SIZE)
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
      //Report an error
      return ERROR_INVALID_LENGTH;
   }

   //Make sure the current buffer is available for writing
   if((txDmaDesc[txIndex].tdes3 & ETH_TDES3_OWN) != 0)
   {
      return ERROR_FAILURE;
   }

   //Copy user data to the transmit buffer
   netBufferRead(txBuffer[txIndex], buffer, offset, length);

   //Set the start address of the buffer
   txDmaDesc[txIndex].tdes0 = (uint32_t) txBuffer[txIndex];
   //Write the number of bytes to send
   txDmaDesc[txIndex].tdes2 = ETH_TDES2_IOC | (length & ETH_TDES2_B1L);
   //Give the ownership of the descriptor to the DMA
   txDmaDesc[txIndex].tdes3 = ETH_TDES3_OWN | ETH_TDES3_FD | ETH_TDES3_LD;

   //Data synchronization barrier
   __DSB();

   //Clear TBU flag to resume processing
   ETH->DMAC0SR = ETH_DMAC0SR_TBU;
   //Instruct the DMA to poll the transmit descriptor list
   ETH->DMAC0TXDTPR = 0;

   //Increment index and wrap around if necessary
   if(++txIndex >= STM32MP1XX_ETH_TX_BUFFER_COUNT)
   {
      txIndex = 0;
   }

   //Check whether the next buffer is available for writing
   if((txDmaDesc[txIndex].tdes3 & ETH_TDES3_OWN) == 0)
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

error_t stm32mp1xxEthReceivePacket(NetInterface *interface)
{
   error_t error;
   size_t n;
   NetRxAncillary ancillary;

   //Current buffer available for reading?
   if((rxDmaDesc[rxIndex].rdes3 & ETH_RDES3_OWN) == 0)
   {
      //FD and LD flags should be set
      if((rxDmaDesc[rxIndex].rdes3 & ETH_RDES3_FD) != 0 &&
         (rxDmaDesc[rxIndex].rdes3 & ETH_RDES3_LD) != 0)
      {
         //Make sure no error occurred
         if((rxDmaDesc[rxIndex].rdes3 & ETH_RDES3_ES) == 0)
         {
            //Retrieve the length of the frame
            n = rxDmaDesc[rxIndex].rdes3 & ETH_RDES3_PL;
            //Limit the number of data to read
            n = MIN(n, STM32MP1XX_ETH_RX_BUFFER_SIZE);

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
      rxDmaDesc[rxIndex].rdes3 = ETH_RDES3_OWN | ETH_RDES3_IOC | ETH_RDES3_BUF1V;

      //Increment index and wrap around if necessary
      if(++rxIndex >= STM32MP1XX_ETH_RX_BUFFER_COUNT)
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
   ETH->DMAC0SR = ETH_DMAC0SR_RBU;
   //Instruct the DMA to poll the receive descriptor list
   ETH->DMAC0RXDTPR = 0;

   //Return status code
   return error;
}


/**
 * @brief Configure MAC address filtering
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t stm32mp1xxEthUpdateMacAddrFilter(NetInterface *interface)
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
   ETH->MACA0LR = interface->macAddr.w[0] | (interface->macAddr.w[1] << 16);
   ETH->MACA0HR = interface->macAddr.w[2];

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
            crc = stm32mp1xxEthCalcCrc(&entry->addr, sizeof(MacAddr));

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
      ETH->MACA1LR = unicastMacAddr[0].w[0] | (unicastMacAddr[0].w[1] << 16);
      ETH->MACA1HR = unicastMacAddr[0].w[2] | ETH_MACA1HR_AE;
   }
   else
   {
      //When the AE bit is cleared, the entry is ignored
      ETH->MACA1LR = 0;
      ETH->MACA1HR = 0;
   }

   //Configure the second unicast address filter
   if(j >= 2)
   {
      //When the AE bit is set, the entry is used for perfect filtering
      ETH->MACA2LR = unicastMacAddr[1].w[0] | (unicastMacAddr[1].w[1] << 16);
      ETH->MACA2HR = unicastMacAddr[1].w[2] | ETH_MACA2HR_AE;
   }
   else
   {
      //When the AE bit is cleared, the entry is ignored
      ETH->MACA2LR = 0;
      ETH->MACA2HR = 0;
   }

   //Configure the third unicast address filter
   if(j >= 3)
   {
      //When the AE bit is set, the entry is used for perfect filtering
      ETH->MACA3LR = unicastMacAddr[2].w[0] | (unicastMacAddr[2].w[1] << 16);
      ETH->MACA3HR = unicastMacAddr[2].w[2] | ETH_MACA3HR_AE;
   }
   else
   {
      //When the AE bit is cleared, the entry is ignored
      ETH->MACA3LR = 0;
      ETH->MACA3HR = 0;
   }

   //Configure the multicast address filter
   ETH->MACHT0R = hashTable[0];
   ETH->MACHT1R = hashTable[1];

   //Debug message
   TRACE_DEBUG("  MACHT0R = %08" PRIX32 "\r\n", ETH->MACHT0R);
   TRACE_DEBUG("  MACHT1R = %08" PRIX32 "\r\n", ETH->MACHT1R);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Adjust MAC configuration parameters for proper operation
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t stm32mp1xxEthUpdateMacConfig(NetInterface *interface)
{
   uint32_t config;

   //Read current MAC configuration
   config = ETH->MACCR;

   //1000BASE-T operation mode?
   if(interface->linkSpeed == NIC_LINK_SPEED_1GBPS)
   {
      config &= ~ETH_MACCR_PS;
      config &= ~ETH_MACCR_FES;
   }
   //100BASE-TX operation mode?
   else if(interface->linkSpeed == NIC_LINK_SPEED_100MBPS)
   {
      config |= ETH_MACCR_PS;
      config |= ETH_MACCR_FES;
   }
   //10BASE-T operation mode?
   else
   {
      config |= ETH_MACCR_PS;
      config &= ~ETH_MACCR_FES;
   }

   //Half-duplex or full-duplex mode?
   if(interface->duplexMode == NIC_FULL_DUPLEX_MODE)
   {
      config |= ETH_MACCR_DM;
   }
   else
   {
      config &= ~ETH_MACCR_DM;
   }

   //Update MAC configuration register
   ETH->MACCR = config;

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

void stm32mp1xxEthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data)
{
   uint32_t temp;

   //Valid opcode?
   if(opcode == SMI_OPCODE_WRITE)
   {
      //Take care not to alter MDC clock configuration
      temp = ETH->MACMDIOAR & ETH_MACMDIOAR_CR;
      //Set up a write operation
      temp |= ETH_MACMDIOAR_GOC_Val(1) | ETH_MACMDIOAR_GB;
      //PHY address
      temp |= (phyAddr << 21) & ETH_MACMDIOAR_PA;
      //Register address
      temp |= (regAddr << 16) & ETH_MACMDIOAR_RDA;

      //Data to be written in the PHY register
      ETH->MACMDIODR = data & ETH_MACMDIODR_GD;

      //Start a write operation
      ETH->MACMDIOAR = temp;
      //Wait for the write to complete
      while((ETH->MACMDIOAR & ETH_MACMDIOAR_GB) != 0)
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

uint16_t stm32mp1xxEthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr)
{
   uint16_t data;
   uint32_t temp;

   //Valid opcode?
   if(opcode == SMI_OPCODE_READ)
   {
      //Take care not to alter MDC clock configuration
      temp = ETH->MACMDIOAR & ETH_MACMDIOAR_CR;
      //Set up a read operation
      temp |= ETH_MACMDIOAR_GOC_Val(3) | ETH_MACMDIOAR_GB;
      //PHY address
      temp |= (phyAddr << 21) & ETH_MACMDIOAR_PA;
      //Register address
      temp |= (regAddr << 16) & ETH_MACMDIOAR_RDA;

      //Start a read operation
      ETH->MACMDIOAR = temp;
      //Wait for the read to complete
      while((ETH->MACMDIOAR & ETH_MACMDIOAR_GB) != 0)
      {
      }

      //Get register value
      data = ETH->MACMDIODR & ETH_MACMDIODR_GD;
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

uint32_t stm32mp1xxEthCalcCrc(const void *data, size_t length)
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
