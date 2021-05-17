/**
 * @file ksz8795_driver.c
 * @brief KSZ8795 5-port Ethernet switch driver
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
#include "core/ethernet_misc.h"
#include "drivers/switch/ksz8795_driver.h"
#include "debug.h"


/**
 * @brief KSZ8795 Ethernet switch driver
 **/

const SwitchDriver ksz8795SwitchDriver =
{
   ksz8795Init,
   ksz8795Tick,
   ksz8795EnableIrq,
   ksz8795DisableIrq,
   ksz8795EventHandler,
   ksz8795TagFrame,
   ksz8795UntagFrame,
   ksz8795GetLinkState,
   ksz8795GetLinkSpeed,
   ksz8795GetDuplexMode,
   ksz8795SetPortState,
   ksz8795GetPortState,
   ksz8795SetAgingTime,
   ksz8795EnableIgmpSnooping,
   ksz8795EnableMldSnooping,
   ksz8795EnableRsvdMcastTable,
   ksz8795AddStaticFdbEntry,
   ksz8795DeleteStaticFdbEntry,
   ksz8795GetStaticFdbEntry,
   ksz8795FlushStaticFdbTable,
   ksz8795GetDynamicFdbEntry,
   ksz8795FlushDynamicFdbTable,
   ksz8795SetUnknownMcastFwdPorts
};


/**
 * @brief Tail tag rules (host to KSZ8795)
 **/

const uint8_t ksz8795IngressTailTag[5] =
{
   KSZ8795_TAIL_TAG_NORMAL_ADDR_LOOKUP,
   KSZ8795_TAIL_TAG_PORT_SEL | KSZ8795_TAIL_TAG_DEST_PORT1,
   KSZ8795_TAIL_TAG_PORT_SEL | KSZ8795_TAIL_TAG_DEST_PORT2,
   KSZ8795_TAIL_TAG_PORT_SEL | KSZ8795_TAIL_TAG_DEST_PORT3,
   KSZ8795_TAIL_TAG_PORT_SEL | KSZ8795_TAIL_TAG_DEST_PORT4
};


/**
 * @brief KSZ8795 Ethernet switch initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t ksz8795Init(NetInterface *interface)
{
   uint_t port;
   uint8_t temp;

   //Debug message
   TRACE_INFO("Initializing KSZ8795...\r\n");

   //SPI slave mode?
   if(interface->spiDriver != NULL)
   {
      //Initialize SPI interface
      interface->spiDriver->init();

      //Wait for the serial interface to be ready
      do
      {
         //Read CHIP_ID0 register
         temp = ksz8795ReadSwitchReg(interface, KSZ8795_CHIP_ID0);

         //The returned data is invalid until the serial interface is ready
      } while(temp != KSZ8795_CHIP_ID0_FAMILY_ID_DEFAULT);

#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
      //Enable tail tag feature
      temp = ksz8795ReadSwitchReg(interface, KSZ8795_GLOBAL_CTRL10);
      temp |= KSZ8795_GLOBAL_CTRL10_TAIL_TAG_EN;
      ksz8795WriteSwitchReg(interface, KSZ8795_GLOBAL_CTRL10, temp);
#else
      //Disable tail tag feature
      temp = ksz8795ReadSwitchReg(interface, KSZ8795_GLOBAL_CTRL10);
      temp &= ~KSZ8795_GLOBAL_CTRL10_TAIL_TAG_EN;
      ksz8795WriteSwitchReg(interface, KSZ8795_GLOBAL_CTRL10, temp);
#endif

      //Loop through the ports
      for(port = KSZ8795_PORT1; port <= KSZ8795_PORT4; port++)
      {
#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
         //Port separation mode?
         if(interface->port != 0)
         {
            //Disable packet transmission and address learning
            ksz8795SetPortState(interface, port, SWITCH_PORT_STATE_LISTENING);
         }
         else
#endif
         {
            //Enable transmission, reception and address learning
            ksz8795SetPortState(interface, port, SWITCH_PORT_STATE_FORWARDING);
         }
      }

      //Silicon errata workaround #2
      ksz8795WriteSwitchReg(interface, KSZ8795_INDIRECT_CTRL0, 0x20);
      ksz8795WriteSwitchReg(interface, KSZ8795_INDIRECT_CTRL1, 0x35);
      ksz8795WriteSwitchReg(interface, KSZ8795_INDIRECT_BYTE, 0x00);

      //Silicon errata workaround #3
      ksz8795WriteSwitchReg(interface, KSZ8795_INDIRECT_CTRL0, 0xA0);
      ksz8795WriteSwitchReg(interface, KSZ8795_INDIRECT_CTRL1, 0x3C);
      ksz8795WriteSwitchReg(interface, KSZ8795_INDIRECT_BYTE, 0x15);

      //Silicon errata workaround #4
      ksz8795WriteSwitchReg(interface, KSZ8795_INDIRECT_CTRL0, 0xA0);
      ksz8795WriteSwitchReg(interface, KSZ8795_INDIRECT_CTRL1, 0x4E);
      ksz8795WriteSwitchReg(interface, KSZ8795_INDIRECT_BYTE, 0x40);

      //Dump switch registers for debugging purpose
      ksz8795DumpSwitchReg(interface);
   }
   else
   {
      //Initialize serial management interface
      if(interface->smiDriver != NULL)
      {
         interface->smiDriver->init();
      }

      //Loop through the ports
      for(port = KSZ8795_PORT1; port <= KSZ8795_PORT4; port++)
      {
         //Debug message
         TRACE_DEBUG("Port %u:\r\n", port);
         //Dump PHY registers for debugging purpose
         ksz8795DumpPhyReg(interface, port);
      }
   }

   //Force the TCP/IP stack to poll the link state at startup
   interface->phyEvent = TRUE;
   //Notify the TCP/IP stack of the event
   osSetEvent(&netEvent);

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief KSZ8795 timer handler
 * @param[in] interface Underlying network interface
 **/

void ksz8795Tick(NetInterface *interface)
{
   uint_t port;
   bool_t linkState;

#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
   //Port separation mode?
   if(interface->port != 0)
   {
      uint_t i;
      NetInterface *virtualInterface;

      //Loop through network interfaces
      for(i = 0; i < NET_INTERFACE_COUNT; i++)
      {
         //Point to the current interface
         virtualInterface = &netInterface[i];

         //Check whether the current virtual interface is attached to the
         //physical interface
         if(virtualInterface == interface ||
            virtualInterface->parent == interface)
         {
            //Retrieve current link state
            linkState = ksz8795GetLinkState(interface, virtualInterface->port);

            //Link up or link down event?
            if(linkState != virtualInterface->linkState)
            {
               //Set event flag
               interface->phyEvent = TRUE;
               //Notify the TCP/IP stack of the event
               osSetEvent(&netEvent);
            }
         }
      }
   }
   else
#endif
   {
      //Initialize link state
      linkState = FALSE;

      //Loop through the ports
      for(port = KSZ8795_PORT1; port <= KSZ8795_PORT4; port++)
      {
         //Retrieve current link state
         if(ksz8795GetLinkState(interface, port))
         {
            linkState = TRUE;
         }
      }

      //Link up or link down event?
      if(linkState != interface->linkState)
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

void ksz8795EnableIrq(NetInterface *interface)
{
}


/**
 * @brief Disable interrupts
 * @param[in] interface Underlying network interface
 **/

void ksz8795DisableIrq(NetInterface *interface)
{
}


/**
 * @brief KSZ8795 event handler
 * @param[in] interface Underlying network interface
 **/

void ksz8795EventHandler(NetInterface *interface)
{
   uint_t port;
   bool_t linkState;

#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
   //Port separation mode?
   if(interface->port != 0)
   {
      uint_t i;
      NetInterface *virtualInterface;

      //Loop through network interfaces
      for(i = 0; i < NET_INTERFACE_COUNT; i++)
      {
         //Point to the current interface
         virtualInterface = &netInterface[i];

         //Check whether the current virtual interface is attached to the
         //physical interface
         if(virtualInterface == interface ||
            virtualInterface->parent == interface)
         {
            //Get the port number associated with the current interface
            port = virtualInterface->port;

            //Valid port?
            if(port >= KSZ8795_PORT1 && port <= KSZ8795_PORT4)
            {
               //Retrieve current link state
               linkState = ksz8795GetLinkState(interface, port);

               //Link up event?
               if(linkState && !virtualInterface->linkState)
               {
                  //Retrieve host interface speed
                  interface->linkSpeed = ksz8795GetLinkSpeed(interface,
                     KSZ8795_PORT5);

                  //Retrieve host interface duplex mode
                  interface->duplexMode = ksz8795GetDuplexMode(interface,
                     KSZ8795_PORT5);

                  //Adjust MAC configuration parameters for proper operation
                  interface->nicDriver->updateMacConfig(interface);

                  //Check current speed
                  virtualInterface->linkSpeed = ksz8795GetLinkSpeed(interface,
                     port);

                  //Check current duplex mode
                  virtualInterface->duplexMode = ksz8795GetDuplexMode(interface,
                     port);

                  //Update link state
                  virtualInterface->linkState = TRUE;

                  //Process link state change event
                  nicNotifyLinkChange(virtualInterface);
               }
               //Link down event
               else if(!linkState && virtualInterface->linkState)
               {
                  //Update link state
                  virtualInterface->linkState = FALSE;

                  //Process link state change event
                  nicNotifyLinkChange(virtualInterface);
               }
            }
         }
      }
   }
   else
#endif
   {
      //Initialize link state
      linkState = FALSE;

      //Loop through the ports
      for(port = KSZ8795_PORT1; port <= KSZ8795_PORT4; port++)
      {
         //Retrieve current link state
         if(ksz8795GetLinkState(interface, port))
         {
            linkState = TRUE;
         }
      }

      //Link up event?
      if(linkState)
      {
         //Retrieve host interface speed
         interface->linkSpeed = ksz8795GetLinkSpeed(interface, KSZ8795_PORT5);
         //Retrieve host interface duplex mode
         interface->duplexMode = ksz8795GetDuplexMode(interface, KSZ8795_PORT5);

         //Adjust MAC configuration parameters for proper operation
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
}


/**
 * @brief Add tail tag to Ethernet frame
 * @param[in] interface Underlying network interface
 * @param[in] buffer Multi-part buffer containing the payload
 * @param[in,out] offset Offset to the first payload byte
 * @param[in] ancillary Additional options passed to the stack along with
 *   the packet
 * @return Error code
 **/

error_t ksz8795TagFrame(NetInterface *interface, NetBuffer *buffer,
   size_t *offset, NetTxAncillary *ancillary)
{
   error_t error;

   //Initialize status code
   error = NO_ERROR;

#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
   //SPI slave mode?
   if(interface->spiDriver != NULL)
   {
      //Valid port?
      if(ancillary->port <= KSZ8795_PORT4)
      {
         size_t length;
         const uint8_t *tailTag;

         //The one byte tail tagging is used to indicate the destination port
         tailTag = &ksz8795IngressTailTag[ancillary->port];

         //Retrieve the length of the Ethernet frame
         length = netBufferGetLength(buffer) - *offset;

         //The host controller should manually add padding to the packet before
         //inserting the tail tag
         error = ethPadFrame(buffer, &length);

         //Check status code
         if(!error)
         {
            //The tail tag is inserted at the end of the packet, just before
            //the CRC
            error = netBufferAppend(buffer, tailTag, sizeof(uint8_t));
         }
      }
      else
      {
         //The port number is not valid
         error = ERROR_INVALID_PORT;
      }
   }
#endif

   //Return status code
   return error;
}


/**
 * @brief Decode tail tag from incoming Ethernet frame
 * @param[in] interface Underlying network interface
 * @param[in,out] frame Pointer to the received Ethernet frame
 * @param[in,out] length Length of the frame, in bytes
 * @param[in,out] ancillary Additional options passed to the stack along with
 *   the packet
 * @return Error code
 **/

error_t ksz8795UntagFrame(NetInterface *interface, uint8_t **frame,
   size_t *length, NetRxAncillary *ancillary)
{
   error_t error;

   //Initialize status code
   error = NO_ERROR;

#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
   //SPI slave mode?
   if(interface->spiDriver != NULL)
   {
      //Valid Ethernet frame received?
      if(*length >= (sizeof(EthHeader) + sizeof(uint8_t)))
      {
         uint8_t *tailTag;

         //The tail tag is inserted at the end of the packet, just before
         //the CRC
         tailTag = *frame + *length - sizeof(uint8_t);

         //The one byte tail tagging is used to indicate the source port
         ancillary->port = (*tailTag & KSZ8795_TAIL_TAG_SRC_PORT) + 1;

         //Strip tail tag from Ethernet frame
         *length -= sizeof(uint8_t);
      }
      else
      {
         //Drop the received frame
         error = ERROR_INVALID_LENGTH;
      }
   }
   else
   {
      //Tail tagging mode cannot be enabled through MDC/MDIO interface
      ancillary->port = 0;
   }
#endif

   //Return status code
   return error;
}


/**
 * @brief Get link state
 * @param[in] interface Underlying network interface
 * @param[in] port Port number
 * @return Link state
 **/

bool_t ksz8795GetLinkState(NetInterface *interface, uint8_t port)
{
   uint16_t status;
   bool_t linkState;

   //Check port number
   if(port >= KSZ8795_PORT1 && port <= KSZ8795_PORT4)
   {
      //SPI slave mode?
      if(interface->spiDriver != NULL)
      {
         //Read port status 2 register
         status = ksz8795ReadSwitchReg(interface, KSZ8795_PORTn_STAT2(port));

         //Retrieve current link state
         linkState = (status & KSZ8795_PORTn_STAT2_LINK_GOOD) ? TRUE : FALSE;
      }
      else
      {
         //Read status register
         status = ksz8795ReadPhyReg(interface, port, KSZ8795_BMSR);

         //Retrieve current link state
         linkState = (status & KSZ8795_BMSR_LINK_STATUS) ? TRUE : FALSE;
      }
   }
   else
   {
      //The specified port number is not valid
      linkState = FALSE;
   }

   //Return link status
   return linkState;
}


/**
 * @brief Get link speed
 * @param[in] interface Underlying network interface
 * @param[in] port Port number
 * @return Link speed
 **/

uint32_t ksz8795GetLinkSpeed(NetInterface *interface, uint8_t port)
{
   uint8_t type;
   uint8_t value;
   uint32_t linkSpeed;

   //SPI slave mode?
   if(interface->spiDriver != NULL)
   {
      //Check port number
      if(port >= KSZ8795_PORT1 && port <= KSZ8795_PORT4)
      {
         //Read port status 1 register
         value = ksz8795ReadSwitchReg(interface, KSZ8795_PORTn_STAT1(port));

         //Retrieve current link speed
         if((value & KSZ8795_PORTn_STAT1_OP_SPEED) != 0)
         {
            linkSpeed = NIC_LINK_SPEED_100MBPS;
         }
         else
         {
            linkSpeed = NIC_LINK_SPEED_10MBPS;
         }
      }
      else if(port == KSZ8795_PORT5)
      {
         //Read port 5 interface control 6 register
         value = ksz8795ReadSwitchReg(interface, KSZ8795_PORT5_IF_CTRL6);

         //Retrieve host interface type
         type = value & KSZ8795_PORT5_IF_CTRL6_IF_MODE_SEL;

         //Gigabit interface?
         if((type == KSZ8795_PORT5_IF_CTRL6_IF_MODE_SEL_GMII ||
            type == KSZ8795_PORT5_IF_CTRL6_IF_MODE_SEL_RGMII) &&
            (value & KSZ8795_PORT5_IF_CTRL6_IS_1GBPS) != 0)
         {
            //1000 Mb/s mode
            linkSpeed = NIC_LINK_SPEED_1GBPS;
         }
         else
         {
            //Read global control 4 register
            value = ksz8795ReadSwitchReg(interface, KSZ8795_GLOBAL_CTRL4);

            //Retrieve host interface speed
            if((value & KSZ8795_GLOBAL_CTRL4_SW5_SPEED) != 0)
            {
               //10 Mb/s mode
               linkSpeed = NIC_LINK_SPEED_10MBPS;
            }
            else
            {
               //100 Mb/s mode
               linkSpeed = NIC_LINK_SPEED_100MBPS;
            }
         }
      }
      else
      {
         //The specified port number is not valid
         linkSpeed = NIC_LINK_SPEED_UNKNOWN;
      }
   }
   else
   {
      //The MDC/MDIO interface does not have access to all the configuration
      //registers. It can only access the standard MIIM registers
      linkSpeed = NIC_LINK_SPEED_100MBPS;
   }

   //Return link status
   return linkSpeed;
}


/**
 * @brief Get duplex mode
 * @param[in] interface Underlying network interface
 * @param[in] port Port number
 * @return Duplex mode
 **/

NicDuplexMode ksz8795GetDuplexMode(NetInterface *interface, uint8_t port)
{
   uint8_t value;
   NicDuplexMode duplexMode;

   //SPI slave mode?
   if(interface->spiDriver != NULL)
   {
      //Check port number
      if(port >= KSZ8795_PORT1 && port <= KSZ8795_PORT4)
      {
         //Read port status 1 register
         value = ksz8795ReadSwitchReg(interface, KSZ8795_PORTn_STAT1(port));

         //Retrieve current duplex mode
         if((value & KSZ8795_PORTn_STAT1_OP_DUPLEX) != 0)
         {
            duplexMode = NIC_FULL_DUPLEX_MODE;
         }
         else
         {
            duplexMode = NIC_HALF_DUPLEX_MODE;
         }
      }
      else if(port == KSZ8795_PORT5)
      {
         //Read global control 4 register
         value = ksz8795ReadSwitchReg(interface, KSZ8795_GLOBAL_CTRL4);

         //Retrieve host interface duplex mode
         if((value & KSZ8795_GLOBAL_CTRL4_SW5_HALF_DUPLEX_MODE) != 0)
         {
            duplexMode = NIC_HALF_DUPLEX_MODE;
         }
         else
         {
            duplexMode = NIC_FULL_DUPLEX_MODE;
         }
      }
      else
      {
         //The specified port number is not valid
         duplexMode = NIC_UNKNOWN_DUPLEX_MODE;
      }
   }
   else
   {
      //The MDC/MDIO interface does not have access to all the configuration
      //registers. It can only access the standard MIIM registers
      duplexMode = NIC_FULL_DUPLEX_MODE;
   }

   //Return duplex mode
   return duplexMode;
}


/**
 * @brief Set port state
 * @param[in] interface Underlying network interface
 * @param[in] port Port number
 * @param[in] state Port state
 **/

void ksz8795SetPortState(NetInterface *interface, uint8_t port,
   SwitchPortState state)
{
   uint8_t temp;

   //Check port number
   if(port >= KSZ8795_PORT1 && port <= KSZ8795_PORT4)
   {
      //Read port control 2 register
      temp = ksz8795ReadSwitchReg(interface, KSZ8795_PORTn_CTRL2(port));

      //Update port state
      switch(state)
      {
      //Listening state
      case SWITCH_PORT_STATE_LISTENING:
         temp &= ~KSZ8795_PORTn_CTRL2_TRANSMIT_EN;
         temp |= KSZ8795_PORTn_CTRL2_RECEIVE_EN;
         temp |= KSZ8795_PORTn_CTRL2_LEARNING_DIS;
         break;

      //Learning state
      case SWITCH_PORT_STATE_LEARNING:
         temp &= ~KSZ8795_PORTn_CTRL2_TRANSMIT_EN;
         temp &= ~KSZ8795_PORTn_CTRL2_RECEIVE_EN;
         temp &= ~KSZ8795_PORTn_CTRL2_LEARNING_DIS;
         break;

      //Forwarding state
      case SWITCH_PORT_STATE_FORWARDING:
         temp |= KSZ8795_PORTn_CTRL2_TRANSMIT_EN;
         temp |= KSZ8795_PORTn_CTRL2_RECEIVE_EN;
         temp &= ~KSZ8795_PORTn_CTRL2_LEARNING_DIS;
         break;

      //Disabled state
      default:
         temp &= ~KSZ8795_PORTn_CTRL2_TRANSMIT_EN;
         temp &= ~KSZ8795_PORTn_CTRL2_RECEIVE_EN;
         temp |= KSZ8795_PORTn_CTRL2_LEARNING_DIS;
         break;
      }

      //Write the value back to port control 2 register
      ksz8795WriteSwitchReg(interface, KSZ8795_PORTn_CTRL2(port), temp);
   }
}


/**
 * @brief Get port state
 * @param[in] interface Underlying network interface
 * @param[in] port Port number
 * @return Port state
 **/

SwitchPortState ksz8795GetPortState(NetInterface *interface, uint8_t port)
{
   uint8_t temp;
   SwitchPortState state;

   //Check port number
   if(port >= KSZ8795_PORT1 && port <= KSZ8795_PORT4)
   {
      //Read port control 2 register
      temp = ksz8795ReadSwitchReg(interface, KSZ8795_PORTn_CTRL2(port));

      //Check port state
      if((temp & KSZ8795_PORTn_CTRL2_TRANSMIT_EN) == 0 &&
         (temp & KSZ8795_PORTn_CTRL2_RECEIVE_EN) == 0 &&
         (temp & KSZ8795_PORTn_CTRL2_LEARNING_DIS) != 0)
      {
         //Disabled state
         state = SWITCH_PORT_STATE_DISABLED;
      }
      else if((temp & KSZ8795_PORTn_CTRL2_TRANSMIT_EN) == 0 &&
         (temp & KSZ8795_PORTn_CTRL2_RECEIVE_EN) != 0 &&
         (temp & KSZ8795_PORTn_CTRL2_LEARNING_DIS) != 0)
      {
         //Listening state
         state = SWITCH_PORT_STATE_LISTENING;
      }
      else if((temp & KSZ8795_PORTn_CTRL2_TRANSMIT_EN) == 0 &&
         (temp & KSZ8795_PORTn_CTRL2_RECEIVE_EN) == 0 &&
         (temp & KSZ8795_PORTn_CTRL2_LEARNING_DIS) == 0)
      {
         //Learning state
         state = SWITCH_PORT_STATE_LEARNING;
      }
      else if((temp & KSZ8795_PORTn_CTRL2_TRANSMIT_EN) != 0 &&
         (temp & KSZ8795_PORTn_CTRL2_RECEIVE_EN) != 0 &&
         (temp & KSZ8795_PORTn_CTRL2_LEARNING_DIS) == 0)
      {
         //Forwarding state
         state = SWITCH_PORT_STATE_FORWARDING;
      }
      else
      {
         //Unknown state
         state = SWITCH_PORT_STATE_UNKNOWN;
      }
   }
   else
   {
      //The specified port number is not valid
      state = SWITCH_PORT_STATE_DISABLED;
   }

   //Return port state
   return state;
}


/**
 * @brief Set aging time for dynamic filtering entries
 * @param[in] interface Underlying network interface
 * @param[in] agingTime Aging time, in seconds
 **/

void ksz8795SetAgingTime(NetInterface *interface, uint32_t agingTime)
{
   //The aging period is fixed to 300 seconds
}


/**
 * @brief Enable IGMP snooping
 * @param[in] interface Underlying network interface
 * @param[in] enable Enable or disable IGMP snooping
 **/

void ksz8795EnableIgmpSnooping(NetInterface *interface, bool_t enable)
{
   uint8_t temp;

   //Read global control 3 register
   temp = ksz8795ReadSwitchReg(interface, KSZ8795_GLOBAL_CTRL3);

   //Enable or disable IGMP snooping
   if(enable)
   {
      temp |= KSZ8795_GLOBAL_CTRL3_SW5_IGMP_SNOOP_EN;
   }
   else
   {
      temp &= ~KSZ8795_GLOBAL_CTRL3_SW5_IGMP_SNOOP_EN;
   }

   //Write the value back to global control 3 register
   ksz8795WriteSwitchReg(interface, KSZ8795_GLOBAL_CTRL3, temp);
}


/**
 * @brief Enable MLD snooping
 * @param[in] interface Underlying network interface
 * @param[in] enable Enable or disable MLD snooping
 **/

void ksz8795EnableMldSnooping(NetInterface *interface, bool_t enable)
{
   //Not implemented
}


/**
 * @brief Enable reserved multicast table
 * @param[in] interface Underlying network interface
 * @param[in] enable Enable or disable reserved group addresses
 **/

void ksz8795EnableRsvdMcastTable(NetInterface *interface, bool_t enable)
{
   uint_t i;
   SwitchFdbEntry entry;

   //The reserved group addresses are in the range of 01-80-C2-00-00-00 to
   //01-80-C2-00-00-0F
   for(i = 0; i <= 15; i++)
   {
      //Specify the reserved group address to be added or removed
      entry.macAddr.b[0] = 0x01;
      entry.macAddr.b[1] = 0x80;
      entry.macAddr.b[2] = 0xC2;
      entry.macAddr.b[3] = 0x00;
      entry.macAddr.b[4] = 0x00;
      entry.macAddr.b[5] = i;

      //Format forwarding database entry
      entry.srcPort = 0;
      entry.destPorts = SWITCH_CPU_PORT_MASK;
      entry.override = TRUE;

      //Update the static MAC table
      if(enable)
      {
         ksz8795AddStaticFdbEntry(interface, &entry);
      }
      else
      {
         ksz8795DeleteStaticFdbEntry(interface, &entry);
      }
   }
}


/**
 * @brief Add a new entry to the static MAC table
 * @param[in] interface Underlying network interface
 * @param[in] entry Pointer to the forwarding database entry
 * @return Error code
 **/

error_t ksz8795AddStaticFdbEntry(NetInterface *interface,
   const SwitchFdbEntry *entry)
{
   error_t error;
   uint_t i;
   uint_t j;
   uint8_t *p;
   SwitchFdbEntry currentEntry;
   Ksz8795StaticMacEntryW newEntry;

   //Keep track of the first free entry
   j = KSZ8795_STATIC_MAC_TABLE_SIZE;

   //Loop through the static MAC table
   for(i = 0; i < KSZ8795_STATIC_MAC_TABLE_SIZE; i++)
   {
      //Read current entry
      error = ksz8795GetStaticFdbEntry(interface, i, &currentEntry);

      //Valid entry?
      if(!error)
      {
         //Check whether the table already contains the specified MAC address
         if(macCompAddr(&currentEntry.macAddr, &entry->macAddr))
         {
            j = i;
            break;
         }
      }
      else
      {
         //Keep track of the first free entry
         if(j == KSZ8795_STATIC_MAC_TABLE_SIZE)
         {
            j = i;
         }
      }
   }

   //Any entry available?
   if(j < KSZ8795_STATIC_MAC_TABLE_SIZE)
   {
      //Format MAC entry
      newEntry.fid = 0;
      newEntry.useFid = 0;
      newEntry.override = entry->override;
      newEntry.valid = TRUE;
      newEntry.macAddr = entry->macAddr;

      //Set the relevant forward ports
      if(entry->destPorts == SWITCH_CPU_PORT_MASK)
      {
         newEntry.forwardPorts = KSZ8795_PORT5_MASK;
      }
      else
      {
         newEntry.forwardPorts = entry->destPorts & KSZ8795_PORT_MASK;
      }

      //Select the static MAC address table
      ksz8795WriteSwitchReg(interface, KSZ8795_INDIRECT_CTRL0,
         KSZ8795_INDIRECT_CTRL0_READ |
         KSZ8795_INDIRECT_CTRL0_TABLE_SEL_STATIC_MAC);

      //Point to the MAC entry
      p = (uint8_t *) &newEntry;

      //Write indirect data registers
      for(i = 0; i < sizeof(Ksz8795StaticMacEntryW); i++)
      {
         ksz8795WriteSwitchReg(interface, KSZ8795_INDIRECT_DATA7 + i, p[i]);
      }

      //Setup a write operation
      ksz8795WriteSwitchReg(interface, KSZ8795_INDIRECT_CTRL0,
         KSZ8795_INDIRECT_CTRL0_WRITE |
         KSZ8795_INDIRECT_CTRL0_TABLE_SEL_STATIC_MAC);

      //Trigger the write operation
      ksz8795WriteSwitchReg(interface, KSZ8795_INDIRECT_CTRL1, j);

      //Successful processing
      error = NO_ERROR;
   }
   else
   {
      //The static MAC table is full
      error = ERROR_TABLE_FULL;
   }

   //Return status code
   return error;
}


/**
 * @brief Remove an entry from the static MAC table
 * @param[in] interface Underlying network interface
 * @param[in] entry Forwarding database entry to remove from the table
 * @return Error code
 **/

error_t ksz8795DeleteStaticFdbEntry(NetInterface *interface,
   const SwitchFdbEntry *entry)
{
   error_t error;
   uint_t i;
   uint_t j;
   SwitchFdbEntry currentEntry;

   //Loop through the static MAC table
   for(j = 0; j < KSZ8795_STATIC_MAC_TABLE_SIZE; j++)
   {
      //Read current entry
      error = ksz8795GetStaticFdbEntry(interface, j, &currentEntry);

      //Valid entry?
      if(!error)
      {
         //Check whether the table contains the specified MAC address
         if(macCompAddr(&currentEntry.macAddr, &entry->macAddr))
         {
            break;
         }
      }
   }

   //Any matching entry?
   if(j < KSZ8795_STATIC_MAC_TABLE_SIZE)
   {
      //Select the static MAC address table
      ksz8795WriteSwitchReg(interface, KSZ8795_INDIRECT_CTRL0,
         KSZ8795_INDIRECT_CTRL0_READ |
         KSZ8795_INDIRECT_CTRL0_TABLE_SEL_STATIC_MAC);

      //Clear indirect data registers
      for(i = 0; i < sizeof(Ksz8795StaticMacEntryW); i++)
      {
         ksz8795WriteSwitchReg(interface, KSZ8795_INDIRECT_DATA7 + i, 0);
      }

      //Setup a write operation
      ksz8795WriteSwitchReg(interface, KSZ8795_INDIRECT_CTRL0,
         KSZ8795_INDIRECT_CTRL0_WRITE |
         KSZ8795_INDIRECT_CTRL0_TABLE_SEL_STATIC_MAC);

      //Trigger the write operation
      ksz8795WriteSwitchReg(interface, KSZ8795_INDIRECT_CTRL1, j);

      //Successful processing
      error = NO_ERROR;
   }
   else
   {
      //The static MAC table does not contain the specified address
      error = ERROR_NOT_FOUND;
   }

   //Return status code
   return error;
}


/**
 * @brief Read an entry from the static MAC table
 * @param[in] interface Underlying network interface
 * @param[in] index Zero-based index of the entry to read
 * @param[out] entry Pointer to the forwarding database entry
 * @return Error code
 **/

error_t ksz8795GetStaticFdbEntry(NetInterface *interface, uint_t index,
   SwitchFdbEntry *entry)
{
   error_t error;
   uint_t i;
   uint8_t *p;
   Ksz8795StaticMacEntryR currentEntry;

   //Check index parameter
   if(index < KSZ8795_STATIC_MAC_TABLE_SIZE)
   {
      //Select the static MAC address table
      ksz8795WriteSwitchReg(interface, KSZ8795_INDIRECT_CTRL0,
         KSZ8795_INDIRECT_CTRL0_READ |
         KSZ8795_INDIRECT_CTRL0_TABLE_SEL_STATIC_MAC);

      //Trigger the read operation
      ksz8795WriteSwitchReg(interface, KSZ8795_INDIRECT_CTRL1, index);

      //Point to the MAC entry
      p = (uint8_t *) &currentEntry;

      //Read indirect data registers
      for(i = 0; i < sizeof(Ksz8795StaticMacEntryR); i++)
      {
         p[i] = ksz8795ReadSwitchReg(interface, KSZ8795_INDIRECT_DATA7 + i);
      }

      //Valid entry?
      if(currentEntry.valid)
      {
         //Copy MAC entry
         entry->macAddr = currentEntry.macAddr;
         entry->srcPort = 0;
         entry->destPorts = currentEntry.forwardPorts & KSZ8795_PORT_MASK;
         entry->override = currentEntry.override;

         //Successful processing
         error = NO_ERROR;
      }
      else
      {
         //The entry is not valid
         error = ERROR_INVALID_ENTRY;
      }
   }
   else
   {
      //The end of the table has been reached
      error = ERROR_END_OF_TABLE;
   }

   //Return status code
   return error;
}


/**
 * @brief Flush static MAC table
 * @param[in] interface Underlying network interface
 **/

void ksz8795FlushStaticFdbTable(NetInterface *interface)
{
   uint_t i;
   uint_t temp;
   uint8_t state[5];

   //Loop through the ports
   for(i = KSZ8795_PORT1; i <= KSZ8795_PORT5; i++)
   {
      //Save the current state of the port
      state[i - 1] = ksz8795ReadSwitchReg(interface, KSZ8795_PORTn_CTRL2(i));

      //Turn off learning capability
      ksz8795WriteSwitchReg(interface, KSZ8795_PORTn_CTRL2(i),
         state[i - 1] | KSZ8795_PORTn_CTRL2_LEARNING_DIS);
   }

   //All the entries associated with a port that has its learning capability
   //being turned off will be flushed
   temp = ksz8795ReadSwitchReg(interface, KSZ8795_GLOBAL_CTRL0);
   temp |= KSZ8795_GLOBAL_CTRL0_FLUSH_STATIC_MAC_TABLE;
   ksz8795WriteSwitchReg(interface, KSZ8795_GLOBAL_CTRL0, temp);

   //Loop through the ports
   for(i = KSZ8795_PORT1; i <= KSZ8795_PORT5; i++)
   {
      //Restore the original state of the port
      ksz8795WriteSwitchReg(interface, KSZ8795_PORTn_CTRL2(i), state[i - 1]);
   }
}


/**
 * @brief Read an entry from the dynamic MAC table
 * @param[in] interface Underlying network interface
 * @param[in] index Zero-based index of the entry to read
 * @param[out] entry Pointer to the forwarding database entry
 * @return Error code
 **/

error_t ksz8795GetDynamicFdbEntry(NetInterface *interface, uint_t index,
   SwitchFdbEntry *entry)
{
   error_t error;
   uint_t i;
   uint_t n;
   uint8_t *p;
   Ksz8795DynamicMacEntry currentEntry;

   //Check index parameter
   if(index < KSZ8795_DYNAMIC_MAC_TABLE_SIZE)
   {
      //Read the MAC entry at the specified index
      do
      {
         //Select the dynamic MAC address table
         ksz8795WriteSwitchReg(interface, KSZ8795_INDIRECT_CTRL0,
            KSZ8795_INDIRECT_CTRL0_READ |
            KSZ8795_INDIRECT_CTRL0_TABLE_SEL_DYNAMIC_MAC |
            (MSB(index) & KSZ8795_INDIRECT_CTRL0_ADDR_9_8));

         //Trigger the read operation
         ksz8795WriteSwitchReg(interface, KSZ8795_INDIRECT_CTRL1, LSB(index));

         //Point to the MAC entry
         p = (uint8_t *) &currentEntry;

         //Read indirect data registers
         for(i = 0; i < sizeof(Ksz8795DynamicMacEntry); i++)
         {
            p[i] = ksz8795ReadSwitchReg(interface, KSZ8795_INDIRECT_DATA8 + i);
         }

         //Retry until the entry is ready
      } while(currentEntry.dataNotReady);

      //Check whether there are valid entries in the table
      if(!currentEntry.macEmpty)
      {
         //Retrieve the number of valid entries
         n = ((currentEntry.numValidEntriesH << 3) |
            currentEntry.numValidEntriesL) + 1;
      }
      else
      {
         //The table is empty
         n = 0;
      }

      //Valid entry?
      if(index < n)
      {
         //Copy MAC entry
         entry->macAddr = currentEntry.macAddr;
         entry->srcPort = currentEntry.sourcePort + 1;
         entry->destPorts = 0;
         entry->override = FALSE;

         //Successful processing
         error = NO_ERROR;
      }
      else
      {
         //The end of the table has been reached
         error = ERROR_END_OF_TABLE;
      }
   }
   else
   {
      //The end of the table has been reached
      error = ERROR_END_OF_TABLE;
   }

   //Return status code
   return error;
}


/**
 * @brief Flush dynamic MAC table
 * @param[in] interface Underlying network interface
 * @param[in] port Port number
 **/

void ksz8795FlushDynamicFdbTable(NetInterface *interface, uint8_t port)
{
   uint_t i;
   uint_t temp;
   uint8_t state[5];

   //Loop through the ports
   for(i = KSZ8795_PORT1; i <= KSZ8795_PORT5; i++)
   {
      //Matching port number?
      if(i == port || port == 0)
      {
         //Save the current state of the port
         state[i - 1] = ksz8795ReadSwitchReg(interface, KSZ8795_PORTn_CTRL2(i));

         //Turn off learning capability
         ksz8795WriteSwitchReg(interface, KSZ8795_PORTn_CTRL2(i),
            state[i - 1] | KSZ8795_PORTn_CTRL2_LEARNING_DIS);
      }
   }

   //All the entries associated with a port that has its learning capability
   //being turned off will be flushed
   temp = ksz8795ReadSwitchReg(interface, KSZ8795_GLOBAL_CTRL0);
   temp |= KSZ8795_GLOBAL_CTRL0_FLUSH_DYNAMIC_MAC_TABLE;
   ksz8795WriteSwitchReg(interface, KSZ8795_GLOBAL_CTRL0, temp);

   //Loop through the ports
   for(i = KSZ8795_PORT1; i <= KSZ8795_PORT5; i++)
   {
      //Matching port number?
      if(i == port || port == 0)
      {
         //Restore the original state of the port
         ksz8795WriteSwitchReg(interface, KSZ8795_PORTn_CTRL2(i), state[i - 1]);
      }
   }
}


/**
 * @brief Set forward ports for unknown multicast packets
 * @param[in] interface Underlying network interface
 * @param[in] enable Enable or disable forwarding of unknown multicast packets
 * @param[in] forwardPorts Port map
 **/

void ksz8795SetUnknownMcastFwdPorts(NetInterface *interface,
   bool_t enable, uint32_t forwardPorts)
{
   uint8_t temp;

   //Read global control 16 register
   temp = ksz8795ReadSwitchReg(interface, KSZ8795_GLOBAL_CTRL16);

   //Clear port map
   temp &= ~KSZ8795_GLOBAL_CTRL16_UNKNOWN_MCAST_FWD_MAP;

   //Enable or disable forwarding of unknown multicast packets
   if(enable)
   {
      //Enable forwarding
      temp |= KSZ8795_GLOBAL_CTRL16_UNKNOWN_MCAST_FWD;

      //Check whether unknown multicast packets should be forwarded to the CPU port
      if((forwardPorts & SWITCH_CPU_PORT_MASK) != 0)
      {
         temp |= KSZ8795_GLOBAL_CTRL16_UNKNOWN_MCAST_FWD_MAP_PORT5;
      }

      //Select the desired forward ports
      temp |= forwardPorts & KSZ8795_GLOBAL_CTRL16_UNKNOWN_MCAST_FWD_MAP_ALL;
   }
   else
   {
      //Disable forwarding
      temp &= ~KSZ8795_GLOBAL_CTRL16_UNKNOWN_MCAST_FWD;
   }

   //Write the value back to global control 16 register
   ksz8795WriteSwitchReg(interface, KSZ8795_GLOBAL_CTRL16, temp);
}


/**
 * @brief Write PHY register
 * @param[in] interface Underlying network interface
 * @param[in] port Port number
 * @param[in] address PHY register address
 * @param[in] data Register value
 **/

void ksz8795WritePhyReg(NetInterface *interface, uint8_t port,
   uint8_t address, uint16_t data)
{
   //Write the specified PHY register
   if(interface->smiDriver != NULL)
   {
      interface->smiDriver->writePhyReg(SMI_OPCODE_WRITE, port, address, data);
   }
   else
   {
      interface->nicDriver->writePhyReg(SMI_OPCODE_WRITE, port, address, data);
   }
}


/**
 * @brief Read PHY register
 * @param[in] interface Underlying network interface
 * @param[in] port Port number
 * @param[in] address PHY register address
 * @return Register value
 **/

uint16_t ksz8795ReadPhyReg(NetInterface *interface, uint8_t port,
   uint8_t address)
{
   uint16_t data;

   //Read the specified PHY register
   if(interface->smiDriver != NULL)
   {
      data = interface->smiDriver->readPhyReg(SMI_OPCODE_READ, port, address);
   }
   else
   {
      data = interface->nicDriver->readPhyReg(SMI_OPCODE_READ, port, address);
   }

   //Return the value of the PHY register
   return data;
}


/**
 * @brief Dump PHY registers for debugging purpose
 * @param[in] interface Underlying network interface
 * @param[in] port Port number
 **/

void ksz8795DumpPhyReg(NetInterface *interface, uint8_t port)
{
   uint8_t i;

   //Loop through PHY registers
   for(i = 0; i < 32; i++)
   {
      //Display current PHY register
      TRACE_DEBUG("%02" PRIu8 ": 0x%04" PRIX16 "\r\n", i,
         ksz8795ReadPhyReg(interface, port, i));
   }

   //Terminate with a line feed
   TRACE_DEBUG("\r\n");
}


/**
 * @brief Write switch register
 * @param[in] interface Underlying network interface
 * @param[in] address Switch register address
 * @param[in] data Register value
 **/

void ksz8795WriteSwitchReg(NetInterface *interface, uint16_t address,
   uint8_t data)
{
   uint16_t command;

   //SPI slave mode?
   if(interface->spiDriver != NULL)
   {
      //Set up a write operation
      command = KSZ8795_SPI_CMD_WRITE;
      //Set register address
      command |= (address << 1) & KSZ8795_SPI_CMD_ADDR;

      //Pull the CS pin low
      interface->spiDriver->assertCs();

      //Write 16-bit command
      interface->spiDriver->transfer(MSB(command));
      interface->spiDriver->transfer(LSB(command));

      //Write data
      interface->spiDriver->transfer(data);

      //Terminate the operation by raising the CS pin
      interface->spiDriver->deassertCs();
   }
   else
   {
      //The MDC/MDIO interface does not have access to all the configuration
      //registers. It can only access the standard MIIM registers
   }
}


/**
 * @brief Read switch register
 * @param[in] interface Underlying network interface
 * @param[in] address Switch register address
 * @return Register value
 **/

uint8_t ksz8795ReadSwitchReg(NetInterface *interface, uint16_t address)
{
   uint8_t data;
   uint16_t command;

   //SPI slave mode?
   if(interface->spiDriver != NULL)
   {
      //Set up a read operation
      command = KSZ8795_SPI_CMD_READ;
      //Set register address
      command |= (address << 1) & KSZ8795_SPI_CMD_ADDR;

      //Pull the CS pin low
      interface->spiDriver->assertCs();

      //Write 16-bit command
      interface->spiDriver->transfer(MSB(command));
      interface->spiDriver->transfer(LSB(command));

      //Read data
      data = interface->spiDriver->transfer(0xFF);

      //Terminate the operation by raising the CS pin
      interface->spiDriver->deassertCs();
   }
   else
   {
      //The MDC/MDIO interface does not have access to all the configuration
      //registers. It can only access the standard MIIM registers
      data = 0;
   }

   //Return register value
   return data;
}


/**
 * @brief Dump switch registers for debugging purpose
 * @param[in] interface Underlying network interface
 **/

void ksz8795DumpSwitchReg(NetInterface *interface)
{
   uint16_t i;

   //Loop through switch registers
   for(i = 0; i < 256; i++)
   {
      //Display current switch register
      TRACE_DEBUG("0x%02" PRIX16 " (%02" PRIu16 ") : 0x%02" PRIX8 "\r\n",
         i, i, ksz8795ReadSwitchReg(interface, i));
   }

   //Terminate with a line feed
   TRACE_DEBUG("\r\n");
}
