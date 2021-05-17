/**
 * @file wf200_driver.c
 * @brief WF200 Wi-Fi controller
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
#include "sl_wfx.h"
#include "sl_wfx_host.h"
#include "sl_wfx_host_api.h"
#include "core/net.h"
#include "drivers/wifi/wf200_driver.h"
#include "debug.h"

//Underlying network interface
static NetInterface *wf200StaInterface = NULL;
static NetInterface *wf200ApInterface = NULL;

//WF200 context
static sl_wfx_context_t wf200Context;

//Forward declaration of functions
sl_status_t sl_wfx_host_init_pins(void);


/**
 * @brief WF200 driver (STA mode)
 **/

const NicDriver wf200StaDriver =
{
   NIC_TYPE_ETHERNET,
   ETH_MTU,
   wf200Init,
   wf200Tick,
   wf200EnableIrq,
   wf200DisableIrq,
   wf200EventHandler,
   wf200SendPacket,
   wf200UpdateMacAddrFilter,
   NULL,
   NULL,
   NULL,
   TRUE,
   TRUE,
   TRUE,
   TRUE
};


/**
 * @brief WF200 driver (AP mode)
 **/

const NicDriver wf200ApDriver =
{
   NIC_TYPE_ETHERNET,
   ETH_MTU,
   wf200Init,
   wf200Tick,
   wf200EnableIrq,
   wf200DisableIrq,
   wf200EventHandler,
   wf200SendPacket,
   wf200UpdateMacAddrFilter,
   NULL,
   NULL,
   NULL,
   TRUE,
   TRUE,
   TRUE,
   TRUE
};


/**
 * @brief WF200 initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t wf200Init(NetInterface *interface)
{
   sl_status_t status;
   sl_wfx_mac_address_t macAddr;

   //STA or AP mode?
   if(interface->nicDriver == &wf200StaDriver)
   {
      //Debug message
      TRACE_INFO("Initializing WF200 (STA mode)...\r\n");
      //Save underlying network interface
      wf200StaInterface = interface;
   }
   else
   {
      //Debug message
      TRACE_INFO("Initializing WF200 (AP mode)...\r\n");
      //Save underlying network interface
      wf200ApInterface = interface;
   }

   //Initialization sequence is performed once
   if(wf200StaInterface == NULL || wf200ApInterface == NULL)
   {
      //Initialize WF200 pins
      sl_wfx_host_init_pins();

      //Initialize WF200 controller
      status = sl_wfx_init(&wf200Context);
   }
   else
   {
      //Initialization was already done
      status = SL_STATUS_OK;
   }

   //Check status code
   if(status == SL_STATUS_OK)
   {
      //Optionally set the MAC address
      if(macCompAddr(&interface->macAddr, &MAC_UNSPECIFIED_ADDR))
      {
         //Use the factory preprogrammed MAC address
         if(interface == wf200StaInterface)
         {
            macCopyAddr(&interface->macAddr, wf200Context.mac_addr_0.octet);
         }
         else
         {
            macCopyAddr(&interface->macAddr, wf200Context.mac_addr_1.octet);
         }

         //Generate the 64-bit interface identifier
         macAddrToEui64(&interface->macAddr, &interface->eui64);
      }
      else
      {
         //Override the factory preprogrammed address
         macCopyAddr(&macAddr, &interface->macAddr);

         //Assign MAC addresses
         if(interface == wf200StaInterface)
         {
            status = sl_wfx_set_mac_address(&macAddr, SL_WFX_STA_INTERFACE);
         }
         else
         {
            status = sl_wfx_set_mac_address(&macAddr, SL_WFX_SOFTAP_INTERFACE);
         }
      }
   }

   //WF200 is now ready to send
   osSetEvent(&interface->nicTxEvent);

   //Return status code
   if(status == SL_STATUS_OK)
   {
      return NO_ERROR;
   }
   else
   {
      return ERROR_FAILURE;
   }
}


/**
 * @brief WF200 timer handler
 *
 * This routine is periodically called by the TCP/IP stack to handle periodic
 * operations such as polling the link state
 *
 * @param[in] interface Underlying network interface
 **/

void wf200Tick(NetInterface *interface)
{
}


/**
 * @brief Enable interrupts
 * @param[in] interface Underlying network interface
 **/

void wf200EnableIrq(NetInterface *interface)
{
}


/**
 * @brief Disable interrupts
 * @param[in] interface Underlying network interface
 **/

void wf200DisableIrq(NetInterface *interface)
{
}


/**
 * @brief WF200 event handler
 * @param[in] interface Underlying network interface
 **/

void wf200EventHandler(NetInterface *interface)
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

error_t wf200SendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary)
{
   size_t n;
   size_t length;
   sl_status_t status;
   sl_wfx_send_frame_req_t *req;

   //Retrieve the length of the packet
   length = netBufferGetLength(buffer) - offset;

   //Make sure the link is up before transmitting the frame
   if(!interface->linkState)
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
      //Drop current packet
      return NO_ERROR;
   }

   //Get the length of the packet
   n = SL_WFX_ROUND_UP(length, 2);

   //Allocate a memory buffer to hold the request
   sl_wfx_allocate_command_buffer((sl_wfx_generic_message_t **) &req,
      SL_WFX_SEND_FRAME_REQ_ID, SL_WFX_TX_FRAME_BUFFER,
      n + sizeof(sl_wfx_send_frame_req_t));

   //Successful memory allocation?
   if(req != NULL)
   {
      //Copy user data to the transmit buffer
      netBufferRead(req->body.packet_data, buffer, offset, length);

      //Send packet
      if(interface == wf200StaInterface)
      {
         status = sl_wfx_send_ethernet_frame(req, length, SL_WFX_STA_INTERFACE, 0);
      }
      else
      {
         status = sl_wfx_send_ethernet_frame(req, length, SL_WFX_SOFTAP_INTERFACE, 0);
      }

      //Release previously allocated memory
      sl_wfx_free_command_buffer((sl_wfx_generic_message_t *) req,
         SL_WFX_SEND_FRAME_REQ_ID, SL_WFX_TX_FRAME_BUFFER);
   }
   else
   {
      //Failed to allocate memory
      status = SL_STATUS_ALLOCATION_FAILED;
   }

   //The transmitter can accept another packet
   osSetEvent(&interface->nicTxEvent);

   //Return status code
   if(status == SL_STATUS_OK)
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

error_t wf200UpdateMacAddrFilter(NetInterface *interface)
{
   uint_t i;
   MacFilterEntry *entry;
   sl_status_t status;
   sl_wfx_mac_address_t macAddr;

   //Debug message
   TRACE_INFO("Updating WF200 multicast filter...\r\n");

   //Initialize status code
   status = SL_STATUS_OK;

   //The MAC address filter contains the list of MAC addresses to accept
   //when receiving an Ethernet frame
   for(i = 0; i < MAC_ADDR_FILTER_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->macAddrFilter[i];

      //Valid entry?
      if(!macCompAddr(&entry->addr, &MAC_UNSPECIFIED_ADDR))
      {
         //Copy the MAC address
         macCopyAddr(&macAddr, &entry->addr);

         //Update MAC filter table only if necessary
         if(entry->addFlag)
         {
            //Add a new entry to the MAC filter table
            if(interface == wf200StaInterface)
            {
               status = sl_wfx_add_multicast_address(&macAddr,
                  SL_WFX_STA_INTERFACE);
            }
            else
            {
               status = sl_wfx_add_multicast_address(&macAddr,
                  SL_WFX_SOFTAP_INTERFACE);
            }
         }
         else if(entry->deleteFlag)
         {
            //Remove the current entry from the MAC filter table
            if(interface == wf200StaInterface)
            {
               status = sl_wfx_remove_multicast_address(&macAddr,
                  SL_WFX_STA_INTERFACE);
            }
            else
            {
               status = sl_wfx_remove_multicast_address(&macAddr,
                  SL_WFX_SOFTAP_INTERFACE);
            }
         }
      }
   }

   //Return status code
   if(status == SL_STATUS_OK)
   {
      return NO_ERROR;
   }
   else
   {
      return ERROR_FAILURE;
   }
}


/**
 * @brief Station connected callback
 * @param[in] event Event information
 * @return Error code
 **/

void wf200ConnectCallback(void)
{
   //Valid STA interface?
   if(wf200StaInterface != NULL)
   {
      //Link is up
      wf200StaInterface->linkState = TRUE;

      //Get exclusive access
      osAcquireMutex(&netMutex);
      //Process link state change event
      nicNotifyLinkChange(wf200StaInterface);
      //Release exclusive access
      osReleaseMutex(&netMutex);
   }
}


/**
 * @brief Station disconnected callback
 * @param[in] event Event information
 * @return Error code
 **/

void wf200DisconnectCallback(void)
{
   //Valid STA interface?
   if(wf200StaInterface != NULL)
   {
      //Link is down
      wf200StaInterface->linkState = FALSE;

      //Get exclusive access
      osAcquireMutex(&netMutex);
      //Process link state change event
      nicNotifyLinkChange(wf200StaInterface);
      //Release exclusive access
      osReleaseMutex(&netMutex);
   }
}


/**
 * @brief Station connected callback
 * @param[in] event Event information
 * @return Error code
 **/

void wf200StartApCallback(void)
{
   //Valid AP interface?
   if(wf200ApInterface != NULL)
   {
      //Link is up
      wf200ApInterface->linkState = TRUE;

      //Get exclusive access
      osAcquireMutex(&netMutex);
      //Process link state change event
      nicNotifyLinkChange(wf200ApInterface);
      //Release exclusive access
      osReleaseMutex(&netMutex);
   }
}


/**
 * @brief Station disconnected callback
 * @param[in] event Event information
 * @return Error code
 **/

void wf200StopApCallback(void)
{
   //Valid AP interface?
   if(wf200ApInterface != NULL)
   {
      //Link is down
      wf200ApInterface->linkState = FALSE;

      //Get exclusive access
      osAcquireMutex(&netMutex);
      //Process link state change event
      nicNotifyLinkChange(wf200ApInterface);
      //Release exclusive access
      osReleaseMutex(&netMutex);
   }
}


/**
 * @brief Callback function that handles incoming packets
 * @param[in] ind Pointer to the received indication
 **/

void sl_wfx_host_received_frame_callback(sl_wfx_received_ind_t *ind)
{
   NetInterface *interface;
   NetRxAncillary ancillary;

   //STA or AP interface?
   if((ind->header.info & SL_WFX_MSG_INFO_INTERFACE_MASK) ==
      (SL_WFX_STA_INTERFACE << SL_WFX_MSG_INFO_INTERFACE_OFFSET))
   {
      interface = wf200StaInterface;
   }
   else
   {
      interface = wf200ApInterface;
   }

   //Valid interface?
   if(interface != NULL)
   {
      //Get exclusive access
      osAcquireMutex(&netMutex);

      //Additional options can be passed to the stack along with the packet
      ancillary = NET_DEFAULT_RX_ANCILLARY;

      //Pass the packet to the upper layer
      nicProcessPacket(interface, ind->body.frame + ind->body.frame_padding,
         ind->body.frame_length, &ancillary);

      //Release exclusive access
      osReleaseMutex(&netMutex);
   }
}
