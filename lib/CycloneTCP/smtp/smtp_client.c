/**
 * @file smtp_client.c
 * @brief SMTP client (Simple Mail Transfer Protocol)
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
 * SMTP is designed as a mail transport and delivery protocol. Refer to
 * the following RFCs for complete details:
 * - RFC 5321: Simple Mail Transfer Protocol
 * - RFC 4954: SMTP Service Extension for Authentication
 * - RFC 3207: SMTP Service Extension for Secure SMTP over TLS
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
#include "smtp/smtp_client_transport.h"
#include "smtp/smtp_client_misc.h"
#include "str.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (SMTP_CLIENT_SUPPORT == ENABLED)


/**
 * @brief Initialize SMTP client context
 * @param[in] context Pointer to the SMTP client context
 * @return Error code
 **/

error_t smtpClientInit(SmtpClientContext *context)
{
#if (SMTP_CLIENT_TLS_SUPPORT == ENABLED)
   error_t error;
#endif

   //Make sure the SMTP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Clear SMTP client context
   osMemset(context, 0, sizeof(SmtpClientContext));

#if (SMTP_CLIENT_TLS_SUPPORT == ENABLED)
   //Initialize TLS session state
   error = tlsInitSessionState(&context->tlsSession);
   //Any error to report?
   if(error)
      return error;
#endif

   //Initialize SMTP client state
   context->state = SMTP_CLIENT_STATE_DISCONNECTED;

   //Default timeout
   context->timeout = SMTP_CLIENT_DEFAULT_TIMEOUT;

   //Successful initialization
   return NO_ERROR;
}


#if (SMTP_CLIENT_TLS_SUPPORT == ENABLED)

/**
 * @brief Register TLS initialization callback function
 * @param[in] context Pointer to the SMTP client context
 * @param[in] callback TLS initialization callback function
 * @return Error code
 **/

error_t smtpClientRegisterTlsInitCallback(SmtpClientContext *context,
   SmtpClientTlsInitCallback callback)
{
   //Check parameters
   if(context == NULL || callback == NULL)
      return ERROR_INVALID_PARAMETER;

   //Save callback function
   context->tlsInitCallback = callback;

   //Successful processing
   return NO_ERROR;
}

#endif


/**
 * @brief Set communication timeout
 * @param[in] context Pointer to the SMTP client context
 * @param[in] timeout Timeout value, in milliseconds
 * @return Error code
 **/

error_t smtpClientSetTimeout(SmtpClientContext *context, systime_t timeout)
{
   //Make sure the SMTP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Save timeout value
   context->timeout = timeout;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Bind the SMTP client to a particular network interface
 * @param[in] context Pointer to the SMTP client context
 * @param[in] interface Network interface to be used
 * @return Error code
 **/

error_t smtpClientBindToInterface(SmtpClientContext *context,
   NetInterface *interface)
{
   //Make sure the SMTP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Explicitly associate the SMTP client with the specified interface
   context->interface = interface;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Establish a connection with the specified SMTP server
 * @param[in] context Pointer to the SMTP client context
 * @param[in] serverIpAddr IP address of the SMTP server
 * @param[in] serverPort Port number
 * @param[in] mode SMTP connection mode
 * @return Error code
 **/

error_t smtpClientConnect(SmtpClientContext *context,
   const IpAddr *serverIpAddr, uint16_t serverPort, SmtpConnectionMode mode)
{
   error_t error;

   //Check parameters
   if(context == NULL || serverIpAddr == NULL)
      return ERROR_INVALID_PARAMETER;

#if (SMTP_CLIENT_TLS_SUPPORT == ENABLED)
   //Check connection mode
   if(mode != SMTP_MODE_PLAINTEXT &&
      mode != SMTP_MODE_IMPLICIT_TLS &&
      mode != SMTP_MODE_EXPLICIT_TLS)
   {
      //The connection mode is not valid
      return ERROR_INVALID_PARAMETER;
   }
#else
   //Check connection mode
   if(mode != SMTP_MODE_PLAINTEXT)
   {
      //The connection mode is not valid
      return ERROR_INVALID_PARAMETER;
   }
#endif

   //Initialize status code
   error = NO_ERROR;

   //Establish connection with the SMTP server
   while(!error)
   {
      //Check current state
      if(context->state == SMTP_CLIENT_STATE_DISCONNECTED)
      {
         //Reset parameters
         context->startTlsSupported = FALSE;
         context->authLoginSupported = FALSE;
         context->authPlainSupported = FALSE;
         context->authCramMd5Supported = FALSE;

#if (SMTP_CLIENT_MIME_SUPPORT == ENABLED)
         //Reset MIME-specific parameters
         osStrcpy(context->contentType, "");
         osStrcpy(context->boundary, "this-is-a-boundary");
#endif
         //Open TCP socket
         error = smtpClientOpenConnection(context);

         //Check status code
         if(!error)
         {
            //Establish TCP connection
            smtpClientChangeState(context, SMTP_CLIENT_STATE_CONNECTING_TCP);
         }
      }
      else if(context->state == SMTP_CLIENT_STATE_CONNECTING_TCP)
      {
         //Establish TCP connection
         error = smtpClientEstablishConnection(context, serverIpAddr,
            serverPort);

         //Check status code
         if(!error)
         {
            //Implicit TLS?
            if(mode == SMTP_MODE_IMPLICIT_TLS)
            {
               //TLS initialization
               error = smtpClientOpenSecureConnection(context);

               //Check status code
               if(!error)
               {
                  //Perform TLS handshake
                  smtpClientChangeState(context, SMTP_CLIENT_STATE_CONNECTING_TLS);
               }
            }
            else
            {
               //Flush buffer
               context->bufferPos = 0;
               context->commandLen = 0;
               context->replyLen = 0;

               //Wait for the connection greeting reply
               smtpClientChangeState(context, SMTP_CLIENT_STATE_SUB_COMMAND_1);
            }
         }
      }
      else if(context->state == SMTP_CLIENT_STATE_CONNECTING_TLS)
      {
         //Perform TLS handshake
         error = smtpClientEstablishSecureConnection(context);

         //Check status code
         if(!error)
         {
            //Implicit TLS?
            if(mode == SMTP_MODE_IMPLICIT_TLS)
            {
               //Flush buffer
               context->bufferPos = 0;
               context->commandLen = 0;
               context->replyLen = 0;

               //Wait for the connection greeting reply
               smtpClientChangeState(context, SMTP_CLIENT_STATE_SUB_COMMAND_1);
            }
            else
            {
               //Format EHLO command
               error = smtpClientFormatCommand(context, "EHLO [127.0.0.1]", NULL);

               //Check status code
               if(!error)
               {
                  //Send EHLO command and wait for the server's response
                  smtpClientChangeState(context, SMTP_CLIENT_STATE_SUB_COMMAND_2);
               }
            }
         }
      }
      else if(context->state == SMTP_CLIENT_STATE_SUB_COMMAND_1)
      {
         //Wait for the connection greeting reply
         error = smtpClientSendCommand(context, NULL);

         //Check status code
         if(!error)
         {
            //Check SMTP response code
            if(SMTP_REPLY_CODE_2YZ(context->replyCode))
            {
               //Format EHLO command
               error = smtpClientFormatCommand(context, "EHLO [127.0.0.1]", NULL);

               //Check status code
               if(!error)
               {
                  //Send EHLO command and wait for the server's response
                  smtpClientChangeState(context, SMTP_CLIENT_STATE_SUB_COMMAND_2);
               }
            }
            else
            {
               //Report an error
               error = ERROR_UNEXPECTED_RESPONSE;
            }
         }
      }
      else if(context->state == SMTP_CLIENT_STATE_SUB_COMMAND_2)
      {
         //Send EHLO command and wait for the server's response
         error = smtpClientSendCommand(context, smtpClientParseEhloReply);

         //Check status code
         if(!error)
         {
            //Check SMTP response code
            if(SMTP_REPLY_CODE_2YZ(context->replyCode))
            {
#if (SMTP_CLIENT_TLS_SUPPORT == ENABLED)
               //Explicit TLS?
               if(mode == SMTP_MODE_EXPLICIT_TLS && context->tlsContext == NULL)
               {
                  //Format STARTTLS command
                  error = smtpClientFormatCommand(context, "STARTTLS", NULL);

                  //Check status code
                  if(!error)
                  {
                     //Send STARTTLS command and wait for the server's response
                     smtpClientChangeState(context, SMTP_CLIENT_STATE_SUB_COMMAND_3);
                  }
               }
               else
#endif
               {
                  //The SMTP client is connected
                  smtpClientChangeState(context, SMTP_CLIENT_STATE_CONNECTED);
               }
            }
            else
            {
               //Report an error
               error = ERROR_UNEXPECTED_RESPONSE;
            }
         }
      }
      else if(context->state == SMTP_CLIENT_STATE_SUB_COMMAND_3)
      {
         //Send STARTTLS command and wait for the server's response
         error = smtpClientSendCommand(context, NULL);

         //Check status code
         if(!error)
         {
            //Check SMTP response code
            if(SMTP_REPLY_CODE_2YZ(context->replyCode))
            {
               //TLS initialization
               error = smtpClientOpenSecureConnection(context);

               //Check status code
               if(!error)
               {
                  //Perform TLS handshake
                  smtpClientChangeState(context, SMTP_CLIENT_STATE_CONNECTING_TLS);
               }
            }
            else
            {
               //Report an error
               error = ERROR_UNEXPECTED_RESPONSE;
            }
         }
      }
      else if(context->state == SMTP_CLIENT_STATE_CONNECTED)
      {
         //The SMTP client is connected
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
      error = smtpClientCheckTimeout(context);
   }

   //Failed to establish connection with the SMTP server?
   if(error != NO_ERROR && error != ERROR_WOULD_BLOCK)
   {
      //Clean up side effects
      smtpClientCloseConnection(context);
      //Update SMTP client state
      smtpClientChangeState(context, SMTP_CLIENT_STATE_DISCONNECTED);
   }

   //Return status code
   return error;
}


/**
 * @brief Login to the SMTP server using the provided user name and password
 * @param[in] context Pointer to the SMTP client context
 * @param[in] username NULL-terminated string containing the user name
 * @param[in] password NULL-terminated string containing the user's password
 * @return Error code
 **/

error_t smtpClientLogin(SmtpClientContext *context, const char_t *username,
   const char_t *password)
{
   error_t error;

   //Check parameters
   if(context == NULL || username == NULL || password == NULL)
      return ERROR_INVALID_PARAMETER;

#if (SMTP_CLIENT_CRAM_MD5_AUTH_SUPPORT == ENABLED)
   //CRAM-MD5 authentication mechanism supported?
   if(context->authCramMd5Supported)
   {
      //Perform CRAM-MD5 authentication
      error = smtpClientCramMd5Auth(context, username, password);
   }
   else
#endif
#if (SMTP_CLIENT_LOGIN_AUTH_SUPPORT == ENABLED)
   //LOGIN authentication mechanism supported?
   if(context->authLoginSupported)
   {
      //Perform LOGIN authentication
      error = smtpClientLoginAuth(context, username, password);
   }
   else
#endif
#if (SMTP_CLIENT_PLAIN_AUTH_SUPPORT == ENABLED)
   //PLAIN authentication mechanism supported?
   if(context->authPlainSupported)
   {
      //Perform PLAIN authentication
      error = smtpClientPlainAuth(context, username, password);
   }
   else
#endif
   {
      //Report an error
      error = ERROR_AUTHENTICATION_FAILED;
   }

   //Return status code
   return error;
}


/**
 * @brief Set the content type to be used
 * @param[in] context Pointer to the SMTP client context
 * @param[in] contentType NULL-terminated string that holds the content type
 * @return Error code
 **/

error_t smtpClientSetContentType(SmtpClientContext *context,
   const char_t *contentType)
{
#if (SMTP_CLIENT_MIME_SUPPORT == ENABLED)
   size_t n;

   //Check parameters
   if(context == NULL || contentType == NULL)
      return ERROR_INVALID_PARAMETER;

   //Retrieve the length of the boundary string
   n = osStrlen(contentType);

   //Check the length of the string
   if(n < 1 || n > SMTP_CLIENT_CONTENT_TYPE_MAX_LEN)
      return ERROR_INVALID_LENGTH;

   //Save content type
   osStrcpy(context->contentType, contentType);

   //Successful processing
   return NO_ERROR;
#else
   //MIME extension is not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Define the boundary string to be used (multipart encoding)
 * @param[in] context Pointer to the SMTP client context
 * @param[in] boundary NULL-terminated string that holds the boundary string
 * @return Error code
 **/

error_t smtpClientSetMultipartBoundary(SmtpClientContext *context,
   const char_t *boundary)
{
#if (SMTP_CLIENT_MIME_SUPPORT == ENABLED)
   size_t n;

   //Check parameters
   if(context == NULL || boundary == NULL)
      return ERROR_INVALID_PARAMETER;

   //Retrieve the length of the boundary string
   n = osStrlen(boundary);

   //The boundary parameter consists of 1 to 70 characters
   if(n < 1 || n > SMTP_CLIENT_BOUNDARY_MAX_LEN)
      return ERROR_INVALID_LENGTH;

   //Save boundary string
   osStrcpy(context->boundary, boundary);

   //Successful processing
   return NO_ERROR;
#else
   //MIME extension is not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Write email header
 * @param[in] context Pointer to the SMTP client context
 * @param[in] from Email address of the sender
 * @param[in] recipients Email addresses of the recipients
 * @param[in] numRecipients Number of email addresses in the list
 * @param[in] subject NULL-terminated string containing the email subject
 * @return Error code
 **/

error_t smtpClientWriteMailHeader(SmtpClientContext *context,
   const SmtpMailAddr *from, const SmtpMailAddr *recipients,
   uint_t numRecipients, const char_t *subject)
{
   error_t error;
   size_t n;

   //Check parameters
   if(context == NULL || from == NULL || recipients == NULL || subject == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Execute SMTP command sequence
   while(!error)
   {
      //Check current state
      if(context->state == SMTP_CLIENT_STATE_CONNECTED)
      {
         //Format MAIL FROM command
         error = smtpClientFormatCommand(context, "MAIL FROM", from->addr);

         //Check status code
         if(!error)
         {
            //Point to the first recipient of the list
            context->recipientIndex = 0;
            //Send MAIL FROM command and wait for the server's response
            smtpClientChangeState(context, SMTP_CLIENT_STATE_SUB_COMMAND_1);
         }
      }
      else if(context->state == SMTP_CLIENT_STATE_SUB_COMMAND_1)
      {
         //Wait for the server's response
         error = smtpClientSendCommand(context, NULL);

         //Check status code
         if(!error)
         {
            //Check SMTP response code
            if(SMTP_REPLY_CODE_2YZ(context->replyCode))
            {
               //Process each recipients of the list
               if(context->recipientIndex < numRecipients)
               {
                  //Format RCPT TO command
                  error = smtpClientFormatCommand(context, "RCPT TO",
                     recipients[context->recipientIndex].addr);

                  //Check status code
                  if(!error)
                  {
                     //Point to the next recipient
                     context->recipientIndex++;
                     //Send RCPT TO command and wait for the server's response
                     smtpClientChangeState(context, SMTP_CLIENT_STATE_SUB_COMMAND_1);
                  }
               }
               else
               {
                  //Format DATA command
                  error = smtpClientFormatCommand(context, "DATA", NULL);

                  //Check status code
                  if(!error)
                  {
                     //Send DATA command and wait for the server's response
                     smtpClientChangeState(context, SMTP_CLIENT_STATE_SUB_COMMAND_2);
                  }
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
         //Send DATA command and wait for the server's response
         error = smtpClientSendCommand(context, NULL);

         //Check status code
         if(!error)
         {
            //Check SMTP response code
            if(SMTP_REPLY_CODE_3YZ(context->replyCode))
            {
               //Format email header
               error = smtpClientFormatMailHeader(context, from, recipients,
                  numRecipients, subject);

               //Check status code
               if(!error)
               {
                  //Send email header
                  smtpClientChangeState(context, SMTP_CLIENT_STATE_MAIL_HEADER);
               }
            }
            else
            {
               //Report an error
               error = ERROR_UNEXPECTED_RESPONSE;
            }
         }
      }
      else if(context->state == SMTP_CLIENT_STATE_MAIL_HEADER)
      {
         //Send email header
         if(context->bufferPos < context->bufferLen)
         {
            //Send more data
            error = smtpClientSendData(context,
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
            //Flush transmit buffer
            context->bufferPos = 0;
            context->bufferLen = 0;

            //Update SMTP client state
            smtpClientChangeState(context, SMTP_CLIENT_STATE_MAIL_BODY);
            //The email header has been successfully written
            break;
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
}


/**
 * @brief Write email body
 * @param[in] context Pointer to the SMTP client context
 * @param[in] data Pointer to a buffer containing the data to be written
 * @param[in] length Number of data bytes to write
 * @param[in] written Number of bytes that have been written (optional parameter)
 * @param[in] flags Set of flags that influences the behavior of this function
 * @return Error code
 **/

error_t smtpClientWriteMailBody(SmtpClientContext *context,
   const void *data, size_t length, size_t *written, uint_t flags)
{
   error_t error;
   size_t n;

   //Make sure the SMTP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Check parameters
   if(data == NULL && length != 0)
      return ERROR_INVALID_PARAMETER;

   //Actual number of bytes written
   n = 0;

   //Check current state
   if(context->state == SMTP_CLIENT_STATE_MAIL_BODY)
   {
      //Transmit the contents of the body
      error = smtpClientSendData(context, data, length, &n, flags);

      //Check status code
      if(error == NO_ERROR || error == ERROR_TIMEOUT)
      {
         //Any data transmitted?
         if(n > 0)
         {
            //Save current time
            context->timestamp = osGetSystemTime();
         }
      }
   }
   else
   {
      //Invalid state
      error = ERROR_WRONG_STATE;
   }

  //Check status code
   if(error == ERROR_WOULD_BLOCK || error == ERROR_TIMEOUT)
   {
      //Check whether the timeout has elapsed
      error = smtpClientCheckTimeout(context);
   }

   //Total number of data that have been written
   if(written != NULL)
      *written = n;

   //Return status code
   return error;
}


/**
 * @brief Write multipart header
 * @param[in] context Pointer to the SMTP client context
 * @param[in] filename NULL-terminated string that holds the file name
 *   (optional parameter)
 * @param[in] contentType NULL-terminated string that holds the content type
 *   (optional parameter)
 * @param[in] contentTransferEncoding NULL-terminated string that holds the
 *   content transfer encoding (optional parameter)
 * @param[in] last This flag indicates whether the multipart header is the
 *   final one
 * @return Error code
 **/

error_t smtpClientWriteMultipartHeader(SmtpClientContext *context,
   const char_t *filename, const char_t *contentType,
   const char_t *contentTransferEncoding, bool_t last)
{
#if (SMTP_CLIENT_MIME_SUPPORT == ENABLED)
   error_t error;
   size_t n;

   //Make sure the SMTP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Format and send multipart header
   while(!error)
   {
      //Check current state
      if(context->state == SMTP_CLIENT_STATE_MAIL_BODY ||
         context->state == SMTP_CLIENT_STATE_MULTIPART_BODY)
      {
         //Any data residue?
         if(context->bufferLen > 0 && context->bufferLen < 4)
         {
            //Encode the final quantum
            base64Encode(context->buffer, context->bufferLen,
               context->buffer, &n);

            //Save the length of the Base64-encoded string
            context->bufferLen = n;
            context->bufferPos = 0;
         }
         else if(context->bufferPos < context->bufferLen)
         {
            //Send more data
            error = smtpClientSendData(context,
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
            //Rewind to the beginning of the buffer
            context->bufferPos = 0;
            context->bufferLen = 0;

            //Format multipart header
            error = smtpClientFormatMultipartHeader(context, filename,
               contentType, contentTransferEncoding, last);

            //Check status code
            if(!error)
            {
               //Send multipart header
               smtpClientChangeState(context, SMTP_CLIENT_STATE_MULTIPART_HEADER);
            }
         }
      }
      else if(context->state == SMTP_CLIENT_STATE_MULTIPART_HEADER)
      {
         //Send multipart header
         if(context->bufferPos < context->bufferLen)
         {
            //Send more data
            error = smtpClientSendData(context,
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
            //Rewind to the beginning of the buffer
            context->bufferPos = 0;
            context->bufferLen = 0;

            //Last multipart header?
            if(last)
            {
               //The last multipart header has been successfully transmitted
               smtpClientChangeState(context, SMTP_CLIENT_STATE_MAIL_BODY);
            }
            else
            {
               //Send multipart body
               smtpClientChangeState(context, SMTP_CLIENT_STATE_MULTIPART_BODY);
            }

            //The email header has been successfully written
            break;
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
   //MIME extension is not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Write data to the the multipart body
 * @param[in] context Pointer to the SMTP client context
 * @param[in] data Pointer to the buffer containing the data to be transmitted
 * @param[in] length Number of data bytes to send
 * @param[out] written Actual number of bytes written (optional parameter)
 * @param[in] flags Set of flags that influences the behavior of this function
 * @return Error code
 **/

error_t smtpClientWriteMultipartBody(SmtpClientContext *context,
   const void *data, size_t length, size_t *written, uint_t flags)
{
#if (SMTP_CLIENT_MIME_SUPPORT == ENABLED)
   error_t error;
   size_t n;
   size_t totalLength;

   //Make sure the SMTP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Check parameters
   if(data == NULL && length != 0)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Actual number of bytes written
   totalLength = 0;

   //Check current state
   if(context->state == SMTP_CLIENT_STATE_MULTIPART_BODY)
   {
      //Base64 encoding?
      if(context->base64Encoding)
      {
         //Send as much data as possible
         while(totalLength < length && !error)
         {
            //Any data pending in the transmit buffer?
            if(context->bufferLen < 4)
            {
               //Base64 maps a 3-byte block to 4 printable characters
               n = (SMTP_CLIENT_BUFFER_SIZE * 3) / 4;

               //Calculate the number of bytes to copy at a time
               n = MIN(n - context->bufferLen, length - totalLength);

               //The raw data must be an integral multiple of 24 bits
               if((context->bufferLen + n) > 3)
               {
                  n -= (context->bufferLen + n) % 3;
               }

               //Copy the raw data to the transmit buffer
               osMemcpy(context->buffer + context->bufferLen, data, n);

               //Advance data pointer
               data = (uint8_t *) data + n;
               //Update the length of the buffer
               context->bufferLen += n;
               //Actual number of bytes written
               totalLength += n;

               //The raw data is processed block by block
               if(context->bufferLen >= 3)
               {
                  //Encode the data with Base64 algorithm
                  base64Encode(context->buffer, context->bufferLen,
                     context->buffer, &n);

                  //Save the length of the Base64-encoded string
                  context->bufferLen = n;
                  context->bufferPos = 0;
               }
            }
            else if(context->bufferPos < context->bufferLen)
            {
               //Send more data
               error = smtpClientSendData(context,
                  context->buffer + context->bufferPos,
                  context->bufferLen - context->bufferPos, &n, 0);

               //Check status code
               if(error == NO_ERROR || error == ERROR_TIMEOUT)
               {
                  //Any data transmitted?
                  if(n > 0)
                  {
                     //Advance data pointer
                     context->bufferPos += n;
                     //Save current time
                     context->timestamp = osGetSystemTime();
                  }
               }
            }
            else
            {
               //Rewind to the beginning of the buffer
               context->bufferPos = 0;
               context->bufferLen = 0;
            }
         }
      }
      else
      {
         //Send raw data
         error = smtpClientSendData(context, data, length, &n, flags);

         //Check status code
         if(error == NO_ERROR || error == ERROR_TIMEOUT)
         {
            //Any data transmitted?
            if(n > 0)
            {
               //Actual number of bytes written
               totalLength += n;
               //Save current time
               context->timestamp = osGetSystemTime();
            }
         }
      }
   }
   else
   {
      //Invalid state
      error = ERROR_WRONG_STATE;
   }

   //Check status code
   if(error == ERROR_WOULD_BLOCK || error == ERROR_TIMEOUT)
   {
      //Check whether the timeout has elapsed
      error = smtpClientCheckTimeout(context);
   }

   //Total number of data that have been written
   if(written != NULL)
      *written = totalLength;

   //Return status code
   return error;
#else
   //MIME extension is not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Complete email sending process and wait for server's status
 * @param[in] context Pointer to the SMTP client context
 * @return Error code
 **/

error_t smtpClientCloseMailBody(SmtpClientContext *context)
{
   error_t error;

   //Make sure the SMTP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Execute SMTP command sequence
   while(!error)
   {
      //Check current state
      if(context->state == SMTP_CLIENT_STATE_MAIL_BODY)
      {
         //SMTP indicates the end of the mail data by sending a line containing
         //only a "." (refer to RFC 5321, section 3.3)
         error = smtpClientFormatCommand(context, ".", NULL);

         //Check status code
         if(!error)
         {
            //Wait for the server's response
            smtpClientChangeState(context, SMTP_CLIENT_STATE_SUB_COMMAND_1);
         }
      }
      else if(context->state == SMTP_CLIENT_STATE_SUB_COMMAND_1)
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
               //The email has been accepted by the server
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
}


/**
 * @brief Retrieve server's reply code
 * @param[in] context Pointer to the SMTP client context
 * @return SMTP reply code
 **/

uint_t smtpClientGetReplyCode(SmtpClientContext *context)
{
   uint_t replyCode;

   //Make sure the SMTP client context is valid
   if(context != NULL)
   {
      //Get server's reply code
      replyCode = context->replyCode;
   }
   else
   {
      //The SMTP client context is not valid
      replyCode = 0;
   }

   //Return SMTP reply code
   return replyCode;
}


/**
 * @brief Gracefully disconnect from the SMTP server
 * @param[in] context Pointer to the SMTP client context
 * @return Error code
 **/

error_t smtpClientDisconnect(SmtpClientContext *context)
{
   error_t error;

   //Make sure the SMTP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Execute SMTP command sequence
   while(!error)
   {
      //Check current state
      if(context->state == SMTP_CLIENT_STATE_CONNECTED)
      {
         //Format QUIT command
         error = smtpClientFormatCommand(context, "QUIT", NULL);

         //Check status code
         if(!error)
         {
            //Send QUIT command and wait for the server's response
            smtpClientChangeState(context, SMTP_CLIENT_STATE_SUB_COMMAND_1);
         }
      }
      else if(context->state == SMTP_CLIENT_STATE_SUB_COMMAND_1)
      {
         //Send QUIT command and wait for the server's response
         error = smtpClientSendCommand(context, NULL);

         //Check status code
         if(!error)
         {
            //Update SMTP client state
            smtpClientChangeState(context, SMTP_CLIENT_STATE_DISCONNECTING);
         }
      }
      else if(context->state == SMTP_CLIENT_STATE_DISCONNECTING)
      {
         //Shutdown connection
         error = smtpClientShutdownConnection(context);

         //Check status code
         if(!error)
         {
            //Close connection
            smtpClientCloseConnection(context);
            //Update SMTP client state
            smtpClientChangeState(context, SMTP_CLIENT_STATE_DISCONNECTED);
         }
      }
      else if(context->state == SMTP_CLIENT_STATE_DISCONNECTED)
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
      error = smtpClientCheckTimeout(context);
   }

   //Failed to gracefully disconnect from the SMTP server?
   if(error != NO_ERROR && error != ERROR_WOULD_BLOCK)
   {
      //Close connection
      smtpClientCloseConnection(context);
      //Update SMTP client state
      smtpClientChangeState(context, SMTP_CLIENT_STATE_DISCONNECTED);
   }

   //Return status code
   return error;
}


/**
 * @brief Close the connection with the SMTP server
 * @param[in] context Pointer to the SMTP client context
 * @return Error code
 **/

error_t smtpClientClose(SmtpClientContext *context)
{
   //Make sure the SMTP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Close connection
   smtpClientCloseConnection(context);
   //Update SMTP client state
   smtpClientChangeState(context, SMTP_CLIENT_STATE_DISCONNECTED);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Release SMTP client context
 * @param[in] context Pointer to the SMTP client context
 **/

void smtpClientDeinit(SmtpClientContext *context)
{
   //Make sure the SMTP client context is valid
   if(context != NULL)
   {
      //Close connection
      smtpClientCloseConnection(context);

#if (SMTP_CLIENT_TLS_SUPPORT == ENABLED)
      //Release TLS session state
      tlsFreeSessionState(&context->tlsSession);
#endif

      //Clear SMTP client context
      osMemset(context, 0, sizeof(SmtpClientContext));
   }
}

#endif
