/**
 * @file web_socket.c
 * @brief WebSocket API (client and server)
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
#define TRACE_LEVEL WEB_SOCKET_TRACE_LEVEL

//Dependencies
#include <stdlib.h>
#include "core/net.h"
#include "web_socket/web_socket.h"
#include "web_socket/web_socket_auth.h"
#include "web_socket/web_socket_frame.h"
#include "web_socket/web_socket_transport.h"
#include "web_socket/web_socket_misc.h"
#include "str.h"
#include "encoding/base64.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (WEB_SOCKET_SUPPORT == ENABLED)

//WebSocket table
WebSocket webSocketTable[WEB_SOCKET_MAX_COUNT];
//Random data generation callback function
WebSocketRandCallback webSockRandCallback;


/**
 * @brief WebSocket related initialization
 * @return Error code
 **/

error_t webSocketInit(void)
{
   //Initialize WebSockets
   osMemset(webSocketTable, 0, sizeof(webSocketTable));

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief Register RNG callback function
 * @param[in] callback RNG callback function
 * @return Error code
 **/

error_t webSocketRegisterRandCallback(WebSocketRandCallback callback)
{
   //Check parameter
   if(callback == NULL)
      return ERROR_INVALID_PARAMETER;

   //Save callback function
   webSockRandCallback = callback;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Create a WebSocket
 * @return Handle referencing the new WebSocket
 **/

WebSocket *webSocketOpen(void)
{
   error_t error;
   uint_t i;
   WebSocket *webSocket;

   //Initialize WebSocket handle
   webSocket = NULL;

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Loop through WebSocket descriptors
   for(i = 0; i < WEB_SOCKET_MAX_COUNT; i++)
   {
      //Unused WebSocket found?
      if(webSocketTable[i].state == WS_STATE_UNUSED)
      {
         //Save socket handle
         webSocket = &webSocketTable[i];

         //Clear associated structure
         osMemset(webSocket, 0, sizeof(WebSocket));

#if (WEB_SOCKET_TLS_SUPPORT == ENABLED)
         //Initialize TLS session state
         error = tlsInitSessionState(&webSocket->tlsSession);
#else
         //TLS is not implemented
         error = NO_ERROR;
#endif

         //Check status code
         if(!error)
         {
            //Set the default timeout to be used
            webSocket->timeout = INFINITE_DELAY;
            //Enter the CLOSED state
            webSocket->state = WS_STATE_CLOSED;

            //We are done
            break;
         }
      }
   }

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Return a handle to the freshly created WebSocket
   return webSocket;
}


/**
 * @brief Upgrade a socket to a WebSocket
 * @param[in] socket Handle referencing the socket
 * @return Handle referencing the new WebSocket
 **/

WebSocket *webSocketUpgradeSocket(Socket *socket)
{
   WebSocket *webSocket;

   //Valid socket handle?
   if(socket != NULL)
   {
      //Create a new WebSocket
      webSocket = webSocketOpen();

      //WebSocket successfully created?
      if(webSocket != NULL)
      {
         //Attach the socket handle
         webSocket->socket = socket;
         //Initialize state
         webSocket->state = WS_STATE_INIT;
      }
   }
   else
   {
      //The specified socket is not valid...
      webSocket = NULL;
   }

   //Return a handle to the freshly created WebSocket
   return webSocket;
}


#if (WEB_SOCKET_TLS_SUPPORT == ENABLED)

/**
 * @brief Upgrade a secure socket to a secure WebSocket
 * @param[in] socket Handle referencing the socket
 * @param[in] tlsContext Pointer to the TLS context
 * @return Handle referencing the new WebSocket
 **/

WebSocket *webSocketUpgradeSecureSocket(Socket *socket, TlsContext *tlsContext)
{
   WebSocket *webSocket;

   //Valid TLS context?
   if(tlsContext != NULL)
   {
      //Create a new WebSocket
      webSocket = webSocketOpen();

      //WebSocket successfully created?
      if(webSocket != NULL)
      {
         //Attach the socket handle
         webSocket->socket = socket;
         //Attach the TLS context
         webSocket->tlsContext = tlsContext;
         //Initialize state
         webSocket->state = WS_STATE_INIT;
      }
   }
   else
   {
      //The specified socket is not valid...
      webSocket = NULL;
   }

   //Return a handle to the freshly created WebSocket
   return webSocket;
}


/**
 * @brief Register TLS initialization callback function
 * @param[in] webSocket Handle that identifies a WebSocket
 * @param[in] callback TLS initialization callback function
 * @return Error code
 **/

error_t webSocketRegisterTlsInitCallback(WebSocket *webSocket,
   WebSocketTlsInitCallback callback)
{
   //Check parameters
   if(webSocket == NULL || callback == NULL)
      return ERROR_INVALID_PARAMETER;

   //Save callback function
   webSocket->tlsInitCallback = callback;

   //Successful processing
   return NO_ERROR;
}

#endif


/**
 * @brief Set timeout value for blocking operations
 * @param[in] webSocket Handle to a WebSocket
 * @param[in] timeout Maximum time to wait
 * @return Error code
 **/

error_t webSocketSetTimeout(WebSocket *webSocket, systime_t timeout)
{
   //Make sure the WebSocket handle is valid
   if(webSocket == NULL)
      return ERROR_INVALID_PARAMETER;

   //Save timeout value
   webSocket->timeout = timeout;

   //Valid socket?
   if(webSocket->socket != NULL)
      socketSetTimeout(webSocket->socket, timeout);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set the domain name of the server (for virtual hosting)
 * @param[in] webSocket Handle to a WebSocket
 * @param[in] host NULL-terminated string containing the hostname
 * @return Error code
 **/

error_t webSocketSetHost(WebSocket *webSocket, const char_t *host)
{
   //Check parameters
   if(webSocket == NULL || host == NULL)
      return ERROR_INVALID_PARAMETER;

   //Save the hostname
   strSafeCopy(webSocket->host, host, WEB_SOCKET_HOST_MAX_LEN);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set the origin header field
 * @param[in] webSocket Handle to a WebSocket
 * @param[in] origin NULL-terminated string containing the origin
 * @return Error code
 **/

error_t webSocketSetOrigin(WebSocket *webSocket, const char_t *origin)
{
   //Check parameters
   if(webSocket == NULL || origin == NULL)
      return ERROR_INVALID_PARAMETER;

   //Save origin
   strSafeCopy(webSocket->origin, origin, WEB_SOCKET_ORIGIN_MAX_LEN);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set the sub-protocol header field
 * @param[in] webSocket Handle to a WebSocket
 * @param[in] subProtocol NULL-terminated string containing the sub-protocol
 * @return Error code
 **/

error_t webSocketSetSubProtocol(WebSocket *webSocket, const char_t *subProtocol)
{
   //Check parameters
   if(webSocket == NULL || subProtocol == NULL)
      return ERROR_INVALID_PARAMETER;

   //Save sub-protocol
   strSafeCopy(webSocket->subProtocol, subProtocol, WEB_SOCKET_SUB_PROTOCOL_MAX_LEN);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set authentication information
 * @param[in] webSocket Handle to a WebSocket
 * @param[in] username NULL-terminated string containing the user name to be used
 * @param[in] password NULL-terminated string containing the password to be used
 * @param[in] allowedAuthModes Logic OR of allowed HTTP authentication schemes
 * @return Error code
 **/

error_t webSocketSetAuthInfo(WebSocket *webSocket, const char_t *username,
   const char_t *password, uint_t allowedAuthModes)
{
#if (WEB_SOCKET_BASIC_AUTH_SUPPORT == ENABLED || WEB_SOCKET_DIGEST_AUTH_SUPPORT == ENABLED)
   WebSocketAuthContext *authContext;

   //Check parameters
   if(webSocket == NULL || username == NULL || password == NULL)
      return ERROR_INVALID_PARAMETER;

   //Point to the authentication context
   authContext = &webSocket->authContext;

   //Save user name
   strSafeCopy(authContext->username, username, WEB_SOCKET_USERNAME_MAX_LEN);
   //Save password
   strSafeCopy(authContext->password, password, WEB_SOCKET_PASSWORD_MAX_LEN);
   //Save the list of allowed HTTP authentication schemes
   authContext->allowedAuthModes = allowedAuthModes;
#endif

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Bind the WebSocket to a particular network interface
 * @param[in] webSocket Handle to a WebSocket
 * @param[in] interface Network interface to be used
 * @return Error code
 **/

error_t webSocketBindToInterface(WebSocket *webSocket, NetInterface *interface)
{
   //Make sure the WebSocket handle is valid
   if(webSocket == NULL)
      return ERROR_INVALID_PARAMETER;

   //Explicitly associate the WebSocket with the specified interface
   webSocket->interface = interface;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Establish a WebSocket connection
 * @param[in] webSocket Handle to a WebSocket
 * @param[in] serverIpAddr IP address of the WebSocket server to connect to
 * @param[in] serverPort TCP port number that will be used to establish the
 *   connection
 * @param[in] uri NULL-terminated string that contains the resource name
 * @return Error code
 **/

error_t webSocketConnect(WebSocket *webSocket, const IpAddr *serverIpAddr,
   uint16_t serverPort, const char_t *uri)
{
   error_t error;
   size_t n;
   WebSocketFrameContext *txContext;

   //Make sure the WebSocket handle is valid
   if(webSocket == NULL)
      return ERROR_INVALID_PARAMETER;

   //Point to the TX context
   txContext = &webSocket->txContext;

   //Initialize status code
   error = NO_ERROR;

   //Establish connection
   while(webSocket->state != WS_STATE_OPEN)
   {
      //Check current state
      if(webSocket->state == WS_STATE_CLOSED)
      {
         //Check parameters
         if(serverIpAddr == NULL || uri == NULL)
         {
            //Report an error
            error = ERROR_INVALID_PARAMETER;
         }
         else
         {
            //A WebSocket client is a WebSocket endpoint that initiates a
            //connection to a peer
            webSocket->endpoint = WS_ENDPOINT_CLIENT;

            //Save the URI
            strSafeCopy(webSocket->uri, uri, WEB_SOCKET_URI_MAX_LEN);
            //Reset retry counter
            webSocket->retryCount = 0;

#if (WEB_SOCKET_BASIC_AUTH_SUPPORT == ENABLED || WEB_SOCKET_DIGEST_AUTH_SUPPORT == ENABLED)
            //HTTP authentication is not used for the first connection attempt
            webSocket->authContext.requiredAuthMode = WS_AUTH_MODE_NONE;
            webSocket->authContext.selectedAuthMode = WS_AUTH_MODE_NONE;
#endif
            //Initialize the WebSocket connection
            webSocketChangeState(webSocket, WS_STATE_INIT);
         }
      }
      else if(webSocket->state == WS_STATE_INIT)
      {
         //Increment retry counter
         webSocket->retryCount++;

         //Limit the number of connection attempts
         if(webSocket->retryCount > WEB_SOCKET_MAX_CONN_RETRIES)
         {
            //Report an error
            error = ERROR_OPEN_FAILED;
         }
         else
         {
            //Open network connection
            error = webSocketOpenConnection(webSocket);
         }

         //Check status code
         if(!error)
         {
            //Establish connection
            webSocketChangeState(webSocket, WS_STATE_CONNECTING);
         }
      }
      else if(webSocket->state == WS_STATE_CONNECTING)
      {
         //Establish connection
         error = webSocketEstablishConnection(webSocket,
            serverIpAddr, serverPort);

         //Check status code
         if(!error)
         {
            //Generate client's key
            error = webSocketGenerateClientKey(webSocket);
         }

         //Check status code
         if(!error)
         {
            //Format client handshake
            error = webSocketFormatClientHandshake(webSocket, serverPort);
         }

         //Check status code
         if(!error)
         {
            //Send client handshake
            webSocketChangeState(webSocket, WS_STATE_CLIENT_HANDSHAKE);
         }
      }
      else if(webSocket->state == WS_STATE_CLIENT_HANDSHAKE)
      {
         //Any remaining data to be sent?
         if(txContext->bufferPos < txContext->bufferLen)
         {
            //Send more data
            error = webSocketSendData(webSocket,
               txContext->buffer + txContext->bufferPos,
               txContext->bufferLen - txContext->bufferPos, &n, 0);

            //Advance data pointer
            txContext->bufferPos += n;
         }
         else
         {
            //Wait for server handshake
            webSocketChangeState(webSocket, WS_STATE_SERVER_HANDSHAKE);
         }
      }
      else if(webSocket->state == WS_STATE_SERVER_HANDSHAKE)
      {
         //Parse the server handshake
         error = webSocketParseHandshake(webSocket);
      }
      else if(webSocket->state == WS_STATE_SERVER_RESP_BODY)
      {
         //Check Connection header field
         if(webSocket->handshakeContext.connectionClose)
         {
            //Close connection
            webSocketCloseConnection(webSocket);
            //Try to connect again
            webSocketChangeState(webSocket, WS_STATE_INIT);
         }
         else
         {
            //Any remaining data to read in the response body?
            if(webSocket->handshakeContext.contentLength > 0)
            {
               //Limit the number of bytes to read at a time
               n = MIN(webSocket->handshakeContext.contentLength, WEB_SOCKET_BUFFER_SIZE);
               //Discard any received data
               error = webSocketReceiveData(webSocket, txContext->buffer, n, &n, 0);
               //Decrement byte counter
               webSocket->handshakeContext.contentLength -= n;
            }
            else
            {
               //Format client handshake
               error = webSocketFormatClientHandshake(webSocket, serverPort);
               //Try to authenticate again
               webSocketChangeState(webSocket, WS_STATE_CLIENT_HANDSHAKE);
            }
         }
      }
      else
      {
         //Invalid state
         error = ERROR_WRONG_STATE;
      }

      //Check whether authentication is required
      if(error == ERROR_AUTH_REQUIRED)
      {
#if (WEB_SOCKET_BASIC_AUTH_SUPPORT == ENABLED)
         //Basic authentication?
         if(webSocket->authContext.requiredAuthMode == WS_AUTH_MODE_BASIC ||
            webSocket->authContext.requiredAuthMode == WS_AUTH_MODE_NONE)
         {
            //Check whether the basic authentication scheme is allowed
            if(webSocket->authContext.allowedAuthModes & WS_AUTH_MODE_BASIC)
            {
               //Do not try to connect again if the credentials are not valid...
               if(webSocket->authContext.selectedAuthMode == WS_AUTH_MODE_NONE)
               {
                  //Catch exception
                  error = NO_ERROR;
                  //Force the WebSocket client to use basic authentication
                  webSocket->authContext.selectedAuthMode = WS_AUTH_MODE_BASIC;
                  //Read response body, if any
                  webSocketChangeState(webSocket, WS_STATE_SERVER_RESP_BODY);
               }
            }
         }
#endif
#if (WEB_SOCKET_DIGEST_AUTH_SUPPORT == ENABLED)
         //Digest authentication?
         if(webSocket->authContext.requiredAuthMode == WS_AUTH_MODE_DIGEST)
         {
            //Check whether the digest authentication scheme is allowed
            if(webSocket->authContext.allowedAuthModes & WS_AUTH_MODE_DIGEST)
            {
               //Do not try to connect again if the credentials are not valid...
               if(webSocket->authContext.selectedAuthMode == WS_AUTH_MODE_NONE)
               {
                  //Force the WebSocket client to use digest authentication
                  webSocket->authContext.selectedAuthMode = WS_AUTH_MODE_DIGEST;

                  //Make sure that the RNG callback function has been registered
                  if(webSockRandCallback != NULL)
                  {
                     //Generate a random cnonce
                     error = webSockRandCallback(txContext->buffer,
                        WEB_SOCKET_CNONCE_SIZE);
                  }
                  else
                  {
                     //A cryptographically strong random number generator
                     //must be used to generate the cnonce
                     error = ERROR_PRNG_NOT_READY;
                  }

                  //Convert the byte array to hex string
                  webSocketConvertArrayToHexString(txContext->buffer,
                     WEB_SOCKET_CNONCE_SIZE, webSocket->authContext.cnonce);

                  //Read response body, if any
                  webSocketChangeState(webSocket, WS_STATE_SERVER_RESP_BODY);
               }
            }
         }
#endif
      }

      //If an error occurred, then the client must fail the WebSocket
      //connection
      if(error)
      {
#if (NET_RTOS_SUPPORT == DISABLED)
         //Timeout error?
         if(error == ERROR_WOULD_BLOCK || error == ERROR_TIMEOUT)
            break;
#endif
         //Close connection
         webSocketCloseConnection(webSocket);

         //Switch to the CLOSED state
         webSocketChangeState(webSocket, WS_STATE_CLOSED);
         //Exit immediately
         break;
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Set client's key
 * @param[in] webSocket Handle to a WebSocket
 * @param[in] clientKey NULL-terminated string that holds the the client's key
 * @return Error code
 **/

error_t webSocketSetClientKey(WebSocket *webSocket, const char_t *clientKey)
{
   error_t error;
   size_t n;

   //Check parameters
   if(webSocket == NULL || clientKey == NULL)
      return ERROR_INVALID_PARAMETER;

   //Get the length of the client's key
   n = osStrlen(clientKey);

   //Check the length of the key
   if(n > WEB_SOCKET_CLIENT_KEY_SIZE)
      return ERROR_INVALID_LENGTH;

   //Copy client's key
   osStrcpy(webSocket->handshakeContext.clientKey, clientKey);

   //a WebSocket server is a WebSocket endpoint that awaits
   //connections from peers
   webSocket->endpoint = WS_ENDPOINT_SERVER;

   //Initialize status code
   webSocket->statusCode = WS_STATUS_CODE_NO_STATUS_RCVD;

   //Initialize handshake parameters
   webSocket->handshakeContext.version = WS_HTTP_VERSION_1_1;
   webSocket->handshakeContext.connectionUpgrade = TRUE;
   webSocket->handshakeContext.upgradeWebSocket = TRUE;

   //Initialize FIN flag
   webSocket->rxContext.fin = TRUE;

   //Verify client's key
   error = webSocketVerifyClientKey(webSocket);
   //Any error to report?
   if(error)
      return error;

   //Generate server's key
   error = webSocketGenerateServerKey(webSocket);
   //Any error to report?
   if(error)
      return error;

   //Format server handshake
   error = webSocketFormatServerHandshake(webSocket);
   //Any error to report?
   if(error)
      return error;

   //Update FSM state
   webSocket->state = WS_STATE_SERVER_HANDSHAKE;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse client's handshake
 * @param[in] webSocket Handle that identifies a WebSocket
 * @return Error code
 **/

error_t webSocketParseClientHandshake(WebSocket *webSocket)
{
   error_t error;

   //Make sure the WebSocket handle is valid
   if(webSocket == NULL)
      return ERROR_INVALID_PARAMETER;

   //a WebSocket server is a WebSocket endpoint that awaits
   //connections from peers
   webSocket->endpoint = WS_ENDPOINT_SERVER;

   //Initialize status code
   error = NO_ERROR;

   //Establish connection
   while(webSocket->state != WS_STATE_SERVER_HANDSHAKE)
   {
      //Check current state
      if(webSocket->state == WS_STATE_INIT)
      {
         //Open network connection
         error = webSocketOpenConnection(webSocket);

         //Check status code
         if(!error)
         {
            //Establish connection
            webSocketChangeState(webSocket, WS_STATE_CONNECTING);
         }
      }
      else if(webSocket->state == WS_STATE_CONNECTING)
      {
#if (WEB_SOCKET_TLS_SUPPORT == ENABLED)
         //TLS-secured connection?
         if(webSocket->tlsInitCallback != NULL)
         {
            //Establish TLS connection
            error = tlsConnect(webSocket->tlsContext);
         }
#endif
         //Check status code
         if(!error)
         {
            //Parse client handshake
            webSocketChangeState(webSocket, WS_STATE_CLIENT_HANDSHAKE);
         }
      }
      else if(webSocket->state == WS_STATE_CLIENT_HANDSHAKE)
      {
         //Parse the client handshake
         error = webSocketParseHandshake(webSocket);

         //Check status code
         if(!error)
         {
            //Generate server's key
            error = webSocketGenerateServerKey(webSocket);
         }

         //Check status code
         if(!error)
         {
            //Format server handshake
            error = webSocketFormatServerHandshake(webSocket);
         }
      }
      else
      {
         //Invalid state
         error = ERROR_WRONG_STATE;
      }

      //Any error to report?
      if(error)
         break;
   }

   //Return status code
   return error;
}


/**
 * @brief Send server's handshake
 * @param[in] webSocket Handle that identifies a WebSocket
 * @return Error code
 **/

error_t webSocketSendServerHandshake(WebSocket *webSocket)
{
   error_t error;
   size_t n;
   WebSocketFrameContext *txContext;

   //Make sure the WebSocket handle is valid
   if(webSocket == NULL)
      return ERROR_INVALID_PARAMETER;

   //Point to the TX context
   txContext = &webSocket->txContext;

   //Initialize status code
   error = NO_ERROR;

   //Establish connection
   while(webSocket->state != WS_STATE_OPEN)
   {
      //Check current state
      if(webSocket->state == WS_STATE_SERVER_HANDSHAKE)
      {
         //Any remaining data to be sent?
         if(txContext->bufferPos < txContext->bufferLen)
         {
            //Send more data
            error = webSocketSendData(webSocket,
               txContext->buffer + txContext->bufferPos,
               txContext->bufferLen - txContext->bufferPos, &n, 0);

            //Advance data pointer
            txContext->bufferPos += n;
         }
         else
         {
            //The WebSocket connection is established
            webSocketChangeState(webSocket, WS_STATE_OPEN);
         }
      }
      else
      {
         //Invalid state
         error = ERROR_WRONG_STATE;
      }

      //Any error to report?
      if(error)
         break;
   }

   //Return status code
   return error;
}


/**
 * @brief Send HTTP error response to the client
 * @param[in] webSocket Handle that identifies a WebSocket
 * @param[in] statusCode HTTP status code
 * @param[in] message Text message
 * @return Error code
 **/

error_t webSocketSendErrorResponse(WebSocket *webSocket,
   uint_t statusCode, const char_t *message)
{
   error_t error;
   size_t n;
   WebSocketFrameContext *txContext;

   //Make sure the WebSocket handle is valid
   if(webSocket == NULL)
      return ERROR_INVALID_PARAMETER;

   //Point to the TX context
   txContext = &webSocket->txContext;

   //Initialize status code
   error = NO_ERROR;

   //Send HTTP error message
   while(1)
   {
      //Check current state
      if(txContext->state == WS_SUB_STATE_INIT)
      {
         //Format HTTP error response
         error = webSocketFormatErrorResponse(webSocket, statusCode, message);

         //Send the response
         txContext->state = WS_SUB_STATE_FRAME_PAYLOAD;
      }
      else if(txContext->state == WS_SUB_STATE_FRAME_PAYLOAD)
      {
         //Any remaining data to be sent?
         if(txContext->bufferPos < txContext->bufferLen)
         {
            //Send more data
            error = webSocketSendData(webSocket,
               txContext->buffer + txContext->bufferPos,
               txContext->bufferLen - txContext->bufferPos, &n, 0);

            //Advance data pointer
            txContext->bufferPos += n;
         }
         else
         {
            //We are done
            webSocketChangeState(webSocket, WS_STATE_SHUTDOWN);
            break;
         }
      }
      else
      {
         //Invalid state
         error = ERROR_WRONG_STATE;
      }

      //Any error to report?
      if(error)
         break;
   }

   //Return status code
   return error;
}


/**
 * @brief Transmit data over the WebSocket connection
 * @param[in] webSocket Handle that identifies a WebSocket
 * @param[in] data Pointer to a buffer containing the data to be transmitted
 * @param[in] length Number of data bytes to send
 * @param[in] type Frame type
 * @param[out] written Actual number of bytes written (optional parameter)
 * @return Error code
 **/

error_t webSocketSend(WebSocket *webSocket, const void *data,
   size_t length, WebSocketFrameType type, size_t *written)
{
   //An unfragmented message consists of a single frame with the FIN bit
   //set and an opcode other than 0
   return webSocketSendEx(webSocket, data, length,
      type, written, TRUE, TRUE);
}


/**
 * @brief Transmit data over the WebSocket connection
 * @param[in] webSocket Handle that identifies a WebSocket
 * @param[in] data Pointer to a buffer containing the data to be transmitted
 * @param[in] length Number of data bytes to send
 * @param[in] type Frame type
 * @param[out] written Actual number of bytes written (optional parameter)
 * @param[in] firstFrag First fragment of the message
 * @param[in] lastFrag Last fragment of the message
 **/

error_t webSocketSendEx(WebSocket *webSocket, const void *data, size_t length,
   WebSocketFrameType type, size_t *written, bool_t firstFrag, bool_t lastFrag)
{
   error_t error;
   size_t i;
   size_t j;
   size_t k;
   size_t n;
   const uint8_t *p;
   WebSocketFrameContext *txContext;

   //Check parameters
   if(webSocket == NULL || data == NULL)
      return ERROR_INVALID_PARAMETER;

   //A data frame may be transmitted by either the client or the server at
   //any time after opening handshake completion and before that endpoint
   //has sent a Close frame
   if(webSocket->state != WS_STATE_OPEN)
      return ERROR_NOT_CONNECTED;

   //Point to the TX context
   txContext = &webSocket->txContext;

   //Initialize status code
   error = NO_ERROR;

   //Point to the application data to be written
   p = (const uint8_t *) data;
   //No data has been transmitted yet
   i = 0;

   //Send as much data as possible
   while(1)
   {
      //Check current sub-state
      if(txContext->state == WS_SUB_STATE_INIT)
      {
         //A fragmented message consists of a single frame with the FIN bit
         //clear and an opcode other than 0, followed by zero or more frames
         //with the FIN bit clear and the opcode set to 0, and terminated by
         //a single frame with the FIN bit set and an opcode of 0
         if(!firstFrag)
            type = WS_FRAME_TYPE_CONTINUATION;

         //Format WebSocket frame header
         error = webSocketFormatFrameHeader(webSocket, lastFrag, type, length - i);

         //Send the frame header
         txContext->state = WS_SUB_STATE_FRAME_HEADER;
      }
      else if(txContext->state == WS_SUB_STATE_FRAME_HEADER)
      {
         //Any remaining data to be sent?
         if(txContext->bufferPos < txContext->bufferLen)
         {
            //Send more data
            error = webSocketSendData(webSocket,
               txContext->buffer + txContext->bufferPos,
               txContext->bufferLen - txContext->bufferPos, &n, 0);

            //Advance data pointer
            txContext->bufferPos += n;
         }
         else
         {
            //Flush the transmit buffer
            txContext->payloadPos = 0;
            txContext->bufferPos = 0;
            txContext->bufferLen = 0;

            //Send the payload of the WebSocket frame
            txContext->state = WS_SUB_STATE_FRAME_PAYLOAD;
         }
      }
      else if(txContext->state == WS_SUB_STATE_FRAME_PAYLOAD)
      {
         //Any remaining data to be sent?
         if(txContext->bufferPos < txContext->bufferLen)
         {
            //Send more data
            error = webSocketSendData(webSocket,
               txContext->buffer + txContext->bufferPos,
               txContext->bufferLen - txContext->bufferPos, &n, 0);

            //Advance data pointer
            txContext->payloadPos += n;
            txContext->bufferPos += n;

            //Total number of data that have been written
            i += n;
         }
         else
         {
            //Send as much data as possible
            if(txContext->payloadPos < txContext->payloadLen)
            {
               //Calculate the number of bytes that are pending
               n = MIN(length - i, txContext->payloadLen - txContext->payloadPos);
               //Limit the number of bytes to be copied at a time
               n = MIN(n, WEB_SOCKET_BUFFER_SIZE);

               //Copy application data to the transmit buffer
               osMemcpy(txContext->buffer, p + i, n);

               //All frames sent from the client to the server are masked
               if(webSocket->endpoint == WS_ENDPOINT_CLIENT)
               {
                  //Apply masking
                  for(j = 0; j < n; j++)
                  {
                     //Index of the masking key to be applied
                     k = (txContext->payloadPos + j) % 4;
                     //Convert unmasked data into masked data
                     txContext->buffer[j] ^= txContext->maskingKey[k];
                  }
               }

               //Rewind to the beginning of the buffer
               txContext->bufferPos = 0;
               //Update the number of data buffered but not yet sent
               txContext->bufferLen = n;
            }
            else
            {
               //Prepare to send a new WebSocket frame
               txContext->state = WS_SUB_STATE_INIT;

               //Write operation complete?
               if(i >= length)
                  break;
            }
         }
      }
      else
      {
         //Invalid state
         error = ERROR_WRONG_STATE;
      }

      //Any error to report?
      if(error)
         break;
   }

   //Total number of data that have been written
   if(written != NULL)
      *written = i;

   //Return status code
   return error;
}


/**
 * @brief Receive data from a WebSocket connection
 * @param[in] webSocket Handle that identifies a WebSocket
 * @param[out] data Buffer where to store the incoming data
 * @param[in] size Maximum number of bytes that can be received
 * @param[out] type Frame type
 * @param[out] received Number of bytes that have been received
 * @return Error code
 **/

error_t webSocketReceive(WebSocket *webSocket, void *data,
   size_t size, WebSocketFrameType *type, size_t *received)
{
   bool_t firstFrag;
   bool_t lastFrag;

   return webSocketReceiveEx(webSocket, data, size,
      type, received, &firstFrag, &lastFrag);
}


/**
 * @brief Receive data from a WebSocket connection
 * @param[in] webSocket Handle that identifies a WebSocket
 * @param[out] data Buffer where to store the incoming data
 * @param[in] size Maximum number of bytes that can be received
 * @param[out] type Frame type
 * @param[out] received Number of bytes that have been received
 * @param[out] firstFrag First fragment of the message
 * @param[out] lastFrag Last fragment of the message
 * @return Error code
 **/

error_t webSocketReceiveEx(WebSocket *webSocket, void *data, size_t size,
   WebSocketFrameType *type, size_t *received, bool_t *firstFrag, bool_t *lastFrag)
{
   error_t error;
   size_t i;
   size_t j;
   size_t k;
   size_t n;
   WebSocketFrame *frame;
   WebSocketFrameContext *rxContext;

   //Make sure the WebSocket handle is valid
   if(webSocket == NULL)
      return ERROR_INVALID_PARAMETER;

   //Check the state of the WebSocket connection
   if(webSocket->state != WS_STATE_OPEN &&
      webSocket->state != WS_STATE_CLOSING_RX)
      return ERROR_NOT_CONNECTED;

   //Point to the RX context
   rxContext = &webSocket->rxContext;
   //Point to the WebSocket frame header
   frame = (WebSocketFrame *) rxContext->buffer;

   //Initialize status code
   error = NO_ERROR;

   //Initialize flags
   if(type != NULL)
      *type = WS_FRAME_TYPE_CONTINUATION;
   if(firstFrag != NULL)
      *firstFrag = FALSE;
   if(lastFrag != NULL)
      *lastFrag = FALSE;

   //No data has been read yet
   i = 0;

   //Read as much data as possible
   while(i < size)
   {
      //Check current sub-state
      if(rxContext->state == WS_SUB_STATE_INIT)
      {
         //Flush the receive buffer
         rxContext->bufferPos = 0;
         rxContext->bufferLen = sizeof(WebSocketFrame);

         //Decode the frame header
         rxContext->state = WS_SUB_STATE_FRAME_HEADER;
      }
      else if(rxContext->state == WS_SUB_STATE_FRAME_HEADER)
      {
         //Incomplete frame header?
         if(rxContext->bufferPos < rxContext->bufferLen)
         {
            //Read more data
            error = webSocketReceiveData(webSocket,
               rxContext->buffer + rxContext->bufferPos,
               rxContext->bufferLen - rxContext->bufferPos, &n, 0);

            //Advance data pointer
            rxContext->bufferPos += n;
         }
         else
         {
            //Check the Payload Length field
            if(frame->payloadLen == 126)
               rxContext->bufferLen += sizeof(uint16_t);
            else if(frame->payloadLen == 127)
               rxContext->bufferLen += sizeof(uint64_t);

            //Check whether the masking key is present
            if(frame->mask)
               rxContext->bufferLen += sizeof(uint32_t);

            //The Opcode field defines the interpretation of the payload data
            if(frame->opcode == WS_FRAME_TYPE_CLOSE)
            {
               //All control frames must have a payload length of 125 bytes or less
               if(frame->payloadLen <= 125)
               {
                  //Retrieve the length of the WebSocket frame
                  rxContext->bufferLen += frame->payloadLen;
               }
               else
               {
                  //Report a protocol error
                  webSocket->statusCode = WS_STATUS_CODE_PROTOCOL_ERROR;
                  //Terminate the WebSocket connection
                  error = ERROR_INVALID_FRAME;
               }
            }

            //Decode the extended payload length and the masking key, if any
            rxContext->state = WS_SUB_STATE_FRAME_EXT_HEADER;
         }
      }
      else if(rxContext->state == WS_SUB_STATE_FRAME_EXT_HEADER)
      {
         //Incomplete frame header?
         if(rxContext->bufferPos < rxContext->bufferLen)
         {
            //Read more data
            error = webSocketReceiveData(webSocket,
               rxContext->buffer + rxContext->bufferPos,
               rxContext->bufferLen - rxContext->bufferPos, &n, 0);

            //Advance data pointer
            rxContext->bufferPos += n;
         }
         else
         {
            //Parse the header of the WebSocket frame
            error = webSocketParseFrameHeader(webSocket, frame, type);

            //Check status code
            if(error == ERROR_UNEXPECTED_MESSAGE)
            {
               error = NO_ERROR;
               break;
            }
            else if(error == NO_ERROR)
            {
               if(firstFrag != NULL)
                  *firstFrag = TRUE;
            }

            //Flush the receive buffer
            rxContext->payloadPos = 0;
            rxContext->bufferPos = 0;
            rxContext->bufferLen = 0;

            //Decode the payload of the WebSocket frame
            rxContext->state = WS_SUB_STATE_FRAME_PAYLOAD;
         }
      }
      else if(rxContext->state == WS_SUB_STATE_FRAME_PAYLOAD)
      {
         if(rxContext->payloadPos < rxContext->payloadLen)
         {
            //Limit the number of bytes to read at a time
            n = MIN(size - i, rxContext->payloadLen - rxContext->payloadPos);
            //Limit the number of bytes to be copied at a time
            n = MIN(n, WEB_SOCKET_BUFFER_SIZE);

            //Read more data
            error = webSocketReceiveData(webSocket, rxContext->buffer, n, &n, 0);

            //All frames sent from the client to the server are masked
            if(rxContext->mask)
            {
               //Unmask the data
               for(j = 0; j < n; j++)
               {
                  //Index of the masking key to be applied
                  k = (rxContext->payloadPos + j) % 4;
                  //Convert masked data into unmasked data
                  rxContext->buffer[j] ^= rxContext->maskingKey[k];
               }
            }

            //Text frame?
            if(rxContext->dataFrameType == WS_FRAME_TYPE_TEXT &&
               rxContext->controlFrameType == WS_FRAME_TYPE_CONTINUATION)
            {
               //Compute the number of remaining data bytes in the UTF-8 stream
               if(rxContext->fin)
                  k = rxContext->payloadLen - rxContext->payloadPos;
               else
                  k = 0;

               //Invalid UTF-8 sequence?
               if(!webSocketCheckUtf8Stream(&webSocket->utf8Context,
                  rxContext->buffer, n, k))
               {
                  //The received data is not consistent with the type of the message
                  webSocket->statusCode = WS_STATUS_CODE_INVALID_PAYLOAD_DATA;
                  //The endpoint must fail the WebSocket connection
                  error = ERROR_INVALID_FRAME;
               }
            }

            //Sanity check
            if(data != NULL)
            {
               //Copy application data
               osMemcpy((uint8_t *) data + i, rxContext->buffer, n);
            }

            //Advance data pointer
            rxContext->payloadPos += n;

            //Total number of data that have been read
            i += n;
         }

         if(rxContext->payloadPos == rxContext->payloadLen)
         {
            //Decode the next WebSocket frame
            rxContext->state = WS_SUB_STATE_INIT;

            //Last fragment of the message?
            if(rxContext->fin || rxContext->controlFrameType != WS_FRAME_TYPE_CONTINUATION)
            {
               if(lastFrag != NULL)
                  *lastFrag = TRUE;

               //Exit immediately
               break;
            }
         }
      }
      else
      {
         //Invalid state
         error = ERROR_WRONG_STATE;
      }

      //Any error to report?
      if(error)
         break;
   }

   //Check status code
   if(!error)
   {
      //Return the frame type
      if(type != NULL)
      {
         //Control or data frame?
         if(rxContext->controlFrameType != WS_FRAME_TYPE_CONTINUATION)
            *type = rxContext->controlFrameType;
         else
            *type = rxContext->dataFrameType;
      }
   }

   //Return the total number of data that have been read
   if(received != NULL)
      *received = i;

   //Return status code
   return error;
}


/**
 * @brief Check whether some data is available in the receive buffer
 * @param[in] webSocket Handle to a WebSocket
 * @return The function returns TRUE if some data is pending and can be read
 *   immediately without blocking. Otherwise, FALSE is returned
 **/

bool_t webSocketIsRxReady(WebSocket *webSocket)
{
   bool_t available = FALSE;

#if (WEB_SOCKET_TLS_SUPPORT == ENABLED)
   //Check whether a secure connection is being used
   if(webSocket->tlsContext != NULL)
   {
      //Check whether some data is pending in the receive buffer
      if(webSocket->tlsContext->rxBufferLen > 0)
         available = TRUE;
   }
#endif

   //The function returns TRUE if some data can be read immediately
   //without blocking
   return available;
}


/**
 * @brief Gracefully close a WebSocket connection
 * @param[in] webSocket Handle to a WebSocket
 **/

error_t webSocketShutdown(WebSocket *webSocket)
{
   error_t error;
   size_t n;
   WebSocketFrameContext *txContext;

   //Make sure the WebSocket handle is valid
   if(webSocket == NULL)
      return ERROR_INVALID_PARAMETER;

   //Point to the TX context
   txContext = &webSocket->txContext;

   //Initialize status code
   error = NO_ERROR;

   //Closing handshake
   while(webSocket->state != WS_STATE_CLOSED)
   {
      //Check current state
      if(webSocket->state == WS_STATE_OPEN)
      {
         //Check whether the latest frame has been completely transmitted
         if(txContext->payloadPos == txContext->payloadLen)
         {
            //Format Close frame
            error = webSocketFormatCloseFrame(webSocket);
            //Send Close frame
            webSocket->state = WS_STATE_CLOSING_TX;
         }
         else
         {
            //The WebSocket connection cannot be closed until the
            //transmission of the frame is complete...
            error = ERROR_FAILURE;
         }
      }
      else if(webSocket->state == WS_STATE_CLOSING_TX)
      {
         //Any remaining data to be sent?
         if(txContext->bufferPos < txContext->bufferLen)
         {
            //Send more data
            error = webSocketSendData(webSocket,
               txContext->buffer + txContext->bufferPos,
               txContext->bufferLen - txContext->bufferPos, &n, 0);

            //Advance data pointer
            txContext->bufferPos += n;
         }
         else
         {
            //Check whether a Close frame has been received from the peer
            if(webSocket->handshakeContext.closingFrameReceived)
               webSocket->state = WS_STATE_SHUTDOWN;
            else
               webSocket->state = WS_STATE_CLOSING_RX;
         }
      }
      else if(webSocket->state == WS_STATE_CLOSING_RX)
      {
         //After receiving a control frame indicating the connection should
         //be closed, a peer discards any further data received
         error = webSocketReceive(webSocket, NULL, WEB_SOCKET_BUFFER_SIZE, NULL, 0);

         //Check status code
         if(error == NO_ERROR)
         {
            //Close frame received?
            if(webSocket->handshakeContext.closingFrameReceived)
            {
               //Properly shutdown the network connection
               webSocket->state = WS_STATE_SHUTDOWN;
            }
         }
         else if(error == ERROR_INVALID_FRAME || error == ERROR_END_OF_STREAM)
         {
            //Catch exception
            error = NO_ERROR;
            //Properly shutdown the network connection
            webSocket->state = WS_STATE_SHUTDOWN;
         }
      }
      else if(webSocket->state == WS_STATE_SHUTDOWN)
      {
         //Properly dispose the network connection
         error = webSocketShutdownConnection(webSocket);

         //Check status code
         if(!error)
         {
            //The connection has been properly closed
            webSocket->state = WS_STATE_CLOSED;
         }
      }
      else
      {
         //Invalid state
         error = ERROR_WRONG_STATE;
      }

      //Any error to report?
      if(error)
         break;
   }

   //Return status code
   return error;
}


/**
 * @brief Close a WebSocket connection
 * @param[in] webSocket Handle identifying the WebSocket to close
 **/

void webSocketClose(WebSocket *webSocket)
{
   //Make sure the WebSocket handle is valid
   if(webSocket != NULL)
   {
      //Close connection
      webSocketCloseConnection(webSocket);

#if (WEB_SOCKET_TLS_SUPPORT == ENABLED)
      //Release TLS session state
      tlsFreeSessionState(&webSocket->tlsSession);
#endif

      //Release the WebSocket
      webSocketChangeState(webSocket, WS_STATE_UNUSED);
   }
}

#endif
