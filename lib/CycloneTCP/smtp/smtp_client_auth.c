/**
 * @file smtp_client_auth.c
 * @brief SMTP authentication mechanism
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
#include "smtp/smtp_client_auth.h"
#include "smtp/smtp_client_misc.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (SMTP_CLIENT_SUPPORT == ENABLED)

/**
 * @brief Perform LOGIN authentication
 * @param[in] context Pointer to the SMTP client context
 * @param[in] username NULL-terminated string containing the user name
 * @param[in] password NULL-terminated string containing the user's password
 * @return Error code
 **/

error_t smtpClientLoginAuth(SmtpClientContext *context,
   const char_t *username, const char_t *password)
{
#if (SMTP_CLIENT_LOGIN_AUTH_SUPPORT == ENABLED)
   error_t error;
   size_t n;

   //Initialize status code
   error = NO_ERROR;

   //Execute SMTP command sequence
   while(!error)
   {
      //Check current state
      if(context->state == SMTP_CLIENT_STATE_CONNECTED)
      {
         //Format AUTH LOGIN command
         error = smtpClientFormatCommand(context, "AUTH LOGIN", NULL);

         //Check status code
         if(!error)
         {
            //Send AUTH LOGIN command and wait for the server's response
            smtpClientChangeState(context, SMTP_CLIENT_STATE_SUB_COMMAND_1);
         }
      }
      else if(context->state == SMTP_CLIENT_STATE_SUB_COMMAND_1)
      {
         //Send AUTH LOGIN command and wait for the server's response
         error = smtpClientSendCommand(context, NULL);

         //Check status code
         if(!error)
         {
            //Check SMTP response code
            if(SMTP_REPLY_CODE_3YZ(context->replyCode))
            {
               //Retrieve the length of the user name
               n = osStrlen(username);

               //Encode the user name with Base64 algorithm
               base64Encode(username, n, context->buffer, NULL);
               //Terminate the line with a CRLF sequence
               osStrcat(context->buffer, "\r\n");

               //Calculate the length of the SMTP command
               context->commandLen = osStrlen(context->buffer);

               //Debug message
               TRACE_DEBUG("SMTP client: %s", context->buffer);

               //Flush receive buffer
               context->bufferPos = 0;
               context->replyLen = 0;

               //Send the second command of the AUTH LOGIN sequence
               smtpClientChangeState(context, SMTP_CLIENT_STATE_SUB_COMMAND_2);
            }
            else
            {
               //Update SMTP client state
               smtpClientChangeState(context, SMTP_CLIENT_STATE_CONNECTED);
               //Report an error
               error = ERROR_UNEXPECTED_RESPONSE;
            }
         }
      }
      else if(context->state == SMTP_CLIENT_STATE_SUB_COMMAND_2)
      {
         //Wait for the server's response
         error = smtpClientSendCommand(context, NULL);

         //Check status code
         if(!error)
         {
            //Check SMTP response code
            if(SMTP_REPLY_CODE_3YZ(context->replyCode))
            {
               //Retrieve the length of the password
               n = osStrlen(password);

               //Encode the password with Base64 algorithm
               base64Encode(password, n, context->buffer, NULL);
               //Terminate the line with a CRLF sequence
               osStrcat(context->buffer, "\r\n");

               //Calculate the length of the SMTP command
               context->commandLen = osStrlen(context->buffer);

               //Debug message
               TRACE_DEBUG("SMTP client: %s", context->buffer);

               //Flush receive buffer
               context->bufferPos = 0;
               context->replyLen = 0;

               //Send the third command of the AUTH LOGIN sequence
               smtpClientChangeState(context, SMTP_CLIENT_STATE_SUB_COMMAND_3);
            }
            else
            {
               //Update SMTP client state
               smtpClientChangeState(context, SMTP_CLIENT_STATE_CONNECTED);
               //Report an error
               error = ERROR_UNEXPECTED_RESPONSE;
            }
         }
      }
      else if(context->state == SMTP_CLIENT_STATE_SUB_COMMAND_3)
      {
         //Wait for the server's response
         error = smtpClientSendCommand(context, NULL);

         //Check status code
         if(!error)
         {
            //Check SMTP response code
            if(SMTP_REPLY_CODE_2YZ(context->replyCode))
            {
               //Update SMTP client state
               smtpClientChangeState(context, SMTP_CLIENT_STATE_CONNECTED);
               //Successful user authentication
               break;
            }
            else
            {
               //Update SMTP client state
               smtpClientChangeState(context, SMTP_CLIENT_STATE_CONNECTED);
               //Report an error
               error = ERROR_UNEXPECTED_RESPONSE;
            }
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
      error = smtpClientCheckTimeout(context);
   }

   //Return status code
   return error;
#else
   //LOGIN authentication is not supported
   return ERROR_AUTHENTICATION_FAILED;
#endif
}


/**
 * @brief Perform PLAIN authentication
 * @param[in] context Pointer to the SMTP client context
 * @param[in] username NULL-terminated string containing the user name
 * @param[in] password NULL-terminated string containing the user's password
 * @return Error code
 **/

error_t smtpClientPlainAuth(SmtpClientContext *context,
   const char_t *username, const char_t *password)
{
#if (SMTP_CLIENT_PLAIN_AUTH_SUPPORT == ENABLED)
   error_t error;
   size_t m;
   size_t n;
   char_t *p;

   //Initialize status code
   error = NO_ERROR;

   //Execute SMTP command sequence
   while(!error)
   {
      //Check current state
      if(context->state == SMTP_CLIENT_STATE_CONNECTED)
      {
         //Point to the buffer
         p = context->buffer;

         //Format AUTH PLAIN command
         m = osSprintf(p, "AUTH PLAIN ");

         //Authorization identity
         n = osSprintf(p + m, "%s", username) + 1;
         //Authentication identity
         n += osSprintf(p + m + n, "%s", username) + 1;
         //Password
         n += osSprintf(p + m + n, "%s", password);

         //Base64 encoding
         base64Encode(p + m, n, p + m, NULL);
         //Terminate the line with a CRLF sequence
         osStrcat(p, "\r\n");

         //Calculate the length of the SMTP command
         context->commandLen = osStrlen(p);

         //Debug message
         TRACE_DEBUG("SMTP client: %s", context->buffer);

         //Flush receive buffer
         context->bufferPos = 0;
         context->replyLen = 0;

         //Send AUTH PLAIN command and wait for the server's response
         smtpClientChangeState(context, SMTP_CLIENT_STATE_SUB_COMMAND_1);
      }
      else if(context->state == SMTP_CLIENT_STATE_SUB_COMMAND_1)
      {
         //Send AUTH PLAIN command and wait for the server's response
         error = smtpClientSendCommand(context, NULL);

         //Check status code
         if(!error)
         {
            //Check SMTP response code
            if(SMTP_REPLY_CODE_2YZ(context->replyCode))
            {
               //Update SMTP client state
               smtpClientChangeState(context, SMTP_CLIENT_STATE_CONNECTED);
               //Successful user authentication
               break;
            }
            else
            {
               //Update SMTP client state
               smtpClientChangeState(context, SMTP_CLIENT_STATE_CONNECTED);
               //Report an error
               error = ERROR_UNEXPECTED_RESPONSE;
            }
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
      error = smtpClientCheckTimeout(context);
   }

   //Return status code
   return error;
#else
   //PLAIN authentication is not supported
   return ERROR_AUTHENTICATION_FAILED;
#endif
}


/**
 * @brief Perform CRAM-MD5 authentication
 * @param[in] context Pointer to the SMTP client context
 * @param[in] username NULL-terminated string containing the user name
 * @param[in] password NULL-terminated string containing the user's password
 * @return Error code
 **/

error_t smtpClientCramMd5Auth(SmtpClientContext *context,
   const char_t *username, const char_t *password)
{
#if (SMTP_CLIENT_CRAM_MD5_AUTH_SUPPORT == ENABLED)
   error_t error;
   int_t i;
   size_t n;
   char_t *p;

   //Hex conversion table
   static const char_t hexDigit[] =
   {
      '0', '1', '2', '3', '4', '5', '6', '7',
      '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
   };

   //Initialize status code
   error = NO_ERROR;

   //Execute SMTP command sequence
   while(!error)
   {
      //Check current state
      if(context->state == SMTP_CLIENT_STATE_CONNECTED)
      {
         //Format AUTH CRAM-MD5 command
         error = smtpClientFormatCommand(context, "AUTH CRAM-MD5", NULL);

         //Check status code
         if(!error)
         {
            //Send AUTH CRAM-MD5 command and wait for the server's response
            smtpClientChangeState(context, SMTP_CLIENT_STATE_SUB_COMMAND_1);
         }
      }
      else if(context->state == SMTP_CLIENT_STATE_SUB_COMMAND_1)
      {
         //Send AUTH CRAM-MD5 command and wait for the server's response
         error = smtpClientSendCommand(context, NULL);

         //Check status code
         if(!error)
         {
            //Check SMTP response code
            if(SMTP_REPLY_CODE_3YZ(context->replyCode))
            {
               //Point to the buffer
               p = context->buffer;
               //Retrieve the length of the response
               n = osStrlen(p);

               //Unexpected response from the SMTP server?
               if(n <= 4)
               {
                  //Report an error
                  error = ERROR_INVALID_SYNTAX;
               }

               //Check status code
               if(!error)
               {
                  //Decrypt the Base64-encoded challenge
                  error = base64Decode(p + 4, n - 4, p, &n);
               }

               //Check status code
               if(!error)
               {
                  //Compute HMAC using MD5
                  error = hmacCompute(MD5_HASH_ALGO, password, osStrlen(password),
                     p, n, p);
               }

               //Check status code
               if(!error)
               {
                  //Convert the digest to hexadecimal string
                  for(i = MD5_DIGEST_SIZE - 1; i >= 0; i--)
                  {
                     //Convert the lower nibble
                     p[i * 2 + 1] = hexDigit[p[i] & 0x0F];
                     //Then convert the upper nibble
                     p[i * 2] = hexDigit[(p[i] >> 4) & 0x0F];
                  }

                  //Properly terminate the string with a NULL character
                  p[MD5_DIGEST_SIZE * 2] = '\0';

                  //Make room for the username
                  n = osStrlen(username);
                  osMemmove(p + n + 1, p, MD5_DIGEST_SIZE * 2 + 1);

                  //Concatenate the user name and the text representation of
                  //the digest
                  osStrcpy(p, username);
                  p[n] = ' ';

                  //Encode the resulting string with Base64 algorithm
                  base64Encode(p, osStrlen(p), p, NULL);
                  //Terminate the line with a CRLF sequence
                  osStrcat(p, "\r\n");

                  //Calculate the length of the SMTP command
                  context->commandLen = osStrlen(p);

                  //Debug message
                  TRACE_DEBUG("SMTP client: %s", context->buffer);

                  //Flush receive buffer
                  context->bufferPos = 0;
                  context->replyLen = 0;

                  //Send the second command of the AUTH CRAM-MD5 sequence
                  smtpClientChangeState(context, SMTP_CLIENT_STATE_SUB_COMMAND_2);
               }
            }
            else
            {
               //Update SMTP client state
               smtpClientChangeState(context, SMTP_CLIENT_STATE_CONNECTED);
               //Report an error
               error = ERROR_UNEXPECTED_RESPONSE;
            }
         }
      }
      else if(context->state == SMTP_CLIENT_STATE_SUB_COMMAND_2)
      {
         //Wait for the server's response
         error = smtpClientSendCommand(context, NULL);

         //Check status code
         if(!error)
         {
            //Check SMTP response code
            if(SMTP_REPLY_CODE_2YZ(context->replyCode))
            {
               //Update SMTP client state
               smtpClientChangeState(context, SMTP_CLIENT_STATE_CONNECTED);
               //Successful user authentication
               break;
            }
            else
            {
               //Update SMTP client state
               smtpClientChangeState(context, SMTP_CLIENT_STATE_CONNECTED);
               //Report an error
               error = ERROR_UNEXPECTED_RESPONSE;
            }
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
      error = smtpClientCheckTimeout(context);
   }

   //Return status code
   return error;
#else
   //CRAM-MD5 authentication is not supported
   return ERROR_AUTHENTICATION_FAILED;
#endif
}

#endif
