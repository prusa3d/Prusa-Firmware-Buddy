/**
 * @file http_client.c
 * @brief HTTP client (HyperText Transfer Protocol)
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
 * @section Description
 *
 * The Hypertext Transfer Protocol (HTTP) is an application-level protocol for
 * distributed, collaborative, hypermedia information systems. Refer to the
 * following RFCs for complete details:
 * - RFC 1945: Hypertext Transfer Protocol - HTTP/1.0
 * - RFC 2616: Hypertext Transfer Protocol - HTTP/1.1
 * - RFC 2818: HTTP Over TLS
 * - RFC 7230: Hypertext Transfer Protocol (HTTP/1.1): Message Syntax and Routing
 * - RFC 7231: Hypertext Transfer Protocol (HTTP/1.1): Semantics and Content
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

//Switch to the appropriate trace level
#define TRACE_LEVEL HTTP_TRACE_LEVEL

//Dependencies
#include <limits.h>
#include <stdarg.h>
#include "core/net.h"
#include "http/http_client.h"
#include "http/http_client_auth.h"
#include "http/http_client_transport.h"
#include "http/http_client_misc.h"
#include "str.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (HTTP_CLIENT_SUPPORT == ENABLED)


/**
 * @brief Initialize HTTP client context
 * @param[in] context Pointer to the HTTP client context
 * @return Error code
 **/

error_t httpClientInit(HttpClientContext *context)
{
#if (HTTP_CLIENT_TLS_SUPPORT == ENABLED)
   error_t error;
#endif

   //Make sure the HTTP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Clear HTTP client context
   osMemset(context, 0, sizeof(HttpClientContext));

#if (HTTP_CLIENT_TLS_SUPPORT == ENABLED)
   //Initialize TLS session state
   error = tlsInitSessionState(&context->tlsSession);
   //Any error to report?
   if(error)
      return error;
#endif

   //Initialize HTTP connection state
   context->state = HTTP_CLIENT_STATE_DISCONNECTED;
   //Initialize HTTP request state
   context->requestState = HTTP_REQ_STATE_INIT;

   //Default protocol version
   context->version = HTTP_VERSION_1_1;
   //Default timeout
   context->timeout = HTTP_CLIENT_DEFAULT_TIMEOUT;

   //Successful initialization
   return NO_ERROR;
}


#if (HTTP_CLIENT_TLS_SUPPORT == ENABLED)

/**
 * @brief Register TLS initialization callback function
 * @param[in] context Pointer to the HTTP client context
 * @param[in] callback TLS initialization callback function
 * @return Error code
 **/

error_t httpClientRegisterTlsInitCallback(HttpClientContext *context,
   HttpClientTlsInitCallback callback)
{
   //Make sure the HTTP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Save callback function
   context->tlsInitCallback = callback;

   //Successful processing
   return NO_ERROR;
}

#endif


/**
 * @brief Register random data generation callback function
 * @param[in] context Pointer to the HTTP client context
 * @param[in] callback Random data generation callback function
 * @return Error code
 **/

error_t httpClientRegisterRandCallback(HttpClientContext *context,
   HttpClientRandCallback callback)
{
#if (HTTP_CLIENT_DIGEST_AUTH_SUPPORT == ENABLED)
   //Make sure the HTTP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Save callback function
   context->randCallback = callback;

   //Successful processing
   return NO_ERROR;
#else
   //Digest authentication is not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Set the HTTP protocol version to be used
 * @param[in] context Pointer to the HTTP client context
 * @param[in] version HTTP protocol version (1.0 or 1.1)
 * @return Error code
 **/

error_t httpClientSetVersion(HttpClientContext *context, HttpVersion version)
{
   //Make sure the HTTP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Check HTTP version
   if(version != HTTP_VERSION_1_0 && version != HTTP_VERSION_1_1)
      return ERROR_INVALID_VERSION;

   //Save the protocol version to be used
   context->version = version;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set communication timeout
 * @param[in] context Pointer to the HTTP client context
 * @param[in] timeout Timeout value, in milliseconds
 * @return Error code
 **/

error_t httpClientSetTimeout(HttpClientContext *context, systime_t timeout)
{
   //Make sure the HTTP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Save timeout value
   context->timeout = timeout;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set authentication information
 * @param[in] context Pointer to the HTTP client context
 * @param[in] username NULL-terminated string containing the user name to be used
 * @param[in] password NULL-terminated string containing the password to be used
 * @return Error code
 **/

error_t httpClientSetAuthInfo(HttpClientContext *context,
   const char_t *username, const char_t *password)
{
#if (HTTP_CLIENT_AUTH_SUPPORT == ENABLED)
   //Check parameters
   if(context == NULL || username == NULL || password == NULL)
      return ERROR_INVALID_PARAMETER;

   //Make sure the length of the user name is acceptable
   if(osStrlen(username) > HTTP_CLIENT_MAX_USERNAME_LEN)
      return ERROR_INVALID_LENGTH;

   //Save user name
   osStrcpy(context->authParams.username, username);

   //Make sure the length of the password is acceptable
   if(osStrlen(password) > HTTP_CLIENT_MAX_PASSWORD_LEN)
      return ERROR_INVALID_LENGTH;

   //Save password
   osStrcpy(context->authParams.password, password);

   //Successful processing
   return NO_ERROR;
#else
   //HTTP authentication is not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Bind the HTTP client to a particular network interface
 * @param[in] context Pointer to the HTTP client context
 * @param[in] interface Network interface to be used
 * @return Error code
 **/

error_t httpClientBindToInterface(HttpClientContext *context,
   NetInterface *interface)
{
   //Make sure the HTTP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Explicitly associate the HTTP client with the specified interface
   context->interface = interface;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Establish a connection with the specified HTTP server
 * @param[in] context Pointer to the HTTP client context
 * @param[in] serverIpAddr IP address of the HTTP server to connect to
 * @param[in] serverPort Port number
 * @return Error code
 **/

error_t httpClientConnect(HttpClientContext *context,
   const IpAddr *serverIpAddr, uint16_t serverPort)
{
   error_t error;

   //Make sure the HTTP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Establish connection with the HTTP server
   while(!error)
   {
      //Check HTTP connection state
      if(context->state == HTTP_CLIENT_STATE_DISCONNECTED)
      {
         //First connection attempt?
         if(serverIpAddr != NULL)
         {
            //Save the IP address of the HTTP server
            context->serverIpAddr = *serverIpAddr;
            //Save the TCP port number to be used
            context->serverPort = serverPort;

#if (HTTP_CLIENT_AUTH_SUPPORT == ENABLED)
            //HTTP authentication is not used for the first connection attempt
            httpClientInitAuthParams(&context->authParams);
#endif
         }

         //Open network connection
         error = httpClientOpenConnection(context);

         //Check status code
         if(!error)
         {
            //Establish network connection
            httpClientChangeState(context, HTTP_CLIENT_STATE_CONNECTING);
         }
      }
      else if(context->state == HTTP_CLIENT_STATE_CONNECTING)
      {
         //Establish network connection
         error = httpClientEstablishConnection(context, &context->serverIpAddr,
            context->serverPort);

         //Check status code
         if(error == NO_ERROR)
         {
            //The client is connected to the HTTP server
            httpClientChangeState(context, HTTP_CLIENT_STATE_CONNECTED);
         }
         else if(error == ERROR_WOULD_BLOCK || error == ERROR_TIMEOUT)
         {
            //Check whether the timeout has elapsed
            error = httpClientCheckTimeout(context);
         }
         else
         {
            //A communication error has occurred
         }
      }
      else if(context->state == HTTP_CLIENT_STATE_CONNECTED)
      {
         //The HTTP client is connected
         break;
      }
      else
      {
         //Invalid state
         error = ERROR_WRONG_STATE;
      }
   }

   //Failed to establish connection with the HTTP server?
   if(error != NO_ERROR && error != ERROR_WOULD_BLOCK)
   {
      //Clean up side effects
      httpClientCloseConnection(context);
      //Update HTTP connection state
      httpClientChangeState(context, HTTP_CLIENT_STATE_DISCONNECTED);
   }

   //Return status code
   return error;
}


/**
 * @brief Create a new HTTP request
 * @param[in] context Pointer to the HTTP client context
 * @return Error code
 **/

error_t httpClientCreateRequest(HttpClientContext *context)
{
   error_t error;

   //Make sure the HTTP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Format default HTTP request header
   error = httpClientFormatRequestHeader(context);

   //Check status code
   if(!error)
   {
      //The HTTP request body is empty
      context->bodyLen = 0;
      context->bodyPos = 0;

      //Reset status code
      context->statusCode = 0;

      //Update HTTP request state
      httpClientChangeRequestState(context, HTTP_REQ_STATE_FORMAT_HEADER);
   }

   //Return status code
   return error;
}


/**
 * @brief Set HTTP request method
 * @param[in] context Pointer to the HTTP client context
 * @param[in] method NULL-terminating string containing the HTTP method
 * @return Error code
 **/

error_t httpClientSetMethod(HttpClientContext *context, const char_t *method)
{
   size_t m;
   size_t n;
   char_t *p;

   //Check parameters
   if(context == NULL || method == NULL)
      return ERROR_INVALID_PARAMETER;

   //Compute the length of the HTTP method
   n = osStrlen(method);

   //Make sure the length of the user name is acceptable
   if(n == 0 || n > HTTP_CLIENT_MAX_METHOD_LEN)
      return ERROR_INVALID_LENGTH;

   //Make sure the buffer contains a valid HTTP request
   if(context->bufferLen > HTTP_CLIENT_BUFFER_SIZE)
      return ERROR_INVALID_SYNTAX;

   //Properly terminate the string with a NULL character
   context->buffer[context->bufferLen] = '\0';

   //The Request-Line begins with a method token
   p = osStrchr(context->buffer, ' ');
   //Any parsing error?
   if(p == NULL)
      return ERROR_INVALID_SYNTAX;

   //Compute the length of the current method token
   m = p - context->buffer;

   //Make sure the buffer is large enough to hold the new HTTP request method
   if((context->bufferLen + n - m) > HTTP_CLIENT_BUFFER_SIZE)
      return ERROR_BUFFER_OVERFLOW;

   //Make room for the new method token
   osMemmove(context->buffer + n, p, context->bufferLen + 1 - m);
   //Copy the new method token
   osStrncpy(context->buffer, method, n);

   //Adjust the length of the request header
   context->bufferLen = context->bufferLen + n - m;

   //Save HTTP request method
   osStrcpy(context->method, method);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set request URI
 * @param[in] context Pointer to the HTTP client context
 * @param[in] uri NULL-terminated string that contains the resource name
 * @return Error code
 **/

error_t httpClientSetUri(HttpClientContext *context, const char_t *uri)
{
   size_t m;
   size_t n;
   char_t *p;
   char_t *q;

   //Check parameters
   if(context == NULL || uri == NULL)
      return ERROR_INVALID_PARAMETER;

   //The resource name must not be empty
   if(uri[0] == '\0')
      return ERROR_INVALID_PARAMETER;

   //Check HTTP request state
   if(context->requestState != HTTP_REQ_STATE_FORMAT_HEADER)
      return ERROR_WRONG_STATE;

   //Make sure the buffer contains a valid HTTP request
   if(context->bufferLen > HTTP_CLIENT_BUFFER_SIZE)
      return ERROR_INVALID_SYNTAX;

   //Properly terminate the string with a NULL character
   context->buffer[context->bufferLen] = '\0';

   //The Request-Line begins with a method token
   p = osStrchr(context->buffer, ' ');
   //Any parsing error?
   if(p == NULL)
      return ERROR_INVALID_SYNTAX;

   //The method token is followed by the Request-URI
   p++;

   //Point to the end of the Request-URI
   q = strpbrk(p, " ?");
   //Any parsing error?
   if(q == NULL)
      return ERROR_INVALID_SYNTAX;

   //Compute the length of the current URI
   m = q - p;
   //Compute the length of the new URI
   n = osStrlen(uri);

   //Make sure the buffer is large enough to hold the new resource name
   if((context->bufferLen + n - m) > HTTP_CLIENT_BUFFER_SIZE)
      return ERROR_BUFFER_OVERFLOW;

   //Make room for the new resource name
   osMemmove(p + n, q, context->buffer + context->bufferLen + 1 - q);
   //Copy the new resource name
   osStrncpy(p, uri, n);

   //Adjust the length of the request header
   context->bufferLen = context->bufferLen + n - m;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set the hostname and port number of the resource being requested
 * @param[in] context Pointer to the HTTP client context
 * @param[in] host NULL-terminated string containing the hostname
 * @param[in] port TCP port number
 * @return Error code
 **/

error_t httpClientSetHost(HttpClientContext *context, const char_t *host,
   uint16_t port)
{
   size_t m;
   size_t n;
   char_t temp[7];

   //Check parameters
   if(context == NULL || host == NULL)
      return ERROR_INVALID_PARAMETER;

   //Check HTTP request state
   if(context->requestState != HTTP_REQ_STATE_FORMAT_HEADER)
      return ERROR_WRONG_STATE;

   //Make sure the buffer contains a valid HTTP request
   if(context->bufferLen < 2 || context->bufferLen > HTTP_CLIENT_BUFFER_SIZE)
      return ERROR_INVALID_SYNTAX;

   //Default port number?
   if(port == HTTP_PORT)
   {
      //A host without any trailing port information implies the default port
      //for the service requested
      osStrcpy(temp, "");
   }
   else
   {
      //Format port number information
      osSprintf(temp, ":%" PRIu16, port);
   }

   //Compute the length of the hostname
   n = osStrlen(host);
   //Compute the length of the trailing port information
   m = osStrlen(temp);

   //Make sure the buffer is large enough to hold the new header field
   if((context->bufferLen + n + m + 8) > HTTP_CLIENT_BUFFER_SIZE)
      return ERROR_BUFFER_OVERFLOW;

   //The Host request-header field specifies the Internet host and port number
   //of the resource being requested
   osSprintf(context->buffer + context->bufferLen - 2, "Host: %s%s\r\n\r\n",
      host, temp);

   //Adjust the length of the request header
   context->bufferLen += n + m + 8;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set query string
 * @param[in] context Pointer to the HTTP client context
 * @param[in] queryString NULL-terminated string that contains the query string
 * @return Error code
 **/

error_t httpClientSetQueryString(HttpClientContext *context,
   const char_t *queryString)
{
   size_t m;
   size_t n;
   char_t *p;
   char_t *q;

   //Check parameters
   if(context == NULL || queryString == NULL)
      return ERROR_INVALID_PARAMETER;

   //Check HTTP request state
   if(context->requestState != HTTP_REQ_STATE_FORMAT_HEADER)
      return ERROR_WRONG_STATE;

   //Make sure the buffer contains a valid HTTP request
   if(context->bufferLen > HTTP_CLIENT_BUFFER_SIZE)
      return ERROR_INVALID_SYNTAX;

   //Properly terminate the string with a NULL character
   context->buffer[context->bufferLen] = '\0';

   //The Request-Line begins with a method token
   p = osStrchr(context->buffer, ' ');
   //Any parsing error?
   if(p == NULL)
      return ERROR_INVALID_SYNTAX;

   //The method token is followed by the Request-URI
   p = strpbrk(p + 1, " ?");
   //Any parsing error?
   if(p == NULL)
      return ERROR_INVALID_SYNTAX;

   //The question mark is used as a separator
   if(*p == '?')
   {
      //Point to the end of the query string
      q = osStrchr(p + 1, ' ');
      //Any parsing error?
      if(q == NULL)
         return ERROR_INVALID_SYNTAX;

      //Compute the length of the actual query string
      m = q - p;
   }
   else
   {
      //The query string is not present
      q = p;
      m = 0;
   }

   //Compute the length of the new query string
   n = osStrlen(queryString);

   //Empty query string?
   if(n == 0)
   {
      //Remove the query string
      osMemmove(p, p + m, context->buffer + context->bufferLen + 1 - q);
   }
   else
   {
      //The question mark is not part of the query string
      n++;

      //Make sure the buffer is large enough to hold the new query string
      if((context->bufferLen + n - m) > HTTP_CLIENT_BUFFER_SIZE)
         return ERROR_BUFFER_OVERFLOW;

      //Make room for the new query string
      osMemmove(p + n, q, context->buffer + context->bufferLen + 1 - q);

      //The question mark is used as a separator
      p[0] = '?';
      //Copy the new query string
      osStrncpy(p + 1, queryString, n - 1);
   }

   //Adjust the length of the request header
   context->bufferLen = context->bufferLen + n - m;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Add a key/value pair to the query string
 * @param[in] context Pointer to the HTTP client context
 * @param[in] name NULL-terminated string that holds the parameter name
 * @param[in] value NULL-terminated string that holds the parameter value
 * @return Error code
 **/

error_t httpClientAddQueryParam(HttpClientContext *context,
   const char_t *name, const char_t *value)
{
   size_t nameLen;
   size_t valueLen;
   char_t separator;
   char_t *p;

   //Check parameters
   if(context == NULL || name == NULL)
      return ERROR_INVALID_PARAMETER;

   //The parameter name must not be empty
   if(name[0] == '\0')
      return ERROR_INVALID_PARAMETER;

   //Check HTTP request state
   if(context->requestState != HTTP_REQ_STATE_FORMAT_HEADER)
      return ERROR_WRONG_STATE;

   //Make sure the buffer contains a valid HTTP request
   if(context->bufferLen > HTTP_CLIENT_BUFFER_SIZE)
      return ERROR_INVALID_SYNTAX;

   //Properly terminate the string with a NULL character
   context->buffer[context->bufferLen] = '\0';

   //The Request-Line begins with a method token
   p = osStrchr(context->buffer, ' ');
   //Any parsing error?
   if(p == NULL)
      return ERROR_INVALID_SYNTAX;

   //The method token is followed by the Request-URI
   p = strpbrk(p + 1, " ?");
   //Any parsing error?
   if(p == NULL)
      return ERROR_INVALID_SYNTAX;

   //The question mark is used as a separator
   if(*p == '?')
   {
      //Point to the end of the query string
      p = osStrchr(p + 1, ' ');
      //Any parsing error?
      if(p == NULL)
         return ERROR_INVALID_SYNTAX;

      //multiple query parameters are separated by an ampersand
      separator = '&';
   }
   else
   {
      //The query string is not present
      separator = '?';
   }

   //Compute the length of the parameter value
   nameLen = osStrlen(name);

   //Empty parameter value?
   if(value == NULL)
   {
      //Make sure the buffer is large enough to hold the new query parameter
      if((context->bufferLen + nameLen + 1) > HTTP_CLIENT_BUFFER_SIZE)
         return ERROR_BUFFER_OVERFLOW;

      //Make room for the new query parameter
      osMemmove(p + nameLen + 1, p, context->buffer + context->bufferLen + 1 - p);

      //Multiple query parameters are separated by a delimiter
      p[0] = separator;
      //Copy parameter name
      osStrncpy(p + 1, name, nameLen);

      //Adjust the length of the request header
      context->bufferLen += nameLen + 1;
   }
   else
   {
      //Compute the length of the parameter value
      valueLen = osStrlen(value);

      //Make sure the buffer is large enough to hold the new query parameter
      if((context->bufferLen + nameLen + valueLen + 2) > HTTP_CLIENT_BUFFER_SIZE)
         return ERROR_BUFFER_OVERFLOW;

      //Make room for the new query parameter
      osMemmove(p + nameLen + valueLen + 2, p, context->buffer +
         context->bufferLen + 1 - p);

      //Multiple query parameters are separated by a delimiter
      p[0] = separator;
      //Copy parameter name
      osStrncpy(p + 1, name, nameLen);
      //The field name and value are separated by an equals sign
      p[nameLen + 1] = '=';
      //Copy parameter value
      osStrncpy(p + nameLen + 2, value, valueLen);

      //Adjust the length of the request header
      context->bufferLen += nameLen + valueLen + 2;
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Add a header field to the HTTP request
 * @param[in] context Pointer to the HTTP client context
 * @param[in] name NULL-terminated string that holds the header field name
 * @param[in] value NULL-terminated string that holds the header field value
 * @return Error code
 **/

error_t httpClientAddHeaderField(HttpClientContext *context,
   const char_t *name, const char_t *value)
{
   error_t error;
   size_t n;
   size_t nameLen;
   size_t valueLen;

   //Check parameters
   if(context == NULL || name == NULL || value == NULL)
      return ERROR_INVALID_PARAMETER;

   //The field name must not be empty
   if(name[0] == '\0')
      return ERROR_INVALID_PARAMETER;

   //Check HTTP request state
   if(context->requestState != HTTP_REQ_STATE_FORMAT_HEADER &&
      context->requestState != HTTP_REQ_STATE_FORMAT_TRAILER)
   {
      return ERROR_WRONG_STATE;
   }

   //Make sure the buffer contains a valid HTTP request
   if(context->bufferLen < 2 || context->bufferLen > HTTP_CLIENT_BUFFER_SIZE)
      return ERROR_INVALID_SYNTAX;

   //Retrieve the length of the field name and value
   nameLen = osStrlen(name);
   valueLen = osStrlen(value);

   //Determine the length of the new header field
   n = nameLen + valueLen + 4;

   //Make sure the buffer is large enough to hold the new header field
   if((context->bufferLen + n) > HTTP_CLIENT_BUFFER_SIZE)
      return ERROR_BUFFER_OVERFLOW;

   //Each header field consists of a case-insensitive field name followed
   //by a colon, optional leading whitespace and the field value
   osSprintf(context->buffer + context->bufferLen - 2, "%s: %s\r\n\r\n",
      name, value);

   //Check header field name
   if(!osStrcasecmp(name, "Connection"))
   {
      //Parse Connection header field
      error = httpClientParseConnectionField(context, value);
   }
   else if(!osStrcasecmp(name, "Transfer-Encoding"))
   {
      //Parse Transfer-Encoding header field
      error = httpClientParseTransferEncodingField(context, value);
   }
   else if(!osStrcasecmp(name, "Content-Length"))
   {
      //Parse Content-Length header field
      error = httpClientParseContentLengthField(context, value);
   }
   else
   {
      //Discard unknown header fields
      error = NO_ERROR;
   }

   //Adjust the length of the request header
   context->bufferLen += n;

   //Return status code
   return error;
}


/**
 * @brief Format an HTTP header field
 * @param[in] context Pointer to the HTTP client context
 * @param[in] name NULL-terminated string that holds the header field name
 * @param[in] format NULL-terminated string that that contains a format string
 * @param[in] ... Optional arguments
 * @return Error code
 **/

error_t httpClientFormatHeaderField(HttpClientContext *context,
   const char_t *name, const char_t *format, ...)
{
   error_t error;
   size_t n;
   size_t size;
   size_t nameLen;
   char_t *value;
   va_list args;

   //Check parameters
   if(context == NULL || name == NULL || format == NULL)
      return ERROR_INVALID_PARAMETER;

   //The field name must not be empty
   if(name[0] == '\0')
      return ERROR_INVALID_PARAMETER;

   //Check HTTP request state
   if(context->requestState != HTTP_REQ_STATE_FORMAT_HEADER &&
      context->requestState != HTTP_REQ_STATE_FORMAT_TRAILER)
   {
      return ERROR_WRONG_STATE;
   }

   //Make sure the buffer contains a valid HTTP request
   if(context->bufferLen < 2 || context->bufferLen > HTTP_CLIENT_BUFFER_SIZE)
      return ERROR_INVALID_SYNTAX;

   //Retrieve the length of the field name
   nameLen = osStrlen(name);

   //Make sure the buffer is large enough to hold the new header field
   if((context->bufferLen + nameLen + 4) > HTTP_CLIENT_BUFFER_SIZE)
      return ERROR_BUFFER_OVERFLOW;

   //Point to the buffer where to format the field value
   value = context->buffer + context->bufferLen + nameLen;
   //Calculate the maximum size of the formatted string
   size = HTTP_CLIENT_BUFFER_SIZE - context->bufferLen - nameLen - 4;

   //Initialize processing of a varying-length argument list
   va_start(args, format);
   //Format field value
   n = osVsnprintf(value, size, format, args);
   //End varying-length argument list processing
   va_end(args);

   //A return value of size or more means that the output was truncated
   if(n >= size)
      return ERROR_BUFFER_OVERFLOW;

   //Each header field consists of a case-insensitive field name followed
   //by a colon, optional leading whitespace and the field value
   osStrncpy(context->buffer + context->bufferLen - 2, name, nameLen);
   osStrncpy(context->buffer + context->bufferLen + nameLen - 2, ": ", 2);

   //Check header field name
   if(!osStrcasecmp(name, "Connection"))
   {
      //Parse Connection header field
      error = httpClientParseConnectionField(context, value);
   }
   else if(!osStrcasecmp(name, "Transfer-Encoding"))
   {
      //Parse Transfer-Encoding header field
      error = httpClientParseTransferEncodingField(context, value);
   }
   else if(!osStrcasecmp(name, "Content-Length"))
   {
      //Parse Content-Length header field
      error = httpClientParseContentLengthField(context, value);
   }
   else
   {
      //Discard unknown header fields
      error = NO_ERROR;
   }

   //Terminate the header field with a CRLF sequence
   osStrcpy(context->buffer + context->bufferLen + nameLen + n, "\r\n\r\n");

   //Adjust the length of the request header
   context->bufferLen += nameLen + n + 4;

   //Return status code
   return error;
}


/**
 * @brief Set the length of the HTTP request body
 * @param[in] context Pointer to the HTTP client context
 * @param[in] length Length of the HTTP request body, in bytes
 * @return Error code
 **/

error_t httpClientSetContentLength(HttpClientContext *context, size_t length)
{
   char_t temp[11];

   //Make sure the HTTP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Check HTTP request state
   if(context->requestState != HTTP_REQ_STATE_FORMAT_HEADER)
      return ERROR_WRONG_STATE;

   //The Content-Length header field indicates the size of the body, in
   //decimal number of octets
   osSprintf(temp, "%" PRIuSIZE, length);

   //Add the Content-Length header field
   return httpClientAddHeaderField(context, "Content-Length", temp);
}


/**
 * @brief Write HTTP request header
 * @param[in] context Pointer to the HTTP client context
 * @return Error code
 **/

error_t httpClientWriteHeader(HttpClientContext *context)
{
   error_t error;
   size_t n;

   //Make sure the HTTP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Send HTTP request header
   while(!error)
   {
      //Check HTTP connection state
      if(context->state == HTTP_CLIENT_STATE_DISCONNECTED ||
         context->state == HTTP_CLIENT_STATE_CONNECTING)
      {
         //If the HTTP connection is not persistent, then a new connection
         //must be established
         error = httpClientConnect(context, NULL, 0);
      }
      else if(context->state == HTTP_CLIENT_STATE_CONNECTED)
      {
         //Check HTTP request state
         if(context->requestState == HTTP_REQ_STATE_FORMAT_HEADER)
         {
#if (HTTP_CLIENT_AUTH_SUPPORT == ENABLED)
            //HTTP authentication requested by the server?
            if(context->authParams.mode != HTTP_AUTH_MODE_NONE &&
               context->authParams.username[0] != '\0')
            {
               //Format Authorization header field
               error = httpClientFormatAuthorizationField(context);
            }
#endif
            //Check status code
            if(!error)
            {
               //Dump HTTP request header
               TRACE_DEBUG("HTTP request header:\r\n%s", context->buffer);

               //Initiate the sending process
               httpClientChangeRequestState(context, HTTP_REQ_STATE_SEND_HEADER);
            }
         }
         else if(context->requestState == HTTP_REQ_STATE_SEND_HEADER)
         {
            //Any remaining data to be sent?
            if(context->bufferPos < context->bufferLen)
            {
               //Send more data
               error = httpClientSendData(context,
                  context->buffer + context->bufferPos,
                  context->bufferLen - context->bufferPos, &n, 0);

               //Check status code
               if(error == NO_ERROR || error == ERROR_TIMEOUT)
               {
                  //Advance data pointer
                  context->bufferPos += n;
               }
            }
            else
            {
               //The request header has been successfully transmitted
               if(!osStrcasecmp(context->method, "POST") ||
                  !osStrcasecmp(context->method, "PUT") ||
                  !osStrcasecmp(context->method, "PATCH"))
               {
                  //POST, PUT and PATCH requests have a body
                  httpClientChangeRequestState(context, HTTP_REQ_STATE_SEND_BODY);
               }
               else
               {
                  //GET, HEAD and DELETE requests do not have a body
                  httpClientChangeRequestState(context,
                     HTTP_REQ_STATE_RECEIVE_STATUS_LINE);
               }
            }
         }
         else if(context->requestState == HTTP_REQ_STATE_SEND_BODY ||
            context->requestState == HTTP_REQ_STATE_RECEIVE_STATUS_LINE)
         {
            //We are done
            break;
         }
         else
         {
            //Invalid HTTP request state
            error = ERROR_WRONG_STATE;
         }
      }
      else
      {
         //Invalid HTTP connection state
         error = ERROR_WRONG_STATE;
      }
   }

   //Check status code
   if(error == ERROR_WOULD_BLOCK || error == ERROR_TIMEOUT)
   {
      //Check whether the timeout has elapsed
      error = httpClientCheckTimeout(context);
   }

   //Return status code
   return error;
}


/**
 * @brief Write HTTP request body
 * @param[in] context Pointer to the HTTP client context
 * @param[in] data Pointer to the buffer containing the data to be transmitted
 * @param[in] length Number of data bytes to send
 * @param[out] written Actual number of bytes written (optional parameter)
 * @param[in] flags Set of flags that influences the behavior of this function
 * @return Error code
 **/

error_t httpClientWriteBody(HttpClientContext *context, const void *data,
   size_t length, size_t *written, uint_t flags)
{
   error_t error;
   size_t n;
   size_t totalLength;

   //Make sure the HTTP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Check HTTP connection state
   if(context->state != HTTP_CLIENT_STATE_CONNECTED)
      return ERROR_WRONG_STATE;

   //Initialize status code
   error = NO_ERROR;

   //Actual number of bytes written
   totalLength = 0;

   //Send as much data as possible
   while(totalLength < length && !error)
   {
      //Check HTTP request state
      if(context->requestState == HTTP_REQ_STATE_SEND_BODY)
      {
         //Chunked transfer encoding?
         if(context->chunkedEncoding)
         {
            //The chunked encoding modifies the body in order to transfer it
            //as a series of chunks, each with its own size indicator
            error = httpClientFormatChunkSize(context, length - totalLength);
         }
         else
         {
            //Number of bytes left to send
            n = length - totalLength;

            //The length of the body shall not exceed the value specified in
            //the Content-Length field
            if(n <= (context->bodyLen - context->bodyPos))
            {
               //Send request body
               error = httpClientSendData(context, data, n, &n, flags);

               //Check status code
               if(error == NO_ERROR || error == ERROR_TIMEOUT)
               {
                  //Any data transmitted?
                  if(n > 0)
                  {
                     //Advance data pointer
                     data = (uint8_t *) data + n;
                     totalLength += n;
                     context->bodyPos += n;

                     //Save current time
                     context->timestamp = osGetSystemTime();
                  }
               }
            }
            else
            {
               //Report an error
               error = ERROR_INVALID_LENGTH;
            }
         }
      }
      else if(context->requestState == HTTP_REQ_STATE_SEND_CHUNK_SIZE)
      {
         //Send the chunk-size field
         if(context->bufferPos < context->bufferLen)
         {
            //Send more data
            error = httpClientSendData(context,
               context->buffer + context->bufferPos,
               context->bufferLen - context->bufferPos, &n, 0);

            //Check status code
            if(error == NO_ERROR || error == ERROR_TIMEOUT)
            {
               //Advance data pointer
               context->bufferPos += n;
            }
         }
         else
         {
            //Send chunk data
            httpClientChangeRequestState(context, HTTP_REQ_STATE_SEND_CHUNK_DATA);
         }
      }
      else if(context->requestState == HTTP_REQ_STATE_SEND_CHUNK_DATA)
      {
         //The data stream is divided into a series of chunks
         if(context->bodyPos < context->bodyLen)
         {
            //The length of the chunk shall not exceed the value specified in
            //the chunk-size field
            n = MIN(length - totalLength, context->bodyLen - context->bodyPos);

            //Send chunk data
            error = httpClientSendData(context, data, n, &n, flags);

            //Check status code
            if(error == NO_ERROR || error == ERROR_TIMEOUT)
            {
               //Any data transmitted?
               if(n > 0)
               {
                  //Advance data pointer
                  data = (uint8_t *) data + n;
                  totalLength += n;
                  context->bodyPos += n;

                  //Save current time
                  context->timestamp = osGetSystemTime();
               }
            }
         }
         else
         {
            //The chunked encoding modifies the body in order to transfer it
            //as a series of chunks, each with its own size indicator
            error = httpClientFormatChunkSize(context, length - totalLength);
         }
      }
      else
      {
         //Invalid state
         error = ERROR_WRONG_STATE;
      }
   }

   //Check status code
   if(error == ERROR_WOULD_BLOCK || error == ERROR_TIMEOUT)
   {
      //Check whether the timeout has elapsed
      error = httpClientCheckTimeout(context);
   }

   //Total number of data that have been written
   if(written != NULL)
      *written = totalLength;

   //Return status code
   return error;
}


/**
 * @brief Write HTTP trailer
 * @param[in] context Pointer to the HTTP client context
 * @return Error code
 **/

error_t httpClientWriteTrailer(HttpClientContext *context)
{
   error_t error;
   size_t n;

   //Make sure the HTTP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Check HTTP connection state
   if(context->state != HTTP_CLIENT_STATE_CONNECTED)
      return ERROR_WRONG_STATE;

   //Initialize status code
   error = NO_ERROR;

   //Send HTTP request header
   while(!error)
   {
      //Check HTTP request state
      if(context->requestState == HTTP_REQ_STATE_FORMAT_TRAILER)
      {
         //Initiate the sending process
         httpClientChangeRequestState(context, HTTP_REQ_STATE_SEND_TRAILER);
      }
      else if(context->requestState == HTTP_REQ_STATE_SEND_TRAILER)
      {
         //Any remaining data to be sent?
         if(context->bufferPos < context->bufferLen)
         {
            //Send more data
            error = httpClientSendData(context,
               context->buffer + context->bufferPos,
               context->bufferLen - context->bufferPos, &n, 0);

            //Check status code
            if(error == NO_ERROR || error == ERROR_TIMEOUT)
            {
               //Advance data pointer
               context->bufferPos += n;
            }
         }
         else
         {
            //The request trailer has been successfully transmitted
            httpClientChangeRequestState(context,
               HTTP_REQ_STATE_RECEIVE_STATUS_LINE);
         }
      }
      else if(context->requestState == HTTP_REQ_STATE_RECEIVE_STATUS_LINE)
      {
         //We are done
         break;
      }
      else
      {
         //Invalid state
         error = ERROR_WRONG_STATE;
      }
   }

   //Check status code
   if(error == ERROR_WOULD_BLOCK || error == ERROR_TIMEOUT)
   {
      //Check whether the timeout has elapsed
      error = httpClientCheckTimeout(context);
   }

   //Return status code
   return error;
}


/**
 * @brief Read HTTP response header
 * @param[in] context Pointer to the HTTP client context
 * @return Error code
 **/

error_t httpClientReadHeader(HttpClientContext *context)
{
   error_t error;
   size_t n;

   //Make sure the HTTP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Check HTTP connection state
   if(context->state != HTTP_CLIENT_STATE_CONNECTED)
      return ERROR_WRONG_STATE;

   //Initialize status code
   error = NO_ERROR;

   //Send HTTP request header
   while(!error)
   {
      //Check HTTP request state
      if(context->requestState == HTTP_REQ_STATE_SEND_BODY ||
         context->requestState == HTTP_REQ_STATE_SEND_CHUNK_DATA)
      {
         //Close HTTP request body
         error = httpClientCloseBody(context);
      }
      else if(context->requestState == HTTP_REQ_STATE_FORMAT_TRAILER ||
         context->requestState == HTTP_REQ_STATE_SEND_TRAILER)
      {
         //The last chunk is followed by an optional trailer
         error = httpClientWriteTrailer(context);
      }
      else if(context->requestState == HTTP_REQ_STATE_RECEIVE_STATUS_LINE ||
         context->requestState == HTTP_REQ_STATE_RECEIVE_HEADER)
      {
         //The CRLF sequence is always used to terminate a line
         if(context->bufferLen >= (context->bufferPos + 2) &&
            context->buffer[context->bufferLen - 2] == '\r' &&
            context->buffer[context->bufferLen - 1] == '\n')
         {
            //Strip the CRLF at the end of the line
            context->bufferLen -= 2;

            //The first line of a response message is the Status-Line
            if(context->requestState == HTTP_REQ_STATE_RECEIVE_STATUS_LINE)
            {
               //Thee Status-Line consists of the protocol version followed by
               //a numeric status code and its associated textual phrase
               error = httpClientParseStatusLine(context, context->buffer +
                  context->bufferPos, context->bufferLen - context->bufferPos);

               //Valid syntax?
               if(!error)
               {
                  //Receive response header fields
                  httpClientChangeRequestState(context, HTTP_REQ_STATE_RECEIVE_HEADER);
               }
            }
            else
            {
               //An empty line indicates the end of the header fields
               if(context->bufferLen == context->bufferPos)
               {
                  //Save TLS session
                  error = httpClientSaveSession(context);

                  //Check status code
                  if(!error)
                  {
                     //Debug message
                     TRACE_DEBUG("\r\n");

                     //The HTTP response header has been successfully received
                     httpClientChangeRequestState(context, HTTP_REQ_STATE_PARSE_HEADER);
                  }
               }
               else
               {
                  //The response header fields allow the server to pass additional
                  //information about the response which cannot be placed in the
                  //Status-Line
                  error = httpClientParseHeaderField(context, context->buffer +
                     context->bufferPos, context->bufferLen - context->bufferPos);
               }
            }
         }
         else if(context->bufferLen < HTTP_CLIENT_BUFFER_SIZE)
         {
            //Receive more data
            error = httpClientReceiveData(context, context->buffer +
               context->bufferLen, HTTP_CLIENT_BUFFER_SIZE - context->bufferLen,
               &n, HTTP_FLAG_BREAK_CRLF);

            //Check status code
            if(!error)
            {
               //Adjust the length of the buffer
               context->bufferLen += n;
               //Save current time
               context->timestamp = osGetSystemTime();
            }
         }
         else
         {
            //The client implementation limits the size of headers it accepts
            error = ERROR_BUFFER_OVERFLOW;
         }
      }
      else if(context->requestState == HTTP_REQ_STATE_PARSE_HEADER)
      {
         //Rewind to the beginning of the buffer
         context->bufferPos = 0;
         //We are done
         break;
      }
      else
      {
         //Invalid state
         error = ERROR_WRONG_STATE;
      }
   }

   //Check status code
   if(error == ERROR_WOULD_BLOCK || error == ERROR_TIMEOUT)
   {
      //Check whether the timeout has elapsed
      error = httpClientCheckTimeout(context);
   }

   //Return status code
   return error;
}


/**
 * @brief Retrieve the HTTP status code of the response
 * @param[in] context Pointer to the HTTP client context
 * @return HTTP status code
 **/

uint_t httpClientGetStatus(HttpClientContext *context)
{
   uint_t status;

   //Initialize status code
   status = 0;

   //Make sure the HTTP client context is valid
   if(context != NULL)
   {
      //The status code is a three-digit integer code giving the result
      //of the attempt to understand and satisfy the request
      status = context->statusCode;
   }

   //Return HTTP status code
   return status;
}


/**
 * @brief Retrieve the value of the specified header field name
 * @param[in] context Pointer to the HTTP client context
 * @param[in] name NULL-terminated string that specifies the header field name
 * @return Value of the header field
 **/

const char_t *httpClientGetHeaderField(HttpClientContext *context,
   const char_t *name)
{
   size_t i;
   size_t nameLen;
   size_t valueLen;
   const char_t *value;

   //Initialize field value
   value = NULL;

   //Check parameters
   if(context != NULL && name != NULL)
   {
      //Check HTTP request state
      if(context->requestState == HTTP_REQ_STATE_PARSE_HEADER ||
         context->requestState == HTTP_REQ_STATE_PARSE_TRAILER)
      {
         //Point to the first header field of the response
         i = 0;

         //Parse HTTP response header
         while(i < context->bufferLen)
         {
            //Calculate the length of the field name
            nameLen = osStrlen(context->buffer + i);
            //Calculate the length of the field value
            valueLen = osStrlen(context->buffer + i + nameLen + 1);

            //Check whether the current header field matches the specified name
            if(!osStrcasecmp(context->buffer + i, name))
            {
               //Retrieve the value of the header field
               value = context->buffer + i + nameLen + 1;
               //Exit immediately
               break;
            }

            //Point to next header field
            i += nameLen + valueLen + 2;
         }
      }
   }

   //Return the value of the header field
   return value;
}


/**
 * @brief Iterate through the HTTP response header
 * @param[in] context Pointer to the HTTP client context
 * @param[out] name NULL-terminated string that contains the name of the next
 *   header field
 * @param[out] value NULL-terminated string that contains the value of the next
 *   header field
 * @return Error code
 **/

error_t httpClientGetNextHeaderField(HttpClientContext *context,
   const char_t **name, const char_t **value)
{
   size_t nameLen;
   size_t valueLen;

   //Check parameters
   if(context == NULL || name == NULL || value == NULL)
      return ERROR_INVALID_PARAMETER;

   //Check HTTP request state
   if(context->requestState != HTTP_REQ_STATE_PARSE_HEADER &&
      context->requestState != HTTP_REQ_STATE_PARSE_TRAILER)
   {
      return ERROR_WRONG_STATE;
   }

   //Check whether the end of the HTTP response header has been reached
   if(context->bufferPos >= context->bufferLen)
      return ERROR_END_OF_STREAM;

   //Calculate the length of the field name
   nameLen = osStrlen(context->buffer + context->bufferPos);
   //Calculate the length of the field value
   valueLen = osStrlen(context->buffer + context->bufferPos + nameLen + 1);

   //Return the name and the value of the header field
   *name = context->buffer + context->bufferPos;
   *value = context->buffer + context->bufferPos + nameLen + 1;

   //Point to the next header field
   context->bufferPos += nameLen + valueLen + 2;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Read HTTP response body
 * @param[in] context Pointer to the HTTP client context
 * @param[out] data Buffer where to store the incoming data
 * @param[in] size Maximum number of bytes that can be received
 * @param[out] received Number of bytes that have been received
 * @param[in] flags Set of flags that influences the behavior of this function
 * @return Error code
 **/

error_t httpClientReadBody(HttpClientContext *context, void *data,
   size_t size, size_t *received, uint_t flags)
{
   error_t error;
   size_t n;
   char_t *p;

   //Check parameters
   if(context == NULL || data == NULL || received == NULL)
      return ERROR_INVALID_PARAMETER;

   //Check HTTP connection state
   if(context->state != HTTP_CLIENT_STATE_CONNECTED)
      return ERROR_WRONG_STATE;

   //Initialize status code
   error = NO_ERROR;

   //Point to the output buffer
   p = data;
   //No data has been read yet
   *received = 0;

   //Read as much data as possible
   while(*received < size && !error)
   {
      //Check HTTP request state
      if(context->requestState == HTTP_REQ_STATE_PARSE_HEADER)
      {
         //Flush receive buffer
         context->bufferLen = 0;
         context->bufferPos = 0;

         //Any response to a HEAD request and any response with a 1xx, 204 or
         //304 status code is always terminated by the first empty line after
         //the header fields, regardless of the header fields present in the
         //message, and thus cannot contain a message body
         if(!osStrcasecmp(context->method, "HEAD") ||
            HTTP_STATUS_CODE_1YZ(context->statusCode) ||
            context->statusCode == 204 ||
            context->statusCode == 304)
         {
            //The HTTP response does not contain a body
            httpClientChangeRequestState(context, HTTP_REQ_STATE_COMPLETE);
         }
         else
         {
            //The HTTP response contains a body
            httpClientChangeRequestState(context, HTTP_REQ_STATE_RECEIVE_BODY);
         }
      }
      else if(context->requestState == HTTP_REQ_STATE_RECEIVE_BODY)
      {
         //Chunked transfer encoding?
         if(context->chunkedEncoding)
         {
            //The chunked encoding modifies the body in order to transfer it
            //as a series of chunks, each with its own size indicator
            httpClientChangeRequestState(context, HTTP_REQ_STATE_RECEIVE_CHUNK_SIZE);
         }
         else
         {
            //The length of the body is determined by the Content-Length field
            if(context->bodyPos < context->bodyLen)
            {
               //Limit the number of bytes to read
               n = MIN(size - *received, context->bodyLen - context->bodyPos);

               //Read response body
               error = httpClientReceiveData(context, p, n, &n, flags);

               //Check status code
               if(!error)
               {
                  //Advance data pointer
                  p += n;
                  *received += n;
                  context->bodyPos += n;

                  //Save current time
                  context->timestamp = osGetSystemTime();

                  //We are done
                  break;
               }
               else
               {
                  //In practice, many widely deployed HTTPS servers close connections
                  //abruptly, without any prior notice, and in particular without
                  //sending the close_notify alert
                  if(!context->keepAlive && context->bodyLen == UINT_MAX)
                  {
                     //The HTTP transaction is complete
                     httpClientChangeRequestState(context, HTTP_REQ_STATE_COMPLETE);
                     //Close the HTTP connection
                     httpClientChangeState(context, HTTP_CLIENT_STATE_DISCONNECTING);
                     //The end of the response body has been reached
                     error = ERROR_END_OF_STREAM;
                  }
               }
            }
            else
            {
               //The HTTP transaction is complete
               httpClientChangeRequestState(context, HTTP_REQ_STATE_COMPLETE);
               //The end of the response body has been reached
               error = ERROR_END_OF_STREAM;
            }
         }
      }
      else if(context->requestState == HTTP_REQ_STATE_RECEIVE_CHUNK_SIZE)
      {
         //The chunked encoding modifies the body in order to transfer it as
         //a series of chunks, each with its own size indicator
         if(context->bufferLen >= 3 &&
            context->buffer[context->bufferLen - 2] == '\r' &&
            context->buffer[context->bufferLen - 1] == '\n')
         {
            //Parse chunk-size field
            error = httpClientParseChunkSize(context, context->buffer,
               context->bufferLen);
         }
         else if(context->bufferLen < HTTP_CLIENT_BUFFER_SIZE)
         {
            //Receive more data
            error = httpClientReceiveData(context, context->buffer +
               context->bufferLen, HTTP_CLIENT_BUFFER_SIZE - context->bufferLen,
               &n, HTTP_FLAG_BREAK_CRLF);

            //Check status code
            if(!error)
            {
               //Adjust the length of the buffer
               context->bufferLen += n;
               //Save current time
               context->timestamp = osGetSystemTime();
            }
         }
         else
         {
            //The client implementation limits the length of the chunk-size field
            error = ERROR_BUFFER_OVERFLOW;
         }
      }
      else if(context->requestState == HTTP_REQ_STATE_RECEIVE_CHUNK_DATA)
      {
         //The data stream is divided into a series of chunks
         if(context->bodyPos < context->bodyLen)
         {
            //The length of the data chunk is determined by the chunk-size field
            n = MIN(size - *received, context->bodyLen - context->bodyPos);

            //Read chunk data
            error = httpClientReceiveData(context, p, n, &n, flags);

            //Check status code
            if(!error)
            {
               //Total number of data that have been read
               *received += n;
               //Number of bytes left to process in the current chunk
               context->bodyPos += n;

               //Save current time
               context->timestamp = osGetSystemTime();

               //Check flags
               if((flags & HTTP_FLAG_BREAK_CRLF) != 0)
               {
                  //The HTTP_FLAG_BREAK_CHAR flag causes the function to stop
                  //reading data as soon as the specified break character is
                  //encountered
                  if(p[n - 1] == LSB(flags))
                     break;
               }
               else if((flags & HTTP_FLAG_WAIT_ALL) == 0)
               {
                  //The HTTP_FLAG_WAIT_ALL flag causes the function to return
                  //only when the requested number of bytes have been read
                  break;
               }
               else
               {
                  //Just for sanity
               }

               //Advance data pointer
               p += n;
            }
         }
         else
         {
            //The chunked encoding modifies the body in order to transfer it
            //as a series of chunks, each with its own size indicator
            httpClientChangeRequestState(context, HTTP_REQ_STATE_RECEIVE_CHUNK_SIZE);
         }
      }
      else if(context->requestState == HTTP_REQ_STATE_RECEIVE_TRAILER ||
         context->requestState == HTTP_REQ_STATE_PARSE_TRAILER ||
         context->requestState == HTTP_REQ_STATE_COMPLETE)
      {
         //The user must be satisfied with data already on hand
         if(*received > 0)
         {
            //Some data are pending in the receive buffer
            break;
         }
         else
         {
            //The end of the response body has been reached
            error = ERROR_END_OF_STREAM;
         }
      }
      else
      {
         //Invalid state
         error = ERROR_WRONG_STATE;
      }
   }

   //Check status code
   if(error == ERROR_WOULD_BLOCK || error == ERROR_TIMEOUT)
   {
      //Check whether the timeout has elapsed
      error = httpClientCheckTimeout(context);
   }

   //Return status code
   return error;
}


/**
 * @brief Read HTTP trailer
 * @param[in] context Pointer to the HTTP client context
 * @return Error code
 **/

error_t httpClientReadTrailer(HttpClientContext *context)
{
   error_t error;
   size_t n;

   //Make sure the HTTP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Send HTTP request header
   while(!error)
   {
      //Check HTTP connection state
      if(context->state == HTTP_CLIENT_STATE_CONNECTED)
      {
         //Check HTTP request state
         if(context->requestState == HTTP_REQ_STATE_RECEIVE_TRAILER)
         {
            //The CRLF sequence is always used to terminate a line
            if(context->bufferLen >= (context->bufferPos + 2) &&
               context->buffer[context->bufferLen - 2] == '\r' &&
               context->buffer[context->bufferLen - 1] == '\n')
            {
               //Strip the CRLF at the end of the line
               context->bufferLen -= 2;

               //An empty line indicates the end of the header fields
               if(context->bufferLen == context->bufferPos)
               {
                  //The HTTP transaction is complete
                  httpClientChangeRequestState(context, HTTP_REQ_STATE_PARSE_TRAILER);
               }
               else
               {
                  //The response header fields allow the server to pass additional
                  //information about the response which cannot be placed in the
                  //Status-Line
                  error = httpClientParseHeaderField(context, context->buffer +
                     context->bufferPos, context->bufferLen - context->bufferPos);
               }
            }
            else if(context->bufferLen < HTTP_CLIENT_BUFFER_SIZE)
            {
               //Receive more data
               error = httpClientReceiveData(context, context->buffer +
                  context->bufferLen, HTTP_CLIENT_BUFFER_SIZE - context->bufferLen,
                  &n, HTTP_FLAG_BREAK_CRLF);

               //Check status code
               if(!error)
               {
                  //Adjust the length of the buffer
                  context->bufferLen += n;
                  //Save current time
                  context->timestamp = osGetSystemTime();
               }
            }
            else
            {
               //The client implementation limits the size of headers it accepts
               error = ERROR_BUFFER_OVERFLOW;
            }
         }
         else if(context->requestState == HTTP_REQ_STATE_PARSE_TRAILER ||
            context->requestState == HTTP_REQ_STATE_COMPLETE)
         {
            //Rewind to the beginning of the buffer
            context->bufferPos = 0;

            //Persistent HTTP connection?
            if(context->keepAlive)
            {
               //Persistent connections stay open across transactions, until
               //either the client or the server decides to close them
               break;
            }
            else
            {
               //The connection will be closed after completion of the response
               httpClientChangeState(context, HTTP_CLIENT_STATE_DISCONNECTING);
            }
         }
         else
         {
            //Invalid HTTP request state
            error = ERROR_WRONG_STATE;
         }
      }
      else if(context->state == HTTP_CLIENT_STATE_DISCONNECTING)
      {
         //Shutdown connection
         error = httpClientDisconnect(context);
      }
      else if(context->state == HTTP_CLIENT_STATE_DISCONNECTED)
      {
         //Rewind to the beginning of the buffer
         context->bufferPos = 0;
         //We are done
         break;
      }
      else
      {
         //Invalid HTTP connection state
         error = ERROR_WRONG_STATE;
      }
   }

   //Check status code
   if(error == ERROR_WOULD_BLOCK || error == ERROR_TIMEOUT)
   {
      //Check whether the timeout has elapsed
      error = httpClientCheckTimeout(context);
   }

   //Return status code
   return error;
}


/**
 * @brief Close HTTP request or response body
 * @param[in] context Pointer to the HTTP client context
 * @return Error code
 **/

error_t httpClientCloseBody(HttpClientContext *context)
{
   error_t error;
   size_t n;

   //Make sure the HTTP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Close HTTP request or response body
   while(!error)
   {
      //Check HTTP connection state
      if(context->state == HTTP_CLIENT_STATE_CONNECTED)
      {
         //Check HTTP request state
         if(context->requestState == HTTP_REQ_STATE_SEND_BODY)
         {
            //Chunked transfer encoding?
            if(context->chunkedEncoding)
            {
               //The chunked encoding is ended by any chunk whose size is zero
               error = httpClientFormatChunkSize(context, 0);
            }
            else
            {
               //Ensure the HTTP request body is complete
               if(context->bodyPos == context->bodyLen)
               {
                  //Flush receive buffer
                  context->bufferLen = 0;
                  context->bufferPos = 0;

                  //Receive the HTTP response header
                  httpClientChangeRequestState(context,
                     HTTP_REQ_STATE_RECEIVE_STATUS_LINE);
               }
               else
               {
                  //Incomplete request body
                  error = ERROR_WRONG_STATE;
               }
            }
         }
         else if(context->requestState == HTTP_REQ_STATE_SEND_CHUNK_DATA)
         {
            //Ensure the chunk data is complete
            if(context->bodyPos == context->bodyLen)
            {
               //The chunked encoding is ended by any chunk whose size is zero
               error = httpClientFormatChunkSize(context, 0);
            }
            else
            {
               //Incomplete chunk data
               error = ERROR_WRONG_STATE;
            }
         }
         else if(context->requestState == HTTP_REQ_STATE_FORMAT_TRAILER ||
            context->requestState == HTTP_REQ_STATE_RECEIVE_STATUS_LINE)
         {
            //The HTTP request body is closed
            break;
         }
         else if(context->requestState == HTTP_REQ_STATE_PARSE_HEADER ||
            context->requestState == HTTP_REQ_STATE_RECEIVE_BODY ||
            context->requestState == HTTP_REQ_STATE_RECEIVE_CHUNK_SIZE ||
            context->requestState == HTTP_REQ_STATE_RECEIVE_CHUNK_DATA)
         {
            //Consume HTTP response body
            error = httpClientReadBody(context, context->buffer,
               HTTP_CLIENT_BUFFER_SIZE, &n, 0);

            //Check whether the end of the response body has been reached
            if(error == ERROR_END_OF_STREAM)
            {
               //Continue reading the optional trailer
               error = NO_ERROR;
            }
         }
         else if(context->requestState == HTTP_REQ_STATE_RECEIVE_TRAILER)
         {
            //Consume HTTP trailer
            error = httpClientReadTrailer(context);
         }
         else if(context->requestState == HTTP_REQ_STATE_PARSE_TRAILER ||
            context->requestState == HTTP_REQ_STATE_COMPLETE)
         {
            //The HTTP response body is closed
            break;
         }
         else
         {
            //Invalid HTTP request state
            error = ERROR_WRONG_STATE;
         }
      }
      else if(context->state == HTTP_CLIENT_STATE_DISCONNECTING)
      {
         //Shutdown connection
         error = httpClientDisconnect(context);
      }
      else if(context->state == HTTP_CLIENT_STATE_DISCONNECTED)
      {
         //Rewind to the beginning of the buffer
         context->bufferPos = 0;
         //We are done
         break;
      }
      else
      {
         //Invalid HTTP connection state
         error = ERROR_WRONG_STATE;
      }
   }

   //Check status code
   if(error == ERROR_WOULD_BLOCK || error == ERROR_TIMEOUT)
   {
      //Check whether the timeout has elapsed
      error = httpClientCheckTimeout(context);
   }

   //Return status code
   return error;
}


/**
 * @brief Gracefully disconnect from the HTTP server
 * @param[in] context Pointer to the HTTP client context
 * @return Error code
 **/

error_t httpClientDisconnect(HttpClientContext *context)
{
   error_t error;

   //Make sure the HTTP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Execute HTTP command sequence
   while(!error)
   {
      //Check HTTP connection state
      if(context->state == HTTP_CLIENT_STATE_CONNECTED)
      {
         //Gracefully disconnect from the HTTP server
         httpClientChangeState(context, HTTP_CLIENT_STATE_DISCONNECTING);
      }
      else if(context->state == HTTP_CLIENT_STATE_DISCONNECTING)
      {
         //Shutdown connection
         error = httpClientShutdownConnection(context);

         //Check status code
         if(error == NO_ERROR)
         {
            //Close connection
            httpClientCloseConnection(context);
            //Update HTTP connection state
            httpClientChangeState(context, HTTP_CLIENT_STATE_DISCONNECTED);
         }
         else if(error == ERROR_WOULD_BLOCK || error == ERROR_TIMEOUT)
         {
            //Check whether the timeout has elapsed
            error = httpClientCheckTimeout(context);
         }
         else
         {
            //A communication error has occurred
         }
      }
      else if(context->state == HTTP_CLIENT_STATE_DISCONNECTED)
      {
         //We are done
         break;
      }
      else
      {
         //Invalid state
         error = ERROR_WRONG_STATE;
      }
   }

   //Failed to gracefully disconnect from the HTTP server?
   if(error != NO_ERROR && error != ERROR_WOULD_BLOCK)
   {
      //Close connection
      httpClientCloseConnection(context);
      //Update HTTP connection state
      httpClientChangeState(context, HTTP_CLIENT_STATE_DISCONNECTED);
   }

   //Return status code
   return error;
}


/**
 * @brief Close the connection with the HTTP server
 * @param[in] context Pointer to the HTTP client context
 * @return Error code
 **/

error_t httpClientClose(HttpClientContext *context)
{
   //Make sure the HTTP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Close connection
   httpClientCloseConnection(context);
   //Update HTTP connection state
   httpClientChangeState(context, HTTP_CLIENT_STATE_DISCONNECTED);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Release HTTP client context
 * @param[in] context Pointer to the HTTP client context
 **/

void httpClientDeinit(HttpClientContext *context)
{
   //Make sure the HTTP client context is valid
   if(context != NULL)
   {
      //Close connection
      httpClientCloseConnection(context);

#if (HTTP_CLIENT_TLS_SUPPORT == ENABLED)
      //Release TLS session state
      tlsFreeSessionState(&context->tlsSession);
#endif

      //Clear HTTP client context
      osMemset(context, 0, sizeof(HttpClientContext));
   }
}

#endif
