/**
 * @file lan9303_driver.c
 * @brief LAN9303 3-port Ethernet switch driver
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
#include "drivers/switch/lan9303_driver.h"
#include "debug.h"


/**
 * @brief LAN9303 Ethernet switch driver
 **/

const SwitchDriver lan9303SwitchDriver =
{
   lan9303Init,
   lan9303Tick,
   lan9303EnableIrq,
   lan9303DisableIrq,
   lan9303EventHandler,
   lan9303TagFrame,
   lan9303UntagFrame,
   lan9303GetLinkState,
   lan9303GetLinkSpeed,
   lan9303GetDuplexMode,
   lan9303SetPortState,
   lan9303GetPortState,
   lan9303SetAgingTime,
   lan9303EnableIgmpSnooping,
   lan9303EnableMldSnooping,
   lan9303EnableRsvdMcastTable,
   lan9303AddStaticFdbEntry,
   lan9303DeleteStaticFdbEntry,
   lan9303GetStaticFdbEntry,
   lan9303FlushStaticFdbTable,
   lan9303GetDynamicFdbEntry,
   lan9303FlushDynamicFdbTable,
   lan9303SetUnknownMcastFwdPorts
};


/**
 * @brief LAN9303 Ethernet switch initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t lan9303Init(NetInterface *interface)
{
   uint_t port;
   uint32_t value;

   //Debug message
   TRACE_INFO("Initializing LAN9303...\r\n");

   //Initialize serial management interface
   if(interface->smiDriver != NULL)
   {
      interface->smiDriver->init();
   }

   //Chip-level reset/configuration completion can be determined by first
   //polling the BYTE_TEST register
   do
   {
      //Read BYTE_TEST register
      value = lan9303ReadSysReg(interface, LAN9303_BYTE_TEST);

      //The returned data is invalid until the serial interface reset is
      //complete
   } while(value != LAN9303_BYTE_TEST_DEFAULT);

   //The completion of the entire chip-level reset must then be determined
   //by polling the READY bit of the HW_CFG register
   do
   {
      //Read HW_CFG register
      value = lan9303ReadSysReg(interface, LAN9303_HW_CFG);

      //When set, the READY bit indicates that the reset has completed and
      //the device is ready to be accessed
   } while((value & LAN9303_HW_CFG_DEVICE_READY) == 0);

#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
   //Enable special VLAN tagging mode
   lan9303WriteSwitchReg(interface, LAN9303_SWE_INGRSS_PORT_TYP,
      LAN9303_SWE_INGRSS_PORT_TYP_PORT0);

   //Configure egress VLAN tagging rules
   lan9303WriteSwitchReg(interface, LAN9303_BM_EGRSS_PORT_TYPE,
      LAN9303_BM_EGRSS_PORT_TYPE_PORT0_CPU);
#else
   //Disable special VLAN tagging mode
   lan9303WriteSwitchReg(interface, LAN9303_SWE_INGRSS_PORT_TYP, 0);

   //Revert to default configuration
   lan9303WriteSwitchReg(interface, LAN9303_BM_EGRSS_PORT_TYPE, 0);
#endif

#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
   //Port separation mode?
   if(interface->port != 0)
   {
      //Enable port mirroring
      lan9303WriteSwitchReg(interface, LAN9303_SWE_PORT_MIRROR,
         LAN9303_SWE_PORT_MIRROR_RX_MIRRORING_FILT_EN |
         LAN9303_SWE_PORT_MIRROR_SNIFFER_PORT0 |
         LAN9303_SWE_PORT_MIRROR_MIRRORED_PORT2 |
         LAN9303_SWE_PORT_MIRROR_MIRRORED_PORT1 |
         LAN9303_SWE_PORT_MIRROR_RX_MIRRORING_EN);

      //Configure port state
      lan9303WriteSwitchReg(interface, LAN9303_SWE_PORT_STATE,
         LAN9303_SWE_PORT_STATE_PORT2_LISTENING |
         LAN9303_SWE_PORT_STATE_PORT1_LISTENING |
         LAN9303_SWE_PORT_STATE_PORT0_FORWARDING);
   }
   else
#endif
   {
      //Disable port mirroring
      lan9303WriteSwitchReg(interface, LAN9303_SWE_PORT_MIRROR, 0);

      //Configure port state
      lan9303WriteSwitchReg(interface, LAN9303_SWE_PORT_STATE,
         LAN9303_SWE_PORT_STATE_PORT2_FORWARDING |
         LAN9303_SWE_PORT_STATE_PORT1_FORWARDING |
         LAN9303_SWE_PORT_STATE_PORT0_FORWARDING);
   }

   //Configure port 0 receive parameters
   lan9303WriteSwitchReg(interface, LAN9303_MAC_RX_CFG(0),
      LAN9303_MAC_RX_CFG_REJECT_MAC_TYPES | LAN9303_MAC_RX_CFG_RX_EN);

   //Configure port 0 transmit parameters
   lan9303WriteSwitchReg(interface, LAN9303_MAC_TX_CFG(0),
      LAN9303_MAC_TX_CFG_IFG_CONFIG_DEFAULT | LAN9303_MAC_TX_CFG_TX_PAD_EN |
      LAN9303_MAC_TX_CFG_TX_EN);

   //Loop through the ports
   for(port = LAN9303_PORT1; port <= LAN9303_PORT2; port++)
   {
      //Debug message
      TRACE_DEBUG("Port %u:\r\n", port);
      //Dump PHY registers for debugging purpose
      lan9303DumpPhyReg(interface, port);
   }

   //Force the TCP/IP stack to poll the link state at startup
   interface->phyEvent = TRUE;
   //Notify the TCP/IP stack of the event
   osSetEvent(&netEvent);

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief LAN9303 timer handler
 * @param[in] interface Underlying network interface
 **/

void lan9303Tick(NetInterface *interface)
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
            linkState = lan9303GetLinkState(interface, virtualInterface->port);

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
      for(port = LAN9303_PORT1; port <= LAN9303_PORT2; port++)
      {
         //Retrieve current link state
         if(lan9303GetLinkState(interface, port))
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

void lan9303EnableIrq(NetInterface *interface)
{
}


/**
 * @brief Disable interrupts
 * @param[in] interface Underlying network interface
 **/

void lan9303DisableIrq(NetInterface *interface)
{
}


/**
 * @brief LAN9303 event handler
 * @param[in] interface Underlying network interface
 **/

void lan9303EventHandler(NetInterface *interface)
{
   uint_t port;
   bool_t linkState;

#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
   //Port separation mode?
   if(interface->port != 0)
   {
      uint_t i;
      uint16_t status;
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
            if(port >= LAN9303_PORT1 && port <= LAN9303_PORT2)
            {
               //Retrieve current link state
               linkState = lan9303GetLinkState(interface, port);

               //Link up event?
               if(linkState && !virtualInterface->linkState)
               {
                  //Adjust MAC configuration parameters for proper operation
                  interface->linkSpeed = NIC_LINK_SPEED_100MBPS;
                  interface->duplexMode = NIC_FULL_DUPLEX_MODE;
                  interface->nicDriver->updateMacConfig(interface);

                  //Read PHY special control/status register
                  status = lan9303ReadPhyReg(interface, port, LAN9303_PSCSR);

                  //Check current operation mode
                  switch(status & LAN9303_PSCSR_SPEED)
                  {
                  //10BASE-T half-duplex
                  case LAN9303_PSCSR_SPEED_10BT_HD:
                     virtualInterface->linkSpeed = NIC_LINK_SPEED_10MBPS;
                     virtualInterface->duplexMode = NIC_HALF_DUPLEX_MODE;
                     break;

                  //10BASE-T full-duplex
                  case LAN9303_PSCSR_SPEED_10BT_FD:
                     virtualInterface->linkSpeed = NIC_LINK_SPEED_10MBPS;
                     virtualInterface->duplexMode = NIC_FULL_DUPLEX_MODE;
                     break;

                  //100BASE-TX half-duplex
                  case LAN9303_PSCSR_SPEED_100BTX_HD:
                     virtualInterface->linkSpeed = NIC_LINK_SPEED_100MBPS;
                     virtualInterface->duplexMode = NIC_HALF_DUPLEX_MODE;
                     break;

                  //100BASE-TX full-duplex
                  case LAN9303_PSCSR_SPEED_100BTX_FD:
                     virtualInterface->linkSpeed = NIC_LINK_SPEED_100MBPS;
                     virtualInterface->duplexMode = NIC_FULL_DUPLEX_MODE;
                     break;

                  //Unknown operation mode
                  default:
                     //Debug message
                     TRACE_WARNING("Invalid operation mode!\r\n");
                     break;
                  }

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
      for(port = LAN9303_PORT1; port <= LAN9303_PORT2; port++)
      {
         //Retrieve current link state
         if(lan9303GetLinkState(interface, port))
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
 * @brief Add special VLAN tag to Ethernet frame
 * @param[in] interface Underlying network interface
 * @param[in] buffer Multi-part buffer containing the payload
 * @param[in,out] offset Offset to the first payload byte
 * @param[in] ancillary Additional options passed to the stack along with
 *   the packet
 * @return Error code
 **/

error_t lan9303TagFrame(NetInterface *interface, NetBuffer *buffer,
   size_t *offset, NetTxAncillary *ancillary)
{
   error_t error;

   //Initialize status code
   error = NO_ERROR;

#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
   //Valid port?
   if(ancillary->port <= LAN9303_PORT2)
   {
      //Is there enough space for the VLAN tag?
      if(*offset >= sizeof(VlanTag))
      {
         EthHeader *header;
         VlanTag *vlanTag;

         //Make room for the special VLAN tag
         *offset -= sizeof(VlanTag);
         //Point to the beginning of the frame
         header = netBufferAt(buffer, *offset);

         //Move the Ethernet header to make room for the special VLAN tag
         osMemmove(header, (uint8_t *) header + sizeof(VlanTag),
            sizeof(EthHeader));

         //The special VLAN tag is a normal VLAN tag where the VID field is
         //used as the destination port indicator
         vlanTag = (VlanTag *) header->data;

         //Default port number?
         if(ancillary->port == 0)
         {
            //If VID bit 3 is one, then the normal ALR lookup is performed
            vlanTag->tci = htons(LAN9303_VID_ALR_LOOKUP);
         }
         else
         {
            //VID bits 0 and 1 specify the destination port
            vlanTag->tci = htons(LAN9303_VID_STP_OVERRIDE | ancillary->port);
         }

         //The EtherType field indicates which protocol is encapsulated in
         //the payload
         vlanTag->type = header->type;

         //A distinct Ethertype has been allocated for use in the TPID field
         header->type = HTONS(ETH_TYPE_VLAN);
      }
      else
      {
         //Report an error
         error = ERROR_INVALID_PARAMETER;
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
 * @brief Decode special VLAN tag from incoming Ethernet frame
 * @param[in] interface Underlying network interface
 * @param[in,out] frame Pointer to the received Ethernet frame
 * @param[in,out] length Length of the frame, in bytes
 * @param[in,out] ancillary Additional options passed to the stack along with
 *   the packet
 * @return Error code
 **/

error_t lan9303UntagFrame(NetInterface *interface, uint8_t **frame,
   size_t *length, NetRxAncillary *ancillary)
{
   error_t error;
#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
   EthHeader *header;
   VlanTag *vlanTag;
#endif

   //Initialize status code
   error = NO_ERROR;

#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
   //Point to the beginning of the frame
   header = (EthHeader *) *frame;

   //Check whether VLAN tagging is used
   if(ntohs(header->type) == ETH_TYPE_VLAN)
   {
      //Valid Ethernet frame received?
      if(*length >= (sizeof(EthHeader) + sizeof(VlanTag)))
      {
         //The special VLAN tag is a normal VLAN tag where the VID field is
         //used as a source port indicator
         vlanTag = (VlanTag *) header->data;

         //VID bits 0 and 1 specify the source port
         ancillary->port = ntohs(vlanTag->tci) & LAN9303_VID_SRC_PORT;
         //Fix the value of the EtherType field
         header->type = vlanTag->type;

         //Strip the VLAN tag from the Ethernet frame
         osMemmove(*frame + sizeof(VlanTag), *frame, sizeof(EthHeader));

         //Point to the Ethernet frame header
         *frame += sizeof(VlanTag);
         //Retrieve the length of the original frame
         *length -= sizeof(VlanTag);

         //Successful processing
         error = NO_ERROR;
      }
      else
      {
         //Drop the received frame
         error = ERROR_INVALID_LENGTH;
      }
   }
   else
   {
      //If the interface is configured to accept VLAN-tagged frames, then
      //drop the incoming Ethernet frame
      error = ERROR_WRONG_IDENTIFIER;
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

bool_t lan9303GetLinkState(NetInterface *interface, uint8_t port)
{
   uint16_t status;
   bool_t linkState;

   //Check port number
   if(port >= LAN9303_PORT1 && port <= LAN9303_PORT2)
   {
      //Read status register
      status = lan9303ReadPhyReg(interface, port, LAN9303_BMSR);

      //Retrieve current link state
      linkState = (status & LAN9303_BMSR_LINK_STATUS) ? TRUE : FALSE;
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

uint32_t lan9303GetLinkSpeed(NetInterface *interface, uint8_t port)
{
   uint16_t status;
   uint32_t linkSpeed;

   //Check port number
   if(port >= LAN9303_PORT1 && port <= LAN9303_PORT2)
   {
      //Read PHY special control/status register
      status = lan9303ReadPhyReg(interface, port, LAN9303_PSCSR);

      //Check current operation mode
      switch(status & LAN9303_PSCSR_SPEED)
      {
      //10BASE-T
      case LAN9303_PSCSR_SPEED_10BT_HD:
      case LAN9303_PSCSR_SPEED_10BT_FD:
         linkSpeed = NIC_LINK_SPEED_10MBPS;
         break;

      //100BASE-TX
      case LAN9303_PSCSR_SPEED_100BTX_HD:
      case LAN9303_PSCSR_SPEED_100BTX_FD:
         linkSpeed = NIC_LINK_SPEED_100MBPS;
         break;

      //Unknown operation mode
      default:
         linkSpeed = NIC_LINK_SPEED_UNKNOWN;
         break;
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

NicDuplexMode lan9303GetDuplexMode(NetInterface *interface, uint8_t port)
{
   uint16_t status;
   NicDuplexMode duplexMode;

   //Check port number
   if(port >= LAN9303_PORT1 && port <= LAN9303_PORT2)
   {
      //Read PHY special control/status register
      status = lan9303ReadPhyReg(interface, port, LAN9303_PSCSR);

      //Check current operation mode
      switch(status & LAN9303_PSCSR_SPEED)
      {
      //10BASE-T or 100BASE-TX half-duplex
      case LAN9303_PSCSR_SPEED_10BT_HD:
      case LAN9303_PSCSR_SPEED_100BTX_HD:
         duplexMode = NIC_HALF_DUPLEX_MODE;
         break;

      //10BASE-T or 100BASE-TX full-duplex
      case LAN9303_PSCSR_SPEED_100BTX_FD:
      case LAN9303_PSCSR_SPEED_10BT_FD:
         duplexMode = NIC_FULL_DUPLEX_MODE;
         break;

      //Unknown operation mode
      default:
         duplexMode = NIC_UNKNOWN_DUPLEX_MODE;
         break;
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

void lan9303SetPortState(NetInterface *interface, uint8_t port,
   SwitchPortState state)
{
   uint32_t temp;

   //Read port state register
   temp = lan9303ReadSwitchReg(interface, LAN9303_SWE_PORT_STATE);

   //Check port number
   if(port == LAN9303_PORT1)
   {
      //Clear current configuration
      temp &= ~LAN9303_SWE_PORT_STATE_PORT1;

      //Update port 1 state
      switch(state)
      {
      case SWITCH_PORT_STATE_BLOCKING:
         temp |= LAN9303_SWE_PORT_STATE_PORT1_LISTENING;
         break;
      case SWITCH_PORT_STATE_LEARNING:
         temp |= LAN9303_SWE_PORT_STATE_PORT1_LEARNING;
         break;
      case SWITCH_PORT_STATE_FORWARDING:
         temp |= LAN9303_SWE_PORT_STATE_PORT1_FORWARDING;
         break;
      default:
         temp |= LAN9303_SWE_PORT_STATE_PORT1_DISABLED;
         break;
      }
   }
   else if(port == LAN9303_PORT2)
   {
      //Clear current configuration
      temp &= ~LAN9303_SWE_PORT_STATE_PORT2;

      //Update port 2 state
      switch(state)
      {
      case SWITCH_PORT_STATE_BLOCKING:
         temp |= LAN9303_SWE_PORT_STATE_PORT2_LISTENING;
         break;
      case SWITCH_PORT_STATE_LEARNING:
         temp |= LAN9303_SWE_PORT_STATE_PORT2_LEARNING;
         break;
      case SWITCH_PORT_STATE_FORWARDING:
         temp |= LAN9303_SWE_PORT_STATE_PORT2_FORWARDING;
         break;
      default:
         temp |= LAN9303_SWE_PORT_STATE_PORT2_DISABLED;
         break;
      }
   }
   else
   {
      //The specified port number is not valid
   }

   //Write the value back to port state register
   lan9303WriteSwitchReg(interface, LAN9303_SWE_PORT_STATE, temp);
}


/**
 * @brief Get port state
 * @param[in] interface Underlying network interface
 * @param[in] port Port number
 * @return Port state
 **/

SwitchPortState lan9303GetPortState(NetInterface *interface, uint8_t port)
{
   uint32_t temp;
   SwitchPortState state;

   //Read port state register
   temp = lan9303ReadSwitchReg(interface, LAN9303_SWE_PORT_STATE);

   //Check port number
   if(port == LAN9303_PORT1)
   {
      //Check port 1 state
      switch(temp & LAN9303_SWE_PORT_STATE_PORT1)
      {
      case LAN9303_SWE_PORT_STATE_PORT1_LISTENING:
         state = SWITCH_PORT_STATE_BLOCKING;
         break;
      case LAN9303_SWE_PORT_STATE_PORT1_LEARNING:
         state = SWITCH_PORT_STATE_LEARNING;
         break;
      case LAN9303_SWE_PORT_STATE_PORT1_FORWARDING:
         state = SWITCH_PORT_STATE_FORWARDING;
         break;
      default:
         state = SWITCH_PORT_STATE_DISABLED;
         break;
      }
   }
   else if(port == LAN9303_PORT2)
   {
      //Check port 2 state
      switch(temp & LAN9303_SWE_PORT_STATE_PORT2)
      {
      case LAN9303_SWE_PORT_STATE_PORT2_LISTENING:
         state = SWITCH_PORT_STATE_BLOCKING;
         break;
      case LAN9303_SWE_PORT_STATE_PORT2_LEARNING:
         state = SWITCH_PORT_STATE_LEARNING;
         break;
      case LAN9303_SWE_PORT_STATE_PORT2_FORWARDING:
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

void lan9303SetAgingTime(NetInterface *interface, uint32_t agingTime)
{
   //The aging period is fixed to 5 minutes
}


/**
 * @brief Enable IGMP snooping
 * @param[in] interface Underlying network interface
 * @param[in] enable Enable or disable IGMP snooping
 **/

void lan9303EnableIgmpSnooping(NetInterface *interface, bool_t enable)
{
   uint32_t temp;

   //Read the Switch Engine Global Ingress Configuration register
   temp = lan9303ReadSwitchReg(interface, LAN9303_SWE_GLB_INGRESS_CFG);

   //Enable or disable IGMP monitoring
   if(enable)
   {
      temp |= LAN9303_SWE_GLB_INGRESS_CFG_IGMP_MONITORING_EN;
   }
   else
   {
      temp &= ~LAN9303_SWE_GLB_INGRESS_CFG_IGMP_MONITORING_EN;
   }

   //Set the port bit map where IGMP packets are sent
   temp = (temp & ~LAN9303_SWE_GLB_INGRESS_CFG_IGMP_MONITOR_PORT) |
      LAN9303_SWE_GLB_INGRESS_CFG_IGMP_MONITOR_PORT_0;

   //Write the value back to Switch Engine Global Ingress Configuration register
   lan9303WriteSwitchReg(interface, LAN9303_SWE_GLB_INGRESS_CFG, temp);
}


/**
 * @brief Enable MLD snooping
 * @param[in] interface Underlying network interface
 * @param[in] enable Enable or disable MLD snooping
 **/

void lan9303EnableMldSnooping(NetInterface *interface, bool_t enable)
{
   //Not implemented
}


/**
 * @brief Enable reserved multicast table
 * @param[in] interface Underlying network interface
 * @param[in] enable Enable or disable reserved group addresses
 **/

void lan9303EnableRsvdMcastTable(NetInterface *interface, bool_t enable)
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
         lan9303AddStaticFdbEntry(interface, &entry);
      }
      else
      {
         lan9303DeleteStaticFdbEntry(interface, &entry);
      }
   }
}


/**
 * @brief Add a new entry to the static MAC table
 * @param[in] interface Underlying network interface
 * @param[in] entry Pointer to the forwarding database entry
 * @return Error code
 **/

error_t lan9303AddStaticFdbEntry(NetInterface *interface,
   const SwitchFdbEntry *entry)
{
   uint32_t value;
   uint32_t ports;

   //Check whether the forward port is the CPU port
   if(entry->destPorts == SWITCH_CPU_PORT_MASK)
   {
      ports = LAN9303_PORT0_MASK;
   }
   else
   {
      ports = entry->destPorts & LAN9303_PORT_MASK;
   }

   //Valid forward ports?
   if(ports != 0)
   {
      //Write SWE_ALR_WR_DAT_0 and SWE_ALR_WR_DAT_1 with the desired MAC
      //address and control bits
      value = LAN9303_SWE_ALR_WR_DAT_1_VALID | LAN9303_SWE_ALR_WR_DAT_1_STATIC;

      //When the Override bit set, packets received with a destination
      //address that matches the MAC address in the SWE_ALR_WR_DAT_1 and
      //SWE_ALR_WR_DAT_0 registers will be forwarded regardless of the port
      //state
      if(entry->override)
      {
         value |= LAN9303_SWE_ALR_WR_DAT_1_AGE_OVERRIDE;
      }

      //Set the ports associated with this MAC address
      switch(ports)
      {
      case LAN9303_PORT0_MASK:
         value |= LAN9303_SWE_ALR_WR_DAT_1_PORT_0;
         break;
      case LAN9303_PORT1_MASK:
         value |= LAN9303_SWE_ALR_WR_DAT_1_PORT_1;
         break;
      case LAN9303_PORT2_MASK:
         value |= LAN9303_SWE_ALR_WR_DAT_1_PORT_2;
         break;
      case LAN9303_PORT0_1_MASK:
         value |= LAN9303_SWE_ALR_WR_DAT_1_PORT_0_1;
         break;
      case LAN9303_PORT0_2_MASK:
         value |= LAN9303_SWE_ALR_WR_DAT_1_PORT_0_2;
         break;
      case LAN9303_PORT1_2_MASK:
         value |= LAN9303_SWE_ALR_WR_DAT_1_PORT_1_2;
         break;
      default:
         value |= LAN9303_SWE_ALR_WR_DAT_1_PORT_0_1_2;
         break;
      }
   }
   else
   {
      //An entry can be deleted by setting the Valid bit to 0
      value = LAN9303_SWE_ALR_WR_DAT_1_STATIC;
   }

   //Copy MAC address (last 16 bits)
   value |= entry->macAddr.b[4] | (entry->macAddr.b[5] << 8);
   //Write SWE_ALR_WR_DAT_1 register
   lan9303WriteSwitchReg(interface, LAN9303_SWE_ALR_WR_DAT_1, value);

   //Copy MAC address (first 32 bits)
   value = entry->macAddr.b[0] | (entry->macAddr.b[1] << 8) |
      (entry->macAddr.b[2] << 16) | (entry->macAddr.b[3] << 24);

   //Write SWE_ALR_WR_DAT_0 register
   lan9303WriteSwitchReg(interface, LAN9303_SWE_ALR_WR_DAT_0, value);

   //Write the SWE_ALR_CMD register
   lan9303WriteSwitchReg(interface, LAN9303_SWE_ALR_CMD,
      LAN9303_SWE_ALR_CMD_MAKE_ENTRY);

   //Poll the Make Pending bit in the SWE_ALR_CMD_STS register until it is
   //cleared
   do
   {
      //Read the SWE_ALR_CMD_STS register
      value = lan9303ReadSwitchReg(interface, LAN9303_SWE_ALR_CMD_STS);

      //Check the Make Pending bit
   } while((value & LAN9303_SWE_ALR_CMD_STS_MAKE_PENDING) != 0);

   //Clear the SWE_ALR_CMD register
   lan9303WriteSwitchReg(interface, LAN9303_SWE_ALR_CMD, 0);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Remove an entry from the static MAC table
 * @param[in] interface Underlying network interface
 * @param[in] entry Forwarding database entry to remove from the table
 * @return Error code
 **/

error_t lan9303DeleteStaticFdbEntry(NetInterface *interface,
   const SwitchFdbEntry *entry)
{
   uint32_t value;

   //An entry can be deleted by setting the Valid bit to 0
   value = LAN9303_SWE_ALR_WR_DAT_1_STATIC;

   //Specify the MAC address to remove (last 16 bits)
   value |= entry->macAddr.b[4] | (entry->macAddr.b[5] << 8);
   //Write SWE_ALR_WR_DAT_1 register
   lan9303WriteSwitchReg(interface, LAN9303_SWE_ALR_WR_DAT_1, value);

   //Specify the MAC address to remove (first 32 bits)
   value = entry->macAddr.b[0] | (entry->macAddr.b[1] << 8) |
      (entry->macAddr.b[2] << 16) | (entry->macAddr.b[3] << 24);

   //Write SWE_ALR_WR_DAT_0 register
   lan9303WriteSwitchReg(interface, LAN9303_SWE_ALR_WR_DAT_0, value);

   //Write the SWE_ALR_CMD register
   lan9303WriteSwitchReg(interface, LAN9303_SWE_ALR_CMD,
      LAN9303_SWE_ALR_CMD_MAKE_ENTRY);

   //Poll the Make Pending bit in the SWE_ALR_CMD_STS register until it is
   //cleared
   do
   {
      //Read the SWE_ALR_CMD_STS register
      value = lan9303ReadSwitchReg(interface, LAN9303_SWE_ALR_CMD_STS);

      //Check the Make Pending bit
   } while((value & LAN9303_SWE_ALR_CMD_STS_MAKE_PENDING) != 0);

   //Clear the SWE_ALR_CMD register
   lan9303WriteSwitchReg(interface, LAN9303_SWE_ALR_CMD, 0);

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

error_t lan9303GetStaticFdbEntry(NetInterface *interface, uint_t index,
   SwitchFdbEntry *entry)
{
   uint32_t value;

   //Loop through the ALR table
   while(index < LAN9303_ALR_TABLE_SIZE)
   {
      //First entry?
      if(index == 0)
      {
         value = LAN9303_SWE_ALR_CMD_GET_FIRST_ENTRY;
      }
      else
      {
         value = LAN9303_SWE_ALR_CMD_GET_NEXT_ENTRY;
      }

      //Write the SWE_ALR_CMD register
      lan9303WriteSwitchReg(interface, LAN9303_SWE_ALR_CMD, value);
      //Clear the SWE_ALR_CMD register
      lan9303WriteSwitchReg(interface, LAN9303_SWE_ALR_CMD, 0);

      //Poll the Valid and End of Table bits in the SWE_ALR_RD_DAT_1 register
      //until either is set
      while(1)
      {
         //Read SWE_ALR_RD_DAT_1 register
         value = lan9303ReadSwitchReg(interface, LAN9303_SWE_ALR_RD_DAT_1);

         //If the End of Table bit is set, then exit
         if((value & LAN9303_SWE_ALR_RD_DAT_1_END_OF_TABLE) != 0)
         {
            return ERROR_END_OF_TABLE;
         }

         //If the Valid bit is set, then the entry is valid
         if((value & LAN9303_SWE_ALR_RD_DAT_1_VALID) != 0)
         {
            break;
         }
      }

      //Static entry?
      if((value & LAN9303_SWE_ALR_RD_DAT_1_STATIC) != 0)
      {
         break;
      }

      //Skip dynamic entries
      index++;
   }

   //Store the data from SWE_ALR_RD_DAT_0 and SWE_ALR_RD_DAT_1 registers
   entry->srcPort = 0;
   entry->override = FALSE;

   //Retrieve the ports associated with this MAC address
   switch(value & LAN9303_SWE_ALR_RD_DAT_1_PORT)
   {
   case LAN9303_SWE_ALR_RD_DAT_1_PORT_0:
      entry->destPorts = LAN9303_PORT0_MASK;
      break;
   case LAN9303_SWE_ALR_RD_DAT_1_PORT_1:
      entry->destPorts = LAN9303_PORT1_MASK;
      break;
   case LAN9303_SWE_ALR_RD_DAT_1_PORT_2:
      entry->destPorts = LAN9303_PORT2_MASK;
      break;
   case LAN9303_SWE_ALR_RD_DAT_1_PORT_0_1:
      entry->destPorts = LAN9303_PORT0_1_MASK;
      break;
   case LAN9303_SWE_ALR_RD_DAT_1_PORT_0_2:
      entry->destPorts = LAN9303_PORT0_2_MASK;
      break;
   case LAN9303_SWE_ALR_RD_DAT_1_PORT_1_2:
      entry->destPorts = LAN9303_PORT1_2_MASK;
      break;
   case LAN9303_SWE_ALR_RD_DAT_1_PORT_0_1_2:
      entry->destPorts = LAN9303_PORT0_1_2_MASK;
      break;
   default:
      entry->destPorts = 0;
      break;
   }

   //Copy MAC address (last 16 bits)
   entry->macAddr.b[4] = value & 0xFF;
   entry->macAddr.b[5] = (value >> 8) & 0xFF;

   //Read SWE_ALR_RD_DAT_0 register
   value = lan9303ReadSwitchReg(interface, LAN9303_SWE_ALR_RD_DAT_0);

   //Copy MAC address (first 32 bits)
   entry->macAddr.b[0] = value & 0xFF;
   entry->macAddr.b[1] = (value >> 8) & 0xFF;
   entry->macAddr.b[2] = (value >> 16) & 0xFF;
   entry->macAddr.b[3] = (value >> 24) & 0xFF;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Flush static MAC table
 * @param[in] interface Underlying network interface
 **/

void lan9303FlushStaticFdbTable(NetInterface *interface)
{
   error_t error;
   uint_t i;
   SwitchFdbEntry entry;

   //Loop through the ALR table
   for(i = 0; i < LAN9303_ALR_TABLE_SIZE; i++)
   {
      //Read current entry
      error = lan9303GetStaticFdbEntry(interface, i, &entry);

      //Valid entry?
      if(!error)
      {
         //An entry can be deleted by setting the Valid bit to 0
         lan9303DeleteStaticFdbEntry(interface, &entry);
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

error_t lan9303GetDynamicFdbEntry(NetInterface *interface, uint_t index,
   SwitchFdbEntry *entry)
{
   uint32_t value;

   //Loop through the ALR table
   while(index < LAN9303_ALR_TABLE_SIZE)
   {
      //First entry?
      if(index == 0)
      {
         value = LAN9303_SWE_ALR_CMD_GET_FIRST_ENTRY;
      }
      else
      {
         value = LAN9303_SWE_ALR_CMD_GET_NEXT_ENTRY;
      }

      //Write the SWE_ALR_CMD register
      lan9303WriteSwitchReg(interface, LAN9303_SWE_ALR_CMD, value);
      //Clear the SWE_ALR_CMD register
      lan9303WriteSwitchReg(interface, LAN9303_SWE_ALR_CMD, 0);

      //Poll the Valid and End of Table bits in the SWE_ALR_RD_DAT_1 register
      //until either is set
      while(1)
      {
         //Read SWE_ALR_RD_DAT_1 register
         value = lan9303ReadSwitchReg(interface, LAN9303_SWE_ALR_RD_DAT_1);

         //If the End of Table bit is set, then exit
         if((value & LAN9303_SWE_ALR_RD_DAT_1_END_OF_TABLE) != 0)
         {
            return ERROR_END_OF_TABLE;
         }

         //If the Valid bit is set, then the entry is valid
         if((value & LAN9303_SWE_ALR_RD_DAT_1_VALID) != 0)
         {
            break;
         }
      }

      //Dynamic entry?
      if((value & LAN9303_SWE_ALR_RD_DAT_1_STATIC) == 0)
      {
         break;
      }

      //Skip static entries
      index++;
   }

   //Store the data from SWE_ALR_RD_DAT_0 and SWE_ALR_RD_DAT_1 registers
   entry->destPorts = 0;
   entry->override = FALSE;

   //Retrieve the port associated with this MAC address
   switch(value & LAN9303_SWE_ALR_RD_DAT_1_PORT)
   {
   case LAN9303_SWE_ALR_RD_DAT_1_PORT_0:
      entry->srcPort = LAN9303_PORT0;
      break;
   case LAN9303_SWE_ALR_RD_DAT_1_PORT_1:
      entry->srcPort = LAN9303_PORT1;
      break;
   case LAN9303_SWE_ALR_RD_DAT_1_PORT_2:
      entry->srcPort = LAN9303_PORT2;
      break;
   default:
      entry->srcPort = 0;
      break;
   }

   //Copy MAC address (last 16 bits)
   entry->macAddr.b[4] = value & 0xFF;
   entry->macAddr.b[5] = (value >> 8) & 0xFF;

   //Read SWE_ALR_RD_DAT_0 register
   value = lan9303ReadSwitchReg(interface, LAN9303_SWE_ALR_RD_DAT_0);

   //Copy MAC address (first 32 bits)
   entry->macAddr.b[0] = value & 0xFF;
   entry->macAddr.b[1] = (value >> 8) & 0xFF;
   entry->macAddr.b[2] = (value >> 16) & 0xFF;
   entry->macAddr.b[3] = (value >> 24) & 0xFF;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Flush dynamic MAC table
 * @param[in] interface Underlying network interface
 * @param[in] port Port number
 **/

void lan9303FlushDynamicFdbTable(NetInterface *interface, uint8_t port)
{
   error_t error;
   uint_t i;
   uint32_t value;
   SwitchFdbEntry entry;

   //Loop through the ALR table
   for(i = 0; i < LAN9303_ALR_TABLE_SIZE; i++)
   {
      //Read current entry
      error = lan9303GetDynamicFdbEntry(interface, i, &entry);

      //Valid entry?
      if(!error)
      {
         //Matching port number?
         if(entry.srcPort == port || port == 0)
         {
            //Specify the MAC address to remove (last 16 bits)
            value = entry.macAddr.b[4] | (entry.macAddr.b[5] << 8);
            //Write SWE_ALR_WR_DAT_1 register
            lan9303WriteSwitchReg(interface, LAN9303_SWE_ALR_WR_DAT_1, value);

            //Specify the MAC address to remove (first 32 bits)
            value = entry.macAddr.b[0] | (entry.macAddr.b[1] << 8) |
               (entry.macAddr.b[2] << 16) | (entry.macAddr.b[3] << 24);

            //Write SWE_ALR_WR_DAT_0 register
            lan9303WriteSwitchReg(interface, LAN9303_SWE_ALR_WR_DAT_0, value);

            //Write the SWE_ALR_CMD register
            lan9303WriteSwitchReg(interface, LAN9303_SWE_ALR_CMD,
               LAN9303_SWE_ALR_CMD_MAKE_ENTRY);

            //Poll the Make Pending bit in the SWE_ALR_CMD_STS register until
            //it is cleared
            do
            {
               //Read the SWE_ALR_CMD_STS register
               value = lan9303ReadSwitchReg(interface, LAN9303_SWE_ALR_CMD_STS);

               //Check the Make Pending bit
            } while((value & LAN9303_SWE_ALR_CMD_STS_MAKE_PENDING) != 0);

            //Clear the SWE_ALR_CMD register
            lan9303WriteSwitchReg(interface, LAN9303_SWE_ALR_CMD, 0);
         }
      }
      else
      {
         //The end of the table has been reached
         break;
      }
   }
}


/**
 * @brief Set forward ports for unknown multicast packets
 * @param[in] interface Underlying network interface
 * @param[in] enable Enable or disable forwarding of unknown multicast packets
 * @param[in] forwardPorts Port map
 **/

void lan9303SetUnknownMcastFwdPorts(NetInterface *interface,
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

void lan9303WritePhyReg(NetInterface *interface, uint8_t port,
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

uint16_t lan9303ReadPhyReg(NetInterface *interface, uint8_t port,
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

void lan9303DumpPhyReg(NetInterface *interface, uint8_t port)
{
   uint8_t i;

   //Loop through PHY registers
   for(i = 0; i < 32; i++)
   {
      //Display current PHY register
      TRACE_DEBUG("%02" PRIu8 ": 0x%04" PRIX16 "\r\n", i,
         lan9303ReadPhyReg(interface, port, i));
   }

   //Terminate with a line feed
   TRACE_DEBUG("\r\n");
}


/**
 * @brief Write system CSR register
 * @param[in] interface Underlying network interface
 * @param[in] address System register address
 * @param[in] data Register value
 **/

void lan9303WriteSysReg(NetInterface *interface, uint16_t address,
   uint32_t data)
{
   uint8_t phyAddr;
   uint8_t regAddr;

   //PHY address bit 4 is 1 for SMI commands. PHY address 3:0 form system
   //register address bits 9:6
   phyAddr = 0x10 | ((address >> 6) & 0x0F);

   //Register address field forms register address bits 5:1
   regAddr = (address >> 1) & 0x1F;

   //Write system CSR register
   if(interface->smiDriver != NULL)
   {
      //Write the low word of the SMI register
      interface->smiDriver->writePhyReg(SMI_OPCODE_WRITE, phyAddr, regAddr,
         data & 0xFFFF);

      //Write the high word of the SMI register
      interface->smiDriver->writePhyReg(SMI_OPCODE_WRITE, phyAddr, regAddr + 1,
         (data >> 16) & 0xFFFF);
   }
   else
   {
      //Write the low word of the SMI register
      interface->nicDriver->writePhyReg(SMI_OPCODE_WRITE, phyAddr, regAddr,
         data & 0xFFFF);

      //Write the high word of the SMI register
      interface->nicDriver->writePhyReg(SMI_OPCODE_WRITE, phyAddr, regAddr + 1,
         (data >> 16) & 0xFFFF);
   }
}


/**
 * @brief Read system CSR register
 * @param[in] interface Underlying network interface
 * @param[in] address System register address
 * @return Register value
 **/

uint32_t lan9303ReadSysReg(NetInterface *interface, uint16_t address)
{
   uint8_t phyAddr;
   uint8_t regAddr;
   uint32_t data;

   //PHY address bit 4 is 1 for SMI commands. PHY address 3:0 form system
   //register address bits 9:6
   phyAddr = 0x10 | ((address >> 6) & 0x0F);

   //Register address field forms register address bits 5:1
   regAddr = (address >> 1) & 0x1F;

   //Read system CSR register
   if(interface->smiDriver != NULL)
   {
      //Read the low word of the SMI register
      data = interface->smiDriver->readPhyReg(SMI_OPCODE_READ, phyAddr,
         regAddr);

      //Read the high word of the SMI register
      data |= interface->smiDriver->readPhyReg(SMI_OPCODE_READ, phyAddr,
         regAddr + 1) << 16;
   }
   else
   {
      //Read the low word of the SMI register
      data = interface->nicDriver->readPhyReg(SMI_OPCODE_READ, phyAddr,
         regAddr);

      //Read the high word of the SMI register
      data |= interface->nicDriver->readPhyReg(SMI_OPCODE_READ, phyAddr,
         regAddr + 1) << 16;
   }

   //Return register value
   return data;
}


/**
 * @brief Dump system CSR registers for debugging purpose
 * @param[in] interface Underlying network interface
 **/

void lan9303DumpSysReg(NetInterface *interface)
{
   uint16_t i;

   //Loop through system registers
   for(i = 80; i < 512; i += 4)
   {
      //Display current system register
      TRACE_DEBUG("0x%03" PRIX16 ": 0x%08" PRIX32 "\r\n", i,
         lan9303ReadSysReg(interface, i));
   }

   //Terminate with a line feed
   TRACE_DEBUG("\r\n");
}


/**
 * @brief Write switch fabric CSR register
 * @param[in] interface Underlying network interface
 * @param[in] address Switch fabric register address
 * @param[in] data Register value
 **/

void lan9303WriteSwitchReg(NetInterface *interface, uint16_t address,
   uint32_t data)
{
   uint32_t value;

   //To perform a write to an individual switch fabric register, the desired
   //data must first be written into the SWITCH_CSR_DATA register
   lan9303WriteSysReg(interface, LAN9303_SWITCH_CSR_DATA, data);

   //Set up a write operation
   value = LAN9303_SWITCH_CSR_CMD_BUSY | LAN9303_SWITCH_CSR_CMD_BE;
   //Set register address
   value |= address & LAN9303_SWITCH_CSR_CMD_ADDR;

   //The write cycle is initiated by performing a single write to the
   //SWITCH_CSR_CMD register
   lan9303WriteSysReg(interface, LAN9303_SWITCH_CSR_CMD, value);

   //The completion of the write cycle is indicated by the clearing of the
   //CSR_BUSY bit
   do
   {
      //Read SWITCH_CSR_CMD register
      value = lan9303ReadSysReg(interface, LAN9303_SWITCH_CSR_CMD);

      //Poll CSR_BUSY bit
   } while((value & LAN9303_SWITCH_CSR_CMD_BUSY) != 0);
}


/**
 * @brief Read switch fabric CSR register
 * @param[in] interface Underlying network interface
 * @param[in] address Switch fabric register address
 * @return Register value
 **/

uint32_t lan9303ReadSwitchReg(NetInterface *interface, uint16_t address)
{
   uint32_t value;

   //Set up a read operation
   value = LAN9303_SWITCH_CSR_CMD_BUSY | LAN9303_SWITCH_CSR_CMD_READ |
      LAN9303_SWITCH_CSR_CMD_BE;

   //Set register address
   value |= address & LAN9303_SWITCH_CSR_CMD_ADDR;

   //To perform a read of an individual switch fabric register, the read cycle
   //must be initiated by performing a single write to the SWITCH_CSR_CMD
   //register
   lan9303WriteSysReg(interface, LAN9303_SWITCH_CSR_CMD, value);

   //Valid data is available for reading when the CSR_BUSY bit is cleared
   do
   {
      //Read SWITCH_CSR_CMD register
      value = lan9303ReadSysReg(interface, LAN9303_SWITCH_CSR_CMD);

      //Poll CSR_BUSY bit
   } while((value & LAN9303_SWITCH_CSR_CMD_BUSY) != 0);

   //Read data from the SWITCH_CSR_DATA register
   return lan9303ReadSysReg(interface, LAN9303_SWITCH_CSR_DATA);
}
