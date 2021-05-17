/**
 * @file coap_client_misc.c
 * @brief Helper functions for CoAP client
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
#define TRACE_LEVEL COAP_TRACE_LEVEL

//Dependencies
#include <stdlib.h>
#include "core/net.h"
#include "coap/coap_client.h"
#include "coap/coap_client_observe.h"
#include "coap/coap_client_transport.h"
#include "coap/coap_client_misc.h"
#include "coap/coap_common.h"
#include "coap/coap_debug.h"
#include "debug.h"
#include "error.h"

//Check TCP/IP stack configuration
#if (COAP_CLIENT_SUPPORT == ENABLED)


/**
 * @brief Process CoAP client events
 * @param[in] context Pointer to the CoAP client context
 * @param[in] timeout Maximum time to wait before returning
 * @return Error code
 **/

error_t coapClientProcessEvents(CoapClientContext *context, systime_t timeout)
{
   error_t error;
   uint_t i;
   systime_t d;
   systime_t startTime;
   systime_t currentTime;

   //Flush receive buffer
   context->response.length = 0;

   //Save current time
   currentTime = osGetSystemTime();
   startTime = currentTime;

#if (NET_RTOS_SUPPORT == ENABLED)
   //Process events
   do
   {
#endif
      //Maximum time to wait for an incoming datagram
      if(timeCompare(startTime + timeout, currentTime) > 0)
         d = startTime + timeout - currentTime;
      else
         d = 0;

      //Limit the delay
      d = MIN(d, COAP_CLIENT_TICK_INTERVAL);

      //Wait for incoming traffic
      coapClientWaitForDatagram(context, d);

      //Get current time
      currentTime = osGetSystemTime();

      //Receive datagram, if any
      error = coapClientReceiveDatagram(context, context->response.buffer,
         COAP_MAX_MSG_SIZE, &context->response.length);

      //Any datagram received?
      if(error == NO_ERROR)
      {
         //Parse the received datagram
         error = coapParseMessage(&context->response);

         //Check status code
         if(error == NO_ERROR)
         {
            //Rewind to the beginning of the buffer
            context->response.pos = 0;
            //Terminate the payload with a NULL character
            context->response.buffer[context->response.length] = '\0';

            //Debug message
            TRACE_INFO("CoAP message received (%" PRIuSIZE " bytes)...\r\n",
               context->response.length);

            //Dump the contents of the message for debugging purpose
            coapDumpMessage(context->response.buffer,
               context->response.length);

            //Try to match the response with an outstanding request
            for(i = 0; i < COAP_CLIENT_NSTART; i++)
            {
               //Apply request/response matching rules
               error = coapClientMatchResponse(&context->request[i],
                  &context->response);

               //Test if the response matches the current request
               if(error != ERROR_UNEXPECTED_MESSAGE)
                  break;
            }

            //Check status code
            if(error == NO_ERROR)
            {
               //Process the received CoAP message
               error = coapClientProcessResponse(&context->request[i],
                  &context->response);
            }
            else if(error == ERROR_UNEXPECTED_MESSAGE)
            {
               //Reject the received CoAP message
               error = coapClientRejectResponse(context, &context->response);
            }
            else
            {
               //Just for sanity
            }
         }
         else
         {
            //Debug message
            TRACE_DEBUG("Invalid CoAP message received!\r\n");
            //Silently discard the received datagram
            error = NO_ERROR;
         }
      }
      else if(error == ERROR_WOULD_BLOCK || error == ERROR_TIMEOUT)
      {
         //No datagram has been received
         error = NO_ERROR;
      }
      else
      {
         //Just for sanity
      }

      //Check status code
      if(error == NO_ERROR)
      {
         //Process request-specific events
         for(i = 0; i < COAP_CLIENT_NSTART; i++)
         {
            //Manage retransmission for the current request
            error = coapClientProcessRequestEvents(&context->request[i]);
            //Any error to report?
            if(error)
               break;
         }
      }

#if (NET_RTOS_SUPPORT == ENABLED)
      //Check whether the timeout has elapsed
   } while(error == NO_ERROR && context->response.length == 0 && d > 0);
#endif

   //Return status code
   return error;
}


/**
 * @brief Process request-specific events
 * @param[in] request CoAP request handle
 * @return Error code
 **/

error_t coapClientProcessRequestEvents(CoapClientRequest *request)
{
   error_t error;
   systime_t time;
   CoapClientContext *context;
   CoapMessageHeader *header;

   //Initialize status code
   error = NO_ERROR;

   //Get current time
   time = osGetSystemTime();

   //Point to the CoAP client context
   context = request->context;
   //Point to the CoAP message header
   header = (CoapMessageHeader *) request->message.buffer;

   //Check current state
   if(request->state == COAP_REQ_STATE_TRANSMIT)
   {
      //Debug message
      TRACE_INFO("Sending CoAP message (%" PRIuSIZE " bytes)...\r\n",
         request->message.length);

      //Dump the contents of the message for debugging purpose
      coapDumpMessage(request->message.buffer, request->message.length);

      //Send CoAP request
      error = coapClientSendDatagram(context, request->message.buffer,
         request->message.length);

      //Save the time at which the message was sent
      request->retransmitStartTime = osGetSystemTime();

      //Check retransmission counter
      if(request->retransmitCount == 0)
      {
         //Save request start time
         request->startTime = request->retransmitStartTime;

         //The initial timeout is set to a random duration
         request->retransmitTimeout = netGetRandRange(COAP_CLIENT_ACK_TIMEOUT_MIN,
            COAP_CLIENT_ACK_TIMEOUT_MAX);
      }
      else
      {
         //The timeout is doubled
         request->retransmitTimeout *= 2;
      }

      //Increment retransmission counter
      request->retransmitCount++;

      //Wait for a response to be received
      coapClientChangeRequestState(request, COAP_REQ_STATE_RECEIVE);
   }
   else if(request->state == COAP_REQ_STATE_RECEIVE)
   {
      //Check whether the timeout has elapsed
      if(timeCompare(time, request->startTime + request->timeout) >= 0)
      {
         //Report a timeout error
         error = coapClientChangeRequestState(request, COAP_REQ_STATE_TIMEOUT);
      }
      else if(timeCompare(time, request->retransmitStartTime +
         request->retransmitTimeout) >= 0)
      {
         //The reliable transmission of a message is initiated by marking the
         //message as Confirmable in the CoAP header
         if(header->type == COAP_TYPE_CON)
         {
            //The sender retransmits the Confirmable message at exponentially
            //increasing intervals, until it receives an acknowledgment or
            //runs out of attempts (refer to RFC 7252, section 4.2)
            if(request->retransmitCount < COAP_CLIENT_MAX_RETRANSMIT)
            {
               //When the timeout is triggered and the retransmission counter is
               //less than MAX_RETRANSMIT, the message is retransmitted
               error = coapClientChangeRequestState(request,
                  COAP_REQ_STATE_TRANSMIT);
            }
            else
            {
               //If the retransmission counter reaches MAX_RETRANSMIT on a
               //timeout, then the attempt to transmit the message is canceled
               //and the application process informed of failure
               error = coapClientChangeRequestState(request,
                  COAP_REQ_STATE_TIMEOUT);
            }
         }
      }
      else
      {
         //Just for sanity
      }
   }
   else if(request->state == COAP_REQ_STATE_SEPARATE)
   {
      //Check whether the timeout has elapsed
      if(timeCompare(time, request->startTime + request->timeout) >= 0)
      {
         //Report a timeout error
         error = coapClientChangeRequestState(request, COAP_REQ_STATE_TIMEOUT);
      }
   }
   else if(request->state == COAP_REQ_STATE_OBSERVE)
   {
      //Check whether the timeout has elapsed
      if(timeCompare(time, request->retransmitStartTime +
         request->retransmitTimeout) >= 0)
      {
         //Point to the CoAP message header
         header = (CoapMessageHeader *) request->message.buffer;

         //The message ID is a 16-bit unsigned integer that is generated by
         //the sender of a Confirmable or Non-confirmable message
         coapClientGenerateMessageId(context, header);

         //Reset retransmission counter
         request->retransmitCount = 0;

         //To re-register its interest in a resource, a client may issue a
         //new GET request with the same token as the original
         error = coapClientChangeRequestState(request, COAP_REQ_STATE_TRANSMIT);
      }
   }
   else
   {
      //Just for sanity
   }

   //Return status code
   return error;
}


/**
 * @brief Update CoAP request state
 * @param[in] request CoAP request handle
 * @param[in] newState New state to switch to
 * @return Error code
 **/

error_t coapClientChangeRequestState(CoapClientRequest *request,
   CoapRequestState newState)
{
   error_t error;
   CoapRequestStatus status;
   CoapClientContext *context;

   //Initialize status code
   error = NO_ERROR;

   //Point to the CoAP client context
   context = request->context;

   //Switch to the new state
   request->state = newState;

   //Check whether a request is ready to be transmitted
   if(newState == COAP_REQ_STATE_TRANSMIT)
   {
      //Notify the CoAP client
      osSetEvent(&context->event);
   }

   //The CoAP client operates in asynchronous mode when a callback function
   //has been defined
   if(request->callback != NULL)
   {
      //Check whether the request has terminated
      if(newState == COAP_REQ_STATE_OBSERVE ||
         newState == COAP_REQ_STATE_DONE ||
         newState == COAP_REQ_STATE_RESET ||
         newState == COAP_REQ_STATE_TIMEOUT ||
         newState == COAP_REQ_STATE_CANCELED)
      {
         //Get the status of the request
         if(newState == COAP_REQ_STATE_RESET)
         {
            //A Reset message has been received
            status = COAP_REQUEST_STATUS_RESET;
         }
         else if(newState == COAP_REQ_STATE_TIMEOUT)
         {
            //A timeout error has occurred
            status = COAP_REQUEST_STATUS_TIMEOUT;
         }
         else if(newState == COAP_REQ_STATE_CANCELED)
         {
            //The request has been canceled
            status = COAP_REQUEST_STATUS_CANCELED;
         }
         else
         {
            //The request has successfully completed
            status = COAP_REQUEST_STATUS_SUCCESS;
         }

         //Release exclusive access to the CoAP client context
         osReleaseMutex(&context->mutex);
         //Invoke callback function
         error = request->callback(context, request, status, request->param);
         //Acquire exclusive access to the CoAP client context
         osAcquireMutex(&context->mutex);
      }

      //Asynchronous requests are automatically released
      if(newState == COAP_REQ_STATE_DONE ||
         newState == COAP_REQ_STATE_RESET ||
         newState == COAP_REQ_STATE_TIMEOUT ||
         newState == COAP_REQ_STATE_CANCELED)
      {
         if(request->state != COAP_REQ_STATE_TRANSMIT)
         {
            //Release the resources associated with a CoAP request
            request->state = COAP_REQ_STATE_UNUSED;
         }
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Check whether a response matches the specified request
 * @param[in] request CoAP request handle
 * @param[in] response Pointer to the response message
 * @return Error code
 **/

error_t coapClientMatchResponse(const CoapClientRequest *request,
   const CoapMessage *response)
{
   error_t error;
   const CoapMessageHeader *reqHeader;
   const CoapMessageHeader *respHeader;

   //Get CoAP request and response headers
   reqHeader = (CoapMessageHeader *) request->message.buffer;
   respHeader = (CoapMessageHeader *) response->buffer;

   //Initialize status code
   error = ERROR_UNEXPECTED_MESSAGE;

   //Check request state
   if(request->state == COAP_REQ_STATE_RECEIVE)
   {
      //Confirmable request?
      if(reqHeader->type == COAP_TYPE_CON)
      {
         //Check the type of the response
         if(respHeader->type == COAP_TYPE_CON ||
            respHeader->type == COAP_TYPE_NON)
         {
            //Tokens are used to match a response with a request
            if(coapCompareToken(respHeader, reqHeader))
            {
               //A matching response has been received
               error = NO_ERROR;
            }
         }
         else if(respHeader->type == COAP_TYPE_ACK)
         {
            //The message ID of the Acknowledgment must match the message ID
            //of the Confirmable message
            if(respHeader->mid == reqHeader->mid)
            {
               //Empty ACK received?
               if(respHeader->code == COAP_CODE_EMPTY)
               {
                  //When a Confirmable message carrying a request is acknowledged
                  //with an Empty message, a separate response is sent in a
                  //separate message exchange
                  error = NO_ERROR;
               }
               else
               {
                  //In a piggybacked response, the message ID of the Confirmable
                  //request and the Acknowledgment must match, and the tokens of
                  //the response and original request must match
                  if(coapCompareToken(respHeader, reqHeader))
                  {
                     //A matching response has been received
                     error = NO_ERROR;
                  }
               }
            }
         }
         else if(respHeader->code == COAP_TYPE_RST)
         {
            //The message ID of the Reset must match the message ID of
            //the Confirmable message
            if(respHeader->mid == reqHeader->mid)
            {
               //A Reset message indicates that the request was received by the
               //server, but some context is missing to properly process it
               error = NO_ERROR;
            }
         }
         else
         {
            //Just for sanity
         }
      }
      //Non-Confirmable request?
      else
      {
         //If a request is sent in a Non-confirmable message, then the response
         //is sent using a new Non-confirmable message, although the server may
         //instead send a Confirmable message (refer to RFC 7252, section 2.2)
         if(respHeader->type == COAP_TYPE_CON ||
            respHeader->type == COAP_TYPE_NON)
         {
            //Tokens are used to match a response with a request
            if(coapCompareToken(respHeader, reqHeader))
            {
               //A matching response has been received
               error = NO_ERROR;
            }
         }
         else if(respHeader->type == COAP_TYPE_RST)
         {
            //The message ID of the Reset must match the message ID of
            //the Non-confirmable message
            if(respHeader->mid == reqHeader->mid)
            {
               //A matching response has been received
               error = NO_ERROR;
            }
         }
         else
         {
            //Just for sanity
         }
      }
   }
   else if(request->state == COAP_REQ_STATE_SEPARATE ||
      request->state == COAP_REQ_STATE_OBSERVE)
   {
      //Check the type of the response
      if(respHeader->type == COAP_TYPE_CON ||
         respHeader->type == COAP_TYPE_NON)
      {
         //Tokens are used to match a response with a request
         if(coapCompareToken(respHeader, reqHeader))
         {
            //A matching response has been received
            error = NO_ERROR;
         }
      }
   }
   else
   {
      //Just for sanity
   }

   //Return status code
   return error;
}


/**
 * @brief Process CoAP response
 * @param[in] request CoAP request handle
 * @param[in] response Pointer to the response message
 * @return Error code
 **/

error_t coapClientProcessResponse(CoapClientRequest *request,
   const CoapMessage *response)
{
   error_t error;
   CoapClientContext *context;
   const CoapMessageHeader *header;

   //Point to the CoAP client context
   context = request->context;
   //Point to CoAP response header
   header = (CoapMessageHeader *) response->buffer;

   //Confirmable response received?
   if(header->type == COAP_TYPE_CON)
   {
      //The Acknowledgment message must echo the message ID of the
      //Confirmable message
      coapClientSendAck(context, ntohs(header->mid));
   }

   //Check the type of the response
   if(header->type == COAP_TYPE_ACK &&
      header->code == COAP_CODE_EMPTY)
   {
      //When a Confirmable message carrying a request is acknowledged with
      //an Empty message, a separate response is sent in a separate message
      //exchange
      error = coapClientChangeRequestState(request, COAP_REQ_STATE_SEPARATE);
   }
   else if(header->type == COAP_TYPE_CON ||
      header->type == COAP_TYPE_NON ||
      header->type == COAP_TYPE_ACK)
   {
#if (COAP_CLIENT_OBSERVE_SUPPORT == ENABLED)
      uint32_t value;

      //Search the CoAP request for an Observe option
      error = coapGetUintOption(&request->message, COAP_OPT_OBSERVE, 0, &value);

      //Observe option included in the request?
      if(!error)
      {
         //The client requests the server to add or remove an entry in the
         //list of observers of the resource depending on the option value
         if(value == COAP_OBSERVE_REGISTER)
         {
            //Process notification response
            error = coapClientProcessNotification(request, response);
         }
         else
         {
            //When the client deregisters, it is considered no longer
            //interested in the resource
            error = coapClientChangeRequestState(request,
               COAP_REQ_STATE_CANCELED);
         }
      }
      else
#endif
      {
         //The request has successfully completed
         error = coapClientChangeRequestState(request, COAP_REQ_STATE_DONE);
      }
   }
   else if(header->type == COAP_TYPE_RST)
   {
      //A Reset message indicates that the request was received by the server,
      //but some context is missing to properly process it
      error = coapClientChangeRequestState(request, COAP_REQ_STATE_RESET);
   }
   else
   {
      //Report an error
      error = ERROR_INVALID_TYPE;
   }

   //Return status code
   return error;
}


/**
 * @brief Reject a CoAP response
 * @param[in] context Pointer to the CoAP client context
 * @param[in] response Pointer to the response message to be rejected
 * @return Error code
 **/

error_t coapClientRejectResponse(CoapClientContext *context,
   const CoapMessage *response)
{
   error_t error;
   const CoapMessageHeader *header;

   //Debug message
   TRACE_INFO("Rejecting CoAP message...\r\n");

   //Point to the CoAP message header
   header = (CoapMessageHeader *) response;

   //Check the type of the response
   if(header->type == COAP_TYPE_CON)
   {
      //Rejecting a Confirmable message is effected by sending a matching
      //Reset message
      error = coapClientSendReset(context, ntohs(header->mid));
   }
   else if(header->type == COAP_TYPE_NON)
   {
      //Rejecting a Non-confirmable message may involve sending a matching
      //Reset message, and apart from the Reset message the rejected message
      //must be silently ignored (refer to RFC 7252, section 4.3)
      error = NO_ERROR;
   }
   else
   {
      //Rejecting an Acknowledgment or Reset message is effected by
      //silently ignoring it (refer to RFC 7252, section 4.2)
      error = NO_ERROR;
   }

   //Return status code
   return error;
}


/**
 * @brief Send Acknowledgment message
 * @param[in] context Pointer to the CoAP client context
 * @param[in] mid Message ID
 * @return Error code
 **/

error_t coapClientSendAck(CoapClientContext *context, uint16_t mid)
{
   CoapMessageHeader message;

   //Format Acknowledgment message
   message.version = COAP_VERSION_1;
   message.type = COAP_TYPE_ACK;
   message.tokenLen = 0;
   message.code = COAP_CODE_EMPTY;

   //The Acknowledgment message message must echo the message ID of the
   //confirmable message and must be empty (refer to RFC 7252, section 4.2)
   message.mid = htons(mid);

   //Debug message
   TRACE_INFO("Sending Acknowledgment message (%" PRIuSIZE " bytes)...\r\n",
      sizeof(message));

   //Dump the contents of the message for debugging purpose
   coapDumpMessage(&message, sizeof(message));

   //Send CoAP message
   return coapClientSendDatagram(context, &message, sizeof(message));
}


/**
 * @brief Send Reset message
 * @param[in] context Pointer to the CoAP client context
 * @param[in] mid Message ID
 * @return Error code
 **/

error_t coapClientSendReset(CoapClientContext *context, uint16_t mid)
{
   CoapMessageHeader message;

   //Format Reset message
   message.version = COAP_VERSION_1;
   message.type = COAP_TYPE_RST;
   message.tokenLen = 0;
   message.code = COAP_CODE_EMPTY;

   //The Reset message message must echo the message ID of the confirmable
   //message and must be empty (refer to RFC 7252, section 4.2)
   message.mid = htons(mid);

   //Debug message
   TRACE_INFO("Sending Reset message (%" PRIuSIZE " bytes)...\r\n",
      sizeof(message));

   //Dump the contents of the message for debugging purpose
   coapDumpMessage(&message, sizeof(message));

   //Send CoAP message
   return coapClientSendDatagram(context, &message, sizeof(message));
}


/**
 * @brief Generate a new message identifier
 * @param[in] context Pointer to the CoAP client context
 * @param[in] header Pointer to the CoAP message header
 **/

void coapClientGenerateMessageId(CoapClientContext *context,
   CoapMessageHeader *header)
{
   //The message ID is a 16-bit unsigned integer that is generated by
   //the sender of a Confirmable or Non-confirmable message
   header->mid = htons(context->mid);

   //Increment message identifier
   context->mid++;
}


/**
 * @brief Generate a new token
 * @param[in] context Pointer to the CoAP client context
 * @param[in] header Pointer to the CoAP message header
 **/

void coapClientGenerateToken(CoapClientContext *context,
   CoapMessageHeader *header)
{
   uint8_t i;

   //Generate a random token
   for(i = 0; i < header->tokenLen; i++)
   {
      header->token[i] = (uint8_t) netGetRand();
   }
}

#endif
