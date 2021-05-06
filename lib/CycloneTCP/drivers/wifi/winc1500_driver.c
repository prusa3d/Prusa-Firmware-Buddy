/**
 * @file winc1500_driver.c
 * @brief WINC1500 Wi-Fi controller
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
#include "driver/include/m2m_wifi.h"
#include "core/net.h"
#include "drivers/wifi/winc1500_driver.h"
#include "winc1500_config.h"
#include "debug.h"

//Underlying network interface
static NetInterface *nicDriverInterface;

//Transmit buffer
static uint8_t txBuffer[WINC1500_TX_BUFFER_SIZE];
//Receive buffer
static uint8_t rxBuffer[WINC1500_RX_BUFFER_SIZE];


/**
 * @brief WINC1500 driver
 **/

const NicDriver winc1500Driver =
{
   NIC_TYPE_ETHERNET,
   ETH_MTU,
   winc1500Init,
   winc1500Tick,
   winc1500EnableIrq,
   winc1500DisableIrq,
   winc1500EventHandler,
   winc1500SendPacket,
   winc1500UpdateMacAddrFilter,
   NULL,
   NULL,
   NULL,
   TRUE,
   TRUE,
   TRUE,
   TRUE
};


/**
 * @brief WINC1500 initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t winc1500Init(NetInterface *interface)
{
   int8_t status;
   tstrWifiInitParam param;

   //Debug message
   TRACE_INFO("Initializing WINC1500...\r\n");
   //Save underlying network interface
   nicDriverInterface = interface;

   //Start of exception handling block
   do
   {
      //Low-level initialization
      status = nm_bsp_init();

      //Check status code
      if(status != M2M_SUCCESS)
      {
         break;
      }

      //Set default parameters
      osMemset(&param, 0, sizeof(param));

      //Register callback functions
      param.pfAppWifiCb = winc1500AppWifiEvent;
      param.pfAppMonCb = NULL;
      param.strEthInitParam.pfAppWifiCb = NULL;
      param.strEthInitParam.pfAppEthCb = winc1500AppEthEvent;

      //Set receive buffer
      param.strEthInitParam.au8ethRcvBuf = rxBuffer;
      param.strEthInitParam.u16ethRcvBufSize = WINC1500_RX_BUFFER_SIZE;

      //Enable Ethernet mode
      param.strEthInitParam.u8EthernetEnable = M2M_WIFI_MODE_ETHERNET;

      //Initialize WINC1500 controller
      status = m2m_wifi_init(&param);

      //Check status code
      if(status != M2M_SUCCESS)
      {
         break;
      }

      //Optionally set the station MAC address
      if(macCompAddr(&interface->macAddr, &MAC_UNSPECIFIED_ADDR))
      {
         //Use the factory preprogrammed station address
         status = m2m_wifi_get_mac_address(interface->macAddr.b);

         //Check status code
         if(status != M2M_SUCCESS)
         {
            break;
         }

         //Generate the 64-bit interface identifier
         macAddrToEui64(&interface->macAddr, &interface->eui64);
      }
      else
      {
         //Override the factory preprogrammed address
         status = m2m_wifi_set_mac_address(interface->macAddr.b);

         //Check status code
         if(status != M2M_SUCCESS)
         {
            break;
         }
      }

      //End of exception handling block
   } while(0);

   //WINC1500 is now ready to send
   osSetEvent(&interface->nicTxEvent);

   //Return status code
   if(status == M2M_SUCCESS)
   {
      return NO_ERROR;
   }
   else
   {
      return ERROR_FAILURE;
   }
}


/**
 * @brief WINC1500 timer handler
 *
 * This routine is periodically called by the TCP/IP stack to handle periodic
 * operations such as polling the link state
 *
 * @param[in] interface Underlying network interface
 **/

void winc1500Tick(NetInterface *interface)
{
}


/**
 * @brief Enable interrupts
 * @param[in] interface Underlying network interface
 **/

void winc1500EnableIrq(NetInterface *interface)
{
}


/**
 * @brief Disable interrupts
 * @param[in] interface Underlying network interface
 **/

void winc1500DisableIrq(NetInterface *interface)
{
}


/**
 * @brief WINC1500 interrupt service routine
 * @return TRUE if a higher priority task must be woken. Else FALSE is returned
 **/

bool_t winc1500IrqHandler(void)
{
   bool_t flag;

   //This flag will be set if a higher priority task must be woken
   flag = FALSE;

   //Set event flag
   nicDriverInterface->nicEvent = TRUE;
   //Notify the TCP/IP stack of the event
   flag = osSetEventFromIsr(&netEvent);

   //A higher priority task must be woken?
   return flag;
}


/**
 * @brief WINC1500 event handler
 * @param[in] interface Underlying network interface
 **/

void winc1500EventHandler(NetInterface *interface)
{
   //Process Wi-Fi events
   m2m_wifi_handle_events(NULL);
}


/**
 * @brief Send a packet
 * @param[in] interface Underlying network interface
 * @param[in] buffer Multi-part buffer containing the data to send
 * @param[in] offset Offset to the first data byte
 * @param[in] ancillary Additional options passed to the stack along with
 *   the packet
 * @return Error code
 **/

error_t winc1500SendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary)
{
   int8_t status;
   size_t length;

   //Retrieve the length of the packet
   length = netBufferGetLength(buffer) - offset;

   //Check the frame length
   if(length > WINC1500_TX_BUFFER_SIZE)
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
      //Report an error
      return ERROR_INVALID_LENGTH;
   }

   //Make sure the link is up before transmitting the frame
   if(!interface->linkState)
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
      //Drop current packet
      return NO_ERROR;
   }

   //Copy user data to the transmit buffer
   netBufferRead(txBuffer, buffer, offset, length);

   //Send packet
   status = m2m_wifi_send_ethernet_pkt(txBuffer, length);

   //The transmitter can accept another packet
   osSetEvent(&interface->nicTxEvent);

   //Return status code
   if(status == M2M_SUCCESS)
   {
      return NO_ERROR;
   }
   else
   {
      return ERROR_FAILURE;
   }
}


/**
 * @brief Configure MAC address filtering
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t winc1500UpdateMacAddrFilter(NetInterface *interface)
{
   uint_t i;
   MacFilterEntry *entry;

   //Debug message
   TRACE_INFO("Updating WINC1500 multicast filter...\r\n");

   //The MAC address filter contains the list of MAC addresses to accept
   //when receiving an Ethernet frame
   for(i = 0; i < MAC_ADDR_FILTER_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->macAddrFilter[i];

      //Valid entry?
      if(!macCompAddr(&entry->addr, &MAC_UNSPECIFIED_ADDR))
      {
         //Check whether the MAC filter table should be updated for the
         //current multicast address
         if(entry->addFlag)
         {
            //Add a new entry to the MAC filter table
            m2m_wifi_enable_mac_mcast(entry->addr.b, TRUE);
         }
         else if(entry->deleteFlag)
         {
            //Remove the current entry from the MAC filter table
            m2m_wifi_enable_mac_mcast(entry->addr.b, FALSE);
         }
      }
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Callback function that handles Wi-Fi events
 * @param[in] msgType Type of notification
 * @param[in] msg Pointer to the buffer containing the notification parameters
 **/

void winc1500AppWifiEvent(uint8_t msgType, void *msg)
{
   tstrM2mWifiStateChanged *stateChangedMsg;

   //Debug message
   TRACE_INFO("WINC1500 Wi-Fi event callback\r\n");

   //Check message type
   if(msgType == M2M_WIFI_RESP_CON_STATE_CHANGED)
   {
      //Debug message
      TRACE_INFO("  M2M_WIFI_RESP_CON_STATE_CHANGED\r\n");

      //Connection state
      stateChangedMsg = (tstrM2mWifiStateChanged *) msg;

      //Check link state
      if(stateChangedMsg->u8CurrState == M2M_WIFI_CONNECTED)
      {
         //Link is up
         nicDriverInterface->linkState = TRUE;
      }
      else
      {
         //Link is down
         nicDriverInterface->linkState = FALSE;
      }

      //Process link state change event
      nicNotifyLinkChange(nicDriverInterface);
   }

#if defined(CONF_WINC_EVENT_HOOK)
   //Release exclusive access
   osReleaseMutex(&netMutex);
   //Invoke user callback function
   CONF_WINC_EVENT_HOOK(msgType, msg);
   //Get exclusive access
   osAcquireMutex(&netMutex);
#endif
}


/**
 * @brief Callback function that handles events in bypass mode
 * @param[in] msgType Type of notification
 * @param[in] msg Pointer to the buffer containing the notification parameters
 * @param[in] ctrlBuf Pointer to the control buffer
 **/

void winc1500AppEthEvent(uint8_t msgType, void *msg, void *ctrlBuf)
{
   size_t n;
   tstrM2mIpCtrlBuf *ctrl;
   NetRxAncillary ancillary;

   //Debug message
   TRACE_DEBUG("WINC1500 RX event callback\r\n");

   //Point to the control buffer
   ctrl = (tstrM2mIpCtrlBuf *) ctrlBuf;

   //Check message type
   if(msgType == M2M_WIFI_REQ_SEND_ETHERNET_PACKET)
   {
      //Debug message
      TRACE_DEBUG("  M2M_WIFI_REQ_SEND_ETHERNET_PACKET\r\n");
   }
   else if(msgType == M2M_WIFI_RESP_ETHERNET_RX_PACKET)
   {
      //Debug message
      TRACE_DEBUG("  M2M_WIFI_RESP_ETHERNET_RX_PACKET\r\n");

      //Retrieve the length of the packet
      n = ctrl->u16DataSize;

      //Additional options can be passed to the stack along with the packet
      ancillary = NET_DEFAULT_RX_ANCILLARY;

      //Pass the packet to the upper layer
      nicProcessPacket(nicDriverInterface, rxBuffer, n, &ancillary);
   }
}
