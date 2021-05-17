/**
 * @file dp83640_driver.c
 * @brief DP83640 Ethernet PHY driver
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
#include "drivers/phy/dp83640_driver.h"
#include "debug.h"


/**
 * @brief DP83640 Ethernet PHY driver
 **/

const PhyDriver dp83640PhyDriver =
{
   dp83640Init,
   dp83640Tick,
   dp83640EnableIrq,
   dp83640DisableIrq,
   dp83640EventHandler
};


/**
 * @brief DP83640 PHY transceiver initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t dp83640Init(NetInterface *interface)
{
   uint16_t value;

   //Debug message
   TRACE_INFO("Initializing DP83640...\r\n");

   //Undefined PHY address?
   if(interface->phyAddr >= 32)
   {
      //Use the default address
      interface->phyAddr = DP83640_PHY_ADDR;
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

   //A software reset is accomplished by setting the RESET bit of the BMCR register
   dp83640WritePhyReg(interface, DP83640_BMCR, DP83640_BMCR_RESET);

   //Wait for the reset to complete
   while(dp83640ReadPhyReg(interface, DP83640_BMCR) & DP83640_BMCR_RESET)
   {
   }

   //Dump PHY registers for debugging purpose
   dp83640DumpPhyReg(interface);

   //Configure PWR_DOWN/INT pin as an interrupt output
   dp83640WritePhyReg(interface, DP83640_MICR, DP83640_MICR_INTEN |
      DP83640_MICR_INT_OE);

   //The PHY will generate interrupts when link status changes are detected
   dp83640WritePhyReg(interface, DP83640_MISR, DP83640_MISR_LINK_INT_EN);

   //Select page 6
   dp83640WritePhyReg(interface, DP83640_PAGSR, 6);

   //If the CLK_OUT pin is to be used as a 50 MHz RMII clock, the default PTP
   //clock output function must be disabled
   value = dp83640ReadPhyReg(interface, DP83640_PTP_COC);
   value &= ~DP83640_PTP_COC_PTP_CLKOUT_EN;
   dp83640WritePhyReg(interface, DP83640_PTP_COC, value);

   //Select page 0
   dp83640WritePhyReg(interface, DP83640_PAGSR, 0);

   //Force the TCP/IP stack to poll the link state at startup
   interface->phyEvent = TRUE;
   //Notify the TCP/IP stack of the event
   osSetEvent(&netEvent);

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief DP83640 timer handler
 * @param[in] interface Underlying network interface
 **/

void dp83640Tick(NetInterface *interface)
{
   uint16_t value;
   bool_t linkState;

   //No external interrupt line driver?
   if(interface->extIntDriver == NULL)
   {
      //Read basic status register
      value = dp83640ReadPhyReg(interface, DP83640_BMSR);
      //Retrieve current link state
      linkState = (value & DP83640_BMSR_LINK_STATUS) ? TRUE : FALSE;

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

void dp83640EnableIrq(NetInterface *interface)
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

void dp83640DisableIrq(NetInterface *interface)
{
   //Disable PHY transceiver interrupts
   if(interface->extIntDriver != NULL)
   {
      interface->extIntDriver->disableIrq();
   }
}


/**
 * @brief DP83640 event handler
 * @param[in] interface Underlying network interface
 **/

void dp83640EventHandler(NetInterface *interface)
{
   uint16_t status;

   //Read status register to acknowledge the interrupt
   status = dp83640ReadPhyReg(interface, DP83640_MISR);

   //Link status change?
   if((status & DP83640_MISR_LINK_INT) != 0)
   {
      //Read PHY status register
      status = dp83640ReadPhyReg(interface, DP83640_PHYSTS);

      //Link is up?
      if((status & DP83640_PHYSTS_LINK_STATUS) != 0)
      {
         //Check current speed
         if((status & DP83640_PHYSTS_SPEED_STATUS) != 0)
         {
            interface->linkSpeed = NIC_LINK_SPEED_10MBPS;
         }
         else
         {
            interface->linkSpeed = NIC_LINK_SPEED_100MBPS;
         }

         //Check duplex mode
         if((status & DP83640_PHYSTS_DUPLEX_STATUS) != 0)
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

void dp83640WritePhyReg(NetInterface *interface, uint8_t address,
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

uint16_t dp83640ReadPhyReg(NetInterface *interface, uint8_t address)
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

void dp83640DumpPhyReg(NetInterface *interface)
{
   uint8_t i;

   //Loop through PHY registers
   for(i = 0; i < 32; i++)
   {
      //Display current PHY register
      TRACE_DEBUG("%02" PRIu8 ": 0x%04" PRIX16 "\r\n", i,
         dp83640ReadPhyReg(interface, i));
   }

   //Terminate with a line feed
   TRACE_DEBUG("\r\n");
}
