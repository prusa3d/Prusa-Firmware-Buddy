/**
 * @file ip101_driver.c
 * @brief IC+ IP101 Ethernet PHY driver
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
#include "drivers/phy/ip101_driver.h"
#include "debug.h"


/**
 * @brief IP101 Ethernet PHY driver
 **/

const PhyDriver ip101PhyDriver =
{
   ip101Init,
   ip101Tick,
   ip101EnableIrq,
   ip101DisableIrq,
   ip101EventHandler
};


/**
 * @brief IP101 PHY transceiver initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t ip101Init(NetInterface *interface)
{
   //Debug message
   TRACE_INFO("Initializing IP101...\r\n");

   //Undefined PHY address?
   if(interface->phyAddr >= 32)
   {
      //Use the default address
      interface->phyAddr = IP101_PHY_ADDR;
   }

   //Initialize serial management interface
   if(interface->smiDriver != NULL)
   {
      interface->smiDriver->init();
   }

   //Reset PHY transceiver
   ip101WritePhyReg(interface, IP101_BMCR, IP101_BMCR_RESET);

   //Wait for the reset to complete
   while(ip101ReadPhyReg(interface, IP101_BMCR) & IP101_BMCR_RESET)
   {
   }

   //Dump PHY registers for debugging purpose
   ip101DumpPhyReg(interface);

   //Force the TCP/IP stack to poll the link state at startup
   interface->phyEvent = TRUE;
   //Notify the TCP/IP stack of the event
   osSetEvent(&netEvent);

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief IP101 timer handler
 * @param[in] interface Underlying network interface
 **/

void ip101Tick(NetInterface *interface)
{
   uint16_t value;
   bool_t linkState;

   //Read basic status register
   value = ip101ReadPhyReg(interface, IP101_PHYMCSSR);
   //Retrieve current link state
   linkState = (value & IP101_PHYMCSSR_LINK_UP) ? TRUE : FALSE;

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

void ip101EnableIrq(NetInterface *interface)
{
}


/**
 * @brief Disable interrupts
 * @param[in] interface Underlying network interface
 **/

void ip101DisableIrq(NetInterface *interface)
{
}


/**
 * @brief IP101 event handler
 * @param[in] interface Underlying network interface
 **/

void ip101EventHandler(NetInterface *interface)
{
   uint16_t value;

   //Read PHY status register
   value = ip101ReadPhyReg(interface, IP101_PHYMCSSR);

   //Link is up?
   if((value & IP101_PHYMCSSR_LINK_UP) != 0)
   {
      //Check current operation mode
      switch(value & IP101_PHYMCSSR_OP_MODE)
      {
      //10BASE-T half-duplex
      case IP101_PHYMCSSR_OP_MODE_10M_HD:
         interface->linkSpeed = NIC_LINK_SPEED_10MBPS;
         interface->duplexMode = NIC_HALF_DUPLEX_MODE;
         break;
      //10BASE-T full-duplex
      case IP101_PHYMCSSR_OP_MODE_10M_FD:
         interface->linkSpeed = NIC_LINK_SPEED_10MBPS;
         interface->duplexMode = NIC_FULL_DUPLEX_MODE;
         break;
      //100BASE-TX half-duplex
      case IP101_PHYMCSSR_OP_MODE_100M_HD:
         interface->linkSpeed = NIC_LINK_SPEED_100MBPS;
         interface->duplexMode = NIC_HALF_DUPLEX_MODE;
         break;
      //100BASE-TX full-duplex
      case IP101_PHYMCSSR_OP_MODE_100M_FD:
         interface->linkSpeed = NIC_LINK_SPEED_100MBPS;
         interface->duplexMode = NIC_FULL_DUPLEX_MODE;
         break;
      //Unknown operation mode
      default:
         //Debug message
         TRACE_WARNING("Invalid operation mode!\r\n");
         break;
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

void ip101WritePhyReg(NetInterface *interface, uint8_t address,
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

uint16_t ip101ReadPhyReg(NetInterface *interface, uint8_t address)
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

void ip101DumpPhyReg(NetInterface *interface)
{
   uint8_t i;

   //Loop through PHY registers
   for(i = 0; i < 32; i++)
   {
      //Display current PHY register
      TRACE_DEBUG("%02" PRIu8 ": 0x%04" PRIX16 "\r\n", i,
         ip101ReadPhyReg(interface, i));
   }

   //Terminate with a line feed
   TRACE_DEBUG("\r\n");
}
