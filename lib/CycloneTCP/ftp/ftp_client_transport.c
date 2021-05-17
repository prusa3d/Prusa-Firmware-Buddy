/**
 * @file ftp_client_transport.c
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
#define TRACE_LEVEL FTP_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "ftp/ftp_client.h"
#include "ftp/ftp_client_transport.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (FTP_CLIENT_SUPPORT == ENABLED)


/**
 * @brief Open network connection
 * @param[in] context Pointer to the FTP client context
 * @param[in] channel Control or data channel
 * @param[in] txBufferSize TX buffer size
 * @param[in] rxBufferSize RX buffer size
 * @return Error code
 **/

error_t ftpClientOpenChannel(FtpClientContext *context,
   FtpClientChannel *channel, size_t txBufferSize, size_t rxBufferSize)
{
   error_t error;

   //Open a TCP socket
   channel->socket = socketOpen(SOCKET_TYPE_STREAM, SOCKET_IP_PROTO_TCP);
   //Failed to open socket?
   if(channel->socket == NULL)
      return ERROR_OPEN_FAILED;

   //Associate the socket with the relevant interface
   error = socketBindToInterface(channel->socket, context->interface);
   //Any error to report?
   if(error)
      return error;

   //Set timeout
   error = socketSetTimeout(channel->socket, context->timeout);
   //Any error to report?
   if(error)
      return error;

   //Specify the size of the send buffer
   error = socketSetTxBufferSize(channel->socket, txBufferSize);
   //Any error to report?
   if(error)
      return error;

   //Specify the size of the receive buffer
   error = socketSetRxBufferSize(channel->socket, rxBufferSize);
   //Any error to report?
   if(error)
      return error;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Open secure connection
 * @param[in] context Pointer to the FTP client context
 * @param[in] channel Control or data channel
 * @param[in] txBufferSize TX buffer size
 * @param[in] rxBufferSize RX buffer size
 * @return Error code
 **/

error_t ftpClientOpenSecureChannel(FtpClientContext *context,
   FtpClientChannel *channel, size_t txBufferSize, size_t rxBufferSize)
{
#if (FTP_CLIENT_TLS_SUPPORT == ENABLED)
   error_t error;

   //Allocate TLS context
   channel->tlsContext = tlsInit();
   //Failed to allocate TLS context?
   if(channel->tlsContext == NULL)
      return ERROR_OPEN_FAILED;

   //Select client operation mode
   error = tlsSetConnectionEnd(channel->tlsContext, TLS_CONNECTION_END_CLIENT);
   //Any error to report?
   if(error)
      return error;

   //Bind TLS to the relevant socket
   error = tlsSetSocket(channel->tlsContext, channel->socket);
   //Any error to report?
   if(error)
      return error;

   //Set TX and RX buffer size
   error = tlsSetBufferSize(channel->tlsContext, txBufferSize, rxBufferSize);
   //Any error to report?
   if(error)
      return error;

   //Data channel?
   if(channel == &context->dataChannel)
   {
      //Save TLS session from control connection
      error = tlsSaveSessionState(context->controlChannel.tlsContext,
         &context->tlsSession);
      //Any error to report?
      if(error)
         return error;
   }

   //Restore TLS session
   error = tlsRestoreSessionState(channel->tlsContext, &context->tlsSession);
   //Any error to report?
   if(error)
      return error;

   //Invoke user-defined callback, if any
   if(context->tlsInitCallback != NULL)
   {
      //Perform TLS related initialization
      error = context->tlsInitCallback(context, channel->tlsContext);
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
 * @param[in] channel Control or data channel
 * @return Error code
 **/

error_t ftpClientEstablishSecureChannel(FtpClientChannel *channel)
{
#if (FTP_CLIENT_TLS_SUPPORT == ENABLED)
   //Establish TLS connection
   return tlsConnect(channel->tlsContext);
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Shutdown network connection
 * @param[in] channel Control or data channel
 * @return Error code
 **/

error_t ftpClientShutdownChannel(FtpClientChannel *channel)
{
   error_t error;

   //Initialize status code
   error = NO_ERROR;

#if (FTP_CLIENT_TLS_SUPPORT == ENABLED)
   //Valid TLS context?
   if(channel->tlsContext != NULL)
   {
      //Shutdown TLS session
      error = tlsShutdown(channel->tlsContext);
   }
#endif

   //Check status code
   if(!error)
   {
      //Valid TCP socket?
      if(channel->socket != NULL)
      {
         //Shutdown TCP connection
         error = socketShutdown(channel->socket, SOCKET_SD_BOTH);
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Close network connection
 * @param[in] channel Control or data channel
 **/

void ftpClientCloseChannel(FtpClientChannel *channel)
{
#if (FTP_CLIENT_TLS_SUPPORT == ENABLED)
   //Release TLS context
   if(channel->tlsContext != NULL)
   {
      tlsFree(channel->tlsContext);
      channel->tlsContext = NULL;
   }
#endif

   //Close TCP connection
   if(channel->socket != NULL)
   {
      socketClose(channel->socket);
      channel->socket = NULL;
   }
}


/**
 * @brief Send data using the relevant transport protocol
 * @param[in] channel Control or data channel
 * @param[in] data Pointer to a buffer containing the data to be transmitted
 * @param[in] length Number of bytes to be transmitted
 * @param[out] written Actual number of bytes written (optional parameter)
 * @param[in] flags Set of flags that influences the behavior of this function
 * @return Error code
 **/

error_t ftpClientWriteChannel(FtpClientChannel *channel, const void *data,
   size_t length, size_t *written, uint_t flags)
{
   error_t error;

#if (FTP_CLIENT_TLS_SUPPORT == ENABLED)
   //TLS-secured connection?
   if(channel->tlsContext != NULL)
   {
      //Send TLS-encrypted data
      error = tlsWrite(channel->tlsContext, data, length, written, flags);
   }
   else
#endif
   {
      //Transmit data
      error = socketSend(channel->socket, data, length, written, flags);
   }

   //Return status code
   return error;
}


/**
 * @brief Receive data using the relevant transport protocol
 * @param[in] channel Control or data channel
 * @param[out] data Buffer into which received data will be placed
 * @param[in] size Maximum number of bytes that can be received
 * @param[out] received Number of bytes that have been received
 * @param[in] flags Set of flags that influences the behavior of this function
 * @return Error code
 **/

error_t ftpClientReadChannel(FtpClientChannel *channel, void *data,
   size_t size, size_t *received, uint_t flags)
{
   error_t error;

#if (FTP_CLIENT_TLS_SUPPORT == ENABLED)
   //TLS-secured connection?
   if(channel->tlsContext != NULL)
   {
      //Receive TLS-encrypted data
      error = tlsRead(channel->tlsContext, data, size, received, flags);
   }
   else
#endif
   {
      //Receive data
      error = socketReceive(channel->socket, data, size, received, flags);
   }

   //Return status code
   return error;
}

#endif
