/**
 * @file smtp_client_transport.c
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
#define TRACE_LEVEL SMTP_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "smtp/smtp_client.h"
#include "smtp/smtp_client_transport.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (SMTP_CLIENT_SUPPORT == ENABLED)


/**
 * @brief Open network connection
 * @param[in] context Pointer to the SMTP client context
 * @return Error code
 **/

error_t smtpClientOpenConnection(SmtpClientContext *context)
{
   error_t error;

   //Open a TCP socket
   context->socket = socketOpen(SOCKET_TYPE_STREAM, SOCKET_IP_PROTO_TCP);
   //Failed to open socket?
   if(context->socket == NULL)
      return ERROR_OPEN_FAILED;

   //Associate the socket with the relevant interface
   error = socketBindToInterface(context->socket, context->interface);
   //Any error to report?
   if(error)
      return error;

   //Set timeout
   error = socketSetTimeout(context->socket, context->timeout);
   //Any error to report?
   if(error)
      return error;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Establish network connection
 * @param[in] context Pointer to the SMTP client context
 * @param[in] serverIpAddr IP address of the SMTP server to connect to
 * @param[in] serverPort TCP port number that will be used to establish the
 *   connection
 * @return Error code
 **/

error_t smtpClientEstablishConnection(SmtpClientContext *context,
   const IpAddr *serverIpAddr, uint16_t serverPort)
{
   //Establish TCP connection
   return socketConnect(context->socket, serverIpAddr, serverPort);
}


/**
 * @brief Open secure connection
 * @param[in] context Pointer to the SMTP client context
 * @return Error code
 **/

error_t smtpClientOpenSecureConnection(SmtpClientContext *context)
{
#if (SMTP_CLIENT_TLS_SUPPORT == ENABLED)
   error_t error;

   //Allocate TLS context
   context->tlsContext = tlsInit();
   //Failed to allocate TLS context?
   if(context->tlsContext == NULL)
      return ERROR_OPEN_FAILED;

   //Select client operation mode
   error = tlsSetConnectionEnd(context->tlsContext, TLS_CONNECTION_END_CLIENT);
   //Any error to report?
   if(error)
      return error;

   //Bind TLS to the relevant socket
   error = tlsSetSocket(context->tlsContext, context->socket);
   //Any error to report?
   if(error)
      return error;

   //Set TX and RX buffer size
   error = tlsSetBufferSize(context->tlsContext,
      SMTP_CLIENT_TLS_TX_BUFFER_SIZE, SMTP_CLIENT_TLS_RX_BUFFER_SIZE);
   //Any error to report?
   if(error)
      return error;

   //Restore TLS session
   error = tlsRestoreSessionState(context->tlsContext, &context->tlsSession);
   //Any error to report?
   if(error)
      return error;

   //Invoke user-defined callback, if any
   if(context->tlsInitCallback != NULL)
   {
      //Perform TLS related initialization
      error = context->tlsInitCallback(context, context->tlsContext);
      //Any error to report?
      if(error)
         return error;
   }

   //Successful processing
   return NO_ERROR;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Establish secure connection
 * @param[in] context Pointer to the SMTP client context
 * @return Error code
 **/

error_t smtpClientEstablishSecureConnection(SmtpClientContext *context)
{
#if (SMTP_CLIENT_TLS_SUPPORT == ENABLED)
   error_t error;

   //Establish TLS connection
   error = tlsConnect(context->tlsContext);

   //Check status code
   if(!error)
   {
      //Save TLS session
      error = tlsSaveSessionState(context->tlsContext, &context->tlsSession);
   }

   //Return status code
   return error;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Shutdown network connection
 * @param[in] context Pointer to the SMTP client context
 * @return Error code
 **/

error_t smtpClientShutdownConnection(SmtpClientContext *context)
{
   error_t error;

   //Initialize status code
   error = NO_ERROR;

#if (SMTP_CLIENT_TLS_SUPPORT == ENABLED)
   //Valid TLS context?
   if(context->tlsContext != NULL)
   {
      //Shutdown TLS session
      error = tlsShutdown(context->tlsContext);
   }
#endif

   //Check status code
   if(!error)
   {
      //Valid TCP socket?
      if(context->socket != NULL)
      {
         //Shutdown TCP connection
         error = socketShutdown(context->socket, SOCKET_SD_BOTH);
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Close network connection
 * @param[in] context Pointer to the SMTP client context
 **/

void smtpClientCloseConnection(SmtpClientContext *context)
{
#if (SMTP_CLIENT_TLS_SUPPORT == ENABLED)
   //Release TLS context
   if(context->tlsContext != NULL)
   {
      tlsFree(context->tlsContext);
      context->tlsContext = NULL;
   }
#endif

   //Close TCP connection
   if(context->socket != NULL)
   {
      socketClose(context->socket);
      context->socket = NULL;
   }
}


/**
 * @brief Send data using the relevant transport protocol
 * @param[in] context Pointer to the SMTP client context
 * @param[in] data Pointer to a buffer containing the data to be transmitted
 * @param[in] length Number of bytes to be transmitted
 * @param[out] written Actual number of bytes written (optional parameter)
 * @param[in] flags Set of flags that influences the behavior of this function
 * @return Error code
 **/

error_t smtpClientSendData(SmtpClientContext *context, const void *data,
   size_t length, size_t *written, uint_t flags)
{
   error_t error;

#if (SMTP_CLIENT_TLS_SUPPORT == ENABLED)
   //TLS-secured connection?
   if(context->tlsContext != NULL)
   {
      //Send TLS-encrypted data
      error = tlsWrite(context->tlsContext, data, length, written, flags);
   }
   else
#endif
   {
      //Transmit data
      error = socketSend(context->socket, data, length, written, flags);
   }

   //Return status code
   return error;
}


/**
 * @brief Receive data using the relevant transport protocol
 * @param[in] context Pointer to the SMTP client context
 * @param[out] data Buffer into which received data will be placed
 * @param[in] size Maximum number of bytes that can be received
 * @param[out] received Number of bytes that have been received
 * @param[in] flags Set of flags that influences the behavior of this function
 * @return Error code
 **/

error_t smtpClientReceiveData(SmtpClientContext *context, void *data,
   size_t size, size_t *received, uint_t flags)
{
   error_t error;

#if (SMTP_CLIENT_TLS_SUPPORT == ENABLED)
   //TLS-secured connection?
   if(context->tlsContext != NULL)
   {
      //Receive TLS-encrypted data
      error = tlsRead(context->tlsContext, data, size, received, flags);
   }
   else
#endif
   {
      //Receive data
      error = socketReceive(context->socket, data, size, received, flags);
   }

   //Return status code
   return error;
}

#endif
