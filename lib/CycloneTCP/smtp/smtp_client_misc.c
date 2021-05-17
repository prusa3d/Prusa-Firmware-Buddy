/**
 * @file smtp_client_misc.c
 * @brief Helper functions for SMTP client
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
#include <stdlib.h>
#include <ctype.h>
#include "core/net.h"
#include "smtp/smtp_client.h"
#include "smtp/smtp_client_transport.h"
#include "smtp/smtp_client_misc.h"
#include "str.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (SMTP_CLIENT_SUPPORT == ENABLED)


/**
 * @brief Update SMTP client state
 * @param[in] context Pointer to the SMTP client context
 * @param[in] newState New state to switch to
 **/

void smtpClientChangeState(SmtpClientContext *context,
   SmtpClientState newState)
{
   //Switch to the new state
   context->state = newState;

   //Save current time
   context->timestamp = osGetSystemTime();
}


/**
 * @brief Send SMTP command and wait for a reply
 * @param[in] context Pointer to the SMTP client context
 * @param[in] callback Optional callback to parse each line of the reply
 * @return Error code
 **/

error_t smtpClientSendCommand(SmtpClientContext *context,
   SmtpClientReplyCallback callback)
{
   error_t error;
   size_t n;
   bool_t more;
   char_t *reply;

   //Initialize status code
   error = NO_ERROR;

   //Point to the server's response
   reply = context->buffer;

   //Send SMTP command and wait for the SMTP reply to be received
   while(!error)
   {
      //Send SMTP command
      if(context->bufferPos < context->commandLen)
      {
         //Send more data
         error = smtpClientSendData(context,
            context->buffer + context->bufferPos,
            context->commandLen - context->bufferPos, &n, 0);

         //Check status code
         if(error == NO_ERROR || error == ERROR_TIMEOUT)
         {
            //Advance data pointer
            context->bufferPos += n;
         }
      }
      else
      {
         //Determine whether more data should be collected
         if(context->replyLen != 0 && reply[context->replyLen - 1] == '\n')
            more = FALSE;
         else if(context->replyLen == (SMTP_CLIENT_BUFFER_SIZE - 1))
            more = FALSE;
         else
            more = TRUE;

         //Receive SMTP response
         if(more)
         {
            //Receive more data
            error = smtpClientReceiveData(context,
               context->buffer + context->replyLen,
               SMTP_CLIENT_BUFFER_SIZE - 1 - context->replyLen,
               &n, SOCKET_FLAG_BREAK_CRLF);

            //Check status code
            if(error == NO_ERROR)
            {
               //Advance data pointer
               context->replyLen += n;
            }
         }
         else
         {
            //Properly terminate the response with a NULL character
            reply[context->replyLen] = '\0';

            //Remove trailing whitespace from the response
            strRemoveTrailingSpace(reply);

            //Debug message
            TRACE_DEBUG("SMTP server: %s\r\n", reply);

            //All replies begin with a three digit numeric code
            if(osIsdigit(reply[0]) &&
               osIsdigit(reply[1]) &&
               osIsdigit(reply[2]))
            {
               //A space character follows the response code for the last line
               if(reply[3] == ' ' || reply[3] == '\0')
               {
                  //Retrieve SMTP reply code
                  context->replyCode = osStrtoul(reply, NULL, 10);

                  //A valid SMTP response has been received
                  break;
               }
               else
               {
                  //Any callback function defined?
                  if(callback)
                  {
                     //Parse intermediary line
                     error = callback(context, reply);
                  }

                  //Flush receive buffer
                  context->replyLen = 0;
               }
            }
            else
            {
               //Ignore incorrectly formatted lines
               context->replyLen = 0;
            }
         }
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Format SMTP command
 * @param[in] context Pointer to the SMTP client context
 * @param[in] command NULL-terminated string containing the SMTP command
 * @param[in] argument NULL-terminated string containing the argument
 * @return Error code
 **/

error_t smtpClientFormatCommand(SmtpClientContext *context,
   const char_t *command, const char_t *argument)
{
   //Check SMTP command name
   if(!osStrcasecmp(command, "MAIL FROM") ||
      !osStrcasecmp(command, "RCPT TO"))
   {
      //Check whether the address is valid
      if(argument != NULL)
      {
         //Format MAIL FROM or RCPT TO command
         osSprintf(context->buffer, "%s: <%s>\r\n", command, argument);
      }
      else
      {
         //A null return path is accepted
         osSprintf(context->buffer, "%s: <>\r\n", command);
      }

      //Debug message
      TRACE_DEBUG("SMTP client: %s", context->buffer);
   }
   else if(!osStrcasecmp(command, "."))
   {
      //SMTP indicates the end of the mail data by sending a line containing
      //only a "." (refer to RFC 5321, section 3.3)
      osSprintf(context->buffer, "\r\n.\r\n");

      //Debug message
      TRACE_DEBUG("%s", context->buffer);
   }
   else
   {
      //The argument is optional
      if(argument != NULL)
      {
         //Format SMTP command
         osSprintf(context->buffer, "%s %s\r\n", command, argument);
      }
      else
      {
         //Format SMTP command
         osSprintf(context->buffer, "%s\r\n", command);
      }

      //Debug message
      TRACE_DEBUG("SMTP client: %s", context->buffer);
   }

   //Calculate the length of the SMTP command
   context->commandLen = osStrlen(context->buffer);

   //Flush receive buffer
   context->bufferPos = 0;
   context->replyLen = 0;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse EHLO response
 * @param[in] context SMTP client context
 * @param[in] replyLine Response line
 * @return Error code
 **/

error_t smtpClientParseEhloReply(SmtpClientContext *context,
   char_t *replyLine)
{
   char_t *p;
   char_t *token;

   //The line must be at least 4 characters long
   if(osStrlen(replyLine) < 4)
      return ERROR_INVALID_SYNTAX;

   //Skip the response code and the separator
   replyLine += 4;

   //Get the first keyword
   token = osStrtok_r(replyLine, " ", &p);
   //Check whether the response line is empty
   if(token == NULL)
      return ERROR_INVALID_SYNTAX;

   //The response to EHLO is a multiline reply. Each line of the response
   //contains a keyword
   if(!osStrcasecmp(token, "STARTTLS"))
   {
      //The STARTTLS keyword is used to tell the SMTP client that the
      //SMTP server allows use of TLS
      context->startTlsSupported = TRUE;
   }
   else if(!osStrcasecmp(token, "AUTH"))
   {
      //The AUTH keyword contains a space-separated list of names of
      //available authentication mechanisms
      token = osStrtok_r(NULL, " ", &p);

      //Parse the list of keywords
      while(token != NULL)
      {
         //Check the name of the authentication mechanism
         if(!osStrcasecmp(token, "LOGIN"))
         {
            //LOGIN authentication mechanism is supported
            context->authLoginSupported = TRUE;
         }
         else if(!osStrcasecmp(token, "PLAIN"))
         {
            //PLAIN authentication mechanism is supported
            context->authPlainSupported = TRUE;
         }
         else if(!osStrcasecmp(token, "CRAM-MD5"))
         {
            //CRAM-MD5 authentication mechanism is supported
            context->authCramMd5Supported = TRUE;
         }
         else
         {
            //Unknown authentication mechanism
         }

         //Get the next keyword
         token = osStrtok_r(NULL, " ", &p);
      }
   }
   else
   {
      //Discard unknown keywords
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Format email header
 * @param[in] context Pointer to the SMTP client context
 * @param[in] from Email address of the sender
 * @param[in] recipients Email addresses of the recipients
 * @param[in] numRecipients Number of email addresses in the list
 * @param[in] subject NULL-terminated string containing the email subject
 * @return Error code
 **/

error_t smtpClientFormatMailHeader(SmtpClientContext *context,
   const SmtpMailAddr *from, const SmtpMailAddr *recipients,
   uint_t numRecipients, const char_t *subject)
{
   char_t *p;
   uint_t i;
   uint_t type;
   bool_t first;

   //Point to the buffer
   p = context->buffer;

   //Valid sender address?
   if(from->addr != NULL)
   {
      //Valid friendly name?
      if(from->name && from->name[0] != '\0')
      {
         //A friendly name may be associated with the sender address
         p += osSprintf(p, "From: \"%s\" <%s>\r\n", from->name, from->addr);
      }
      else
      {
         //Format sender address
         p += osSprintf(p, "From: %s\r\n", from->addr);
      }
   }

   //Process TO, CC and BCC recipients
   for(type = SMTP_ADDR_TYPE_TO; type <= SMTP_ADDR_TYPE_BCC; type++)
   {
      //Loop through the list of recipients
      for(first = TRUE, i = 0; i < numRecipients; i++)
      {
         //Ensure the current email address is valid
         if(recipients[i].addr != NULL)
         {
            //Check recipient type
            if(recipients[i].type == type)
            {
               //The first item of the list requires special handling
               if(first)
               {
                  //Check recipient type
                  if(type == SMTP_ADDR_TYPE_TO)
                  {
                     //List of recipients
                     p += osSprintf(p, "To: ");
                  }
                  else if(type == SMTP_ADDR_TYPE_CC)
                  {
                     //List of recipients which should get a carbon copy (CC)
                     //of the message
                     p += osSprintf(p, "Cc: ");
                  }
                  else if(type == SMTP_ADDR_TYPE_BCC)
                  {
                     //List of recipients which should get a blind carbon copy
                     //(BCC) of the message
                     p += osSprintf(p, "Bcc: ");
                  }
                  else
                  {
                     //Invalid recipient type
                     return ERROR_INVALID_PARAMETER;
                  }
               }
               else
               {
                  //The addresses are comma-separated
                  p += osSprintf(p, ", ");
               }

               //Valid friendly name?
               if(recipients[i].name && recipients[i].name[0] != '\0')
               {
                  //A friendly name may be associated with the address
                  p += osSprintf(p, "\"%s\" <%s>", recipients[i].name,
                     recipients[i].addr);
               }
               else
               {
                  //Add the email address to the list of recipients
                  p += osSprintf(p, "%s", recipients[i].addr);
               }

               //The current recipient is valid
               first = FALSE;
            }
         }
      }

      //Any recipients found?
      if(!first)
      {
         //Terminate the line with a CRLF sequence
         p += osSprintf(p, "\r\n");
      }
   }

   //Valid subject?
   if(subject != NULL && subject[0] != '\0')
   {
      //Format email subject
      p += osSprintf(p, "Subject: %s\r\n", subject);
   }

#if (SMTP_CLIENT_MIME_SUPPORT == ENABLED)
   //Valid content type?
   if(context->contentType[0] != '\0')
   {
      //The presence of this header field indicates the message is
      //MIME-formatted
      p += osSprintf(p, "MIME-Version: 1.0\r\n");

      //Check whether multipart encoding is being used
      if(!osStrncasecmp(context->contentType, "multipart/", 10))
      {
         //This Content-Type header field defines the boundary string
         p += osSprintf(p, "Content-Type: %s; boundary=%s\r\n",
            context->contentType, context->boundary);
      }
      else
      {
         //This Content-Type header field indicates the media type of the
         //message content, consisting of a type and subtype
         p += osSprintf(p, "Content-Type: %s\r\n", context->contentType);
      }
   }
#endif

   //The header and the body are separated by an empty line
   osSprintf(p, "\r\n");

   //Debug message
   TRACE_DEBUG("%s", context->buffer);

   //Save the length of the header
   context->bufferLen = osStrlen(context->buffer);
   context->bufferPos = 0;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Format multipart header
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

error_t smtpClientFormatMultipartHeader(SmtpClientContext *context,
   const char_t *filename, const char_t *contentType,
   const char_t *contentTransferEncoding, bool_t last)
{
#if (SMTP_CLIENT_MIME_SUPPORT == ENABLED)
   char_t *p;

   //Point to the buffer
   p = context->buffer;

   //Check whether the multipart header is the final one
   if(!last)
   {
      //The encapsulation boundary is defined as a line consisting entirely
      //of two hyphen characters followed by the boundary parameter value
      //from the Content-Type header field. The encapsulation boundary must
      //occur at the beginning of a line
      p += osSprintf(p, "\r\n--%s\r\n", context->boundary);

      //Valid file name?
      if(filename != NULL && filename[0] != '\0')
      {
         //The Content-Disposition header field specifies the presentation style
         p += osSprintf(p, "Content-Disposition: attachment; filename=\"%s\"\r\n",
            filename);
      }

      //Valid content type?
      if(contentType != NULL && contentType[0] != '\0')
      {
         //This Content-Type header field indicates the media type of the
         //message content, consisting of a type and subtype
         p += osSprintf(p, "Content-Type: %s\r\n", contentType);
      }

      //Valid content transfer encoding?
      if(contentTransferEncoding != NULL && contentTransferEncoding[0] != '\0')
      {
         //The Content-Transfer-Encoding header can be used for representing
         //binary data in formats other than ASCII text format
         p += osSprintf(p, "Content-Transfer-Encoding: %s\r\n",
            contentTransferEncoding);

         //Base64 encoding?
         if(!osStrcasecmp(contentTransferEncoding, "base64"))
         {
            context->base64Encoding = TRUE;
         }
      }
   }
   else
   {
      //The encapsulation boundary following the last body part is a
      //distinguished delimiter that indicates that no further body parts
      //will follow. Such a delimiter is identical to the previous
      //delimiters, with the addition of two more hyphens at the end of
      //the line
      p += osSprintf(p, "\r\n--%s--\r\n", context->boundary);
   }

   //Terminate the multipart header with an empty line
   osSprintf(p, "\r\n");

   //Debug message
   TRACE_DEBUG("%s", context->buffer);

   //Save the length of the header
   context->bufferLen = osStrlen(context->buffer);
   context->bufferPos = 0;

   //Successful processing
   return NO_ERROR;
#else
   //MIME extension is not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Determine whether a timeout error has occurred
 * @param[in] context Pointer to the SMTP client context
 * @return Error code
 **/

error_t smtpClientCheckTimeout(SmtpClientContext *context)
{
#if (NET_RTOS_SUPPORT == DISABLED)
   error_t error;
   systime_t time;

   //Get current time
   time = osGetSystemTime();

   //Check whether the timeout has elapsed
   if(timeCompare(time, context->timestamp + context->timeout) >= 0)
   {
      //Report a timeout error
      error = ERROR_TIMEOUT;
   }
   else
   {
      //The operation would block
      error = ERROR_WOULD_BLOCK;
   }

   //Return status code
   return error;
#else
   //Report a timeout error
   return ERROR_TIMEOUT;
#endif
}

#endif
