/**
 * @file coap_server_transport.c
 * @brief Transport protocol abstraction layer
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
#include "debug.h"

//Check TCP/IP stack configuration
#if (COAP_SERVER_SUPPORT == ENABLED && COAP_SERVER_DTLS_SUPPORT == ENABLED)

//Forward declaration of functions
error_t coapServerSendCallback(void *handle, const void *data,
   size_t length, size_t *written, uint_t flags);

error_t coapServerReceiveCallback(void *handle, void *data,
   size_t size, size_t *received, uint_t flags);

error_t coapServerCookieGenerateCallback(TlsContext *context,
   const DtlsClientParameters *clientParams, uint8_t *cookie,
   size_t *length, void *param);

error_t coapServerCookieVerifyCallback(TlsContext *context,
   const DtlsClientParameters *clientParams, const uint8_t *cookie,
   size_t length, void *param);


/**
 * @brief Accept a new connection from a client
 * @param[in] context Pointer to the CoAP server context
 * @param[in] session Pointer to the DTLS session
 * @param[in] remoteIpAddr Client IP address
 * @param[in] remotePort Client port number
 * @return Error code
 **/

error_t coapServerAcceptSession(CoapServerContext *context,
   CoapDtlsSession *session, const IpAddr *remoteIpAddr, uint16_t remotePort)
{
   error_t error;
   TlsState state;

   //Clear DTLS session
   osMemset(session, 0, sizeof(CoapDtlsSession));

   //Initialize session parameters
   session->context = context;
   session->serverIpAddr = context->serverIpAddr;
   session->clientIpAddr = context->clientIpAddr;
   session->clientPort = context->clientPort;
   session->timestamp = osGetSystemTime();

   //Allocate DTLS context
   session->dtlsContext = tlsInit();

   //DTLS context successfully created?
   if(session->dtlsContext != NULL)
   {
      //Start of exception handling block
      do
      {
         //Select server operation mode
         error = tlsSetConnectionEnd(session->dtlsContext,
            TLS_CONNECTION_END_SERVER);
         //Any error to report?
         if(error)
            break;

         //Use datagram transport protocol
         error = tlsSetTransportProtocol(session->dtlsContext,
            TLS_TRANSPORT_PROTOCOL_DATAGRAM);
         //Any error to report?
         if(error)
            break;

         //Set send and receive callbacks (I/O abstraction layer)
         error = tlsSetSocketCallbacks(session->dtlsContext, coapServerSendCallback,
            coapServerReceiveCallback, (TlsSocketHandle) session);
         //Any error to report?
         if(error)
            break;

         //Set cookie generation/verification callbacks
         error = tlsSetCookieCallbacks(session->dtlsContext,
            coapServerCookieGenerateCallback, coapServerCookieVerifyCallback,
            session);
         //Any error to report?
         if(error)
            break;

         //Invoke user-defined callback, if any
         if(context->settings.dtlsInitCallback != NULL)
         {
            //Perform DTLS related initialization
            error = context->settings.dtlsInitCallback(context,
               session->dtlsContext);
            //Any error to report?
            if(error)
               break;
         }

         //Initiate DTLS handshake
         error = tlsConnect(session->dtlsContext);
         //Any error to report?
         if(error != NO_ERROR && error != ERROR_WOULD_BLOCK)
            break;

         //Retrieve current state
         state = tlsGetState(session->dtlsContext);

         //The DTLS server verifies the cookie and proceeds with the handshake
         //only if it is valid
         if(state == TLS_STATE_INIT ||
            state == TLS_STATE_CLIENT_HELLO ||
            state == TLS_STATE_CLOSED)
         {
            //Do not allocate connection state yet if the stateless cookie
            //exchange is being performed
            error = ERROR_WRONG_COOKIE;
            break;
         }

         //The DTLS implementation decides to continue with the connection
         error = NO_ERROR;

         //Debug message
         TRACE_INFO("CoAP Server: DTLS session established with client %s port %"
            PRIu16 "...\r\n", ipAddrToString(remoteIpAddr, NULL), ntohs(remotePort));

         //End of exception handling block
      } while(0);

      //Check status code
      if(error)
      {
         //Release DTLS context
         tlsFree(session->dtlsContext);
         session->dtlsContext = NULL;
      }
   }
   else
   {
      //Failed to allocate DTLS context
      error = ERROR_OUT_OF_MEMORY;
   }

   //Return status code
   return error;
}


/**
 * @brief DTLS session demultiplexing
 * @param[in] context Pointer to the CoAP server context
 * @return Error code
 **/

error_t coapServerDemultiplexSession(CoapServerContext *context)
{
   error_t error;
   uint_t i;
   size_t length;
   systime_t time;
   CoapDtlsSession *session;
   CoapDtlsSession *firstFreeSession;
   CoapDtlsSession *oldestSession;

   //Initialize status code
   error = NO_ERROR;

   //Get current time
   time = osGetSystemTime();

   //Keep track of the first free entry
   firstFreeSession = NULL;
   //Keep track of the oldest entry
   oldestSession = NULL;

   //Demultiplexing of incoming datagrams into separate DTLS sessions
   for(i = 0; i < COAP_SERVER_MAX_SESSIONS; i++)
   {
      //Point to the current DTLS session
      session = &context->session[i];

      //Valid DTLS session?
      if(session->dtlsContext != NULL)
      {
         //Determine if a DTLS session matches the incoming datagram
         if(ipCompAddr(&session->serverIpAddr, &context->serverIpAddr) &&
            ipCompAddr(&session->clientIpAddr, &context->clientIpAddr) &&
            session->clientPort == context->clientPort)
         {
            //Save current time
            session->timestamp = osGetSystemTime();

            //The UDP datagram is passed to the DTLS implementation
            error = tlsRead(session->dtlsContext, context->buffer,
               COAP_SERVER_BUFFER_SIZE, &length, 0);

            //Check status code
            if(!error)
            {
               //Process the received CoAP message
               error = coapServerProcessRequest(context, context->buffer,
                  length);
            }
            else if(error == ERROR_TIMEOUT || error == ERROR_WOULD_BLOCK)
            {
               //The UDP datagram contains DTLS handshake messages
            }
            else
            {
               //Debug message
               TRACE_INFO("CoAP Server: Failed to read DTLS datagram!\r\n");

               //Release DTLS session
               coapServerDeleteSession(session);
            }

            //We are done
            break;
         }
         else
         {
            //Keep track of the oldest entry
            if(oldestSession == NULL)
            {
               oldestSession = session;
            }
            else if((time - session->timestamp) > (time - oldestSession->timestamp))
            {
               oldestSession = session;
            }
         }
      }
      else
      {
         //Keep track of the first free entry
         if(firstFreeSession == NULL)
         {
            firstFreeSession = session;
         }
      }
   }

   //No matching DTLS session?
   if(i >= COAP_SERVER_MAX_SESSIONS)
   {
      //Any DTLS session available for use in the table?
      if(firstFreeSession != NULL)
      {
         session = firstFreeSession;
      }
      else
      {
         //The oldest DTLS session is closed whenever the table runs out of space
         tlsShutdown(oldestSession->dtlsContext);
         coapServerDeleteSession(oldestSession);

         //Point to the DTLS session to be reused
         session = oldestSession;
      }

      //Process the new connection attempt
      error = coapServerAcceptSession(context, session, &context->clientIpAddr,
         context->clientPort);
   }

   //Return status code
   return error;
}


/**
 * @brief Delete DTLS session
 * @param[in] session Pointer to the DTLS session
 **/

void coapServerDeleteSession(CoapDtlsSession *session)
{
   //Debug message
   TRACE_INFO("CoAP Server: DTLS session closed...\r\n");

   //Valid DTLS context?
   if(session->dtlsContext != NULL)
   {
      //Release DTLS context
      tlsFree(session->dtlsContext);
      session->dtlsContext = NULL;
   }
}


/**
 * @brief DTLS send callback
 * @param[in] handle Handle referencing a client connection
 * @param[in] data Pointer to a buffer containing the data to be transmitted
 * @param[in] length Number of data bytes to send
 * @param[out] written Number of bytes that have been transmitted
 * @param[in] flags Unused parameter
 * @return Error code
 **/

error_t coapServerSendCallback(void *handle, const void *data,
   size_t length, size_t *written, uint_t flags)
{
   error_t error;
   CoapServerContext *context;
   CoapDtlsSession *session;

   //Point to the DTLS session
   session = handle;
   //Point to the CoAP server context
   context = session->context;

   //Send datagram
   error = socketSendTo(context->socket, &session->clientIpAddr,
      session->clientPort, data, length, written, flags);

   //Return status code
   return error;
}


/**
 * @brief DTLS receive callback
 * @param[in] handle Handle referencing a client connection
 * @param[out] data Buffer where to store the incoming data
 * @param[in] size Maximum number of bytes that can be received
 * @param[out] received Number of bytes that have been received
 * @param[in] flags Unused parameter
 * @return Error code
 **/

error_t coapServerReceiveCallback(void *handle, void *data,
   size_t size, size_t *received, uint_t flags)
{
   error_t error;
   CoapServerContext *context;
   CoapDtlsSession *session;

   //Initialize status code
   error = ERROR_WOULD_BLOCK;

   //Point to the DTLS session
   session = (CoapDtlsSession *) handle;
   //Point to the CoAP server context
   context = session->context;

   //Any pending datagram?
   if(context->bufferLen > 0)
   {
      //Pass incoming datagram to the proper connection
      if(ipCompAddr(&context->serverIpAddr, &session->serverIpAddr) &&
         ipCompAddr(&context->clientIpAddr, &session->clientIpAddr) &&
         context->clientPort == session->clientPort)
      {
         //Make sure the length of the datagram is acceptable
         if(context->bufferLen < size)
         {
            //Copy incoming datagram
            osMemcpy(data, context->buffer, context->bufferLen);
            //Return the length of the datagram
            *received = context->bufferLen;

            //Successful processing
            error = NO_ERROR;
         }

         //Flush the receive buffer
         context->bufferLen = 0;
      }
   }

   //Return status code
   return error;
}


/**
 * @brief DTLS cookie generation callback function
 * @param[in] context Pointer to the DTLS context
 * @param[in] clientParams Client's parameters
 * @param[out] cookie Pointer to the first byte of the cookie
 * @param[in,out] length Length of the cookie, in bytes
 * @param[in] param Pointer to the DTLS session
 * @return Error code
 **/

error_t coapServerCookieGenerateCallback(TlsContext *context,
   const DtlsClientParameters *clientParams, uint8_t *cookie,
   size_t *length, void *param)
{
   error_t error;
   CoapDtlsSession *session;
   HmacContext hmacContext;

   //Point to the DTLS session
   session = (CoapDtlsSession *) param;

   //Debug message
   TRACE_INFO("CoAP Server: DTLS cookie generation...\r\n");

   //Make sure the output buffer is large enough to hold the cookie
   if(*length < SHA256_DIGEST_SIZE)
      return ERROR_BUFFER_OVERFLOW;

   //Invalid cookie secret?
   if(session->context->cookieSecretLen == 0)
   {
      //Generate a cookie secret
      error = context->prngAlgo->read(context->prngContext,
         session->context->cookieSecret, COAP_SERVER_MAX_COOKIE_SECRET_SIZE);
      //Any error to report?
      if(error)
         return error;

      //Save the length of the generated secret
      session->context->cookieSecretLen = COAP_SERVER_MAX_COOKIE_SECRET_SIZE;
   }

   //Initialize HMAC context
   hmacInit(&hmacContext, SHA256_HASH_ALGO, session->context->cookieSecret,
      session->context->cookieSecretLen);

   //Generate stateless cookie
   hmacUpdate(&hmacContext, (uint8_t *) &session->clientIpAddr + sizeof(size_t),
      session->clientIpAddr.length);

   //The server should use client parameters (version, random, session_id,
   //cipher_suites, compression_method) to generate its cookie
   hmacUpdate(&hmacContext, &clientParams->version, sizeof(uint16_t));
   hmacUpdate(&hmacContext, clientParams->random, clientParams->randomLen);
   hmacUpdate(&hmacContext, clientParams->sessionId, clientParams->sessionIdLen);
   hmacUpdate(&hmacContext, clientParams->cipherSuites, clientParams->cipherSuitesLen);
   hmacUpdate(&hmacContext, clientParams->compressMethods, clientParams->compressMethodsLen);

   //Finalize HMAC computation
   hmacFinal(&hmacContext, cookie);

   //Return the length of the cookie
   *length = SHA256_DIGEST_SIZE;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief DTLS cookie verification callback function
 * @param[in] context Pointer to the DTLS context
 * @param[in] clientParams Client's parameters
 * @param[in] cookie Pointer to the first byte of the cookie
 * @param[in] length Length of the cookie, in bytes
 * @param[in] param Pointer to the DTLS session
 * @return Error code
 **/

error_t coapServerCookieVerifyCallback(TlsContext *context,
   const DtlsClientParameters *clientParams, const uint8_t *cookie,
   size_t length, void *param)
{
   error_t error;
   CoapDtlsSession *session;
   HmacContext hmacContext;

   //Point to the DTLS session
   session = (CoapDtlsSession *) param;

   //Debug message
   TRACE_INFO("CoAP Server: DTLS cookie verification...\r\n");

   //Make sure the length of the cookie is acceptable
   if(length != SHA256_DIGEST_SIZE)
      return ERROR_WRONG_COOKIE;

   //Invalid cookie secret?
   if(session->context->cookieSecretLen == 0)
   {
      //Generate a cookie secret
      error = context->prngAlgo->read(context->prngContext,
         session->context->cookieSecret, COAP_SERVER_MAX_COOKIE_SECRET_SIZE);
      //Any error to report?
      if(error)
         return error;

      //Save the length of the generated secret
      session->context->cookieSecretLen = COAP_SERVER_MAX_COOKIE_SECRET_SIZE;
   }

   //Initialize HMAC context
   hmacInit(&hmacContext, SHA256_HASH_ALGO, session->context->cookieSecret,
      session->context->cookieSecretLen);

   //Generate stateless cookie
   hmacUpdate(&hmacContext, (uint8_t *) &session->clientIpAddr + sizeof(size_t),
      session->clientIpAddr.length);

   //The server should use client parameters (version, random, session_id,
   //cipher_suites, compression_method) to generate its cookie
   hmacUpdate(&hmacContext, &clientParams->version, sizeof(uint16_t));
   hmacUpdate(&hmacContext, clientParams->random, clientParams->randomLen);
   hmacUpdate(&hmacContext, clientParams->sessionId, clientParams->sessionIdLen);
   hmacUpdate(&hmacContext, clientParams->cipherSuites, clientParams->cipherSuitesLen);
   hmacUpdate(&hmacContext, clientParams->compressMethods, clientParams->compressMethodsLen);

   //Finalize HMAC computation
   hmacFinal(&hmacContext, NULL);

   //Compare the received cookie against the expected value
   if(!osMemcmp(cookie, hmacContext.digest, length))
   {
      //The cookie is valid
      error = NO_ERROR;
   }
   else
   {
      //The cookie is invalid
      error = ERROR_WRONG_COOKIE;
   }

   //Return status code
   return error;
}

#endif
