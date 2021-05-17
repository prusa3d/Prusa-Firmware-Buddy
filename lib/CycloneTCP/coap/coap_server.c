/**
 * @file coap_server.c
 * @brief CoAP server
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
#include "coap/coap_debug.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (COAP_SERVER_SUPPORT == ENABLED)


/**
 * @brief Initialize settings with default values
 * @param[out] settings Structure that contains CoAP server settings
 **/

void coapServerGetDefaultSettings(CoapServerSettings *settings)
{
   //The CoAP server is not bound to any interface
   settings->interface = NULL;

   //CoAP port number
   settings->port = COAP_PORT;

#if (COAP_SERVER_DTLS_SUPPORT == ENABLED)
   //DTLS initialization callback function
   settings->dtlsInitCallback = NULL;
#endif

   //CoAP request callback function
   settings->requestCallback = NULL;
}


/**
 * @brief CoAP server initialization
 * @param[in] context Pointer to the CoAP server context
 * @param[in] settings CoAP server specific settings
 * @return Error code
 **/

error_t coapServerInit(CoapServerContext *context,
   const CoapServerSettings *settings)
{
   error_t error;

   //Debug message
   TRACE_INFO("Initializing CoAP server...\r\n");

   //Ensure the parameters are valid
   if(context == NULL || settings == NULL)
      return ERROR_INVALID_PARAMETER;

   //Clear the CoAP server context
   osMemset(context, 0, sizeof(CoapServerContext));

   //Save user settings
   context->settings = *settings;

   //Start of exception handling block
   do
   {
      //Open a UDP socket
      context->socket = socketOpen(SOCKET_TYPE_DGRAM, SOCKET_IP_PROTO_UDP);

      //Failed to open socket?
      if(context->socket == NULL)
      {
         //Report an error
         error = ERROR_OPEN_FAILED;
         //Exit immediately
         break;
      }

      //Set timeout for blocking functions
      error = socketSetTimeout(context->socket, COAP_SERVER_TICK_INTERVAL);
      //Any error to report?
      if(error)
         return error;

      //Associate the socket with the relevant interface
      error = socketBindToInterface(context->socket, settings->interface);
      //Unable to bind the socket to the desired interface?
      if(error)
         break;

      //The CoAP server listens for datagrams on port 5683
      error = socketBind(context->socket, &IP_ADDR_ANY, settings->port);
      //Unable to bind the socket to the desired port?
      if(error)
         break;

      //End of exception handling block
   } while(0);

   //Check status code
   if(error)
   {
      //Clean up side effects
      coapServerDeinit(context);
   }

   //Return status code
   return error;
}


/**
 * @brief Set cookie secret
 *
 * This function specifies the cookie secret used while generating and
 * verifying a cookie during the DTLS handshake
 *
 * @param[in] context Pointer to the CoAP server context
 * @param[in] cookieSecret Pointer to the secret key
 * @param[in] cookieSecretLen Length of the secret key, in bytes
 * @return Error code
 **/

error_t coapServerSetCookieSecret(CoapServerContext *context,
   const uint8_t *cookieSecret, size_t cookieSecretLen)
{
#if (COAP_SERVER_DTLS_SUPPORT == ENABLED)
   //Ensure the parameters are valid
   if(context == NULL || cookieSecret == NULL)
      return ERROR_INVALID_PARAMETER;
   if(cookieSecretLen > COAP_SERVER_MAX_COOKIE_SECRET_SIZE)
      return ERROR_INVALID_PARAMETER;

   //Save the secret key
   osMemcpy(context->cookieSecret, cookieSecret, cookieSecretLen);
   //Save the length of the secret key
   context->cookieSecretLen = cookieSecretLen;

   //Successful processing
   return NO_ERROR;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Start CoAP server
 * @param[in] context Pointer to the CoAP server context
 * @return Error code
 **/

error_t coapServerStart(CoapServerContext *context)
{
   OsTask *task;

   //Make sure the CoAP server context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Debug message
   TRACE_INFO("Starting CoAP server...\r\n");

   //Create the CoAP server task
   task = osCreateTask("CoAP Server", (OsTaskCode) coapServerTask,
      context, COAP_SERVER_STACK_SIZE, COAP_SERVER_PRIORITY);

   //Unable to create the task?
   if(task == OS_INVALID_HANDLE)
      return ERROR_OUT_OF_RESOURCES;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief CoAP server task
 * @param[in] context Pointer to the CoAP server context
 **/

void coapServerTask(CoapServerContext *context)
{
   error_t error;

#if (NET_RTOS_SUPPORT == ENABLED)
   //Task prologue
   osEnterTask();

   //Process events
   while(1)
   {
#endif
      //Wait for an incoming datagram
      error = socketReceiveEx(context->socket, &context->clientIpAddr,
         &context->clientPort, &context->serverIpAddr, context->buffer,
         COAP_SERVER_BUFFER_SIZE, &context->bufferLen, 0);

      //Any datagram received?
      if(!error)
      {
         //An endpoint must be prepared to receive multicast messages but may
         //ignore them if multicast service discovery is not desired
         if(!ipIsMulticastAddr(&context->serverIpAddr))
         {
#if (COAP_SERVER_DTLS_SUPPORT == ENABLED)
            //DTLS-secured communication?
            if(context->settings.dtlsInitCallback != NULL)
            {
               //Demultiplexing of incoming datagrams into separate DTLS sessions
               error = coapServerDemultiplexSession(context);
            }
            else
#endif
            {
               //Process the received CoAP message
               error = coapServerProcessRequest(context, context->buffer,
                  context->bufferLen);
            }
         }
      }

      //Handle periodic operations
      coapServerTick(context);
#if (NET_RTOS_SUPPORT == ENABLED)
   }
#endif
}


/**
 * @brief Release CoAP server context
 * @param[in] context Pointer to the CoAP server context
 **/

void coapServerDeinit(CoapServerContext *context)
{
   //Make sure the CoAP server context is valid
   if(context != NULL)
   {
#if (COAP_SERVER_DTLS_SUPPORT == ENABLED)
      uint_t i;

      //Loop through DTLS sessions
      for(i = 0; i < COAP_SERVER_MAX_SESSIONS; i++)
      {
         //Release DTLS session
         coapServerDeleteSession(&context->session[i]);
      }
#endif

      //Close listening socket
      socketClose(context->socket);

      //Clear CoAP server context
      osMemset(context, 0, sizeof(CoapServerContext));
   }
}

#endif
