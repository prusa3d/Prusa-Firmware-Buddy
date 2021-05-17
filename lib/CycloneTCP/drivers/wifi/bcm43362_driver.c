/**
 * @file bcm43362_driver.c
 * @brief BCM43362 Wi-Fi controller
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
#include "drivers/wifi/bcm43362_driver.h"
#include "debug.h"

//WICED dependencies
#include "platform_init.h"
#include "platform_mcu_peripheral.h"
#include "wwd_constants.h"
#include "wwd_structures.h"
#include "wwd_buffer.h"
#include "wwd_events.h"
#include "wwd_management.h"
#include "wwd_poll.h"
#include "wwd_wifi.h"
#include "wwd_buffer_interface.h"
#include "wwd_bus_protocol_interface.h"
#include "wwd_network_constants.h"
#include "wwd_network_interface.h"

//Underlying network interface
static NetInterface *bcm43362StaInterface = NULL;
static NetInterface *bcm43362ApInterface = NULL;

//RX queue
QueueHandle_t wwdRxQueue;

//Regitered Wi-Fi events
static const wwd_event_num_t app_wifi_events[] =
{
   WLC_E_IF,
   WLC_E_LINK,
   WLC_E_ASSOC_IND,
   WLC_E_DISASSOC_IND,
   WLC_E_NONE
};

//Forward declaration of functions
void *app_wifi_event_handler(const wwd_event_header_t *event_header,
   const uint8_t *event_data, void *handler_user_data);


/**
 * @brief BCM43362 driver (STA mode)
 **/

const NicDriver bcm43362StaDriver =
{
   NIC_TYPE_ETHERNET,
   ETH_MTU,
   bcm43362Init,
   bcm43362Tick,
   bcm43362EnableIrq,
   bcm43362DisableIrq,
   bcm43362EventHandler,
   bcm43362SendPacket,
   bcm43362UpdateMacAddrFilter,
   NULL,
   NULL,
   NULL,
   TRUE,
   TRUE,
   TRUE,
   TRUE
};


/**
 * @brief BCM43362 driver (AP mode)
 **/

const NicDriver bcm43362ApDriver =
{
   NIC_TYPE_ETHERNET,
   ETH_MTU,
   bcm43362Init,
   bcm43362Tick,
   bcm43362EnableIrq,
   bcm43362DisableIrq,
   bcm43362EventHandler,
   bcm43362SendPacket,
   bcm43362UpdateMacAddrFilter,
   NULL,
   NULL,
   NULL,
   TRUE,
   TRUE,
   TRUE,
   TRUE
};


/**
 * @brief BCM43362 initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t bcm43362Init(NetInterface *interface)
{
   wwd_result_t ret;
   //MacAddr macAddr;

   //STA or AP mode?
   if(interface->nicDriver == &bcm43362StaDriver)
   {
      //Debug message
      TRACE_INFO("Initializing BCM43362 (STA mode)...\r\n");
   }
   else
   {
      //Debug message
      TRACE_INFO("Initializing BCM43362 (AP mode)...\r\n");
   }

   //Start of exception handling block
   do
   {
      //Initialization sequence is performed once at startup
      if(bcm43362StaInterface == NULL && bcm43362ApInterface == NULL)
      {
         platform_init_mcu_infrastructure();
         wwd_buffer_init(NULL);

         //Create TX queue
         wwdRxQueue = xQueueCreate(16, sizeof(wiced_buffer_t));

         //Initialize Wi-Fi controller
         ret = wwd_management_wifi_on(WICED_COUNTRY_FRANCE);
         TRACE_INFO("wwd_management_wifi_on=%d (0x%04X)\r\n", ret, ret);

         ret = wwd_management_set_event_handler(app_wifi_events, app_wifi_event_handler, NULL, WWD_AP_INTERFACE);
         TRACE_INFO("wwd_management_set_event_handler=%d (0x%04X)\r\n", ret, ret);
      }
      else
      {
         //Initialization was already done
         ret = WWD_SUCCESS;
      }

      //STA or AP mode?
      if(interface->nicDriver == &bcm43362StaDriver)
      {
         //Save underlying network interface (STA mode)
         bcm43362StaInterface = interface;

         //Optionally set the station MAC address
         //if(macCompAddr(&interface->macAddr, &MAC_UNSPECIFIED_ADDR))
         {
            //Use the factory preprogrammed station address
            ret = wwd_wifi_get_mac_address((wiced_mac_t *) &interface->macAddr, WWD_STA_INTERFACE);
            TRACE_INFO("wwd_wifi_get_mac_address=%d (0x%04X)\r\n", ret, ret);
            TRACE_INFO("MAC=%s\r\n", macAddrToString(&interface->macAddr, NULL));

            //Generate the 64-bit interface identifier
            macAddrToEui64(&interface->macAddr, &interface->eui64);
         }
      }
      else
      {
         //Save underlying network interface (AP mode)
         bcm43362ApInterface = interface;

         //Optionally set the station MAC address
         //if(macCompAddr(&interface->macAddr, &MAC_UNSPECIFIED_ADDR))
         {
            //Use the factory preprogrammed station address
            ret = wwd_wifi_get_mac_address((wiced_mac_t *) &interface->macAddr, WWD_STA_INTERFACE);
            TRACE_INFO("wwd_wifi_get_mac_address=%d (0x%04X)\r\n", ret, ret);
            TRACE_INFO("MAC=%s\r\n", macAddrToString(&interface->macAddr, NULL));

            //Generate the 64-bit interface identifier
            macAddrToEui64(&interface->macAddr, &interface->eui64);
         }
      }

      //End of exception handling block
   } while(0);

   //BCM43362 is now ready to send
   osSetEvent(&interface->nicTxEvent);

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief BCM43362 timer handler
 *
 * This routine is periodically called by the TCP/IP stack to handle periodic
 * operations such as polling the link state
 *
 * @param[in] interface Underlying network interface
 **/

void bcm43362Tick(NetInterface *interface)
{
}


/**
 * @brief Enable interrupts
 * @param[in] interface Underlying network interface
 **/

void bcm43362EnableIrq(NetInterface *interface)
{
}


/**
 * @brief Disable interrupts
 * @param[in] interface Underlying network interface
 **/

void bcm43362DisableIrq(NetInterface *interface)
{
}


/**
 * @brief BCM43362 interrupt service routine
 * @return TRUE if a higher priority task must be woken. Else FALSE is returned
 **/

bool_t bcm43362IrqHandler(void)
{
   bool_t flag;

   //This flag will be set if a higher priority task must be woken
   flag = FALSE;

   //STA and/or AP mode?
   if(bcm43362StaInterface != NULL)
   {
      bcm43362StaInterface->nicEvent = TRUE;
   }
   else if(bcm43362ApInterface != NULL)
   {
      bcm43362ApInterface->nicEvent = TRUE;
   }

   //Notify the TCP/IP stack of the event
   flag = osSetEventFromIsr(&netEvent);

   //A higher priority task must be woken?
   return flag;
}


/**
 * @brief BCM43362 event handler
 * @param[in] interface Underlying network interface
 **/

void bcm43362EventHandler(NetInterface *interface)
{
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

error_t bcm43362SendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary)
{
   wwd_result_t ret;
   wiced_buffer_t packet;
   size_t length;
   uint8_t *p;

   //Retrieve the length of the packet
   length = netBufferGetLength(buffer) - offset;

   //Allocate a network buffer
   ret = host_buffer_get(&packet, WWD_NETWORK_TX, length + WICED_LINK_OVERHEAD_BELOW_ETHERNET_FRAME_MAX, FALSE);

   //Check status code
   if(ret == WWD_SUCCESS)
   {
      //Make room for additional headers
      host_buffer_add_remove_at_front(&packet, WICED_LINK_OVERHEAD_BELOW_ETHERNET_FRAME_MAX);

      //Point to the data payload
      p = host_buffer_get_current_piece_data_pointer(packet);

      //Copy user data
      netBufferRead(p, buffer, offset, length);

      //Adjust the length of the buffer
      host_buffer_set_size(packet, length);

      //STA or AP mode?
      if(interface == bcm43362StaInterface)
      {
         //Send packet
         wwd_network_send_ethernet_data(packet, WWD_STA_INTERFACE);
      }
      else
      {
         //Send packet
         wwd_network_send_ethernet_data(packet, WWD_AP_INTERFACE);
      }
   }
   else
   {
      TRACE_ERROR("##### bcm43362SendPacket ALLOC FAILED ####\r\n");
   }

   //The transmitter can accept another packet
   osSetEvent(&interface->nicTxEvent);

   //Return status code
   if(ret == WWD_SUCCESS)
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

error_t bcm43362UpdateMacAddrFilter(NetInterface *interface)
{
   uint_t i;
   wwd_result_t ret;
   MacFilterEntry *entry;

   //Debug message
   TRACE_INFO("Updating BCM43362 multicast filter...\r\n");

   //STA interface?
   if(interface == bcm43362StaInterface)
   {
      //The MAC address filter contains the list of MAC addresses to accept
      //when receiving an Ethernet frame
      for(i = 0; i < MAC_ADDR_FILTER_SIZE; i++)
      {
         //Point to the current entry
         entry = &interface->macAddrFilter[i];

         //Check whether the MAC filter table should be updated for the
         //current multicast address
         if(!macCompAddr(&entry->addr, &MAC_UNSPECIFIED_ADDR))
         {
            if(entry->addFlag)
            {
               //Add a new entry to the MAC filter table
               //ret = wwd_wifi_register_multicast_address((wiced_mac_t *) entry->addr.b);
               //TRACE_ERROR("wwd_wifi_register_multicast_address=%d (0x%04X)\r\n", ret, ret);
            }
            else if(entry->deleteFlag)
            {
               //Remove the current entry from the MAC filter table
               //ret = wwd_wifi_unregister_multicast_address((wiced_mac_t *) entry->addr.b);
               //TRACE_ERROR("wwd_wifi_unregister_multicast_address=%d (0x%04X)\r\n", ret, ret);
            }
         }
      }
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Callback function that handles Wi-Fi events
 **/

void *app_wifi_event_handler(const wwd_event_header_t *event_header, const uint8_t *event_data, void *handler_user_data)
{
   //Check event type
   switch(event_header->event_type)
   {
   //I/F change?
   case WLC_E_IF:
      TRACE_INFO("### app_wifi_event_handler: WLC_E_IF\r\n");
      break;

   //802.11 ASSOC indication?
   case WLC_E_ASSOC_IND:
      TRACE_INFO("### app_wifi_event_handler: WLC_E_ASSOC_IND\r\n");
      break;

   //802.11 DISASSOC indication?
   case WLC_E_DISASSOC_IND:
      TRACE_INFO("### app_wifi_event_handler: WLC_E_DISASSOC_IND\r\n");
      break;

   //Generic link indication?
   case WLC_E_LINK:
      //Debug message
      TRACE_INFO("### app_wifi_event_handler: WLC_E_LINK\r\n");

      //STA interface?
      if(event_header->interface == WWD_STA_INTERFACE)
      {
         if(bcm43362StaInterface != NULL)
         {
            //Check link state
            if((event_header->flags & 0x01) != 0)
            {
               bcm43362StaInterface->linkState = TRUE;
            }
            else
            {
               bcm43362StaInterface->linkState = FALSE;
            }

            //Get exclusive access
            osAcquireMutex(&netMutex);
            //Process link state change event
            nicNotifyLinkChange(bcm43362StaInterface);
            //Release exclusive access
            osReleaseMutex(&netMutex);
         }
      }
      //AP interface?
      else if(event_header->interface == WWD_AP_INTERFACE)
      {
         if(bcm43362ApInterface != NULL)
         {
            //Check link state
            if((event_header->flags & 0x01) != 0)
            {
               bcm43362ApInterface->linkState = TRUE;
            }
            else
            {
               bcm43362ApInterface->linkState = FALSE;
            }

            //Get exclusive access
            osAcquireMutex(&netMutex);
            //Process link state change event
            nicNotifyLinkChange(bcm43362ApInterface);
            //Release exclusive access
            osReleaseMutex(&netMutex);
         }
      }

      break;

   //Unknown event?
   default:
      TRACE_INFO("### app_wifi_event_handler: Unknown event\r\n");
      break;
   }

   return handler_user_data;
}


/**
 * @brief Callback function that handles incoming packets
 **/

void host_network_process_ethernet_data(wiced_buffer_t buffer, wwd_interface_t interface)
{
   size_t n;
   uint8_t *p;
   NetRxAncillary ancillary;

   //Point to the incoming packet
   p = host_buffer_get_current_piece_data_pointer(buffer);
   //Retrieve the length of the packet
   n = host_buffer_get_current_piece_size(buffer);

   //Valid packet received?
   if(p != NULL && n > 0)
   {
      if(interface == WWD_STA_INTERFACE)
      {
         if(bcm43362StaInterface != NULL)
         {
            //Get exclusive access
            osAcquireMutex(&netMutex);

            //Additional options can be passed to the stack along with the packet
            ancillary = NET_DEFAULT_RX_ANCILLARY;

            //Process link state change event
            nicProcessPacket(bcm43362StaInterface, p, n, &ancillary);

            //Release exclusive access
            osReleaseMutex(&netMutex);
         }
      }
      else if(interface == WWD_AP_INTERFACE)
      {
         if(bcm43362ApInterface != NULL)
         {
            //Get exclusive access
            osAcquireMutex(&netMutex);

            //Additional options can be passed to the stack along with the packet
            ancillary = NET_DEFAULT_RX_ANCILLARY;

            //Process link state change event
            nicProcessPacket(bcm43362ApInterface, p, n, &ancillary);

            //Release exclusive access
            osReleaseMutex(&netMutex);
         }
      }
   }

   //Release network buffer
   host_buffer_release(buffer, WWD_NETWORK_RX);
}


//Miscellaneous WICED dependencies
signed int xTaskIsTaskFinished(void *xTask)
{
   TRACE_INFO("### xTaskIsTaskFinished\r\n");
   return pdTRUE;
}

portBASE_TYPE vTaskFreeTerminated(TaskHandle_t xTask)
{
   TRACE_INFO("### vTaskFreeTerminated\r\n");
   return pdTRUE;
}
