/**
 * @file mv88e1512_driver.c
 * @brief 88E1512 Gigabit Ethernet PHY driver
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
#include "drivers/phy/mv88e1512_driver.h"
#include "debug.h"


/**
 * @brief 88E1512 Ethernet PHY driver
 **/

const PhyDriver mv88e1512PhyDriver =
{
   mv88e1512Init,
   mv88e1512Tick,
   mv88e1512EnableIrq,
   mv88e1512DisableIrq,
   mv88e1512EventHandler
};


/**
 * @brief 88E1512 PHY transceiver initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t mv88e1512Init(NetInterface *interface)
{
   //Debug message
   TRACE_INFO("Initializing 88E1512...\r\n");

   //Undefined PHY address?
   if(interface->phyAddr >= 32)
   {
      //Use the default address
      interface->phyAddr = MV88E1512_PHY_ADDR;
   }

   //Initialize serial management interface
   if(interface->smiDriver != NULL)
   {
      interface->smiDriver->init();
   }

   //Reset PHY transceiver
   mv88e1512WritePhyReg(interface, MV88E1512_COPPER_CTRL,
      MV88E1512_COPPER_CTRL_RESET);

   //Wait for the reset to complete
   while(mv88e1512ReadPhyReg(interface, MV88E1512_COPPER_CTRL) &
      MV88E1512_COPPER_CTRL_RESET)
   {
   }

   //Dump PHY registers for debugging purpose
   mv88e1512DumpPhyReg(interface);

   //Force the TCP/IP stack to poll the link state at startup
   interface->phyEvent = TRUE;
   //Notify the TCP/IP stack of the event
   osSetEvent(&netEvent);

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief 88E1512 timer handler
 * @param[in] interface Underlying network interface
 **/

void mv88e1512Tick(NetInterface *interface)
{
   uint16_t value;
   bool_t linkState;

   //Read copper status register
   value = mv88e1512ReadPhyReg(interface, MV88E1512_COPPER_STAT);
   //Retrieve current link state
   linkState = (value & MV88E1512_COPPER_STAT_LINK_STATUS) ? TRUE : FALSE;

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


/**
 * @brief Enable interrupts
 * @param[in] interface Underlying network interface
 **/

void mv88e1512EnableIrq(NetInterface *interface)
{
}


/**
 * @brief Disable interrupts
 * @param[in] interface Underlying network interface
 **/

void mv88e1512DisableIrq(NetInterface *interface)
{
}


/**
 * @brief 88E1512 event handler
 * @param[in] interface Underlying network interface
 **/

void mv88e1512EventHandler(NetInterface *interface)
{
   uint16_t status;

   //Read copper specific status 1 register
   status = mv88e1512ReadPhyReg(interface, MV88E1512_COPPER_STAT1);

   //Link is up?
   if((status & MV88E1512_COPPER_STAT1_LINK) != 0)
   {
      //Check current speed
      switch(status & MV88E1512_COPPER_STAT1_SPEED)
      {
      //10BASE-T
      case MV88E1512_COPPER_STAT1_SPEED_10MBPS:
         interface->linkSpeed = NIC_LINK_SPEED_10MBPS;
         break;
      //100BASE-TX
      case MV88E1512_COPPER_STAT1_SPEED_100MBPS:
         interface->linkSpeed = NIC_LINK_SPEED_100MBPS;
         break;
      //1000BASE-T
      case MV88E1512_COPPER_STAT1_SPEED_1000MBPS:
         interface->linkSpeed = NIC_LINK_SPEED_1GBPS;
         break;
      //Unknown speed
      default:
         //Debug message
         TRACE_WARNING("Invalid speed\r\n");
         break;
      }

      //Check current duplex mode
      if((status & MV88E1512_COPPER_STAT1_DUPLEX) != 0)
      {
         interface->duplexMode = NIC_FULL_DUPLEX_MODE;
      }
      else
      {
         interface->duplexMode = NIC_HALF_DUPLEX_MODE;
      }

      //Update link state
      interface->linkState = TRUE;

      //Adjust MAC configuration parameters for proper operation
      interface->nicDriver->updateMacConfig(interface);
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

void mv88e1512WritePhyReg(NetInterface *interface, uint8_t address,
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

uint16_t mv88e1512ReadPhyReg(NetInterface *interface, uint8_t address)
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

void mv88e1512DumpPhyReg(NetInterface *interface)
{
   uint8_t i;

   //Loop through PHY registers
   for(i = 0; i < 32; i++)
   {
      //Display current PHY register
      TRACE_DEBUG("%02" PRIu8 ": 0x%04" PRIX16 "\r\n", i,
         mv88e1512ReadPhyReg(interface, i));
   }

   //Terminate with a line feed
   TRACE_DEBUG("\r\n");
}
