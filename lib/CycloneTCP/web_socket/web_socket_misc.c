/**
 * @file web_socket_misc.c
 * @brief Helper functions for WebSockets
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
#include "encoding/base64.h"
#include "hash/sha1.h"
#include "str.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (WEB_SOCKET_SUPPORT == ENABLED)

//WebSocket GUID
const char_t webSocketGuid[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";


/**
 * @brief HTTP status codes
 **/

static const WebSocketStatusCodeDesc statusCodeList[] =
{
   //Success
   {200, "OK"},
   {201, "Created"},
   {202, "Accepted"},
   {204, "No Content"},
   //Redirection
   {301, "Moved Permanently"},
   {302, "Found"},
   {304, "Not Modified"},
   //Client error
   {400, "Bad Request"},
   {401, "Unauthorized"},
   {403, "Forbidden"},
   {404, "Not Found"},
   //Server error
   {500, "Internal Server Error"},
   {501, "Not Implemented"},
   {502, "Bad Gateway"},
   {503, "Service Unavailable"}
};


/**
 * @brief Update WebSocket state
 * @param[in] webSocket Handle to a WebSocket
 * @param[in] newState New state to switch to
 **/

void webSocketChangeState(WebSocket *webSocket, WebSocketState newState)
{
   //Switch to the new state
   webSocket->state = newState;
   //Save current time;
   webSocket->timestamp = osGetSystemTime();

   //Reset sub-state
   webSocket->txContext.state = WS_SUB_STATE_INIT;
   webSocket->rxContext.state = WS_SUB_STATE_INIT;
}


/**
 * @brief Parse client or server handshake
 * @param[in] webSocket Handle to a WebSocket
 * @return Error code
 **/

error_t webSocketParseHandshake(WebSocket *webSocket)
{
   error_t error;
   size_t n;
   WebSocketFrameContext *rxContext;

   //Point to the RX context
   rxContext = &webSocket->rxContext;

   //Initialize status code
   error = NO_ERROR;

   //Wait for the handshake to complete
   while(1)
   {
      //Client or server operation?
      if(webSocket->endpoint == WS_ENDPOINT_CLIENT)
      {
         if(webSocket->state == WS_STATE_OPEN)
            break;
      }
      else
      {
         if(webSocket->state == WS_STATE_SERVER_HANDSHAKE)
            break;
      }

      //Check current sub-state
      if(rxContext->state == WS_SUB_STATE_INIT)
      {
         //Initialize status code
         webSocket->statusCode = WS_STATUS_CODE_NO_STATUS_RCVD;

         //Initialize FIN flag
         rxContext->fin = TRUE;

         //Flush the receive buffer
         rxContext->bufferPos = 0;
         rxContext->bufferLen = 0;

         //Initialize variables
         webSocket->handshakeContext.statusCode = 0;
         webSocket->handshakeContext.upgradeWebSocket = FALSE;
         webSocket->handshakeContext.connectionUpgrade = FALSE;
         webSocket->handshakeContext.connectionClose = FALSE;
         webSocket->handshakeContext.contentLength = 0;
         webSocket->handshakeContext.closingFrameSent = FALSE;
         webSocket->handshakeContext.closingFrameReceived = FALSE;

#if (WEB_SOCKET_BASIC_AUTH_SUPPORT == ENABLED || WEB_SOCKET_DIGEST_AUTH_SUPPORT == ENABLED)
         webSocket->authContext.requiredAuthMode = WS_AUTH_MODE_NONE;
#endif

#if (WEB_SOCKET_DIGEST_AUTH_SUPPORT == ENABLED)
         osStrcpy(webSocket->authContext.nonce, "");
         osStrcpy(webSocket->authContext.opaque, "");
         webSocket->authContext.stale = FALSE;
#endif

         //Client or server operation?
         if(webSocket->endpoint == WS_ENDPOINT_CLIENT)
         {
            //Clear server key
            osStrcpy(webSocket->handshakeContext.serverKey, "");

            //Debug message
            TRACE_DEBUG("WebSocket: server handshake\r\n");
         }
         else
         {
            //Clear client key
            osStrcpy(webSocket->handshakeContext.clientKey, "");

            //Debug message
            TRACE_DEBUG("WebSocket: client handshake\r\n");
         }

         //Decode the leading line
         rxContext->state = WS_SUB_STATE_HANDSHAKE_LEADING_LINE;
      }
      else if(rxContext->state == WS_SUB_STATE_HANDSHAKE_LEADING_LINE)
      {
         //Check whether more data is required
         if(rxContext->bufferLen < 2)
         {
            //Limit the number of characters to read at a time
            n = WEB_SOCKET_BUFFER_SIZE - 1 - rxContext->bufferLen;

            //Read data until a CLRF character is encountered
            error = webSocketReceiveData(webSocket, rxContext->buffer +
               rxContext->bufferLen, n, &n, SOCKET_FLAG_BREAK_CRLF);

            //Update the length of the buffer
            rxContext->bufferLen += n;
         }
         else if(rxContext->bufferLen >= (WEB_SOCKET_BUFFER_SIZE - 1))
         {
            //Report an error
            error = ERROR_INVALID_REQUEST;
         }
         else if(osStrncmp((char_t *) rxContext->buffer + rxContext->bufferLen - 2, "\r\n", 2))
         {
            //Limit the number of characters to read at a time
            n = WEB_SOCKET_BUFFER_SIZE - 1 - rxContext->bufferLen;

            //Read data until a CLRF character is encountered
            error = webSocketReceiveData(webSocket, rxContext->buffer +
               rxContext->bufferLen, n, &n, SOCKET_FLAG_BREAK_CRLF);

            //Update the length of the buffer
            rxContext->bufferLen += n;
         }
         else
         {
            //Properly terminate the string with a NULL character
            rxContext->buffer[rxContext->bufferLen] = '\0';

            //Client or server operation?
            if(webSocket->endpoint == WS_ENDPOINT_CLIENT)
            {
               //The leading line from the server follows the Status-Line format
               error = webSocketParseStatusLine(webSocket, (char_t *) rxContext->buffer);
            }
            else
            {
               //The leading line from the client follows the Request-Line format
               error = webSocketParseRequestLine(webSocket, (char_t *) rxContext->buffer);
            }

            //Flush the receive buffer
            rxContext->bufferPos = 0;
            rxContext->bufferLen = 0;

            //Parse the header fields of the handshake
            rxContext->state = WS_SUB_STATE_HANDSHAKE_HEADER_FIELD;
         }
      }
      else if(rxContext->state == WS_SUB_STATE_HANDSHAKE_HEADER_FIELD)
      {
         //Check whether more data is required
         if(rxContext->bufferLen < 2)
         {
            //Limit the number of characters to read at a time
            n = WEB_SOCKET_BUFFER_SIZE - 1 - rxContext->bufferLen;

            //Read data until a CLRF character is encountered
            error = webSocketReceiveData(webSocket, rxContext->buffer +
               rxContext->bufferLen, n, &n, SOCKET_FLAG_BREAK_CRLF);

            //Update the length of the buffer
            rxContext->bufferLen += n;
         }
         else if(rxContext->bufferLen >= (WEB_SOCKET_BUFFER_SIZE - 1))
         {
            //Report an error
            error = ERROR_INVALID_REQUEST;
         }
         else if(osStrncmp((char_t *) rxContext->buffer + rxContext->bufferLen - 2, "\r\n", 2))
         {
            //Limit the number of characters to read at a time
            n = WEB_SOCKET_BUFFER_SIZE - 1 - rxContext->bufferLen;

            //Read data until a CLRF character is encountered
            error = webSocketReceiveData(webSocket, rxContext->buffer +
               rxContext->bufferLen, n, &n, SOCKET_FLAG_BREAK_CRLF);

            //Update the length of the buffer
            rxContext->bufferLen += n;
         }
         else
         {
            //Properly terminate the string with a NULL character
            rxContext->buffer[rxContext->bufferLen] = '\0';

            //An empty line indicates the end of the header fields
            if(!osStrcmp((char_t *) rxContext->buffer, "\r\n"))
            {
               //Client or server operation?
               if(webSocket->endpoint == WS_ENDPOINT_CLIENT)
               {
                  //Verify server's handshake
                  error = webSocketVerifyServerHandshake(webSocket);
               }
               else
               {
                  //Verify client's handshake
                  error = webSocketVerifyClientHandshake(webSocket);
               }
            }
            else
            {
               //Read the next character to detect if the CRLF is immediately
               //followed by a LWSP character
               rxContext->state = WS_SUB_STATE_HANDSHAKE_LWSP;
            }
         }
      }
      else if(rxContext->state == WS_SUB_STATE_HANDSHAKE_LWSP)
      {
         char_t nextChar;

         //Read the next character
         error = webSocketReceiveData(webSocket, &nextChar, sizeof(char_t), &n, 0);

         //Successful read operation?
         if(!error && n == sizeof(char_t))
         {
            //LWSP character found?
            if(nextChar == ' ' || nextChar == '\t')
            {
               //Unfolding is accomplished by regarding CRLF immediately
               //followed by a LWSP as equivalent to the LWSP character
               if(rxContext->bufferLen >= 2)
               {
                  //Remove trailing CRLF sequence
                  rxContext->bufferLen -= 2;
               }

               //The header field spans multiple line
               rxContext->state = WS_SUB_STATE_HANDSHAKE_HEADER_FIELD;
            }
            else
            {
               //Parse header field
               error = webSocketParseHeaderField(webSocket, (char_t *) rxContext->buffer);

               //Restore the very first character of the header field
               rxContext->buffer[0] = nextChar;
               //Adjust the length of the receive buffer
               rxContext->bufferLen = sizeof(char_t);

               //Decode the next header field
               rxContext->state = WS_SUB_STATE_HANDSHAKE_HEADER_FIELD;
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

   //Return status code
   return error;
}


/**
 * @brief Parse the Request-Line of the client's handshake
 * @param[in] webSocket Handle to a WebSocket
 * @param[in] line NULL-terminated string that contains the Request-Line
 * @return Error code
 **/

error_t webSocketParseRequestLine(WebSocket *webSocket, char_t *line)
{
   error_t error;
   char_t *token;
   char_t *p;
   char_t *s;

   //Debug message
   TRACE_DEBUG("%s", line);

   //The Request-Line begins with a method token
   token = osStrtok_r(line, " \r\n", &p);
   //Unable to retrieve the method?
   if(token == NULL)
      return ERROR_INVALID_REQUEST;

   //The method of the request must be GET
   if(osStrcasecmp(token, "GET"))
      return ERROR_INVALID_REQUEST;

   //The Request-URI is following the method token
   token = osStrtok_r(NULL, " \r\n", &p);
   //Unable to retrieve the Request-URI?
   if(token == NULL)
      return ERROR_INVALID_REQUEST;

   //Check whether a query string is present
   s = osStrchr(token, '?');

   //Query string found?
   if(s != NULL)
   {
      //Split the string
      *s = '\0';

      //Save the Request-URI
      error = webSocketDecodePercentEncodedString(token,
         webSocket->uri, WEB_SOCKET_URI_MAX_LEN);
      //Any error to report?
      if(error)
         return ERROR_INVALID_REQUEST;

      //Check the length of the query string
      if(osStrlen(s + 1) > WEB_SOCKET_QUERY_STRING_MAX_LEN)
         return ERROR_INVALID_REQUEST;

      //Save the query string
      osStrcpy(webSocket->queryString, s + 1);
   }
   else
   {
      //Save the Request-URI
      error = webSocketDecodePercentEncodedString(token,
         webSocket->uri, WEB_SOCKET_URI_MAX_LEN);
      //Any error to report?
      if(error)
         return ERROR_INVALID_REQUEST;

      //No query string
      webSocket->queryString[0] = '\0';
   }

   //The protocol version is following the Request-URI
   token = osStrtok_r(NULL, " \r\n", &p);

   //HTTP version 0.9?
   if(token == NULL)
   {
      //Save version number
      webSocket->handshakeContext.version = WS_HTTP_VERSION_0_9;
      //Persistent connections are not supported
      webSocket->handshakeContext.connectionClose = TRUE;
   }
   //HTTP version 1.0?
   else if(!osStrcasecmp(token, "HTTP/1.0"))
   {
      //Save version number
      webSocket->handshakeContext.version = WS_HTTP_VERSION_1_0;
      //By default connections are not persistent
      webSocket->handshakeContext.connectionClose = TRUE;
   }
   //HTTP version 1.1?
   else if(!osStrcasecmp(token, "HTTP/1.1"))
   {
      //Save version number
      webSocket->handshakeContext.version = WS_HTTP_VERSION_1_1;
      //HTTP 1.1 makes persistent connections the default
      webSocket->handshakeContext.connectionClose = FALSE;
   }
   //HTTP version not supported?
   else
   {
      //Report an error
      return ERROR_INVALID_REQUEST;
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse the Status-Line of the server's handshake
 * @param[in] webSocket Handle to a WebSocket
 * @param[in] line NULL-terminated string that contains the Status-Line
 * @return Error code
 **/

error_t webSocketParseStatusLine(WebSocket *webSocket, char_t *line)
{
   char_t *p;
   char_t *token;

   //Debug message
   TRACE_DEBUG("%s", line);

   //Retrieve the HTTP-Version field
   token = osStrtok_r(line, " ", &p);
   //Any parsing error?
   if(token == NULL)
      return ERROR_INVALID_SYNTAX;

   //Retrieve the Status-Code field
   token = osStrtok_r(NULL, " ", &p);
   //Any parsing error?
   if(token == NULL)
      return ERROR_INVALID_SYNTAX;

   //Convert the status code
   webSocket->handshakeContext.statusCode = osStrtoul(token, &p, 10);
   //Any parsing error?
   if(*p != '\0')
      return ERROR_INVALID_SYNTAX;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse a header field
 * @param[in] webSocket Handle to a WebSocket
 * @param[in] line NULL-terminated string that contains the header field
 * @return Error code
 **/

error_t webSocketParseHeaderField(WebSocket *webSocket, char_t *line)
{
   char_t *separator;
   char_t *name;
   char_t *value;
   WebSocketHandshakeContext *handshakeContext;

   //Point to the handshake context
   handshakeContext = &webSocket->handshakeContext;

   //Debug message
   TRACE_DEBUG("%s", line);

   //Check whether a separator is present
   separator = osStrchr(line, ':');

   //Separator found?
   if(separator != NULL)
   {
      //Split the line
      *separator = '\0';

      //Get field name and value
      name = strTrimWhitespace(line);
      value = strTrimWhitespace(separator + 1);

      //Upgrade header field found?
      if(!osStrcasecmp(name, "Upgrade"))
      {
         if(!osStrcasecmp(value, "websocket"))
            handshakeContext->upgradeWebSocket = TRUE;

      }
      //Connection header field found?
      else if(!osStrcasecmp(name, "Connection"))
      {
         //Parse Connection header field
         webSocketParseConnectionField(webSocket, value);
      }
      //Sec-WebSocket-Key header field found?
      else if(!osStrcasecmp(name, "Sec-WebSocket-Key"))
      {
         //Server operation?
         if(webSocket->endpoint == WS_ENDPOINT_SERVER)
         {
            //Save the contents of the Sec-WebSocket-Key header field
            strSafeCopy(handshakeContext->clientKey, value,
               WEB_SOCKET_CLIENT_KEY_SIZE + 1);
         }
      }
      //Sec-WebSocket-Accept header field found?
      else if(!osStrcasecmp(name, "Sec-WebSocket-Accept"))
      {
         //Client operation?
         if(webSocket->endpoint == WS_ENDPOINT_CLIENT)
         {
            //Save the contents of the Sec-WebSocket-Accept header field
            strSafeCopy(handshakeContext->serverKey, value,
               WEB_SOCKET_SERVER_KEY_SIZE + 1);
         }
      }
#if (WEB_SOCKET_BASIC_AUTH_SUPPORT == ENABLED || WEB_SOCKET_DIGEST_AUTH_SUPPORT == ENABLED)
      //WWW-Authenticate header field found?
      else if(!osStrcasecmp(name, "WWW-Authenticate"))
      {
         //Parse WWW-Authenticate header field
         webSocketParseAuthenticateField(webSocket, value);
      }
#endif
      //Content-Length header field found?
      else if(!osStrcasecmp(name, "Content-Length"))
      {
         handshakeContext->contentLength = osStrtoul(value, NULL, 10);
      }
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse Connection header field
 * @param[in] webSocket Handle to a WebSocket
 * @param[in] value NULL-terminated string that contains the value of header field
 **/

void webSocketParseConnectionField(WebSocket *webSocket, char_t *value)
{
   char_t *p;
   char_t *token;

   //Get the first value of the list
   token = osStrtok_r(value, ",", &p);

   //Parse the comma-separated list
   while(token != NULL)
   {
      //Trim whitespace characters
      value = strTrimWhitespace(token);

      //Check current value
      if(!osStrcasecmp(value, "keep-alive"))
      {
         //The connection is persistent
         webSocket->handshakeContext.connectionClose = FALSE;
      }
      else if(!osStrcasecmp(value, "close"))
      {
         //The connection will be closed after completion of the response
         webSocket->handshakeContext.connectionClose = TRUE;
      }
      else if(!osStrcasecmp(value, "upgrade"))
      {
         //Upgrade the connection
         webSocket->handshakeContext.connectionUpgrade = TRUE;
      }

      //Get next value
      token = osStrtok_r(NULL, ",", &p);
   }
}


/**
 * @brief Format client's handshake
 * @param[in] webSocket Handle to a WebSocket
 * @param[in] serverPort TCP port number used to establish the connection
 * @return Error code
 **/

error_t webSocketFormatClientHandshake(WebSocket *webSocket, uint16_t serverPort)
{
   char_t *p;
   WebSocketFrameContext *txContext;

   //Point to the TX context
   txContext = &webSocket->txContext;
   //Point to the buffer where to format the client's handshake
   p = (char_t *) txContext->buffer;

   //The Request-Line begins with a method token, followed by the
   //Request-URI and the protocol version, and ending with CRLF
   p += osSprintf(p, "GET %s HTTP/1.1\r\n", webSocket->uri);

   //Add Host header field
   if(webSocket->host[0] != '\0')
   {
      //The Host header field specifies the Internet host and port number of
      //the resource being requested
      p += osSprintf(p, "Host: %s:%d\r\n", webSocket->host, serverPort);
   }
   else
   {
      //If the requested URI does not include a host name for the service being
      //requested, then the Host header field must be given with an empty value
      p += osSprintf(p, "Host:\r\n");
   }

#if (WEB_SOCKET_BASIC_AUTH_SUPPORT == ENABLED || WEB_SOCKET_DIGEST_AUTH_SUPPORT == ENABLED)
   //Check whether authentication is required
   if(webSocket->authContext.selectedAuthMode != WS_AUTH_MODE_NONE)
   {
      //Add Authorization header field
      p += webSocketAddAuthorizationField(webSocket, p);
   }
#endif

   //Add Origin header field
   if(webSocket->origin[0] != '\0')
      p += osSprintf(p, "Origin: %s\r\n", webSocket->origin);
   else
      p += osSprintf(p, "Origin: null\r\n");

   //Add Upgrade header field
   p += osSprintf(p, "Upgrade: websocket\r\n");
   //Add Connection header field
   p += osSprintf(p, "Connection: Upgrade\r\n");

   //Add Sec-WebSocket-Protocol header field
   if(webSocket->subProtocol[0] != '\0')
      p += osSprintf(p, "Sec-WebSocket-Protocol: %s\r\n", webSocket->subProtocol);

   //Add Sec-WebSocket-Key header field
   p += osSprintf(p, "Sec-WebSocket-Key: %s\r\n",
      webSocket->handshakeContext.clientKey);

   //Add Sec-WebSocket-Version header field
   p += osSprintf(p, "Sec-WebSocket-Version: 13\r\n");
   //An empty line indicates the end of the header fields
   p += osSprintf(p, "\r\n");

   //Debug message
   TRACE_DEBUG("\r\n");
   TRACE_DEBUG("WebSocket: client handshake\r\n");
   TRACE_DEBUG("%s", txContext->buffer);

   //Rewind to the beginning of the buffer
   txContext->bufferPos = 0;
   //Update the number of data buffered but not yet sent
   txContext->bufferLen = osStrlen((char_t *) txContext->buffer);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Format server's handshake
 * @param[in] webSocket Handle to a WebSocket
 * @return Error code
 **/

error_t webSocketFormatServerHandshake(WebSocket *webSocket)
{
   char_t *p;
   WebSocketFrameContext *txContext;

   //Point to the TX context
   txContext = &webSocket->txContext;
   //Point to the buffer where to format the client's handshake
   p = (char_t *) txContext->buffer;

   //The first line is an HTTP Status-Line, with the status code 101
   p += osSprintf(p, "HTTP/1.1 101 Switching Protocols\r\n");

   //Add Upgrade header field
   p += osSprintf(p, "Upgrade: websocket\r\n");
   //Add Connection header field
   p += osSprintf(p, "Connection: Upgrade\r\n");

   //Add Sec-WebSocket-Protocol header field
   if(webSocket->subProtocol[0] != '\0')
      p += osSprintf(p, "Sec-WebSocket-Protocol: %s\r\n", webSocket->subProtocol);

   //Add Sec-WebSocket-Accept header field
   p += osSprintf(p, "Sec-WebSocket-Accept: %s\r\n",
      webSocket->handshakeContext.serverKey);

   //An empty line indicates the end of the header fields
   p += osSprintf(p, "\r\n");

   //Debug message
   TRACE_DEBUG("\r\n");
   TRACE_DEBUG("WebSocket: server handshake\r\n");
   TRACE_DEBUG("%s", txContext->buffer);

   //Rewind to the beginning of the buffer
   txContext->bufferPos = 0;
   //Update the number of data buffered but not yet sent
   txContext->bufferLen = osStrlen((char_t *) txContext->buffer);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Format HTTP error response
 * @param[in] webSocket Handle to a WebSocket
 * @param[in] statusCode HTTP status code
 * @param[in] message User message
 * @return Error code
 **/

error_t webSocketFormatErrorResponse(WebSocket *webSocket,
   uint_t statusCode, const char_t *message)
{
   uint_t i;
   size_t length;
   char_t *p;
   WebSocketFrameContext *txContext;

   //HTML response template
   static const char_t template[] =
      "<!doctype html>\r\n"
      "<html>\r\n"
      "<head><title>Error %03d</title></head>\r\n"
      "<body>\r\n"
      "<h2>Error %03d</h2>\r\n"
      "<p>%s</p>\r\n"
      "</body>\r\n"
      "</html>\r\n";

   //Point to the TX context
   txContext = &webSocket->txContext;
   //Point to the buffer where to format the client's handshake
   p = (char_t *) txContext->buffer;

   //The first line of a response message is the Status-Line, consisting
   //of the protocol version followed by a numeric status code and its
   //associated textual phrase
   p += osSprintf(p, "HTTP/%u.%u %u ", MSB(webSocket->handshakeContext.version),
      LSB(webSocket->handshakeContext.version), statusCode);

   //Retrieve the Reason-Phrase that corresponds to the Status-Code
   for(i = 0; i < arraysize(statusCodeList); i++)
   {
      //Check the status code
      if(statusCodeList[i].value == statusCode)
      {
         //Append the textual phrase to the Status-Line
         p += osSprintf(p, statusCodeList[i].message);
         //Break the loop and continue processing
         break;
      }
   }

   //Properly terminate the Status-Line
   p += osSprintf(p, "\r\n");

   //Content type
   p += osSprintf(p, "Content-Type: %s\r\n", "text/html");

   //Compute the length of the response
   length = osStrlen(template) + osStrlen(message) - 4;
   //Set Content-Length field
   p += osSprintf(p, "Content-Length: %" PRIuSIZE "\r\n", length);

   //Terminate the header with an empty line
   p += osSprintf(p, "\r\n");

   //Format HTML response
   p += osSprintf(p, template, statusCode, statusCode, message);

   //Rewind to the beginning of the buffer
   txContext->bufferPos = 0;
   //Update the number of data buffered but not yet sent
   txContext->bufferLen = osStrlen((char_t *) txContext->buffer);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Verify client's handshake
 * @param[in] webSocket Handle to a WebSocket
 * @return Error code
 **/

error_t webSocketVerifyClientHandshake(WebSocket *webSocket)
{
   error_t error;
   WebSocketHandshakeContext *handshakeContext;

   //Debug message
   TRACE_DEBUG("WebSocket: verifying client handshake\r\n");

   //Point to the handshake context
   handshakeContext = &webSocket->handshakeContext;

   //The HTTP version must be at least 1.1
   if(handshakeContext->version < WS_HTTP_VERSION_1_1)
      return ERROR_INVALID_REQUEST;

   //The request must contain an Upgrade header field whose value
   //must include the "websocket" keyword
   if(!handshakeContext->upgradeWebSocket)
      return ERROR_INVALID_REQUEST;

   //The request must contain a Connection header field whose value
   //must include the "Upgrade" token
   if(!handshakeContext->connectionUpgrade)
      return ERROR_INVALID_REQUEST;

   //The request must include a header field with the name Sec-WebSocket-Key
   if(handshakeContext->clientKey[0] == 0)
      return ERROR_INVALID_REQUEST;

   //Check the Sec-WebSocket-Key header field
   error = webSocketVerifyClientKey(webSocket);
   //Verification failed?
   if(error)
      return error;

   //Generate the server part of the handshake
   webSocketChangeState(webSocket, WS_STATE_SERVER_HANDSHAKE);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Verify server's handshake
 * @param[in] webSocket Handle to a WebSocket
 * @return Error code
 **/

error_t webSocketVerifyServerHandshake(WebSocket *webSocket)
{
   error_t error;
   WebSocketHandshakeContext *handshakeContext;

   //Debug message
   TRACE_DEBUG("WebSocket: verifying server handshake\r\n");

   //Point to the handshake context
   handshakeContext = &webSocket->handshakeContext;

   //If the status code received from the server is not 101, the client
   //handles the response per HTTP procedures
   if(handshakeContext->statusCode == 401)
   {
      //Authorization required
      return ERROR_AUTH_REQUIRED;
   }
   else if(handshakeContext->statusCode != 101)
   {
      //Unknown status code
      return ERROR_INVALID_STATUS;
   }

   //If the response lacks an Upgrade header field or the Upgrade header field
   //contains a value that is not an ASCII case-insensitive match for the
   //value "websocket", the client must fail the WebSocket connection
   if(!handshakeContext->upgradeWebSocket)
      return ERROR_INVALID_SYNTAX;

   //If the response lacks a Connection header field or the Connection header
   //field doesn't contain a token that is an ASCII case-insensitive match for
   //the value "Upgrade", the client must fail the WebSocket connection
   if(!handshakeContext->connectionUpgrade)
      return ERROR_INVALID_SYNTAX;

   //If the response lacks a Sec-WebSocket-Accept header field, the client
   //must fail the WebSocket connection
   if(osStrlen(handshakeContext->serverKey) == 0)
      return ERROR_INVALID_SYNTAX;

   //Check the Sec-WebSocket-Accept header field
   error = webSocketVerifyServerKey(webSocket);
   //Verification failed?
   if(error)
      return error;

   //If the server's response is validated as provided for above, it is
   //said that the WebSocket connection is established and that the
   //WebSocket connection is in the OPEN state
   webSocketChangeState(webSocket, WS_STATE_OPEN);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Generate client's key
 * @param[in] webSocket Handle to a WebSocket
 * @return Error code
 **/

error_t webSocketGenerateClientKey(WebSocket *webSocket)
{
   error_t error;
   size_t n;
   uint8_t nonce[16];
   WebSocketHandshakeContext *handshakeContext;

   //Debug message
   TRACE_DEBUG("WebSocket: Generating client's key...\r\n");

   //Point to the handshake context
   handshakeContext = &webSocket->handshakeContext;

   //Make sure that the RNG callback function has been registered
   if(webSockRandCallback == NULL)
   {
      //A cryptographically strong random number generator
      //must be used to generate the nonce
      return ERROR_PRNG_NOT_READY;
   }

   //A nonce must be selected randomly for each connection
   error = webSockRandCallback(nonce, sizeof(nonce));
   //Any error to report?
   if(error)
      return error;

   //Encode the client's key
   base64Encode(nonce, sizeof(nonce), handshakeContext->clientKey, &n);

   //Debug message
   TRACE_DEBUG("  Client key: %s\r\n", handshakeContext->clientKey);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Generate server's key
 * @param[in] webSocket Handle to a WebSocket
 * @return Error code
 **/

error_t webSocketGenerateServerKey(WebSocket *webSocket)
{
   size_t n;
   WebSocketHandshakeContext *handshakeContext;
   Sha1Context sha1Context;

   //Debug message
   TRACE_DEBUG("WebSocket: Generating server's key...\r\n");

   //Point to the handshake context
   handshakeContext = &webSocket->handshakeContext;

   //Retrieve the length of the client key
   n = osStrlen(handshakeContext->clientKey);

   //Concatenate the Sec-WebSocket-Key with the GUID string and digest
   //the resulting string using SHA-1
   sha1Init(&sha1Context);
   sha1Update(&sha1Context, handshakeContext->clientKey, n);
   sha1Update(&sha1Context, webSocketGuid, osStrlen(webSocketGuid));
   sha1Final(&sha1Context, NULL);

   //Encode the result using Base64
   base64Encode(sha1Context.digest, SHA1_DIGEST_SIZE,
      handshakeContext->serverKey, &n);

   //Debug message
   TRACE_DEBUG("  Server key: %s\r\n", handshakeContext->serverKey);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Verify client's key
 * @param[in] webSocket Handle to a WebSocket
 * @return Error code
 **/

error_t webSocketVerifyClientKey(WebSocket *webSocket)
{
   error_t error;
   size_t n;
   char_t *buffer;
   WebSocketHandshakeContext *handshakeContext;

   //Debug message
   TRACE_DEBUG("WebSocket: Verifying client's key...\r\n");

   //Temporary buffer
   buffer = (char_t *) webSocket->txContext.buffer;

   //Point to the handshake context
   handshakeContext = &webSocket->handshakeContext;

   //Retrieve the length of the client's key
   n = osStrlen(handshakeContext->clientKey);

   //The value of the Sec-WebSocket-Key header field must be a 16-byte
   //value that has been Base64-encoded
   error = base64Decode(handshakeContext->clientKey, n, buffer, &n);
   //Decoding failed?
   if(error)
      return ERROR_INVALID_KEY;

   //Check the length of the resulting value
   if(n != 16)
      return ERROR_INVALID_KEY;

   //Successful verification
   return NO_ERROR;
}


/**
 * @brief Verify server's key
 * @param[in] webSocket Handle to a WebSocket
 * @return Error code
 **/

error_t webSocketVerifyServerKey(WebSocket *webSocket)
{
   size_t n;
   char_t *buffer;
   WebSocketHandshakeContext *handshakeContext;
   Sha1Context sha1Context;

   //Debug message
   TRACE_DEBUG("WebSocket: Verifying server's key...\r\n");

   //Temporary buffer
   buffer = (char_t *) webSocket->txContext.buffer;

   //Point to the handshake context
   handshakeContext = &webSocket->handshakeContext;

   //Retrieve the length of the client's key
   n = osStrlen(handshakeContext->clientKey);

   //Concatenate the Sec-WebSocket-Key with the GUID string and digest
   //the resulting string using SHA-1
   sha1Init(&sha1Context);
   sha1Update(&sha1Context, handshakeContext->clientKey, n);
   sha1Update(&sha1Context, webSocketGuid, osStrlen(webSocketGuid));
   sha1Final(&sha1Context, NULL);

   //Encode the result using Base64
   base64Encode(sha1Context.digest, SHA1_DIGEST_SIZE, buffer, &n);

   //Debug message
   TRACE_DEBUG("  Client key: %s\r\n", handshakeContext->clientKey);
   TRACE_DEBUG("  Server key: %s\r\n", handshakeContext->serverKey);
   TRACE_DEBUG("  Calculated key: %s\r\n", webSocket->txContext.buffer);

   //Check whether the server's key is valid
   if(osStrcmp(handshakeContext->serverKey, buffer))
      return ERROR_INVALID_KEY;

   //Successful verification
   return NO_ERROR;
}


/**
 * @brief Check whether a status code is valid
 * @param[in] statusCode Status code
 * @return The function returns TRUE is the specified status code is
 *   valid. Otherwise, FALSE is returned
 **/

bool_t webSocketCheckStatusCode(uint16_t statusCode)
{
   bool_t valid;

   //Check status code
   if(statusCode == WS_STATUS_CODE_NORMAL_CLOSURE ||
      statusCode == WS_STATUS_CODE_GOING_AWAY ||
      statusCode == WS_STATUS_CODE_PROTOCOL_ERROR ||
      statusCode == WS_STATUS_CODE_UNSUPPORTED_DATA ||
      statusCode == WS_STATUS_CODE_INVALID_PAYLOAD_DATA ||
      statusCode == WS_STATUS_CODE_POLICY_VIOLATION ||
      statusCode == WS_STATUS_CODE_MESSAGE_TOO_BIG ||
      statusCode == WS_STATUS_CODE_MANDATORY_EXT ||
      statusCode == WS_STATUS_CODE_INTERNAL_ERROR)
   {
      valid = TRUE;
   }
   else if(statusCode >= 3000)
   {
      valid = TRUE;
   }
   else
   {
      valid = FALSE;
   }

   //The function returns TRUE is the specified status code is valid
   return valid;
}


/**
 * @brief Decode a percent-encoded string
 * @param[in] input NULL-terminated string to be decoded
 * @param[out] output NULL-terminated string resulting from the decoding process
 * @param[in] outputSize Size of the output buffer in bytes
 * @return Error code
 **/

error_t webSocketDecodePercentEncodedString(const char_t *input,
   char_t *output, size_t outputSize)
{
   size_t i;
   char_t buffer[3];

   //Check parameters
   if(input == NULL || output == NULL)
      return ERROR_INVALID_PARAMETER;

   //Decode the percent-encoded string
   for(i = 0; *input != '\0' && i < outputSize; i++)
   {
      //Check current character
      if(*input == '+')
      {
         //Replace '+' characters with spaces
         output[i] = ' ';
         //Advance data pointer
         input++;
      }
      else if(input[0] == '%' && input[1] != '\0' && input[2] != '\0')
      {
         //Process percent-encoded characters
         buffer[0] = input[1];
         buffer[1] = input[2];
         buffer[2] = '\0';
         //String to integer conversion
         output[i] = (uint8_t) osStrtoul(buffer, NULL, 16);
         //Advance data pointer
         input += 3;
      }
      else
      {
         //Copy any other characters
         output[i] = *input;
         //Advance data pointer
         input++;
      }
   }

   //Check whether the output buffer runs out of space
   if(i >= outputSize)
      return ERROR_FAILURE;

   //Properly terminate the resulting string
   output[i] = '\0';
   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Check whether a an UTF-8 stream is valid
 * @param[in] context UTF-8 decoding context
 * @param[in] data Pointer to the chunk of data to be processed
 * @param[in] length Data chunk length
 * @param[in] remaining number of remaining bytes in the UTF-8 stream
 * @return The function returns TRUE is the specified UTF-8 stream is
 *   valid. Otherwise, FALSE is returned
 **/

bool_t webSocketCheckUtf8Stream(WebSocketUtf8Context *context,
   const uint8_t *data, size_t length, size_t remaining)
{
   size_t i;
   bool_t valid;

   //Initialize flag
   valid = TRUE;

   //Interpret the byte stream as UTF-8
   for(i = 0; i < length && valid; i++)
   {
      //Leading or continuation byte?
      if(context->utf8CharIndex == 0)
      {
         //7-bit code point?
         if((data[i] & 0x80) == 0x00)
         {
            //The code point consist of a single byte
            context->utf8CharSize = 1;
            //Decode the first byte of the sequence
            context->utf8CodePoint = data[i] & 0x7F;
         }
         //11-bit code point?
         else if((data[i] & 0xE0) == 0xC0)
         {
            //The code point consist of a 2 bytes
            context->utf8CharSize = 2;
            //Decode the first byte of the sequence
            context->utf8CodePoint = (data[i] & 0x1F) << 6;
         }
         //16-bit code point?
         else if((data[i] & 0xF0) == 0xE0)
         {
            //The code point consist of a 3 bytes
            context->utf8CharSize = 3;
            //Decode the first byte of the sequence
            context->utf8CodePoint = (data[i] & 0x0F) << 12;
         }
         //21-bit code point?
         else if((data[i] & 0xF8) == 0xF0)
         {
            //The code point consist of a 3 bytes
            context->utf8CharSize = 4;
            //Decode the first byte of the sequence
            context->utf8CodePoint = (data[i] & 0x07) << 18;
         }
         else
         {
            //The UTF-8 stream is not valid
            valid = FALSE;
         }

         //This test only applies to frames that are not fragmented
         if(length <= remaining)
         {
            //Make sure the UTF-8 stream is properly terminated
            if((i + context->utf8CharSize) > remaining)
            {
               //The UTF-8 stream is not valid
               valid = FALSE;
            }
         }

         //Decode the next byte of the sequence
         context->utf8CharIndex = context->utf8CharSize - 1;
      }
      else
      {
         //Continuation bytes all have 10 in the high-order position
         if((data[i] & 0xC0) == 0x80)
         {
            //Decode the multi-byte sequence
            context->utf8CharIndex--;
            //All continuation bytes contain exactly 6 bits from the code point
            context->utf8CodePoint |= (data[i] & 0x3F) << (context->utf8CharIndex * 6);

            //The correct encoding of a code point use only the minimum number
            //of bytes required to hold the significant bits of the code point
            if(context->utf8CharSize == 2)
            {
               //Overlong encoding is not supported
               if((context->utf8CodePoint & ~0x7F) == 0)
                  valid = FALSE;
            }
            if(context->utf8CharSize == 3 && context->utf8CharIndex < 2)
            {
               //Overlong encoding is not supported
               if((context->utf8CodePoint & ~0x7FF) == 0)
                  valid = FALSE;
            }
            if(context->utf8CharSize == 4 && context->utf8CharIndex < 3)
            {
               //Overlong encoding is not supported
               if((context->utf8CodePoint & ~0xFFFF) == 0)
                  valid = FALSE;
            }

            //According to the UTF-8 definition (RFC 3629) the high and low
            //surrogate halves used by UTF-16 (U+D800 through U+DFFF) are not
            //legal Unicode values, and their UTF-8 encoding should be treated
            //as an invalid byte sequence
            if(context->utf8CodePoint >= 0xD800 && context->utf8CodePoint < 0xE000)
               valid = FALSE;

            //Code points greater than U+10FFFF are not valid
            if(context->utf8CodePoint >= 0x110000)
               valid = FALSE;
         }
         else
         {
            //The start byte is not followed by enough continuation bytes
            valid = FALSE;
         }
      }
   }

   //The function returns TRUE is the specified UTF-8 stream is valid
   return valid;
}

#endif
