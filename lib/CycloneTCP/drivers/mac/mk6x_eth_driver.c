/**
 * @file mk6x_eth_driver.c
 * @brief NXP Kinetis K60/K64/K65/K66 Ethernet MAC driver
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

//MK60N512MD100 device?
#if defined(MK60N512MD100)
   #include "mk60n512md100.h"
//MK60D10 device?
#elif defined(MK60D10)
   #include "mk60d10.h"
//MK60F12 device?
#elif defined(MK60F12)
   #include "mk60f12.h"
//MK64F12 device?
#elif defined(MK64F12)
   #include "mk64f12.h"
//MK65F18 device?
#elif defined(MK65F18)
   #include "mk65f18.h"
//MK66F18 device?
#elif defined(MK66F18)
   #include "mk66f18.h"
#endif

//Dependencies
#include "core/net.h"
#include "drivers/mac/mk6x_eth_driver.h"
#include "debug.h"

//Underlying network interface
static NetInterface *nicDriverInterface;

//IAR EWARM compiler?
#if defined(__ICCARM__)

//TX buffer
#pragma data_alignment = 16
static uint8_t txBuffer[MK6X_ETH_TX_BUFFER_COUNT][MK6X_ETH_TX_BUFFER_SIZE];
//RX buffer
#pragma data_alignment = 16
static uint8_t rxBuffer[MK6X_ETH_RX_BUFFER_COUNT][MK6X_ETH_RX_BUFFER_SIZE];
//TX buffer descriptors
#pragma data_alignment = 16
static uint16_t txBufferDesc[MK6X_ETH_TX_BUFFER_COUNT][16];
//RX buffer descriptors
#pragma data_alignment = 16
static uint16_t rxBufferDesc[MK6X_ETH_RX_BUFFER_COUNT][16];

//ARM or GCC compiler?
#else

//TX buffer
static uint8_t txBuffer[MK6X_ETH_TX_BUFFER_COUNT][MK6X_ETH_TX_BUFFER_SIZE]
   __attribute__((aligned(16)));
//RX buffer
static uint8_t rxBuffer[MK6X_ETH_RX_BUFFER_COUNT][MK6X_ETH_RX_BUFFER_SIZE]
   __attribute__((aligned(16)));
//TX buffer descriptors
static uint16_t txBufferDesc[MK6X_ETH_TX_BUFFER_COUNT][16]
   __attribute__((aligned(16)));
//RX buffer descriptors
static uint16_t rxBufferDesc[MK6X_ETH_RX_BUFFER_COUNT][16]
   __attribute__((aligned(16)));

#endif

//TX buffer index
static uint_t txBufferIndex;
//RX buffer index
static uint_t rxBufferIndex;


/**
 * @brief Kinetis K6x Ethernet MAC driver
 **/

const NicDriver mk6xEthDriver =
{
   NIC_TYPE_ETHERNET,
   ETH_MTU,
   mk6xEthInit,
   mk6xEthTick,
   mk6xEthEnableIrq,
   mk6xEthDisableIrq,
   mk6xEthEventHandler,
   mk6xEthSendPacket,
   mk6xEthUpdateMacAddrFilter,
   mk6xEthUpdateMacConfig,
   mk6xEthWritePhyReg,
   mk6xEthReadPhyReg,
   TRUE,
   TRUE,
   TRUE,
   FALSE
};


/**
 * @brief Kinetis K6x Ethernet MAC initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t mk6xEthInit(NetInterface *interface)
{
   error_t error;
   uint32_t value;

   //Debug message
   TRACE_INFO("Initializing Kinetis K6x Ethernet MAC...\r\n");

   //Save underlying network interface
   nicDriverInterface = interface;

   //Disable MPU
   MPU->CESR &= ~MPU_CESR_VLD_MASK;

   //Enable external reference clock
   OSC->CR |= OSC_CR_ERCLKEN_MASK;
   //Enable ENET peripheral clock
   SIM->SCGC2 |= SIM_SCGC2_ENET_MASK;

   //GPIO configuration
   mk6xEthInitGpio(interface);

   //Reset ENET module
   ENET->ECR = ENET_ECR_RESET_MASK;
   //Wait for the reset to complete
   while((ENET->ECR & ENET_ECR_RESET_MASK) != 0)
   {
   }

   //Receive control register
   ENET->RCR = ENET_RCR_MAX_FL(MK6X_ETH_RX_BUFFER_SIZE) |
      ENET_RCR_RMII_MODE_MASK | ENET_RCR_MII_MODE_MASK;

   //Transmit control register
   ENET->TCR = 0;
   //Configure MDC clock frequency
   ENET->MSCR = ENET_MSCR_MII_SPEED(59);

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

   //Set the MAC address of the station (upper 16 bits)
   value = interface->macAddr.b[5];
   value |= (interface->macAddr.b[4] << 8);
   ENET->PAUR = ENET_PAUR_PADDR2(value) | ENET_PAUR_TYPE(0x8808);

   //Set the MAC address of the station (lower 32 bits)
   value = interface->macAddr.b[3];
   value |= (interface->macAddr.b[2] << 8);
   value |= (interface->macAddr.b[1] << 16);
   value |= (interface->macAddr.b[0] << 24);
   ENET->PALR = ENET_PALR_PADDR1(value);

   //Hash table for unicast address filtering
   ENET->IALR = 0;
   ENET->IAUR = 0;
   //Hash table for multicast address filtering
   ENET->GALR = 0;
   ENET->GAUR = 0;

   //Disable transmit accelerator functions
   ENET->TACC = 0;
   //Disable receive accelerator functions
   ENET->RACC = 0;

   //Use enhanced buffer descriptors
   ENET->ECR = ENET_ECR_EN1588_MASK;

   //Reset statistics counters
   ENET->MIBC = ENET_MIBC_MIB_CLEAR_MASK;
   ENET->MIBC = 0;

   //Initialize buffer descriptors
   mk6xEthInitBufferDesc(interface);

   //Clear any pending interrupts
   ENET->EIR = 0xFFFFFFFF;
   //Enable desired interrupts
   ENET->EIMR = ENET_EIMR_TXF_MASK | ENET_EIMR_RXF_MASK | ENET_EIMR_EBERR_MASK;

   //Set priority grouping (4 bits for pre-emption priority, no bits for subpriority)
   NVIC_SetPriorityGrouping(MK6X_ETH_IRQ_PRIORITY_GROUPING);

   //Configure ENET transmit interrupt priority
   NVIC_SetPriority(ENET_Transmit_IRQn, NVIC_EncodePriority(MK6X_ETH_IRQ_PRIORITY_GROUPING,
      MK6X_ETH_IRQ_GROUP_PRIORITY, MK6X_ETH_IRQ_SUB_PRIORITY));

   //Configure ENET receive interrupt priority
   NVIC_SetPriority(ENET_Receive_IRQn, NVIC_EncodePriority(MK6X_ETH_IRQ_PRIORITY_GROUPING,
      MK6X_ETH_IRQ_GROUP_PRIORITY, MK6X_ETH_IRQ_SUB_PRIORITY));

   //Configure ENET error interrupt priority
   NVIC_SetPriority(ENET_Error_IRQn, NVIC_EncodePriority(MK6X_ETH_IRQ_PRIORITY_GROUPING,
      MK6X_ETH_IRQ_GROUP_PRIORITY, MK6X_ETH_IRQ_SUB_PRIORITY));

   //Enable Ethernet MAC
   ENET->ECR |= ENET_ECR_ETHEREN_MASK;
   //Instruct the DMA to poll the receive descriptor list
   ENET->RDAR = ENET_RDAR_RDAR_MASK;

   //Accept any packets from the upper layer
   osSetEvent(&interface->nicTxEvent);

   //Successful initialization
   return NO_ERROR;
}


//FRDM-K64F, FRDM-K66F, TWR-K60N512, TWR-K60F120M, TWR-K64F120M,
//TWR-K65F180M or embOS/IP Switch Board?
#if defined(USE_FRDM_K64F) || defined(USE_FRDM_K66F) || \
   defined(USE_TWR_K60N512) || defined(USE_TWR_K60F120M) || \
   defined(USE_TWR_K64F120M) || defined(USE_TWR_K65F180M) || \
   defined(USE_EMBOS_IP_SWITCH_BOARD)

/**
 * @brief GPIO configuration
 * @param[in] interface Underlying network interface
 **/

void mk6xEthInitGpio(NetInterface *interface)
{
//TWR-K60N512 or TWR-K60F120M evaluation board?
#if defined(USE_TWR_K60N512) || defined(USE_TWR_K60F120M)
   //Enable PORTA and PORTB peripheral clocks
   SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK | SIM_SCGC5_PORTB_MASK;

   //Configure RMII0_RXER (PTA5)
   PORTA->PCR[5] = PORT_PCR_MUX(4) | PORT_PCR_PE_MASK;
   //Configure RMII0_RXD1 (PTA12)
   PORTA->PCR[12] = PORT_PCR_MUX(4);
   //Configure RMII0_RXD0 (PTA13)
   PORTA->PCR[13] = PORT_PCR_MUX(4);
   //Configure RMII0_CRS_DV (PTA14)
   PORTA->PCR[14] = PORT_PCR_MUX(4);
   //Configure RMII0_TXEN (PTA15)
   PORTA->PCR[15] = PORT_PCR_MUX(4);
   //Configure RMII0_TXD0 (PTA16)
   PORTA->PCR[16] = PORT_PCR_MUX(4);
   //Configure RMII0_TXD1 (PTA17)
   PORTA->PCR[17] = PORT_PCR_MUX(4);

   //Configure RMII0_MDIO (PTB0)
   PORTB->PCR[0] = PORT_PCR_MUX(4) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
   //Configure RMII0_MDC (PTB1)
   PORTB->PCR[1] = PORT_PCR_MUX(4);

//FRDM-K64F or TWR-K64F120M evaluation board?
#elif defined(USE_FRDM_K64F) || defined(USE_TWR_K64F120M)
   //Enable PORTA and PORTB peripheral clocks
   SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK | SIM_SCGC5_PORTB_MASK;

   //Configure RMII0_RXER (PTA5)
   PORTA->PCR[5] = PORT_PCR_MUX(4) | PORT_PCR_PE_MASK;
   //Configure RMII0_RXD1 (PTA12)
   PORTA->PCR[12] = PORT_PCR_MUX(4);
   //Configure RMII0_RXD0 (PTA13)
   PORTA->PCR[13] = PORT_PCR_MUX(4);
   //Configure RMII0_CRS_DV (PTA14)
   PORTA->PCR[14] = PORT_PCR_MUX(4);
   //Configure RMII0_TXEN (PTA15)
   PORTA->PCR[15] = PORT_PCR_MUX(4);
   //Configure RMII0_TXD0 (PTA16)
   PORTA->PCR[16] = PORT_PCR_MUX(4);
   //Configure RMII0_TXD1 (PTA17)
   PORTA->PCR[17] = PORT_PCR_MUX(4);

   //Configure RMII0_MDIO (PTB0)
   PORTB->PCR[0] = PORT_PCR_MUX(4) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
   //Configure RMII0_MDC (PTB1)
   PORTB->PCR[1] = PORT_PCR_MUX(4);

   //Select RMII clock source (EXTAL)
   SIM->SOPT2 &= ~SIM_SOPT2_RMIISRC_MASK;

//TWR-K65F180M evaluation board?
#elif defined(USE_TWR_K65F180M)
   //Enable PORTA and PORTE peripheral clocks
   SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK | SIM_SCGC5_PORTE_MASK;

   //Configure RMII0_RXER (PTA5)
   PORTA->PCR[5] = PORT_PCR_MUX(4) | PORT_PCR_PE_MASK;
   //Configure RMII0_MDIO (PTA7)
   PORTA->PCR[7] = PORT_PCR_MUX(5) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
   //Configure RMII0_MDC (PTA8)
   PORTA->PCR[8] = PORT_PCR_MUX(5);
   //Configure RMII0_RXD1 (PTA12)
   PORTA->PCR[12] = PORT_PCR_MUX(4);
   //Configure RMII0_RXD0 (PTA13)
   PORTA->PCR[13] = PORT_PCR_MUX(4);
   //Configure RMII0_CRS_DV (PTA14)
   PORTA->PCR[14] = PORT_PCR_MUX(4);
   //Configure RMII0_TXEN (PTA15)
   PORTA->PCR[15] = PORT_PCR_MUX(4);
   //Configure RMII0_TXD0 (PTA16)
   PORTA->PCR[16] = PORT_PCR_MUX(4);
   //Configure RMII0_TXD1 (PTA17)
   PORTA->PCR[17] = PORT_PCR_MUX(4);

   //Configure ENET_1588_CLKIN (PTE26)
   PORTE->PCR[26] = PORT_PCR_MUX(2);

   //Select RMII clock source (ENET_1588_CLKIN)
   SIM->SOPT2 |= SIM_SOPT2_RMIISRC_MASK;

//FRDM-K66F evaluation board?
#elif defined(USE_FRDM_K66F)
//Enable PORTA, PORTB and PORTE peripheral clocks
   SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK |
      SIM_SCGC5_PORTB_MASK | SIM_SCGC5_PORTE_MASK;

   //Configure RMII0_RXER (PTA5)
   PORTA->PCR[5] = PORT_PCR_MUX(4) | PORT_PCR_PE_MASK;
   //Configure RMII0_RXD1 (PTA12)
   PORTA->PCR[12] = PORT_PCR_MUX(4);
   //Configure RMII0_RXD0 (PTA13)
   PORTA->PCR[13] = PORT_PCR_MUX(4);
   //Configure RMII0_CRS_DV (PTA14)
   PORTA->PCR[14] = PORT_PCR_MUX(4);
   //Configure RMII0_TXEN (PTA15)
   PORTA->PCR[15] = PORT_PCR_MUX(4);
   //Configure RMII0_TXD0 (PTA16)
   PORTA->PCR[16] = PORT_PCR_MUX(4);
   //Configure RMII0_TXD1 (PTA17)
   PORTA->PCR[17] = PORT_PCR_MUX(4);

   //Configure RMII0_MDIO (PTB0)
   PORTB->PCR[0] = PORT_PCR_MUX(4) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
   //Configure RMII0_MDC (PTB1)
   PORTB->PCR[1] = PORT_PCR_MUX(4);

   //Configure ENET_1588_CLKIN (PTE26)
   PORTE->PCR[26] = PORT_PCR_MUX(2);

   //Select RMII clock source (ENET_1588_CLKIN)
   SIM->SOPT2 |= SIM_SOPT2_RMIISRC_MASK;

//embOS/IP Switch Board?
#elif defined(USE_EMBOS_IP_SWITCH_BOARD)
   //Enable PORTA and PORTE peripheral clocks
   SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK | SIM_SCGC5_PORTE_MASK;

   //Configure RMII0_RXD1 (PTA12)
   PORTA->PCR[12] = PORT_PCR_MUX(4);
   //Configure RMII0_RXD0 (PTA13)
   PORTA->PCR[13] = PORT_PCR_MUX(4);
   //Configure RMII0_CRS_DV (PTA14)
   PORTA->PCR[14] = PORT_PCR_MUX(4);
   //Configure RMII0_TXEN (PTA15)
   PORTA->PCR[15] = PORT_PCR_MUX(4);
   //Configure RMII0_TXD0 (PTA16)
   PORTA->PCR[16] = PORT_PCR_MUX(4);
   //Configure RMII0_TXD1 (PTA17)
   PORTA->PCR[17] = PORT_PCR_MUX(4);

   //Configure ENET_1588_CLKIN (PTE26)
   PORTE->PCR[26] = PORT_PCR_MUX(2);

   //Select RMII clock source (ENET_1588_CLKIN)
   SIM->SOPT2 |= SIM_SOPT2_RMIISRC_MASK;
#endif
}

#endif


/**
 * @brief Initialize buffer descriptors
 * @param[in] interface Underlying network interface
 **/

void mk6xEthInitBufferDesc(NetInterface *interface)
{
   uint_t i;
   uint32_t address;

   //Clear TX and RX buffer descriptors
   osMemset(txBufferDesc, 0, sizeof(txBufferDesc));
   osMemset(rxBufferDesc, 0, sizeof(rxBufferDesc));

   //Initialize TX buffer descriptors
   for(i = 0; i < MK6X_ETH_TX_BUFFER_COUNT; i++)
   {
      //Calculate the address of the current TX buffer
      address = (uint32_t) txBuffer[i];
      //Transmit buffer address
      txBufferDesc[i][2] = htobe16(address >> 16);
      txBufferDesc[i][3] = htobe16(address & 0xFFFF);
      //Generate interrupts
      txBufferDesc[i][4] = HTOBE16(ENET_TBD4_INT);
   }

   //Mark the last descriptor entry with the wrap flag
   txBufferDesc[i - 1][0] |= HTOBE16(ENET_TBD0_W);
   //Initialize TX buffer index
   txBufferIndex = 0;

   //Initialize RX buffer descriptors
   for(i = 0; i < MK6X_ETH_RX_BUFFER_COUNT; i++)
   {
      //Calculate the address of the current RX buffer
      address = (uint32_t) rxBuffer[i];
      //The descriptor is initially owned by the DMA
      rxBufferDesc[i][0] = HTOBE16(ENET_RBD0_E);
      //Receive buffer address
      rxBufferDesc[i][2] = htobe16(address >> 16);
      rxBufferDesc[i][3] = htobe16(address & 0xFFFF);
      //Generate interrupts
      rxBufferDesc[i][4] = HTOBE16(ENET_RBD4_INT);
   }

   //Mark the last descriptor entry with the wrap flag
   rxBufferDesc[i - 1][0] |= HTOBE16(ENET_RBD0_W);
   //Initialize RX buffer index
   rxBufferIndex = 0;

   //Start location of the TX descriptor list
   ENET->TDSR = (uint32_t) txBufferDesc;
   //Start location of the RX descriptor list
   ENET->RDSR = (uint32_t) rxBufferDesc;
   //Maximum receive buffer size
   ENET->MRBR = MK6X_ETH_RX_BUFFER_SIZE;
}


/**
 * @brief Kinetis K6x Ethernet MAC timer handler
 *
 * This routine is periodically called by the TCP/IP stack to handle periodic
 * operations such as polling the link state
 *
 * @param[in] interface Underlying network interface
 **/

void mk6xEthTick(NetInterface *interface)
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

void mk6xEthEnableIrq(NetInterface *interface)
{
   //Enable Ethernet MAC interrupts
   NVIC_EnableIRQ(ENET_Transmit_IRQn);
   NVIC_EnableIRQ(ENET_Receive_IRQn);
   NVIC_EnableIRQ(ENET_Error_IRQn);


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

void mk6xEthDisableIrq(NetInterface *interface)
{
   //Disable Ethernet MAC interrupts
   NVIC_DisableIRQ(ENET_Transmit_IRQn);
   NVIC_DisableIRQ(ENET_Receive_IRQn);
   NVIC_DisableIRQ(ENET_Error_IRQn);


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
 * @brief Ethernet MAC transmit interrupt
 **/

void ENET_Transmit_IRQHandler(void)
{
   bool_t flag;

   //Interrupt service routine prologue
   osEnterIsr();

   //This flag will be set if a higher priority task must be woken
   flag = FALSE;

   //Packet transmitted?
   if((ENET->EIR & ENET_EIR_TXF_MASK) != 0)
   {
      //Clear TXF interrupt flag
      ENET->EIR = ENET_EIR_TXF_MASK;

      //Check whether the TX buffer is available for writing
      if((txBufferDesc[txBufferIndex][0] & HTOBE16(ENET_TBD0_R)) == 0)
      {
         //Notify the TCP/IP stack that the transmitter is ready to send
         flag = osSetEventFromIsr(&nicDriverInterface->nicTxEvent);
      }

      //Instruct the DMA to poll the transmit descriptor list
      ENET->TDAR = ENET_TDAR_TDAR_MASK;
   }

   //Interrupt service routine epilogue
   osExitIsr(flag);
}


/**
 * @brief Ethernet MAC receive interrupt
 **/

void ENET_Receive_IRQHandler(void)
{
   bool_t flag;

   //Interrupt service routine prologue
   osEnterIsr();

   //This flag will be set if a higher priority task must be woken
   flag = FALSE;

   //Packet received?
   if((ENET->EIR & ENET_EIR_RXF_MASK) != 0)
   {
      //Disable RXF interrupt
      ENET->EIMR &= ~ENET_EIMR_RXF_MASK;

      //Set event flag
      nicDriverInterface->nicEvent = TRUE;
      //Notify the TCP/IP stack of the event
      flag = osSetEventFromIsr(&netEvent);
   }

   //Interrupt service routine epilogue
   osExitIsr(flag);
}


/**
 * @brief Ethernet MAC error interrupt
 **/

void ENET_Error_IRQHandler(void)
{
   bool_t flag;

   //Interrupt service routine prologue
   osEnterIsr();

   //This flag will be set if a higher priority task must be woken
   flag = FALSE;

   //System bus error?
   if((ENET->EIR & ENET_EIR_EBERR_MASK) != 0)
   {
      //Disable EBERR interrupt
      ENET->EIMR &= ~ENET_EIMR_EBERR_MASK;

      //Set event flag
      nicDriverInterface->nicEvent = TRUE;
      //Notify the TCP/IP stack of the event
      flag |= osSetEventFromIsr(&netEvent);
   }

   //Interrupt service routine epilogue
   osExitIsr(flag);
}


/**
 * @brief Kinetis K6x Ethernet MAC event handler
 * @param[in] interface Underlying network interface
 **/

void mk6xEthEventHandler(NetInterface *interface)
{
   error_t error;
   uint32_t status;

   //Read interrupt event register
   status = ENET->EIR;

   //Packet received?
   if((status & ENET_EIR_RXF_MASK) != 0)
   {
      //Clear RXF interrupt flag
      ENET->EIR = ENET_EIR_RXF_MASK;

      //Process all pending packets
      do
      {
         //Read incoming packet
         error = mk6xEthReceivePacket(interface);

         //No more data in the receive buffer?
      } while(error != ERROR_BUFFER_EMPTY);
   }

   //System bus error?
   if((status & ENET_EIR_EBERR_MASK) != 0)
   {
      //Clear EBERR interrupt flag
      ENET->EIR = ENET_EIR_EBERR_MASK;

      //Disable Ethernet MAC
      ENET->ECR &= ~ENET_ECR_ETHEREN_MASK;
      //Reset buffer descriptors
      mk6xEthInitBufferDesc(interface);
      //Resume normal operation
      ENET->ECR |= ENET_ECR_ETHEREN_MASK;
      //Instruct the DMA to poll the receive descriptor list
      ENET->RDAR = ENET_RDAR_RDAR_MASK;
   }

   //Re-enable Ethernet MAC interrupts
   ENET->EIMR = ENET_EIMR_TXF_MASK | ENET_EIMR_RXF_MASK | ENET_EIMR_EBERR_MASK;
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

error_t mk6xEthSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary)
{
   size_t length;

   //Retrieve the length of the packet
   length = netBufferGetLength(buffer) - offset;

   //Check the frame length
   if(length > MK6X_ETH_TX_BUFFER_SIZE)
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
      //Report an error
      return ERROR_INVALID_LENGTH;
   }

   //Make sure the current buffer is available for writing
   if(txBufferDesc[txBufferIndex][0] & HTOBE16(ENET_TBD0_R))
   {
      return ERROR_FAILURE;
   }

   //Copy user data to the transmit buffer
   netBufferRead(txBuffer[txBufferIndex], buffer, offset, length);

   //Set frame length
   txBufferDesc[txBufferIndex][1] = HTOBE16(length);
   //Clear BDU flag
   txBufferDesc[txBufferIndex][8] = 0;

   //Check current index
   if(txBufferIndex < (MK6X_ETH_TX_BUFFER_COUNT - 1))
   {
      //Give the ownership of the descriptor to the DMA engine
      txBufferDesc[txBufferIndex][0] = HTOBE16(ENET_TBD0_R |
         ENET_TBD0_L | ENET_TBD0_TC);

      //Point to the next buffer
      txBufferIndex++;
   }
   else
   {
      //Give the ownership of the descriptor to the DMA engine
      txBufferDesc[txBufferIndex][0] = HTOBE16(ENET_TBD0_R |
         ENET_TBD0_W | ENET_TBD0_L | ENET_TBD0_TC);

      //Wrap around
      txBufferIndex = 0;
   }

   //Instruct the DMA to poll the transmit descriptor list
   ENET->TDAR = ENET_TDAR_TDAR_MASK;

   //Check whether the next buffer is available for writing
   if((txBufferDesc[txBufferIndex][0] & HTOBE16(ENET_TBD0_R)) == 0)
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

error_t mk6xEthReceivePacket(NetInterface *interface)
{
   error_t error;
   size_t n;
   NetRxAncillary ancillary;

   //Current buffer available for reading?
   if((rxBufferDesc[rxBufferIndex][0] & HTOBE16(ENET_RBD0_E)) == 0)
   {
      //The frame should not span multiple buffers
      if((rxBufferDesc[rxBufferIndex][0] & HTOBE16(ENET_RBD0_L)) != 0)
      {
         //Check whether an error occurred
         if((rxBufferDesc[rxBufferIndex][0] & HTOBE16(ENET_RBD0_LG |
            ENET_RBD0_NO | ENET_RBD0_CR | ENET_RBD0_OV | ENET_RBD0_TR)) == 0)
         {
            //Retrieve the length of the frame
            n = betoh16(rxBufferDesc[rxBufferIndex][1]);
            //Limit the number of data to read
            n = MIN(n, MK6X_ETH_RX_BUFFER_SIZE);

            //Additional options can be passed to the stack along with the packet
            ancillary = NET_DEFAULT_RX_ANCILLARY;

            //Pass the packet to the upper layer
            nicProcessPacket(interface, rxBuffer[rxBufferIndex], n, &ancillary);

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

      //Clear BDU flag
      rxBufferDesc[rxBufferIndex][8] = 0;

      //Check current index
      if(rxBufferIndex < (MK6X_ETH_RX_BUFFER_COUNT - 1))
      {
         //Give the ownership of the descriptor back to the DMA engine
         rxBufferDesc[rxBufferIndex][0] = HTOBE16(ENET_RBD0_E);
         //Point to the next buffer
         rxBufferIndex++;
      }
      else
      {
         //Give the ownership of the descriptor back to the DMA engine
         rxBufferDesc[rxBufferIndex][0] = HTOBE16(ENET_RBD0_E | ENET_RBD0_W);
         //Wrap around
         rxBufferIndex = 0;
      }

      //Instruct the DMA to poll the receive descriptor list
      ENET->RDAR = ENET_RDAR_RDAR_MASK;
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

error_t mk6xEthUpdateMacAddrFilter(NetInterface *interface)
{
   uint_t i;
   uint_t k;
   uint32_t crc;
   uint32_t value;
   uint32_t unicastHashTable[2];
   uint32_t multicastHashTable[2];
   MacFilterEntry *entry;

   //Debug message
   TRACE_DEBUG("Updating MAC filter...\r\n");

   //Set the MAC address of the station (upper 16 bits)
   value = interface->macAddr.b[5];
   value |= (interface->macAddr.b[4] << 8);
   ENET->PAUR = ENET_PAUR_PADDR2(value) | ENET_PAUR_TYPE(0x8808);

   //Set the MAC address of the station (lower 32 bits)
   value = interface->macAddr.b[3];
   value |= (interface->macAddr.b[2] << 8);
   value |= (interface->macAddr.b[1] << 16);
   value |= (interface->macAddr.b[0] << 24);
   ENET->PALR = ENET_PALR_PADDR1(value);

   //Clear hash table (unicast address filtering)
   unicastHashTable[0] = 0;
   unicastHashTable[1] = 0;

   //Clear hash table (multicast address filtering)
   multicastHashTable[0] = 0;
   multicastHashTable[1] = 0;

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
         crc = mk6xEthCalcCrc(&entry->addr, sizeof(MacAddr));

         //The upper 6 bits in the CRC register are used to index the
         //contents of the hash table
         k = (crc >> 26) & 0x3F;

         //Multicast address?
         if(macIsMulticastAddr(&entry->addr))
         {
            //Update the multicast hash table
            multicastHashTable[k / 32] |= (1 << (k % 32));
         }
         else
         {
            //Update the unicast hash table
            unicastHashTable[k / 32] |= (1 << (k % 32));
         }
      }
   }

   //Write the hash table (unicast address filtering)
   ENET->IALR = unicastHashTable[0];
   ENET->IAUR = unicastHashTable[1];

   //Write the hash table (multicast address filtering)
   ENET->GALR = multicastHashTable[0];
   ENET->GAUR = multicastHashTable[1];

   //Debug message
   TRACE_DEBUG("  IALR = %08" PRIX32 "\r\n", ENET->IALR);
   TRACE_DEBUG("  IAUR = %08" PRIX32 "\r\n", ENET->IAUR);
   TRACE_DEBUG("  GALR = %08" PRIX32 "\r\n", ENET->GALR);
   TRACE_DEBUG("  GAUR = %08" PRIX32 "\r\n", ENET->GAUR);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Adjust MAC configuration parameters for proper operation
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t mk6xEthUpdateMacConfig(NetInterface *interface)
{
   //Disable Ethernet MAC while modifying configuration registers
   ENET->ECR &= ~ENET_ECR_ETHEREN_MASK;

   //10BASE-T or 100BASE-TX operation mode?
   if(interface->linkSpeed == NIC_LINK_SPEED_100MBPS)
   {
      //100 Mbps operation
      ENET->RCR &= ~ENET_RCR_RMII_10T_MASK;
   }
   else
   {
      //10 Mbps operation
      ENET->RCR |= ENET_RCR_RMII_10T_MASK;
   }

   //Half-duplex or full-duplex mode?
   if(interface->duplexMode == NIC_FULL_DUPLEX_MODE)
   {
      //Full-duplex mode
      ENET->TCR |= ENET_TCR_FDEN_MASK;
      //Receive path operates independently of transmit
      ENET->RCR &= ~ENET_RCR_DRT_MASK;
   }
   else
   {
      //Half-duplex mode
      ENET->TCR &= ~ENET_TCR_FDEN_MASK;
      //Disable reception of frames while transmitting
      ENET->RCR |= ENET_RCR_DRT_MASK;
   }

   //Reset buffer descriptors
   mk6xEthInitBufferDesc(interface);

   //Re-enable Ethernet MAC
   ENET->ECR |= ENET_ECR_ETHEREN_MASK;
   //Instruct the DMA to poll the receive descriptor list
   ENET->RDAR = ENET_RDAR_RDAR_MASK;

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

void mk6xEthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data)
{
   uint32_t temp;

   //Valid opcode?
   if(opcode == SMI_OPCODE_WRITE)
   {
      //Set up a write operation
      temp = ENET_MMFR_ST(1) | ENET_MMFR_OP(1) | ENET_MMFR_TA(2);
      //PHY address
      temp |= ENET_MMFR_PA(phyAddr);
      //Register address
      temp |= ENET_MMFR_RA(regAddr);
      //Register value
      temp |= ENET_MMFR_DATA(data);

      //Clear MII interrupt flag
      ENET->EIR = ENET_EIR_MII_MASK;
      //Start a write operation
      ENET->MMFR = temp;

      //Wait for the write to complete
      while((ENET->EIR & ENET_EIR_MII_MASK) == 0)
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

uint16_t mk6xEthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr)
{
   uint16_t data;
   uint32_t temp;

   //Valid opcode?
   if(opcode == SMI_OPCODE_READ)
   {
      //Set up a read operation
      temp = ENET_MMFR_ST(1) | ENET_MMFR_OP(2) | ENET_MMFR_TA(2);
      //PHY address
      temp |= ENET_MMFR_PA(phyAddr);
      //Register address
      temp |= ENET_MMFR_RA(regAddr);

      //Clear MII interrupt flag
      ENET->EIR = ENET_EIR_MII_MASK;
      //Start a read operation
      ENET->MMFR = temp;

      //Wait for the read to complete
      while((ENET->EIR & ENET_EIR_MII_MASK) == 0)
      {
      }

      //Get register value
      data = ENET->MMFR & ENET_MMFR_DATA_MASK;
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

uint32_t mk6xEthCalcCrc(const void *data, size_t length)
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
