/**
 * @file coap_server_misc.c
 * @brief Helper functions for CoAP server
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
#include "coap/coap_server.h"
#include "coap/coap_server_transport.h"
#include "coap/coap_server_misc.h"
#include "coap/coap_common.h"
#include "coap/coap_debug.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (COAP_SERVER_SUPPORT == ENABLED)


/**
 * @brief Handle periodic operations
 * @param[in] context Pointer to the CoAP server context
 **/

void coapServerTick(CoapServerContext *context)
{
#if (COAP_SERVER_DTLS_SUPPORT == ENABLED)
   error_t error;
   uint_t i;
   systime_t time;
   CoapDtlsSession *session;

   //Get current time
   time = osGetSystemTime();

   //Loop through DTLS sessions
   for(i = 0; i < COAP_SERVER_MAX_SESSIONS; i++)
   {
      //Point to the current DTLS session
      session = &context->session[i];

      //Valid DTLS session?
      if(session->dtlsContext != NULL)
      {
         //Implementations should limit the lifetime of established sessions
         if(timeCompare(time, session->timestamp + COAP_SERVER_SESSION_TIMEOUT) >= 0)
         {
            //Debug message
            TRACE_INFO("CoAP Server: DTLS session timeout!\r\n");

            //Send a close notify alert to the client
            tlsShutdown(session->dtlsContext);
            //Release DTLS session
            coapServerDeleteSession(session);
         }
         else
         {
            //Continue DTLS handshake
            error = tlsConnect(session->dtlsContext);

            //Any error to report?
            if(error != NO_ERROR &&
               error != ERROR_TIMEOUT &&
               error != ERROR_WOULD_BLOCK)
            {
               //Debug message
               TRACE_INFO("CoAP Server: DTLS handshake failed!\r\n");

               //Release DTLS session
               coapServerDeleteSession(session);
            }
         }
      }
   }
#endif
}


/**
 * @brief Process CoAP request
 * @param[in] context Pointer to the CoAP server context
 * @param[in] data Pointer to the incoming CoAP message
 * @param[in] length Length of the CoAP message, in bytes
 * @return Error code
 **/

error_t coapServerProcessRequest(CoapServerContext *context,
   const uint8_t *data, size_t length)
{
   error_t error;
   CoapCode code;
   CoapMessageType type;

   //Check the length of the CoAP message
   if(length > COAP_MAX_MSG_SIZE)
      return ERROR_INVALID_LENGTH;

   //Copy the request message
   osMemcpy(context->request.buffer, data, length);

   //Save the length of the request message
   context->request.length = length;
   context->request.pos = 0;

   //Parse the received message
   error = coapParseMessage(&context->request);

   //Valid CoAP message?
   if(error == NO_ERROR)
   {
      //Terminate the payload with a NULL character
      context->request.buffer[context->request.length] = '\0';

      //Debug message
      TRACE_INFO("CoAP Server: CoAP message received (%" PRIuSIZE " bytes)...\r\n",
         context->request.length);

      //Dump the contents of the message for debugging purpose
      coapDumpMessage(context->request.buffer, context->request.length);

      //Retrieve message type and method code
      coapGetType(&context->request, &type);
      coapGetCode(&context->request, &code);

      //Initialize CoAP response message
      coapServerInitResponse(context);

      //Check the type of the request
      if(type == COAP_TYPE_CON || type == COAP_TYPE_NON)
      {
         //Check message code
         if(code == COAP_CODE_GET ||
            code == COAP_CODE_POST ||
            code == COAP_CODE_PUT ||
            code == COAP_CODE_DELETE ||
            code == COAP_CODE_FETCH ||
            code == COAP_CODE_PATCH ||
            code == COAP_CODE_IPATCH)
         {
            //Reconstruct the path component from Uri-Path options
            coapJoinRepeatableOption(&context->request, COAP_OPT_URI_PATH,
               context->uri, COAP_SERVER_MAX_URI_LEN, '/');

            //If the resource name is the empty string, set it to a single "/"
            //character (refer to RFC 7252, section 6.5)
            if(context->uri[0] == '\0')
            {
               osStrcpy(context->uri, "/");
            }

            //Any registered callback?
            if(context->settings.requestCallback != NULL)
            {
               //Invoke user callback function
               error = context->settings.requestCallback(context, code,
                  context->uri);
            }
            else
            {
               //Generate a 4.04 piggybacked response
               error = coapSetCode(&context->response, COAP_CODE_NOT_FOUND);
            }
         }
         else if(code == COAP_CODE_EMPTY)
         {
            //Provoking a Reset message by sending an Empty Confirmable message
            //can be used to check of the liveness of an endpoint (refer to
            //RFC 7252, section 4.3)
            error = coapServerRejectRequest(context);
         }
         else
         {
            //A request with an unrecognized or unsupported method code must
            //generate a 4.05 piggybacked response (refer to RFC 7252, section
            //5.8)
            error = coapSetCode(&context->response, COAP_CODE_METHOD_NOT_ALLOWED);
         }
      }
      else
      {
         //Recipients of Acknowledgement and Reset messages must not respond
         //with either Acknowledgement or Reset messages
         error = ERROR_INVALID_REQUEST;
      }
   }
   else if(error == ERROR_INVALID_HEADER || error == ERROR_INVALID_VERSION)
   {
      //Messages with a size smaller than four bytes or with unknown version
      //numbers must be silently ignored (refer to RFC 7252, section 3)
   }
   else
   {
      //Other message format errors, such as an incomplete datagram or the
      //usage of reserved values, are rejected with a Reset message
      error = coapServerRejectRequest(context);
   }

   //Check status code
   if(!error)
   {
      //Any response?
      if(context->response.length > 0)
      {
         //Debug message
         TRACE_INFO("CoAP Server: Sending CoAP message (%" PRIuSIZE " bytes)...\r\n",
            context->response.length);

         //Dump the contents of the message for debugging purpose
         coapDumpMessage(context->response.buffer, context->response.length);

         //Send CoAP response message
         error = coapServerSendResponse(context, context->response.buffer,
            context->response.length);
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Reject a CoAP request
 * @param[in] context Pointer to the CoAP server context
 * @return Error code
 **/

error_t coapServerRejectRequest(CoapServerContext *context)
{
   error_t error;
   const CoapMessageHeader *header;

   //Initialize status code
   error = NO_ERROR;

   //Debug message
   TRACE_INFO("CoAP Server: Rejecting CoAP message...\r\n");

   //Point to the CoAP message header
   header = (CoapMessageHeader *) &context->request.buffer;

   //Check the type of the request
   if(header->type == COAP_TYPE_CON)
   {
      //Rejecting a Confirmable message is effected by sending a matching
      //Reset message
      error = coapServerFormatReset(context, ntohs(header->mid));
   }
   else if(header->type == COAP_TYPE_NON)
   {
      //Rejecting a Non-confirmable message may involve sending a matching
      //Reset message, and apart from the Reset message the rejected message
      //must be silently ignored (refer to RFC 7252, section 4.3)
      error = coapServerFormatReset(context, ntohs(header->mid));
   }
   else
   {
      //Rejecting an Acknowledgment or Reset message is effected by
      //silently ignoring it (refer to RFC 7252, section 4.2)
      context->response.length = 0;
   }

   //Return status code
   return error;
}


/**
 * @brief Initialize CoAP response message
 * @param[in] context Pointer to the CoAP server context
 * @return Error code
 **/

error_t coapServerInitResponse(CoapServerContext *context)
{
   CoapMessageHeader *requestHeader;
   CoapMessageHeader *responseHeader;

   //Point to the CoAP request header
   requestHeader = (CoapMessageHeader *) context->request.buffer;
   //Point to the CoAP response header
   responseHeader = (CoapMessageHeader *) context->response.buffer;

   //Format message header
   responseHeader->version = COAP_VERSION_1;
   responseHeader->tokenLen = requestHeader->tokenLen;
   responseHeader->code = COAP_CODE_INTERNAL_SERVER;
   responseHeader->mid = requestHeader->mid;

   //If immediately available, the response to a request carried in a
   //Confirmable message is carried in an Acknowledgement (ACK) message
   if(requestHeader->type == COAP_TYPE_CON)
   {
      responseHeader->type = COAP_TYPE_ACK;
   }
   else
   {
      responseHeader->type = COAP_TYPE_NON;
   }

   //The token is used to match a response with a request
   osMemcpy(responseHeader->token, requestHeader->token,
      requestHeader->tokenLen);

   //Set the length of the CoAP message
   context->response.length = sizeof(CoapMessageHeader) + responseHeader->tokenLen;
   context->response.pos = 0;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Send CoAP response
 * @param[in] context Pointer to the CoAP server context
 * @param[in] data Pointer to a buffer containing the response message
 * @param[in] length Length of the response message, in bytes
 * @return Error code
 **/

error_t coapServerSendResponse(CoapServerContext *context,
   const void *data, size_t length)
{
   error_t error;

#if (COAP_SERVER_DTLS_SUPPORT == ENABLED)
   //DTLS-secured communication?
   if(context->settings.dtlsInitCallback != NULL)
   {
      uint_t i;
      CoapDtlsSession *session;

      //Loop through DTLS sessions
      for(i = 0; i < COAP_SERVER_MAX_SESSIONS; i++)
      {
         //Point to the current DTLS session
         session = &context->session[i];

         //Valid DTLS session?
         if(session->dtlsContext != NULL)
         {
            //Matching DTLS session?
            if(ipCompAddr(&session->serverIpAddr, &context->serverIpAddr) &&
               ipCompAddr(&session->clientIpAddr, &context->clientIpAddr) &&
               session->clientPort == context->clientPort)
            {
               break;
            }
         }
      }

      //Any matching DTLS session?
      if(i < COAP_SERVER_MAX_SESSIONS)
      {
         //Send DTLS datagram
         error = tlsWrite(session->dtlsContext, data, length, NULL, 0);
      }
      else
      {
         //Report an error
         error = ERROR_FAILURE;
      }
   }
   else
#endif
   {
      //Send UDP datagram
      error = socketSendTo(context->socket, &context->clientIpAddr,
         context->clientPort, data, length, NULL, 0);
   }

   //Return status code
   return error;
}


/**
 * @brief Format Reset message
 * @param[in] context Pointer to the CoAP server context
 * @param[in] mid Message ID
 * @return Error code
 **/

error_t coapServerFormatReset(CoapServerContext *context, uint16_t mid)
{
   CoapMessageHeader *header;

   //Point to the CoAP response header
   header = (CoapMessageHeader *) context->response.buffer;

   //Format Reset message
   header->version = COAP_VERSION_1;
   header->type = COAP_TYPE_RST;
   header->tokenLen = 0;
   header->code = COAP_CODE_EMPTY;

   //The Reset message message must echo the message ID of the confirmable
   //message and must be empty (refer to RFC 7252, section 4.2)
   header->mid = htons(mid);

   //Set the length of the CoAP message
   context->response.length = sizeof(CoapMessageHeader);

   //Successful processing
   return NO_ERROR;
}

#endif
