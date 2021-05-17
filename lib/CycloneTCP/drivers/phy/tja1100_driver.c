/**
 * @file tja1100_driver.c
 * @brief TJA1100 100Base-T1 Ethernet PHY driver
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
#include "drivers/phy/tja1100_driver.h"
#include "debug.h"


/**
 * @brief TJA1100 Ethernet PHY driver
 **/

const PhyDriver tja1100PhyDriver =
{
   tja1100Init,
   tja1100Tick,
   tja1100EnableIrq,
   tja1100DisableIrq,
   tja1100EventHandler
};


/**
 * @brief TJA1100 PHY transceiver initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t tja1100Init(NetInterface *interface)
{
   uint16_t value;

   //Debug message
   TRACE_INFO("Initializing TJA1100...\r\n");

   //Undefined PHY address?
   if(interface->phyAddr >= 32)
   {
      //Use the default address
      interface->phyAddr = TJA1100_PHY_ADDR;
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

   //Reset PHY transceiver
   tja1100WritePhyReg(interface, TJA1100_BASIC_CTRL,
      TJA1100_BASIC_CTRL_RESET);

   //Wait for the reset to complete
   while(tja1100ReadPhyReg(interface, TJA1100_BASIC_CTRL) &
      TJA1100_BASIC_CTRL_RESET)
   {
   }

   //Dump PHY registers for debugging purpose
   tja1100DumpPhyReg(interface);

   //Enable configuration register access
   value = tja1100ReadPhyReg(interface, TJA1100_EXTENDED_CTRL);
   value |= TJA1100_EXTENDED_CTRL_CONFIG_EN;
   tja1100WritePhyReg(interface, TJA1100_EXTENDED_CTRL, value);

   //Select RMII mode (25MHz XTAL)
   value = tja1100ReadPhyReg(interface, TJA1100_CONFIG1);
   value &= ~TJA1100_CONFIG1_MII_MODE;
   value |= TJA1100_CONFIG1_MII_MODE_RMII_25MHZ_XTAL;
   tja1100WritePhyReg(interface, TJA1100_CONFIG1, value);

   //The PHY is configured for autonomous operation
   value = tja1100ReadPhyReg(interface, TJA1100_CONFIG1);
   value |= TJA1100_CONFIG1_AUTO_OP;
   tja1100WritePhyReg(interface, TJA1100_CONFIG1, value);

   //Force the TCP/IP stack to poll the link state at startup
   interface->phyEvent = TRUE;
   //Notify the TCP/IP stack of the event
   osSetEvent(&netEvent);

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief TJA1100 timer handler
 * @param[in] interface Underlying network interface
 **/

void tja1100Tick(NetInterface *interface)
{
   uint16_t value;
   bool_t linkState;

   //No external interrupt line driver?
   if(interface->extIntDriver == NULL)
   {
      //Read status register
      value = tja1100ReadPhyReg(interface, TJA1100_BASIC_STAT);
      //Retrieve current link state
      linkState = (value & TJA1100_BASIC_STAT_LINK_STATUS) ? TRUE : FALSE;

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

void tja1100EnableIrq(NetInterface *interface)
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

void tja1100DisableIrq(NetInterface *interface)
{
   //Disable PHY transceiver interrupts
   if(interface->extIntDriver != NULL)
   {
      interface->extIntDriver->disableIrq();
   }
}


/**
 * @brief TJA1100 event handler
 * @param[in] interface Underlying network interface
 **/

void tja1100EventHandler(NetInterface *interface)
{
   uint16_t value;

   //Read status register
   value = tja1100ReadPhyReg(interface, TJA1100_BASIC_STAT);

   //Link is up?
   if((value & TJA1100_BASIC_STAT_LINK_STATUS) != 0)
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

void tja1100WritePhyReg(NetInterface *interface, uint8_t address,
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

uint16_t tja1100ReadPhyReg(NetInterface *interface, uint8_t address)
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

void tja1100DumpPhyReg(NetInterface *interface)
{
   uint8_t i;

   //Loop through PHY registers
   for(i = 0; i < 32; i++)
   {
      //Display current PHY register
      TRACE_DEBUG("%02" PRIu8 ": 0x%04" PRIX16 "\r\n", i,
         tja1100ReadPhyReg(interface, i));
   }

   //Terminate with a line feed
   TRACE_DEBUG("\r\n");
}
