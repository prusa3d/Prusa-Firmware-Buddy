/**
 * @file dm9161_driver.c
 * @brief DM9161 Ethernet PHY driver
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
#include "drivers/phy/dm9161_driver.h"
#include "debug.h"


/**
 * @brief DM9161 Ethernet PHY driver
 **/

const PhyDriver dm9161PhyDriver =
{
   dm9161Init,
   dm9161Tick,
   dm9161EnableIrq,
   dm9161DisableIrq,
   dm9161EventHandler
};


/**
 * @brief DM9161 PHY transceiver initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t dm9161Init(NetInterface *interface)
{
   //Debug message
   TRACE_INFO("Initializing DM9161...\r\n");

   //Undefined PHY address?
   if(interface->phyAddr >= 32)
   {
      //Use the default address
      interface->phyAddr = DM9161_PHY_ADDR;
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
   dm9161WritePhyReg(interface, DM9161_BMCR, DM9161_BMCR_RESET);

   //Wait for the reset to complete
   while(dm9161ReadPhyReg(interface, DM9161_BMCR) & DM9161_BMCR_RESET)
   {
   }

   //Dump PHY registers for debugging purpose
   dm9161DumpPhyReg(interface);

   //The PHY will generate interrupts when link status changes are detected
   dm9161WritePhyReg(interface, DM9161_MDINTR, ~(DM9161_MDINTR_LINK_MASK |
      DM9161_MDINTR_INTR_MASK));

   //Force the TCP/IP stack to poll the link state at startup
   interface->phyEvent = TRUE;
   //Notify the TCP/IP stack of the event
   osSetEvent(&netEvent);

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief DM9161 timer handler
 * @param[in] interface Underlying network interface
 **/

void dm9161Tick(NetInterface *interface)
{
   uint16_t value;
   bool_t linkState;

   //No external interrupt line driver?
   if(interface->extIntDriver == NULL)
   {
      //Read basic status register
      value = dm9161ReadPhyReg(interface, DM9161_BMSR);
      //Retrieve current link state
      linkState = (value & DM9161_BMSR_LINK_STATUS) ? TRUE : FALSE;

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

void dm9161EnableIrq(NetInterface *interface)
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

void dm9161DisableIrq(NetInterface *interface)
{
   //Disable PHY transceiver interrupts
   if(interface->extIntDriver != NULL)
   {
      interface->extIntDriver->disableIrq();
   }
}


/**
 * @brief DM9161 event handler
 * @param[in] interface Underlying network interface
 **/

void dm9161EventHandler(NetInterface *interface)
{
   uint16_t value;
   bool_t end;

   //Read status register to acknowledge the interrupt
   value = dm9161ReadPhyReg(interface, DM9161_MDINTR);

   //Link status change?
   if((value & DM9161_MDINTR_LINK_CHANGE) != 0)
   {
      //Any link failure condition is latched in the BMSR register. Reading
      //the register twice will always return the actual link status
      value = dm9161ReadPhyReg(interface, DM9161_BMSR);
      value = dm9161ReadPhyReg(interface, DM9161_BMSR);

      //Link is up?
      if((value & DM9161_BMSR_LINK_STATUS) != 0)
      {
         //Wait for the auto-negotiation to complete
         do
         {
            //Read DSCSR register
            value = dm9161ReadPhyReg(interface, DM9161_DSCSR);

            //Check current state
            switch(value & DM9161_DSCSR_ANMB)
            {
            //Auto-negotiation is still in progress?
            case DM9161_DSCSR_ANMB_ABILITY_MATCH:
            case DM9161_DSCSR_ANMB_ACK_MATCH:
            case DM9161_DSCSR_ANMB_CONSIST_MATCH:
            case DM9161_DSCSR_ANMB_LINK_READY:
               end = FALSE;
               break;
            //Auto-negotiation is complete?
            default:
               end = TRUE;
               break;
            }

            //Check loop condition variable
         } while(!end);

         //Read DSCSR register
         value = dm9161ReadPhyReg(interface, DM9161_DSCSR);

         //Check current operation mode
         if((value & DM9161_DSCSR_10HDX) != 0)
         {
            //10BASE-T half-duplex
            interface->linkSpeed = NIC_LINK_SPEED_10MBPS;
            interface->duplexMode = NIC_HALF_DUPLEX_MODE;
         }
         else if((value & DM9161_DSCSR_10FDX) != 0)
         {
            //10BASE-T full-duplex
            interface->linkSpeed = NIC_LINK_SPEED_10MBPS;
            interface->duplexMode = NIC_FULL_DUPLEX_MODE;
         }
         else if((value & DM9161_DSCSR_100HDX) != 0)
         {
            //100BASE-TX half-duplex
            interface->linkSpeed = NIC_LINK_SPEED_100MBPS;
            interface->duplexMode = NIC_HALF_DUPLEX_MODE;
         }
         else if((value & DM9161_DSCSR_100FDX) != 0)
         {
            //100BASE-TX full-duplex
            interface->linkSpeed = NIC_LINK_SPEED_100MBPS;
            interface->duplexMode = NIC_FULL_DUPLEX_MODE;
         }
         else
         {
            //Debug message
            TRACE_WARNING("Invalid operation mode!\r\n");
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

void dm9161WritePhyReg(NetInterface *interface, uint8_t address,
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

uint16_t dm9161ReadPhyReg(NetInterface *interface, uint8_t address)
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

void dm9161DumpPhyReg(NetInterface *interface)
{
   uint8_t i;

   //Loop through PHY registers
   for(i = 0; i < 32; i++)
   {
      //Display current PHY register
      TRACE_DEBUG("%02" PRIu8 ": 0x%04" PRIX16 "\r\n", i,
         dm9161ReadPhyReg(interface, i));
   }

   //Terminate with a line feed
   TRACE_DEBUG("\r\n");
}
