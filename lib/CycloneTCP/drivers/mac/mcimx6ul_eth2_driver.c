/**
 * @file mcimx6ul_eth2_driver.c
 * @brief NXP i.MX6UL Ethernet MAC driver (ENET2 instance)
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
#include "fsl_gpio.h"
#include "fsl_iomuxc.h"
#include "core/net.h"
#include "drivers/mac/mcimx6ul_eth2_driver.h"
#include "debug.h"

//Underlying network interface
static NetInterface *nicDriverInterface;

//IAR EWARM compiler?
#if defined(__ICCARM__)

//TX buffer
#pragma data_alignment = 64
#pragma location = MCIMX6UL_ETH2_RAM_SECTION
static uint8_t txBuffer[MCIMX6UL_ETH2_TX_BUFFER_COUNT][MCIMX6UL_ETH2_TX_BUFFER_SIZE];
//RX buffer
#pragma data_alignment = 64
#pragma location = MCIMX6UL_ETH2_RAM_SECTION
static uint8_t rxBuffer[MCIMX6UL_ETH2_RX_BUFFER_COUNT][MCIMX6UL_ETH2_RX_BUFFER_SIZE];
//TX buffer descriptors
#pragma data_alignment = 64
#pragma location = MCIMX6UL_ETH2_RAM_SECTION
static uint32_t txBufferDesc[MCIMX6UL_ETH2_TX_BUFFER_COUNT][8];
//RX buffer descriptors
#pragma data_alignment = 64
#pragma location = MCIMX6UL_ETH2_RAM_SECTION
static uint32_t rxBufferDesc[MCIMX6UL_ETH2_RX_BUFFER_COUNT][8];

//ARM or GCC compiler?
#else

//TX buffer
static uint8_t txBuffer[MCIMX6UL_ETH2_TX_BUFFER_COUNT][MCIMX6UL_ETH2_TX_BUFFER_SIZE]
   __attribute__((aligned(64), __section__(MCIMX6UL_ETH2_RAM_SECTION)));
//RX buffer
static uint8_t rxBuffer[MCIMX6UL_ETH2_RX_BUFFER_COUNT][MCIMX6UL_ETH2_RX_BUFFER_SIZE]
   __attribute__((aligned(64), __section__(MCIMX6UL_ETH2_RAM_SECTION)));
//TX buffer descriptors
static uint32_t txBufferDesc[MCIMX6UL_ETH2_TX_BUFFER_COUNT][8]
   __attribute__((aligned(64), __section__(MCIMX6UL_ETH2_RAM_SECTION)));
//RX buffer descriptors
static uint32_t rxBufferDesc[MCIMX6UL_ETH2_RX_BUFFER_COUNT][8]
   __attribute__((aligned(64), __section__(MCIMX6UL_ETH2_RAM_SECTION)));

#endif

//TX buffer index
static uint_t txBufferIndex;
//RX buffer index
static uint_t rxBufferIndex;


/**
 * @brief i.MX6UL Ethernet MAC driver (ENET2 instance)
 **/

const NicDriver mcimx6ulEth2Driver =
{
   NIC_TYPE_ETHERNET,
   ETH_MTU,
   mcimx6ulEth2Init,
   mcimx6ulEth2Tick,
   mcimx6ulEth2EnableIrq,
   mcimx6ulEth2DisableIrq,
   mcimx6ulEth2EventHandler,
   mcimx6ulEth2SendPacket,
   mcimx6ulEth2UpdateMacAddrFilter,
   mcimx6ulEth2UpdateMacConfig,
   mcimx6ulEth2WritePhyReg,
   mcimx6ulEth2ReadPhyReg,
   TRUE,
   TRUE,
   TRUE,
   FALSE
};


/**
 * @brief i.MX6UL Ethernet MAC initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t mcimx6ulEth2Init(NetInterface *interface)
{
   error_t error;
   uint32_t value;

   //Debug message
   TRACE_INFO("Initializing i.MX6UL Ethernet MAC (ENET2)...\r\n");

   //Save underlying network interface
   nicDriverInterface = interface;

   //Enable ENET peripheral clock
   CLOCK_EnableClock(kCLOCK_Enet);

   //GPIO configuration
   mcimx6ulEth2InitGpio(interface);

   //Reset ENET2 module
   ENET2->ECR = ENET_ECR_RESET_MASK;
   //Wait for the reset to complete
   while((ENET2->ECR & ENET_ECR_RESET_MASK) != 0)
   {
   }

   //Receive control register
   ENET2->RCR = ENET_RCR_MAX_FL(MCIMX6UL_ETH2_RX_BUFFER_SIZE) |
      ENET_RCR_RMII_MODE_MASK | ENET_RCR_MII_MODE_MASK;

   //Transmit control register
   ENET2->TCR = 0;
   //Configure MDC clock frequency
   ENET2->MSCR = ENET_MSCR_HOLDTIME(10) | ENET_MSCR_MII_SPEED(120);

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
   ENET2->PAUR = ENET_PAUR_PADDR2(value) | ENET_PAUR_TYPE(0x8808);

   //Set the MAC address of the station (lower 32 bits)
   value = interface->macAddr.b[3];
   value |= (interface->macAddr.b[2] << 8);
   value |= (interface->macAddr.b[1] << 16);
   value |= (interface->macAddr.b[0] << 24);
   ENET2->PALR = ENET_PALR_PADDR1(value);

   //Hash table for unicast address filtering
   ENET2->IALR = 0;
   ENET2->IAUR = 0;
   //Hash table for multicast address filtering
   ENET2->GALR = 0;
   ENET2->GAUR = 0;

   //Disable transmit accelerator functions
   ENET2->TACC = 0;
   //Disable receive accelerator functions
   ENET2->RACC = 0;

   //Use enhanced buffer descriptors
   ENET2->ECR = ENET_ECR_DBSWP_MASK | ENET_ECR_EN1588_MASK;

   //Reset statistics counters
   ENET2->MIBC = ENET_MIBC_MIB_CLEAR_MASK;
   ENET2->MIBC = 0;

   //Initialize buffer descriptors
   mcimx6ulEth2InitBufferDesc(interface);

   //Clear any pending interrupts
   ENET2->EIR = 0xFFFFFFFF;
   //Enable desired interrupts
   ENET2->EIMR = ENET_EIMR_TXF_MASK | ENET_EIMR_RXF_MASK | ENET_EIMR_EBERR_MASK;

   //Configure ENET2 interrupt priority
   GIC_SetPriority(ENET2_IRQn, MCIMX6UL_ETH2_IRQ_PRIORITY);

   //Enable Ethernet MAC
   ENET2->ECR |= ENET_ECR_ETHEREN_MASK;
   //Instruct the DMA to poll the receive descriptor list
   ENET2->RDAR = ENET_RDAR_RDAR_MASK;

   //Accept any packets from the upper layer
   osSetEvent(&interface->nicTxEvent);

   //Successful initialization
   return NO_ERROR;
}


//MCIMX6UL-EVKB or MCIMX6ULL-EVK evaluation board?
#if defined(USE_MCIMX6UL_EVKB) || defined(USE_MCIMX6ULL_EVK)

/**
 * @brief GPIO configuration
 * @param[in] interface Underlying network interface
 **/

void mcimx6ulEth2InitGpio(NetInterface *interface)
{
   gpio_pin_config_t pinConfig;

   //Enable ENET2_TX_CLK output driver
   IOMUXC_GPR->GPR1 |= IOMUXC_GPR_GPR1_ENET2_TX_CLK_DIR(1);

   //Enable IOMUXC clock
   CLOCK_EnableClock(kCLOCK_Iomuxc);

   //Configure ENET2_TX_CLK pin as ENET2_REF_CLK2
   IOMUXC_SetPinMux(IOMUXC_ENET2_TX_CLK_ENET2_REF_CLK2, 1);

   //Set ENET2_TX_CLK pad properties
   IOMUXC_SetPinConfig(IOMUXC_ENET2_TX_CLK_ENET2_REF_CLK2,
      IOMUXC_SW_PAD_CTL_PAD_HYS(0) |
      IOMUXC_SW_PAD_CTL_PAD_PUS(0) |
      IOMUXC_SW_PAD_CTL_PAD_PUE(0) |
      IOMUXC_SW_PAD_CTL_PAD_PKE(0) |
      IOMUXC_SW_PAD_CTL_PAD_ODE(0) |
      IOMUXC_SW_PAD_CTL_PAD_SPEED(3) |
      IOMUXC_SW_PAD_CTL_PAD_DSE(5) |
      IOMUXC_SW_PAD_CTL_PAD_SRE(1));

   //Configure ENET2_TX_EN pin as ENET2_TX_EN
   IOMUXC_SetPinMux(IOMUXC_ENET2_TX_EN_ENET2_TX_EN, 0);

   //Set ENET2_TX_EN pad properties
   IOMUXC_SetPinConfig(IOMUXC_ENET2_TX_EN_ENET2_TX_EN,
      IOMUXC_SW_PAD_CTL_PAD_HYS(0) |
      IOMUXC_SW_PAD_CTL_PAD_PUS(0) |
      IOMUXC_SW_PAD_CTL_PAD_PUE(0) |
      IOMUXC_SW_PAD_CTL_PAD_PKE(0) |
      IOMUXC_SW_PAD_CTL_PAD_ODE(0) |
      IOMUXC_SW_PAD_CTL_PAD_SPEED(3) |
      IOMUXC_SW_PAD_CTL_PAD_DSE(5) |
      IOMUXC_SW_PAD_CTL_PAD_SRE(1));

   //Configure ENET2_TX_DATA0 pin as ENET2_TDATA00
   IOMUXC_SetPinMux(IOMUXC_ENET2_TX_DATA0_ENET2_TDATA00, 0);

   //Set ENET2_TX_DATA0 pad properties
   IOMUXC_SetPinConfig(IOMUXC_ENET2_TX_DATA0_ENET2_TDATA00,
      IOMUXC_SW_PAD_CTL_PAD_HYS(0) |
      IOMUXC_SW_PAD_CTL_PAD_PUS(0) |
      IOMUXC_SW_PAD_CTL_PAD_PUE(0) |
      IOMUXC_SW_PAD_CTL_PAD_PKE(0) |
      IOMUXC_SW_PAD_CTL_PAD_ODE(0) |
      IOMUXC_SW_PAD_CTL_PAD_SPEED(3) |
      IOMUXC_SW_PAD_CTL_PAD_DSE(5) |
      IOMUXC_SW_PAD_CTL_PAD_SRE(1));

   //Configure ENET2_TX_DATA1 pin as ENET2_TDATA01
   IOMUXC_SetPinMux(IOMUXC_ENET2_TX_DATA1_ENET2_TDATA01, 0);

   //Set ENET2_TX_DATA1 pad properties
   IOMUXC_SetPinConfig(IOMUXC_ENET2_TX_DATA1_ENET2_TDATA01,
      IOMUXC_SW_PAD_CTL_PAD_HYS(0) |
      IOMUXC_SW_PAD_CTL_PAD_PUS(0) |
      IOMUXC_SW_PAD_CTL_PAD_PUE(0) |
      IOMUXC_SW_PAD_CTL_PAD_PKE(0) |
      IOMUXC_SW_PAD_CTL_PAD_ODE(0) |
      IOMUXC_SW_PAD_CTL_PAD_SPEED(3) |
      IOMUXC_SW_PAD_CTL_PAD_DSE(5) |
      IOMUXC_SW_PAD_CTL_PAD_SRE(1));

   //Configure ENET2_RX_EN pin as ENET2_RX_EN
   IOMUXC_SetPinMux(IOMUXC_ENET2_RX_EN_ENET2_RX_EN, 0);

   //Set ENET2_RX_EN pad properties
   IOMUXC_SetPinConfig(IOMUXC_ENET2_RX_EN_ENET2_RX_EN,
      IOMUXC_SW_PAD_CTL_PAD_HYS(0) |
      IOMUXC_SW_PAD_CTL_PAD_PUS(0) |
      IOMUXC_SW_PAD_CTL_PAD_PUE(0) |
      IOMUXC_SW_PAD_CTL_PAD_PKE(0) |
      IOMUXC_SW_PAD_CTL_PAD_ODE(0) |
      IOMUXC_SW_PAD_CTL_PAD_SPEED(3) |
      IOMUXC_SW_PAD_CTL_PAD_DSE(0) |
      IOMUXC_SW_PAD_CTL_PAD_SRE(1));

   //Configure ENET2_RX_ER pin as ENET2_RX_ER
   IOMUXC_SetPinMux(IOMUXC_ENET2_RX_ER_ENET2_RX_ER, 0);

   //Set ENET2_RX_ER pad properties
   IOMUXC_SetPinConfig(IOMUXC_ENET2_RX_ER_ENET2_RX_ER,
      IOMUXC_SW_PAD_CTL_PAD_HYS(0) |
      IOMUXC_SW_PAD_CTL_PAD_PUS(0) |
      IOMUXC_SW_PAD_CTL_PAD_PUE(0) |
      IOMUXC_SW_PAD_CTL_PAD_PKE(0) |
      IOMUXC_SW_PAD_CTL_PAD_ODE(0) |
      IOMUXC_SW_PAD_CTL_PAD_SPEED(3) |
      IOMUXC_SW_PAD_CTL_PAD_DSE(0) |
      IOMUXC_SW_PAD_CTL_PAD_SRE(1));

   //Configure ENET2_RX_DATA0 pin as ENET2_RDATA00
   IOMUXC_SetPinMux(IOMUXC_ENET2_RX_DATA0_ENET2_RDATA00, 0);

   //Set ENET2_RX_DATA0 pad properties
   IOMUXC_SetPinConfig(IOMUXC_ENET2_RX_DATA0_ENET2_RDATA00,
      IOMUXC_SW_PAD_CTL_PAD_HYS(0) |
      IOMUXC_SW_PAD_CTL_PAD_PUS(0) |
      IOMUXC_SW_PAD_CTL_PAD_PUE(0) |
      IOMUXC_SW_PAD_CTL_PAD_PKE(0) |
      IOMUXC_SW_PAD_CTL_PAD_ODE(0) |
      IOMUXC_SW_PAD_CTL_PAD_SPEED(3) |
      IOMUXC_SW_PAD_CTL_PAD_DSE(0) |
      IOMUXC_SW_PAD_CTL_PAD_SRE(1));

   //Configure ENET2_RX_DATA1 pin as ENET2_RDATA01
   IOMUXC_SetPinMux(IOMUXC_ENET2_RX_DATA1_ENET2_RDATA01, 0);

   //Set ENET2_RX_DATA1 pad properties
   IOMUXC_SetPinConfig(IOMUXC_ENET2_RX_DATA1_ENET2_RDATA01,
      IOMUXC_SW_PAD_CTL_PAD_HYS(0) |
      IOMUXC_SW_PAD_CTL_PAD_PUS(0) |
      IOMUXC_SW_PAD_CTL_PAD_PUE(0) |
      IOMUXC_SW_PAD_CTL_PAD_PKE(0) |
      IOMUXC_SW_PAD_CTL_PAD_ODE(0) |
      IOMUXC_SW_PAD_CTL_PAD_SPEED(3) |
      IOMUXC_SW_PAD_CTL_PAD_DSE(0) |
      IOMUXC_SW_PAD_CTL_PAD_SRE(1));

#if defined(USE_MCIMX6UL_EVKB)
   //Configure SNVS_TAMPER6 pin as GPIO5_IO06
   IOMUXC_SetPinMux(IOMUXC_SNVS_TAMPER6_GPIO5_IO06, 0);

   //Set SNVS_TAMPER5 pad properties
   IOMUXC_SetPinConfig(IOMUXC_SNVS_TAMPER6_GPIO5_IO06,
      IOMUXC_SW_PAD_CTL_PAD_HYS(0) |
      IOMUXC_SW_PAD_CTL_PAD_PUS(2) |
      IOMUXC_SW_PAD_CTL_PAD_PUE(1) |
      IOMUXC_SW_PAD_CTL_PAD_PKE(1) |
      IOMUXC_SW_PAD_CTL_PAD_ODE(0) |
      IOMUXC_SW_PAD_CTL_PAD_SPEED(0) |
      IOMUXC_SW_PAD_CTL_PAD_DSE(0) |
      IOMUXC_SW_PAD_CTL_PAD_SRE(0));

#elif defined(USE_MCIMX6ULL_EVK)
   //Configure SNVS_TAMPER6 pin as GPIO5_IO06
   IOMUXC_SetPinMux(IOMUXC_SNVS_SNVS_TAMPER6_GPIO5_IO06, 0);

   //Set SNVS_TAMPER5 pad properties
   IOMUXC_SetPinConfig(IOMUXC_SNVS_SNVS_TAMPER6_GPIO5_IO06,
      IOMUXC_SW_PAD_CTL_PAD_HYS(0) |
      IOMUXC_SW_PAD_CTL_PAD_PUS(2) |
      IOMUXC_SW_PAD_CTL_PAD_PUE(1) |
      IOMUXC_SW_PAD_CTL_PAD_PKE(1) |
      IOMUXC_SW_PAD_CTL_PAD_ODE(0) |
      IOMUXC_SW_PAD_CTL_PAD_SPEED(0) |
      IOMUXC_SW_PAD_CTL_PAD_DSE(0) |
      IOMUXC_SW_PAD_CTL_PAD_SRE(0));
#endif

   //Configure ENET2_INT as an input
   pinConfig.direction = kGPIO_DigitalInput;
   pinConfig.outputLogic = 0;
   pinConfig.interruptMode = kGPIO_NoIntmode;
   GPIO_PinInit(GPIO5, 6, &pinConfig);
}

#endif


/**
 * @brief Initialize buffer descriptors
 * @param[in] interface Underlying network interface
 **/

void mcimx6ulEth2InitBufferDesc(NetInterface *interface)
{
   uint_t i;
   uint32_t address;

   //Clear TX and RX buffer descriptors
   osMemset(txBufferDesc, 0, sizeof(txBufferDesc));
   osMemset(rxBufferDesc, 0, sizeof(rxBufferDesc));

   //Initialize TX buffer descriptors
   for(i = 0; i < MCIMX6UL_ETH2_TX_BUFFER_COUNT; i++)
   {
      //Calculate the address of the current TX buffer
      address = (uint32_t) txBuffer[i];
      //Transmit buffer address
      txBufferDesc[i][1] = address;
      //Generate interrupts
      txBufferDesc[i][2] = ENET_TBD2_INT;
   }

   //Mark the last descriptor entry with the wrap flag
   txBufferDesc[i - 1][0] |= ENET_TBD0_W;
   //Initialize TX buffer index
   txBufferIndex = 0;

   //Initialize RX buffer descriptors
   for(i = 0; i < MCIMX6UL_ETH2_RX_BUFFER_COUNT; i++)
   {
      //Calculate the address of the current RX buffer
      address = (uint32_t) rxBuffer[i];
      //The descriptor is initially owned by the DMA
      rxBufferDesc[i][0] = ENET_RBD0_E;
      //Receive buffer address
      rxBufferDesc[i][1] = address;
      //Generate interrupts
      rxBufferDesc[i][2] = ENET_RBD2_INT;
   }

   //Mark the last descriptor entry with the wrap flag
   rxBufferDesc[i - 1][0] |= ENET_RBD0_W;
   //Initialize RX buffer index
   rxBufferIndex = 0;

   //Start location of the TX descriptor list
   ENET2->TDSR = (uint32_t) txBufferDesc;
   //Start location of the RX descriptor list
   ENET2->RDSR = (uint32_t) rxBufferDesc;
   //Maximum receive buffer size
   ENET2->MRBR = MCIMX6UL_ETH2_RX_BUFFER_SIZE;
}


/**
 * @brief i.MX6UL Ethernet MAC timer handler
 *
 * This routine is periodically called by the TCP/IP stack to handle periodic
 * operations such as polling the link state
 *
 * @param[in] interface Underlying network interface
 **/

void mcimx6ulEth2Tick(NetInterface *interface)
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

void mcimx6ulEth2EnableIrq(NetInterface *interface)
{
   //Enable Ethernet MAC interrupts
   GIC_EnableIRQ(ENET2_IRQn);

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

void mcimx6ulEth2DisableIrq(NetInterface *interface)
{
   //Disable Ethernet MAC interrupts
   GIC_DisableIRQ(ENET2_IRQn);

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
 * @brief Ethernet MAC interrupt (ENET2 instance)
 * @param[in] giccIar Value of the GICC_IAR register
 * @param[in] userParam User parameter
 **/

void ENET2_DriverIRQHandler(uint32_t giccIar, void *userParam)
{
   bool_t flag;
   uint32_t events;

   //Interrupt service routine prologue
   osEnterIsr();

   //This flag will be set if a higher priority task must be woken
   flag = FALSE;
   //Read interrupt event register
   events = ENET2->EIR;

   //Packet transmitted?
   if((events & ENET_EIR_TXF_MASK) != 0)
   {
      //Clear TXF interrupt flag
      ENET2->EIR = ENET_EIR_TXF_MASK;

      //Check whether the TX buffer is available for writing
      if((txBufferDesc[txBufferIndex][0] & ENET_TBD0_R) == 0)
      {
         //Notify the TCP/IP stack that the transmitter is ready to send
         flag = osSetEventFromIsr(&nicDriverInterface->nicTxEvent);
      }

      //Instruct the DMA to poll the transmit descriptor list
      ENET2->TDAR = ENET_TDAR_TDAR_MASK;
   }

   //Packet received?
   if((events & ENET_EIR_RXF_MASK) != 0)
   {
      //Disable RXF interrupt
      ENET2->EIMR &= ~ENET_EIMR_RXF_MASK;

      //Set event flag
      nicDriverInterface->nicEvent = TRUE;
      //Notify the TCP/IP stack of the event
      flag = osSetEventFromIsr(&netEvent);
   }

   //System bus error?
   if((events & ENET_EIR_EBERR_MASK) != 0)
   {
      //Disable EBERR interrupt
      ENET2->EIMR &= ~ENET_EIMR_EBERR_MASK;

      //Set event flag
      nicDriverInterface->nicEvent = TRUE;
      //Notify the TCP/IP stack of the event
      flag |= osSetEventFromIsr(&netEvent);
   }

   //Interrupt service routine epilogue
   osExitIsr(flag);
}


/**
 * @brief i.MX6UL Ethernet MAC event handler
 * @param[in] interface Underlying network interface
 **/

void mcimx6ulEth2EventHandler(NetInterface *interface)
{
   error_t error;
   uint32_t status;

   //Read interrupt event register
   status = ENET2->EIR;

   //Packet received?
   if((status & ENET_EIR_RXF_MASK) != 0)
   {
      //Clear RXF interrupt flag
      ENET2->EIR = ENET_EIR_RXF_MASK;

      //Process all pending packets
      do
      {
         //Read incoming packet
         error = mcimx6ulEth2ReceivePacket(interface);

         //No more data in the receive buffer?
      } while(error != ERROR_BUFFER_EMPTY);
   }

   //System bus error?
   if((status & ENET_EIR_EBERR_MASK) != 0)
   {
      //Clear EBERR interrupt flag
      ENET2->EIR = ENET_EIR_EBERR_MASK;

      //Disable Ethernet MAC
      ENET2->ECR &= ~ENET_ECR_ETHEREN_MASK;
      //Reset buffer descriptors
      mcimx6ulEth2InitBufferDesc(interface);
      //Resume normal operation
      ENET2->ECR |= ENET_ECR_ETHEREN_MASK;
      //Instruct the DMA to poll the receive descriptor list
      ENET2->RDAR = ENET_RDAR_RDAR_MASK;
   }

   //Re-enable Ethernet MAC interrupts
   ENET2->EIMR = ENET_EIMR_TXF_MASK | ENET_EIMR_RXF_MASK | ENET_EIMR_EBERR_MASK;
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

error_t mcimx6ulEth2SendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary)
{
   static uint8_t temp[MCIMX6UL_ETH2_TX_BUFFER_SIZE];
   size_t length;

   //Retrieve the length of the packet
   length = netBufferGetLength(buffer) - offset;

   //Check the frame length
   if(length > MCIMX6UL_ETH2_TX_BUFFER_SIZE)
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
      //Report an error
      return ERROR_INVALID_LENGTH;
   }

   //Make sure the current buffer is available for writing
   if((txBufferDesc[txBufferIndex][0] & ENET_TBD0_R) != 0)
   {
      return ERROR_FAILURE;
   }

   //Copy user data to the transmit buffer
   netBufferRead(temp, buffer, offset, length);
   osMemcpy(txBuffer[txBufferIndex], temp, (length + 3) & ~3UL);

   //Clear BDU flag
   txBufferDesc[txBufferIndex][4] = 0;

   //Check current index
   if(txBufferIndex < (MCIMX6UL_ETH2_TX_BUFFER_COUNT - 1))
   {
      //Give the ownership of the descriptor to the DMA engine
      txBufferDesc[txBufferIndex][0] = ENET_TBD0_R | ENET_TBD0_L |
         ENET_TBD0_TC | (length & ENET_TBD0_DATA_LENGTH);

      //Point to the next buffer
      txBufferIndex++;
   }
   else
   {
      //Give the ownership of the descriptor to the DMA engine
      txBufferDesc[txBufferIndex][0] = ENET_TBD0_R | ENET_TBD0_W |
         ENET_TBD0_L | ENET_TBD0_TC | (length & ENET_TBD0_DATA_LENGTH);

      //Wrap around
      txBufferIndex = 0;
   }

   //Data synchronization barrier
   __DSB();

   //Instruct the DMA to poll the transmit descriptor list
   ENET2->TDAR = ENET_TDAR_TDAR_MASK;

   //Check whether the next buffer is available for writing
   if((txBufferDesc[txBufferIndex][0] & ENET_TBD0_R) == 0)
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

error_t mcimx6ulEth2ReceivePacket(NetInterface *interface)
{
   static uint8_t temp[MCIMX6UL_ETH2_RX_BUFFER_SIZE];
   error_t error;
   size_t n;
   NetRxAncillary ancillary;

   //Current buffer available for reading?
   if((rxBufferDesc[rxBufferIndex][0] & ENET_RBD0_E) == 0)
   {
      //The frame should not span multiple buffers
      if((rxBufferDesc[rxBufferIndex][0] & ENET_RBD0_L) != 0)
      {
         //Check whether an error occurred
         if((rxBufferDesc[rxBufferIndex][0] & (ENET_RBD0_LG | ENET_RBD0_NO |
            ENET_RBD0_CR | ENET_RBD0_OV | ENET_RBD0_TR)) == 0)
         {
            //Retrieve the length of the frame
            n = rxBufferDesc[rxBufferIndex][0] & ENET_RBD0_DATA_LENGTH;
            //Limit the number of data to read
            n = MIN(n, MCIMX6UL_ETH2_RX_BUFFER_SIZE);

            //Copy data from the receive buffer
            osMemcpy(temp, rxBuffer[rxBufferIndex], (n + 3) & ~3UL);

            //Additional options can be passed to the stack along with the packet
            ancillary = NET_DEFAULT_RX_ANCILLARY;

            //Pass the packet to the upper layer
            nicProcessPacket(interface, temp, n, &ancillary);

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
      rxBufferDesc[rxBufferIndex][4] = 0;

      //Check current index
      if(rxBufferIndex < (MCIMX6UL_ETH2_RX_BUFFER_COUNT - 1))
      {
         //Give the ownership of the descriptor back to the DMA engine
         rxBufferDesc[rxBufferIndex][0] = ENET_RBD0_E;
         //Point to the next buffer
         rxBufferIndex++;
      }
      else
      {
         //Give the ownership of the descriptor back to the DMA engine
         rxBufferDesc[rxBufferIndex][0] = ENET_RBD0_E | ENET_RBD0_W;
         //Wrap around
         rxBufferIndex = 0;
      }

      //Instruct the DMA to poll the receive descriptor list
      ENET2->RDAR = ENET_RDAR_RDAR_MASK;
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

error_t mcimx6ulEth2UpdateMacAddrFilter(NetInterface *interface)
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
   ENET2->PAUR = ENET_PAUR_PADDR2(value) | ENET_PAUR_TYPE(0x8808);

   //Set the MAC address of the station (lower 32 bits)
   value = interface->macAddr.b[3];
   value |= (interface->macAddr.b[2] << 8);
   value |= (interface->macAddr.b[1] << 16);
   value |= (interface->macAddr.b[0] << 24);
   ENET2->PALR = ENET_PALR_PADDR1(value);

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
         crc = mcimx6ulEth2CalcCrc(&entry->addr, sizeof(MacAddr));

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
   ENET2->IALR = unicastHashTable[0];
   ENET2->IAUR = unicastHashTable[1];

   //Write the hash table (multicast address filtering)
   ENET2->GALR = multicastHashTable[0];
   ENET2->GAUR = multicastHashTable[1];

   //Debug message
   TRACE_DEBUG("  IALR = %08" PRIX32 "\r\n", ENET2->IALR);
   TRACE_DEBUG("  IAUR = %08" PRIX32 "\r\n", ENET2->IAUR);
   TRACE_DEBUG("  GALR = %08" PRIX32 "\r\n", ENET2->GALR);
   TRACE_DEBUG("  GAUR = %08" PRIX32 "\r\n", ENET2->GAUR);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Adjust MAC configuration parameters for proper operation
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t mcimx6ulEth2UpdateMacConfig(NetInterface *interface)
{
   //Disable Ethernet MAC while modifying configuration registers
   ENET2->ECR &= ~ENET_ECR_ETHEREN_MASK;

   //10BASE-T or 100BASE-TX operation mode?
   if(interface->linkSpeed == NIC_LINK_SPEED_100MBPS)
   {
      //100 Mbps operation
      ENET2->RCR &= ~ENET_RCR_RMII_10T_MASK;
   }
   else
   {
      //10 Mbps operation
      ENET2->RCR |= ENET_RCR_RMII_10T_MASK;
   }

   //Half-duplex or full-duplex mode?
   if(interface->duplexMode == NIC_FULL_DUPLEX_MODE)
   {
      //Full-duplex mode
      ENET2->TCR |= ENET_TCR_FDEN_MASK;
      //Receive path operates independently of transmit
      ENET2->RCR &= ~ENET_RCR_DRT_MASK;
   }
   else
   {
      //Half-duplex mode
      ENET2->TCR &= ~ENET_TCR_FDEN_MASK;
      //Disable reception of frames while transmitting
      ENET2->RCR |= ENET_RCR_DRT_MASK;
   }

   //Reset buffer descriptors
   mcimx6ulEth2InitBufferDesc(interface);

   //Re-enable Ethernet MAC
   ENET2->ECR |= ENET_ECR_ETHEREN_MASK;
   //Instruct the DMA to poll the receive descriptor list
   ENET2->RDAR = ENET_RDAR_RDAR_MASK;

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

void mcimx6ulEth2WritePhyReg(uint8_t opcode, uint8_t phyAddr,
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
      ENET1->EIR = ENET_EIR_MII_MASK;
      //Start a write operation
      ENET1->MMFR = temp;

      //Wait for the write to complete
      while((ENET1->EIR & ENET_EIR_MII_MASK) == 0)
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

uint16_t mcimx6ulEth2ReadPhyReg(uint8_t opcode, uint8_t phyAddr,
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
      ENET1->EIR = ENET_EIR_MII_MASK;
      //Start a read operation
      ENET1->MMFR = temp;

      //Wait for the read to complete
      while((ENET1->EIR & ENET_EIR_MII_MASK) == 0)
      {
      }

      //Get register value
      data = ENET1->MMFR & ENET_MMFR_DATA_MASK;
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

uint32_t mcimx6ulEth2CalcCrc(const void *data, size_t length)
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
