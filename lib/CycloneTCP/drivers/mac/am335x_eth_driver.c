/**
 * @file am335x_eth_driver.c
 * @brief Sitara AM335x Gigabit Ethernet MAC driver
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
#include "soc_am335x.h"
#include "hw_types.h"
#include "hw_cm_per.h"
#include "hw_control_am335x.h"
#include "hw_cpsw_ale.h"
#include "hw_cpsw_cpdma.h"
#include "hw_cpsw_cpdma_stateram.h"
#include "hw_cpsw_port.h"
#include "hw_cpsw_sl.h"
#include "hw_cpsw_ss.h"
#include "hw_cpsw_wr.h"
#include "hw_mdio.h"
#include "interrupt.h"
#include "core/net.h"
#include "drivers/mac/am335x_eth_driver.h"
#include "debug.h"

//MDIO input clock frequency
#define MDIO_INPUT_CLK 125000000
//MDIO output clock frequency
#define MDIO_OUTPUT_CLK 1000000

//Underlying network interface (port 1)
static NetInterface *nicDriverInterface1 = NULL;
//Underlying network interface (port 2)
static NetInterface *nicDriverInterface2 = NULL;

//IAR EWARM compiler?
#if defined(__ICCARM__)

//Transmit buffer (port 1)
#pragma data_alignment = 4
#pragma location = AM335X_ETH_RAM_SECTION
static uint8_t txBuffer1[AM335X_ETH_TX_BUFFER_COUNT][AM335X_ETH_TX_BUFFER_SIZE];
//Transmit buffer (port 2)
#pragma data_alignment = 4
#pragma location = AM335X_ETH_RAM_SECTION
static uint8_t txBuffer2[AM335X_ETH_TX_BUFFER_COUNT][AM335X_ETH_TX_BUFFER_SIZE];
//Receive buffer
#pragma data_alignment = 4
#pragma location = AM335X_ETH_RAM_SECTION
static uint8_t rxBuffer[AM335X_ETH_RX_BUFFER_COUNT][AM335X_ETH_RX_BUFFER_SIZE];
//Transmit buffer descriptors (port 1)
#pragma data_alignment = 4
#pragma location = AM335X_ETH_RAM_CPPI_SECTION
static Am335xTxBufferDesc txBufferDesc1[AM335X_ETH_TX_BUFFER_COUNT];
//Transmit buffer descriptors (port 2)
#pragma data_alignment = 4
#pragma location = AM335X_ETH_RAM_CPPI_SECTION
static Am335xTxBufferDesc txBufferDesc2[AM335X_ETH_TX_BUFFER_COUNT];
//Receive buffer descriptors
#pragma data_alignment = 4
#pragma location = AM335X_ETH_RAM_CPPI_SECTION
static Am335xRxBufferDesc rxBufferDesc[AM335X_ETH_RX_BUFFER_COUNT];

//GCC compiler?
#else

//Transmit buffer (port 1)
static uint8_t txBuffer1[AM335X_ETH_TX_BUFFER_COUNT][AM335X_ETH_TX_BUFFER_SIZE]
   __attribute__((aligned(4), __section__(AM335X_ETH_RAM_SECTION)));
//Transmit buffer (port 2)
static uint8_t txBuffer2[AM335X_ETH_TX_BUFFER_COUNT][AM335X_ETH_TX_BUFFER_SIZE]
   __attribute__((aligned(4), __section__(AM335X_ETH_RAM_SECTION)));
//Receive buffer
static uint8_t rxBuffer[AM335X_ETH_RX_BUFFER_COUNT][AM335X_ETH_RX_BUFFER_SIZE]
   __attribute__((aligned(4), __section__(AM335X_ETH_RAM_SECTION)));
//Transmit buffer descriptors (port 1)
static Am335xTxBufferDesc txBufferDesc1[AM335X_ETH_TX_BUFFER_COUNT]
   __attribute__((aligned(4), __section__(AM335X_ETH_RAM_CPPI_SECTION)));
//Transmit buffer descriptors (port 2)
static Am335xTxBufferDesc txBufferDesc2[AM335X_ETH_TX_BUFFER_COUNT]
   __attribute__((aligned(4), __section__(AM335X_ETH_RAM_CPPI_SECTION)));
//Receive buffer descriptors
static Am335xRxBufferDesc rxBufferDesc[AM335X_ETH_RX_BUFFER_COUNT]
   __attribute__((aligned(4), __section__(AM335X_ETH_RAM_CPPI_SECTION)));

#endif

//Pointer to the current TX buffer descriptor (port1)
static Am335xTxBufferDesc *txCurBufferDesc1;
//Pointer to the current TX buffer descriptor (port 2)
static Am335xTxBufferDesc *txCurBufferDesc2;
//Pointer to the current RX buffer descriptor
static Am335xRxBufferDesc *rxCurBufferDesc;


/**
 * @brief AM335x Ethernet MAC driver (port1)
 **/

const NicDriver am335xEthPort1Driver =
{
   NIC_TYPE_ETHERNET,
   ETH_MTU,
   am335xEthInitPort1,
   am335xEthTick,
   am335xEthEnableIrq,
   am335xEthDisableIrq,
   am335xEthEventHandler,
   am335xEthSendPacketPort1,
   am335xEthUpdateMacAddrFilter,
   am335xEthUpdateMacConfig,
   am335xEthWritePhyReg,
   am335xEthReadPhyReg,
   FALSE,
   TRUE,
   TRUE,
   FALSE
};


/**
 * @brief AM335x Ethernet MAC driver (port2)
 **/

const NicDriver am335xEthPort2Driver =
{
   NIC_TYPE_ETHERNET,
   ETH_MTU,
   am335xEthInitPort2,
   am335xEthTick,
   am335xEthEnableIrq,
   am335xEthDisableIrq,
   am335xEthEventHandler,
   am335xEthSendPacketPort2,
   am335xEthUpdateMacAddrFilter,
   am335xEthUpdateMacConfig,
   am335xEthWritePhyReg,
   am335xEthReadPhyReg,
   FALSE,
   TRUE,
   TRUE,
   FALSE
};


/**
 * @brief AM335x Ethernet MAC initialization (port 1)
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t am335xEthInitPort1(NetInterface *interface)
{
   error_t error;
   uint32_t temp;

   //Debug message
   TRACE_INFO("Initializing AM335x Ethernet MAC (port 1)...\r\n");

   //Initialize CPSW instance
   am335xEthInitInstance(interface);

   //Save underlying network interface
   nicDriverInterface1 = interface;

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

   //Unspecifield MAC address?
   if(macCompAddr(&interface->macAddr, &MAC_UNSPECIFIED_ADDR))
   {
      //Use the factory preprogrammed MAC address
      interface->macAddr.b[0] = CONTROL_MAC_ID_HI_R(0) >> CONTROL_MAC_ID0_HI_MACADDR_47_40_SHIFT;
      interface->macAddr.b[1] = CONTROL_MAC_ID_HI_R(0) >> CONTROL_MAC_ID0_HI_MACADDR_39_32_SHIFT;
      interface->macAddr.b[2] = CONTROL_MAC_ID_HI_R(0) >> CONTROL_MAC_ID0_HI_MACADDR_31_24_SHIFT;
      interface->macAddr.b[3] = CONTROL_MAC_ID_HI_R(0) >> CONTROL_MAC_ID0_HI_MACADDR_23_16_SHIFT;
      interface->macAddr.b[4] = CONTROL_MAC_ID_LO_R(0) >> CONTROL_MAC_ID0_LO_MACADDR_15_8_SHIFT;
      interface->macAddr.b[5] = CONTROL_MAC_ID_LO_R(0) >> CONTROL_MAC_ID0_LO_MACADDR_7_0_SHIFT;

      //Generate the 64-bit interface identifier
      macAddrToEui64(&interface->macAddr, &interface->eui64);
   }

   //Set port state to forward
   temp = CPSW_ALE_PORTCTL_R(1) & ~CPSW_ALE_PORTCTL_PORT_STATE_MASK;
   CPSW_ALE_PORTCTL_R(1) = temp | CPSW_ALE_PORTCTL_PORT_STATE_FORWARD;

   //Set the MAC address of the station
   CPSW_PORT1_SA_HI_R = interface->macAddr.w[0] | (interface->macAddr.w[1] << 16);
   CPSW_PORT1_SA_LO_R = interface->macAddr.w[2];

   //Configure VLAN identifier and VLAN priority
   CPSW_PORT1_VLAN_R = (0 << CPSW_PORT_P_VLAN_PRI_SHIFT) |
      (CPSW_PORT1 << CPSW_PORT_P_VLAN_VID_SHIFT);

   //Add a VLAN entry in the ALE table
   am335xEthAddVlanEntry(CPSW_PORT1, CPSW_PORT1);

   //Add a VLAN/unicast address entry in the ALE table
   am335xEthAddVlanAddrEntry(CPSW_PORT1, CPSW_PORT1, &interface->macAddr);

   //Enable CPSW statistics
   CPSW_SS_STAT_PORT_EN_R |= CPSW_SS_STAT_PORT_EN_P1_MASK;

   //Enable TX and RX
   CPSW_SL1_MACCTRL_R = CPSW_SL_MACCTRL_GMII_EN_MASK;

   //Accept any packets from the upper layer
   osSetEvent(&interface->nicTxEvent);

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief AM335x Ethernet MAC initialization (port 2)
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t am335xEthInitPort2(NetInterface *interface)
{
   error_t error;
   uint32_t temp;

   //Debug message
   TRACE_INFO("Initializing AM335x Ethernet MAC (port 2)...\r\n");

   //Initialize CPSW instance
   am335xEthInitInstance(interface);

   //Save underlying network interface
   nicDriverInterface2 = interface;

   //PHY transceiver initialization
   error = interface->phyDriver->init(interface);
   //Failed to initialize PHY transceiver?
   if(error)
   {
      return error;
   }

   //Unspecifield MAC address?
   if(macCompAddr(&interface->macAddr, &MAC_UNSPECIFIED_ADDR))
   {
      //Use the factory preprogrammed MAC address
      interface->macAddr.b[0] = CONTROL_MAC_ID_HI_R(1) >> CONTROL_MAC_ID1_HI_MACADDR_47_40_SHIFT;
      interface->macAddr.b[1] = CONTROL_MAC_ID_HI_R(1) >> CONTROL_MAC_ID1_HI_MACADDR_39_32_SHIFT;
      interface->macAddr.b[2] = CONTROL_MAC_ID_HI_R(1) >> CONTROL_MAC_ID1_HI_MACADDR_31_24_SHIFT;
      interface->macAddr.b[3] = CONTROL_MAC_ID_HI_R(1) >> CONTROL_MAC_ID1_HI_MACADDR_23_16_SHIFT;
      interface->macAddr.b[4] = CONTROL_MAC_ID_LO_R(1) >> CONTROL_MAC_ID1_LO_MACADDR_15_8_SHIFT;
      interface->macAddr.b[5] = CONTROL_MAC_ID_LO_R(1) >> CONTROL_MAC_ID1_LO_MACADDR_7_0_SHIFT;

      //Generate the 64-bit interface identifier
      macAddrToEui64(&interface->macAddr, &interface->eui64);
   }

   //Set port state to forward
   temp = CPSW_ALE_PORTCTL_R(2) & ~CPSW_ALE_PORTCTL_PORT_STATE_MASK;
   CPSW_ALE_PORTCTL_R(2) = temp | CPSW_ALE_PORTCTL_PORT_STATE_FORWARD;

   //Set the MAC address of the station
   CPSW_PORT2_SA_HI_R = interface->macAddr.w[0] | (interface->macAddr.w[1] << 16);
   CPSW_PORT2_SA_LO_R = interface->macAddr.w[2];

   //Configure VLAN identifier and VLAN priority
   CPSW_PORT2_VLAN_R = (0 << CPSW_PORT_P_VLAN_PRI_SHIFT) |
      (CPSW_PORT2 << CPSW_PORT_P_VLAN_VID_SHIFT);

   //Add a VLAN entry in the ALE table
   am335xEthAddVlanEntry(CPSW_PORT2, CPSW_PORT2);

   //Add a VLAN/unicast address entry in the ALE table
   am335xEthAddVlanAddrEntry(CPSW_PORT2, CPSW_PORT2, &interface->macAddr);

   //Enable CPSW statistics
   CPSW_SS_STAT_PORT_EN_R |= CPSW_SS_STAT_PORT_EN_P2_MASK;

   //Enable TX and RX
   CPSW_SL2_MACCTRL_R = CPSW_SL_MACCTRL_GMII_EN_MASK;

   //Accept any packets from the upper layer
   osSetEvent(&interface->nicTxEvent);

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief Initialize CPSW instance
 * @param[in] interface Underlying network interface
 **/

void am335xEthInitInstance(NetInterface *interface)
{
   uint_t i;
   uint32_t temp;
#ifdef ti_sysbios_BIOS___VERS
   Hwi_Params hwiParams;
#endif

   //Initialization sequence is performed once
   if(nicDriverInterface1 == NULL && nicDriverInterface2 == NULL)
   {
      //Select the interface mode (MII/RMII/RGMII) and configure pin muxing
      am335xEthInitGpio(interface);

      //Enable the CPSW subsystem clocks
      CM_PER_CPGMAC0_CLKCTRL_R = CM_PER_CPGMAC0_CLKCTRL_MODULEMODE_ENABLE;

      //Wait for the CPSW module to be fully functional
      do
      {
         //Get module idle status
         temp = (CM_PER_CPGMAC0_CLKCTRL_R & CM_PER_CPGMAC0_CLKCTRL_IDLEST) >>
            CM_PER_CPGMAC0_CLKCTRL_IDLEST_SHIFT;

         //Keep looping as long as the module is not fully functional
      } while(temp != CM_PER_CPGMAC0_CLKCTRL_IDLEST_FUNC);

      //Start a software forced wake-up transition
      CM_PER_CPSW_CLKSTCTRL_R = CM_PER_CPSW_CLKSTCTRL_CLKTRCTRL_SW_WKUP;

      //Wait for the CPSW 125 MHz OCP clock to be active
      do
      {
         //Get the state of the CPSW 125 MHz OCP clock
         temp = (CM_PER_CPSW_CLKSTCTRL_R & CM_PER_CPSW_CLKSTCTRL_CLKACTIVITY_CPSW_125MHZ_GCLK) >>
            CM_PER_CPSW_CLKSTCTRL_CLKACTIVITY_CPSW_125MHZ_GCLK_SHIFT;

         //Keep looping as long as the clock is inactive
      } while(temp != CM_PER_CPSW_CLKSTCTRL_CLKACTIVITY_CPSW_125MHZ_GCLK_ACT);

      //Reset CPSW subsystem
      CPSW_SS_SOFT_RESET_R = CPSW_SS_SOFT_RESET_MASK;
      //Wait for the reset to complete
      while((CPSW_SS_SOFT_RESET_R & CPSW_SS_SOFT_RESET_MASK) != 0)
      {
      }

      //Reset CPSW wrapper module
      CPSW_WR_SOFT_RESET_R = CPSW_WR_SOFT_RESET_MASK;
      //Wait for the reset to complete
      while((CPSW_WR_SOFT_RESET_R & CPSW_WR_SOFT_RESET_MASK) != 0)
      {
      }

      //Reset CPSW sliver 1 logic
      CPSW_SL1_SOFT_RESET_R = CPSW_SL_SOFT_RESET_MASK;
      //Wait for the reset to complete
      while((CPSW_SL1_SOFT_RESET_R & CPSW_SL_SOFT_RESET_MASK) != 0)
      {
      }

      //Reset CPSW sliver 2 logic
      CPSW_SL2_SOFT_RESET_R = CPSW_SL_SOFT_RESET_MASK;
      //Wait for the reset to complete
      while((CPSW_SL2_SOFT_RESET_R & CPSW_SL_SOFT_RESET_MASK) != 0)
      {
      }

      //Reset CPSW CPDMA module
      CPSW_CPDMA_SOFT_RESET_R = CPSW_CPDMA_SOFT_RESET_MASK;
      //Wait for the reset to complete
      while((CPSW_CPDMA_SOFT_RESET_R & CPSW_CPDMA_SOFT_RESET_MASK) != 0)
      {
      }

      //Initialize the HDPs and the CPs to NULL
      for(i = CPSW_CH0; i <= CPSW_CH7; i++)
      {
         //TX head descriptor pointer
         CPSW_CPDMA_STATERAM_TX_HDP_R(i) = 0;
         //TX completion pointer
         CPSW_CPDMA_STATERAM_TX_CP_R(i) = 0;
         //RX head descriptor pointer
         CPSW_CPDMA_STATERAM_RX_HDP_R(i) = 0;
         //RX completion pointer
         CPSW_CPDMA_STATERAM_RX_CP_R(i) = 0;
      }

      //Enable ALE and clear ALE address table
      CPSW_ALE_CTRL_R = CPSW_ALE_CTRL_EN_MASK | CPSW_ALE_CTRL_CLR_TBL_MASK;

      //For dual MAC mode, configure VLAN aware mode
      CPSW_ALE_CTRL_R |= CPSW_ALE_CTRL_VLAN_AWARE_MASK;

      //Set dual MAC mode for port 0
      temp = CPSW_PORT0_TX_IN_CTL_R & ~CPSW_PORT_P_TX_IN_CTL_SEL_MASK;
      CPSW_PORT0_TX_IN_CTL_R = temp | CPSW_PORT_P_TX_IN_CTL_SEL_DUAL_MAC;

      //Set port 0 state to forward
      temp = CPSW_ALE_PORTCTL_R(0) & ~CPSW_ALE_PORTCTL_PORT_STATE_MASK;
      CPSW_ALE_PORTCTL_R(0) = temp | CPSW_ALE_PORTCTL_PORT_STATE_FORWARD;

      //Enable CPSW statistics
      CPSW_SS_STAT_PORT_EN_R = CPSW_SS_STAT_PORT_EN_P0_MASK;

      //Configure TX and RX buffer descriptors
      am335xEthInitBufferDesc(interface);

      //Acknowledge TX and interrupts for proper interrupt pulsing
      CPSW_CPDMA_EOI_VECTOR_R = CPSW_CPDMA_EOI_VECTOR_TX_PULSE;
      CPSW_CPDMA_EOI_VECTOR_R = CPSW_CPDMA_EOI_VECTOR_RX_PULSE;

      //Enable channel 1 and 2 interrupts of the DMA engine
      CPSW_CPDMA_TX_INTMASK_SET_R = (1 << CPSW_CH1) | (1 << CPSW_CH2);
      //Enable TX completion interrupts
      CPSW_WR_C_TX_EN_R(CPSW_CORE0) |= (1 << CPSW_CH1) | (1 << CPSW_CH2);

      //Enable channel 0 interrupts of the DMA engine
      CPSW_CPDMA_RX_INTMASK_SET_R = (1 << CPSW_CH0);
      //Enable RX completion interrupts
      CPSW_WR_C_RX_EN_R(CPSW_CORE0) |= (1 << CPSW_CH0);

#ifdef ti_sysbios_BIOS___VERS
      //Configure TX interrupt
      Hwi_Params_init(&hwiParams);
      hwiParams.enableInt = FALSE;
      hwiParams.priority = AM335X_ETH_IRQ_PRIORITY;

      //Register TX interrupt handler
      Hwi_create(SYS_INT_3PGSWTXINT0, (Hwi_FuncPtr) am335xEthTxIrqHandler,
         &hwiParams, NULL);

      //Configure RX interrupt
      Hwi_Params_init(&hwiParams);
      hwiParams.enableInt = FALSE;
      hwiParams.priority = AM335X_ETH_IRQ_PRIORITY;

      //Register RX interrupt handler
      Hwi_create(SYS_INT_3PGSWRXINT0, (Hwi_FuncPtr) am335xEthRxIrqHandler,
         &hwiParams, NULL);
#else
      //Register interrupt handlers
      IntRegister(SYS_INT_3PGSWTXINT0, am335xEthTxIrqHandler);
      IntRegister(SYS_INT_3PGSWRXINT0, am335xEthRxIrqHandler);

      //Configure TX interrupt priority
      IntPrioritySet(SYS_INT_3PGSWTXINT0, AM335X_ETH_IRQ_PRIORITY,
         AINTC_HOSTINT_ROUTE_IRQ);

      //Configure RX interrupt priority
      IntPrioritySet(SYS_INT_3PGSWRXINT0, AM335X_ETH_IRQ_PRIORITY,
         AINTC_HOSTINT_ROUTE_IRQ);
#endif

      //Enable the transmission and reception
      CPSW_CPDMA_TX_CTRL_R = CPSW_CPDMA_TX_CTRL_EN_MASK;
      CPSW_CPDMA_RX_CTRL_R = CPSW_CPDMA_RX_CTRL_EN_MASK;

      //Calculate the MDC clock divider to be used
      temp = (MDIO_INPUT_CLK / MDIO_OUTPUT_CLK) - 1;

      //Initialize MDIO interface
      MDIO_CTRL_R = MDIO_CTRL_EN_MASK | MDIO_CTRL_FAULTENB_MASK |
         (temp & MDIO_CTRL_CLKDIV_MASK);
   }
}


//BeagleBone Black, TMDSSK3358, OSD3358-SM-RED or SBC DIVA board?
#if defined(USE_BEAGLEBONE_BLACK) || defined(USE_TMDSSK3358) || \
   defined(USE_OSD3358_SM_RED) || defined(USE_SBC_DIVA)

/**
 * @brief GPIO configuration
 * @param[in] interface Underlying network interface
 **/

void am335xEthInitGpio(NetInterface *interface)
{
//BeagleBone Black board?
#if defined(USE_BEAGLEBONE_BLACK)
   //Select MII interface mode for port 1
   CONTROL_GMII_SEL_R = CONTROL_GMII_SEL_GMII1_SEL_MII;

   //Configure MII1_TX_CLK (GPIO3_9)
   CONTROL_CONF_MII1_TXCLK_R = CONTROL_CONF_RXACTIVE | CONTROL_CONF_MUXMODE(0);
   //Configure MII1_TX_EN (GPIO3_3)
   CONTROL_CONF_MII1_TXEN_R = CONTROL_CONF_MUXMODE(0);
   //Configure MII1_TXD0 (GPIO0_28)
   CONTROL_CONF_MII1_TXD0_R = CONTROL_CONF_MUXMODE(0);
   //Configure MII1_TXD1 (GPIO0_21)
   CONTROL_CONF_MII1_TXD1_R = CONTROL_CONF_MUXMODE(0);
   //Configure MII1_TXD2 (GPIO0_17)
   CONTROL_CONF_MII1_TXD2_R = CONTROL_CONF_MUXMODE(0);
   //Configure MII1_TXD3 (GPIO0_16)
   CONTROL_CONF_MII1_TXD3_R = CONTROL_CONF_MUXMODE(0);

   //Configure MII1_RX_CLK (GPIO3_10)
   CONTROL_CONF_MII1_RXCLK_R = CONTROL_CONF_RXACTIVE | CONTROL_CONF_MUXMODE(0);
   //Configure MII1_RXD0 (GPIO2_21)
   CONTROL_CONF_MII1_RXD0_R = CONTROL_CONF_RXACTIVE | CONTROL_CONF_MUXMODE(0);
   //Configure MII1_RXD1 (GPIO2_20)
   CONTROL_CONF_MII1_RXD1_R = CONTROL_CONF_RXACTIVE | CONTROL_CONF_MUXMODE(0);
   //Configure MII1_RXD2 (GPIO2_19)
   CONTROL_CONF_MII1_RXD2_R = CONTROL_CONF_RXACTIVE | CONTROL_CONF_MUXMODE(0);
   //Configure MII1_RXD3 (GPIO2_18)
   CONTROL_CONF_MII1_RXD3_R = CONTROL_CONF_RXACTIVE | CONTROL_CONF_MUXMODE(0);
   //Configure MII1_COL (GPIO3_0)
   CONTROL_CONF_MII1_COL_R = CONTROL_CONF_RXACTIVE | CONTROL_CONF_MUXMODE(0);
   //Configure MII1_CRS (GPIO3_1)
   CONTROL_CONF_MII1_CRS_R = CONTROL_CONF_RXACTIVE | CONTROL_CONF_MUXMODE(0);
   //Configure MII1_RX_ER (GPIO3_2)
   CONTROL_CONF_MII1_RXERR_R = CONTROL_CONF_RXACTIVE | CONTROL_CONF_MUXMODE(0);
   //Configure MII1_RX_DV (GPIO3_4)
   CONTROL_CONF_MII1_RXDV_R = CONTROL_CONF_RXACTIVE | CONTROL_CONF_MUXMODE(0);

   //Configure MDIO (GPIO0_0)
   CONTROL_CONF_MDIO_DATA_R = CONTROL_CONF_RXACTIVE | CONTROL_CONF_PULLUPSEL |
      CONTROL_CONF_MUXMODE(0);

   //Configure MDC (GPIO0_1)
   CONTROL_CONF_MDIO_CLK_R = CONTROL_CONF_PULLUPSEL | CONTROL_CONF_MUXMODE(0);

//TMDSSK3358 board?
#elif defined(USE_TMDSSK3358)
   //Select RGMII interface mode for both port 1 and port 2
   CONTROL_GMII_SEL_R = CONTROL_GMII_SEL_GMII1_SEL_RGMII |
      CONTROL_GMII_SEL_GMII2_SEL_RGMII;

   //Configure RGMII1_TCLK (GPIO3_9)
   CONTROL_CONF_MII1_TXCLK_R = CONTROL_CONF_MUXMODE(2);
   //Configure RGMII1_TCTL (GPIO3_3)
   CONTROL_CONF_MII1_TXEN_R = CONTROL_CONF_MUXMODE(2);
   //Configure RGMII1_TD0 (GPIO0_28)
   CONTROL_CONF_MII1_TXD0_R = CONTROL_CONF_MUXMODE(2);
   //Configure RGMII1_TD1 (GPIO0_21)
   CONTROL_CONF_MII1_TXD1_R = CONTROL_CONF_MUXMODE(2);
   //Configure RGMII1_TD2 (GPIO0_17)
   CONTROL_CONF_MII1_TXD2_R = CONTROL_CONF_MUXMODE(2);
   //Configure RGMII1_TD3 (GPIO0_16)
   CONTROL_CONF_MII1_TXD3_R = CONTROL_CONF_MUXMODE(2);

   //Configure RGMII1_RCLK (GPIO3_10)
   CONTROL_CONF_MII1_RXCLK_R = CONTROL_CONF_RXACTIVE | CONTROL_CONF_MUXMODE(2);
   //Configure RGMII1_RCTL (GPIO3_4)
   CONTROL_CONF_MII1_RXDV_R = CONTROL_CONF_RXACTIVE | CONTROL_CONF_MUXMODE(2);
   //Configure RGMII1_RD0 (GPIO2_21)
   CONTROL_CONF_MII1_RXD0_R = CONTROL_CONF_RXACTIVE | CONTROL_CONF_MUXMODE(2);
   //Configure /RGMII1_RD1 (GPIO2_20)
   CONTROL_CONF_MII1_RXD1_R = CONTROL_CONF_RXACTIVE | CONTROL_CONF_MUXMODE(2);
   //Configure RGMII1_RD2 (GPIO2_19)
   CONTROL_CONF_MII1_RXD2_R = CONTROL_CONF_RXACTIVE | CONTROL_CONF_MUXMODE(2);
   //Configure RGMII1_RD3 (GPIO2_18)
   CONTROL_CONF_MII1_RXD3_R = CONTROL_CONF_RXACTIVE | CONTROL_CONF_MUXMODE(2);

   //Configure RGMII2_TCLK (GPIO1_22/GPMC_A6)
   CONTROL_CONF_GPMC_A_R(6) = CONTROL_CONF_MUXMODE(2);
   //Configure RGMII2_TCTL (GPIO1_16/GPMC_A0)
   CONTROL_CONF_GPMC_A_R(0) = CONTROL_CONF_MUXMODE(2);
   //Configure RGMII2_TD0 (GPIO1_21/GPMC_A5)
   CONTROL_CONF_GPMC_A_R(5) = CONTROL_CONF_MUXMODE(2);
   //Configure RGMII2_TD1 (GPIO1_20/GPMC_A4)
   CONTROL_CONF_GPMC_A_R(4) = CONTROL_CONF_MUXMODE(2);
   //Configure RGMII2_TD2 (GPIO1_19/GPMC_A3)
   CONTROL_CONF_GPMC_A_R(3) = CONTROL_CONF_MUXMODE(2);
   //Configure RGMII2_TD3 (GPIO1_18/GPMC_A2)
   CONTROL_CONF_GPMC_A_R(2) = CONTROL_CONF_MUXMODE(2);

   //Configure RGMII2_RCLK (GPIO1_23/GPMC_A7)
   CONTROL_CONF_GPMC_A_R(7) = CONTROL_CONF_RXACTIVE | CONTROL_CONF_MUXMODE(2);
   //Configure RGMII2_RCTL (GPIO1_17/GPMC_A1)
   CONTROL_CONF_GPMC_A_R(1) = CONTROL_CONF_RXACTIVE | CONTROL_CONF_MUXMODE(2);
   //Configure RGMII2_RD0 (GPIO1_27/GPMC_A11)
   CONTROL_CONF_GPMC_A_R(11) = CONTROL_CONF_RXACTIVE | CONTROL_CONF_MUXMODE(2);
   //Configure RGMII2_RD1 (GPIO1_26/GPMC_A10)
   CONTROL_CONF_GPMC_A_R(10) = CONTROL_CONF_RXACTIVE | CONTROL_CONF_MUXMODE(2);
   //Configure RGMII2_RD2 (GPIO1_25/GPMC_A9)
   CONTROL_CONF_GPMC_A_R(9) = CONTROL_CONF_RXACTIVE | CONTROL_CONF_MUXMODE(2);
   //Configure RGMII2_RD3 (GPIO1_24/GPMC_A8)
   CONTROL_CONF_GPMC_A_R(8) = CONTROL_CONF_RXACTIVE | CONTROL_CONF_MUXMODE(2);

   //Configure MDIO (GPIO0_0)
   CONTROL_CONF_MDIO_DATA_R = CONTROL_CONF_RXACTIVE | CONTROL_CONF_PULLUPSEL |
      CONTROL_CONF_MUXMODE(0);

   //Configure MDC (GPIO0_1)
   CONTROL_CONF_MDIO_CLK_R = CONTROL_CONF_PULLUPSEL | CONTROL_CONF_MUXMODE(0);

//OSD3358-SM-RED board?
#elif defined(USE_OSD3358_SM_RED)
   //Select RGMII interface mode for both port 1
   CONTROL_GMII_SEL_R = CONTROL_GMII_SEL_GMII1_SEL_RGMII;

   //Configure RGMII1_TCLK (GPIO3_9)
   CONTROL_CONF_MII1_TXCLK_R = CONTROL_CONF_MUXMODE(2);
   //Configure RGMII1_TCTL (GPIO3_3)
   CONTROL_CONF_MII1_TXEN_R = CONTROL_CONF_MUXMODE(2);
   //Configure RGMII1_TD0 (GPIO0_28)
   CONTROL_CONF_MII1_TXD0_R = CONTROL_CONF_MUXMODE(2);
   //Configure RGMII1_TD1 (GPIO0_21)
   CONTROL_CONF_MII1_TXD1_R = CONTROL_CONF_MUXMODE(2);
   //Configure RGMII1_TD2 (GPIO0_17)
   CONTROL_CONF_MII1_TXD2_R = CONTROL_CONF_MUXMODE(2);
   //Configure RGMII1_TD3 (GPIO0_16)
   CONTROL_CONF_MII1_TXD3_R = CONTROL_CONF_MUXMODE(2);

   //Configure RGMII1_RCLK (GPIO3_10)
   CONTROL_CONF_MII1_RXCLK_R = CONTROL_CONF_RXACTIVE | CONTROL_CONF_MUXMODE(2);
   //Configure RGMII1_RCTL (GPIO3_4)
   CONTROL_CONF_MII1_RXDV_R = CONTROL_CONF_RXACTIVE | CONTROL_CONF_MUXMODE(2);
   //Configure RGMII1_RD0 (GPIO2_21)
   CONTROL_CONF_MII1_RXD0_R = CONTROL_CONF_RXACTIVE | CONTROL_CONF_MUXMODE(2);
   //Configure /RGMII1_RD1 (GPIO2_20)
   CONTROL_CONF_MII1_RXD1_R = CONTROL_CONF_RXACTIVE | CONTROL_CONF_MUXMODE(2);
   //Configure RGMII1_RD2 (GPIO2_19)
   CONTROL_CONF_MII1_RXD2_R = CONTROL_CONF_RXACTIVE | CONTROL_CONF_MUXMODE(2);
   //Configure RGMII1_RD3 (GPIO2_18)
   CONTROL_CONF_MII1_RXD3_R = CONTROL_CONF_RXACTIVE | CONTROL_CONF_MUXMODE(2);

   //Configure MDIO (GPIO0_0)
   CONTROL_CONF_MDIO_DATA_R = CONTROL_CONF_RXACTIVE | CONTROL_CONF_PULLUPSEL |
      CONTROL_CONF_MUXMODE(0);

   //Configure MDC (GPIO0_1)
   CONTROL_CONF_MDIO_CLK_R = CONTROL_CONF_PULLUPSEL | CONTROL_CONF_MUXMODE(0);

//SBC DIVA board?
#elif defined(USE_SBC_DIVA)
   //Select RMII interface mode for port 1 and RGMII interface mode for port 2
   CONTROL_GMII_SEL_R = CONTROL_GMII_SEL_RMII1_IO_CLK_EN |
      CONTROL_GMII_SEL_GMII1_SEL_RMII | CONTROL_GMII_SEL_GMII2_SEL_RGMII;

   //Configure RMII1_REF_CLK (GPIO0_29)
   CONTROL_CONF_RMII1_REFCLK_R = CONTROL_CONF_RXACTIVE | CONTROL_CONF_MUXMODE(0);

   //Configure RMII1_TX_EN (GPIO3_3)
   CONTROL_CONF_MII1_TXEN_R = CONTROL_CONF_MUXMODE(1);
   //Configure RMII1_TXD0 (GPIO0_28)
   CONTROL_CONF_MII1_TXD0_R = CONTROL_CONF_MUXMODE(1);
   //Configure RMII1_TXD1 (GPIO0_21)
   CONTROL_CONF_MII1_TXD1_R = CONTROL_CONF_MUXMODE(1);

   //Configure RMII1_RXD0 (GPIO2.21)
   CONTROL_CONF_MII1_RXD0_R = CONTROL_CONF_RXACTIVE | CONTROL_CONF_MUXMODE(1);
   //Configure RMII1_RXD1 (GPIO2.20)
   CONTROL_CONF_MII1_RXD1_R = CONTROL_CONF_RXACTIVE | CONTROL_CONF_MUXMODE(1);
   //Configure RMII1_CRS_DV (GPIO3_1)
   CONTROL_CONF_MII1_CRS_R = CONTROL_CONF_RXACTIVE | CONTROL_CONF_MUXMODE(1);
   //Configure RMII1_RX_ER (GPIO3_2)
   CONTROL_CONF_MII1_RXERR_R = CONTROL_CONF_RXACTIVE | CONTROL_CONF_MUXMODE(1);

   //Configure RGMII2_TCLK (GPIO1_22/GPMC_A6)
   CONTROL_CONF_GPMC_A_R(6) = CONTROL_CONF_MUXMODE(2);
   //Configure RGMII2_TCTL (GPIO1_16/GPMC_A0)
   CONTROL_CONF_GPMC_A_R(0) = CONTROL_CONF_MUXMODE(2);
   //Configure RGMII2_TD0 (GPIO1_21/GPMC_A5)
   CONTROL_CONF_GPMC_A_R(5) = CONTROL_CONF_MUXMODE(2);
   //Configure RGMII2_TD1 (GPIO1_20/GPMC_A4)
   CONTROL_CONF_GPMC_A_R(4) = CONTROL_CONF_MUXMODE(2);
   //Configure RGMII2_TD2 (GPIO1_19/GPMC_A3)
   CONTROL_CONF_GPMC_A_R(3) = CONTROL_CONF_MUXMODE(2);
   //Configure RGMII2_TD3 (GPIO1_18/GPMC_A2)
   CONTROL_CONF_GPMC_A_R(2) = CONTROL_CONF_MUXMODE(2);

   //Configure RGMII2_RCLK (GPIO1_23/GPMC_A7)
   CONTROL_CONF_GPMC_A_R(7) = CONTROL_CONF_RXACTIVE | CONTROL_CONF_MUXMODE(2);
   //Configure RGMII2_RCTL (GPIO1_17/GPMC_A1)
   CONTROL_CONF_GPMC_A_R(1) = CONTROL_CONF_RXACTIVE | CONTROL_CONF_MUXMODE(2);
   //Configure RGMII2_RD0 (GPIO1_27/GPMC_A11)
   CONTROL_CONF_GPMC_A_R(11) = CONTROL_CONF_RXACTIVE | CONTROL_CONF_MUXMODE(2);
   //Configure RGMII2_RD1 (GPIO1_26/GPMC_A10)
   CONTROL_CONF_GPMC_A_R(10) = CONTROL_CONF_RXACTIVE | CONTROL_CONF_MUXMODE(2);
   //Configure RGMII2_RD2 (GPIO1_25/GPMC_A9)
   CONTROL_CONF_GPMC_A_R(9) = CONTROL_CONF_RXACTIVE | CONTROL_CONF_MUXMODE(2);
   //Configure RGMII2_RD3 (GPIO1_24/GPMC_A8)
   CONTROL_CONF_GPMC_A_R(8) = CONTROL_CONF_RXACTIVE | CONTROL_CONF_MUXMODE(2);

   //Configure MDIO (GPIO0_0)
   CONTROL_CONF_MDIO_DATA_R = CONTROL_CONF_RXACTIVE | CONTROL_CONF_PULLUPSEL |
      CONTROL_CONF_MUXMODE(0);

   //Configure MDC (GPIO0_1)
   CONTROL_CONF_MDIO_CLK_R = CONTROL_CONF_PULLUPSEL | CONTROL_CONF_MUXMODE(0);
#endif
}

#endif


/**
 * @brief Initialize buffer descriptor lists
 * @param[in] interface Underlying network interface
 **/

void am335xEthInitBufferDesc(NetInterface *interface)
{
   uint_t i;
   uint_t nextIndex;
   uint_t prevIndex;

   //Initialize TX buffer descriptor list (port 1)
   for(i = 0; i < AM335X_ETH_TX_BUFFER_COUNT; i++)
   {
      //Index of the next buffer
      nextIndex = (i + 1) % AM335X_ETH_TX_BUFFER_COUNT;
      //Index of the previous buffer
      prevIndex = (i + AM335X_ETH_TX_BUFFER_COUNT - 1) % AM335X_ETH_TX_BUFFER_COUNT;

      //Next descriptor pointer
      txBufferDesc1[i].word0 = (uint32_t) NULL;
      //Buffer pointer
      txBufferDesc1[i].word1 = (uint32_t) txBuffer1[i];
      //Buffer offset and buffer length
      txBufferDesc1[i].word2 = 0;
      //Status flags and packet length
      txBufferDesc1[i].word3 = 0;

      //Form a doubly linked list
      txBufferDesc1[i].next = &txBufferDesc1[nextIndex];
      txBufferDesc1[i].prev = &txBufferDesc1[prevIndex];
   }

   //Point to the very first descriptor
   txCurBufferDesc1 = &txBufferDesc1[0];

   //Mark the end of the queue
   txCurBufferDesc1->prev->word3 = CPSW_TX_WORD3_SOP |
      CPSW_TX_WORD3_EOP | CPSW_TX_WORD3_EOQ;

   //Initialize TX buffer descriptor list (port 2)
   for(i = 0; i < AM335X_ETH_TX_BUFFER_COUNT; i++)
   {
      //Index of the next buffer
      nextIndex = (i + 1) % AM335X_ETH_TX_BUFFER_COUNT;
      //Index of the previous buffer
      prevIndex = (i + AM335X_ETH_TX_BUFFER_COUNT - 1) % AM335X_ETH_TX_BUFFER_COUNT;

      //Next descriptor pointer
      txBufferDesc2[i].word0 = (uint32_t) NULL;
      //Buffer pointer
      txBufferDesc2[i].word1 = (uint32_t) txBuffer2[i];
      //Buffer offset and buffer length
      txBufferDesc2[i].word2 = 0;
      //Status flags and packet length
      txBufferDesc2[i].word3 = 0;

      //Form a doubly linked list
      txBufferDesc2[i].next = &txBufferDesc2[nextIndex];
      txBufferDesc2[i].prev = &txBufferDesc2[prevIndex];
   }

   //Point to the very first descriptor
   txCurBufferDesc2 = &txBufferDesc2[0];

   //Mark the end of the queue
   txCurBufferDesc2->prev->word3 = CPSW_TX_WORD3_SOP |
      CPSW_TX_WORD3_EOP | CPSW_TX_WORD3_EOQ;

   //Initialize RX buffer descriptor list
   for(i = 0; i < AM335X_ETH_RX_BUFFER_COUNT; i++)
   {
      //Index of the next buffer
      nextIndex = (i + 1) % AM335X_ETH_RX_BUFFER_COUNT;
      //Index of the previous buffer
      prevIndex = (i + AM335X_ETH_RX_BUFFER_COUNT - 1) % AM335X_ETH_RX_BUFFER_COUNT;

      //Next descriptor pointer
      rxBufferDesc[i].word0 = (uint32_t) &rxBufferDesc[nextIndex];
      //Buffer pointer
      rxBufferDesc[i].word1 = (uint32_t) rxBuffer[i];
      //Buffer offset and buffer length
      rxBufferDesc[i].word2 = AM335X_ETH_RX_BUFFER_SIZE;
      //Status flags and packet length
      rxBufferDesc[i].word3 = CPSW_RX_WORD3_OWNER;

      //Form a doubly linked list
      rxBufferDesc[i].next = &rxBufferDesc[nextIndex];
      rxBufferDesc[i].prev = &rxBufferDesc[prevIndex];
   }

   //Point to the very first descriptor
   rxCurBufferDesc = &rxBufferDesc[0];

   //Mark the end of the queue
   rxCurBufferDesc->prev->word0 = (uint32_t) NULL;

   //Write the RX DMA head descriptor pointer
   CPSW_CPDMA_STATERAM_RX_HDP_R(CPSW_CH0) = (uint32_t) rxCurBufferDesc;
}


/**
 * @brief AM335x Ethernet MAC timer handler
 *
 * This routine is periodically called by the TCP/IP stack to handle periodic
 * operations such as polling the link state
 *
 * @param[in] interface Underlying network interface
 **/

void am335xEthTick(NetInterface *interface)
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

   //Misqueued buffer condition?
   if((rxCurBufferDesc->word3 & CPSW_RX_WORD3_OWNER) != 0)
   {
      if(CPSW_CPDMA_STATERAM_RX_HDP_R(CPSW_CH0) == 0)
      {
         //The host acts on the misqueued buffer condition by writing the added
         //buffer descriptor address to the appropriate RX DMA head descriptor
         //pointer
         CPSW_CPDMA_STATERAM_RX_HDP_R(CPSW_CH0) = (uint32_t) rxCurBufferDesc;
      }
   }
}


/**
 * @brief Enable interrupts
 * @param[in] interface Underlying network interface
 **/

void am335xEthEnableIrq(NetInterface *interface)
{
#ifdef ti_sysbios_BIOS___VERS
   //Enable Ethernet MAC interrupts
   Hwi_enableInterrupt(SYS_INT_3PGSWTXINT0);
   Hwi_enableInterrupt(SYS_INT_3PGSWRXINT0);
#else
   //Enable Ethernet MAC interrupts
   IntSystemEnable(SYS_INT_3PGSWTXINT0);
   IntSystemEnable(SYS_INT_3PGSWRXINT0);
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

void am335xEthDisableIrq(NetInterface *interface)
{
#ifdef ti_sysbios_BIOS___VERS
   //Disable Ethernet MAC interrupts
   Hwi_disableInterrupt(SYS_INT_3PGSWTXINT0);
   Hwi_disableInterrupt(SYS_INT_3PGSWRXINT0);
#else
   //Disable Ethernet MAC interrupts
   IntSystemDisable(SYS_INT_3PGSWTXINT0);
   IntSystemDisable(SYS_INT_3PGSWRXINT0);
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
 * @brief Ethernet MAC transmit interrupt
 **/

void am335xEthTxIrqHandler(void)
{
   bool_t flag;
   uint32_t status;
   uint32_t temp;
   Am335xTxBufferDesc *p;

   //Interrupt service routine prologue
   osEnterIsr();

   //This flag will be set if a higher priority task must be woken
   flag = FALSE;

   //Read the TX_STAT register to determine which channels caused the interrupt
   status = CPSW_WR_C_TX_STAT_R(CPSW_CORE0);

   //Packet transmitted on channel 1?
   if(status & (1 << CPSW_CH1))
   {
      //Point to the buffer descriptor
      p = (Am335xTxBufferDesc *) CPSW_CPDMA_STATERAM_TX_CP_R(CPSW_CH1);

      //Read the status flags
      temp = p->word3 & (CPSW_TX_WORD3_SOP | CPSW_TX_WORD3_EOP |
         CPSW_TX_WORD3_OWNER | CPSW_TX_WORD3_EOQ);

      //Misqueued buffer condition?
      if(temp == (CPSW_TX_WORD3_SOP | CPSW_TX_WORD3_EOP | CPSW_TX_WORD3_EOQ))
      {
         //Check whether the next descriptor pointer is non-zero
         if(p->word0 != 0)
         {
            //The host corrects the misqueued buffer condition by writing the
            //misqueued packet’s buffer descriptor address to the appropriate
            //TX DMA head descriptor pointer
            CPSW_CPDMA_STATERAM_TX_HDP_R(CPSW_CH1) = (uint32_t) p->word0;
         }
      }

      //Write the TX completion pointer
      CPSW_CPDMA_STATERAM_TX_CP_R(CPSW_CH1) = (uint32_t) p;

      //Check whether the TX buffer is available for writing
      if((txCurBufferDesc1->word3 & CPSW_TX_WORD3_OWNER) == 0)
      {
         //Notify the TCP/IP stack that the transmitter is ready to send
         flag |= osSetEventFromIsr(&nicDriverInterface1->nicTxEvent);
      }
   }

   //Packet transmitted on channel 2?
   if(status & (1 << CPSW_CH2))
   {
      //Point to the buffer descriptor
      p = (Am335xTxBufferDesc *) CPSW_CPDMA_STATERAM_TX_CP_R(CPSW_CH2);

      //Read the status flags
      temp = p->word3 & (CPSW_TX_WORD3_SOP | CPSW_TX_WORD3_EOP |
         CPSW_TX_WORD3_OWNER | CPSW_TX_WORD3_EOQ);

      //Misqueued buffer condition?
      if(temp == (CPSW_TX_WORD3_SOP | CPSW_TX_WORD3_EOP | CPSW_TX_WORD3_EOQ))
      {
         //Check whether the next descriptor pointer is non-zero
         if(p->word0 != 0)
         {
            //The host corrects the misqueued buffer condition by writing the
            //misqueued packet’s buffer descriptor address to the appropriate
            //TX DMA head descriptor pointer
            CPSW_CPDMA_STATERAM_TX_HDP_R(CPSW_CH2) = (uint32_t) p->word0;
         }
      }

      //Write the TX completion pointer
      CPSW_CPDMA_STATERAM_TX_CP_R(CPSW_CH2) = (uint32_t) p;

      //Check whether the TX buffer is available for writing
      if((txCurBufferDesc2->word3 & CPSW_TX_WORD3_OWNER) == 0)
      {
         //Notify the TCP/IP stack that the transmitter is ready to send
         flag |= osSetEventFromIsr(&nicDriverInterface2->nicTxEvent);
      }
   }

   //Write the DMA end of interrupt vector
   CPSW_CPDMA_EOI_VECTOR_R = CPSW_CPDMA_EOI_VECTOR_TX_PULSE;

   //Interrupt service routine epilogue
   osExitIsr(flag);
}


/**
 * @brief Ethernet MAC receive interrupt
 **/

void am335xEthRxIrqHandler(void)
{
   bool_t flag;
   uint32_t status;

   //Interrupt service routine prologue
   osEnterIsr();

   //This flag will be set if a higher priority task must be woken
   flag = FALSE;

   //Read the RX_STAT register to determine which channels caused the interrupt
   status = CPSW_WR_C_RX_STAT_R(CPSW_CORE0);

   //Packet received on channel 0?
   if(status & (1 << CPSW_CH0))
   {
      //Disable RX interrupts
      CPSW_WR_C_RX_EN_R(CPSW_CORE0) &= ~(1 << CPSW_CH0);

      //Set event flag
      if(nicDriverInterface1 != NULL)
      {
         nicDriverInterface1->nicEvent = TRUE;
      }
      else if(nicDriverInterface2 != NULL)
      {
         nicDriverInterface2->nicEvent = TRUE;
      }

      //Notify the TCP/IP stack of the event
      flag |= osSetEventFromIsr(&netEvent);
   }

   //Write the DMA end of interrupt vector
   CPSW_CPDMA_EOI_VECTOR_R = CPSW_CPDMA_EOI_VECTOR_RX_PULSE;

   //Interrupt service routine epilogue
   osExitIsr(flag);
}


/**
 * @brief AM335x Ethernet MAC event handler
 * @param[in] interface Underlying network interface
 **/

void am335xEthEventHandler(NetInterface *interface)
{
   static uint8_t buffer[AM335X_ETH_RX_BUFFER_SIZE];
   error_t error;
   size_t n;
   uint32_t temp;

   //Process all pending packets
   do
   {
      //Current buffer available for reading?
      if((rxCurBufferDesc->word3 & CPSW_RX_WORD3_OWNER) == 0)
      {
         //SOP and EOP flags should be set
         if((rxCurBufferDesc->word3 & CPSW_RX_WORD3_SOP) != 0 &&
            (rxCurBufferDesc->word3 & CPSW_RX_WORD3_EOP) != 0)
         {
            //Make sure no error occurred
            if((rxCurBufferDesc->word3 & CPSW_RX_WORD3_PKT_ERROR) == 0)
            {
               //Check the port on which the packet was received
               switch(rxCurBufferDesc->word3 & CPSW_RX_WORD3_FROM_PORT)
               {
               //Port 1?
               case CPSW_RX_WORD3_FROM_PORT_1:
                  interface = nicDriverInterface1;
                  break;
               //Port 1?
               case CPSW_RX_WORD3_FROM_PORT_2:
                  interface = nicDriverInterface2;
                  break;
               //Invalid port number?
               default:
                  interface = NULL;
                  break;
               }

               //Retrieve the length of the frame
               n = rxCurBufferDesc->word3 & CPSW_RX_WORD3_PACKET_LENGTH;
               //Limit the number of data to read
               n = MIN(n, AM335X_ETH_RX_BUFFER_SIZE);

               //Sanity check
               if(interface != NULL)
               {
                  //Copy data from the receive buffer
                  osMemcpy(buffer, (uint8_t *) rxCurBufferDesc->word1, (n + 3) & ~3UL);

                  //Packet successfully received
                  error = NO_ERROR;
               }
               else
               {
                  //The port number is invalid
                  error = ERROR_INVALID_PACKET;
               }
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

         //Mark the end of the queue with a NULL pointer
         rxCurBufferDesc->word0 = (uint32_t) NULL;
         //Restore the length of the buffer
         rxCurBufferDesc->word2 = AM335X_ETH_RX_BUFFER_SIZE;
         //Give the ownership of the descriptor back to the DMA
         rxCurBufferDesc->word3 = CPSW_RX_WORD3_OWNER;

         //Link the current descriptor to the previous descriptor
         rxCurBufferDesc->prev->word0 = (uint32_t) rxCurBufferDesc;

         //Read the status flags of the previous descriptor
         temp = rxCurBufferDesc->prev->word3 & (CPSW_RX_WORD3_SOP |
            CPSW_RX_WORD3_EOP | CPSW_RX_WORD3_OWNER | CPSW_RX_WORD3_EOQ);

         //Misqueued buffer condition?
         if(temp == (CPSW_RX_WORD3_SOP | CPSW_RX_WORD3_EOP | CPSW_RX_WORD3_EOQ))
         {
            //The host acts on the misqueued buffer condition by writing the added
            //buffer descriptor address to the appropriate RX DMA head descriptor
            //pointer
            CPSW_CPDMA_STATERAM_RX_HDP_R(CPSW_CH0) = (uint32_t) rxCurBufferDesc;
         }

         //Write the RX completion pointer
         CPSW_CPDMA_STATERAM_RX_CP_R(CPSW_CH0) = (uint32_t) rxCurBufferDesc;

         //Point to the next descriptor in the list
         rxCurBufferDesc = rxCurBufferDesc->next;
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
         nicProcessPacket(interface, buffer, n, &ancillary);
      }

      //No more data in the receive buffer?
   } while(error != ERROR_BUFFER_EMPTY);

   //Re-enable RX interrupts
   CPSW_WR_C_RX_EN_R(CPSW_CORE0) |= (1 << CPSW_CH0);
}


/**
 * @brief Send a packet (port 1)
 * @param[in] interface Underlying network interface
 * @param[in] buffer Multi-part buffer containing the data to send
 * @param[in] offset Offset to the first data byte
 * @param[in] ancillary Additional options passed to the stack along with
 *   the packet
 * @return Error code
 **/

error_t am335xEthSendPacketPort1(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary)
{
   static uint8_t temp[AM335X_ETH_TX_BUFFER_SIZE];
   size_t length;
   uint32_t value;

   //Retrieve the length of the packet
   length = netBufferGetLength(buffer) - offset;

   //Check the frame length
   if(length > AM335X_ETH_TX_BUFFER_SIZE)
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
      //Report an error
      return ERROR_INVALID_LENGTH;
   }

   //Make sure the current buffer is available for writing
   if((txCurBufferDesc1->word3 & CPSW_TX_WORD3_OWNER) != 0)
   {
      return ERROR_FAILURE;
   }

   //Mark the end of the queue with a NULL pointer
   txCurBufferDesc1->word0 = (uint32_t) NULL;

   //Copy user data to the transmit buffer
   netBufferRead(temp, buffer, offset, length);
   osMemcpy((uint8_t *) txCurBufferDesc1->word1, temp, (length + 3) & ~3UL);

   //Set the length of the buffer
   txCurBufferDesc1->word2 = length & CPSW_TX_WORD2_BUFFER_LENGTH;

   //Set the length of the packet
   value = length & CPSW_TX_WORD3_PACKET_LENGTH;
   //Set SOP and EOP flags as the data fits in a single buffer
   value |= CPSW_TX_WORD3_SOP | CPSW_TX_WORD3_EOP;
   //Redirect the packet to the relevant port number
   value |= CPSW_TX_WORD3_TO_PORT_EN | CPSW_TX_WORD3_TO_PORT_1;

   //Give the ownership of the descriptor to the DMA
   txCurBufferDesc1->word3 = CPSW_TX_WORD3_OWNER | value;

   //Link the current descriptor to the previous descriptor
   txCurBufferDesc1->prev->word0 = (uint32_t) txCurBufferDesc1;

   //Read the status flags of the previous descriptor
   value = txCurBufferDesc1->prev->word3 & (CPSW_TX_WORD3_SOP |
      CPSW_TX_WORD3_EOP | CPSW_TX_WORD3_OWNER | CPSW_TX_WORD3_EOQ);

   //Misqueued buffer condition?
   if(value == (CPSW_TX_WORD3_SOP | CPSW_TX_WORD3_EOP | CPSW_TX_WORD3_EOQ))
   {
      //Clear the misqueued buffer condition
      txCurBufferDesc1->prev->word3 = 0;

      //The host corrects the misqueued buffer condition by writing the
      //misqueued packet’s buffer descriptor address to the appropriate
      //TX DMA head descriptor pointer
      CPSW_CPDMA_STATERAM_TX_HDP_R(CPSW_CH1) = (uint32_t) txCurBufferDesc1;
   }

   //Point to the next descriptor in the list
   txCurBufferDesc1 = txCurBufferDesc1->next;

   //Check whether the next buffer is available for writing
   if((txCurBufferDesc1->word3 & CPSW_TX_WORD3_OWNER) == 0)
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
   }

   //Data successfully written
   return NO_ERROR;
}


/**
 * @brief Send a packet (port 2)
 * @param[in] interface Underlying network interface
 * @param[in] buffer Multi-part buffer containing the data to send
 * @param[in] offset Offset to the first data byte
 * @param[in] ancillary Additional options passed to the stack along with
 *   the packet
 * @return Error code
 **/

error_t am335xEthSendPacketPort2(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary)
{
   static uint8_t temp[AM335X_ETH_TX_BUFFER_SIZE];
   size_t length;
   uint32_t value;

   //Retrieve the length of the packet
   length = netBufferGetLength(buffer) - offset;

   //Check the frame length
   if(length > AM335X_ETH_TX_BUFFER_SIZE)
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
      //Report an error
      return ERROR_INVALID_LENGTH;
   }

   //Make sure the current buffer is available for writing
   if((txCurBufferDesc2->word3 & CPSW_TX_WORD3_OWNER) != 0)
   {
      return ERROR_FAILURE;
   }

   //Mark the end of the queue with a NULL pointer
   txCurBufferDesc2->word0 = (uint32_t) NULL;

   //Copy user data to the transmit buffer
   netBufferRead(temp, buffer, offset, length);
   osMemcpy((uint8_t *) txCurBufferDesc2->word1, temp, (length + 3) & ~3UL);

   //Set the length of the buffer
   txCurBufferDesc2->word2 = length & CPSW_TX_WORD2_BUFFER_LENGTH;

   //Set the length of the packet
   value = length & CPSW_TX_WORD3_PACKET_LENGTH;
   //Set SOP and EOP flags as the data fits in a single buffer
   value |= CPSW_TX_WORD3_SOP | CPSW_TX_WORD3_EOP;
   //Redirect the packet to the relevant port number
   value |= CPSW_TX_WORD3_TO_PORT_EN | CPSW_TX_WORD3_TO_PORT_2;

   //Give the ownership of the descriptor to the DMA
   txCurBufferDesc2->word3 = CPSW_TX_WORD3_OWNER | value;

   //Link the current descriptor to the previous descriptor
   txCurBufferDesc2->prev->word0 = (uint32_t) txCurBufferDesc2;

   //Read the status flags of the previous descriptor
   value = txCurBufferDesc2->prev->word3 & (CPSW_TX_WORD3_SOP |
      CPSW_TX_WORD3_EOP | CPSW_TX_WORD3_OWNER | CPSW_TX_WORD3_EOQ);

   //Misqueued buffer condition?
   if(value == (CPSW_TX_WORD3_SOP | CPSW_TX_WORD3_EOP | CPSW_TX_WORD3_EOQ))
   {
      //Clear the misqueued buffer condition
      txCurBufferDesc2->prev->word3 = 0;

      //The host corrects the misqueued buffer condition by writing the
      //misqueued packet’s buffer descriptor address to the appropriate
      //TX DMA head descriptor pointer
      CPSW_CPDMA_STATERAM_TX_HDP_R(CPSW_CH2) = (uint32_t) txCurBufferDesc2;
   }

   //Point to the next descriptor in the list
   txCurBufferDesc2 = txCurBufferDesc2->next;

   //Check whether the next buffer is available for writing
   if((txCurBufferDesc2->word3 & CPSW_TX_WORD3_OWNER) == 0)
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
   }

   //Data successfully written
   return NO_ERROR;
}


/**
 * @brief Configure MAC address filtering
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t am335xEthUpdateMacAddrFilter(NetInterface *interface)
{
   uint_t i;
   uint_t port;
   MacFilterEntry *entry;

   //Debug message
   TRACE_DEBUG("Updating AM335x ALE table...\r\n");

   //Select the relevant port number
   if(interface == nicDriverInterface1)
   {
      port = CPSW_PORT1;
   }
   else if(interface == nicDriverInterface2)
   {
      port = CPSW_PORT2;
   }
   else
   {
      port = CPSW_PORT0;
   }

   //The MAC address filter contains the list of MAC addresses to accept
   //when receiving an Ethernet frame
   for(i = 0; i < MAC_ADDR_FILTER_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->macAddrFilter[i];

      //Check whether the ALE table should be updated for the
      //current multicast address
      if(!macCompAddr(&entry->addr, &MAC_UNSPECIFIED_ADDR))
      {
         if(entry->addFlag)
         {
            //Add VLAN/multicast address entry to the ALE table
            am335xEthAddVlanAddrEntry(port, port, &entry->addr);
         }
         else if(entry->deleteFlag)
         {
            //Remove VLAN/multicast address entry from the ALE table
            am335xEthDeleteVlanAddrEntry(port, port, &entry->addr);
         }
      }
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Adjust MAC configuration parameters for proper operation
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t am335xEthUpdateMacConfig(NetInterface *interface)
{
   uint32_t config = 0;

   //Read MAC control register
   if(interface == nicDriverInterface1)
   {
      config = CPSW_SL1_MACCTRL_R;
   }
   else if(interface == nicDriverInterface2)
   {
      config = CPSW_SL2_MACCTRL_R;
   }

   //1000BASE-T operation mode?
   if(interface->linkSpeed == NIC_LINK_SPEED_1GBPS)
   {
      config |= CPSW_SL_MACCTRL_GIG_MASK;
      config &= ~(CPSW_SL_MACCTRL_IFCTL_A_MASK | CPSW_SL_MACCTRL_IFCTL_B_MASK);
   }
   //100BASE-TX operation mode?
   else if(interface->linkSpeed == NIC_LINK_SPEED_100MBPS)
   {
      config &= ~CPSW_SL_MACCTRL_GIG_MASK;
      config |= CPSW_SL_MACCTRL_IFCTL_A_MASK | CPSW_SL_MACCTRL_IFCTL_B_MASK;
   }
   //10BASE-T operation mode?
   else
   {
      config &= ~CPSW_SL_MACCTRL_GIG_MASK;
      config &= ~(CPSW_SL_MACCTRL_IFCTL_A_MASK | CPSW_SL_MACCTRL_IFCTL_B_MASK);
   }

   //Half-duplex or full-duplex mode?
   if(interface->duplexMode == NIC_FULL_DUPLEX_MODE)
   {
      config |= CPSW_SL_MACCTRL_FULLDUPLEX_MASK;
   }
   else
   {
      config &= ~CPSW_SL_MACCTRL_FULLDUPLEX_MASK;
   }

   //Update MAC control register
   if(interface == nicDriverInterface1)
   {
      CPSW_SL1_MACCTRL_R = config;
   }
   else if(interface == nicDriverInterface2)
   {
      CPSW_SL2_MACCTRL_R = config;
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

void am335xEthWritePhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr, uint16_t data)
{
   uint32_t temp;

   //Valid opcode?
   if(opcode == SMI_OPCODE_WRITE)
   {
      //Set up a write operation
      temp = MDIO_USERACCESS_GO_MASK | MDIO_USERACCESS_WRITE;
      //PHY address
      temp |= (phyAddr << MDIO_USERACCESS_PHYADR_SHIFT) & MDIO_USERACCESS_PHYADR_MASK;
      //Register address
      temp |= (regAddr << MDIO_USERACCESS_REGADR_SHIFT) & MDIO_USERACCESS_REGADR_MASK;
      //Register value
      temp |= data & MDIO_USERACCESS_DATA_MASK;

      //Start a write operation
      MDIO_USERACCESS_R(0) = temp;
      //Wait for the write to complete
      while((MDIO_USERACCESS_R(0) & MDIO_USERACCESS_GO_MASK) != 0)
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

uint16_t am335xEthReadPhyReg(uint8_t opcode, uint8_t phyAddr,
   uint8_t regAddr)
{
   uint16_t data;
   uint32_t temp;

   //Valid opcode?
   if(opcode == SMI_OPCODE_READ)
   {
      //Set up a read operation
      temp = MDIO_USERACCESS_GO_MASK | MDIO_USERACCESS_READ;
      //PHY address
      temp |= (phyAddr << MDIO_USERACCESS_PHYADR_SHIFT) & MDIO_USERACCESS_PHYADR_MASK;
      //Register address
      temp |= (regAddr << MDIO_USERACCESS_REGADR_SHIFT) & MDIO_USERACCESS_REGADR_MASK;

      //Start a read operation
      MDIO_USERACCESS_R(0) = temp;
      //Wait for the read to complete
      while((MDIO_USERACCESS_R(0) & MDIO_USERACCESS_GO_MASK) != 0)
      {
      }

      //Get register value
      data = MDIO_USERACCESS_R(0) & MDIO_USERACCESS_DATA_MASK;
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
 * @brief Write an ALE table entry
 * @param[in] index Entry index
 * @param[in] entry Pointer to the ALE table entry
 **/

void am335xEthWriteEntry(uint_t index, const Am335xAleEntry *entry)
{
   //Copy the content of the entry to be written
   CPSW_ALE_TBLW_R(2) = entry->word2;
   CPSW_ALE_TBLW_R(1) = entry->word1;
   CPSW_ALE_TBLW_R(0) = entry->word0;

   //Write the ALE entry at the specified index
   CPSW_ALE_TBLCTL_R = CPSW_ALE_TBLCTL_WRITE_RDZ_MASK | index;
}


/**
 * @brief Read an ALE table entry
 * @param[in] index Entry index
 * @param[out] entry Pointer to the ALE table entry
 **/

void am335xEthReadEntry(uint_t index, Am335xAleEntry *entry)
{
   //Read the ALE entry at the specified index
   CPSW_ALE_TBLCTL_R = index;

   //Copy the content of the entry
   entry->word2 = CPSW_ALE_TBLW_R(2);
   entry->word1 = CPSW_ALE_TBLW_R(1);
   entry->word0 = CPSW_ALE_TBLW_R(0);
}


/**
 * @brief Find a free entry in the ALE table
 * @return Index of the first free entry
 **/

uint_t am335xEthFindFreeEntry(void)
{
   uint_t index;
   uint32_t type;
   Am335xAleEntry entry;

   //Loop through the ALE table entries
   for(index = 0; index < CPSW_ALE_MAX_ENTRIES; index++)
   {
      //Read the current entry
      am335xEthReadEntry(index, &entry);

      //Retrieve the type of the ALE entry
      type = entry.word1 & CPSW_ALE_WORD1_ENTRY_TYPE_MASK;

      //Free entry?
      if(type == CPSW_ALE_WORD1_ENTRY_TYPE_FREE)
      {
         //Exit immediately
         break;
      }
   }

   //Return the index of the entry
   return index;
}


/**
 * @brief Search the ALE table for the specified VLAN entry
 * @param[in] vlanId VLAN identifier
 * @return Index of the matching entry
 **/

uint_t am335xEthFindVlanEntry(uint_t vlanId)
{
   uint_t index;
   uint32_t value;
   Am335xAleEntry entry;

   //Loop through the ALE table entries
   for(index = 0; index < CPSW_ALE_MAX_ENTRIES; index++)
   {
      //Read the current entry
      am335xEthReadEntry(index, &entry);

      //Retrieve the type of the ALE entry
      value = entry.word1 & CPSW_ALE_WORD1_ENTRY_TYPE_MASK;

      //Check the type of the ALE entry
      if(value == CPSW_ALE_WORD1_ENTRY_TYPE_VLAN_ADDR)
      {
         //Get the VLAN identifier
         value = entry.word1 & CPSW_ALE_WORD1_VLAN_ID_MASK;

         //Compare the VLAN identifier
         if(value == CPSW_ALE_WORD1_VLAN_ID(vlanId))
         {
            //Matching ALE entry found
            break;
         }
      }
   }

   //Return the index of the entry
   return index;
}


/**
 * @brief Search the ALE table for the specified VLAN/address entry
 * @param[in] vlanId VLAN identifier
 * @param[in] macAddr MAC address
 * @return Index of the matching entry
 **/

uint_t am335xEthFindVlanAddrEntry(uint_t vlanId, MacAddr *macAddr)
{
   uint_t index;
   uint32_t value;
   Am335xAleEntry entry;

   //Loop through the ALE table entries
   for(index = 0; index < CPSW_ALE_MAX_ENTRIES; index++)
   {
      //Read the current entry
      am335xEthReadEntry(index, &entry);

      //Retrieve the type of the ALE entry
      value = entry.word1 & CPSW_ALE_WORD1_ENTRY_TYPE_MASK;

      //Check the type of the ALE entry
      if(value == CPSW_ALE_WORD1_ENTRY_TYPE_VLAN_ADDR)
      {
         //Get the VLAN identifier
         value = entry.word1 & CPSW_ALE_WORD1_VLAN_ID_MASK;

         //Compare the VLAN identifier
         if(value == CPSW_ALE_WORD1_VLAN_ID(vlanId))
         {
            //Compare the MAC address
            if(macAddr->b[0] == (uint8_t) (entry.word1 >> 8) &&
               macAddr->b[1] == (uint8_t) (entry.word1 >> 0) &&
               macAddr->b[2] == (uint8_t) (entry.word0 >> 24) &&
               macAddr->b[3] == (uint8_t) (entry.word0 >> 16) &&
               macAddr->b[4] == (uint8_t) (entry.word0 >> 8) &&
               macAddr->b[5] == (uint8_t) (entry.word0 >> 0))
            {
               //Matching ALE entry found
               break;
            }
         }
      }
   }

   //Return the index of the entry
   return index;
}


/**
 * @brief Add a VLAN entry in the ALE table
 * @param[in] port Port number
 * @param[in] vlanId VLAN identifier
 * @return Error code
 **/

error_t am335xEthAddVlanEntry(uint_t port, uint_t vlanId)
{
   error_t error;
   uint_t index;
   Am335xAleEntry entry;

   //Ensure that there are no duplicate address entries in the ALE table
   index = am335xEthFindVlanEntry(vlanId);

   //No matching entry found?
   if(index >= CPSW_ALE_MAX_ENTRIES)
   {
      //Find a free entry in the ALE table
      index = am335xEthFindFreeEntry();
   }

   //Sanity check
   if(index < CPSW_ALE_MAX_ENTRIES)
   {
      //Set up a VLAN table entry
      entry.word2 = 0;
      entry.word1 = CPSW_ALE_WORD1_ENTRY_TYPE_VLAN;
      entry.word0 = 0;

      //Set VLAN identifier
      entry.word1 |= CPSW_ALE_WORD1_VLAN_ID(vlanId);

      //Force the packet VLAN tag to be removed on egress
      entry.word0 |= CPSW_ALE_WORD0_FORCE_UNTAG_EGRESS(1 << port) |
         CPSW_ALE_WORD0_FORCE_UNTAG_EGRESS(1 << CPSW_PORT0);

      //Set VLAN member list
      entry.word0 |= CPSW_ALE_WORD0_VLAN_MEMBER_LIST(1 << port) |
         CPSW_ALE_WORD0_VLAN_MEMBER_LIST(1 << CPSW_PORT0);

      //Add a new entry to the ALE table
      am335xEthWriteEntry(index, &entry);

      //Successful processing
      error = NO_ERROR;
   }
   else
   {
      //The ALE table is full
      error = ERROR_FAILURE;
   }

   //Return status code
   return error;
}


/**
 * @brief Add a VLAN/address entry in the ALE table
 * @param[in] port Port number
 * @param[in] vlanId VLAN identifier
 * @param[in] macAddr MAC address
 * @return Error code
 **/

error_t am335xEthAddVlanAddrEntry(uint_t port, uint_t vlanId, MacAddr *macAddr)
{
   error_t error;
   uint_t index;
   Am335xAleEntry entry;

   //Ensure that there are no duplicate address entries in the ALE table
   index = am335xEthFindVlanAddrEntry(vlanId, macAddr);

   //No matching entry found?
   if(index >= CPSW_ALE_MAX_ENTRIES)
   {
      //Find a free entry in the ALE table
      index = am335xEthFindFreeEntry();
   }

   //Sanity check
   if(index < CPSW_ALE_MAX_ENTRIES)
   {
      //Set up a VLAN/address table entry
      entry.word2 = 0;
      entry.word1 = CPSW_ALE_WORD1_ENTRY_TYPE_VLAN_ADDR;
      entry.word0 = 0;

      //Multicast address?
      if(macIsMulticastAddr(macAddr))
      {
         //Set port mask
         entry.word2 |= CPSW_ALE_WORD2_SUPER |
            CPSW_ALE_WORD2_PORT_LIST(1 << port) |
            CPSW_ALE_WORD2_PORT_LIST(1 << CPSW_CH0);

         //Set multicast forward state
         entry.word1 |= CPSW_ALE_WORD1_MCAST_FWD_STATE(0);
      }

      //Set VLAN identifier
      entry.word1 |= CPSW_ALE_WORD1_VLAN_ID(vlanId);

      //Copy the upper 16 bits of the unicast address
      entry.word1 |= (macAddr->b[0] << 8) | macAddr->b[1];

      //Copy the lower 32 bits of the unicast address
      entry.word0 |= (macAddr->b[2] << 24) | (macAddr->b[3] << 16) |
         (macAddr->b[4] << 8) | macAddr->b[5];

      //Add a new entry to the ALE table
      am335xEthWriteEntry(index, &entry);

      //Successful processing
      error = NO_ERROR;
   }
   else
   {
      //The ALE table is full
      error = ERROR_FAILURE;
   }

   //Return status code
   return error;
}


/**
 * @brief Remove a VLAN/address entry from the ALE table
 * @param[in] port Port number
 * @param[in] vlanId VLAN identifier
 * @param[in] macAddr MAC address
 * @return Error code
 **/

error_t am335xEthDeleteVlanAddrEntry(uint_t port, uint_t vlanId, MacAddr *macAddr)
{
   error_t error;
   uint_t index;
   Am335xAleEntry entry;

   //Search the ALE table for the specified VLAN/address entry
   index = am335xEthFindVlanAddrEntry(vlanId, macAddr);

   //Matching ALE entry found?
   if(index < CPSW_ALE_MAX_ENTRIES)
   {
      //Clear the contents of the entry
      entry.word2 = 0;
      entry.word1 = 0;
      entry.word0 = 0;

      //Update the ALE table
      am335xEthWriteEntry(index, &entry);

      //Successful processing
      error = NO_ERROR;
   }
   else
   {
      //Entry not found
      error = ERROR_NOT_FOUND;
   }

   //Return status code
   return error;
}
