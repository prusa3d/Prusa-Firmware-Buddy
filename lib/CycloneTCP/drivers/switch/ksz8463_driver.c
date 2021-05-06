/**
 * @file ksz8463_driver.c
 * @brief KSZ8463 3-port Ethernet switch driver
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
#include "drivers/switch/ksz8463_driver.h"
#include "debug.h"


/**
 * @brief KSZ8463 Ethernet switch driver
 **/

const SwitchDriver ksz8463SwitchDriver =
{
   ksz8463Init,
   ksz8463Tick,
   ksz8463EnableIrq,
   ksz8463DisableIrq,
   ksz8463EventHandler,
   ksz8463TagFrame,
   ksz8463UntagFrame,
   ksz8463GetLinkState,
   ksz8463GetLinkSpeed,
   ksz8463GetDuplexMode,
   ksz8463SetPortState,
   ksz8463GetPortState,
   ksz8463SetAgingTime,
   ksz8463EnableIgmpSnooping,
   ksz8463EnableMldSnooping,
   ksz8463EnableRsvdMcastTable,
   ksz8463AddStaticFdbEntry,
   ksz8463DeleteStaticFdbEntry,
   ksz8463GetStaticFdbEntry,
   ksz8463FlushStaticFdbTable,
   ksz8463GetDynamicFdbEntry,
   ksz8463FlushDynamicFdbTable,
   ksz8463SetUnknownMcastFwdPorts
};


/**
 * @brief Tail tag rules (host to KSZ8463)
 **/

const uint8_t ksz8463IngressTailTag[3] =
{
   KSZ8463_TAIL_TAG_NORMAL_ADDR_LOOKUP,
   KSZ8463_TAIL_TAG_DEST_PORT1,
   KSZ8463_TAIL_TAG_DEST_PORT2
};


/**
 * @brief KSZ8463 Ethernet switch initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t ksz8463Init(NetInterface *interface)
{
   uint_t port;
   uint16_t temp;

   //Debug message
   TRACE_INFO("Initializing KSZ8463...\r\n");

   //SPI slave mode?
   if(interface->spiDriver != NULL)
   {
      //Initialize SPI interface
      interface->spiDriver->init();

      //Wait for the serial interface to be ready
      do
      {
         //Read CIDER register
         temp = ksz8463ReadSwitchReg(interface, KSZ8463_CIDER);

         //The returned data is invalid until the serial interface is ready
      } while((temp & KSZ8463_CIDER_FAMILY_ID) != KSZ8463_CIDER_FAMILY_ID_DEFAULT);

#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
      //Enable tail tag feature
      temp = ksz8463ReadSwitchReg(interface, KSZ8463_SGCR8);
      temp |= KSZ8463_SGCR8_TAIL_TAG_EN;
      ksz8463WriteSwitchReg(interface, KSZ8463_SGCR8, temp);
#else
      //Disable tail tag feature
      temp = ksz8463ReadSwitchReg(interface, KSZ8463_SGCR8);
      temp &= ~KSZ8463_SGCR8_TAIL_TAG_EN;
      ksz8463WriteSwitchReg(interface, KSZ8463_SGCR8, temp);
#endif

      //Loop through the ports
      for(port = KSZ8463_PORT1; port <= KSZ8463_PORT2; port++)
      {
#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
         //Port separation mode?
         if(interface->port != 0)
         {
            //Disable packet transmission and address learning
            ksz8463SetPortState(interface, port, SWITCH_PORT_STATE_LISTENING);
         }
         else
#endif
         {
            //Enable transmission, reception and address learning
            ksz8463SetPortState(interface, port, SWITCH_PORT_STATE_FORWARDING);
         }
      }

      //Dump switch registers for debugging purpose
      ksz8463DumpSwitchReg(interface);
   }
   else
   {
      //Initialize serial management interface
      if(interface->smiDriver != NULL)
      {
         interface->smiDriver->init();
      }

      //Loop through the ports
      for(port = KSZ8463_PORT1; port <= KSZ8463_PORT2; port++)
      {
         //Debug message
         TRACE_DEBUG("Port %u:\r\n", port);
         //Dump PHY registers for debugging purpose
         ksz8463DumpPhyReg(interface, port);
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
 * @brief KSZ8463 timer handler
 * @param[in] interface Underlying network interface
 **/

void ksz8463Tick(NetInterface *interface)
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
            linkState = ksz8463GetLinkState(interface, virtualInterface->port);

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
      for(port = KSZ8463_PORT1; port <= KSZ8463_PORT2; port++)
      {
         //Retrieve current link state
         if(ksz8463GetLinkState(interface, port))
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

void ksz8463EnableIrq(NetInterface *interface)
{
}


/**
 * @brief Disable interrupts
 * @param[in] interface Underlying network interface
 **/

void ksz8463DisableIrq(NetInterface *interface)
{
}


/**
 * @brief KSZ8463 event handler
 * @param[in] interface Underlying network interface
 **/

void ksz8463EventHandler(NetInterface *interface)
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
            if(port >= KSZ8463_PORT1 && port <= KSZ8463_PORT2)
            {
               //Read port status register
               linkState = ksz8463GetLinkState(interface, port);

               //Link up event?
               if(linkState && !virtualInterface->linkState)
               {
                  //Retrieve host interface speed
                  interface->linkSpeed = ksz8463GetLinkSpeed(interface,
                     KSZ8463_PORT3);

                  //Retrieve host interface duplex mode
                  interface->duplexMode = ksz8463GetDuplexMode(interface,
                     KSZ8463_PORT3);

                  //Adjust MAC configuration parameters for proper operation
                  interface->nicDriver->updateMacConfig(interface);

                  //Check current speed
                  virtualInterface->linkSpeed = ksz8463GetLinkSpeed(interface,
                     port);

                  //Check current duplex mode
                  virtualInterface->duplexMode = ksz8463GetDuplexMode(interface,
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
      for(port = KSZ8463_PORT1; port <= KSZ8463_PORT2; port++)
      {
         //Retrieve current link state
         if(ksz8463GetLinkState(interface, port))
         {
            linkState = TRUE;
         }
      }

      //Link up event?
      if(linkState)
      {
         //Retrieve host interface speed
         interface->linkSpeed = ksz8463GetLinkSpeed(interface, KSZ8463_PORT3);
         //Retrieve host interface duplex mode
         interface->duplexMode = ksz8463GetDuplexMode(interface, KSZ8463_PORT3);

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

error_t ksz8463TagFrame(NetInterface *interface, NetBuffer *buffer,
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
      if(ancillary->port <= KSZ8463_PORT2)
      {
         size_t length;
         const uint8_t *tailTag;

         //The one byte tail tagging is used to indicate the destination port
         tailTag = &ksz8463IngressTailTag[ancillary->port];

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

error_t ksz8463UntagFrame(NetInterface *interface, uint8_t **frame,
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
         ancillary->port = (*tailTag & KSZ8463_TAIL_TAG_SRC_PORT) + 1;

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

bool_t ksz8463GetLinkState(NetInterface *interface, uint8_t port)
{
   uint16_t status;
   bool_t linkState;

   //Check port number
   if(port >= KSZ8463_PORT1 && port <= KSZ8463_PORT2)
   {
      //SPI slave mode?
      if(interface->spiDriver != NULL)
      {
         //Read port status register
         status = ksz8463ReadSwitchReg(interface, KSZ8463_PnSR(port));

         //Retrieve current link state
         linkState = (status & KSZ8463_PnSR_LINK_STATUS) ? TRUE : FALSE;
      }
      else
      {
         //Read status register
         status = ksz8463ReadPhyReg(interface, port, KSZ8463_BMSR);

         //Retrieve current link state
         linkState = (status & KSZ8463_BMSR_LINK_STATUS) ? TRUE : FALSE;
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

uint32_t ksz8463GetLinkSpeed(NetInterface *interface, uint8_t port)
{
   uint16_t status;
   uint32_t linkSpeed;

   //Check port number
   if(port >= KSZ8463_PORT1 && port <= KSZ8463_PORT2)
   {
      //Read port status register
      status = ksz8463ReadSwitchReg(interface, KSZ8463_PnSR(port));

      //Retrieve current link speed
      if((status & KSZ8463_PnSR_OP_SPEED) != 0)
      {
         linkSpeed = NIC_LINK_SPEED_100MBPS;
      }
      else
      {
         linkSpeed = NIC_LINK_SPEED_10MBPS;
      }
   }
   else if(port == KSZ8463_PORT3)
   {
      //Read switch global control 3 register
      status = ksz8463ReadSwitchReg(interface, KSZ8463_SGCR3);

      //Retrieve host interface speed
      if((status & KSZ8463_SGCR3_SW_MII_10BT) != 0)
      {
         linkSpeed = NIC_LINK_SPEED_10MBPS;
      }
      else
      {
         linkSpeed = NIC_LINK_SPEED_100MBPS;
      }
   }
   else
   {
      //The specified port number is not valid
      linkSpeed = NIC_LINK_SPEED_UNKNOWN;
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

NicDuplexMode ksz8463GetDuplexMode(NetInterface *interface, uint8_t port)
{
   uint16_t status;
   NicDuplexMode duplexMode;

   //Check port number
   if(port >= KSZ8463_PORT1 && port <= KSZ8463_PORT2)
   {
      //Read port status register
      status = ksz8463ReadSwitchReg(interface, KSZ8463_PnSR(port));

      //Retrieve current duplex mode
      if((status & KSZ8463_PnSR_OP_DUPLEX) != 0)
      {
         duplexMode = NIC_FULL_DUPLEX_MODE;
      }
      else
      {
         duplexMode = NIC_HALF_DUPLEX_MODE;
      }
   }
   else if(port == KSZ8463_PORT3)
   {
      //Read switch global control 3 register
      status = ksz8463ReadSwitchReg(interface, KSZ8463_SGCR3);

      //Retrieve host interface duplex mode
      if((status & KSZ8463_SGCR3_SW_HOST_PORT_HALF_DUPLEX_MODE) != 0)
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

   //Return duplex mode
   return duplexMode;
}


/**
 * @brief Set port state
 * @param[in] interface Underlying network interface
 * @param[in] port Port number
 * @param[in] state Port state
 **/

void ksz8463SetPortState(NetInterface *interface, uint8_t port,
   SwitchPortState state)
{
   uint16_t temp;

   //Check port number
   if(port >= KSZ8463_PORT1 && port <= KSZ8463_PORT2)
   {
      //Read port control 2 register
      temp = ksz8463ReadSwitchReg(interface, KSZ8463_PnCR2(port));

      //Update port state
      switch(state)
      {
      //Listening state
      case SWITCH_PORT_STATE_LISTENING:
         temp &= ~KSZ8463_PnCR2_TRANSMIT_EN;
         temp |= KSZ8463_PnCR2_RECEIVE_EN;
         temp |= KSZ8463_PnCR2_LEARNING_DIS;
         break;

      //Learning state
      case SWITCH_PORT_STATE_LEARNING:
         temp &= ~KSZ8463_PnCR2_TRANSMIT_EN;
         temp &= ~KSZ8463_PnCR2_RECEIVE_EN;
         temp &= ~KSZ8463_PnCR2_LEARNING_DIS;
         break;

      //Forwarding state
      case SWITCH_PORT_STATE_FORWARDING:
         temp |= KSZ8463_PnCR2_TRANSMIT_EN;
         temp |= KSZ8463_PnCR2_RECEIVE_EN;
         temp &= ~KSZ8463_PnCR2_LEARNING_DIS;
         break;

      //Disabled state
      default:
         temp &= ~KSZ8463_PnCR2_TRANSMIT_EN;
         temp &= ~KSZ8463_PnCR2_RECEIVE_EN;
         temp |= KSZ8463_PnCR2_LEARNING_DIS;
         break;
      }

      //Write the value back to port control 2 register
      ksz8463WriteSwitchReg(interface, KSZ8463_PnCR2(port), temp);
   }
}


/**
 * @brief Get port state
 * @param[in] interface Underlying network interface
 * @param[in] port Port number
 * @return Port state
 **/

SwitchPortState ksz8463GetPortState(NetInterface *interface, uint8_t port)
{
   uint16_t temp;
   SwitchPortState state;

   //Check port number
   if(port >= KSZ8463_PORT1 && port <= KSZ8463_PORT2)
   {
      //Read port control 2 register
      temp = ksz8463ReadSwitchReg(interface, KSZ8463_PnCR2(port));

      //Check port state
      if((temp & KSZ8463_PnCR2_TRANSMIT_EN) == 0 &&
         (temp & KSZ8463_PnCR2_RECEIVE_EN) == 0 &&
         (temp & KSZ8463_PnCR2_LEARNING_DIS) != 0)
      {
         //Disabled state
         state = SWITCH_PORT_STATE_DISABLED;
      }
      else if((temp & KSZ8463_PnCR2_TRANSMIT_EN) == 0 &&
         (temp & KSZ8463_PnCR2_RECEIVE_EN) != 0 &&
         (temp & KSZ8463_PnCR2_LEARNING_DIS) != 0)
      {
         //Listening state
         state = SWITCH_PORT_STATE_LISTENING;
      }
      else if((temp & KSZ8463_PnCR2_TRANSMIT_EN) == 0 &&
         (temp & KSZ8463_PnCR2_RECEIVE_EN) == 0 &&
         (temp & KSZ8463_PnCR2_LEARNING_DIS) == 0)
      {
         //Learning state
         state = SWITCH_PORT_STATE_LEARNING;
      }
      else if((temp & KSZ8463_PnCR2_TRANSMIT_EN) != 0 &&
         (temp & KSZ8463_PnCR2_RECEIVE_EN) != 0 &&
         (temp & KSZ8463_PnCR2_LEARNING_DIS) == 0)
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

void ksz8463SetAgingTime(NetInterface *interface, uint32_t agingTime)
{
   //The aging period is fixed to 300 seconds
}


/**
 * @brief Enable IGMP snooping
 * @param[in] interface Underlying network interface
 * @param[in] enable Enable or disable IGMP snooping
 **/

void ksz8463EnableIgmpSnooping(NetInterface *interface, bool_t enable)
{
   uint16_t temp;

   //Read switch global control 2 register
   temp = ksz8463ReadSwitchReg(interface, KSZ8463_SGCR2);

   //Enable or disable IGMP snooping
   if(enable)
   {
      temp |= KSZ8463_SGCR2_IGMP_SNOOP_EN;
   }
   else
   {
      temp &= ~KSZ8463_SGCR2_IGMP_SNOOP_EN;
   }

   //Write the value back to switch global control 2 register
   ksz8463WriteSwitchReg(interface, KSZ8463_SGCR2, temp);
}


/**
 * @brief Enable MLD snooping
 * @param[in] interface Underlying network interface
 * @param[in] enable Enable or disable MLD snooping
 **/

void ksz8463EnableMldSnooping(NetInterface *interface, bool_t enable)
{
   //Not implemented
}


/**
 * @brief Enable reserved multicast table
 * @param[in] interface Underlying network interface
 * @param[in] enable Enable or disable reserved group addresses
 **/

void ksz8463EnableRsvdMcastTable(NetInterface *interface, bool_t enable)
{
   //Not implemented
}


/**
 * @brief Add a new entry to the static MAC table
 * @param[in] interface Underlying network interface
 * @param[in] entry Pointer to the forwarding database entry
 * @return Error code
 **/

error_t ksz8463AddStaticFdbEntry(NetInterface *interface,
   const SwitchFdbEntry *entry)
{
   error_t error;
   uint_t i;
   uint_t j;
   uint8_t *p;
   SwitchFdbEntry currentEntry;
   Ksz8463StaticMacEntry newEntry;

   //Keep track of the first free entry
   j = KSZ8463_STATIC_MAC_TABLE_SIZE;

   //Loop through the static MAC table
   for(i = 0; i < KSZ8463_STATIC_MAC_TABLE_SIZE; i++)
   {
      //Read current entry
      error = ksz8463GetStaticFdbEntry(interface, i, &currentEntry);

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
         if(j == KSZ8463_STATIC_MAC_TABLE_SIZE)
         {
            j = i;
         }
      }
   }

   //Any entry available?
   if(j < KSZ8463_STATIC_MAC_TABLE_SIZE)
   {
      //Format MAC entry
      newEntry.reserved = 0;
      newEntry.fidH = 0;
      newEntry.fidL = 0;
      newEntry.useFid = 0;
      newEntry.override = entry->override;
      newEntry.valid = TRUE;
      newEntry.macAddr = entry->macAddr;

      //Set the relevant forward ports
      if(entry->destPorts == SWITCH_CPU_PORT_MASK)
      {
         newEntry.forwardPorts = KSZ8463_PORT3_MASK;
      }
      else
      {
         newEntry.forwardPorts = entry->destPorts & KSZ8463_PORT_MASK;
      }

      //Point to the MAC entry
      p = (uint8_t *) &newEntry;

      //Write indirect data registers
      ksz8463WriteSwitchReg(interface, KSZ8463_IADR3, p[0] << 8 | p[1]);
      ksz8463WriteSwitchReg(interface, KSZ8463_IADR2, p[2] << 8 | p[3]);
      ksz8463WriteSwitchReg(interface, KSZ8463_IADR5, p[4] << 8 | p[5]);
      ksz8463WriteSwitchReg(interface, KSZ8463_IADR4, p[6] << 8 | p[7]);

      //Trigger a write static MAC table operation
      ksz8463WriteSwitchReg(interface, KSZ8463_IACR, KSZ8463_IACR_WRITE |
         KSZ8463_IACR_TABLE_SEL_STATIC_MAC | j);

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

error_t ksz8463DeleteStaticFdbEntry(NetInterface *interface,
   const SwitchFdbEntry *entry)
{
   error_t error;
   uint_t j;
   SwitchFdbEntry currentEntry;

   //Loop through the static MAC table
   for(j = 0; j < KSZ8463_STATIC_MAC_TABLE_SIZE; j++)
   {
      //Read current entry
      error = ksz8463GetStaticFdbEntry(interface, j, &currentEntry);

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
   if(j < KSZ8463_STATIC_MAC_TABLE_SIZE)
   {
      //Clear indirect data registers
      ksz8463WriteSwitchReg(interface, KSZ8463_IADR3, 0);
      ksz8463WriteSwitchReg(interface, KSZ8463_IADR2, 0);
      ksz8463WriteSwitchReg(interface, KSZ8463_IADR5, 0);
      ksz8463WriteSwitchReg(interface, KSZ8463_IADR4, 0);

      //Trigger a write static MAC table operation
      ksz8463WriteSwitchReg(interface, KSZ8463_IACR, KSZ8463_IACR_WRITE |
         KSZ8463_IACR_TABLE_SEL_STATIC_MAC | j);

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

error_t ksz8463GetStaticFdbEntry(NetInterface *interface, uint_t index,
   SwitchFdbEntry *entry)
{
   error_t error;
   uint16_t temp;
   uint8_t *p;
   Ksz8463StaticMacEntry currentEntry;

   //Check index parameter
   if(index < KSZ8463_STATIC_MAC_TABLE_SIZE)
   {
      //Trigger a read static MAC table operation
      ksz8463WriteSwitchReg(interface, KSZ8463_IACR, KSZ8463_IACR_READ |
         KSZ8463_IACR_TABLE_SEL_STATIC_MAC | index);

      //Point to the MAC entry
      p = (uint8_t *) &currentEntry;

      //Read indirect data registers
      temp = ksz8463ReadSwitchReg(interface, KSZ8463_IADR3);
      p[0] = (temp >> 8) & 0xFF;
      p[1] = temp & 0xFF;
      temp = ksz8463ReadSwitchReg(interface, KSZ8463_IADR2);
      p[2] = (temp >> 8) & 0xFF;
      p[3] = temp & 0xFF;
      temp = ksz8463ReadSwitchReg(interface, KSZ8463_IADR5);
      p[4] = (temp >> 8) & 0xFF;
      p[5] = temp & 0xFF;
      temp = ksz8463ReadSwitchReg(interface, KSZ8463_IADR4);
      p[6] = (temp >> 8) & 0xFF;
      p[7] = temp & 0xFF;

      //Valid entry?
      if(currentEntry.valid)
      {
         //Copy MAC entry
         entry->macAddr = currentEntry.macAddr;
         entry->srcPort = 0;
         entry->destPorts = currentEntry.forwardPorts & KSZ8463_PORT_MASK;
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

void ksz8463FlushStaticFdbTable(NetInterface *interface)
{
   uint_t i;
   uint_t temp;
   uint16_t state[3];

   //Loop through the ports
   for(i = KSZ8463_PORT1; i <= KSZ8463_PORT3; i++)
   {
      //Save the current state of the port
      state[i - 1] = ksz8463ReadSwitchReg(interface, KSZ8463_PnCR2(i));

      //Turn off learning capability
      ksz8463WriteSwitchReg(interface, KSZ8463_PnCR2(i),
         state[i - 1] | KSZ8463_PnCR2_LEARNING_DIS);
   }

   //All the entries associated with a port that has its learning capability
   //being turned off will be flushed
   temp = ksz8463ReadSwitchReg(interface, KSZ8463_SGCR8);
   temp |= KSZ8463_SGCR8_FLUSH_STATIC_MAC_TABLE;
   ksz8463WriteSwitchReg(interface, KSZ8463_SGCR8, temp);

   //Loop through the ports
   for(i = KSZ8463_PORT1; i <= KSZ8463_PORT3; i++)
   {
      //Restore the original state of the port
      ksz8463WriteSwitchReg(interface, KSZ8463_PnCR2(i), state[i - 1]);
   }
}


/**
 * @brief Read an entry from the dynamic MAC table
 * @param[in] interface Underlying network interface
 * @param[in] index Zero-based index of the entry to read
 * @param[out] entry Pointer to the forwarding database entry
 * @return Error code
 **/

error_t ksz8463GetDynamicFdbEntry(NetInterface *interface, uint_t index,
   SwitchFdbEntry *entry)
{
   error_t error;
   uint16_t temp;
   uint_t n;
   uint8_t *p;
   Ksz8463DynamicMacEntry currentEntry;

   //Check index parameter
   if(index < KSZ8463_DYNAMIC_MAC_TABLE_SIZE)
   {
      //Read the MAC entry at the specified index
      do
      {
         //Trigger a read dynamic MAC table operation
         ksz8463WriteSwitchReg(interface, KSZ8463_IACR, KSZ8463_IACR_READ |
            KSZ8463_IACR_TABLE_SEL_DYNAMIC_MAC | index);

         //Point to the MAC entry
         p = (uint8_t *) &currentEntry;

         //Read indirect data registers
         temp = ksz8463ReadSwitchReg(interface, KSZ8463_IADR1);
         p[0] = temp & 0xFF;
         temp = ksz8463ReadSwitchReg(interface, KSZ8463_IADR3);
         p[1] = (temp >> 8) & 0xFF;
         p[2] = temp & 0xFF;
         temp = ksz8463ReadSwitchReg(interface, KSZ8463_IADR2);
         p[3] = (temp >> 8) & 0xFF;
         p[4] = temp & 0xFF;
         temp = ksz8463ReadSwitchReg(interface, KSZ8463_IADR5);
         p[5] = (temp >> 8) & 0xFF;
         p[6] = temp & 0xFF;
         temp = ksz8463ReadSwitchReg(interface, KSZ8463_IADR4);
         p[7] = (temp >> 8) & 0xFF;
         p[8] = temp & 0xFF;

         //Retry until the entry is ready
      } while(currentEntry.dataNotReady);

      //Check whether there are valid entries in the table
      if(!currentEntry.macEmpty)
      {
         //Retrieve the number of valid entries
         n = ((currentEntry.numValidEntriesH << 8) |
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

void ksz8463FlushDynamicFdbTable(NetInterface *interface, uint8_t port)
{
   uint_t i;
   uint_t temp;
   uint16_t state[3];

   //Loop through the ports
   for(i = KSZ8463_PORT1; i <= KSZ8463_PORT3; i++)
   {
      //Matching port number?
      if(i == port || port == 0)
      {
         //Save the current state of the port
         state[i - 1] = ksz8463ReadSwitchReg(interface, KSZ8463_PnCR2(i));

         //Turn off learning capability
         ksz8463WriteSwitchReg(interface, KSZ8463_PnCR2(i),
            state[i - 1] | KSZ8463_PnCR2_LEARNING_DIS);
      }
   }

   //All the entries associated with a port that has its learning capability
   //being turned off will be flushed
   temp = ksz8463ReadSwitchReg(interface, KSZ8463_SGCR8);
   temp |= KSZ8463_SGCR8_FLUSH_DYNAMIC_MAC_TABLE;
   ksz8463WriteSwitchReg(interface, KSZ8463_SGCR8, temp);

   //Loop through the ports
   for(i = KSZ8463_PORT1; i <= KSZ8463_PORT3; i++)
   {
      //Matching port number?
      if(i == port || port == 0)
      {
         //Restore the original state of the port
         ksz8463WriteSwitchReg(interface, KSZ8463_PnCR2(i), state[i - 1]);
      }
   }
}


/**
 * @brief Set forward ports for unknown multicast packets
 * @param[in] interface Underlying network interface
 * @param[in] enable Enable or disable forwarding of unknown multicast packets
 * @param[in] forwardPorts Port map
 **/

void ksz8463SetUnknownMcastFwdPorts(NetInterface *interface,
   bool_t enable, uint32_t forwardPorts)
{
   //Not implemented
}


/**
 * @brief Write PHY register
 * @param[in] interface Underlying network interface
 * @param[in] port Port number
 * @param[in] address PHY register address
 * @param[in] data Register value
 **/

void ksz8463WritePhyReg(NetInterface *interface, uint8_t port,
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

uint16_t ksz8463ReadPhyReg(NetInterface *interface, uint8_t port,
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

void ksz8463DumpPhyReg(NetInterface *interface, uint8_t port)
{
   uint8_t i;

   //Loop through PHY registers
   for(i = 0; i < 32; i++)
   {
      //Display current PHY register
      TRACE_DEBUG("%02" PRIu8 ": 0x%04" PRIX16 "\r\n", i,
         ksz8463ReadPhyReg(interface, port, i));
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

void ksz8463WriteSwitchReg(NetInterface *interface, uint16_t address,
   uint16_t data)
{
   uint16_t command;

   //SPI slave mode?
   if(interface->spiDriver != NULL)
   {
      //Set up a write operation
      command = KSZ8463_SPI_CMD_WRITE;

      //The byte enable bits are set to indicate which bytes will be
      //transferred in the data phase
      if((address & 0x02) != 0)
      {
         command |= KSZ8463_SPI_CMD_B3 | KSZ8463_SPI_CMD_B2;
      }
      else
      {
         command |= KSZ8463_SPI_CMD_B1 | KSZ8463_SPI_CMD_B0;
      }

      //Set register address
      command |= (address << 4) & KSZ8463_SPI_CMD_ADDR;

      //Pull the CS pin low
      interface->spiDriver->assertCs();

      //Write 16-bit command
      interface->spiDriver->transfer(MSB(command));
      interface->spiDriver->transfer(LSB(command));

      //Write 16-bit data
      interface->spiDriver->transfer(LSB(data));
      interface->spiDriver->transfer(MSB(data));

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

uint16_t ksz8463ReadSwitchReg(NetInterface *interface, uint16_t address)
{
   uint16_t data;
   uint16_t command;

   //SPI slave mode?
   if(interface->spiDriver != NULL)
   {
      //Set up a read operation
      command = KSZ8463_SPI_CMD_READ;

      //The byte enable bits are set to indicate which bytes will be
      //transferred in the data phase
      if((address & 0x02) != 0)
      {
         command |= KSZ8463_SPI_CMD_B3 | KSZ8463_SPI_CMD_B2;
      }
      else
      {
         command |= KSZ8463_SPI_CMD_B1 | KSZ8463_SPI_CMD_B0;
      }

      //Set register address
      command |= (address << 4) & KSZ8463_SPI_CMD_ADDR;

      //Pull the CS pin low
      interface->spiDriver->assertCs();

      //Write 16-bit command
      interface->spiDriver->transfer(MSB(command));
      interface->spiDriver->transfer(LSB(command));

      //Read 16-bit data
      data = interface->spiDriver->transfer(0xFF);
      data |= interface->spiDriver->transfer(0xFF) << 8;

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

void ksz8463DumpSwitchReg(NetInterface *interface)
{
   uint16_t i;

   //Loop through switch registers
   for(i = 0; i < 256; i += 2)
   {
      //Display current switch register
      TRACE_DEBUG("0x%02" PRIX16 " (%02" PRIu16 ") : 0x%04" PRIX16 "\r\n",
         i, i, ksz8463ReadSwitchReg(interface, i));
   }

   //Terminate with a line feed
   TRACE_DEBUG("\r\n");
}
