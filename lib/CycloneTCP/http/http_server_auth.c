/**
 * @file http_server_auth.c
 * @brief HTTP authentication
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
#include "net.h"
#include "http/http_server.h"
#include "http/http_server_auth.h"
#include "http/http_server_misc.h"
#include "str.h"
#include "cyclone_debug.h"

//Check TCP/IP stack configuration
#if (HTTP_SERVER_SUPPORT == ENABLED)


/**
 * @brief Password verification
 * @param[in] connection Structure representing an HTTP connection
 * @param[in] password NULL-terminated string containing the password to be checked
 * @param[in] mode HTTP authentication scheme to be used. Acceptable
 *   values are HTTP_AUTH_MODE_BASIC or HTTP_AUTH_MODE_DIGEST
 * @return TRUE if the password is valid, else FALSE
 **/

bool_t httpCheckPassword(HttpConnection *connection,
   const char_t *password, HttpAuthMode mode)
{
   //This flag tells whether the password is valid
   bool_t status = FALSE;

   //Debug message
   TRACE_DEBUG("HTTP password verification...\r\n");

#if (HTTP_SERVER_BASIC_AUTH_SUPPORT == ENABLED)
   //Basic authentication scheme?
   if(mode == HTTP_AUTH_MODE_BASIC)
   {
      //Point to the authentication credentials
      HttpAuthorizationHeader *auth = &connection->request.auth;

      //Make sure authentication credentials have been found
      if(auth->found && auth->mode == HTTP_AUTH_MODE_BASIC)
      {
         //Sanity check
         if(auth->password != NULL)
         {
            //Check whether the password is valid
            if(!osStrcmp(password, auth->password))
               status = TRUE;
         }
      }
   }
#endif
#if (HTTP_SERVER_DIGEST_AUTH_SUPPORT == ENABLED)
   //Digest authentication scheme?
   if(mode == HTTP_AUTH_MODE_DIGEST)
   {
      //Point to the authentication credentials
      HttpAuthorizationHeader *auth = &connection->request.auth;

      //Make sure authentication credentials have been found
      if(auth->found && auth->mode == HTTP_AUTH_MODE_DIGEST)
      {
         //Sanity check
         if(auth->realm != NULL && auth->nonce != NULL &&
            auth->uri != NULL && auth->qop != NULL &&
            auth->nc != NULL && auth->cnonce != NULL &&
            auth->response != NULL)
         {
            error_t error;
            Md5Context *md5Context;
            char_t ha1[2 * MD5_DIGEST_SIZE + 1];
            char_t ha2[2 * MD5_DIGEST_SIZE + 1];

            //Allocate a memory buffer to hold the MD5 context
            md5Context = osAllocMem(sizeof(Md5Context));

            //MD5 context successfully allocated?
            if(md5Context != NULL)
            {
               //Compute HA1 = MD5(username : realm : password)
               md5Init(md5Context);
               md5Update(md5Context, auth->user, osStrlen(auth->user));
               md5Update(md5Context, ":", 1);
               md5Update(md5Context, auth->realm, osStrlen(auth->realm));
               md5Update(md5Context, ":", 1);
               md5Update(md5Context, password, osStrlen(password));
               md5Final(md5Context, NULL);

               //Convert MD5 hash to hex string
               httpConvertArrayToHexString(md5Context->digest, MD5_DIGEST_SIZE, ha1);
               //Debug message
               TRACE_DEBUG("  HA1: %s\r\n", ha1);

               //Compute HA2 = MD5(method : uri)
               md5Init(md5Context);
               md5Update(md5Context, connection->request.method, osStrlen(connection->request.method));
               md5Update(md5Context, ":", 1);
               md5Update(md5Context, auth->uri, osStrlen(auth->uri));
               md5Final(md5Context, NULL);

               //Convert MD5 hash to hex string
               httpConvertArrayToHexString(md5Context->digest, MD5_DIGEST_SIZE, ha2);
               //Debug message
               TRACE_DEBUG("  HA2: %s\r\n", ha2);

               //Compute MD5(HA1 : nonce : nc : cnonce : qop : HA2)
               md5Init(md5Context);
               md5Update(md5Context, ha1, osStrlen(ha1));
               md5Update(md5Context, ":", 1);
               md5Update(md5Context, auth->nonce, osStrlen(auth->nonce));
               md5Update(md5Context, ":", 1);
               md5Update(md5Context, auth->nc, osStrlen(auth->nc));
               md5Update(md5Context, ":", 1);
               md5Update(md5Context, auth->cnonce, osStrlen(auth->cnonce));
               md5Update(md5Context, ":", 1);
               md5Update(md5Context, auth->qop, osStrlen(auth->qop));
               md5Update(md5Context, ":", 1);
               md5Update(md5Context, ha2, osStrlen(ha2));
               md5Final(md5Context, NULL);

               //Convert MD5 hash to hex string
               httpConvertArrayToHexString(md5Context->digest, MD5_DIGEST_SIZE, ha1);
               //Debug message
               TRACE_DEBUG("  response: %s\r\n", ha1);

               //Release MD5 context
               osFreeMem(md5Context);

               //Check response
               if(!osStrcasecmp(auth->response, ha1))
               {
                  //Perform nonce verification
                  error = httpVerifyNonce(connection->serverContext, auth->nonce, auth->nc);

                  //Valid nonce?
                  if(!error)
                  {
                     //Access to the resource is granted
                     status = TRUE;
                  }
                  else
                  {
                     //The client may wish to simply retry the request with a
                     //new encrypted response, without re-prompting the user
                     //for a new username and password
                     connection->response.auth.stale = TRUE;
                  }
               }
            }
         }
      }
   }
#endif

   //Return TRUE is the password is valid, else FALSE
   return status;
}


/**
 * @brief Parse Authorization header field
 * @param[in] connection Structure representing an HTTP connection
 * @param[in] value Authorization field value
 **/

void httpParseAuthorizationField(HttpConnection *connection, char_t *value)
{
   char_t *p;
   char_t *token;

   //Retrieve the authentication scheme
   token = osStrtok_r(value, " \t", &p);

   //Any parsing error?
   if(token == NULL)
   {
      //Exit immediately
      return;
   }
#if (HTTP_SERVER_BASIC_AUTH_SUPPORT == ENABLED)
   //Basic access authentication?
   else if(!osStrcasecmp(token, "Basic"))
   {
      error_t error;
      size_t n;
      char_t *separator;

      //Use the relevant authentication scheme
      connection->request.auth.mode = HTTP_AUTH_MODE_BASIC;
      //Retrieve the credentials
      token = osStrtok_r(NULL, " \t", &p);

      //Any parsing error?
      if(token != NULL)
      {
         //Decrypt the Base64-encoded string
         error = base64Decode(token, osStrlen(token), token, &n);

         //Successful decoding?
         if(!error)
         {
            //Properly terminate the string
            token[n] = '\0';
            //Check whether a separator is present
            separator = osStrchr(token, ':');

            //Separator found?
            if(separator != NULL)
            {
               //Split the line
               *separator = '\0';

               //Save user name
               strSafeCopy(connection->request.auth.user,
                  token, HTTP_SERVER_USERNAME_MAX_LEN);

               //Point to the password
               token = separator + 1;
               //Save password
               connection->request.auth.password = token;
            }
         }
      }

      //Debug message
      TRACE_DEBUG("Authorization header:\r\n");
      TRACE_DEBUG("  username: %s\r\n", connection->request.auth.user);
      TRACE_DEBUG("  password: %s\r\n", connection->request.auth.password);
   }
#endif
#if (HTTP_SERVER_DIGEST_AUTH_SUPPORT == ENABLED)
   //Digest access authentication?
   else if(!osStrcasecmp(token, "Digest"))
   {
      size_t n;
      char_t *separator;
      char_t *name;

      //Use the relevant authentication scheme
      connection->request.auth.mode = HTTP_AUTH_MODE_DIGEST;
      //Get the first parameter
      token = osStrtok_r(NULL, ",", &p);

      //Parse the Authorization header field
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
            if(!osStrcasecmp(name, "username"))
            {
               //Save user name
               strSafeCopy(connection->request.auth.user,
                  value, HTTP_SERVER_USERNAME_MAX_LEN);
            }
            else if(!osStrcasecmp(name, "realm"))
            {
               //Save realm
               connection->request.auth.realm = value;
            }
            else if(!osStrcasecmp(name, "nonce"))
            {
               //Save nonce parameter
               connection->request.auth.nonce = value;
            }
            else if(!osStrcasecmp(name, "uri"))
            {
               //Save uri parameter
               connection->request.auth.uri = value;
            }
            else if(!osStrcasecmp(name, "qop"))
            {
               //Save qop parameter
               connection->request.auth.qop = value;
            }
            else if(!osStrcasecmp(name, "nc"))
            {
               //Save nc parameter
               connection->request.auth.nc = value;
            }
            else if(!osStrcasecmp(name, "cnonce"))
            {
               //Save cnonce parameter
               connection->request.auth.cnonce = value;
            }
            else if(!osStrcasecmp(name, "response"))
            {
               //Save response parameter
               connection->request.auth.response = value;
            }
            else if(!osStrcasecmp(name, "opaque"))
            {
               //Save opaque parameter
               connection->request.auth.opaque = value;
            }

            //Get next parameter
            token = osStrtok_r(NULL, ",", &p);
         }
      }

      //Debug message
      TRACE_DEBUG("Authorization header:\r\n");
      TRACE_DEBUG("  username: %s\r\n", connection->request.auth.user);
      TRACE_DEBUG("  realm: %s\r\n", connection->request.auth.realm);
      TRACE_DEBUG("  nonce: %s\r\n", connection->request.auth.nonce);
      TRACE_DEBUG("  uri: %s\r\n", connection->request.auth.uri);
      TRACE_DEBUG("  qop: %s\r\n", connection->request.auth.qop);
      TRACE_DEBUG("  nc: %s\r\n", connection->request.auth.nc);
      TRACE_DEBUG("  cnonce: %s\r\n", connection->request.auth.cnonce);
      TRACE_DEBUG("  response: %s\r\n", connection->request.auth.response);
      TRACE_DEBUG("  opaque: %s\r\n", connection->request.auth.opaque);
   }
#endif
   else
   {
      //The specified authentication scheme is not supported
      return;
   }

#if (HTTP_SERVER_BASIC_AUTH_SUPPORT == ENABLED || HTTP_SERVER_DIGEST_AUTH_SUPPORT == ENABLED)
   //The Authorization header has been found
   connection->request.auth.found = TRUE;

   //Invoke user-defined callback, if any
   if(connection->settings->authCallback != NULL)
   {
      //Check whether the access to the specified URI is authorized
      connection->status = connection->settings->authCallback(connection,
         connection->request.auth.user, connection->request.uri);
   }
   else
   {
      //Access to the specified URI is allowed
      connection->status = HTTP_ACCESS_ALLOWED;
   }
#endif
}


/**
 * @brief Format WWW-Authenticate header field
 * @param[in] connection Structure representing an HTTP connection
 * @param[out] output Buffer where to format the header field
 * @return Total length of the header field
 **/

size_t httpAddAuthenticateField(HttpConnection *connection, char_t *output)
{
   size_t n;

#if (HTTP_SERVER_BASIC_AUTH_SUPPORT == ENABLED)
   //Basic authentication scheme?
   if(connection->response.auth.mode == HTTP_AUTH_MODE_BASIC)
   {
      //Set WWW-Authenticate field
      n = osSprintf(output, "WWW-Authenticate: Basic realm=\"Protected Area\"\r\n");
   }
   else
#endif
#if (HTTP_SERVER_DIGEST_AUTH_SUPPORT == ENABLED)
   //Digest authentication scheme?
   if(connection->response.auth.mode == HTTP_AUTH_MODE_DIGEST)
   {
      error_t error;
      size_t k;
      uint8_t opaque[16];

      //Set WWW-Authenticate field
      n = osSprintf(output, "WWW-Authenticate: Digest ");
      n += osSprintf(output + n, "realm=\"Protected Area\", ");
      n += osSprintf(output + n, "qop=\"auth\", ");
      n += osSprintf(output + n, "nonce=\"");

      //The nonce is a server-specified data string which should be uniquely
      //generated each time a 401 response is made
      error = httpGenerateNonce(connection->serverContext, output + n, &k);
      //Any error to report?
      if(error)
         return error;

      //Advance pointer
      n += k;
      //Properly terminate the nonce string
      n += osSprintf(output + n, "\", ");

      //Format opaque parameter
      n += osSprintf(output + n, "opaque=\"");

      //Generate a random value
      if(connection->settings->randCallback != NULL)
         error = connection->settings->randCallback(opaque, 16);
      else
         error = ERROR_FAILURE;

      //Random number generation failed?
      if(error)
         return error;

      //Convert the byte array to hex string
      httpConvertArrayToHexString(opaque, 16, output + n);

      //Advance pointer
      n += 32;
      //Properly terminate the opaque string
      n += osSprintf(output + n, "\"");

      //The STALE flag indicates that the previous request from the client
      //was rejected because the nonce value was stale
      if(connection->response.auth.stale)
      {
         n += osSprintf(output + n, ", stale=TRUE");
      }

      //Properly terminate the WWW-Authenticate field
      n += osSprintf(output + n, "\r\n");
   }
   else
#endif
   //Unknown authentication scheme?
   {
      //No need to add the WWW-Authenticate header field
      n = 0;
   }

   //Return the total length of the WWW-Authenticate header field
   return n;
}


/**
 * @brief Nonce generation
 * @param[in] context Pointer to the HTTP server context
 * @param[in] output NULL-terminated string containing the nonce
 * @param[in] length NULL-terminated string containing the nonce count
 * @return Error code
 **/

error_t httpGenerateNonce(HttpServerContext *context,
   char_t *output, size_t *length)
{
#if (HTTP_SERVER_DIGEST_AUTH_SUPPORT == ENABLED)
   error_t error;
   uint_t i;
   systime_t time;
   HttpNonceCacheEntry *entry;
   HttpNonceCacheEntry *oldestEntry;
   uint8_t nonce[HTTP_SERVER_NONCE_SIZE];

   //Acquire exclusive access to the nonce cache
   osAcquireMutex(&context->nonceCacheMutex);

   //Get current time
   time = osGetSystemTime();

   //Keep track of the oldest entry
   oldestEntry = &context->nonceCache[0];

   //Loop through nonce cache entries
   for(i = 0; i < HTTP_SERVER_NONCE_CACHE_SIZE; i++)
   {
      //Point to the current entry
      entry = &context->nonceCache[i];

      //Check whether the entry is currently in use or not
      if(!entry->count)
         break;

      //Keep track of the oldest entry in the table
      if((time - entry->timestamp) > (time - oldestEntry->timestamp))
      {
         oldestEntry = entry;
      }
   }

   //The oldest entry is removed whenever the table runs out of space
   if(i >= HTTP_SERVER_NONCE_CACHE_SIZE)
      entry = oldestEntry;

   //Generate a new nonce
   if(context->settings.randCallback != NULL)
      error = context->settings.randCallback(nonce, HTTP_SERVER_NONCE_SIZE);
   else
      error = ERROR_FAILURE;

   //Check status code
   if(!error)
   {
      //Convert the byte array to hex string
      httpConvertArrayToHexString(nonce, HTTP_SERVER_NONCE_SIZE, entry->nonce);
      //Clear nonce count
      entry->count = 1;
      //Save the time at which the nonce was generated
      entry->timestamp = osGetSystemTime();

      //Copy the nonce to the output buffer
      osStrcpy(output, entry->nonce);
      //Return the length of the nonce excluding the NULL character
      *length = HTTP_SERVER_NONCE_SIZE * 2;
   }
   else
   {
      //Random number generation failed
      osMemset(entry, 0, sizeof(HttpNonceCacheEntry));
   }

   //Release exclusive access to the nonce cache
   osReleaseMutex(&context->nonceCacheMutex);
   //Return status code
   return error;

#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Nonce verification
 * @param[in] context Pointer to the HTTP server context
 * @param[in] nonce NULL-terminated string containing the nonce
 * @param[in] nc NULL-terminated string containing the nonce count
 * @return Error code
 **/

error_t httpVerifyNonce(HttpServerContext *context,
   const char_t *nonce, const char_t *nc)
{
#if (HTTP_SERVER_DIGEST_AUTH_SUPPORT == ENABLED)
   error_t error;
   uint_t i;
   uint32_t count;
   systime_t time;
   HttpNonceCacheEntry *entry;

   //Check parameters
   if(nonce == NULL || nc == NULL)
      return ERROR_INVALID_PARAMETER;

   //Convert the nonce count to integer
   count = osStrtoul(nc, NULL, 16);
   //Get current time
   time = osGetSystemTime();

   //Acquire exclusive access to the nonce cache
   osAcquireMutex(&context->nonceCacheMutex);

   //Loop through nonce cache entries
   for(i = 0; i < HTTP_SERVER_NONCE_CACHE_SIZE; i++)
   {
      //Point to the current entry
      entry = &context->nonceCache[i];

      //Check nonce value
      if(!osStrcasecmp(entry->nonce, nonce))
      {
         //Make sure the nonce timestamp has not expired
         if((time - entry->timestamp) < HTTP_SERVER_NONCE_LIFETIME)
         {
            //Check nonce count to prevent replay attacks
            if(count >= entry->count)
            {
               //Update nonce count to the next expected value
               entry->count = count + 1;
               //We are done
               break;
            }
         }
      }
   }

   //Check whether the nonce is valid
   if(i < HTTP_SERVER_NONCE_CACHE_SIZE)
      error = NO_ERROR;
   else
      error = ERROR_NOT_FOUND;

   //Release exclusive access to the nonce cache
   osReleaseMutex(&context->nonceCacheMutex);
   //Return status code
   return error;

#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}

#endif
