/**
 * @file ksz8563_driver.c
 * @brief KSZ8563 3-port Ethernet switch driver
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
#include "drivers/switch/ksz8563_driver.h"
#include "debug.h"


/**
 * @brief KSZ8563 Ethernet switch driver
 **/

const SwitchDriver ksz8563SwitchDriver =
{
   ksz8563Init,
   ksz8563Tick,
   ksz8563EnableIrq,
   ksz8563DisableIrq,
   ksz8563EventHandler,
   ksz8563TagFrame,
   ksz8563UntagFrame,
   ksz8563GetLinkState,
   ksz8563GetLinkSpeed,
   ksz8563GetDuplexMode,
   ksz8563SetPortState,
   ksz8563GetPortState,
   ksz8563SetAgingTime,
   ksz8563EnableIgmpSnooping,
   ksz8563EnableMldSnooping,
   ksz8563EnableRsvdMcastTable,
   ksz8563AddStaticFdbEntry,
   ksz8563DeleteStaticFdbEntry,
   ksz8563GetStaticFdbEntry,
   ksz8563FlushStaticFdbTable,
   ksz8563GetDynamicFdbEntry,
   ksz8563FlushDynamicFdbTable,
   ksz8563SetUnknownMcastFwdPorts
};


/**
 * @brief Tail tag rules (host to KSZ8563)
 **/

const uint8_t ksz8563IngressTailTag[3] =
{
   KSZ8563_TAIL_TAG_NORMAL_ADDR_LOOKUP,
   KSZ8563_TAIL_TAG_PORT_BLOCKING_OVERRIDE | KSZ8563_TAIL_TAG_DEST_PORT1,
   KSZ8563_TAIL_TAG_PORT_BLOCKING_OVERRIDE | KSZ8563_TAIL_TAG_DEST_PORT2
};


/**
 * @brief KSZ8563 Ethernet switch initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t ksz8563Init(NetInterface *interface)
{
   uint_t port;
   uint8_t temp;

   //Debug message
   TRACE_INFO("Initializing KSZ8563...\r\n");

   //SPI slave mode?
   if(interface->spiDriver != NULL)
   {
      //Initialize SPI interface
      interface->spiDriver->init();

      //Wait for the serial interface to be ready
      do
      {
         //Read CHIP_ID1 register
         temp = ksz8563ReadSwitchReg8(interface, KSZ8563_CHIP_ID1);

         //The returned data is invalid until the serial interface is ready
      } while(temp != KSZ8563_CHIP_ID1_DEFAULT);

      //Reset switch
      ksz8563WriteSwitchReg8(interface, KSZ8563_SWITCH_OP,
         KSZ8563_SWITCH_OP_SOFT_HARD_RESET);

      //Wait for the reset to complete
      do
      {
         //Read switch operation register
         temp = ksz8563ReadSwitchReg8(interface, KSZ8563_SWITCH_OP);

         //The reset bit is self-clearing
      } while((temp & KSZ8563_SWITCH_OP_SOFT_HARD_RESET) != 0);

#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
      //Enable tail tag feature
      temp = ksz8563ReadSwitchReg8(interface, KSZ8563_PORT3_OP_CTRL0);
      temp |= KSZ8563_PORTn_OP_CTRL0_TAIL_TAG_EN;
      ksz8563WriteSwitchReg8(interface, KSZ8563_PORT3_OP_CTRL0, temp);

      //Disable frame length check (silicon errata workaround)
      temp = ksz8563ReadSwitchReg8(interface, KSZ8563_SWITCH_MAC_CTRL0);
      temp &= ~KSZ8563_SWITCH_MAC_CTRL0_FRAME_LEN_CHECK_EN;
      ksz8563WriteSwitchReg8(interface, KSZ8563_SWITCH_MAC_CTRL0, temp);
#else
      //Disable tail tag feature
      temp = ksz8563ReadSwitchReg8(interface, KSZ8563_PORT3_OP_CTRL0);
      temp &= ~KSZ8563_PORTn_OP_CTRL0_TAIL_TAG_EN;
      ksz8563WriteSwitchReg8(interface, KSZ8563_PORT3_OP_CTRL0, temp);

      //Enable frame length check
      temp = ksz8563ReadSwitchReg8(interface, KSZ8563_SWITCH_MAC_CTRL0);
      temp |= KSZ8563_SWITCH_MAC_CTRL0_FRAME_LEN_CHECK_EN;
      ksz8563WriteSwitchReg8(interface, KSZ8563_SWITCH_MAC_CTRL0, temp);
#endif

      //Loop through the ports
      for(port = KSZ8563_PORT1; port <= KSZ8563_PORT2; port++)
      {
#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
         //Port separation mode?
         if(interface->port != 0)
         {
            //Disable packet transmission and address learning
            ksz8563SetPortState(interface, port, SWITCH_PORT_STATE_LISTENING);
         }
         else
#endif
         {
            //Enable transmission, reception and address learning
            ksz8563SetPortState(interface, port, SWITCH_PORT_STATE_FORWARDING);
         }
      }

      //Restore default age count
      ksz8563WriteSwitchReg8(interface, KSZ8563_SWITCH_LUE_CTRL0,
         KSZ8563_SWITCH_LUE_CTRL0_AGE_COUNT_DEFAULT |
         KSZ8563_SWITCH_LUE_CTRL0_HASH_OPTION_CRC);

      //Restore default age period
      ksz8563WriteSwitchReg8(interface, KSZ8563_SWITCH_LUE_CTRL3,
         KSZ8563_SWITCH_LUE_CTRL3_AGE_PERIOD_DEFAULT);

      //Add internal delay to ingress and egress RGMII clocks
      temp = ksz8563ReadSwitchReg8(interface, KSZ8563_PORT3_XMII_CTRL1);
      temp |= KSZ8563_PORTn_XMII_CTRL1_RGMII_ID_IG;
      temp |= KSZ8563_PORTn_XMII_CTRL1_RGMII_ID_EG;
      ksz8563WriteSwitchReg8(interface, KSZ8563_PORT3_XMII_CTRL1, temp);

      //Start switch operation
      ksz8563WriteSwitchReg8(interface, KSZ8563_SWITCH_OP,
         KSZ8563_SWITCH_OP_START_SWITCH);
   }
   else if(interface->smiDriver != NULL)
   {
      //Initialize serial management interface
      interface->smiDriver->init();
   }
   else
   {
      //Just for sanity
   }

   //Loop through the ports
   for(port = KSZ8563_PORT1; port <= KSZ8563_PORT2; port++)
   {
      //Select tri-color dual-LED mode (silicon errata workaround)
      ksz8563WriteMmdReg(interface, port, KSZ8563_MMD_LED_MODE,
         KSZ8563_MMD_LED_MODE_LED_MODE_TRI_COLOR_DUAL |
         KSZ8563_MMD_LED_MODE_RESERVED_DEFAULT);

      //Debug message
      TRACE_DEBUG("Port %u:\r\n", port);
      //Dump PHY registers for debugging purpose
      ksz8563DumpPhyReg(interface, port);
   }

   //Force the TCP/IP stack to poll the link state at startup
   interface->phyEvent = TRUE;
   //Notify the TCP/IP stack of the event
   osSetEvent(&netEvent);

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief KSZ8563 timer handler
 * @param[in] interface Underlying network interface
 **/

void ksz8563Tick(NetInterface *interface)
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
            linkState = ksz8563GetLinkState(interface, virtualInterface->port);

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
      for(port = KSZ8563_PORT1; port <= KSZ8563_PORT2; port++)
      {
         //Retrieve current link state
         if(ksz8563GetLinkState(interface, port))
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

void ksz8563EnableIrq(NetInterface *interface)
{
}


/**
 * @brief Disable interrupts
 * @param[in] interface Underlying network interface
 **/

void ksz8563DisableIrq(NetInterface *interface)
{
}


/**
 * @brief KSZ8563 event handler
 * @param[in] interface Underlying network interface
 **/

void ksz8563EventHandler(NetInterface *interface)
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
            if(port >= KSZ8563_PORT1 && port <= KSZ8563_PORT2)
            {
               //Retrieve current link state
               linkState = ksz8563GetLinkState(interface, port);

               //Link up event?
               if(linkState && !virtualInterface->linkState)
               {
                  //Retrieve host interface speed
                  interface->linkSpeed = ksz8563GetLinkSpeed(interface,
                     KSZ8563_PORT3);

                  //Retrieve host interface duplex mode
                  interface->duplexMode = ksz8563GetDuplexMode(interface,
                     KSZ8563_PORT3);

                  //Adjust MAC configuration parameters for proper operation
                  interface->nicDriver->updateMacConfig(interface);

                  //Check current speed
                  virtualInterface->linkSpeed = ksz8563GetLinkSpeed(interface,
                     port);

                  //Check current duplex mode
                  virtualInterface->duplexMode = ksz8563GetDuplexMode(interface,
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
      for(port = KSZ8563_PORT1; port <= KSZ8563_PORT2; port++)
      {
         //Retrieve current link state
         if(ksz8563GetLinkState(interface, port))
         {
            linkState = TRUE;
         }
      }

      //Link up event?
      if(linkState)
      {
         //Retrieve host interface speed
         interface->linkSpeed = ksz8563GetLinkSpeed(interface, KSZ8563_PORT3);
         //Retrieve host interface duplex mode
         interface->duplexMode = ksz8563GetDuplexMode(interface, KSZ8563_PORT3);

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

error_t ksz8563TagFrame(NetInterface *interface, NetBuffer *buffer,
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
      if(ancillary->port <= KSZ8563_PORT2)
      {
         size_t length;
         const uint8_t *tailTag;

         //The one byte tail tagging is used to indicate the destination port
         tailTag = &ksz8563IngressTailTag[ancillary->port];

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

error_t ksz8563UntagFrame(NetInterface *interface, uint8_t **frame,
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
         ancillary->port = (*tailTag & KSZ8563_TAIL_TAG_SRC_PORT) + 1;

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

bool_t ksz8563GetLinkState(NetInterface *interface, uint8_t port)
{
   uint16_t value;
   bool_t linkState;

   //Check port number
   if(port >= KSZ8563_PORT1 && port <= KSZ8563_PORT2)
   {
      //Any link failure condition is latched in the BMSR register. Reading
      //the register twice will always return the actual link status
      value = ksz8563ReadPhyReg(interface, port, KSZ8563_BMSR);
      value = ksz8563ReadPhyReg(interface, port, KSZ8563_BMSR);

      //Retrieve current link state
      linkState = (value & KSZ8563_BMSR_LINK_STATUS) ? TRUE : FALSE;
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

uint32_t ksz8563GetLinkSpeed(NetInterface *interface, uint8_t port)
{
   uint8_t type;
   uint16_t value;
   uint32_t linkSpeed;

   //Check port number
   if(port >= KSZ8563_PORT1 && port <= KSZ8563_PORT2)
   {
      //Read PHY control register
      value = ksz8563ReadPhyReg(interface, port, KSZ8563_PHYCON);

      //Retrieve current link speed
      if((value & KSZ8563_PHYCON_SPEED_100BTX) != 0)
      {
         //100BASE-TX
         linkSpeed = NIC_LINK_SPEED_100MBPS;
      }
      else if((value & KSZ8563_PHYCON_SPEED_10BT) != 0)
      {
         //10BASE-T
         linkSpeed = NIC_LINK_SPEED_10MBPS;
      }
      else
      {
         //The link speed is not valid
         linkSpeed = NIC_LINK_SPEED_UNKNOWN;
      }
   }
   else if(port == KSZ8563_PORT3)
   {
      //SPI slave mode?
      if(interface->spiDriver != NULL)
      {
         //Read port 3 XMII control 1 register
         value = ksz8563ReadSwitchReg8(interface, KSZ8563_PORT3_XMII_CTRL1);

         //Retrieve host interface type
         type = value & KSZ8563_PORTn_XMII_CTRL1_IF_TYPE;

         //Gigabit interface?
         if(type == KSZ8563_PORTn_XMII_CTRL1_IF_TYPE_RGMII &&
            (value & KSZ8563_PORTn_XMII_CTRL1_SPEED_1000) != 0)
         {
            //1000 Mb/s mode
            linkSpeed = NIC_LINK_SPEED_1GBPS;
         }
         else
         {
            //Read port 3 XMII control 0 register
            value = ksz8563ReadSwitchReg8(interface, KSZ8563_PORT3_XMII_CTRL0);

            //Retrieve host interface speed
            if((value & KSZ8563_PORTn_XMII_CTRL0_SPEED_10_100) != 0)
            {
               //100 Mb/s mode
               linkSpeed = NIC_LINK_SPEED_100MBPS;
            }
            else
            {
               //10 Mb/s mode
               linkSpeed = NIC_LINK_SPEED_10MBPS;
            }
         }
      }
      else
      {
         //The MDC/MDIO interface does not have access to all the configuration
         //registers. It can only access the standard MIIM registers
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

NicDuplexMode ksz8563GetDuplexMode(NetInterface *interface, uint8_t port)
{
   uint16_t value;
   NicDuplexMode duplexMode;

   //Check port number
   if(port >= KSZ8563_PORT1 && port <= KSZ8563_PORT2)
   {
      //Read PHY control register
      value = ksz8563ReadPhyReg(interface, port, KSZ8563_PHYCON);

      //Retrieve current duplex mode
      if((value & KSZ8563_PHYCON_DUPLEX_STATUS) != 0)
      {
         duplexMode = NIC_FULL_DUPLEX_MODE;
      }
      else
      {
         duplexMode = NIC_HALF_DUPLEX_MODE;
      }
   }
   else if(port == KSZ8563_PORT3)
   {
      //SPI slave mode?
      if(interface->spiDriver != NULL)
      {
         //Read port 3 XMII control 0 register
         value = ksz8563ReadSwitchReg8(interface, KSZ8563_PORT3_XMII_CTRL0);

         //Retrieve host interface duplex mode
         if((value & KSZ8563_PORTn_XMII_CTRL0_DUPLEX) != 0)
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
         //The MDC/MDIO interface does not have access to all the configuration
         //registers. It can only access the standard MIIM registers
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

void ksz8563SetPortState(NetInterface *interface, uint8_t port,
   SwitchPortState state)
{
   uint8_t temp;

   //Check port number
   if(port >= KSZ8563_PORT1 && port <= KSZ8563_PORT2)
   {
      //Read MSTP state register
      temp = ksz8563ReadSwitchReg8(interface, KSZ8563_PORTn_MSTP_STATE(port));

      //Update port state
      switch(state)
      {
      //Listening state
      case SWITCH_PORT_STATE_LISTENING:
         temp &= ~KSZ8563_PORTn_MSTP_STATE_TRANSMIT_EN;
         temp |= KSZ8563_PORTn_MSTP_STATE_RECEIVE_EN;
         temp |= KSZ8563_PORTn_MSTP_STATE_LEARNING_DIS;
         break;

      //Learning state
      case SWITCH_PORT_STATE_LEARNING:
         temp &= ~KSZ8563_PORTn_MSTP_STATE_TRANSMIT_EN;
         temp &= ~KSZ8563_PORTn_MSTP_STATE_RECEIVE_EN;
         temp &= ~KSZ8563_PORTn_MSTP_STATE_LEARNING_DIS;
         break;

      //Forwarding state
      case SWITCH_PORT_STATE_FORWARDING:
         temp |= KSZ8563_PORTn_MSTP_STATE_TRANSMIT_EN;
         temp |= KSZ8563_PORTn_MSTP_STATE_RECEIVE_EN;
         temp &= ~KSZ8563_PORTn_MSTP_STATE_LEARNING_DIS;
         break;

      //Disabled state
      default:
         temp &= ~KSZ8563_PORTn_MSTP_STATE_TRANSMIT_EN;
         temp &= ~KSZ8563_PORTn_MSTP_STATE_RECEIVE_EN;
         temp |= KSZ8563_PORTn_MSTP_STATE_LEARNING_DIS;
         break;
      }

      //Write the value back to MSTP state register
      ksz8563WriteSwitchReg8(interface, KSZ8563_PORTn_MSTP_STATE(port), temp);
   }
}


/**
 * @brief Get port state
 * @param[in] interface Underlying network interface
 * @param[in] port Port number
 * @return Port state
 **/

SwitchPortState ksz8563GetPortState(NetInterface *interface, uint8_t port)
{
   uint8_t temp;
   SwitchPortState state;

   //Check port number
   if(port >= KSZ8563_PORT1 && port <= KSZ8563_PORT2)
   {
      //Read MSTP state register
      temp = ksz8563ReadSwitchReg8(interface, KSZ8563_PORTn_MSTP_STATE(port));

      //Check port state
      if((temp & KSZ8563_PORTn_MSTP_STATE_TRANSMIT_EN) == 0 &&
         (temp & KSZ8563_PORTn_MSTP_STATE_RECEIVE_EN) == 0 &&
         (temp & KSZ8563_PORTn_MSTP_STATE_LEARNING_DIS) != 0)
      {
         //Disabled state
         state = SWITCH_PORT_STATE_DISABLED;
      }
      else if((temp & KSZ8563_PORTn_MSTP_STATE_TRANSMIT_EN) == 0 &&
         (temp & KSZ8563_PORTn_MSTP_STATE_RECEIVE_EN) != 0 &&
         (temp & KSZ8563_PORTn_MSTP_STATE_LEARNING_DIS) != 0)
      {
         //Listening state
         state = SWITCH_PORT_STATE_LISTENING;
      }
      else if((temp & KSZ8563_PORTn_MSTP_STATE_TRANSMIT_EN) == 0 &&
         (temp & KSZ8563_PORTn_MSTP_STATE_RECEIVE_EN) == 0 &&
         (temp & KSZ8563_PORTn_MSTP_STATE_LEARNING_DIS) == 0)
      {
         //Learning state
         state = SWITCH_PORT_STATE_LEARNING;
      }
      else if((temp & KSZ8563_PORTn_MSTP_STATE_TRANSMIT_EN) != 0 &&
         (temp & KSZ8563_PORTn_MSTP_STATE_RECEIVE_EN) != 0 &&
         (temp & KSZ8563_PORTn_MSTP_STATE_LEARNING_DIS) == 0)
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

void ksz8563SetAgingTime(NetInterface *interface, uint32_t agingTime)
{
   //The Age Period in combination with the Age Count field determines the
   //aging time of dynamic entries in the address lookup table
   agingTime = (agingTime + 3) / 4;

   //Limit the range of the parameter
   agingTime = MIN(agingTime, 255);

   //Write the value to Switch Lookup Engine Control 3 register
   ksz8563WriteSwitchReg8(interface, KSZ8563_SWITCH_LUE_CTRL3,
      (uint8_t) agingTime);
}


/**
 * @brief Enable IGMP snooping
 * @param[in] interface Underlying network interface
 * @param[in] enable Enable or disable IGMP snooping
 **/

void ksz8563EnableIgmpSnooping(NetInterface *interface, bool_t enable)
{
   uint8_t temp;

   //Read the Global Port Mirroring and Snooping Control register
   temp = ksz8563ReadSwitchReg8(interface,
      KSZ8563_GLOBAL_PORT_MIRROR_SNOOP_CTRL);

   //Enable or disable IGMP snooping
   if(enable)
   {
      temp |= KSZ8563_GLOBAL_PORT_MIRROR_SNOOP_CTRL_IGMP_SNOOP_EN;
   }
   else
   {
      temp &= ~KSZ8563_GLOBAL_PORT_MIRROR_SNOOP_CTRL_IGMP_SNOOP_EN;
   }

   //Write the value back to Global Port Mirroring and Snooping Control register
   ksz8563WriteSwitchReg8(interface, KSZ8563_GLOBAL_PORT_MIRROR_SNOOP_CTRL,
      temp);
}


/**
 * @brief Enable MLD snooping
 * @param[in] interface Underlying network interface
 * @param[in] enable Enable or disable MLD snooping
 **/

void ksz8563EnableMldSnooping(NetInterface *interface, bool_t enable)
{
   uint8_t temp;

   //Read the Global Port Mirroring and Snooping Control register
   temp = ksz8563ReadSwitchReg8(interface,
      KSZ8563_GLOBAL_PORT_MIRROR_SNOOP_CTRL);

   //Enable or disable MLD snooping
   if(enable)
   {
      temp |= KSZ8563_GLOBAL_PORT_MIRROR_SNOOP_CTRL_MLD_SNOOP_EN;
   }
   else
   {
      temp &= ~KSZ8563_GLOBAL_PORT_MIRROR_SNOOP_CTRL_MLD_SNOOP_EN;
   }

   //Write the value back to Global Port Mirroring and Snooping Control register
   ksz8563WriteSwitchReg8(interface, KSZ8563_GLOBAL_PORT_MIRROR_SNOOP_CTRL,
      temp);
}


/**
 * @brief Enable reserved multicast table
 * @param[in] interface Underlying network interface
 * @param[in] enable Enable or disable reserved group addresses
 **/

void ksz8563EnableRsvdMcastTable(NetInterface *interface, bool_t enable)
{
   uint8_t temp;

   //Read the Switch Lookup Engine Control 0 register
   temp = ksz8563ReadSwitchReg8(interface, KSZ8563_SWITCH_LUE_CTRL0);

   //Enable or disable the reserved multicast table
   if(enable)
   {
      temp |= KSZ8563_SWITCH_LUE_CTRL0_RESERVED_MCAST_LOOKUP_EN;
   }
   else
   {
      temp &= ~KSZ8563_SWITCH_LUE_CTRL0_RESERVED_MCAST_LOOKUP_EN;
   }

   //Write the value back to Switch Lookup Engine Control 0 register
   ksz8563WriteSwitchReg8(interface, KSZ8563_SWITCH_LUE_CTRL0, temp);
}


/**
 * @brief Add a new entry to the static MAC table
 * @param[in] interface Underlying network interface
 * @param[in] entry Pointer to the forwarding database entry
 * @return Error code
 **/

error_t ksz8563AddStaticFdbEntry(NetInterface *interface,
   const SwitchFdbEntry *entry)
{
   error_t error;
   uint_t i;
   uint_t j;
   uint32_t value;
   SwitchFdbEntry currentEntry;

   //Keep track of the first free entry
   j = KSZ8563_STATIC_MAC_TABLE_SIZE;

   //Loop through the static MAC table
   for(i = 0; i < KSZ8563_STATIC_MAC_TABLE_SIZE; i++)
   {
      //Read current entry
      error = ksz8563GetStaticFdbEntry(interface, i, &currentEntry);

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
         if(j == KSZ8563_STATIC_MAC_TABLE_SIZE)
         {
            j = i;
         }
      }
   }

   //Any entry available?
   if(j < KSZ8563_STATIC_MAC_TABLE_SIZE)
   {
      //Write the Static Address Table Entry 1 register
      ksz8563WriteSwitchReg32(interface, KSZ8563_STATIC_TABLE_ENTRY1,
         KSZ8563_STATIC_TABLE_ENTRY1_VALID);

      //Set the relevant forward ports
      if(entry->destPorts == SWITCH_CPU_PORT_MASK)
      {
         value = KSZ8563_PORT3_MASK;
      }
      else
      {
         value = entry->destPorts & KSZ8563_PORT_MASK;
      }

      //Enable overriding of port state
      if(entry->override)
      {
         value |= KSZ8563_STATIC_TABLE_ENTRY2_OVERRIDE;
      }

      //Write the Static Address Table Entry 2 register
      ksz8563WriteSwitchReg32(interface, KSZ8563_STATIC_TABLE_ENTRY2, value);

      //Copy MAC address (first 16 bits)
      value = (entry->macAddr.b[0] << 8) | entry->macAddr.b[1];

      //Write the Static Address Table Entry 3 register
      ksz8563WriteSwitchReg32(interface, KSZ8563_STATIC_TABLE_ENTRY3, value);

      //Copy MAC address (last 32 bits)
      value = (entry->macAddr.b[2] << 24) | (entry->macAddr.b[3] << 16) |
         (entry->macAddr.b[4] << 8) | entry->macAddr.b[5];

      //Write the Static Address Table Entry 4 register
      ksz8563WriteSwitchReg32(interface, KSZ8563_STATIC_TABLE_ENTRY4, value);

      //Write the TABLE_INDEX field with the 4-bit index value
      value = (j << 16) & KSZ8563_STATIC_RES_MCAST_TABLE_CTRL_TABLE_INDEX;
      //Set the TABLE_SELECT bit to 0 to select the static address table
      value &= ~KSZ8563_STATIC_RES_MCAST_TABLE_CTRL_TABLE_SELECT;
      //Set the ACTION bit to 0 to indicate a write operation
      value &= ~KSZ8563_STATIC_RES_MCAST_TABLE_CTRL_ACTION;
      //Set the START_FINISH bit to 1 to initiate the operation
      value |= KSZ8563_STATIC_RES_MCAST_TABLE_CTRL_START_FINISH;

      //Start the write operation
      ksz8563WriteSwitchReg32(interface, KSZ8563_STATIC_RES_MCAST_TABLE_CTRL,
         value);

      //When the operation is complete, the START_FINISH bit will be cleared
      //automatically
      do
      {
         //Read the Static Address and Reserved Multicast Table Control register
         value = ksz8563ReadSwitchReg32(interface,
            KSZ8563_STATIC_RES_MCAST_TABLE_CTRL);

         //Poll the START_FINISH bit
      } while((value & KSZ8563_STATIC_RES_MCAST_TABLE_CTRL_START_FINISH) != 0);

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

error_t ksz8563DeleteStaticFdbEntry(NetInterface *interface,
   const SwitchFdbEntry *entry)
{
   error_t error;
   uint_t j;
   uint32_t value;
   SwitchFdbEntry currentEntry;

   //Loop through the static MAC table
   for(j = 0; j < KSZ8563_STATIC_MAC_TABLE_SIZE; j++)
   {
      //Read current entry
      error = ksz8563GetStaticFdbEntry(interface, j, &currentEntry);

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
   if(j < KSZ8563_STATIC_MAC_TABLE_SIZE)
   {
      //Clear Static Address Table Entry registers
      ksz8563WriteSwitchReg32(interface, KSZ8563_STATIC_TABLE_ENTRY1, 0);
      ksz8563WriteSwitchReg32(interface, KSZ8563_STATIC_TABLE_ENTRY2, 0);
      ksz8563WriteSwitchReg32(interface, KSZ8563_STATIC_TABLE_ENTRY3, 0);
      ksz8563WriteSwitchReg32(interface, KSZ8563_STATIC_TABLE_ENTRY4, 0);

      //Write the TABLE_INDEX field with the 4-bit index value
      value = (j << 16) & KSZ8563_STATIC_RES_MCAST_TABLE_CTRL_TABLE_INDEX;
      //Set the TABLE_SELECT bit to 0 to select the static address table
      value &= ~KSZ8563_STATIC_RES_MCAST_TABLE_CTRL_TABLE_SELECT;
      //Set the ACTION bit to 0 to indicate a write operation
      value &= ~KSZ8563_STATIC_RES_MCAST_TABLE_CTRL_ACTION;
      //Set the START_FINISH bit to 1 to initiate the operation
      value |= KSZ8563_STATIC_RES_MCAST_TABLE_CTRL_START_FINISH;

      //Start the write operation
      ksz8563WriteSwitchReg32(interface, KSZ8563_STATIC_RES_MCAST_TABLE_CTRL,
         value);

      //When the operation is complete, the START_FINISH bit will be cleared
      //automatically
      do
      {
         //Read the Static Address and Reserved Multicast Table Control register
         value = ksz8563ReadSwitchReg32(interface,
            KSZ8563_STATIC_RES_MCAST_TABLE_CTRL);

         //Poll the START_FINISH bit
      } while((value & KSZ8563_STATIC_RES_MCAST_TABLE_CTRL_START_FINISH) != 0);

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

error_t ksz8563GetStaticFdbEntry(NetInterface *interface, uint_t index,
   SwitchFdbEntry *entry)
{
   error_t error;
   uint32_t value;

   //Check index parameter
   if(index < KSZ8563_STATIC_MAC_TABLE_SIZE)
   {
      //Write the TABLE_INDEX field with the 4-bit index value
      value = (index << 16) & KSZ8563_STATIC_RES_MCAST_TABLE_CTRL_TABLE_INDEX;
      //Set the TABLE_SELECT bit to 0 to select the static address table
      value &= ~KSZ8563_STATIC_RES_MCAST_TABLE_CTRL_TABLE_SELECT;
      //Set the ACTION bit to 1 to indicate a read operation
      value |= KSZ8563_STATIC_RES_MCAST_TABLE_CTRL_ACTION;
      //Set the START_FINISH bit to 1 to initiate the operation
      value |= KSZ8563_STATIC_RES_MCAST_TABLE_CTRL_START_FINISH;

      //Start the read operation
      ksz8563WriteSwitchReg32(interface, KSZ8563_STATIC_RES_MCAST_TABLE_CTRL,
         value);

      //When the operation is complete, the START_FINISH bit will be cleared
      //automatically
      do
      {
         //Read the Static Address and Reserved Multicast Table Control register
         value = ksz8563ReadSwitchReg32(interface,
            KSZ8563_STATIC_RES_MCAST_TABLE_CTRL);

         //Poll the START_FINISH bit
      } while((value & KSZ8563_STATIC_RES_MCAST_TABLE_CTRL_START_FINISH) != 0);

      //Read the Static Address Table Entry 1 register
      value = ksz8563ReadSwitchReg32(interface, KSZ8563_STATIC_TABLE_ENTRY1);

      //Valid entry?
      if((value & KSZ8563_STATIC_TABLE_ENTRY1_VALID) != 0)
      {
         //Read the Static Address Table Entry 2 register
         value = ksz8563ReadSwitchReg32(interface, KSZ8563_STATIC_TABLE_ENTRY2);

         //Retrieve the ports associated with this MAC address
         entry->srcPort = 0;
         entry->destPorts = value & KSZ8563_STATIC_TABLE_ENTRY2_PORT_FORWARD;

         //Check the value of the OVERRIDE bit
         if((value & KSZ8563_STATIC_TABLE_ENTRY2_OVERRIDE) != 0)
         {
            entry->override = TRUE;
         }
         else
         {
            entry->override = FALSE;
         }

         //Read the Static Address Table Entry 3 register
         value = ksz8563ReadSwitchReg32(interface, KSZ8563_STATIC_TABLE_ENTRY3);

         //Copy MAC address (first 16 bits)
         entry->macAddr.b[0] = (value >> 8) & 0xFF;
         entry->macAddr.b[1] = value & 0xFF;

         //Read the Static Address Table Entry 4 register
         value = ksz8563ReadSwitchReg32(interface, KSZ8563_STATIC_TABLE_ENTRY4);

         //Copy MAC address (last 32 bits)
         entry->macAddr.b[2] = (value >> 24) & 0xFF;
         entry->macAddr.b[3] = (value >> 16) & 0xFF;
         entry->macAddr.b[4] = (value >> 8) & 0xFF;
         entry->macAddr.b[5] = value & 0xFF;

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

void ksz8563FlushStaticFdbTable(NetInterface *interface)
{
   uint_t i;
   uint32_t value;

   //Loop through the static MAC table
   for(i = 0; i < KSZ8563_STATIC_MAC_TABLE_SIZE; i++)
   {
      //Clear Static Address Table Entry registers
      ksz8563WriteSwitchReg32(interface, KSZ8563_STATIC_TABLE_ENTRY1, 0);
      ksz8563WriteSwitchReg32(interface, KSZ8563_STATIC_TABLE_ENTRY2, 0);
      ksz8563WriteSwitchReg32(interface, KSZ8563_STATIC_TABLE_ENTRY3, 0);
      ksz8563WriteSwitchReg32(interface, KSZ8563_STATIC_TABLE_ENTRY4, 0);

      //Write the TABLE_INDEX field with the 4-bit index value
      value = (i << 16) & KSZ8563_STATIC_RES_MCAST_TABLE_CTRL_TABLE_INDEX;
      //Set the TABLE_SELECT bit to 0 to select the static address table
      value &= ~KSZ8563_STATIC_RES_MCAST_TABLE_CTRL_TABLE_SELECT;
      //Set the ACTION bit to 0 to indicate a write operation
      value &= ~KSZ8563_STATIC_RES_MCAST_TABLE_CTRL_ACTION;
      //Set the START_FINISH bit to 1 to initiate the operation
      value |= KSZ8563_STATIC_RES_MCAST_TABLE_CTRL_START_FINISH;

      //Start the write operation
      ksz8563WriteSwitchReg32(interface, KSZ8563_STATIC_RES_MCAST_TABLE_CTRL,
         value);

      //When the operation is complete, the START_FINISH bit will be cleared
      //automatically
      do
      {
         //Read the Static Address and Reserved Multicast Table Control register
         value = ksz8563ReadSwitchReg32(interface,
            KSZ8563_STATIC_RES_MCAST_TABLE_CTRL);

         //Poll the START_FINISH bit
      } while((value & KSZ8563_STATIC_RES_MCAST_TABLE_CTRL_START_FINISH) != 0);
   }
}


/**
 * @brief Read an entry from the dynamic MAC table
 * @param[in] interface Underlying network interface
 * @param[in] index Zero-based index of the entry to read
 * @param[out] entry Pointer to the forwarding database entry
 * @return Error code
 **/

error_t ksz8563GetDynamicFdbEntry(NetInterface *interface, uint_t index,
   SwitchFdbEntry *entry)
{
   error_t error;
   uint32_t value;

   //First entry?
   if(index == 0)
   {
      //Clear the ALU Table Access Control register to stop any operation
      ksz8563WriteSwitchReg32(interface, KSZ8563_ALU_TABLE_CTRL, 0);

      //Start the search operation
      ksz8563WriteSwitchReg32(interface, KSZ8563_ALU_TABLE_CTRL,
         KSZ8563_ALU_TABLE_CTRL_START_FINISH |
         KSZ8563_ALU_TABLE_CTRL_ACTION_SEARCH);
   }

   //Poll the VALID_ENTRY_OR_SEARCH_END bit until it is set
   do
   {
      //Read the ALU Table Access Control register
      value = ksz8563ReadSwitchReg32(interface, KSZ8563_ALU_TABLE_CTRL);

      //This bit goes high to indicate either a new valid entry is returned or
      //the search is complete
   } while((value & KSZ8563_ALU_TABLE_CTRL_VALID_ENTRY_OR_SEARCH_END) == 0);

   //Check whether the next valid entry is ready
   if((value & KSZ8563_ALU_TABLE_CTRL_VALID) != 0)
   {
      //Store the data from the ALU table entry
      entry->destPorts = 0;
      entry->override = FALSE;

      //Read the ALU Table Entry 1 and 2 registers
      value = ksz8563ReadSwitchReg32(interface, KSZ8563_ALU_TABLE_ENTRY1);
      value = ksz8563ReadSwitchReg32(interface, KSZ8563_ALU_TABLE_ENTRY2);

      //Retrieve the port associated with this MAC address
      switch(value & KSZ8563_ALU_TABLE_ENTRY2_PORT_FORWARD)
      {
      case KSZ8563_ALU_TABLE_ENTRY2_PORT1_FORWARD:
         entry->srcPort = KSZ8563_PORT1;
         break;
      case KSZ8563_ALU_TABLE_ENTRY2_PORT2_FORWARD:
         entry->srcPort = KSZ8563_PORT2;
         break;
      case KSZ8563_ALU_TABLE_ENTRY2_PORT3_FORWARD:
         entry->srcPort = KSZ8563_PORT3;
         break;
      default:
         entry->srcPort = 0;
         break;
      }

      //Read the ALU Table Entry 3 register
      value = ksz8563ReadSwitchReg32(interface, KSZ8563_ALU_TABLE_ENTRY3);

      //Copy MAC address (first 16 bits)
      entry->macAddr.b[0] = (value >> 8) & 0xFF;
      entry->macAddr.b[1] = value & 0xFF;

      //Read the ALU Table Entry 4 register
      value = ksz8563ReadSwitchReg32(interface, KSZ8563_ALU_TABLE_ENTRY4);

      //Copy MAC address (last 32 bits)
      entry->macAddr.b[2] = (value >> 24) & 0xFF;
      entry->macAddr.b[3] = (value >> 16) & 0xFF;
      entry->macAddr.b[4] = (value >> 8) & 0xFF;
      entry->macAddr.b[5] = value & 0xFF;

      //Successful processing
      error = NO_ERROR;
   }
   else
   {
      //The search can be stopped any time by setting the START_FINISH bit to 0
      ksz8563WriteSwitchReg32(interface, KSZ8563_ALU_TABLE_CTRL, 0);

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

void ksz8563FlushDynamicFdbTable(NetInterface *interface, uint8_t port)
{
   uint_t temp;
   uint8_t state;

   //Flush only dynamic table entries
   temp = ksz8563ReadSwitchReg8(interface, KSZ8563_SWITCH_LUE_CTRL2);
   temp &= ~KSZ8563_SWITCH_LUE_CTRL2_FLUSH_OPTION;
   temp |= KSZ8563_SWITCH_LUE_CTRL2_FLUSH_OPTION_DYNAMIC;
   ksz8563WriteSwitchReg8(interface, KSZ8563_SWITCH_LUE_CTRL2, temp);

   //Valid port number?
   if(port >= KSZ8563_PORT1 && port <= KSZ8563_PORT3)
   {
      //Save the current state of the port
      state = ksz8563ReadSwitchReg8(interface, KSZ8563_PORTn_MSTP_STATE(port));

      //Turn off learning capability
      ksz8563WriteSwitchReg8(interface, KSZ8563_PORTn_MSTP_STATE(port),
         state | KSZ8563_PORTn_MSTP_STATE_LEARNING_DIS);

      //All the entries associated with a port that has its learning capability
      //being turned off will be flushed
      temp = ksz8563ReadSwitchReg8(interface, KSZ8563_SWITCH_LUE_CTRL1);
      temp |= KSZ8563_SWITCH_LUE_CTRL1_FLUSH_MSTP_ENTRIES;
      ksz8563WriteSwitchReg8(interface, KSZ8563_SWITCH_LUE_CTRL1, temp);

      //Restore the original state of the port
      ksz8563WriteSwitchReg8(interface, KSZ8563_PORTn_MSTP_STATE(port), state);
   }
   else
   {
      //Trigger a flush of the entire address lookup table
      temp = ksz8563ReadSwitchReg8(interface, KSZ8563_SWITCH_LUE_CTRL1);
      temp |= KSZ8563_SWITCH_LUE_CTRL1_FLUSH_ALU_TABLE;
      ksz8563WriteSwitchReg8(interface, KSZ8563_SWITCH_LUE_CTRL1, temp);
   }
}


/**
 * @brief Set forward ports for unknown multicast packets
 * @param[in] interface Underlying network interface
 * @param[in] enable Enable or disable forwarding of unknown multicast packets
 * @param[in] forwardPorts Port map
 **/

void ksz8563SetUnknownMcastFwdPorts(NetInterface *interface,
   bool_t enable, uint32_t forwardPorts)
{
   uint32_t temp;

   //Read Unknown Multicast Control register
   temp = ksz8563ReadSwitchReg32(interface, KSZ8563_UNKONWN_MULTICAST_CTRL);

   //Clear port map
   temp &= ~KSZ8563_UNKONWN_MULTICAST_CTRL_FWD_MAP;

   //Enable or disable forwarding of unknown multicast packets
   if(enable)
   {
      //Enable forwarding
      temp |= KSZ8563_UNKONWN_MULTICAST_CTRL_FWD;
      
      //Check whether unknown multicast packets should be forwarded to the CPU port
      if((forwardPorts & SWITCH_CPU_PORT_MASK) != 0)
      {
         temp |= KSZ8563_UNKONWN_MULTICAST_CTRL_FWD_MAP_PORT3;
      }

      //Select the desired forward ports
      temp |= forwardPorts & KSZ8563_UNKONWN_MULTICAST_CTRL_FWD_MAP_ALL;
   }
   else
   {
      //Disable forwarding
      temp &= ~KSZ8563_UNKONWN_MULTICAST_CTRL_FWD;
   }

   //Write the value back to Unknown Multicast Control register
   ksz8563WriteSwitchReg32(interface, KSZ8563_UNKONWN_MULTICAST_CTRL, temp);
}


/**
 * @brief Write PHY register
 * @param[in] interface Underlying network interface
 * @param[in] port Port number
 * @param[in] address PHY register address
 * @param[in] data Register value
 **/

void ksz8563WritePhyReg(NetInterface *interface, uint8_t port,
   uint8_t address, uint16_t data)
{
   uint16_t n;

   //SPI slave mode?
   if(interface->spiDriver != NULL)
   {
      //The SPI interface provides access to all PHY registers
      n = KSZ8563_PORTn_ETH_PHY_REG(port, address);
      //Write the 16-bit value
      ksz8563WriteSwitchReg16(interface, n, data);
   }
   else if(interface->smiDriver != NULL)
   {
      //Write the specified PHY register
      interface->smiDriver->writePhyReg(SMI_OPCODE_WRITE, port, address, data);
   }
   else
   {
      //Write the specified PHY register
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

uint16_t ksz8563ReadPhyReg(NetInterface *interface, uint8_t port,
   uint8_t address)
{
   uint16_t n;
   uint16_t data;

   //SPI slave mode?
   if(interface->spiDriver != NULL)
   {
      //The SPI interface provides access to all PHY registers
      n = KSZ8563_PORTn_ETH_PHY_REG(port, address);
      //Read the 16-bit value
      data = ksz8563ReadSwitchReg16(interface, n);
   }
   else if(interface->smiDriver != NULL)
   {
      //Read the specified PHY register
      data = interface->smiDriver->readPhyReg(SMI_OPCODE_READ, port, address);
   }
   else
   {
      //Read the specified PHY register
      data = interface->nicDriver->readPhyReg(SMI_OPCODE_READ, port, address);
   }

   //Return register value
   return data;
}


/**
 * @brief Dump PHY registers for debugging purpose
 * @param[in] interface Underlying network interface
 * @param[in] port Port number
 **/

void ksz8563DumpPhyReg(NetInterface *interface, uint8_t port)
{
   uint8_t i;

   //Loop through PHY registers
   for(i = 0; i < 32; i++)
   {
      //Display current PHY register
      TRACE_DEBUG("%02" PRIu8 ": 0x%04" PRIX16 "\r\n", i,
         ksz8563ReadPhyReg(interface, port, i));
   }

   //Terminate with a line feed
   TRACE_DEBUG("\r\n");
}


/**
 * @brief Write MMD register
 * @param[in] interface Underlying network interface
 * @param[in] port Port number
 * @param[in] devAddr Device address
 * @param[in] regAddr Register address
 * @param[in] data Register value
 **/

void ksz8563WriteMmdReg(NetInterface *interface, uint8_t port,
   uint8_t devAddr, uint16_t regAddr, uint16_t data)
{
   //Select register operation
   ksz8563WritePhyReg(interface, port, KSZ8563_MMDACR,
      KSZ8563_MMDACR_FUNC_ADDR | (devAddr & KSZ8563_MMDACR_DEVAD));

   //Write MMD register address
   ksz8563WritePhyReg(interface, port, KSZ8563_MMDAADR, regAddr);

   //Select data operation
   ksz8563WritePhyReg(interface, port, KSZ8563_MMDACR,
      KSZ8563_MMDACR_FUNC_DATA_NO_POST_INC | (devAddr & KSZ8563_MMDACR_DEVAD));

   //Write the content of the MMD register
   ksz8563WritePhyReg(interface, port, KSZ8563_MMDAADR, data);
}


/**
 * @brief Read MMD register
 * @param[in] interface Underlying network interface
 * @param[in] port Port number
 * @param[in] devAddr Device address
 * @param[in] regAddr Register address
 * @return Register value
 **/

uint16_t ksz8563ReadMmdReg(NetInterface *interface, uint8_t port,
   uint8_t devAddr, uint16_t regAddr)
{
   //Select register operation
   ksz8563WritePhyReg(interface, port, KSZ8563_MMDACR,
      KSZ8563_MMDACR_FUNC_ADDR | (devAddr & KSZ8563_MMDACR_DEVAD));

   //Write MMD register address
   ksz8563WritePhyReg(interface, port, KSZ8563_MMDAADR, regAddr);

   //Select data operation
   ksz8563WritePhyReg(interface, port, KSZ8563_MMDACR,
      KSZ8563_MMDACR_FUNC_DATA_NO_POST_INC | (devAddr & KSZ8563_MMDACR_DEVAD));

   //Read the content of the MMD register
   return ksz8563ReadPhyReg(interface, port, KSZ8563_MMDAADR);
}


/**
 * @brief Write switch register (8 bits)
 * @param[in] interface Underlying network interface
 * @param[in] address Switch register address
 * @param[in] data Register value
 **/

void ksz8563WriteSwitchReg8(NetInterface *interface, uint16_t address,
   uint8_t data)
{
   uint32_t command;

   //SPI slave mode?
   if(interface->spiDriver != NULL)
   {
      //Set up a write operation
      command = KSZ8563_SPI_CMD_WRITE;
      //Set register address
      command |= (address << 5) & KSZ8563_SPI_CMD_ADDR;

      //Pull the CS pin low
      interface->spiDriver->assertCs();

      //Write 32-bit command
      interface->spiDriver->transfer((command >> 24) & 0xFF);
      interface->spiDriver->transfer((command >> 16) & 0xFF);
      interface->spiDriver->transfer((command >> 8) & 0xFF);
      interface->spiDriver->transfer(command & 0xFF);

      //Write 8-bit data
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
 * @brief Read switch register (8 bits)
 * @param[in] interface Underlying network interface
 * @param[in] address Switch register address
 * @return Register value
 **/

uint8_t ksz8563ReadSwitchReg8(NetInterface *interface, uint16_t address)
{
   uint8_t data;
   uint32_t command;

   //SPI slave mode?
   if(interface->spiDriver != NULL)
   {
      //Set up a read operation
      command = KSZ8563_SPI_CMD_READ;
      //Set register address
      command |= (address << 5) & KSZ8563_SPI_CMD_ADDR;

      //Pull the CS pin low
      interface->spiDriver->assertCs();

      //Write 32-bit command
      interface->spiDriver->transfer((command >> 24) & 0xFF);
      interface->spiDriver->transfer((command >> 16) & 0xFF);
      interface->spiDriver->transfer((command >> 8) & 0xFF);
      interface->spiDriver->transfer(command & 0xFF);

      //Read 8-bit data
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
 * @brief Write switch register (16 bits)
 * @param[in] interface Underlying network interface
 * @param[in] address Switch register address
 * @param[in] data Register value
 **/

void ksz8563WriteSwitchReg16(NetInterface *interface, uint16_t address,
   uint16_t data)
{
   uint32_t command;

   //SPI slave mode?
   if(interface->spiDriver != NULL)
   {
      //Set up a write operation
      command = KSZ8563_SPI_CMD_WRITE;
      //Set register address
      command |= (address << 5) & KSZ8563_SPI_CMD_ADDR;

      //Pull the CS pin low
      interface->spiDriver->assertCs();

      //Write 32-bit command
      interface->spiDriver->transfer((command >> 24) & 0xFF);
      interface->spiDriver->transfer((command >> 16) & 0xFF);
      interface->spiDriver->transfer((command >> 8) & 0xFF);
      interface->spiDriver->transfer(command & 0xFF);

      //Write 16-bit data
      interface->spiDriver->transfer((data >> 8) & 0xFF);
      interface->spiDriver->transfer(data & 0xFF);

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
 * @brief Read switch register (16 bits)
 * @param[in] interface Underlying network interface
 * @param[in] address Switch register address
 * @return Register value
 **/

uint16_t ksz8563ReadSwitchReg16(NetInterface *interface, uint16_t address)
{
   uint16_t data;
   uint32_t command;

   //SPI slave mode?
   if(interface->spiDriver != NULL)
   {
      //Set up a read operation
      command = KSZ8563_SPI_CMD_READ;
      //Set register address
      command |= (address << 5) & KSZ8563_SPI_CMD_ADDR;

      //Pull the CS pin low
      interface->spiDriver->assertCs();

      //Write 32-bit command
      interface->spiDriver->transfer((command >> 24) & 0xFF);
      interface->spiDriver->transfer((command >> 16) & 0xFF);
      interface->spiDriver->transfer((command >> 8) & 0xFF);
      interface->spiDriver->transfer(command & 0xFF);

      //Read 16-bit data
      data = interface->spiDriver->transfer(0xFF) << 8;
      data |= interface->spiDriver->transfer(0xFF);

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
 * @brief Write switch register (32 bits)
 * @param[in] interface Underlying network interface
 * @param[in] address Switch register address
 * @param[in] data Register value
 **/

void ksz8563WriteSwitchReg32(NetInterface *interface, uint16_t address,
   uint32_t data)
{
   uint32_t command;

   //SPI slave mode?
   if(interface->spiDriver != NULL)
   {
      //Set up a write operation
      command = KSZ8563_SPI_CMD_WRITE;
      //Set register address
      command |= (address << 5) & KSZ8563_SPI_CMD_ADDR;

      //Pull the CS pin low
      interface->spiDriver->assertCs();

      //Write 32-bit command
      interface->spiDriver->transfer((command >> 24) & 0xFF);
      interface->spiDriver->transfer((command >> 16) & 0xFF);
      interface->spiDriver->transfer((command >> 8) & 0xFF);
      interface->spiDriver->transfer(command & 0xFF);

      //Write 32-bit data
      interface->spiDriver->transfer((data >> 24) & 0xFF);
      interface->spiDriver->transfer((data >> 16) & 0xFF);
      interface->spiDriver->transfer((data >> 8) & 0xFF);
      interface->spiDriver->transfer(data & 0xFF);

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
 * @brief Read switch register (32 bits)
 * @param[in] interface Underlying network interface
 * @param[in] address Switch register address
 * @return Register value
 **/

uint32_t ksz8563ReadSwitchReg32(NetInterface *interface, uint16_t address)
{
   uint32_t data;
   uint32_t command;

   //SPI slave mode?
   if(interface->spiDriver != NULL)
   {
      //Set up a read operation
      command = KSZ8563_SPI_CMD_READ;
      //Set register address
      command |= (address << 5) & KSZ8563_SPI_CMD_ADDR;

      //Pull the CS pin low
      interface->spiDriver->assertCs();

      //Write 32-bit command
      interface->spiDriver->transfer((command >> 24) & 0xFF);
      interface->spiDriver->transfer((command >> 16) & 0xFF);
      interface->spiDriver->transfer((command >> 8) & 0xFF);
      interface->spiDriver->transfer(command & 0xFF);

      //Read 32-bit data
      data = interface->spiDriver->transfer(0xFF) << 24;
      data |= interface->spiDriver->transfer(0xFF) << 16;
      data |= interface->spiDriver->transfer(0xFF) << 8;
      data |= interface->spiDriver->transfer(0xFF);

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
