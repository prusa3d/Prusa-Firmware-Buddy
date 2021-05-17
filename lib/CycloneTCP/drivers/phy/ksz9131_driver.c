/**
 * @file ksz9131_driver.c
 * @brief KSZ9131 Gigabit Ethernet PHY driver
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
#include "drivers/phy/ksz9131_driver.h"
#include "debug.h"


/**
 * @brief KSZ9131 Ethernet PHY driver
 **/

const PhyDriver ksz9131PhyDriver =
{
   ksz9131Init,
   ksz9131Tick,
   ksz9131EnableIrq,
   ksz9131DisableIrq,
   ksz9131EventHandler
};


/**
 * @brief KSZ9131 PHY transceiver initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t ksz9131Init(NetInterface *interface)
{
   //Debug message
   TRACE_INFO("Initializing KSZ9131...\r\n");

   //Undefined PHY address?
   if(interface->phyAddr >= 32)
   {
      //Use the default address
      interface->phyAddr = KSZ9131_PHY_ADDR;
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
   ksz9131WritePhyReg(interface, KSZ9131_BMCR, KSZ9131_BMCR_RESET);

   //Wait for the reset to complete
   while(ksz9131ReadPhyReg(interface, KSZ9131_BMCR) & KSZ9131_BMCR_RESET)
   {
   }

   //Dump PHY registers for debugging purpose
   ksz9131DumpPhyReg(interface);

   //The PHY will generate interrupts when link status changes are detected
   ksz9131WritePhyReg(interface, KSZ9131_ICSR, KSZ9131_ICSR_LINK_DOWN_IE |
      KSZ9131_ICSR_LINK_UP_IE);

   //Force the TCP/IP stack to poll the link state at startup
   interface->phyEvent = TRUE;
   //Notify the TCP/IP stack of the event
   osSetEvent(&netEvent);

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief KSZ9131 timer handler
 * @param[in] interface Underlying network interface
 **/

void ksz9131Tick(NetInterface *interface)
{
   uint16_t value;
   bool_t linkState;

   //No external interrupt line driver?
   if(interface->extIntDriver == NULL)
   {
      //Read basic status register
      value = ksz9131ReadPhyReg(interface, KSZ9131_BMSR);
      //Retrieve current link state
      linkState = (value & KSZ9131_BMSR_LINK_STATUS) ? TRUE : FALSE;

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

void ksz9131EnableIrq(NetInterface *interface)
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

void ksz9131DisableIrq(NetInterface *interface)
{
   //Disable PHY transceiver interrupts
   if(interface->extIntDriver != NULL)
   {
      interface->extIntDriver->disableIrq();
   }
}


/**
 * @brief KSZ9131 event handler
 * @param[in] interface Underlying network interface
 **/

void ksz9131EventHandler(NetInterface *interface)
{
   uint16_t value;

   //Read status register to acknowledge the interrupt
   value = ksz9131ReadPhyReg(interface, KSZ9131_ICSR);

   //Link status change?
   if((value & (KSZ9131_ICSR_LINK_DOWN_IF | KSZ9131_ICSR_LINK_UP_IF)) != 0)
   {
      //Any link failure condition is latched in the BMSR register. Reading
      //the register twice will always return the actual link status
      value = ksz9131ReadPhyReg(interface, KSZ9131_BMSR);
      value = ksz9131ReadPhyReg(interface, KSZ9131_BMSR);

      //Link is up?
      if((value & KSZ9131_BMSR_LINK_STATUS) != 0)
      {
         //Read PHY control register
         value = ksz9131ReadPhyReg(interface, KSZ9131_PHYCON);

         //Check current speed
         if((value & KSZ9131_PHYCON_SPEED_1000BT) != 0)
         {
            //1000BASE-T
            interface->linkSpeed = NIC_LINK_SPEED_1GBPS;
         }
         else if((value & KSZ9131_PHYCON_SPEED_100BTX) != 0)
         {
            //100BASE-TX
            interface->linkSpeed = NIC_LINK_SPEED_100MBPS;
         }
         else if((value & KSZ9131_PHYCON_SPEED_10BT) != 0)
         {
            //10BASE-T
            interface->linkSpeed = NIC_LINK_SPEED_10MBPS;
         }
         else
         {
            //Debug message
            TRACE_WARNING("Invalid speed!\r\n");
         }

         //Check current duplex mode
         if((value & KSZ9131_PHYCON_DUPLEX_STATUS) != 0)
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
}


/**
 * @brief Write PHY register
 * @param[in] interface Underlying network interface
 * @param[in] address PHY register address
 * @param[in] data Register value
 **/

void ksz9131WritePhyReg(NetInterface *interface, uint8_t address,
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

uint16_t ksz9131ReadPhyReg(NetInterface *interface, uint8_t address)
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

void ksz9131DumpPhyReg(NetInterface *interface)
{
   uint8_t i;

   //Loop through PHY registers
   for(i = 0; i < 32; i++)
   {
      //Display current PHY register
      TRACE_DEBUG("%02" PRIu8 ": 0x%04" PRIX16 "\r\n", i,
         ksz9131ReadPhyReg(interface, i));
   }

   //Terminate with a line feed
   TRACE_DEBUG("\r\n");
}
