/**
 * @file http_client_auth.c
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
 * @section Description
 *
 * The HTTP authentication framework consists of Basic and Digest access
 * authentication schemes. Basic access authentication scheme is not considered
 * to be a secure method of user authentication (unless used in conjunction with
 * some external secure system such as SSL), as the user name and password are
 * passed over the network as cleartext. Digest access authentication verifies
 * that both parties know a shared secret; unlike Basic, this verification can
 * be done without sending the password in the clear. Refer to the following
 * RFCs for complete details:
 * - RFC 2617: HTTP Authentication: Basic and Digest Access Authentication
 * - RFC 7235: Hypertext Transfer Protocol (HTTP/1.1): Authentication
 * - RFC 7616: HTTP Digest Access Authentication
 * - RFC 7617: The Basic HTTP Authentication Scheme
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

//Switch to the appropriate trace level
#define TRACE_LEVEL HTTP_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "http/http_client.h"
#include "http/http_client_auth.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (HTTP_CLIENT_SUPPORT == ENABLED && HTTP_CLIENT_AUTH_SUPPORT == ENABLED)


/**
 * @brief Initialize HTTP authentication parameters
 * @param[in] authParams HTTP authentication parameters
 **/

void httpClientInitAuthParams(HttpClientAuthParams *authParams)
{
   //Reset authentication scheme to its default value
   authParams->mode = HTTP_AUTH_MODE_NONE;

   //Clear realm
   osStrcpy(authParams->realm, "");

#if (HTTP_CLIENT_DIGEST_AUTH_SUPPORT == ENABLED)
   //Reset authentication parameters
   authParams->qop = HTTP_AUTH_QOP_NONE;
   authParams->algorithm = NULL;
   osStrcpy(authParams->nonce, "");
   osStrcpy(authParams->cnonce, "");
   osStrcpy(authParams->opaque, "");

   //Reset stale flag
   authParams->stale = FALSE;
#endif
}


/**
 * @brief Format Authorization header field
 * @param[in] context Pointer to the HTTP client context
 * @return Error code
 **/

error_t httpClientFormatAuthorizationField(HttpClientContext *context)
{
   size_t n;
   char_t *p;
   HttpClientAuthParams *authParams;

   //Make sure the buffer contains a valid HTTP request
   if(context->bufferLen < 2 || context->bufferLen > HTTP_CLIENT_BUFFER_SIZE)
      return ERROR_INVALID_SYNTAX;

   //Point to the HTTP authentication parameters
   authParams = &context->authParams;

#if (HTTP_CLIENT_BASIC_AUTH_SUPPORT == ENABLED)
   //Basic authentication scheme?
   if(authParams->mode == HTTP_AUTH_MODE_BASIC)
   {
      size_t k;
      size_t m;

      //Calculate the length of the username and password
      n = osStrlen(authParams->username) + osStrlen(authParams->password);

      //Make sure the buffer is large enough
      if((context->bufferLen + n + 22) > HTTP_CLIENT_BUFFER_SIZE)
         return ERROR_BUFFER_OVERFLOW;

      //Point to the buffer where to format the Authorization header field
      p = context->buffer + context->bufferLen - 2;

      //Format Authorization header field
      n = osSprintf(p, "Authorization: Basic ");

      //The client sends the username and password, separated by a single
      //colon character, within a Base64-encoded string in the credentials
      m = osSprintf(p + n, "%s:%s", authParams->username, authParams->password);

      //The first pass calculates the length of the Base64-encoded string
      base64Encode(p + n, m, NULL, &k);

      //Make sure the buffer is large enough
      if((context->bufferLen + n + k) > HTTP_CLIENT_BUFFER_SIZE)
         return ERROR_BUFFER_OVERFLOW;

      //The second pass encodes the string using Base64
      base64Encode(p + n, m, p + n, &k);
      //Update the total length of the header field
      n += k;

      //Make sure the buffer is large enough
      if((context->bufferLen + n + 2) > HTTP_CLIENT_BUFFER_SIZE)
         return ERROR_BUFFER_OVERFLOW;

      //Terminate the header field with a CRLF sequence
      osSprintf(p + n, "\r\n\r\n");

      //Adjust the length of the request header
      context->bufferLen = context->bufferLen + n + 2;
   }
   else
#endif
#if (HTTP_CLIENT_DIGEST_AUTH_SUPPORT == ENABLED)
   //Digest authentication scheme?
   if(authParams->mode == HTTP_AUTH_MODE_DIGEST)
   {
      error_t error;
      const char_t *q;
      const char_t *uri;
      size_t uriLen;
      char_t response[HTTP_CLIENT_MAX_RESPONSE_LEN + 1];

      //Properly terminate the string with a NULL character
      context->buffer[context->bufferLen] = '\0';

      //The Request-Line begins with a method token
      q = osStrchr(context->buffer, ' ');
      //Any parsing error?
      if(q == NULL)
         return ERROR_INVALID_SYNTAX;

      //The method token is followed by the Request-URI
      uri = q + 1;

      //Point to the end of the Request-URI
      q = osStrchr(uri, ' ');
      //Any parsing error?
      if(q == NULL)
         return ERROR_INVALID_SYNTAX;

      //Compute the length of the current URI
      uriLen = q - uri;

      //Check quality of protection
      if(authParams->qop == HTTP_AUTH_QOP_AUTH ||
         authParams->qop == HTTP_AUTH_QOP_AUTH_INT)
      {
         //Make sure that a valid callback function has been registered
         if(context->randCallback == NULL)
            return ERROR_PRNG_NOT_READY;

         //A cryptographically strong random number generator must be used to
         //generate the cnonce
         error = context->randCallback(authParams->cnonce, HTTP_CLIENT_CNONCE_SIZE);
         //Any error to report?
         if(error)
            return error;

         //Convert the byte array to hex string
         httpEncodeHexString(authParams->cnonce, HTTP_CLIENT_CNONCE_SIZE,
            authParams->cnonce);

         //Count of the number of requests (including the current request)
         //that the client has sent with the nonce value in this request
         authParams->nc++;
      }

      //Perform digest operation
      error = httpClientComputeDigest(authParams, context->method,
         osStrlen(context->method), uri, uriLen, response);
      //Any error to report?
      if(error)
         return error;

      //Determine the length of the header field
      n = osStrlen(authParams->username) + osStrlen(authParams->realm) +
         uriLen + osStrlen(authParams->nonce) + osStrlen(authParams->cnonce) +
         osStrlen(response) + osStrlen(authParams->opaque);

      //Make sure the buffer is large enough
      if((context->bufferLen + n + 121) > HTTP_CLIENT_BUFFER_SIZE)
         return ERROR_BUFFER_OVERFLOW;

      //Point to the buffer where to format the Authorization header field
      p = context->buffer + context->bufferLen - 2;

      //Format Authorization header field
      n = osSprintf(p, "Authorization: Digest ");

      //Format username and realm parameter
      n += osSprintf(p + n, "username=\"%s\", ", authParams->username);
      n += osSprintf(p + n, "realm=\"%s\", ", authParams->realm);

      //Format uri parameter
      n += osSprintf(p + n, "uri=\"");
      osStrncpy(p + n, uri, uriLen);
      n += uriLen;
      n += osSprintf(p + n, "\", ");

      //Format nonce parameter
      n += osSprintf(p + n, "nonce=\"%s\", ", authParams->nonce);

      //Check quality of protection
      if(authParams->qop == HTTP_AUTH_QOP_AUTH)
      {
         //Format qop, nc, cnonce parameters
         n += osSprintf(p + n, "qop=auth, ");
         n += osSprintf(p + n, "nc=%08x, ", authParams->nc);
         n += osSprintf(p + n, "cnonce=\"%s\", ", authParams->cnonce);
      }

      //Format response parameter
      n += osSprintf(p + n, "response=\"%s\"", response);

      //The opaque parameter should be returned by the client unchanged in
      //the Authorization header field of subsequent requests
      if(authParams->opaque[0] != '\0')
      {
         //Format opaque parameter
         n += osSprintf(p + n, ", opaque=\"%s\"", authParams->opaque);
      }

      //Terminate the header field with a CRLF sequence
      osSprintf(p + n, "\r\n\r\n");

      //Adjust the length of the request header
      context->bufferLen = context->bufferLen + n + 2;
   }
   else
#endif
   //Unknown authentication scheme?
   {
      //Just for sanity
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse WWW-Authenticate header field
 * @param[in] context Pointer to the HTTP client context
 * @param[in] value NULL-terminated string that contains the value of header field
 * @return Error code
 **/

error_t httpClientParseWwwAuthenticateField(HttpClientContext *context,
   const char_t *value)
{
   error_t error;
   const char_t *p;
   HttpParam param;
   HttpWwwAuthenticateHeader authHeader;

   //Point to the first character of the string
   p = value;

   //Get the first token
   error = httpParseParam(&p, &param);

   //The WWW-Authenticate header field indicates the authentication scheme(s)
   //and parameters applicable to the target resource
   while(!error)
   {
      //The authentication scheme must be a valid token followed by a
      //BWS character
      if(param.value == NULL && (*p == ' ' || *p == '\t'))
      {
         //Clear authentication parameters
         osMemset(&authHeader, 0, sizeof(HttpWwwAuthenticateHeader));

#if (HTTP_CLIENT_DIGEST_AUTH_SUPPORT == ENABLED && HTTP_CLIENT_MD5_SUPPORT == ENABLED)
         //If the algorithm parameter is not present, it is assumed to be
         //MD5 (refer to RFC 7616, section 3.3)
         authHeader.algorithm = MD5_HASH_ALGO;
#endif
         //A case-insensitive token is used to identify the authentication
         //scheme
         if(httpCompareParamName(&param, "Basic"))
         {
            //Basic access authentication
            authHeader.mode = HTTP_AUTH_MODE_BASIC;
         }
         else if(httpCompareParamName(&param, "Digest"))
         {
            //Digest access authentication
            authHeader.mode = HTTP_AUTH_MODE_DIGEST;
         }
         else
         {
            //Unknown authentication scheme
            authHeader.mode = HTTP_AUTH_MODE_NONE;
         }

         //The authentication scheme is followed by a comma-separated list
         //of attribute-value pairs which carry the parameters necessary for
         //achieving authentication via that scheme
         while(!error)
         {
            //Parse authentication parameter
            error = httpParseParam(&p, &param);

            //Check status code
            if(!error)
            {
               //Valid attribute-value pair?
               if(param.value != NULL)
               {
                  //Realm parameter?
                  if(httpCompareParamName(&param, "realm"))
                  {
                     //The realm is a string to be displayed to users so they
                     //know which username and password to use
                     authHeader.realm = param.value;
                     authHeader.realmLen = param.valueLen;
                  }
#if (HTTP_CLIENT_DIGEST_AUTH_SUPPORT == ENABLED)
                  //Quality of protection parameter?
                  else if(httpCompareParamName(&param, "qop"))
                  {
                     //The qop parameter is a quoted string of one or more
                     //tokens indicating the "quality of protection" values
                     //supported by the server
                     httpClientParseQopParam(&param, &authHeader);
                  }
                  //Algorithm parameter?
                  else if(httpCompareParamName(&param, "algorithm"))
                  {
                     //This parameter indicates the algorithm used to produce
                     //the digest
                     httpClientParseAlgorithmParam(&param, &authHeader);
                  }
                  //Nonce parameter?
                  else if(httpCompareParamName(&param, "nonce"))
                  {
                     //The nonce is a server-specified string which should be
                     //uniquely generated each time a 401 response is made
                     authHeader.nonce = param.value;
                     authHeader.nonceLen = param.valueLen;
                  }
                  //Opaque parameter?
                  else if(httpCompareParamName(&param, "opaque"))
                  {
                     //The opaque parameter is a string of data, specified by the
                     //server, that should be returned by the client unchanged in
                     //the Authorization header field of subsequent requests
                     authHeader.opaque = param.value;
                     authHeader.opaqueLen = param.valueLen;
                  }
                  //Stale parameter?
                  else if(httpCompareParamName(&param, "stale"))
                  {
                     //The stale flag is a case-insensitive flag indicating that
                     //the previous request from the client was rejected because
                     //the nonce value was stale
                     if(httpCompareParamValue(&param, "true"))
                        authHeader.stale = TRUE;
                     else
                        authHeader.stale = FALSE;
                  }
#endif
                  //Unknown parameter?
                  else
                  {
                     //Discard unknown attributes
                  }
               }
               else
               {
                  //Parse next authentication scheme
                  break;
               }
            }
         }

#if (HTTP_CLIENT_BASIC_AUTH_SUPPORT == ENABLED)
         //Valid basic authentication parameters?
         if(authHeader.mode == HTTP_AUTH_MODE_BASIC &&
            authHeader.realmLen > 0 &&
            authHeader.realmLen <= HTTP_CLIENT_MAX_REALM_LEN)
         {
            //Save authentication mode
            context->authParams.mode = authHeader.mode;

            //Save realm
            osStrncpy(context->authParams.realm, authHeader.realm, authHeader.realmLen);
            context->authParams.realm[authHeader.realmLen] = '\0';
         }
         else
#endif
#if (HTTP_CLIENT_DIGEST_AUTH_SUPPORT == ENABLED)
         //Valid digest authentication parameters?
         if(authHeader.mode == HTTP_AUTH_MODE_DIGEST &&
            authHeader.algorithm != NULL &&
            authHeader.realmLen > 0 &&
            authHeader.realmLen <= HTTP_CLIENT_MAX_REALM_LEN &&
            authHeader.nonceLen > 0 &&
            authHeader.nonceLen <= HTTP_CLIENT_MAX_NONCE_LEN &&
            authHeader.opaqueLen <= HTTP_CLIENT_MAX_OPAQUE_LEN)
         {
            //Save authentication mode
            context->authParams.mode = authHeader.mode;
            //Save qop parameter
            context->authParams.qop = authHeader.qop;
            //Save digest algorithm
            context->authParams.algorithm = authHeader.algorithm;

            //Save realm
            osStrncpy(context->authParams.realm, authHeader.realm, authHeader.realmLen);
            context->authParams.realm[authHeader.realmLen] = '\0';

            //Save nonce value
            osStrncpy(context->authParams.nonce, authHeader.nonce, authHeader.nonceLen);
            context->authParams.nonce[authHeader.nonceLen] = '\0';

            //Save opaque parameter
            osStrncpy(context->authParams.opaque, authHeader.opaque, authHeader.opaqueLen);
            context->authParams.opaque[authHeader.opaqueLen] = '\0';

            //Save stale flag
            context->authParams.stale = authHeader.stale;
         }
         else
#endif
         //Invalid parameters
         {
            //Ignore the challenge
         }
      }
      else
      {
         //Get next token
         error = httpParseParam(&p, &param);
      }
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse qop parameter
 * @param[in] param Pointer to the algorithm parameter
 * @param[in,out] authHeader Pointer to the WWW-Authenticate header field
 **/

void httpClientParseQopParam(const HttpParam *param,
   HttpWwwAuthenticateHeader *authHeader)
{
#if (HTTP_CLIENT_DIGEST_AUTH_SUPPORT == ENABLED)
   size_t i;
   size_t n;

   //This parameter is a quoted string of one or more tokens indicating
   //the "quality of protection" values supported by the server
   authHeader->qop = HTTP_AUTH_QOP_NONE;

   //Parse the comma-separated list
   for(i = 0; i < param->valueLen; i += (n + 1))
   {
      //Calculate the length of the current token
      for(n = 0; (i + n) < param->valueLen; n++)
      {
         //Separator character found?
         if(osStrchr(", \t", param->value[i + n]) != NULL)
         {
            break;
         }
      }

      //Check current token
      if(n == 4 && !osStrncasecmp(param->value + i, "auth", 4))
      {
         //The value "auth" indicates authentication
         authHeader->qop = HTTP_AUTH_QOP_AUTH;
      }
   }

   //Quality of protection not supported?
   if(authHeader->qop == HTTP_AUTH_QOP_NONE)
   {
      //The challenge should be ignored
      authHeader->mode = HTTP_AUTH_MODE_NONE;
   }
#endif
}


/**
 * @brief Parse algorithm parameter
 * @param[in] param Pointer to the algorithm parameter
 * @param[in,out] authHeader Pointer to the WWW-Authenticate header field
 **/

void httpClientParseAlgorithmParam(const HttpParam *param,
   HttpWwwAuthenticateHeader *authHeader)
{
#if (HTTP_CLIENT_DIGEST_AUTH_SUPPORT == ENABLED)
#if (HTTP_CLIENT_MD5_SUPPORT == ENABLED)
   //MD5 digest algorithm?
   if(httpCompareParamValue(param, "MD5"))
   {
      //Select MD5 digest algorithm
      authHeader->algorithm = MD5_HASH_ALGO;
   }
   else
#endif
#if (HTTP_CLIENT_SHA256_SUPPORT == ENABLED)
   //SHA-256 digest algorithm?
   if(httpCompareParamValue(param, "SHA-256"))
   {
      //Select SHA-256 digest algorithm
      authHeader->algorithm = SHA256_HASH_ALGO;
   }
   else
#endif
#if (HTTP_CLIENT_SHA512_256_SUPPORT == ENABLED)
   //SHA-512/256 digest algorithm?
   if(httpCompareParamValue(param, "SHA-512-256"))
   {
      //Select SHA-512/256 digest algorithm
      authHeader->algorithm = SHA512_256_HASH_ALGO;
   }
   else
#endif
   //Unknown digest algorithm?
   {
      //If the algorithm is not understood, the challenge should be
      //ignored (refer to RFC 7616, section 3.3)
      authHeader->mode = HTTP_AUTH_MODE_NONE;
   }
#endif
}


/**
 * @brief Digest operation
 * @param[in] authParams HTTP authentication parameters
 * @param[in] method Pointer to the HTTP method
 * @param[in] methodLen Length of the HTTP method
 * @param[in] uri Pointer to the URI
 * @param[in] uriLen Length of the URI
 * @param[out] response Pointer to the resulting digest
 * @return Error code
 **/

error_t httpClientComputeDigest(HttpClientAuthParams *authParams,
   const char_t *method, size_t methodLen, const char_t *uri,
   size_t uriLen, char_t *response)
{
#if (HTTP_CLIENT_DIGEST_AUTH_SUPPORT == ENABLED)
   size_t i;
   char_t buffer[9];
   uint8_t ha1[HTTP_CLIENT_MAX_HASH_DIGEST_SIZE];
   uint8_t ha2[HTTP_CLIENT_MAX_HASH_DIGEST_SIZE];
   uint8_t hashContext[HTTP_CLIENT_MAX_HASH_CONTEXT_SIZE];
   const HashAlgo *hash;

   //Point to the hash algorithm to be used
   hash = authParams->algorithm;
   //Make sure the hash algorithm is valid
   if(hash == NULL)
      return ERROR_FAILURE;

   //Compute H(A1) = H(username : realm : password)
   hash->init(hashContext);
   hash->update(hashContext, authParams->username, osStrlen(authParams->username));
   hash->update(hashContext, ":", 1);
   hash->update(hashContext, authParams->realm, osStrlen(authParams->realm));
   hash->update(hashContext, ":", 1);
   hash->update(hashContext, authParams->password, osStrlen(authParams->password));
   hash->final(hashContext, ha1);

   //Compute H(A2) = H(method : uri)
   hash->init(hashContext);
   hash->update(hashContext, method, methodLen);
   hash->update(hashContext, ":", 1);
   hash->update(hashContext, uri, uriLen);
   hash->final(hashContext, ha2);

   //Compute H(H(A1) : nonce : nc : cnonce : qop : H(A2))
   hash->init(hashContext);

   //Digest H(A1) as an hex string
   for(i = 0; i < hash->digestSize; i++)
   {
      //Convert the current byte to hex representation
      osSprintf(buffer, "%02" PRIx8, ha1[i]);
      //Digest the resulting value
      hash->update(hashContext, buffer, 2);
   }

   //Digest nonce parameter
   hash->update(hashContext, ":", 1);
   hash->update(hashContext, authParams->nonce, osStrlen(authParams->nonce));
   hash->update(hashContext, ":", 1);

   //Convert the nonce count to hex string
   osSprintf(buffer, "%08x", authParams->nc);

   //Check quality of protection
   if(authParams->qop == HTTP_AUTH_QOP_AUTH ||
      authParams->qop == HTTP_AUTH_QOP_AUTH_INT)
   {
      //Digest nc, cnonce and qop parameters
      hash->update(hashContext, buffer, 8);
      hash->update(hashContext, ":", 1);
      hash->update(hashContext, authParams->cnonce, osStrlen(authParams->cnonce));
      hash->update(hashContext, ":", 1);
      hash->update(hashContext, "auth", 4);
      hash->update(hashContext, ":", 1);
   }

   //Digest H(A2) as an hex string
   for(i = 0; i < hash->digestSize; i++)
   {
      //Convert the current byte to hex representation
      osSprintf(buffer, "%02" PRIx8, ha2[i]);
      //Digest the resulting value
      hash->update(hashContext, buffer, 2);
   }

   //Finalize hash computation
   hash->final(hashContext, response);

   //Convert the resulting digest to hex string
   httpEncodeHexString(response, hash->digestSize, response);

   //Successful processing
   return NO_ERROR;
#else
   //Digest authentication is not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}

#endif
