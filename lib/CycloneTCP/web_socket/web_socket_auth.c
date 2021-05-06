/**
 * @file web_socket_auth.c
 * @brief HTTP authentication for WebSockets
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
#include "encoding/base64.h"
#include "hash/md5.h"
#include "str.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (WEB_SOCKET_SUPPORT == ENABLED)


/**
 * @brief Parse WWW-Authenticate header field
 * @param[in] webSocket Handle to a WebSocket
 * @param[in] value NULL-terminated string that contains the value of header field
 * @return Error code
 **/

error_t webSocketParseAuthenticateField(WebSocket *webSocket, char_t *value)
{
#if (WEB_SOCKET_BASIC_AUTH_SUPPORT == ENABLED || WEB_SOCKET_DIGEST_AUTH_SUPPORT == ENABLED)
   size_t n;
   char_t *p;
   char_t *token;
   char_t *separator;
   char_t *name;
   WebSocketAuthContext *authContext;

   //Point to the handshake context
   authContext = &webSocket->authContext;

   //Retrieve the authentication scheme
   token = osStrtok_r(value, " \t", &p);

   //Any parsing error?
   if(token == NULL)
      return ERROR_INVALID_SYNTAX;

   //Basic access authentication?
   if(!osStrcasecmp(token, "Basic"))
   {
      //Basic authentication is required by the WebSocket server
      authContext->requiredAuthMode = WS_AUTH_MODE_BASIC;
   }
   //Digest access authentication?
   else if(!osStrcasecmp(token, "Digest"))
   {
      //Digest authentication is required by the WebSocket server
      authContext->requiredAuthMode = WS_AUTH_MODE_DIGEST;
   }
   //Unknown authentication scheme?
   else
   {
      //Report an error
      return ERROR_INVALID_SYNTAX;
   }

   //Get the first parameter
   token = osStrtok_r(NULL, ",", &p);

   //Parse the WWW-Authenticate field
   while(token != NULL)
   {
      //Check whether a separator is present
      separator = osStrchr(token, '=');

      //Separator found?
      if(separator != NULL)
      {
         //Split the string
         *separator = '\0';

         //Get field name and value
         name = strTrimWhitespace(token);
         value = strTrimWhitespace(separator + 1);

         //Retrieve the length of the value field
         n = osStrlen(value);

         //Discard the surrounding quotes
         if(n > 0 && value[n - 1] == '\"')
            value[n - 1] = '\0';
         if(value[0] == '\"')
            value++;

         //Check parameter name
         if(!osStrcasecmp(name, "realm"))
         {
            //Save realm
            strSafeCopy(authContext->realm, value, WEB_SOCKET_REALM_MAX_LEN);
         }
#if (WEB_SOCKET_DIGEST_AUTH_SUPPORT == ENABLED)
         else if(!osStrcasecmp(name, "nonce"))
         {
            //Save nonce
            strSafeCopy(authContext->nonce, value, WEB_SOCKET_NONCE_MAX_LEN + 1);
         }
         else if(!osStrcasecmp(name, "opaque"))
         {
            //Save nonce
            strSafeCopy(authContext->opaque, value, WEB_SOCKET_OPAQUE_MAX_LEN + 1);
         }
         else if(!osStrcasecmp(name, "stale"))
         {
            //Save stale flag
            if(!osStrcasecmp(value, "true"))
               authContext->stale = TRUE;
            else
               authContext->stale = FALSE;
         }
#endif

         //Get next parameter
         token = osStrtok_r(NULL, ",", &p);
      }
   }
#endif

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Format Authorization header field
 * @param[in] webSocket Handle to a WebSocket
 * @param[out] output Buffer where to format the header field
 * @return Total length of the header field
 **/

size_t webSocketAddAuthorizationField(WebSocket *webSocket, char_t *output)
{
   size_t n;

#if (WEB_SOCKET_BASIC_AUTH_SUPPORT == ENABLED || WEB_SOCKET_DIGEST_AUTH_SUPPORT == ENABLED)
   WebSocketAuthContext *authContext;

   //Point to the handshake context
   authContext = &webSocket->authContext;
#endif

#if (WEB_SOCKET_BASIC_AUTH_SUPPORT == ENABLED)
   //Basic authentication scheme?
   if(authContext->selectedAuthMode == WS_AUTH_MODE_BASIC)
   {
      size_t k;
      char_t *temp;

      //Temporary buffer
      temp = (char_t *) webSocket->rxContext.buffer;

      //Format Authorization header field
      n = osSprintf(output, "Authorization: Basic ");

      //The client sends the userid and password, separated by a single colon
      //character, within a Base64-encoded string in the credentials
      k = osSprintf(temp, "%s:%s", authContext->username,
         authContext->password);

      //Encode the resulting string using Base64
      base64Encode(temp, k, output + n, &k);
      //Update the total length of the header field
      n += k;

      //Properly terminate the Authorization header field
      n += osSprintf(output + n, "\r\n");
   }
   else
#endif
#if (WEB_SOCKET_DIGEST_AUTH_SUPPORT == ENABLED)
   //Digest authentication scheme?
   if(authContext->selectedAuthMode == WS_AUTH_MODE_DIGEST)
   {
      Md5Context md5Context;
      char_t ha1[2 * MD5_DIGEST_SIZE + 1];
      char_t ha2[2 * MD5_DIGEST_SIZE + 1];
      char_t nc[9];

      //Count of the number of requests (including the current request)
      //that the client has sent with the nonce value in this request
      authContext->nc++;

      //Convert the value to hex string
      osSprintf(nc, "%08x", authContext->nc);

      //Compute HA1 = MD5(username : realm : password)
      md5Init(&md5Context);
      md5Update(&md5Context, authContext->username, osStrlen(authContext->username));
      md5Update(&md5Context, ":", 1);
      md5Update(&md5Context, authContext->realm, osStrlen(authContext->realm));
      md5Update(&md5Context, ":", 1);
      md5Update(&md5Context, authContext->password, osStrlen(authContext->password));
      md5Final(&md5Context, NULL);

      //Convert MD5 hash to hex string
      webSocketConvertArrayToHexString(md5Context.digest, MD5_DIGEST_SIZE, ha1);
      //Debug message
      TRACE_DEBUG("  HA1: %s\r\n", ha1);

      //Compute HA2 = MD5(method : uri)
      md5Init(&md5Context);
      md5Update(&md5Context, "GET", 3);
      md5Update(&md5Context, ":", 1);
      md5Update(&md5Context, webSocket->uri, osStrlen(webSocket->uri));
      md5Final(&md5Context, NULL);

      //Convert MD5 hash to hex string
      webSocketConvertArrayToHexString(md5Context.digest, MD5_DIGEST_SIZE, ha2);
      //Debug message
      TRACE_DEBUG("  HA2: %s\r\n", ha2);

      //Compute MD5(HA1 : nonce : nc : cnonce : qop : HA2)
      md5Init(&md5Context);
      md5Update(&md5Context, ha1, osStrlen(ha1));
      md5Update(&md5Context, ":", 1);
      md5Update(&md5Context, authContext->nonce, osStrlen(authContext->nonce));
      md5Update(&md5Context, ":", 1);
      md5Update(&md5Context, nc, osStrlen(nc));
      md5Update(&md5Context, ":", 1);
      md5Update(&md5Context, authContext->cnonce, osStrlen(authContext->cnonce));
      md5Update(&md5Context, ":", 1);
      md5Update(&md5Context, "auth", 4);
      md5Update(&md5Context, ":", 1);
      md5Update(&md5Context, ha2, osStrlen(ha2));
      md5Final(&md5Context, NULL);

      //Convert MD5 hash to hex string
      webSocketConvertArrayToHexString(md5Context.digest, MD5_DIGEST_SIZE, ha1);
      //Debug message
      TRACE_DEBUG("  response: %s\r\n", ha1);

      //Format Authorization header field
      n = osSprintf(output, "Authorization: Digest ");

      //Username
      n += osSprintf(output + n, "username=\"%s\", ", authContext->username);
      //Realm
      n += osSprintf(output + n, "realm=\"%s\", ", authContext->realm);
      //Nonce value
      n += osSprintf(output + n, "nonce=\"%s\", ", authContext->nonce);
      //URI
      n += osSprintf(output + n, "uri=\"%s\", ", webSocket->uri);
      //Quality of protection
      n += osSprintf(output + n, "qop=\"auth\", ");
      //Nonce count
      n += osSprintf(output + n, "nc=\"%08x\", ", authContext->nc);
      //Cnonce value
      n += osSprintf(output + n, "cnonce=\"%s\", ", authContext->cnonce);
      //Response
      n += osSprintf(output + n, "response=\"%s\", ", ha1);
      //Opaque parameter
      n += osSprintf(output + n, "opaque=\"%s\"\r\n", authContext->opaque);
   }
   else
#endif
   //Unknown authentication scheme?
   {
      //No need to add the Authorization header field
      n = 0;
   }

   //Return the total length of the Authorization header field
   return n;
}


/**
 * @brief Convert byte array to hex string
 * @param[in] input Point to the byte array
 * @param[in] inputLen Length of the byte array
 * @param[out] output NULL-terminated string resulting from the conversion
 **/

void webSocketConvertArrayToHexString(const uint8_t *input,
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
