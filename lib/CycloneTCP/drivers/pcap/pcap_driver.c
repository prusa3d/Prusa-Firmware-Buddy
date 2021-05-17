/**
 * @file pcap_driver.c
 * @brief PCAP driver
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
#include <stdlib.h>
#include "core/net.h"
#include "drivers/pcap/pcap_driver.h"
#include "debug.h"

//Undefine conflicting definitions
#undef Socket
#undef htons
#undef htonl
#undef ntohs
#undef ntohl

//PCAP dependencies
#include <pcap.h>

//Undefine conflicting definitions
#undef interface


/**
 * @brief Packet descriptor
 **/

typedef struct
{
   size_t length;
   uint8_t data[PCAP_DRIVER_MAX_PACKET_SIZE];
} PcapDriverPacket;


/**
 * @brief PCAP driver context
 **/

typedef struct
{
   pcap_t *handle;
   uint_t writeIndex;
   uint_t readIndex;
   PcapDriverPacket queue[PCAP_DRIVER_QUEUE_SIZE];
} PcapDriverContext;


/**
 * @brief PCAP driver
 **/

const NicDriver pcapDriver =
{
   NIC_TYPE_ETHERNET,
   ETH_MTU,
   pcapDriverInit,
   pcapDriverTick,
   pcapDriverEnableIrq,
   pcapDriverDisableIrq,
   pcapDriverEventHandler,
   pcapDriverSendPacket,
   pcapDriverUpdateMacAddrFilter,
   NULL,
   NULL,
   NULL,
   TRUE,
   TRUE,
   TRUE,
   TRUE
};


/**
 * @brief PCAP driver initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t pcapDriverInit(NetInterface *interface)
{
   int_t ret;
   uint_t i;
   uint_t j;
   pcap_if_t *device;
   pcap_if_t *deviceList;
   struct bpf_program filerCode;
   char_t filterExpr[256];
   char_t errorBuffer[PCAP_ERRBUF_SIZE];
   PcapDriverContext *context;
#if (NET_RTOS_SUPPORT == ENABLED)
   OsTask *task;
#endif

   //Debug message
   TRACE_INFO("Initializing PCAP driver...\r\n");

   //Allocate PCAP driver context
   context = (PcapDriverContext *) malloc(sizeof(PcapDriverContext));

   //Failed to allocate memory?
   if(context == NULL)
   {
      //Debug message
      printf("Failed to allocate context!\r\n");

      //Report an error
      return ERROR_FAILURE;
   }

   //Attach the PCAP driver context to the network interface
   *((PcapDriverContext **) interface->nicContext) = context;
   //Clear PCAP driver context
   osMemset(context, 0, sizeof(PcapDriverContext));

   //Find all the devices
   ret = pcap_findalldevs(&deviceList, errorBuffer);

   //Any error to report?
   if(ret != 0)
   {
      //Debug message
      printf("Failed to list devices!\r\n");

      //Clean up side effects
      free(context);

      //Report an error
      return ERROR_FAILURE;
   }

   //No network adapter found?
   if(deviceList == NULL)
   {
      //Debug message
      printf("No network adapter found!\r\n");

      //Clean up side effects
      free(context);

      //Exit immediately
      return ERROR_FAILURE;
   }

   //Network adapter selection
   while(1)
   {
      //Debug message
      printf("Network adapters:\r\n");

      //Point to the first device
      device = deviceList;
      i = 0;

      //Loop through the list of devices
      while(device != NULL)
      {
         //Index of the current network adapter
         printf("  %-2u", i + 1);

#if !defined(_WIN32)
         //Display the name of the device
         if(device->name != NULL)
         {
            printf(" %-8s", device->name);
         }
#endif
         //Description of the device
         if(device->description != NULL)
         {
            printf(" %s\r\n", device->description);
         }
         else
         {
            printf(" -\r\n");
         }

         //Next device
         device = device->next;
         i++;
      }

      //Display message
      printf("Select network adapter for %s interface (1-%u):", interface->name, i);
      //Get user choice
      scanf("%d", &j);

      //Valid selection?
      if(j >= 1 && j <= i)
      {
         break;
      }
   }

   //Point to the first device
   device = deviceList;

   //Point to the desired network adapter
   for(i = 1; i < j; i++)
   {
      device = device->next;
   }

   //Open the device
   context->handle = pcap_open_live(device->name, 65535,
      TRUE, PCAP_DRIVER_TIMEOUT, errorBuffer);

   //Failed to open device?
   if(context->handle == NULL)
   {
      //Debug message
      printf("Failed to open device!\r\n");

      //Clean up side effects
      pcap_freealldevs(deviceList);
      free(context);

      //Report an error
      return ERROR_FAILURE;
   }

   //Free the device list
   pcap_freealldevs(deviceList);

   //Filter expression
   osSprintf(filterExpr, "!(ether src %02x:%02x:%02x:%02x:%02x:%02x) && "
      "((ether dst %02x:%02x:%02x:%02x:%02x:%02x) || (ether broadcast) || (ether multicast))",
      interface->macAddr.b[0], interface->macAddr.b[1], interface->macAddr.b[2],
      interface->macAddr.b[3], interface->macAddr.b[4], interface->macAddr.b[5],
      interface->macAddr.b[0], interface->macAddr.b[1], interface->macAddr.b[2],
      interface->macAddr.b[3], interface->macAddr.b[4], interface->macAddr.b[5]);

   //Compile the filter
   ret = pcap_compile(context->handle, &filerCode, filterExpr, 1, 0);

   //Failed to open device?
   if(ret != 0)
   {
      //Debug message
      printf("Failed to compile filter!\r\n");

      //Clean up side effects
      pcap_close(context->handle);
      free(context);

      //Report an error
      return ERROR_FAILURE;
   }

   //Set the filter
   ret = pcap_setfilter(context->handle, &filerCode);

   //Failed to open device?
   if(ret != 0)
   {
      //Debug message
      printf("Failed to set filter!\r\n");

      //Clean up side effects
      pcap_close(context->handle);
      free(context);

      //Report an error
      return ERROR_FAILURE;
   }

#if (NET_RTOS_SUPPORT == ENABLED)
   //Create the receive task
   task = osCreateTask("PCAP", (OsTaskCode) pcapDriverTask, interface, 0, 0);

   //Failed to create the task?
   if(task == OS_INVALID_HANDLE)
   {
      //Debug message
      printf("Failed to create task!\r\n");

      //Clean up side effects
      pcap_close(context->handle);
      free(context);

      //Report an error
      return ERROR_FAILURE;
   }
#endif

   //Accept any packets from the upper layer
   osSetEvent(&interface->nicTxEvent);

   //Return status code
   return NO_ERROR;
}


/**
 * @brief PCAP timer handler
 *
 * This routine is periodically called by the TCP/IP stack to handle periodic
 * operations such as polling the link state
 *
 * @param[in] interface Underlying network interface
 **/

void pcapDriverTick(NetInterface *interface)
{
   //Not implemented
}


/**
 * @brief Enable interrupts
 * @param[in] interface Underlying network interface
 **/

void pcapDriverEnableIrq(NetInterface *interface)
{
   //Not implemented
}


/**
 * @brief Disable interrupts
 * @param[in] interface Underlying network interface
 **/

void pcapDriverDisableIrq(NetInterface *interface)
{
   //Not implemented
}


/**
 * @brief PCAP event handler
 * @param[in] interface Underlying network interface
 **/

void pcapDriverEventHandler(NetInterface *interface)
{
   uint_t n;
   PcapDriverContext *context;
   NetRxAncillary ancillary;

   //Point to the PCAP driver context
   context = *((PcapDriverContext **) interface->nicContext);

   //Process all pending packets
   while(context->queue[context->readIndex].length > 0)
   {
      //Additional options can be passed to the stack along with the packet
      ancillary = NET_DEFAULT_RX_ANCILLARY;

      //Pass the packet to the upper layer
      nicProcessPacket(interface, context->queue[context->readIndex].data,
         context->queue[context->readIndex].length, &ancillary);

      //Compute the index of the next packet descriptor
      n = (context->readIndex + 1) % PCAP_DRIVER_QUEUE_SIZE;

      //Release the current packet
      context->queue[context->readIndex].length = 0;
      //Point to the next packet descriptor
      context->readIndex = n;
   }
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

error_t pcapDriverSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary)
{
   int_t ret;
   size_t length;
   PcapDriverContext *context;
   uint8_t temp[PCAP_DRIVER_MAX_PACKET_SIZE];

   //Point to the PCAP driver context
   context = *((PcapDriverContext **) interface->nicContext);

   //Retrieve the length of the packet
   length = netBufferGetLength(buffer) - offset;

   //Check the frame length
   if(length > PCAP_DRIVER_MAX_PACKET_SIZE)
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
      //Report an error
      return ERROR_INVALID_LENGTH;
   }

   //Copy the packet to the transmit buffer
   netBufferRead(temp, buffer, offset, length);

   //Send packet
   ret = pcap_sendpacket(context->handle, temp, length);

   //The transmitter can accept another packet
   osSetEvent(&interface->nicTxEvent);

   //Return status code
   if(ret < 0)
   {
      return ERROR_FAILURE;
   }
   else
   {
      return NO_ERROR;
   }
}


/**
 * @brief Configure MAC address filtering
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t pcapDriverUpdateMacAddrFilter(NetInterface *interface)
{
   //Not implemented
   return NO_ERROR;
}


/**
 * @brief PCAP receive task
 * @param[in] interface Underlying network interface
 **/

void pcapDriverTask(NetInterface *interface)
{
   int_t ret;
   uint_t n;
   uint_t length;
   const uint8_t *data;
   struct pcap_pkthdr *header;
   PcapDriverContext *context;

   //Point to the PCAP driver context
   context = *((PcapDriverContext **) interface->nicContext);

   //Process events
   while(1)
   {
      //Wait for an incoming packet
      ret = pcap_next_ex(context->handle, &header, &data);

      //Any packet received?
      if(ret > 0)
      {
         //Retrieve the length of the packet
         length = header->caplen;

         //Check the length of the received packet
         if(length > 0 && length < PCAP_DRIVER_MAX_PACKET_SIZE)
         {
            //Check whether the link is up
            if(interface->linkState)
            {
               //Compute the index of the next packet descriptor
               n = (context->writeIndex + 1) % PCAP_DRIVER_QUEUE_SIZE;

               //Ensure the receive queue is not full
               if(n != context->readIndex)
               {
                  //Copy the incoming packet
                  osMemcpy(context->queue[context->writeIndex].data, data, length);
                  //Save the length of the packet
                  context->queue[context->writeIndex].length = length;

                  //Point to the next packet descriptor
                  context->writeIndex = n;

                  //Set event flag
                  interface->nicEvent = TRUE;
                  //Notify the TCP/IP stack of the event
                  osSetEvent(&netEvent);
               }
            }
         }
      }
      else
      {
#if (NET_RTOS_SUPPORT == DISABLED)
         //No packet has been received
         break;
#endif
      }
   }
}
