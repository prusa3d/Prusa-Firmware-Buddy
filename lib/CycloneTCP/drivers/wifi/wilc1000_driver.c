/**
 * @file wilc1000_driver.c
 * @brief WILC1000 Wi-Fi controller
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
#include "drivers/wifi/wilc1000_driver.h"
#include "wilc1000_config.h"
#include "debug.h"

//Underlying network interface
NetInterface *wilc1000StaInterface = NULL;
NetInterface *wilc1000ApInterface = NULL;

//Transmit buffer
static uint8_t txBuffer[WILC1000_TX_BUFFER_SIZE];
//Receive buffer
static uint8_t rxBuffer[WILC1000_RX_BUFFER_SIZE];


/**
 * @brief WILC1000 driver (STA mode)
 **/

const NicDriver wilc1000StaDriver =
{
   NIC_TYPE_ETHERNET,
   ETH_MTU,
   wilc1000Init,
   wilc1000Tick,
   wilc1000EnableIrq,
   wilc1000DisableIrq,
   wilc1000EventHandler,
   wilc1000SendPacket,
   wilc1000UpdateMacAddrFilter,
   NULL,
   NULL,
   NULL,
   TRUE,
   TRUE,
   TRUE,
   TRUE
};


/**
 * @brief WILC1000 driver (AP mode)
 **/

const NicDriver wilc1000ApDriver =
{
   NIC_TYPE_ETHERNET,
   ETH_MTU,
   wilc1000Init,
   wilc1000Tick,
   wilc1000EnableIrq,
   wilc1000DisableIrq,
   wilc1000EventHandler,
   wilc1000SendPacket,
   wilc1000UpdateMacAddrFilter,
   NULL,
   NULL,
   NULL,
   TRUE,
   TRUE,
   TRUE,
   TRUE
};


/**
 * @brief WILC1000 initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t wilc1000Init(NetInterface *interface)
{
   int8_t status;
   tstrWifiInitParam param;
#if (M2M_FIRMWARE_VERSION_MAJOR_NO == 4)
   MacAddr staMacAddr;
   MacAddr apMacAddr;
#endif

   //STA or AP mode?
   if(interface->nicDriver == &wilc1000StaDriver)
   {
      //Debug message
      TRACE_INFO("Initializing WILC1000 (STA mode)...\r\n");
      //Save underlying network interface
      wilc1000StaInterface = interface;
   }
   else
   {
      //Debug message
      TRACE_INFO("Initializing WILC1000 (AP mode)...\r\n");
      //Save underlying network interface
      wilc1000ApInterface = interface;
   }

   //Start of exception handling block
   do
   {
      //Initialization sequence is performed once
      if(wilc1000StaInterface == NULL || wilc1000ApInterface == NULL)
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
         param.pfAppWifiCb = wilc1000AppWifiEvent;
         param.pfAppMonCb = NULL;
#if (M2M_FIRMWARE_VERSION_MAJOR_NO == 3)
         param.strEthInitParam.pfAppWifiCb = NULL;
#endif
         param.strEthInitParam.pfAppEthCb = wilc1000AppEthEvent;

         //Set receive buffer
         param.strEthInitParam.au8ethRcvBuf = rxBuffer;
         param.strEthInitParam.u16ethRcvBufSize = WILC1000_RX_BUFFER_SIZE;

         //Initialize WILC1000 controller
         status = m2m_wifi_init(&param);
         //Check status code
         if(status != M2M_SUCCESS)
         {
            break;
         }

#if (M2M_FIRMWARE_VERSION_MAJOR_NO == 3)
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
#endif
      }
      else
      {
         //Initialization was already done
         status = M2M_SUCCESS;
      }

#if (M2M_FIRMWARE_VERSION_MAJOR_NO == 3)
      //STA or AP mode?
      if(interface->nicDriver == &wilc1000StaDriver)
      {
         //Check whether the AP interface has been initialized first
         if(wilc1000ApInterface != NULL)
         {
            //Copy MAC address
            interface->macAddr = wilc1000ApInterface->macAddr;
            interface->eui64 = wilc1000ApInterface->eui64;
         }
      }
      else
      {
         //Check whether the STA interface has been initialized first
         if(wilc1000StaInterface != NULL)
         {
            //Copy MAC
            interface->macAddr = wilc1000StaInterface->macAddr;
            interface->eui64 = wilc1000StaInterface->eui64;
         }
      }

#elif (M2M_FIRMWARE_VERSION_MAJOR_NO == 4)
      //Retrieve current MAC addresses
      status = m2m_wifi_get_mac_address(apMacAddr.b, staMacAddr.b);
      //Check status code
      if(status != M2M_SUCCESS)
      {
         break;
      }

      //Optionally set the MAC address
      if(macCompAddr(&interface->macAddr, &MAC_UNSPECIFIED_ADDR))
      {
         //Use the factory preprogrammed MAC address
         if(interface == wilc1000StaInterface)
         {
            interface->macAddr = staMacAddr;
         }
         else
         {
            interface->macAddr = apMacAddr;
         }

         //Generate the 64-bit interface identifier
         macAddrToEui64(&interface->macAddr, &interface->eui64);
      }
      else
      {
         //Override the factory preprogrammed address
         if(interface == wilc1000StaInterface)
         {
            staMacAddr = interface->macAddr;
         }
         else
         {
            apMacAddr = interface->macAddr;
         }

         //Assign MAC addresses
         status = m2m_wifi_set_mac_address(staMacAddr.b, apMacAddr.b);
         //Check status code
         if(status != M2M_SUCCESS)
         {
            break;
         }
      }
#endif

      //End of exception handling block
   } while(0);

   //WILC1000 is now ready to send
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
 * @brief WILC1000 timer handler
 *
 * This routine is periodically called by the TCP/IP stack to handle periodic
 * operations such as polling the link state
 *
 * @param[in] interface Underlying network interface
 **/

void wilc1000Tick(NetInterface *interface)
{
}


/**
 * @brief Enable interrupts
 * @param[in] interface Underlying network interface
 **/

void wilc1000EnableIrq(NetInterface *interface)
{
}


/**
 * @brief Disable interrupts
 * @param[in] interface Underlying network interface
 **/

void wilc1000DisableIrq(NetInterface *interface)
{
}


/**
 * @brief WILC1000 interrupt service routine
 * @return TRUE if a higher priority task must be woken. Else FALSE is returned
 **/

bool_t wilc1000IrqHandler(void)
{
   bool_t flag;

   //This flag will be set if a higher priority task must be woken
   flag = FALSE;

   //STA and/or AP mode?
   if(wilc1000StaInterface != NULL)
   {
      wilc1000StaInterface->nicEvent = TRUE;
   }
   else if(wilc1000ApInterface != NULL)
   {
      wilc1000ApInterface->nicEvent = TRUE;
   }

   //Notify the TCP/IP stack of the event
   flag = osSetEventFromIsr(&netEvent);

   //A higher priority task must be woken?
   return flag;
}


/**
 * @brief WILC1000 event handler
 * @param[in] interface Underlying network interface
 **/

void wilc1000EventHandler(NetInterface *interface)
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

error_t wilc1000SendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary)
{
   int8_t status;
   size_t length;

   //Retrieve the length of the packet
   length = netBufferGetLength(buffer) - offset;

   //Check the frame length
   if(length > WILC1000_TX_BUFFER_SIZE)
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

#if (M2M_FIRMWARE_VERSION_MAJOR_NO == 3)
   //Copy user data to the transmit buffer
   netBufferRead(txBuffer, buffer, offset, length);
#elif (M2M_FIRMWARE_VERSION_MAJOR_NO == 4)
   //Copy user data to the transmit buffer
   netBufferRead(txBuffer + M2M_ETHERNET_HDR_OFFSET + M2M_ETH_PAD_SIZE,
      buffer, offset, length);
#endif

   //STA or AP mode?
   if(interface == wilc1000StaInterface)
   {
      //Send packet
#if (M2M_FIRMWARE_VERSION_MAJOR_NO == 4 && M2M_FIRMWARE_VERSION_MINOR_NO >= 2)
      status = m2m_wifi_send_ethernet_pkt(txBuffer, length, STATION_INTERFACE);
#else
      status = m2m_wifi_send_ethernet_pkt(txBuffer, length);
#endif
   }
   else
   {
      //Send packet
#if (M2M_FIRMWARE_VERSION_MAJOR_NO == 4 && M2M_FIRMWARE_VERSION_MINOR_NO >= 2)
      status = m2m_wifi_send_ethernet_pkt(txBuffer, length, AP_INTERFACE);
#else
      status = m2m_wifi_send_ethernet_pkt_ifc1(txBuffer, length);
#endif
   }

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

error_t wilc1000UpdateMacAddrFilter(NetInterface *interface)
{
   uint_t i;
   uint_t refCount;
   MacFilterEntry *entry;

   //Debug message
   TRACE_INFO("Updating WILC1000 multicast filter...\r\n");

   //The MAC address filter contains the list of MAC addresses to accept
   //when receiving an Ethernet frame
   for(i = 0; i < MAC_ADDR_FILTER_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->macAddrFilter[i];

      //Valid entry?
      if(!macCompAddr(&entry->addr, &MAC_UNSPECIFIED_ADDR))
      {
         //Check whether the multicast MAC address has already been registered
         //on the alternate interface
         if(interface == wilc1000StaInterface)
         {
            refCount = wilc1000GetAddrRefCount(wilc1000ApInterface, &entry->addr);
         }
         else
         {
            refCount = wilc1000GetAddrRefCount(wilc1000StaInterface, &entry->addr);
         }

         //Ensure that there are no duplicate address entries in the table
         if(refCount == 0)
         {
            //Update MAC filter table only if necessary
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
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Get reference count for the specified multicast MAC address
 * @param[in] interface Underlying network interface
 * @param[in] macAddr MAC address
 * @return Reference count
 **/

bool_t wilc1000GetAddrRefCount(NetInterface *interface, const MacAddr *macAddr)
{
   uint_t i;
   uint_t refCount;
   MacFilterEntry *entry;

   //Clear reference count
   refCount = 0;

   //Valid network interface?
   if(interface != NULL)
   {
      //Go through the multicast filter table
      for(i = 0; i < MAC_ADDR_FILTER_SIZE; i++)
      {
         //Point to the current entry
         entry = &interface->macAddrFilter[i];

         //Valid entry?
         if(entry->refCount > 0)
         {
            //Check whether the specified MAC address matches
            //a multicast address in the table
            if(macCompAddr(&entry->addr, macAddr))
            {
               //Get reference count
               refCount = entry->refCount;
               //We are done
               break;
            }
         }
      }
   }

   //Return reference count
   return refCount;
}


/**
 * @brief Callback function that handles Wi-Fi events
 * @param[in] msgType Type of notification
 * @param[in] msg Pointer to the buffer containing the notification parameters
 **/

void wilc1000AppWifiEvent(uint8_t msgType, void *msg)
{
   tstrM2mWifiStateChanged *stateChangedMsg;

   //Debug message
   TRACE_INFO("WILC1000 Wi-Fi event callback\r\n");

   //Check message type
   if(msgType == M2M_WIFI_RESP_FIRMWARE_STRTED)
   {
      //Debug message
      TRACE_INFO("  M2M_WIFI_RESP_FIRMWARE_STRTED\r\n");
   }
   else if(msgType == M2M_WIFI_RESP_CON_STATE_CHANGED)
   {
      //Debug message
      TRACE_INFO("  M2M_WIFI_RESP_CON_STATE_CHANGED\r\n");

      //Connection state
      stateChangedMsg = (tstrM2mWifiStateChanged *) msg;

      //Check interface identifier
#if (M2M_FIRMWARE_VERSION_MAJOR_NO == 4 && M2M_FIRMWARE_VERSION_MINOR_NO >= 2)
      if(stateChangedMsg->u8IfcId == STATION_INTERFACE)
#else
      if(stateChangedMsg->u8IfcId == INTERFACE_1)
#endif
      {
         //Check whether STA mode is enabled
         if(wilc1000StaInterface != NULL)
         {
            //Check connection state
            if(stateChangedMsg->u8CurrState == M2M_WIFI_CONNECTED)
            {
               //Link is up
               wilc1000StaInterface->linkState = TRUE;
            }
            else
            {
               //Link is down
               wilc1000StaInterface->linkState = FALSE;
            }

            //Process link state change event
            nicNotifyLinkChange(wilc1000StaInterface);
         }
      }
#if (M2M_FIRMWARE_VERSION_MAJOR_NO == 4 && M2M_FIRMWARE_VERSION_MINOR_NO >= 2)
      else if(stateChangedMsg->u8IfcId == AP_INTERFACE)
#else
      else if(stateChangedMsg->u8IfcId == INTERFACE_2)
#endif
      {
         //Check whether AP mode is enabled
         if(wilc1000ApInterface != NULL)
         {
            //Check connection state
            if(stateChangedMsg->u8CurrState == M2M_WIFI_CONNECTED)
            {
               //Check link state
               if(!wilc1000ApInterface->linkState)
               {
                  //Link is up
                  wilc1000ApInterface->linkState = TRUE;
                  //Process link state change event
                  nicNotifyLinkChange(wilc1000ApInterface);
               }
            }
         }
      }
   }

#if defined(CONF_WILC_EVENT_HOOK)
   //Release exclusive access
   osReleaseMutex(&netMutex);
   //Invoke user callback function
   CONF_WILC_EVENT_HOOK(msgType, msg);
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

void wilc1000AppEthEvent(uint8_t msgType, void *msg, void *ctrlBuf)
{
   size_t length;
   uint8_t *packet;
   NetRxAncillary ancillary;
#if (M2M_FIRMWARE_VERSION_MAJOR_NO == 4 && M2M_FIRMWARE_VERSION_MINOR_NO >= 2)
   tstrM2MDataBufCtrl *ctrl;
#else
   tstrM2mIpCtrlBuf *ctrl;
#endif

   //Debug message
   TRACE_DEBUG("WILC1000 RX event callback\r\n");

   //Point to the control buffer
#if (M2M_FIRMWARE_VERSION_MAJOR_NO == 4 && M2M_FIRMWARE_VERSION_MINOR_NO >= 2)
   ctrl = (tstrM2MDataBufCtrl *) ctrlBuf;
#else
   ctrl = (tstrM2mIpCtrlBuf *) ctrlBuf;
#endif

   //Check message type
   if(msgType == M2M_WIFI_RESP_ETHERNET_RX_PACKET)
   {
      //Debug message
      TRACE_DEBUG("  M2M_WIFI_RESP_ETHERNET_RX_PACKET\r\n");

#if (M2M_FIRMWARE_VERSION_MAJOR_NO == 3)
      //Point to the beginning of the packet
      packet = rxBuffer;
#elif (M2M_FIRMWARE_VERSION_MAJOR_NO == 4)
      //Point to the beginning of the packet
      packet = rxBuffer + ctrl->u8DataOffset;
#endif

      //Retrieve the length of the packet
      length = ctrl->u16DataSize;

      //Check interface identifier
#if (M2M_FIRMWARE_VERSION_MAJOR_NO == 4 && M2M_FIRMWARE_VERSION_MINOR_NO >= 2)
      if(ctrl->u8IfcId == STATION_INTERFACE)
#else
      if(ctrl->u8IfcId == INTERFACE_1)
#endif
      {
         //Valid interface?
         if(wilc1000StaInterface != NULL)
         {
            //Check destination MAC address
            if(wilc1000ApInterface != NULL)
            {
               if(macCompAddr(packet, wilc1000ApInterface->macAddr.b))
               {
                  macCopyAddr(packet, wilc1000StaInterface->macAddr.b);
               }
            }

            //Additional options can be passed to the stack along with the packet
            ancillary = NET_DEFAULT_RX_ANCILLARY;

            //Pass the packet to the upper layer
            nicProcessPacket(wilc1000StaInterface, packet, length, &ancillary);
         }
      }
#if (M2M_FIRMWARE_VERSION_MAJOR_NO == 4 && M2M_FIRMWARE_VERSION_MINOR_NO >= 2)
      else if(ctrl->u8IfcId == AP_INTERFACE)
#else
      else if(ctrl->u8IfcId == INTERFACE_2)
#endif
      {
         //Valid interface?
         if(wilc1000ApInterface != NULL)
         {
            //Check destination MAC address
            if(wilc1000StaInterface != NULL)
            {
               if(macCompAddr(packet, wilc1000StaInterface->macAddr.b))
               {
                  macCopyAddr(packet, wilc1000ApInterface->macAddr.b);
               }
            }

            //Additional options can be passed to the stack along with the packet
            ancillary = NET_DEFAULT_RX_ANCILLARY;

            //Pass the packet to the upper layer
            nicProcessPacket(wilc1000ApInterface, packet, length, &ancillary);
         }
      }
   }
}
