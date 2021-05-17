/**
 * @file lan8770_driver.c
 * @brief LAN8770 Ethernet PHY driver
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
#include "core/net.h"
#include "drivers/phy/lan8770_driver.h"
#include "debug.h"


/**
 * @brief LAN8770 Ethernet PHY driver
 **/

const PhyDriver lan8770PhyDriver =
{
   lan8770Init,
   lan8770Tick,
   lan8770EnableIrq,
   lan8770DisableIrq,
   lan8770EventHandler
};


/**
 * @brief LAN8770 PHY transceiver initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t lan8770Init(NetInterface *interface)
{
   uint16_t temp;

   //Debug message
   TRACE_INFO("Initializing LAN8770...\r\n");

   //Undefined PHY address?
   if(interface->phyAddr >= 32)
   {
      //Use the default address
      interface->phyAddr = LAN8770_PHY_ADDR;
   }

   //Initialize serial management interface
   if(interface->smiDriver != NULL)
   {
      interface->smiDriver->init();
   }

   //Initialize external interrupt line driver
   if(interface->extIntDriver != NULL)
   {
      interface->extIntDriver->init();
   }

   //Hardware initialization sequence is suspended to enable pre-configuration
   //by software
   temp = lan8770ReadPhyReg(interface, LAN8770_POWER_DOWN_CONTROL);
   temp &= ~LAN8770_POWER_DOWN_CONTROL_HARD_INIT_SEQ_EN;
   lan8770WritePhyReg(interface, LAN8770_POWER_DOWN_CONTROL, temp);

   //Reset PHY transceiver
   lan8770WritePhyReg(interface, LAN8770_BASIC_CONTROL,
      LAN8770_BASIC_CONTROL_RESET);

   //Wait for the reset to complete
   while(lan8770ReadPhyReg(interface, LAN8770_BASIC_CONTROL) &
      LAN8770_BASIC_CONTROL_RESET)
   {
   }

   //Set TX amplitude
   temp = lan8770ReadExtReg(interface, LAN8770_AFE_PORT_CFG1);
   temp &= ~LAN8770_AFE_PORT_CFG1_TX_AMP;
   temp |= LAN8770_AFE_PORT_CFG1_TX_AMP_DEFAULT;
   lan8770WriteExtReg(interface, LAN8770_AFE_PORT_CFG1, temp);

   //Clear SMI interrupts
   temp = lan8770ReadPhyReg(interface, LAN8770_INTERRUPT_SOURCE);
   //Clear MISC interrupts
   temp = lan8770ReadExtReg(interface, LAN8770_INTERRUPT2_SOURCE);

   //Enable ring oscillator
   temp = lan8770ReadExtReg(interface, LAN8770_WKP_COM_CTL0);
   temp |= LAN8770_WKP_COM_CTL0_RING_OSC_EN;
   lan8770WriteExtReg(interface, LAN8770_WKP_COM_CTL0, temp);

   //Adjust WUR detect length and LPS detect length
   lan8770WriteExtReg(interface, LAN8770_SLEEP_WAKE_DET,
      LAN8770_SLEEP_WAKE_DET_WUR_DETECT_LEN_DEFAULT |
      LAN8770_SLEEP_WAKE_DET_LPS_DETECT_LEN_DEFAULT);

   //Configure MISC registers
   lan8770WriteExtReg(interface, LAN8770_WKP_COM_CTL1, 0x274F);
   lan8770WriteExtReg(interface, LAN8770_WKP_COM_CTL0, 0x80A7);
   lan8770WriteExtReg(interface, LAN8770_WKP_PRT_CTL, 0xF110);

   //Enable hardware initialization sequence
   temp = lan8770ReadPhyReg(interface, LAN8770_POWER_DOWN_CONTROL);
   temp |= LAN8770_POWER_DOWN_CONTROL_HARD_INIT_SEQ_EN;
   lan8770WritePhyReg(interface, LAN8770_POWER_DOWN_CONTROL, temp);

   //Dump PHY registers for debugging purpose
   lan8770DumpPhyReg(interface);

   //Force the TCP/IP stack to poll the link state at startup
   interface->phyEvent = TRUE;
   //Notify the TCP/IP stack of the event
   osSetEvent(&netEvent);

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief LAN8770 timer handler
 * @param[in] interface Underlying network interface
 **/

void lan8770Tick(NetInterface *interface)
{
   uint16_t value;
   bool_t linkState;

   //No external interrupt line driver?
   if(interface->extIntDriver == NULL)
   {
      //Read status register
      value = lan8770ReadPhyReg(interface, LAN8770_BASIC_STATUS);
      //Retrieve current link state
      linkState = (value & LAN8770_BASIC_STATUS_LINK_STATUS) ? TRUE : FALSE;

      //Link up event?
      if(linkState && !interface->linkState)
      {
         //Set event flag
         interface->phyEvent = TRUE;
         //Notify the TCP/IP stack of the event
         osSetEvent(&netEvent);
      }
      //Link down event?
      else if(!linkState && interface->linkState)
      {
         //Set event flag
         interface->phyEvent = TRUE;
         //Notify the TCP/IP stack of the event
         osSetEvent(&netEvent);
      }
   }
}


/**
 * @brief Enable interrupts
 * @param[in] interface Underlying network interface
 **/

void lan8770EnableIrq(NetInterface *interface)
{
   //Enable PHY transceiver interrupts
   if(interface->extIntDriver != NULL)
   {
      interface->extIntDriver->enableIrq();
   }
}


/**
 * @brief Disable interrupts
 * @param[in] interface Underlying network interface
 **/

void lan8770DisableIrq(NetInterface *interface)
{
   //Disable PHY transceiver interrupts
   if(interface->extIntDriver != NULL)
   {
      interface->extIntDriver->disableIrq();
   }
}


/**
 * @brief LAN8770 event handler
 * @param[in] interface Underlying network interface
 **/

void lan8770EventHandler(NetInterface *interface)
{
   uint16_t value;

   //Read status register
   value = lan8770ReadPhyReg(interface, LAN8770_BASIC_STATUS);

   //Link is up?
   if((value & LAN8770_BASIC_STATUS_LINK_STATUS) != 0)
   {
      //Adjust MAC configuration parameters for proper operation
      interface->linkSpeed = NIC_LINK_SPEED_100MBPS;
      interface->duplexMode = NIC_FULL_DUPLEX_MODE;
      interface->nicDriver->updateMacConfig(interface);

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


/**
 * @brief Write PHY register
 * @param[in] interface Underlying network interface
 * @param[in] address PHY register address
 * @param[in] data Register value
 **/

void lan8770WritePhyReg(NetInterface *interface, uint8_t address,
   uint16_t data)
{
   //Write the specified PHY register
   if(interface->smiDriver != NULL)
   {
      interface->smiDriver->writePhyReg(SMI_OPCODE_WRITE,
         interface->phyAddr, address, data);
   }
   else
   {
      interface->nicDriver->writePhyReg(SMI_OPCODE_WRITE,
         interface->phyAddr, address, data);
   }
}


/**
 * @brief Read PHY register
 * @param[in] interface Underlying network interface
 * @param[in] address PHY register address
 * @return Register value
 **/

uint16_t lan8770ReadPhyReg(NetInterface *interface, uint8_t address)
{
   uint16_t data;

   //Read the specified PHY register
   if(interface->smiDriver != NULL)
   {
      data = interface->smiDriver->readPhyReg(SMI_OPCODE_READ,
         interface->phyAddr, address);
   }
   else
   {
      data = interface->nicDriver->readPhyReg(SMI_OPCODE_READ,
         interface->phyAddr, address);
   }

   //Return the value of the PHY register
   return data;
}


/**
 * @brief Dump PHY registers for debugging purpose
 * @param[in] interface Underlying network interface
 **/

void lan8770DumpPhyReg(NetInterface *interface)
{
   uint8_t i;

   //Loop through PHY registers
   for(i = 0; i < 32; i++)
   {
      //Display current PHY register
      TRACE_DEBUG("%02" PRIu8 ": 0x%04" PRIX16 "\r\n", i,
         lan8770ReadPhyReg(interface, i));
   }

   //Terminate with a line feed
   TRACE_DEBUG("\r\n");
}


/**
 * @brief Write external register
 * @param[in] interface Underlying network interface
 * @param[in] bank Register bank
 * @param[in] addr Register address
 * @param[in] data External register value
 **/

void lan8770WriteExtReg(NetInterface *interface, uint8_t bank,
   uint8_t addr, uint16_t data)
{
   uint16_t temp;

   //Set up a write operation
   temp = LAN8770_EXT_REG_CTL_WRITE_CONTROL;
   //Select register bank
   temp |= (bank << 8) & LAN8770_EXT_REG_CTL_REGISTER_BANK;
   //Select register address
   temp |= addr & LAN8770_EXT_REG_CTL_REGISTER_ADDR;

   //Write the EXT_REG_WR_DATA register with the desired 16-bit data
   lan8770WritePhyReg(interface, LAN8770_EXT_REG_WR_DATA, data);

   //Write the EXT_REG_CTL register
   lan8770WritePhyReg(interface, LAN8770_EXT_REG_CTL, temp);
}


/**
 * @brief Read external register
 * @param[in] interface Underlying network interface
 * @param[in] bank Register bank
 * @param[in] addr Register address
 * @return External register value
 **/

uint16_t lan8770ReadExtReg(NetInterface *interface, uint8_t bank,
   uint8_t addr)
{
   uint16_t temp;

   //Set up a read operation
   temp = LAN8770_EXT_REG_CTL_READ_CONTROL;
   //Select register bank
   temp |= (bank << 8) & LAN8770_EXT_REG_CTL_REGISTER_BANK;
   //Select register address
   temp |= addr & LAN8770_EXT_REG_CTL_REGISTER_ADDR;

   //Write the EXT_REG_CTL register
   lan8770WritePhyReg(interface, LAN8770_EXT_REG_CTL, temp);

   //Read the EXT_REG_RD_DATA register
   return lan8770ReadPhyReg(interface, LAN8770_EXT_REG_RD_DATA);
}
