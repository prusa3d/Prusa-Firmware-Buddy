/**
 * @file ppp_hdlc.c
 * @brief PPP HDLC driver
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
#include <stdio.h>
#include "core/net.h"
#include "ppp/ppp.h"
#include "ppp/ppp_hdlc.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (PPP_SUPPORT == ENABLED)


/**
 * @brief PPP HDLC driver
 **/

const NicDriver pppHdlcDriver =
{
   NIC_TYPE_PPP,
   PPP_DEFAULT_MRU,
   pppHdlcDriverInit,
   pppHdlcDriverTick,
   pppHdlcDriverEnableIrq,
   pppHdlcDriverDisableIrq,
   pppHdlcDriverEventHandler,
   pppHdlcDriverSendPacket,
   pppHdlcDriverUpdateMacAddrFilter,
   NULL,
   NULL,
   NULL,
   FALSE,
   FALSE,
   FALSE,
   FALSE
};


/**
 * @brief PPP HDLC driver initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t pppHdlcDriverInit(NetInterface *interface)
{
   PppContext *context;

   //Debug message
   TRACE_INFO("Initializing PPP HDLC driver...\r\n");

   //Point to the PPP context
   context = interface->pppContext;

   //Initialize variables
   context->txBufferLen = 0;
   context->txWriteIndex = 0;
   context->txReadIndex = 0;
   context->rxBufferLen = 0;
   context->rxWriteIndex = 0;
   context->rxReadIndex = 0;
   context->rxFrameCount = 0;

   //Initialize UART
   interface->uartDriver->init();

   //Accept any packets from the upper layer
   osSetEvent(&interface->nicTxEvent);

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief PPP HDLC driver timer handler
 *
 * This routine is periodically called by the TCP/IP stack to
 * handle periodic operations such as polling the link state
 *
 * @param[in] interface Underlying network interface
 **/

void pppHdlcDriverTick(NetInterface *interface)
{
}


/**
 * @brief Enable interrupts
 * @param[in] interface Underlying network interface
 **/

void pppHdlcDriverEnableIrq(NetInterface *interface)
{
   //Enable UART interrupts
   interface->uartDriver->enableIrq();
}


/**
 * @brief Disable interrupts
 * @param[in] interface Underlying network interface
 **/

void pppHdlcDriverDisableIrq(NetInterface *interface)
{
   //USART interrupts are always enabled
}


/**
 * @brief PPP HDLC driver event handler
 * @param[in] interface Underlying network interface
 **/

void pppHdlcDriverEventHandler(NetInterface *interface)
{
   PppContext *context;

   //Point to the PPP context
   context = interface->pppContext;

   //Check PPP state
   if(interface->pppContext->pppPhase != PPP_PHASE_DEAD)
   {
      //Process all pending packets
      while(context->rxFrameCount > 0)
      {
         //Read incoming packet
         pppHdlcDriverReceivePacket(interface);

         //Enter critical section
         __disable_irq();
         //Decrement frame counter
         context->rxFrameCount--;
         //Exit critical section
         __enable_irq();
      }
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

error_t pppHdlcDriverSendPacket(NetInterface *interface,
   const NetBuffer *buffer, size_t offset, NetTxAncillary *ancillary)
{
   uint_t i;
   size_t j;
   size_t n;
   uint8_t *p;
   uint16_t protocol;
   uint32_t accm;
   PppContext *context;

   //Point to the PPP context
   context = interface->pppContext;

   //Point to the beginning of the frame
   p = netBufferAt(buffer, offset);

   //Parse the PPP frame header
   pppParseFrameHeader(p, PPP_FRAME_HEADER_SIZE, &protocol);

   //Check Protocol field
   if(protocol == PPP_PROTOCOL_IP || protocol == PPP_PROTOCOL_IPV6)
   {
      //Use the ACCM value that has been negotiated
      accm = context->peerConfig.accm;
   }
   else
   {
      //Use default ACCM mapping
      accm = PPP_DEFAULT_ACCM;
   }

   //Send flag
   pppHdlcDriverWriteTxQueue(context, PPP_FLAG_CHAR);

   //Loop through data chunks
   for(i = 0; i < buffer->chunkCount; i++)
   {
      //Is there any data to copy from the current chunk?
      if(offset < buffer->chunk[i].length)
      {
         //Point to the first byte to be read
         p = (uint8_t *) buffer->chunk[i].address + offset;
         //Compute the number of bytes to copy at a time
         n = buffer->chunk[i].length - offset;

         //Copy data to TX queue
         for(j = 0; j < n; j++)
         {
            if(p[j] < PPP_MASK_CHAR)
            {
               //Check whether the character is flagged
               if(accm & (1 << p[j]))
               {
                  pppHdlcDriverWriteTxQueue(context, PPP_ESC_CHAR);
                  pppHdlcDriverWriteTxQueue(context, p[j] ^ PPP_MASK_CHAR);
               }
               else
               {
                  //Enqueue current character
                  pppHdlcDriverWriteTxQueue(context, p[j]);
               }
            }
            else if(p[j] == PPP_ESC_CHAR || p[j] == PPP_FLAG_CHAR)
            {
               pppHdlcDriverWriteTxQueue(context, PPP_ESC_CHAR);
               pppHdlcDriverWriteTxQueue(context, p[j] ^ PPP_MASK_CHAR);
            }
            else
            {
               //Enqueue current character
               pppHdlcDriverWriteTxQueue(context, p[j]);
            }
         }

         //Process the next block from the start
         offset = 0;
      }
      else
      {
         //Skip the current chunk
         offset -= buffer->chunk[i].length;
      }
   }

   //Send flag
   pppHdlcDriverWriteTxQueue(context, PPP_FLAG_CHAR);

   //Start transferring data
   interface->uartDriver->startTx();

   //Check whether the TX queue is available for writing
   if(context->txBufferLen <= (PPP_TX_BUFFER_SIZE - 3006))
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
 * @return Error code
 **/

error_t pppHdlcDriverReceivePacket(NetInterface *interface)
{
   size_t n;
   uint8_t c;
   bool_t escFlag;
   uint32_t accm;
   PppContext *context;

   //Point to the PPP context
   context = interface->pppContext;
   //Retrieve ACCM
   accm = context->localConfig.accm;

   //Length of the original PPP frame
   n = 0;
   //This flag tells whether the next character is escaped
   escFlag = FALSE;

   //The receiver must reverse the octet stuffing procedure
   while(n < PPP_MAX_FRAME_SIZE && context->rxBufferLen > 0)
   {
      //Read a single character
      c = pppHdlcDriverReadRxQueue(context);

      if(c < PPP_MASK_CHAR)
      {
         //Check whether the character is flagged
         if(accm & (1 << c))
         {
            //The extra characters must be removed from the incoming data stream
         }
         else
         {
            //Copy current character
            context->frame[n++] = c;
         }
      }
      else if(c == PPP_ESC_CHAR)
      {
         //All occurrences of 0x7D indicate that the next character is escaped
         escFlag = TRUE;
      }
      else if(c == PPP_FLAG_CHAR)
      {
         //0x7E flag found
         break;
      }
      else if(escFlag)
      {
         //The character is XOR'ed with 0x20
         context->frame[n++] = c ^ PPP_MASK_CHAR;
         escFlag = FALSE;
      }
      else
      {
         //Copy current character
         context->frame[n++] = c;
      }
   }

   //Check whether a valid PPP frame has been received
   if(n > 0)
   {
      NetRxAncillary ancillary;

      //Debug message
      TRACE_DEBUG("PPP frame received (%" PRIuSIZE " bytes)...\r\n", n);
      TRACE_DEBUG_ARRAY("  ", context->frame, n);

      //Additional options can be passed to the stack along with the packet
      ancillary = NET_DEFAULT_RX_ANCILLARY;

      //Pass the packet to the upper layer
      nicProcessPacket(interface, context->frame, n, &ancillary);
   }

   //Successful read operation
   return NO_ERROR;
}


/**
 * @brief Configure MAC address filtering
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t pppHdlcDriverUpdateMacAddrFilter(NetInterface *interface)
{
   //Not implemented
   return NO_ERROR;
}


/**
 * @brief Send AT command
 * @param[in] interface Underlying network interface
 * @param[in] data NULL-terminated string that contains the AT command to be sent
 * @return Error code
 **/

error_t pppHdlcDriverSendAtCommand(NetInterface *interface, const char_t *data)
{
   size_t i;
   PppContext *context;

   //Point to the PPP context
   context = interface->pppContext;

   //Send AT command
   for(i = 0; data[i] != '\0' && i < 3006; i++)
      pppHdlcDriverWriteTxQueue(context, data[i]);

   //Start transferring data
   interface->uartDriver->startTx();

   //Check whether the TX queue is available for writing
   if(context->txBufferLen <= (PPP_TX_BUFFER_SIZE - 3006))
   {
      //The transmitter can accept another packet
      osSetEvent(&interface->nicTxEvent);
   }

   //Data successfully written
   return NO_ERROR;
}


/**
 * @brief Wait for an incoming AT command
 * @param[in] interface Underlying network interface
 * @param[out] data Buffer where to store the incoming AT command
 * @param[in] size Size of the buffer, in bytes
 * @return Error code
 **/

error_t pppHdlcDriverReceiveAtCommand(NetInterface *interface, char_t *data,
   size_t size)
{
   uint_t i;
   uint_t k;
   uint_t n;
   bool_t valid;
   PppContext *context;

   //Point to the PPP context
   context = interface->pppContext;

   //Point to the first byte of the receive buffer
   k = context->rxReadIndex;
   //Number of characters pending in the receive buffer
   n = context->rxBufferLen;

   //Loop through received data
   for(i = 0, valid = FALSE; i < n && !valid; i++)
   {
      //Read current character
      data[i] = context->rxBuffer[k];

      //Carriage return?
      if(data[i] == '\r' || data[i] == '\n')
      {
         data[i] = '\0';
         valid = TRUE;
      }
      //Special processing of null-modem connections
      else if(i >= 5 && !osMemcmp(data + i - 5, "CLIENT", 6))
      {
         data[i + 1] = '\0';
         valid = TRUE;
      }
      else if(i >= 5 && !osMemcmp(data + i - 5, "SERVER", 6))
      {
         data[i + 1] = '\0';
         valid = TRUE;
      }
      //Buffer full?
      else if(i == (size - 2))
      {
         data[i + 1] = '\0';
         valid = TRUE;
      }

      //Increment index and wrap around if necessary
      if(++k >= PPP_RX_BUFFER_SIZE)
         k = 0;
   }

   //Valid command received?
   if(valid)
   {
      //Advance read index
      context->rxReadIndex = (context->rxReadIndex + i) % PPP_RX_BUFFER_SIZE;

      //Enter critical section
      __disable_irq();
      //Update the length of the RX buffer
      context->rxBufferLen -= i;
      //Exit critical section
      __enable_irq();

      //Successful processing
      return NO_ERROR;
   }
   else
   {
      //data[i] = '\0';
      //TRACE_INFO("PPP RX buffer residue (%d bytes)\r\n", i);
      //TRACE_INFO_ARRAY("#  ", data, i);
      return ERROR_BUFFER_EMPTY;
   }
}


/**
 * @brief Purge TX buffer
 * @param[in] context Pointer to the PPP context
 * @return Error code
 **/

error_t pppHdlcDriverPurgeTxBuffer(PppContext *context)
{
   //Enter critical section
   __disable_irq();

   //Purge TX buffer
   context->txBufferLen = 0;
   context->txWriteIndex = 0;
   context->txReadIndex = 0;

   //Exit critical section
   __enable_irq();

   //Successful operation
   return NO_ERROR;
}


/**
 * @brief Purge RX buffer
 * @param[in] context Pointer to the PPP context
 * @return Error code
 **/

error_t pppHdlcDriverPurgeRxBuffer(PppContext *context)
{
   //Enter critical section
   __disable_irq();

   //Purge RX buffer
   context->rxBufferLen = 0;
   context->rxWriteIndex = 0;
   context->rxReadIndex = 0;
   context->rxFrameCount = 0;

   //Exit critical section
   __enable_irq();

   //Successful operation
   return NO_ERROR;
}


/**
 * @brief Write TX queue
 * @param[in] context Pointer to the PPP context
 * @param[in] c Character to be written
 **/

void pppHdlcDriverWriteTxQueue(PppContext *context, uint8_t c)
{
   //Enqueue the character
   context->txBuffer[context->txWriteIndex] = c;

   //Increment index and wrap around if necessary
   if(++context->txWriteIndex >= PPP_TX_BUFFER_SIZE)
      context->txWriteIndex = 0;

   //Enter critical section
   __disable_irq();
   //Update the length of the queue
   context->txBufferLen++;
   //Exit critical section
   __enable_irq();
}


/**
 * @brief Read RX queue
 * @param[in] context Pointer to the PPP context
 * @return Character read from the queue
 **/

uint8_t pppHdlcDriverReadRxQueue(PppContext *context)
{
   uint8_t c;

   //Read a single character
   c = context->rxBuffer[context->rxReadIndex];

   //Increment index and wrap around if necessary
   if(++context->rxReadIndex >= PPP_RX_BUFFER_SIZE)
      context->rxReadIndex = 0;

   //Enter critical section
   __disable_irq();
   //Update the length of the queue
   context->rxBufferLen--;
   //Exit critical section
   __enable_irq();

   //Return the character that has been read
   return c;
}


/**
 * @brief Read TX queue
 * @param[in] interface Underlying network interface
 * @param[out] c Character read from the queue
 * @return TRUE if a context switch is required
 **/

bool_t pppHdlcDriverReadTxQueue(NetInterface *interface, int_t *c)
{
   bool_t flag;
   PppContext *context;

   //Point to the PPP context
   context = interface->pppContext;
   //This flag will be set if a higher priority task must be woken
   flag = FALSE;

   //Any data pending in the TX queue?
   if(context->txBufferLen > 0)
   {
      //Read a single character
      *c = context->txBuffer[context->txReadIndex];

      //Increment index and wrap around if necessary
      if(++context->txReadIndex >= PPP_TX_BUFFER_SIZE)
         context->txReadIndex = 0;

      //Update the length of the queue
      context->txBufferLen--;

      //Check whether the TX is available for writing
      if(context->txBufferLen == (PPP_TX_BUFFER_SIZE - 3006))
      {
         flag = osSetEventFromIsr(&interface->nicTxEvent);
      }
   }
   else
   {
      //The TX queue is empty
      *c = EOF;
   }

   //The return value tells whether a context switch is required
   return flag;
}


/**
 * @brief Write RX queue
 * @param[in] interface Underlying network interface
 * @param[in] c Character to be written
 * @return TRUE if a context switch is required
 **/

bool_t pppHdlcDriverWriteRxQueue(NetInterface *interface, uint8_t c)
{
   bool_t flag;
   PppContext *context;

   //Point to the PPP context
   context = interface->pppContext;
   //This flag will be set if a higher priority task must be woken
   flag = FALSE;

   //Make sure the RX queue is not full
   if(context->rxBufferLen < PPP_RX_BUFFER_SIZE)
   {
      //Enqueue the character
      context->rxBuffer[context->rxWriteIndex] = c;

      //Increment index and wrap around if necessary
      if(++context->rxWriteIndex >= PPP_RX_BUFFER_SIZE)
         context->rxWriteIndex = 0;

      //Update the length of the queue
      context->rxBufferLen++;

      //0x7E flag found?
      if(c == PPP_FLAG_CHAR)
      {
         //Increment frame counter
         context->rxFrameCount++;

         //A complete HDLC frame has been received
         interface->nicEvent = TRUE;
         //Notify the TCP/IP stack of the event
         flag = osSetEventFromIsr(&netEvent);
      }
   }

   //The return value tells whether a context switch is required
   return flag;
}

#endif

