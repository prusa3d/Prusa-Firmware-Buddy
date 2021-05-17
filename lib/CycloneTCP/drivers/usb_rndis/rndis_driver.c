/**
 * @file rndis_driver.c
 * @brief RNDIS driver
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
#include "usbd_def.h"
#include "usbd_rndis.h"
#include "core/net.h"
#include "rndis.h"
#include "rndis_driver.h"
#include "rndis_debug.h"
#include "debug.h"

//Underlying network interface
NetInterface *rndisDriverInterface;

//TX and RX buffers
RndisTxBufferDesc rndisTxBuffer[RNDIS_TX_BUFFER_COUNT];
RndisRxBufferDesc rndisRxBuffer[RNDIS_RX_BUFFER_COUNT];

//Buffer indexes
uint_t rndisTxWriteIndex;
uint_t rndisTxReadIndex;
uint_t rndisRxWriteIndex;
uint_t rndisRxReadIndex;


/**
 * @brief RNDIS driver
 **/

const NicDriver rndisDriver =
{
   NIC_TYPE_ETHERNET,
   ETH_MTU,
   rndisDriverInit,
   rndisDriverTick,
   rndisDriverEnableIrq,
   rndisDriverDisableIrq,
   rndisDriverEventHandler,
   rndisDriverSendPacket,
   rndisDriverSetMulticastFilter,
   NULL,
   NULL,
   NULL,
   TRUE,
   TRUE,
   TRUE,
   TRUE
};


/**
 * @brief RNDIS driver initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t rndisDriverInit(NetInterface *interface)
{
   //Debug message
   TRACE_INFO("Initializing RNDIS driver...\r\n");

   //Save underlying network interface
   rndisDriverInterface = interface;

   //Clear TX and RX buffers
   osMemset(rndisTxBuffer, 0, sizeof(rndisTxBuffer));
   osMemset(rndisRxBuffer, 0, sizeof(rndisRxBuffer));

   //Initialize variables
   rndisTxWriteIndex = 0;
   rndisTxReadIndex = 0;
   rndisRxWriteIndex = 0;
   rndisRxReadIndex = 0;

   //The RNDIS driver is now ready to send
   osSetEvent(&interface->nicTxEvent);

   //Force the TCP/IP stack to check the link state
   rndisContext.linkEvent = TRUE;
   interface->nicEvent = TRUE;
   osSetEvent(&netEvent);

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief RNDIS driver timer handler
 *
 * This routine is periodically called by the TCP/IP stack to handle periodic
 * operations such as polling the link state
 *
 * @param[in] interface Underlying network interface
 **/

void rndisDriverTick(NetInterface *interface)
{
   //Not implemented
}


/**
 * @brief Enable interrupts
 * @param[in] interface Underlying network interface
 **/

void rndisDriverEnableIrq(NetInterface *interface)
{
   //Enable OTG_FS interrupts
   NVIC_EnableIRQ(OTG_FS_IRQn);
}


/**
 * @brief Disable interrupts
 * @param[in] interface Underlying network interface
 **/

void rndisDriverDisableIrq(NetInterface *interface)
{
   //Disable OTG_FS interrupts
   NVIC_DisableIRQ(OTG_FS_IRQn);
}


/**
 * @brief RNDIS driver event handler
 * @param[in] interface Underlying network interface
 **/

void rndisDriverEventHandler(NetInterface *interface)
{
   static uint8_t temp[ETH_MAX_FRAME_SIZE];
   error_t error;
   size_t length;

   //Link up/down event is pending?
   if(rndisContext.linkEvent)
   {
      //Clear event flag
      rndisContext.linkEvent = FALSE;

      //Check link state
      if(rndisContext.linkState)
      {
         //Link is up
         interface->linkState = TRUE;
         //Link speed
         interface->linkSpeed = 12000000;
      }
      else
      {
         //Link is down
         interface->linkState = FALSE;
      }

      //Process link state change event
      nicNotifyLinkChange(interface);
   }

   //Process all pending packets
   do
   {
      //Read incoming packet
      error = rndisDriverReceivePacket(interface,
         temp, ETH_MAX_FRAME_SIZE, &length);

      //Check whether a valid packet has been received
      if(!error)
      {
         NetRxAncillary ancillary;

         //Additional options can be passed to the stack along with the packet
         ancillary = NET_DEFAULT_RX_ANCILLARY;

         //Pass the packet to the upper layer
         nicProcessPacket(interface, temp, length, &ancillary);
      }

      //No more data in the receive buffer?
   } while(error != ERROR_BUFFER_EMPTY);
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

error_t rndisDriverSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary)
{
   size_t length;
   RndisPacketMsg *message;

   //Retrieve the length of the packet
   length = netBufferGetLength(buffer) - offset;

   //Check the frame length
   if((length + sizeof(RndisPacketMsg)) > RNDIS_TX_BUFFER_SIZE)
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
      //Report an error
      return ERROR_INVALID_LENGTH;
   }

   //Make sure the current buffer is available for writing
   if(rndisTxBuffer[rndisTxWriteIndex].ready)
      return ERROR_FAILURE;

   //Point to the buffer where to format the RNDIS Packet message
   message = (RndisPacketMsg *) rndisTxBuffer[rndisTxWriteIndex].data;

   //Format the RNDIS Packet message
   message->messageType = RNDIS_PACKET_MSG;
   message->messageLength = sizeof(RndisPacketMsg) + length;
   message->dataOffset = sizeof(RndisPacketMsg) - 8;
   message->dataLength = length;
   message->oobDataOffset = 0;
   message->oobDataLength = 0;
   message->numOobDataElements = 0;
   message->perPacketInfoOffset = 0;
   message->perPacketInfoLength = 0;
   message->vcHandle = 0;
   message->reserved = 0;

   //Copy user data to the transmit buffer
   netBufferRead(message->payload, buffer, offset, length);

   //Debug message
   TRACE_DEBUG("Sending RNDIS Packet message (%" PRIuSIZE " bytes)...\r\n",
      message->messageLength);
   //Dump RNDIS Packet message contents
   rndisDumpMsg((RndisMsg *) message, message->messageLength);

   //Check whether the RNDIS Packet message ends with a USB packet whose
   //length is exactly the wMaxPacketSize for the DATA IN endpoint
   if((message->messageLength % RNDIS_DATA_IN_EP_MPS_FS) == 0)
   {
      //The device may send an additional one-byte zero packet
      message->payload[length] = 0;
      message->messageLength++;
   }

   //Set the number of bytes to send
   rndisTxBuffer[rndisTxWriteIndex].length = message->messageLength;

   //Give the ownership of the buffer to the USB engine
   rndisTxBuffer[rndisTxWriteIndex].ready = TRUE;

   //Debug message
   TRACE_DEBUG("########## Sending DATA IN\r\n");

   //Transmission is currently suspended?
   if(!rndisContext.txState)
   {
      //Start transmitting data
      USBD_LL_Transmit(&USBD_Device, RNDIS_DATA_IN_EP,
         rndisTxBuffer[rndisTxReadIndex].data,
         rndisTxBuffer[rndisTxReadIndex].length);

      //Transmission is active
      rndisContext.txState = TRUE;
   }

   //Increment index and wrap around if necessary
   if(++rndisTxWriteIndex >= RNDIS_TX_BUFFER_COUNT)
      rndisTxWriteIndex = 0;

   //Check whether the next buffer is available for writing
   if(!rndisTxBuffer[rndisTxWriteIndex].ready)
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
   }

   //Data successfully written
   return NO_ERROR;
}


/**
 * @brief Receive a packet
 * @param[in] interface Underlying network interface
 * @param[out] buffer Buffer where to store the incoming data
 * @param[in] size Maximum number of bytes that can be received
 * @param[out] length Number of bytes that have been received
 * @return Error code
 **/

error_t rndisDriverReceivePacket(NetInterface *interface,
   uint8_t *buffer, size_t size, size_t *length)
{
   error_t error;
   size_t n;
   RndisPacketMsg *message;

   //Check whether the current buffer is available for reading
   if(rndisRxBuffer[rndisRxReadIndex].ready)
   {
      //Retrieve the length of the RNDIS Packet message
      n = rndisRxBuffer[rndisRxReadIndex].length;

      //Check the length of message
      if(n >= sizeof(RndisPacketMsg))
      {
         //Point to the RNDIS Packet message
         message = (RndisPacketMsg *) rndisRxBuffer[rndisRxReadIndex].data;

         //Make sure the message is valid
         if((message->dataOffset + message->dataLength) <= n)
         {
            //Limit the number of data to read
            n = MIN(message->dataLength, size);
            //Copy data from the receive buffer
            osMemcpy(buffer, (uint8_t *) message + message->dataOffset + 8, n);

            //Total number of bytes that have been received
            *length = n;
            //Packet successfully received
            error = NO_ERROR;
         }
         else
         {
            //Invalid message
            error = ERROR_INVALID_MESSAGE;
         }
      }
      else
      {
         //Invalid message
         error = ERROR_INVALID_MESSAGE;
      }

      //Reset the length field
      rndisRxBuffer[rndisRxReadIndex].length = 0;
      //Give the ownership of the buffer to the USB engine
      rndisRxBuffer[rndisRxReadIndex].ready = FALSE;

      //Increment index and wrap around if necessary
      if(++rndisRxReadIndex >= RNDIS_RX_BUFFER_COUNT)
         rndisRxReadIndex = 0;

      //Reception is currently suspended?
      if(!rndisContext.rxState)
      {
         //Debug message
         TRACE_DEBUG("### usbdRndisReceivePacket 111 ###\r\n");

         //Prepare DATA OUT endpoint for reception
         USBD_LL_PrepareReceive(&USBD_Device, RNDIS_DATA_OUT_EP,
            rndisContext.rxBuffer, RNDIS_DATA_OUT_EP_MPS_FS);

         //Reception is active
         rndisContext.rxState = TRUE;
      }
   }
   else
   {
      //No more data in the receive buffer
      error = ERROR_BUFFER_EMPTY;
   }

   //Return status code
   return error;
}


/**
 * @brief Configure multicast MAC address filtering
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t rndisDriverSetMulticastFilter(NetInterface *interface)
{
   //Successful processing
   return NO_ERROR;
}
