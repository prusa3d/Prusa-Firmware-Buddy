/**
 * @file http_server_misc.c
 * @brief HTTP server (miscellaneous functions)
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
#define TRACE_LEVEL HTTP_TRACE_LEVEL

//Dependencies
#include <stdlib.h>
#include <limits.h>
#include "net.h"
#include "http/http_server.h"
#include "http/http_server_auth.h"
#include "http/http_server_misc.h"
#include "http/mime.h"
#include "str.h"
#include "path.h"
#include "cyclone_debug.h"

//Check TCP/IP stack configuration
#if (HTTP_SERVER_SUPPORT == ENABLED)


/**
 * @brief HTTP status codes
 **/

static const HttpStatusCodeDesc statusCodeList[] =
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
 * @brief Read HTTP request header and parse its contents
 * @param[in] connection Structure representing an HTTP connection
 * @return Error code
 **/

error_t httpReadRequestHeader(HttpConnection *connection)
{
   error_t error;
   size_t length;

   //Set the maximum time the server will wait for an HTTP
   //request before closing the connection
   error = socketSetTimeout(connection->socket, HTTP_SERVER_IDLE_TIMEOUT);
   //Any error to report?
   if(error)
      return error;
   // reset the buffers, counters
   osMemset(connection->bigBuffer,  0, HTTP_SERVER_BIGBUFFER_SIZE);
   connection->totalRead = 0;
   connection->totalReceived = 0;
   //Read the first line of the request
   error = httpReceive(connection, connection->buffer,
      HTTP_SERVER_BUFFER_SIZE - 1, &length, SOCKET_FLAG_BREAK_CRLF);
   //Unable to read any data?
   if(error)
      return error;

   //Revert to default timeout
   error = socketSetTimeout(connection->socket, HTTP_SERVER_TIMEOUT);
   //Any error to report?
   if(error)
      return error;

   //Properly terminate the string with a NULL character
   connection->buffer[length] = '\0';
   //Debug message
   TRACE_INFO("%s", connection->buffer);

   //Parse the Request-Line
   error = httpParseRequestLine(connection, connection->buffer);
   //Any error to report?
   if(error)
      return error;

   //Default value for properties
   connection->request.chunkedEncoding = FALSE;
   connection->request.contentLength = 0;
#if (HTTP_SERVER_WEB_SOCKET_SUPPORT == ENABLED)
   connection->request.upgradeWebSocket = FALSE;
   connection->request.connectionUpgrade = FALSE;
   osStrcpy(connection->request.clientKey, "");
#endif

//   //HTTP 0.9 does not support Full-Request
//   if(connection->request.version >= HTTP_VERSION_1_0)
//   {
//      //Local variables
//      char_t firstChar;
//      char_t *separator;
//      char_t *name;
//      char_t *value;
//
//      //This variable is used to decode header fields that span multiple lines
//      firstChar = '\0';
//
//      //Parse the header fields of the HTTP request
//      while(1)
//      {
//         //Decode multiple-line header field
//         error = httpReadHeaderField(connection, connection->buffer,
//            HTTP_SERVER_BUFFER_SIZE, &firstChar);
//         //Any error to report?
//         if(error)
//            return error;
//
//         //Debug message
//         TRACE_DEBUG("%s", connection->buffer);
//
//         //An empty line indicates the end of the header fields
//         if(!osStrcmp(connection->buffer, "\r\n"))
//            break;
//
//         //Check whether a separator is present
//         separator = osStrchr(connection->buffer, ':');
//
//         //Separator found?
//         if(separator != NULL)
//         {
//            //Split the line
//            *separator = '\0';
//
//            //Trim whitespace characters
//            name = strTrimWhitespace(connection->buffer);
//            value = strTrimWhitespace(separator + 1);
//
//            //Parse HTTP header field
//            httpParseHeaderField(connection, name, value);
//         }
//      }
//   }

   //Prepare to read the HTTP request body
   if(connection->request.chunkedEncoding)
   {
      connection->request.byteCount = 0;
      connection->request.firstChunk = TRUE;
      connection->request.lastChunk = FALSE;
   }
   else
   {
      connection->request.byteCount = connection->request.contentLength;
   }

   //The request header has been successfully parsed
   return NO_ERROR;
}


/**
 * @brief Parse Request-Line
 * @param[in] connection Structure representing an HTTP connection
 * @param[in] requestLine Pointer to the string that holds the Request-Line
 * @return Error code
 **/

error_t httpParseRequestLine(HttpConnection *connection, char_t *requestLine)
{
   error_t error;
   char_t *token;
   char_t *p;
   char_t *s;

   //The Request-Line begins with a method token
   token = osStrtok_r(requestLine, " \r\n", &p);
   //Unable to retrieve the method?
   if(token == NULL)
      return ERROR_INVALID_REQUEST;

   //The Method token indicates the method to be performed on the
   //resource identified by the Request-URI
   error = strSafeCopy(connection->request.method, token, HTTP_SERVER_METHOD_MAX_LEN);
   //Any error to report?
   if(error)
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
      error = httpDecodePercentEncodedString(token,
         connection->request.uri, HTTP_SERVER_URI_MAX_LEN);
      //Any error to report?
      if(error)
         return ERROR_INVALID_REQUEST;

      //Check the length of the query string
      if(osStrlen(s + 1) > HTTP_SERVER_QUERY_STRING_MAX_LEN)
         return ERROR_INVALID_REQUEST;

      //Save the query string
      osStrcpy(connection->request.queryString, s + 1);
   }
   else
   {
      //Save the Request-URI
      error = httpDecodePercentEncodedString(token,
         connection->request.uri, HTTP_SERVER_URI_MAX_LEN);
      //Any error to report?
      if(error)
         return ERROR_INVALID_REQUEST;

      //No query string
      connection->request.queryString[0] = '\0';
   }

   //Redirect to the default home page if necessary
   if(!osStrcasecmp(connection->request.uri, "/"))
      osStrcpy(connection->request.uri, connection->settings->defaultDocument);

   //Clean the resulting path
   pathCanonicalize(connection->request.uri);

   //The protocol version is following the Request-URI
   token = osStrtok_r(NULL, " \r\n", &p);

   //HTTP version 0.9?
   if(token == NULL)
   {
      //Save version number
      connection->request.version = HTTP_VERSION_0_9;
      //Persistent connections are not supported
      connection->request.keepAlive = FALSE;
   }
   //HTTP version 1.0?
   else if(!osStrcasecmp(token, "HTTP/1.0"))
   {
      //Save version number
      connection->request.version = HTTP_VERSION_1_0;
      //By default connections are not persistent
      connection->request.keepAlive = FALSE;
   }
   //HTTP version 1.1?
   else if(!osStrcasecmp(token, "HTTP/1.1"))
   {
      //Save version number
      connection->request.version = HTTP_VERSION_1_1;
      //HTTP 1.1 makes persistent connections the default
      connection->request.keepAlive = TRUE;
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
 * @brief Read multiple-line header field
 * @param[in] connection Structure representing an HTTP connection
 * @param[out] buffer Buffer where to store the header field
 * @param[in] size Size of the buffer, in bytes
 * @param[in,out] firstChar Leading character of the header line
 * @return Error code
 **/

error_t httpReadHeaderField(HttpConnection *connection,
   char_t *buffer, size_t size, char_t *firstChar)
{
   error_t error;
   size_t n;
   size_t length;

   //This is the actual length of the header field
   length = 0;

   //The process of moving from a multiple-line representation of a header
   //field to its single line representation is called unfolding
   do
   {
      //Check the length of the header field
      if((length + 1) >= size)
      {
         //Report an error
         error = ERROR_INVALID_REQUEST;
         //Exit immediately
         break;
      }

      //NULL character found?
      if(*firstChar == '\0')
      {
         //Prepare to decode the first header field
         length = 0;
      }
      //LWSP character found?
      else if(*firstChar == ' ' || *firstChar == '\t')
      {
         //Unfolding is accomplished by regarding CRLF immediately
         //followed by a LWSP as equivalent to the LWSP character
         buffer[length] = *firstChar;
         //The current header field spans multiple lines
         length++;
      }
      //Any other character?
      else
      {
         //Restore the very first character of the header field
         buffer[0] = *firstChar;
         //Prepare to decode a new header field
         length = 1;
      }

      //Read data until a CLRF character is encountered
      error = httpReceive(connection, buffer + length,
         size - 1 - length, &n, SOCKET_FLAG_BREAK_CRLF);
      //Any error to report?
      if(error)
         break;

      //Update the length of the header field
      length += n;
      //Properly terminate the string with a NULL character
      buffer[length] = '\0';

      //An empty line indicates the end of the header fields
      if(!osStrcmp(buffer, "\r\n"))
         break;

      //Read the next character to detect if the CRLF is immediately
      //followed by a LWSP character
      error = httpReceive(connection, firstChar,
         sizeof(char_t), &n, SOCKET_FLAG_WAIT_ALL);
      //Any error to report?
      if(error)
         break;

      //LWSP character found?
      if(*firstChar == ' ' || *firstChar == '\t')
      {
         //CRLF immediately followed by LWSP as equivalent to the LWSP character
         if(length >= 2)
         {
            if(buffer[length - 2] == '\r' || buffer[length - 1] == '\n')
            {
               //Remove trailing CRLF sequence
               length -= 2;
               //Properly terminate the string with a NULL character
               buffer[length] = '\0';
            }
         }
      }

      //A header field may span multiple lines...
   } while(*firstChar == ' ' || *firstChar == '\t');

   //Return status code
   return error;
}


/**
 * @brief Parse HTTP header field
 * @param[in] connection Structure representing an HTTP connection
 * @param[in] name Name of the header field
 * @param[in] value Value of the header field
 **/

void httpParseHeaderField(HttpConnection *connection,
   const char_t *name, char_t *value)
{
   //Host header field?
   if(!osStrcasecmp(name, "Host"))
   {
      //Save host name
      strSafeCopy(connection->request.host, value,
         HTTP_SERVER_HOST_MAX_LEN);
   }
   //Connection header field?
   else if(!osStrcasecmp(name, "Connection"))
   {
      //Parse Connection header field
      httpParseConnectionField(connection, value);
   }
   //Transfer-Encoding header field?
   else if(!osStrcasecmp(name, "Transfer-Encoding"))
   {
      //Check whether chunked encoding is used
      if(!osStrcasecmp(value, "chunked"))
         connection->request.chunkedEncoding = TRUE;
   }
   //Content-Type field header?
   else if(!osStrcasecmp(name, "Content-Type"))
   {
      //Parse Content-Type header field
      httpParseContentTypeField(connection, value);
   }
   //Content-Length header field?
   else if(!osStrcasecmp(name, "Content-Length"))
   {
      //Get the length of the body data
      connection->request.contentLength = atoi(value);
   }
   //Accept-Encoding field header?
   else if(!osStrcasecmp(name, "Accept-Encoding"))
   {
      //Parse Content-Type header field
      httpParseAcceptEncodingField(connection, value);
   }
   //Authorization header field?
   else if(!osStrcasecmp(name, "Authorization"))
   {
      //Parse Authorization header field
      httpParseAuthorizationField(connection, value);
   }
#if (HTTP_SERVER_WEB_SOCKET_SUPPORT == ENABLED)
   //Upgrade header field?
   else if(!osStrcasecmp(name, "Upgrade"))
   {
      //WebSocket support?
      if(!osStrcasecmp(value, "websocket"))
         connection->request.upgradeWebSocket = TRUE;
   }
   //Sec-WebSocket-Key header field?
   else if(!osStrcasecmp(name, "Sec-WebSocket-Key"))
   {
      //Save the contents of the Sec-WebSocket-Key header field
      strSafeCopy(connection->request.clientKey, value,
         WEB_SOCKET_CLIENT_KEY_SIZE + 1);
   }
#endif
#if (HTTP_SERVER_COOKIE_SUPPORT == ENABLED)
   //Cookie header field?
   else if(!osStrcasecmp(name, "Cookie"))
   {
      //Parse Cookie header field
      httpParseCookieField(connection, value);
   }
#endif
}


/**
 * @brief Parse Connection header field
 * @param[in] connection Structure representing an HTTP connection
 * @param[in] value Connection field value
 **/

void httpParseConnectionField(HttpConnection *connection,
   char_t *value)
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
         connection->request.keepAlive = TRUE;
      }
      else if(!osStrcasecmp(value, "close"))
      {
         //The connection will be closed after completion of the response
         connection->request.keepAlive = FALSE;
      }
#if (HTTP_SERVER_WEB_SOCKET_SUPPORT == ENABLED)
      else if(!osStrcasecmp(value, "upgrade"))
      {
         //Upgrade the connection
         connection->request.connectionUpgrade = TRUE;
      }
#endif

      //Get next value
      token = osStrtok_r(NULL, ",", &p);
   }
}


/**
 * @brief Parse Content-Type header field
 * @param[in] connection Structure representing an HTTP connection
 * @param[in] value Content-Type field value
 **/

void httpParseContentTypeField(HttpConnection *connection,
   char_t *value)
{
#if (HTTP_SERVER_MULTIPART_TYPE_SUPPORT == ENABLED)
   size_t n;
   char_t *p;
   char_t *token;

   //Retrieve type
   token = osStrtok_r(value, "/", &p);
   //Any parsing error?
   if(token == NULL)
      return;

   //The boundary parameter makes sense only for the multipart content-type
   if(!osStrcasecmp(token, "multipart"))
   {
      //Skip subtype
      token = osStrtok_r(NULL, ";", &p);
      //Any parsing error?
      if(token == NULL)
         return;

      //Retrieve parameter name
      token = osStrtok_r(NULL, "=", &p);
      //Any parsing error?
      if(token == NULL)
         return;

      //Trim whitespace characters
      token = strTrimWhitespace(token);

      //Check parameter name
      if(!osStrcasecmp(token, "boundary"))
      {
         //Retrieve parameter value
         token = osStrtok_r(NULL, ";", &p);
         //Any parsing error?
         if(token == NULL)
            return;

         //Trim whitespace characters
         token = strTrimWhitespace(token);
         //Get the length of the boundary string
         n = osStrlen(token);

         //Check the length of the boundary string
         if(n < HTTP_SERVER_BOUNDARY_MAX_LEN)
         {
            //Copy the boundary string
            osStrncpy(connection->request.boundary, token, n);
            //Properly terminate the string
            connection->request.boundary[n] = '\0';

            //Save the length of the boundary string
            connection->request.boundaryLength = n;
         }
      }
   }
#endif
}


/**
 * @brief Parse Accept-Encoding header field
 * @param[in] connection Structure representing an HTTP connection
 * @param[in] value Accept-Encoding field value
 **/

void httpParseAcceptEncodingField(HttpConnection *connection,
   char_t *value)
{
#if (HTTP_SERVER_GZIP_TYPE_SUPPORT == ENABLED)
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
      if(!osStrcasecmp(value, "gzip"))
      {
         //gzip compression is supported
         connection->request.acceptGzipEncoding = TRUE;
      }

      //Get next value
      token = osStrtok_r(NULL, ",", &p);
   }
#endif
}


/**
 * @brief Parse Cookie header field
 * @param[in] connection Structure representing an HTTP connection
 * @param[in] value Accept-Encoding field value
 **/

void httpParseCookieField(HttpConnection *connection, char_t *value)
{
#if (HTTP_SERVER_COOKIE_SUPPORT == ENABLED)
   //Save the value of the header field
   strSafeCopy(connection->request.cookie, value, HTTP_SERVER_COOKIE_MAX_LEN);
#endif
}


/**
 * @brief Read chunk-size field from the input stream
 * @param[in] connection Structure representing an HTTP connection
 **/

error_t httpReadChunkSize(HttpConnection *connection)
{
   error_t error;
   size_t n;
   char_t *end;
   char_t s[8];

   //First chunk to be received?
   if(connection->request.firstChunk)
   {
      //Clear the flag
      connection->request.firstChunk = FALSE;
   }
   else
   {
      //Read the CRLF that follows the previous chunk-data field
      error = httpReceive(connection, s, sizeof(s) - 1, &n, SOCKET_FLAG_BREAK_CRLF);
      //Any error to report?
      if(error)
         return error;

      //Properly terminate the string with a NULL character
      s[n] = '\0';

      //The chunk data must be terminated by CRLF
      if(osStrcmp(s, "\r\n"))
         return ERROR_WRONG_ENCODING;
   }

   //Read the chunk-size field
   error = httpReceive(connection, s, sizeof(s) - 1, &n, SOCKET_FLAG_BREAK_CRLF);
   //Any error to report?
   if(error)
      return error;

   //Properly terminate the string with a NULL character
   s[n] = '\0';
   //Remove extra whitespaces
   strRemoveTrailingSpace(s);

   //Retrieve the size of the chunk
   connection->request.byteCount = osStrtoul(s, &end, 16);

   //No valid conversion could be performed?
   if(end == s || *end != '\0')
      return ERROR_WRONG_ENCODING;

   //Any chunk whose size is zero terminates the data transfer
   if(!connection->request.byteCount)
   {
      //The end of the HTTP request body has been reached
      connection->request.lastChunk = TRUE;

      //Skip the trailer
      while(1)
      {
         //Read a complete line
         error = httpReceive(connection, s, sizeof(s) - 1, &n, SOCKET_FLAG_BREAK_CRLF);
         //Unable to read any data?
         if(error)
            return error;

         //Properly terminate the string with a NULL character
         s[n] = '\0';

         //The trailer is terminated by an empty line
         if(!osStrcmp(s, "\r\n"))
            break;
      }
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Initialize response header
 * @param[in] connection Structure representing an HTTP connection
 **/

void httpInitResponseHeader(HttpConnection *connection)
{
   //Default HTTP header fields
   connection->response.version = connection->request.version;
   connection->response.statusCode = 200;
   connection->response.noCache = FALSE;
   connection->response.maxAge = 0;
   connection->response.location = NULL;
   connection->response.contentType = mimeGetType(connection->request.uri);
   connection->response.chunkedEncoding = TRUE;

#if (HTTP_SERVER_GZIP_TYPE_SUPPORT == ENABLED)
   //Do not use gzip encoding
   connection->response.gzipEncoding = FALSE;
#endif

#if (HTTP_SERVER_PERSISTENT_CONN_SUPPORT == ENABLED)
   //Persistent connections are accepted
   connection->response.keepAlive = connection->request.keepAlive;
#else
   //Connections are not persistent by default
   connection->response.keepAlive = FALSE;
#endif
}


/**
 * @brief Format HTTP response header
 * @param[in] connection Structure representing an HTTP connection
 * @param[out] buffer Pointer to the buffer where to format the HTTP header
 * @return Error code
 **/

error_t httpFormatResponseHeader(HttpConnection *connection, char_t *buffer)
{
   uint_t i;
   char_t *p;

   //HTTP version 0.9?
   if(connection->response.version == HTTP_VERSION_0_9)
   {
      //Enforce default parameters
      connection->response.keepAlive = FALSE;
      connection->response.chunkedEncoding = FALSE;
      //The size of the response body is not limited
      connection->response.byteCount = UINT_MAX;
      //We are done since HTTP 0.9 does not support Full-Response format
      return NO_ERROR;
   }

   //When generating dynamic web pages with HTTP 1.0, the only way to
   //signal the end of the body is to close the connection
   if(connection->response.version == HTTP_VERSION_1_0 &&
      connection->response.chunkedEncoding)
   {
      //Make the connection non persistent
      connection->response.keepAlive = FALSE;
      connection->response.chunkedEncoding = FALSE;
      //The size of the response body is not limited
      connection->response.byteCount = UINT_MAX;
   }
   else
   {
      //Limit the size of the response body
      connection->response.byteCount = connection->response.contentLength;
   }

   //Point to the beginning of the buffer
   p = buffer;

   //The first line of a response message is the Status-Line, consisting
   //of the protocol version followed by a numeric status code and its
   //associated textual phrase
   p += osSprintf(p, "HTTP/%u.%u %u ", MSB(connection->response.version),
      LSB(connection->response.version), connection->response.statusCode);

   //Retrieve the Reason-Phrase that corresponds to the Status-Code
   for(i = 0; i < arraysize(statusCodeList); i++)
   {
      //Check the status code
      if(statusCodeList[i].value == connection->response.statusCode)
      {
         //Append the textual phrase to the Status-Line
         p += osSprintf(p, "%s", statusCodeList[i].message);
         //Break the loop and continue processing
         break;
      }
   }

   //Properly terminate the Status-Line
   p += osSprintf(p, "\r\n");

   //Valid location?
   if(connection->response.location != NULL)
   {
      //Set Location field
      p += osSprintf(p, "Location: %s\r\n", connection->response.location);
   }

   //Persistent connection?
   if(connection->response.keepAlive)
   {
      //Set Connection field
      p += osSprintf(p, "Connection: keep-alive\r\n");

      //Set Keep-Alive field
      p += osSprintf(p, "Keep-Alive: timeout=%u, max=%u\r\n",
         HTTP_SERVER_IDLE_TIMEOUT / 1000, HTTP_SERVER_MAX_REQUESTS);
   }
   else
   {
      //Set Connection field
      p += osSprintf(p, "Connection: close\r\n");
   }

   //Specify the caching policy
   if(connection->response.noCache)
   {
      //Set Pragma field
      p += osSprintf(p, "Pragma: no-cache\r\n");
      //Set Cache-Control field
      p += osSprintf(p, "Cache-Control: no-store, no-cache, must-revalidate\r\n");
      p += osSprintf(p, "Cache-Control: max-age=0, post-check=0, pre-check=0\r\n");
   }
   else if(connection->response.maxAge != 0)
   {
      //Set Cache-Control field
      p += osSprintf(p, "Cache-Control: max-age=%u\r\n", connection->response.maxAge);
   }

#if (HTTP_SERVER_TLS_SUPPORT == ENABLED && HTTP_SERVER_HSTS_SUPPORT == ENABLED)
   //TLS-secured connection?
   if(connection->serverContext->settings.tlsInitCallback != NULL)
   {
      //Set Strict-Transport-Security field
      p += osSprintf(p, "Strict-Transport-Security: max-age=31536000\r\n");
   }
#endif

#if (HTTP_SERVER_BASIC_AUTH_SUPPORT == ENABLED || HTTP_SERVER_DIGEST_AUTH_SUPPORT == ENABLED)
   //Check whether authentication is required
   if(connection->response.auth.mode != HTTP_AUTH_MODE_NONE)
   {
      //Add WWW-Authenticate header field
      p += httpAddAuthenticateField(connection, p);
   }
#endif

#if (HTTP_SERVER_COOKIE_SUPPORT == ENABLED)
   //Valid cookie
   if(connection->response.setCookie[0] != '\0')
   {
      //Add Set-Cookie header field
      p += osSprintf(p, "Set-Cookie: %s\r\n", connection->response.setCookie);
   }
#endif

   //Valid content type?
   if(connection->response.contentType != NULL)
   {
      //Content type
      p += osSprintf(p, "Content-Type: %s\r\n", connection->response.contentType);
   }

#if (HTTP_SERVER_GZIP_TYPE_SUPPORT == ENABLED)
   //Use gzip encoding?
   if(connection->response.gzipEncoding)
   {
      //Set Transfer-Encoding field
      p += osSprintf(p, "Content-Encoding: gzip\r\n");
   }
#endif

   //Use chunked encoding transfer?
   if(connection->response.chunkedEncoding)
   {
      //Set Transfer-Encoding field
      p += osSprintf(p, "Transfer-Encoding: chunked\r\n");
   }
   //Persistent connection?
   else if(connection->response.keepAlive)
   {
      //Set Content-Length field
      p += osSprintf(p, "Content-Length: %" PRIuSIZE "\r\n", connection->response.contentLength);
   }

   //Terminate the header with an empty line
   p += osSprintf(p, "\r\n");

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Send data to the client
 * @param[in] connection Structure representing an HTTP connection
 * @param[in] data Pointer to a buffer containing the data to be transmitted
 * @param[in] length Number of bytes to be transmitted
 * @param[in] flags Set of flags that influences the behavior of this function
 **/

error_t httpSend(HttpConnection *connection,
   const void *data, size_t length, uint_t flags)
{
#if (NET_RTOS_SUPPORT == ENABLED)
   error_t error;

#if (HTTP_SERVER_TLS_SUPPORT == ENABLED)
   //Check whether a secure connection is being used
   if(connection->tlsContext != NULL)
   {
      //Use TLS to transmit data to the client
      error = tlsWrite(connection->tlsContext, data, length, NULL, flags);
   }
   else
#endif
   {
      //Transmit data to the client
      error = send(connection->socket->buddy_f_d, data, length, flags);
   }

   //Return status code
   return error;
#else
   //Prevent buffer overflow
   if((connection->bufferLen + length) > HTTP_SERVER_BUFFER_SIZE)
      return ERROR_BUFFER_OVERFLOW;

   //Copy user data
   osMemcpy(connection->buffer + connection->bufferLen, data, length);
   //Adjust the length of the buffer
   connection->bufferLen += length;

   //Successful processing
   return NO_ERROR;
#endif
}


/**
 * @brief Receive data from the client
 * @param[in] connection Structure representing an HTTP connection
 * @param[out] data Buffer into which received data will be placed
 * @param[in] size Maximum number of bytes that can be received
 * @param[out] received Actual number of bytes that have been received
 * @param[in] flags Set of flags that influences the behavior of this function
 * @return Error code
 **/

error_t httpReceive(HttpConnection *connection,
   void *data, size_t size, size_t *received, uint_t flags)
{
#if (NET_RTOS_SUPPORT == ENABLED)
   error_t error = NO_ERROR;

#if (HTTP_SERVER_TLS_SUPPORT == ENABLED)
   //Check whether a secure connection is being used
   if(connection->tlsContext != NULL)
   {
      //Use TLS to receive data from the client
      error = tlsRead(connection->tlsContext, data, size, received, flags);
   }
   else
#endif
   {
      //Receive data from the client
       ssize_t rec = recv(connection->socket->buddy_f_d, (connection->bigBuffer + connection->totalReceived),
                                   HTTP_SERVER_BIGBUFFER_SIZE - connection->totalReceived, flags);
      if(-1 == rec) {
          error = ERROR_FAILURE;
          *received = 0;
      } else {
          *received = rec;
          error = strSafeCopy(data, connection->bigBuffer, *received);
      }
   }

   //Return status code
   return error;
#else
   error_t error;
   char_t c;
   size_t i;
   size_t n;

   //Number of data bytes that are pending in the receive buffer
   n = connection->bufferLen - connection->bufferPos;

   //Any data to be copied?
   if(n > 0)
   {
      //Limit the number of bytes to read at a time
      n = MIN(n, size);

      //The HTTP_FLAG_BREAK_CHAR flag causes the function to stop reading
      //data as soon as the specified break character is encountered
      if((flags & HTTP_FLAG_BREAK_CHAR) != 0)
      {
         //Retrieve the break character code
         c = LSB(flags);

         //Search for the specified break character
         for(i = 0; i < n && connection->buffer[connection->bufferPos + i] != c; i++);

         //Adjust the number of data to read
         n = MIN(n, i + 1);
      }

      //Copy data to user buffer
      osMemcpy(data, connection->buffer + connection->bufferPos, n);

      //Advance current position
      connection->bufferPos += n;
      //Total number of data that have been read
      *received = n;

      //Successful processing
      error = NO_ERROR;
   }
   else
   {
      //No more data available...
      error = ERROR_END_OF_STREAM;
   }

   //Return status code
   return error;
#endif
}


/**
 * @brief Retrieve the full pathname to the specified resource
 * @param[in] connection Structure representing an HTTP connection
 * @param[in] relative String containing the relative path to the resource
 * @param[out] absolute Resulting string containing the absolute path
 * @param[in] maxLen Maximum acceptable path length
 **/

void httpGetAbsolutePath(HttpConnection *connection,
   const char_t *relative, char_t *absolute, size_t maxLen)
{
   //Copy the root directory
   osStrcpy(absolute, connection->settings->rootDirectory);

   //Append the specified path
   pathCombine(absolute, relative, maxLen);

   //Clean the resulting path
   pathCanonicalize(absolute);
}


/**
 * @brief Compare filename extension
 * @param[in] filename Filename whose extension is to be checked
 * @param[in] extension String defining the extension to be checked
 * @return TRUE is the filename matches the given extension, else FALSE
 **/

bool_t httpCompExtension(const char_t *filename, const char_t *extension)
{
   uint_t n;
   uint_t m;

   //Get the length of the specified filename
   n = osStrlen(filename);
   //Get the length of the extension
   m = osStrlen(extension);

   //Check the length of the filename
   if(n < m)
      return FALSE;

   //Compare extensions
   if(!osStrncasecmp(filename + n - m, extension, m))
      return TRUE;
   else
      return FALSE;
}


/**
 * @brief Decode a percent-encoded string
 * @param[in] input NULL-terminated string to be decoded
 * @param[out] output NULL-terminated string resulting from the decoding process
 * @param[in] outputSize Size of the output buffer in bytes
 * @return Error code
 **/

error_t httpDecodePercentEncodedString(const char_t *input,
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
 * @brief Convert byte array to hex string
 * @param[in] input Point to the byte array
 * @param[in] inputLen Length of the byte array
 * @param[out] output NULL-terminated string resulting from the conversion
 **/

void httpConvertArrayToHexString(const uint8_t *input,
   size_t inputLen, char_t *output)
{
   size_t i;

   //Hex conversion table
   static const char_t hexDigit[16] =
   {
      '0', '1', '2', '3', '4', '5', '6', '7',
      '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
   };

   //Process byte array
   for(i = 0; i < inputLen; i++)
   {
      //Convert upper nibble
      output[i * 2] = hexDigit[(input[i] >> 4) & 0x0F];
      //Then convert lower nibble
      output[i * 2 + 1] = hexDigit[input[i] & 0x0F];
   }

   //Properly terminate the string with a NULL character
   output[i * 2] = '\0';
}

#endif
