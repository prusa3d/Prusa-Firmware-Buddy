/**
 * @file mv88e6060_driver.c
 * @brief 88E6060 6-port Ethernet switch driver
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
#include "drivers/switch/mv88e6060_driver.h"
#include "debug.h"


/**
 * @brief 88E6060 Ethernet switch driver
 **/

const SwitchDriver mv88e6060SwitchDriver =
{
   mv88e6060Init,
   mv88e6060Tick,
   mv88e6060EnableIrq,
   mv88e6060DisableIrq,
   mv88e6060EventHandler,
   mv88e6060TagFrame,
   mv88e6060UntagFrame,
   mv88e6060GetLinkState,
   mv88e6060GetLinkSpeed,
   mv88e6060GetDuplexMode,
   mv88e6060SetPortState,
   mv88e6060GetPortState,
   mv88e6060SetAgingTime,
   mv88e6060EnableIgmpSnooping,
   mv88e6060EnableMldSnooping,
   mv88e6060EnableRsvdMcastTable,
   mv88e6060AddStaticFdbEntry,
   mv88e6060DeleteStaticFdbEntry,
   mv88e6060GetStaticFdbEntry,
   mv88e6060FlushStaticFdbTable,
   mv88e6060GetDynamicFdbEntry,
   mv88e6060FlushDynamicFdbTable,
   mv88e6060SetUnknownMcastFwdPorts
};


/**
 * @brief Ingress trailer (CPU to 88E6060)
 **/

const uint32_t mv88e6060IngressTrailer[6] =
{
   HTONL(0),
   HTONL(MV88E6060_IG_OVERRIDE | MV88E6060_IG_DPV_PORT0 | MV88E6060_IG_MGMT),
   HTONL(MV88E6060_IG_OVERRIDE | MV88E6060_IG_DPV_PORT1 | MV88E6060_IG_MGMT),
   HTONL(MV88E6060_IG_OVERRIDE | MV88E6060_IG_DPV_PORT2 | MV88E6060_IG_MGMT),
   HTONL(MV88E6060_IG_OVERRIDE | MV88E6060_IG_DPV_PORT3 | MV88E6060_IG_MGMT),
   HTONL(MV88E6060_IG_OVERRIDE | MV88E6060_IG_DPV_PORT4 | MV88E6060_IG_MGMT)
};


/**
 * @brief 88E6060 Ethernet switch initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t mv88e6060Init(NetInterface *interface)
{
   uint_t port;
   uint16_t temp;

   //Debug message
   TRACE_INFO("Initializing 88E6060...\r\n");

   //Undefined PHY address?
   if(interface->phyAddr >= 32)
   {
      //Use the default address
      interface->phyAddr = MV88E6060_PHY_ADDR;
   }

   //Initialize serial management interface
   if(interface->smiDriver != NULL)
   {
      interface->smiDriver->init();
   }

   //Wait for the serial interface to be ready
   do
   {
      //Read switch identifier register
      temp = mv88e6060ReadSwitchPortReg(interface, MV88E6060_PORT0,
         MV88E6060_SWITCH_ID);

      //Retrieve device identifier
      temp &= MV88E6060_SWITCH_ID_DEVICE_ID;

      //The returned data is invalid until the serial interface is ready
   } while(temp != MV88E6060_SWITCH_ID_DEVICE_ID_DEFAULT);

#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
   //Enable ingress and egress trailer mode
   mv88e6060WriteSwitchPortReg(interface, MV88E6060_PORT5, MV88E6060_PORT_CTRL,
      MV88E6060_PORT_CTRL_TRAILER | MV88E6060_PORT_CTRL_INGRESS_MODE |
      MV88E6060_PORT_CTRL_PORT_STATE_FORWARDING);
#else
   //Disable ingress and egress trailer mode
   mv88e6060WriteSwitchPortReg(interface, MV88E6060_PORT5, MV88E6060_PORT_CTRL,
      MV88E6060_PORT_CTRL_PORT_STATE_FORWARDING);
#endif

   //Loop through the ports
   for(port = MV88E6060_PORT0; port <= MV88E6060_PORT4; port++)
   {
#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
      //Port separation mode?
      if(interface->port != 0)
      {
         //Use a separate address mapping database for each port
         temp = ((port - 1) << 12) & MV88E6060_PORT_VLAN_MAP_DB_NUM;

         //The bits in the VLAN table are used to restrict which output ports
         //this input port can send frames to
         temp |= MV88E6060_PORT_VLAN_MAP_VLAN_TABLE_PORT5;
      }
      else
#endif
      {
         //Restore the VLAN table to its default value
         temp = ~(1 << (port - 1)) & MV88E6060_PORT_VLAN_MAP_VLAN_TABLE;
      }

      //Set port based VLAN map
      mv88e6060WriteSwitchPortReg(interface, port, MV88E6060_PORT_VLAN_MAP,
         temp);

      //For normal switch operation, the port's bit should be the only bit set
      //in the port association vector
      mv88e6060WriteSwitchPortReg(interface, port, MV88E6060_PORT_ASSOC_VECTOR,
         (1 << (port - 1)) & MV88E6060_PORT_ASSOC_VECTOR_PAV);

      //Enable transmission, reception and address learning
      mv88e6060SetPortState(interface, port, SWITCH_PORT_STATE_FORWARDING);
   }

   //Restore default aging time
   temp = mv88e6060ReadSwitchGlobalReg(interface, MV88E6060_ATU_CTRL);
   temp &= ~MV88E6060_ATU_CTRL_AGE_TIME;
   temp |= MV88E6060_ATU_CTRL_AGE_TIME_DEFAULT;
   mv88e6060WriteSwitchGlobalReg(interface, MV88E6060_ATU_CTRL, temp);

   //Loop through the ports
   for(port = MV88E6060_PORT0; port <= MV88E6060_PORT4; port++)
   {
      //Debug message
      TRACE_DEBUG("Port %u:\r\n", port);
      //Dump PHY registers for debugging purpose
      mv88e6060DumpPhyReg(interface, port);
   }

   //Force the TCP/IP stack to poll the link state at startup
   interface->phyEvent = TRUE;
   //Notify the TCP/IP stack of the event
   osSetEvent(&netEvent);

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief 88E6060 timer handler
 * @param[in] interface Underlying network interface
 **/

void mv88e6060Tick(NetInterface *interface)
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
            linkState = mv88e6060GetLinkState(interface, virtualInterface->port);

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
      for(port = MV88E6060_PORT0; port <= MV88E6060_PORT4; port++)
      {
         //Retrieve current link state
         if(mv88e6060GetLinkState(interface, port))
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

void mv88e6060EnableIrq(NetInterface *interface)
{
}


/**
 * @brief Disable interrupts
 * @param[in] interface Underlying network interface
 **/

void mv88e6060DisableIrq(NetInterface *interface)
{
}


/**
 * @brief 88E6060 event handler
 * @param[in] interface Underlying network interface
 **/

void mv88e6060EventHandler(NetInterface *interface)
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
            if(port >= MV88E6060_PORT0 && port <= MV88E6060_PORT4)
            {
               //Retrieve current link state
               linkState = mv88e6060GetLinkState(interface, port);

               //Link up event?
               if(linkState && !virtualInterface->linkState)
               {
                  //Adjust MAC configuration parameters for proper operation
                  interface->linkSpeed = NIC_LINK_SPEED_100MBPS;
                  interface->duplexMode = NIC_FULL_DUPLEX_MODE;
                  interface->nicDriver->updateMacConfig(interface);

                  //Check current speed
                  virtualInterface->linkSpeed = mv88e6060GetLinkSpeed(interface,
                     port);

                  //Check current duplex mode
                  virtualInterface->duplexMode = mv88e6060GetDuplexMode(interface,
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
      for(port = MV88E6060_PORT0; port <= MV88E6060_PORT4; port++)
      {
         //Retrieve current link state
         if(mv88e6060GetLinkState(interface, port))
         {
            linkState = TRUE;
         }
      }

      //Link up event?
      if(linkState)
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
}


/**
 * @brief Add ingress trailer to Ethernet frame
 * @param[in] interface Underlying network interface
 * @param[in] buffer Multi-part buffer containing the payload
 * @param[in,out] offset Offset to the first payload byte
 * @param[in] ancillary Additional options passed to the stack along with
 *   the packet
 * @return Error code
 **/

error_t mv88e6060TagFrame(NetInterface *interface, NetBuffer *buffer,
   size_t *offset, NetTxAncillary *ancillary)
{
   error_t error;

   //Initialize status code
   error = NO_ERROR;

#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
   //Valid port?
   if(ancillary->port <= MV88E6060_PORT4)
   {
      size_t length;
      const uint32_t *trailer;

      //The ingress trailer is used to indicate the destination port
      trailer = &mv88e6060IngressTrailer[ancillary->port];

      //Retrieve the length of the Ethernet frame
      length = netBufferGetLength(buffer) - *offset;

      //The host controller should manually add padding to the packet before
      //inserting the ingress trailer
      error = ethPadFrame(buffer, &length);

      //Check status code
      if(!error)
      {
         //When the ingress trailer mode is enabled, four extra bytes are added
         //to the end of the frame before the CRC
         error = netBufferAppend(buffer, trailer, sizeof(uint32_t));
      }
   }
   else
   {
      //The port number is not valid
      error = ERROR_INVALID_PORT;
   }
#endif

   //Return status code
   return error;
}


/**
 * @brief Decode egress trailer from incoming Ethernet frame
 * @param[in] interface Underlying network interface
 * @param[in,out] frame Pointer to the received Ethernet frame
 * @param[in,out] length Length of the frame, in bytes
 * @param[in,out] ancillary Additional options passed to the stack along with
 *   the packet
 * @return Error code
 **/

error_t mv88e6060UntagFrame(NetInterface *interface, uint8_t **frame,
   size_t *length, NetRxAncillary *ancillary)
{
   error_t error;

   //Initialize status code
   error = NO_ERROR;

#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
   //Valid Ethernet frame received?
   if(*length >= (sizeof(EthHeader) + sizeof(uint32_t)))
   {
      uint32_t n;
      uint8_t *trailer;

      //When the egress trailer mode is enabled, four extra bytes are added
      //to the end of the frame before the CRC
      trailer = *frame + *length - sizeof(uint32_t);

      //Extract the SPID field from the egress trailer
      n = LOAD32BE(trailer) & MV88E6060_EG_SPID;

      //The SPID field indicates the port where the frame was received
      ancillary->port = (n >> 16) + 1;

      //Remove the trailer from the frame
      *length -= sizeof(uint32_t);
   }
   else
   {
      //Drop the received frame
      error = ERROR_INVALID_LENGTH;
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

bool_t mv88e6060GetLinkState(NetInterface *interface, uint8_t port)
{
   uint16_t status;
   bool_t linkState;

   //Check port number
   if(port >= MV88E6060_PORT0 && port <= MV88E6060_PORT4)
   {
      //Read port status register
      status = mv88e6060ReadSwitchPortReg(interface, port, MV88E6060_PORT_STAT);

      //Retrieve current link state
      linkState = (status & MV88E6060_PORT_STAT_LINK) ? TRUE : FALSE;
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

uint32_t mv88e6060GetLinkSpeed(NetInterface *interface, uint8_t port)
{
   uint16_t status;
   uint32_t linkSpeed;

   //Check port number
   if(port >= MV88E6060_PORT0 && port <= MV88E6060_PORT4)
   {
      //Read port status register
      status = mv88e6060ReadSwitchPortReg(interface, port, MV88E6060_PORT_STAT);

      //Retrieve current link speed
      if((status & MV88E6060_PORT_STAT_SPEED) != 0)
      {
         linkSpeed = NIC_LINK_SPEED_100MBPS;
      }
      else
      {
         linkSpeed = NIC_LINK_SPEED_10MBPS;
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

NicDuplexMode mv88e6060GetDuplexMode(NetInterface *interface, uint8_t port)
{
   uint16_t status;
   NicDuplexMode duplexMode;

   //Check port number
   if(port >= MV88E6060_PORT0 && port <= MV88E6060_PORT4)
   {
      //Read port status register
      status = mv88e6060ReadSwitchPortReg(interface, port, MV88E6060_PORT_STAT);

      //Retrieve current duplex mode
      if((status & MV88E6060_PORT_STAT_DUPLEX) != 0)
      {
         duplexMode = NIC_FULL_DUPLEX_MODE;
      }
      else
      {
         duplexMode = NIC_HALF_DUPLEX_MODE;
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

void mv88e6060SetPortState(NetInterface *interface, uint8_t port,
   SwitchPortState state)
{
   uint16_t temp;

   //Check port number
   if(port >= MV88E6060_PORT0 && port <= MV88E6060_PORT4)
   {
      //Read port control register
      temp = mv88e6060ReadSwitchPortReg(interface, port, MV88E6060_PORT_CTRL);

      //Clear current configuration
      temp &= ~MV88E6060_PORT_CTRL_PORT_STATE;

      //Update port state
      switch(state)
      {
      case SWITCH_PORT_STATE_BLOCKING:
         temp |= MV88E6060_PORT_CTRL_PORT_STATE_BLOCKING;
         break;
      case SWITCH_PORT_STATE_LEARNING:
         temp |= MV88E6060_PORT_CTRL_PORT_STATE_LEARNING;
         break;
      case SWITCH_PORT_STATE_FORWARDING:
         temp |= MV88E6060_PORT_CTRL_PORT_STATE_FORWARDING;
         break;
      default:
         temp |= MV88E6060_PORT_CTRL_PORT_STATE_DISABLED;
         break;
      }

      //Write the value back to port control register
      mv88e6060WriteSwitchPortReg(interface, port, MV88E6060_PORT_CTRL, temp);
   }
}


/**
 * @brief Get port state
 * @param[in] interface Underlying network interface
 * @param[in] port Port number
 * @return Port state
 **/

SwitchPortState mv88e6060GetPortState(NetInterface *interface, uint8_t port)
{
   uint16_t temp;
   SwitchPortState state;

   //Check port number
   if(port >= MV88E6060_PORT0 && port <= MV88E6060_PORT4)
   {
      //Read port control register
      temp = mv88e6060ReadSwitchPortReg(interface, port, MV88E6060_PORT_CTRL);

      //Check port state
      switch(temp & MV88E6060_PORT_CTRL_PORT_STATE)
      {
      case MV88E6060_PORT_CTRL_PORT_STATE_BLOCKING:
         state = SWITCH_PORT_STATE_BLOCKING;
         break;
      case MV88E6060_PORT_CTRL_PORT_STATE_LEARNING:
         state = SWITCH_PORT_STATE_LEARNING;
         break;
      case MV88E6060_PORT_CTRL_PORT_STATE_FORWARDING:
         state = SWITCH_PORT_STATE_FORWARDING;
         break;
      default:
         state = SWITCH_PORT_STATE_DISABLED;
         break;
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

void mv88e6060SetAgingTime(NetInterface *interface, uint32_t agingTime)
{
   uint16_t temp;

   //Read the ATU control register
   temp = mv88e6060ReadSwitchGlobalReg(interface, MV88E6060_ATU_CTRL);

   //The value in this register times 16 is the age time in seconds
   agingTime = (agingTime + 15) / 16;

   //Limit the range of the parameter
   agingTime = MAX(agingTime, 1);
   agingTime = MIN(agingTime, 255);

   //Update the AgeTime field
   temp = (temp & ~MV88E6060_ATU_CTRL_AGE_TIME) | (agingTime << 4);

   //Write the value back to ATU control register
   mv88e6060WriteSwitchGlobalReg(interface, MV88E6060_ATU_CTRL, temp);
}


/**
 * @brief Enable IGMP snooping
 * @param[in] interface Underlying network interface
 * @param[in] enable Enable or disable IGMP snooping
 **/

void mv88e6060EnableIgmpSnooping(NetInterface *interface, bool_t enable)
{
   //Not implemented
}


/**
 * @brief Enable MLD snooping
 * @param[in] interface Underlying network interface
 * @param[in] enable Enable or disable MLD snooping
 **/

void mv88e6060EnableMldSnooping(NetInterface *interface, bool_t enable)
{
   //Not implemented
}


/**
 * @brief Enable reserved multicast table
 * @param[in] interface Underlying network interface
 * @param[in] enable Enable or disable reserved group addresses
 **/

void mv88e6060EnableRsvdMcastTable(NetInterface *interface, bool_t enable)
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
         mv88e6060AddStaticFdbEntry(interface, &entry);
      }
      else
      {
         mv88e6060DeleteStaticFdbEntry(interface, &entry);
      }
   }
}


/**
 * @brief Add a new entry to the static MAC table
 * @param[in] interface Underlying network interface
 * @param[in] entry Pointer to the forwarding database entry
 * @return Error code
 **/

error_t mv88e6060AddStaticFdbEntry(NetInterface *interface,
   const SwitchFdbEntry *entry)
{
   uint16_t value;

   //Make sure the ATU is available by checking the ATUBusy bit
   do
   {
      //Read the ATU operation register
      value = mv88e6060ReadSwitchGlobalReg(interface, MV88E6060_ATU_OPERATION);

      //Poll the ATUBusy bit
   } while((value & MV88E6060_ATU_OPERATION_ATU_BUSY) != 0);

   //Check whether the forward port is the CPU port
   if(entry->destPorts == SWITCH_CPU_PORT_MASK)
   {
      value = MV88E6060_ATU_DATA_DPV_PORT5;
   }
   else
   {
      value = (entry->destPorts << 4) & MV88E6060_ATU_DATA_DPV;
   }

   //Multicast address?
   if(macIsMulticastAddr(&entry->macAddr))
   {
      //The multicast address is locked and does not age
      if(entry->override)
      {
         value |= MV88E6060_ATU_DATA_ENTRY_STATE_LOCKED_MGMT;
      }
      else
      {
         value |= MV88E6060_ATU_DATA_ENTRY_STATE_LOCKED_MULTICAST;
      }
   }
   else
   {
      //The unicast address is locked and does not age
      value |= MV88E6060_ATU_DATA_ENTRY_STATE_LOCKED_UNICAST;
   }

   //Load ATU data register
   mv88e6060WriteSwitchGlobalReg(interface, MV88E6060_ATU_DATA, value);

   //Load ATU MAC address register (bytes 0 and 1)
   mv88e6060WriteSwitchGlobalReg(interface, MV88E6060_ATU_MAC_ADDR_0_1,
      htons(entry->macAddr.w[0]));

   //Load ATU MAC address register (bytes 2 and 3)
   mv88e6060WriteSwitchGlobalReg(interface, MV88E6060_ATU_MAC_ADDR_2_3,
      htons(entry->macAddr.w[1]));

   //Load ATU MAC address register (bytes 4 and 5)
   mv88e6060WriteSwitchGlobalReg(interface, MV88E6060_ATU_MAC_ADDR_4_5,
      htons(entry->macAddr.w[2]));

   //Start the ATU load operation
   mv88e6060WriteSwitchGlobalReg(interface, MV88E6060_ATU_OPERATION,
      MV88E6060_ATU_OPERATION_ATU_BUSY |
      MV88E6060_ATU_OPERATION_ATU_OP_LOAD_PURGE);

   //Completion can be verified by polling the ATUBusy bit
   do
   {
      //Read the ATU operation register
      value = mv88e6060ReadSwitchGlobalReg(interface, MV88E6060_ATU_OPERATION);

      //Poll the ATUBusy bit
   } while((value & MV88E6060_ATU_OPERATION_ATU_BUSY) != 0);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Remove an entry from the static MAC table
 * @param[in] interface Underlying network interface
 * @param[in] entry Forwarding database entry to remove from the table
 * @return Error code
 **/

error_t mv88e6060DeleteStaticFdbEntry(NetInterface *interface,
   const SwitchFdbEntry *entry)
{
   uint16_t value;

   //Make sure the ATU is available by checking the ATUBusy bit
   do
   {
      //Read the ATU operation register
      value = mv88e6060ReadSwitchGlobalReg(interface, MV88E6060_ATU_OPERATION);

      //Poll the ATUBusy bit
   } while((value & MV88E6060_ATU_OPERATION_ATU_BUSY) != 0);

   //Load ATU data register
   mv88e6060WriteSwitchGlobalReg(interface, MV88E6060_ATU_DATA,
      MV88E6060_ATU_DATA_ENTRY_STATE_INVALID);

   //Load ATU MAC address register (bytes 0 and 1)
   mv88e6060WriteSwitchGlobalReg(interface, MV88E6060_ATU_MAC_ADDR_0_1,
      htons(entry->macAddr.w[0]));

   //Load ATU MAC address register (bytes 2 and 3)
   mv88e6060WriteSwitchGlobalReg(interface, MV88E6060_ATU_MAC_ADDR_2_3,
      htons(entry->macAddr.w[1]));

   //Load ATU MAC address register (bytes 4 and 5)
   mv88e6060WriteSwitchGlobalReg(interface, MV88E6060_ATU_MAC_ADDR_4_5,
      htons(entry->macAddr.w[2]));

   //Start the ATU purge operation
   mv88e6060WriteSwitchGlobalReg(interface, MV88E6060_ATU_OPERATION,
      MV88E6060_ATU_OPERATION_ATU_BUSY |
      MV88E6060_ATU_OPERATION_ATU_OP_LOAD_PURGE);

   //Completion can be verified by polling the ATUBusy bit
   do
   {
      //Read the ATU operation register
      value = mv88e6060ReadSwitchGlobalReg(interface, MV88E6060_ATU_OPERATION);

      //Poll the ATUBusy bit
   } while((value & MV88E6060_ATU_OPERATION_ATU_BUSY) != 0);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Read an entry from the static MAC table
 * @param[in] interface Underlying network interface
 * @param[in] index Zero-based index of the entry to read
 * @param[out] entry Pointer to the forwarding database entry
 * @return Error code
 **/

error_t mv88e6060GetStaticFdbEntry(NetInterface *interface, uint_t index,
   SwitchFdbEntry *entry)
{
   uint16_t value;
   uint16_t entryState;

   //Make sure the ATU is available by checking the ATUBusy bit
   do
   {
      //Read the ATU operation register
      value = mv88e6060ReadSwitchGlobalReg(interface, MV88E6060_ATU_OPERATION);

      //Poll the ATUBusy bit
   } while((value & MV88E6060_ATU_OPERATION_ATU_BUSY) != 0);

   //Loop through the ATU table
   while(index < MV88E6060_ATU_TABLE_SIZE)
   {
      //First entry?
      if(index == 0)
      {
         //Use an ATU MAC address of all ones to get the first or lowest active
         //MAC address
         entry->macAddr = MAC_BROADCAST_ADDR;
      }

      //The get next operation starts with the MAC address contained in the ATU
      //MAC registers and returns the next higher active MAC address currently
      //active in the address database
      mv88e6060WriteSwitchGlobalReg(interface, MV88E6060_ATU_MAC_ADDR_0_1,
         htons(entry->macAddr.w[0]));

      //Load ATU MAC address register (bytes 2 and 3)
      mv88e6060WriteSwitchGlobalReg(interface, MV88E6060_ATU_MAC_ADDR_2_3,
         htons(entry->macAddr.w[1]));

      //Load ATU MAC address register (bytes 4 and 5)
      mv88e6060WriteSwitchGlobalReg(interface, MV88E6060_ATU_MAC_ADDR_4_5,
         htons(entry->macAddr.w[2]));

      //Start the ATU get next operation
      mv88e6060WriteSwitchGlobalReg(interface, MV88E6060_ATU_OPERATION,
         MV88E6060_ATU_OPERATION_ATU_BUSY |
         MV88E6060_ATU_OPERATION_ATU_OP_GET_NEXT_DB);

      //Completion can be verified by polling the ATUBusy bit
      do
      {
         //Read the ATU operation register
         value = mv88e6060ReadSwitchGlobalReg(interface, MV88E6060_ATU_OPERATION);

         //Poll the ATUBusy bit
      } while((value & MV88E6060_ATU_OPERATION_ATU_BUSY) != 0);

      //The returned MAC address and its data is accessible in the ATU MAC and
      //the ATU data registers
      value = mv88e6060ReadSwitchGlobalReg(interface, MV88E6060_ATU_DATA);

      //Read ATU MAC address register (bytes 0 and 1)
      entry->macAddr.w[0] = ntohs(mv88e6060ReadSwitchGlobalReg(interface,
         MV88E6060_ATU_MAC_ADDR_0_1));

      //Read ATU MAC address register (bytes 2 and 3)
      entry->macAddr.w[1] = ntohs(mv88e6060ReadSwitchGlobalReg(interface,
         MV88E6060_ATU_MAC_ADDR_2_3));

      //Read ATU MAC address register (bytes 4 and 5)
      entry->macAddr.w[2] = ntohs(mv88e6060ReadSwitchGlobalReg(interface,
         MV88E6060_ATU_MAC_ADDR_4_5));

      //When the returned MAC address is all ones, it always indicates that the
      //end of the table has been reached
      if(macCompAddr(&entry->macAddr, &MAC_BROADCAST_ADDR))
      {
         return ERROR_END_OF_TABLE;
      }

      //Retrieve entry state
      entryState = value & MV88E6060_ATU_DATA_ENTRY_STATE;

      //Multicast address?
      if(macIsMulticastAddr(&entry->macAddr))
      {
         //Static entry?
         if(entryState == MV88E6060_ATU_DATA_ENTRY_STATE_LOCKED_MULTICAST ||
            entryState == MV88E6060_ATU_DATA_ENTRY_STATE_LOCKED_MGMT)
         {
            break;
         }
      }
      else
      {
         //Static entry?
         if(entryState == MV88E6060_ATU_DATA_ENTRY_STATE_LOCKED_UNICAST)
         {
            break;
         }
      }

      //Skip dynamic entries
      index++;
   }

   //Store the data from the ATU MAC and the ATU data registers
   entry->srcPort = 0;
   entry->destPorts = (value & MV88E6060_ATU_DATA_DPV) >> 4;

   //Check entry state
   if(entryState == MV88E6060_ATU_DATA_ENTRY_STATE_LOCKED_MGMT)
   {
      entry->override = TRUE;
   }
   else
   {
      entry->override = FALSE;
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Flush static MAC table
 * @param[in] interface Underlying network interface
 **/

void mv88e6060FlushStaticFdbTable(NetInterface *interface)
{
   error_t error;
   uint_t i;
   SwitchFdbEntry entry;

   //Loop through the ATU table
   for(i = 0; i < MV88E6060_ATU_TABLE_SIZE; i++)
   {
      //Read current entry
      error = mv88e6060GetStaticFdbEntry(interface, i, &entry);

      //Valid entry?
      if(!error)
      {
         //An entry can be deleted by setting the entry state to 0
         mv88e6060DeleteStaticFdbEntry(interface, &entry);
      }
      else
      {
         //The end of the table has been reached
         break;
      }
   }
}


/**
 * @brief Read an entry from the dynamic MAC table
 * @param[in] interface Underlying network interface
 * @param[in] index Zero-based index of the entry to read
 * @param[out] entry Pointer to the forwarding database entry
 * @return Error code
 **/

error_t mv88e6060GetDynamicFdbEntry(NetInterface *interface, uint_t index,
   SwitchFdbEntry *entry)
{
   uint16_t value;
   uint16_t entryState;

   //Make sure the ATU is available by checking the ATUBusy bit
   do
   {
      //Read the ATU operation register
      value = mv88e6060ReadSwitchGlobalReg(interface, MV88E6060_ATU_OPERATION);

      //Poll the ATUBusy bit
   } while((value & MV88E6060_ATU_OPERATION_ATU_BUSY) != 0);

   //Loop through the ATU table
   while(index < MV88E6060_ATU_TABLE_SIZE)
   {
      //First entry?
      if(index == 0)
      {
         //Use an ATU MAC address of all ones to get the first or lowest active
         //MAC address
         entry->macAddr = MAC_BROADCAST_ADDR;
      }

      //The get next operation starts with the MAC address contained in the ATU
      //MAC registers and returns the next higher active MAC address currently
      //active in the address database
      mv88e6060WriteSwitchGlobalReg(interface, MV88E6060_ATU_MAC_ADDR_0_1,
         htons(entry->macAddr.w[0]));

      //Load ATU MAC address register (bytes 2 and 3)
      mv88e6060WriteSwitchGlobalReg(interface, MV88E6060_ATU_MAC_ADDR_2_3,
         htons(entry->macAddr.w[1]));

      //Load ATU MAC address register (bytes 4 and 5)
      mv88e6060WriteSwitchGlobalReg(interface, MV88E6060_ATU_MAC_ADDR_4_5,
         htons(entry->macAddr.w[2]));

      //Start the ATU get next operation
      mv88e6060WriteSwitchGlobalReg(interface, MV88E6060_ATU_OPERATION,
         MV88E6060_ATU_OPERATION_ATU_BUSY |
         MV88E6060_ATU_OPERATION_ATU_OP_GET_NEXT_DB);

      //Completion can be verified by polling the ATUBusy bit
      do
      {
         //Read the ATU operation register
         value = mv88e6060ReadSwitchGlobalReg(interface, MV88E6060_ATU_OPERATION);

         //Poll the ATUBusy bit
      } while((value & MV88E6060_ATU_OPERATION_ATU_BUSY) != 0);

      //The returned MAC address and its data is accessible in the ATU MAC and
      //the ATU data registers
      value = mv88e6060ReadSwitchGlobalReg(interface, MV88E6060_ATU_DATA);

      //Read ATU MAC address register (bytes 0 and 1)
      entry->macAddr.w[0] = ntohs(mv88e6060ReadSwitchGlobalReg(interface,
         MV88E6060_ATU_MAC_ADDR_0_1));

      //Read ATU MAC address register (bytes 2 and 3)
      entry->macAddr.w[1] = ntohs(mv88e6060ReadSwitchGlobalReg(interface,
         MV88E6060_ATU_MAC_ADDR_2_3));

      //Read ATU MAC address register (bytes 4 and 5)
      entry->macAddr.w[2] = ntohs(mv88e6060ReadSwitchGlobalReg(interface,
         MV88E6060_ATU_MAC_ADDR_4_5));

      //When the returned MAC address is all ones, it always indicates that the
      //end of the table has been reached
      if(macCompAddr(&entry->macAddr, &MAC_BROADCAST_ADDR))
      {
         return ERROR_END_OF_TABLE;
      }

      //Retrieve entry state
      entryState = value & MV88E6060_ATU_DATA_ENTRY_STATE;

      //Multicast address?
      if(macIsMulticastAddr(&entry->macAddr))
      {
         //Dynamic entry?
         if(entryState != MV88E6060_ATU_DATA_ENTRY_STATE_INVALID &&
            entryState != MV88E6060_ATU_DATA_ENTRY_STATE_LOCKED_MULTICAST &&
            entryState != MV88E6060_ATU_DATA_ENTRY_STATE_LOCKED_MGMT)
         {
            break;
         }
      }
      else
      {
         //Dynamic entry?
         if(entryState != MV88E6060_ATU_DATA_ENTRY_STATE_INVALID &&
            entryState != MV88E6060_ATU_DATA_ENTRY_STATE_LOCKED_UNICAST)
         {
            break;
         }
      }

      //Skip static entries
      index++;
   }

   //Store the data from the ATU MAC and the ATU data registers
   entry->srcPort = 0;
   entry->override = FALSE;

   //Retrieve the port associated with this MAC address
   switch(value & MV88E6060_ATU_DATA_DPV)
   {
   case MV88E6060_ATU_DATA_DPV_PORT0:
      entry->srcPort = MV88E6060_PORT0;
      break;
   case MV88E6060_ATU_DATA_DPV_PORT1:
      entry->srcPort = MV88E6060_PORT1;
      break;
   case MV88E6060_ATU_DATA_DPV_PORT2:
      entry->srcPort = MV88E6060_PORT2;
      break;
   case MV88E6060_ATU_DATA_DPV_PORT3:
      entry->srcPort = MV88E6060_PORT3;
      break;
   case MV88E6060_ATU_DATA_DPV_PORT4:
      entry->srcPort = MV88E6060_PORT4;
      break;
   case MV88E6060_ATU_DATA_DPV_PORT5:
      entry->srcPort = MV88E6060_PORT5;
      break;
   default:
      entry->srcPort = 0;
      break;
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Flush dynamic MAC table
 * @param[in] interface Underlying network interface
 * @param[in] port Port number
 **/

void mv88e6060FlushDynamicFdbTable(NetInterface *interface, uint8_t port)
{
   error_t error;
   uint_t i;
   uint16_t value;
   SwitchFdbEntry entry;

   //Make sure the ATU is available by checking the ATUBusy bit
   do
   {
      //Read the ATU operation register
      value = mv88e6060ReadSwitchGlobalReg(interface, MV88E6060_ATU_OPERATION);

      //Poll the ATUBusy bit
   } while((value & MV88E6060_ATU_OPERATION_ATU_BUSY) != 0);

   //Valid port number?
   if(port > 0)
   {
      //Loop through the ATU table
      for(i = 0; i < MV88E6060_ATU_TABLE_SIZE; i++)
      {
         //Read current entry
         error = mv88e6060GetDynamicFdbEntry(interface, i, &entry);

         //Valid entry?
         if(!error)
         {
            //Matching port number?
            if(entry.srcPort == port)
            {
               //Load ATU data register
               mv88e6060WriteSwitchGlobalReg(interface, MV88E6060_ATU_DATA,
                  MV88E6060_ATU_DATA_ENTRY_STATE_INVALID);

               //Load ATU MAC address register (bytes 0 and 1)
               mv88e6060WriteSwitchGlobalReg(interface,
                  MV88E6060_ATU_MAC_ADDR_0_1, htons(entry.macAddr.w[0]));

               //Load ATU MAC address register (bytes 2 and 3)
               mv88e6060WriteSwitchGlobalReg(interface,
                  MV88E6060_ATU_MAC_ADDR_2_3, htons(entry.macAddr.w[1]));

               //Load ATU MAC address register (bytes 4 and 5)
               mv88e6060WriteSwitchGlobalReg(interface,
                  MV88E6060_ATU_MAC_ADDR_4_5, htons(entry.macAddr.w[2]));

               //Start the ATU purge operation
               mv88e6060WriteSwitchGlobalReg(interface, MV88E6060_ATU_OPERATION,
                  MV88E6060_ATU_OPERATION_ATU_BUSY |
                  MV88E6060_ATU_OPERATION_ATU_OP_LOAD_PURGE);

               //Completion can be verified by polling the ATUBusy bit
               do
               {
                  //Read the ATU operation register
                  value = mv88e6060ReadSwitchGlobalReg(interface,
                     MV88E6060_ATU_OPERATION);

                  //Poll the ATUBusy bit
               } while((value & MV88E6060_ATU_OPERATION_ATU_BUSY) != 0);
            }
         }
         else
         {
            //The end of the table has been reached
            break;
         }
      }
   }
   else
   {
      //Flush all unlocked entries
      mv88e6060WriteSwitchGlobalReg(interface, MV88E6060_ATU_OPERATION,
         MV88E6060_ATU_OPERATION_ATU_BUSY |
         MV88E6060_ATU_OPERATION_ATU_OP_FLUSH_UNLOCKED);

      //Completion can be verified by polling the ATUBusy bit
      do
      {
         //Read the ATU operation register
         value = mv88e6060ReadSwitchGlobalReg(interface, MV88E6060_ATU_OPERATION);

         //Poll the ATUBusy bit
      } while((value & MV88E6060_ATU_OPERATION_ATU_BUSY) != 0);
   }
}


/**
 * @brief Set forward ports for unknown multicast packets
 * @param[in] interface Underlying network interface
 * @param[in] enable Enable or disable forwarding of unknown multicast packets
 * @param[in] forwardPorts Port map
 **/

void mv88e6060SetUnknownMcastFwdPorts(NetInterface *interface,
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

void mv88e6060WritePhyReg(NetInterface *interface, uint8_t port,
   uint8_t address, uint16_t data)
{
   //PHY registers are accessible using SMI device addresses 0x00 to 0x04 or
   //0x10 to 0x14 depending upon the value of the EE_CLK/ADDR4 pin at reset
   if(interface->smiDriver != NULL)
   {
      interface->smiDriver->writePhyReg(SMI_OPCODE_WRITE,
         interface->phyAddr + port - 1, address, data);
   }
   else
   {
      interface->nicDriver->writePhyReg(SMI_OPCODE_WRITE,
         interface->phyAddr + port - 1, address, data);
   }
}


/**
 * @brief Read PHY register
 * @param[in] interface Underlying network interface
 * @param[in] port Port number
 * @param[in] address PHY register address
 * @return Register value
 **/

uint16_t mv88e6060ReadPhyReg(NetInterface *interface, uint8_t port,
   uint8_t address)
{
   uint16_t data;

   //PHY registers are accessible using SMI device addresses 0x00 to 0x04 or
   //0x10 to 0x14 depending upon the value of the EE_CLK/ADDR4 pin at reset
   if(interface->smiDriver != NULL)
   {
      data = interface->smiDriver->readPhyReg(SMI_OPCODE_READ,
         interface->phyAddr + port - 1, address);
   }
   else
   {
      data = interface->nicDriver->readPhyReg(SMI_OPCODE_READ,
         interface->phyAddr + port - 1, address);
   }

   //Return the value of the PHY register
   return data;
}


/**
 * @brief Dump PHY registers for debugging purpose
 * @param[in] interface Underlying network interface
 * @param[in] port Port number
 **/

void mv88e6060DumpPhyReg(NetInterface *interface, uint8_t port)
{
   uint8_t i;

   //Loop through PHY registers
   for(i = 0; i < 32; i++)
   {
      //Display current PHY register
      TRACE_DEBUG("%02" PRIu8 ": 0x%04" PRIX16 "\r\n", i,
         mv88e6060ReadPhyReg(interface, port, i));
   }

   //Terminate with a line feed
   TRACE_DEBUG("\r\n");
}


/**
 * @brief Write switch port register
 * @param[in] interface Underlying network interface
 * @param[in] port Port number
 * @param[in] address Switch port register address
 * @param[in] data Register value
 **/

void mv88e6060WriteSwitchPortReg(NetInterface *interface, uint8_t port,
   uint8_t address, uint16_t data)
{
   //Switch port registers are accessible using SMI device addresses 0x08
   //to 0x0D, or 0x18 to 0x1D
   if(interface->smiDriver != NULL)
   {
      interface->smiDriver->writePhyReg(SMI_OPCODE_WRITE,
         interface->phyAddr + port + 7, address, data);
   }
   else
   {
      interface->nicDriver->writePhyReg(SMI_OPCODE_WRITE,
         interface->phyAddr + port + 7, address, data);
   }
}


/**
 * @brief Read switch port register
 * @param[in] interface Underlying network interface
 * @param[in] port Port number
 * @param[in] address Switch port register address
 * @return Register value
 **/

uint16_t mv88e6060ReadSwitchPortReg(NetInterface *interface, uint8_t port,
   uint8_t address)
{
   uint16_t data;

   //Switch port registers are accessible using SMI device addresses 0x08
   //to 0x0D, or 0x18 to 0x1D
   if(interface->smiDriver != NULL)
   {
      data = interface->smiDriver->readPhyReg(SMI_OPCODE_READ,
         interface->phyAddr + port + 7, address);
   }
   else
   {
      data = interface->nicDriver->readPhyReg(SMI_OPCODE_READ,
         interface->phyAddr + port + 7, address);
   }

   //Return the value of the switch port register
   return data;
}


/**
 * @brief Write switch global register
 * @param[in] interface Underlying network interface
 * @param[in] address Switch global register address
 * @param[in] data Register value
 **/

void mv88e6060WriteSwitchGlobalReg(NetInterface *interface, uint8_t address,
   uint16_t data)
{
   //Switch global registers are accessible using SMI device address 0x0F
   //or 0x1F
   if(interface->smiDriver != NULL)
   {
      interface->smiDriver->writePhyReg(SMI_OPCODE_WRITE,
         interface->phyAddr + 15, address, data);
   }
   else
   {
      interface->nicDriver->writePhyReg(SMI_OPCODE_WRITE,
         interface->phyAddr + 15, address, data);
   }
}


/**
 * @brief Read switch global register
 * @param[in] interface Underlying network interface
 * @param[in] address Switch global register address
 * @return Register value
 **/

uint16_t mv88e6060ReadSwitchGlobalReg(NetInterface *interface, uint8_t address)
{
   uint16_t data;

   //Switch global registers are accessible using SMI device address 0x0F
   //or 0x1F
   if(interface->smiDriver != NULL)
   {
      data = interface->smiDriver->readPhyReg(SMI_OPCODE_READ,
         interface->phyAddr + 15, address);
   }
   else
   {
      data = interface->nicDriver->readPhyReg(SMI_OPCODE_READ,
         interface->phyAddr + 15, address);
   }

   //Return the value of the switch global register
   return data;
}
