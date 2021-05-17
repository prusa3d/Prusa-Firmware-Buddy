/**
 * @file ftp_client_misc.c
 * @brief Helper functions for FTP client
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
#define TRACE_LEVEL FTP_TRACE_LEVEL

//Dependencies
#include <stdlib.h>
#include <ctype.h>
#include "ftp/ftp_client.h"
#include "ftp/ftp_client_transport.h"
#include "ftp/ftp_client_misc.h"
#include "str.h"
#include "error.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (FTP_CLIENT_SUPPORT == ENABLED)


/**
 * @brief Update FTP client state
 * @param[in] context Pointer to the FTP client context
 * @param[in] newState New state to switch to
 **/

void ftpClientChangeState(FtpClientContext *context, FtpClientState newState)
{
   //Switch to the new state
   context->state = newState;

   //Save current time
   context->timestamp = osGetSystemTime();
}


/**
 * @brief Send FTP command and wait for a reply
 * @param[in] context Pointer to the FTP client context
 * @return Error code
 **/

error_t ftpClientSendCommand(FtpClientContext *context)
{
   error_t error;
   size_t n;
   bool_t more;
   char_t *reply;

   //Initialize status code
   error = NO_ERROR;

   //Point to the server's response
   reply = context->buffer;

   //Send FTP command and wait for the FTP reply to be received
   while(!error)
   {
      //Send FTP command
      if(context->bufferPos < context->commandLen)
      {
         //Send more data
         error = ftpClientWriteChannel(&context->controlChannel,
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
         else if(context->replyLen == (FTP_CLIENT_BUFFER_SIZE - 1))
            more = FALSE;
         else
            more = TRUE;

         //Receive FTP response
         if(more)
         {
            //Receive more data
            error = ftpClientReadChannel(&context->controlChannel,
               context->buffer + context->replyLen,
               FTP_CLIENT_BUFFER_SIZE - 1 - context->replyLen,
               &n, SOCKET_FLAG_BREAK_CRLF);

            //Check status code
            if(!error)
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

            //All replies begin with a three digit numeric code
            if(osIsdigit(reply[0]) &&
               osIsdigit(reply[1]) &&
               osIsdigit(reply[2]))
            {
               //A space character follows the response code for the last line
               if(reply[3] == ' ' || reply[3] == '\0')
               {
                  //Debug message
                  TRACE_DEBUG("FTP server: %s\r\n", reply);

                  //Retrieve FTP reply code
                  context->replyCode = osStrtoul(reply, NULL, 10);

                  //A valid FTP response has been received
                  break;
               }
               else
               {
                  //Ignore all intermediary lines
                  context->replyLen = 0;
               }
            }
            else
            {
               //Ignore all intermediary lines
               context->replyLen = 0;
            }
         }
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Format FTP command
 * @param[in] context Pointer to the FTP client context
 * @param[in] command NULL-terminated string containing the FTP command
 * @param[in] argument NULL-terminated string containing the argument
 * @return Error code
 **/

error_t ftpClientFormatCommand(FtpClientContext *context,
   const char_t *command, const char_t *argument)
{
   //The argument is optional
   if(argument != NULL)
   {
      //Format FTP command
      osSprintf(context->buffer, "%s %s\r\n", command, argument);
   }
   else
   {
      //Format FTP command
      osSprintf(context->buffer, "%s\r\n", command);
   }

   //Calculate the length of the FTP command
   context->commandLen = osStrlen(context->buffer);

   //Debug message
   TRACE_DEBUG("FTP client: %s", context->buffer);

   //Flush receive buffer
   context->bufferPos = 0;
   context->replyLen = 0;

   //Successful processing
   return NO_ERROR;
}



/**
 * @brief Format PORT or EPRT command
 * @param[in] context Pointer to the FTP client context
 * @param[in] ipAddr Host IP address
 * @param[in] port TCP port number
 * @return Error code
 **/

error_t ftpClientFormatPortCommand(FtpClientContext *context,
   const IpAddr *ipAddr, uint16_t port)
{
   error_t error;
   size_t n;
   char_t *p;

   //Initialize status code
   error = NO_ERROR;

#if (IPV4_SUPPORT == ENABLED)
   //IPv4 address?
   if(ipAddr->length == sizeof(Ipv4Addr))
   {
      //Format the PORT command
      n = osSprintf(context->buffer, "PORT ");

      //Append host address
      ipv4AddrToString(ipAddr->ipv4Addr, context->buffer + n);
      //Change dots to commas
      strReplaceChar(context->buffer, '.', ',');

      //Point to the end of the resulting string
      p = context->buffer + osStrlen(context->buffer);
      //Append port number
      osSprintf(p, ",%" PRIu8 ",%" PRIu8 "\r\n", MSB(port), LSB(port));
   }
   else
#endif
#if (IPV6_SUPPORT == ENABLED)
   //IPv6 address?
   if(ipAddr->length == sizeof(Ipv6Addr))
   {
      //Format the EPRT command
      n = osSprintf(context->buffer, "EPRT |2|");

      //Append host address
      ipv6AddrToString(&ipAddr->ipv6Addr, context->buffer + n);

      //Point to the end of the resulting string
      p = context->buffer + osStrlen(context->buffer);
      //Append port number
      osSprintf(p, "|%" PRIu16 "|\r\n", port);
   }
   else
#endif
   //Invalid IP address?
   {
      //Report an error
      error = ERROR_INVALID_ADDRESS;
   }

   //Check status code
   if(!error)
   {
      //Calculate the length of the FTP command
      context->commandLen = osStrlen(context->buffer);

      //Debug message
      TRACE_DEBUG("FTP client: %s", context->buffer);

      //Flush receive buffer
      context->bufferPos = 0;
      context->replyLen = 0;
   }

   //Return status code
   return error;
}


/**
 * @brief Format PASV or EPSV command
 * @param[in] context Pointer to the FTP client context
 * @return Error code
 **/

error_t ftpClientFormatPasvCommand(FtpClientContext *context)
{
   error_t error;

   //Initialize status code
   error = NO_ERROR;

#if (IPV4_SUPPORT == ENABLED)
   //IPv4 address?
   if(context->serverIpAddr.length == sizeof(Ipv4Addr))
   {
      //Format PASV command
      osStrcpy(context->buffer, "PASV\r\n");
   }
   else
#endif
#if (IPV6_SUPPORT == ENABLED)
   //IPv6 address?
   if(context->serverIpAddr.length == sizeof(Ipv6Addr))
   {
      //Format EPSV command
      osStrcpy(context->buffer, "EPSV\r\n");
   }
   else
#endif
   //Invalid IP address?
   {
      //Report an error
      error = ERROR_INVALID_ADDRESS;
   }

   //Check status code
   if(!error)
   {
      //Calculate the length of the FTP command
      context->commandLen = osStrlen(context->buffer);

      //Debug message
      TRACE_DEBUG("FTP client: %s", context->buffer);

      //Flush receive buffer
      context->bufferPos = 0;
      context->replyLen = 0;
   }

   //Return status code
   return error;
}


/**
 * @brief Parse PASV or EPSV response
 * @param[in] context Pointer to the FTP client context
 * @param[out] port The TCP port number the server is listening on
 * @return Error code
 **/

error_t ftpClientParsePasvReply(FtpClientContext *context, uint16_t *port)
{
   char_t *p;
   char_t delimiter;

#if (IPV4_SUPPORT == ENABLED)
   //IPv4 address?
   if(context->serverIpAddr.length == sizeof(Ipv4Addr))
   {
      //Delimiter character
      delimiter = ',';

      //Retrieve the low byte of the port number
      p = strrchr(context->buffer, delimiter);
      //Failed to parse the response?
      if(p == NULL)
         return ERROR_INVALID_SYNTAX;

      //Convert the resulting string
      *port = (uint16_t) osStrtoul(p + 1, NULL, 10);
      //Split the string
      *p = '\0';

      //Retrieve the high byte of the port number
      p = strrchr(context->buffer, delimiter);
      //Failed to parse the response?
      if(p == NULL)
         return ERROR_INVALID_SYNTAX;

      //Convert the resulting string
      *port |= (uint16_t) osStrtoul(p + 1, NULL, 10) << 8;
   }
   else
#endif
#if (IPV6_SUPPORT == ENABLED)
   //IPv6 address?
   if(context->serverIpAddr.length == sizeof(Ipv6Addr))
   {
      //Search for the opening parenthesis
      p = strrchr(context->buffer, '(');
      //Failed to parse the response?
      if(p == NULL || p[1] == '\0')
         return ERROR_INVALID_SYNTAX;

      //Retrieve the delimiter character
      delimiter = p[1];

      //Search for the last delimiter character
      p = strrchr(context->buffer, delimiter);
      //Failed to parse the response?
      if(p == NULL)
         return ERROR_INVALID_SYNTAX;

      //Split the string
      *p = '\0';

      //Search for the last but one delimiter character
      p = strrchr(context->buffer, delimiter);
      //Failed to parse the response?
      if(p == NULL)
         return ERROR_INVALID_SYNTAX;

      //Convert the resulting string
      *port = (uint16_t) osStrtoul(p + 1, NULL, 10);
   }
   else
#endif
   //Invalid IP address?
   {
      //Report an error
      return ERROR_INVALID_ADDRESS;
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse PWD response
 * @param[in] context Pointer to the FTP client context
 * @param[out] path Output buffer where to store the current directory
 * @param[in] maxLen Maximum number of characters the buffer can hold
 * @return Error code
 **/

error_t ftpClientParsePwdReply(FtpClientContext *context, char_t *path,
   size_t maxLen)
{
   size_t length;
   char_t *p;

   //Search for the last double quote
   p = strrchr(context->buffer, '\"');
   //Failed to parse the response?
   if(p == NULL)
      return ERROR_INVALID_SYNTAX;

   //Split the string
   *p = '\0';

   //Search for the first double quote
   p = osStrchr(context->buffer, '\"');
   //Failed to parse the response?
   if(p == NULL)
      return ERROR_INVALID_SYNTAX;

   //Retrieve the length of the working directory
   length = osStrlen(p + 1);
   //Limit the number of characters to copy
   length = MIN(length, maxLen);

   //Copy the string
   osStrncpy(path, p + 1, length);
   //Properly terminate the string with a NULL character
   path[length] = '\0';

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse directory entry
 * @param[in] line NULL-terminated string
 * @param[out] dirEntry Pointer to a directory entry
 * @return Error code
 **/

error_t ftpClientParseDirEntry(char_t *line, FtpDirEntry *dirEntry)
{
   uint_t i;
   size_t n;
   char_t *p;
   char_t *token;

   //Abbreviated months
   static const char_t months[13][4] =
   {
      "   ",
      "Jan",
      "Feb",
      "Mar",
      "Apr",
      "May",
      "Jun",
      "Jul",
      "Aug",
      "Sep",
      "Oct",
      "Nov",
      "Dec"
   };

   //Read first field
   token = osStrtok_r(line, " \t", &p);
   //Invalid directory entry?
   if(token == NULL)
      return ERROR_INVALID_SYNTAX;

   //MS-DOS listing format?
   if(osIsdigit(token[0]))
   {
      //Check modification date format
      if(osStrlen(token) == 8 && token[2] == '-' && token[5] == '-')
      {
         //The format of the date is mm-dd-yy
         dirEntry->modified.month = (uint8_t) osStrtoul(token, NULL, 10);
         dirEntry->modified.day = (uint8_t) osStrtoul(token + 3, NULL, 10);
         dirEntry->modified.year = (uint16_t) osStrtoul(token + 6, NULL, 10) + 2000;
      }
      else if(osStrlen(token) == 10 && token[2] == '/' && token[5] == '/')
      {
         //The format of the date is mm/dd/yyyy
         dirEntry->modified.month = (uint8_t) osStrtoul(token, NULL, 10);
         dirEntry->modified.day = (uint8_t) osStrtoul(token + 3, NULL, 10);
         dirEntry->modified.year = (uint16_t) osStrtoul(token + 6, NULL, 10);
      }
      else
      {
         //Invalid time format
         return ERROR_INVALID_SYNTAX;
      }

      //Read modification time
      token = osStrtok_r(NULL, " ", &p);
      //Invalid directory entry?
      if(token == NULL)
         return ERROR_INVALID_SYNTAX;

      //Check modification time format
      if(osStrlen(token) >= 5 && token[2] == ':')
      {
         //The format of the time hh:mm
         dirEntry->modified.hours = (uint8_t) osStrtoul(token, NULL, 10);
         dirEntry->modified.minutes = (uint8_t) osStrtoul(token + 3, NULL, 10);

         //The PM period covers the 12 hours from noon to midnight
         if(osStrstr(token, "PM") != NULL)
         {
            dirEntry->modified.hours += 12;
         }
      }
      else
      {
         //Invalid time format
         return ERROR_INVALID_SYNTAX;
      }

      //Read next field
      token = osStrtok_r(NULL, " ", &p);
      //Invalid directory entry?
      if(token == NULL)
         return ERROR_INVALID_SYNTAX;

      //Check whether the current entry is a directory
      if(!osStrcmp(token, "<DIR>"))
      {
         //Update attributes
         dirEntry->attributes |= FTP_FILE_ATTR_DIRECTORY;
      }
      else
      {
         //Save the size of the file
         dirEntry->size = osStrtoul(token, NULL, 10);
      }

      //Read filename field
      token = osStrtok_r(NULL, " \r\n", &p);
      //Invalid directory entry?
      if(token == NULL)
         return ERROR_INVALID_SYNTAX;

      //Retrieve the length of the filename
      n = osStrlen(token);
      //Limit the number of characters to copy
      n = MIN(n, FTP_CLIENT_MAX_FILENAME_LEN);

      //Copy the filename
      osStrncpy(dirEntry->name, token, n);
      //Properly terminate the string with a NULL character
      dirEntry->name[n] = '\0';
   }
   //Unix listing format?
   else
   {
      //Check file permissions
      if(osStrchr(token, 'd') != NULL)
      {
         dirEntry->attributes |= FTP_FILE_ATTR_DIRECTORY;
      }

      if(osStrchr(token, 'w') == NULL)
      {
         dirEntry->attributes |= FTP_FILE_ATTR_READ_ONLY;
      }

      //Read next field
      token = osStrtok_r(NULL, " ", &p);
      //Invalid directory entry?
      if(token == NULL)
         return ERROR_INVALID_SYNTAX;

      //Discard owner field
      token = osStrtok_r(NULL, " ", &p);
      //Invalid directory entry?
      if(token == NULL)
         return ERROR_INVALID_SYNTAX;

      //Discard group field
      token = osStrtok_r(NULL, " ", &p);
      //Invalid directory entry?
      if(token == NULL)
         return ERROR_INVALID_SYNTAX;

      //Read size field
      token = osStrtok_r(NULL, " ", &p);
      //Invalid directory entry?
      if(token == NULL)
         return ERROR_INVALID_SYNTAX;

      //Save the size of the file
      dirEntry->size = osStrtoul(token, NULL, 10);

      //Read modification time (month)
      token = osStrtok_r(NULL, " ", &p);
      //Invalid directory entry?
      if(token == NULL)
         return ERROR_INVALID_SYNTAX;

      //Decode the 3-letter month name
      for(i = 1; i <= 12; i++)
      {
         //Compare month name
         if(!osStrcmp(token, months[i]))
         {
            //Save month number
            dirEntry->modified.month = i;
            break;
         }
      }

      //Read modification time (day)
      token = osStrtok_r(NULL, " ", &p);
      //Invalid directory entry?
      if(token == NULL)
         return ERROR_INVALID_SYNTAX;

      //Save day number
      dirEntry->modified.day = (uint8_t) osStrtoul(token, NULL, 10);

      //Read next field
      token = osStrtok_r(NULL, " ", &p);
      //Invalid directory entry?
      if(token == NULL)
         return ERROR_INVALID_SYNTAX;

      //Check modification time format
      if(osStrlen(token) == 4)
      {
         //The format of the year is yyyy
         dirEntry->modified.year = (uint16_t) osStrtoul(token, NULL, 10);

      }
      else if(osStrlen(token) == 5)
      {
         //The format of the time hh:mm
         token[2] = '\0';
         dirEntry->modified.hours = (uint8_t) osStrtoul(token, NULL, 10);
         dirEntry->modified.minutes = (uint8_t) osStrtoul(token + 3, NULL, 10);
      }
      else
      {
         //Invalid time format
         return ERROR_INVALID_SYNTAX;
      }

      //Read filename field
      token = osStrtok_r(NULL, " \r\n", &p);
      //Invalid directory entry?
      if(token == NULL)
         return ERROR_INVALID_SYNTAX;

      //Retrieve the length of the filename
      n = osStrlen(token);
      //Limit the number of characters to copy
      n = MIN(n, FTP_CLIENT_MAX_FILENAME_LEN);

      //Copy the filename
      osStrncpy(dirEntry->name, token, n);
      //Properly terminate the string with a NULL character
      dirEntry->name[n] = '\0';
   }

   //The directory entry is valid
   return NO_ERROR;
}


/**
 * @brief Initiate data transfer
 * @param[in] context Pointer to the FTP client context
 * @param[in] direction Data transfer direction
 * @return Error code
 **/

error_t ftpClientInitDataTransfer(FtpClientContext *context, bool_t direction)
{
   error_t error;
   IpAddr ipAddr;
   uint16_t port;
   Socket *socket;

   //Initialize status code
   error = NO_ERROR;

   //Check current state
   if(context->state == FTP_CLIENT_STATE_SUB_COMMAND_2)
   {
#if (FTP_CLIENT_TLS_SUPPORT == ENABLED)
      //TLS-secured connection?
      if(context->controlChannel.tlsContext != NULL)
      {
         //A PBSZ command must be issued, but must have a parameter
         //of '0' to indicate that no buffering is taking place and
         //the data connection should not be encapsulated
         error = ftpClientFormatCommand(context, "PBSZ", "0");

         //Check status code
         if(!error)
         {
            //Send PBSZ command and wait for the server's response
            ftpClientChangeState(context, FTP_CLIENT_STATE_SUB_COMMAND_3);
         }
      }
      else
#endif
      {
         //Update FTP client state
         ftpClientChangeState(context, FTP_CLIENT_STATE_SUB_COMMAND_5);
      }
   }
   else if(context->state == FTP_CLIENT_STATE_SUB_COMMAND_3)
   {
      //Send PBSZ command and wait for the server's response
      error = ftpClientSendCommand(context);

      //Check status code
      if(!error)
      {
         //Check FTP response code
         if(FTP_REPLY_CODE_2YZ(context->replyCode))
         {
            //If the data connection security level is 'Private', then a TLS
            //negotiation must take place on the data connection
            error = ftpClientFormatCommand(context, "PROT", "P");

            //Check status code
            if(!error)
            {
               //Send PROT command and wait for the server's response
               ftpClientChangeState(context, FTP_CLIENT_STATE_SUB_COMMAND_4);
            }
         }
         else
         {
            //Report an error
            error = ERROR_UNEXPECTED_RESPONSE;
         }
      }
   }
   else if(context->state == FTP_CLIENT_STATE_SUB_COMMAND_4)
   {
      //Send PROT command and wait for the server's response
      error = ftpClientSendCommand(context);

      //Check status code
      if(!error)
      {
         //Check FTP response code
         if(FTP_REPLY_CODE_2YZ(context->replyCode))
         {
            //Update FTP client state
            ftpClientChangeState(context, FTP_CLIENT_STATE_SUB_COMMAND_5);
         }
         else
         {
            //Report an error
            error = ERROR_UNEXPECTED_RESPONSE;
         }
      }
   }
   else if(context->state == FTP_CLIENT_STATE_SUB_COMMAND_5)
   {
      //Check data transfer direction
      if(direction)
      {
         //Open data socket
         error = ftpClientOpenChannel(context, &context->dataChannel,
            FTP_CLIENT_MAX_TCP_BUFFER_SIZE, FTP_CLIENT_MIN_TCP_BUFFER_SIZE);
      }
      else
      {
         //Open data socket
         error = ftpClientOpenChannel(context, &context->dataChannel,
            FTP_CLIENT_MIN_TCP_BUFFER_SIZE, FTP_CLIENT_MAX_TCP_BUFFER_SIZE);
      }

      //Check status code
      if(!error)
      {
         //Check transfer mode
         if(!context->passiveMode)
         {
            //Place the data socket in the listening state
            error = socketListen(context->dataChannel.socket, 1);

            //Check status code
            if(!error)
            {
               //Retrieve local IP address
               error = socketGetLocalAddr(context->controlChannel.socket,
                  &ipAddr, NULL);
            }

            //Check status code
            if(!error)
            {
               //Retrieve local port number
               error = socketGetLocalAddr(context->dataChannel.socket,
                  NULL, &port);
            }

            //Check status code
            if(!error)
            {
               //Set the port to be used in data connection
               error = ftpClientFormatPortCommand(context, &ipAddr, port);
            }

            //Check status code
            if(!error)
            {
               //Send PORT command and wait for the server's response
               ftpClientChangeState(context, FTP_CLIENT_STATE_SUB_COMMAND_6);
            }
         }
         else
         {
            //Enter passive mode
            error = ftpClientFormatPasvCommand(context);

            //Check status code
            if(!error)
            {
               //Send PASV command and wait for the server's response
               ftpClientChangeState(context, FTP_CLIENT_STATE_SUB_COMMAND_6);
            }
         }
      }
   }
   else if(context->state == FTP_CLIENT_STATE_SUB_COMMAND_6)
   {
      //Send PORT/PASV command and wait for the server's response
      error = ftpClientSendCommand(context);

      //Check status code
      if(!error)
      {
         //Check FTP response code
         if(FTP_REPLY_CODE_2YZ(context->replyCode))
         {
            //Check transfer mode
            if(!context->passiveMode)
            {
               //Update FTP client state
               ftpClientChangeState(context, FTP_CLIENT_STATE_SUB_COMMAND_7);
            }
            else
            {
               //Parse server's response
               error = ftpClientParsePasvReply(context, &context->serverPort);

               //Check status code
               if(!error)
               {
                  //Establish data connection
                  ftpClientChangeState(context, FTP_CLIENT_STATE_CONNECTING_TCP);
               }
            }
         }
         else
         {
            //Report an error
            error = ERROR_UNEXPECTED_RESPONSE;
         }
      }
   }
   else if(context->state == FTP_CLIENT_STATE_CONNECTING_TCP)
   {
      //Establish data connection
      error = socketConnect(context->dataChannel.socket,
         &context->serverIpAddr, context->serverPort);

      //Check status code
      if(!error)
      {
         //Update FTP client state
         ftpClientChangeState(context, FTP_CLIENT_STATE_SUB_COMMAND_7);
      }
   }
   else if(context->state == FTP_CLIENT_STATE_SUB_COMMAND_8)
   {
      //Send STOR/APPE/RETR/LIST command and wait for the server's response
      error = ftpClientSendCommand(context);

      //Check status code
      if(!error)
      {
         //Check FTP response code
         if(FTP_REPLY_CODE_1YZ(context->replyCode))
         {
            //Check transfer mode
            if(!context->passiveMode)
            {
               //Wait for the server to connect back to the client's data port
               ftpClientChangeState(context, FTP_CLIENT_STATE_ACCEPTING_TCP);
            }
            else
            {
#if (FTP_CLIENT_TLS_SUPPORT == ENABLED)
               //TLS-secured connection?
               if(context->controlChannel.tlsContext != NULL)
               {
                  //Check data transfer direction
                  if(direction)
                  {
                     //TLS initialization
                     error = ftpClientOpenSecureChannel(context,
                        &context->dataChannel, FTP_CLIENT_TLS_TX_BUFFER_SIZE,
                        FTP_CLIENT_MIN_TLS_RX_BUFFER_SIZE);
                  }
                  else
                  {
                     //TLS initialization
                     error = ftpClientOpenSecureChannel(context,
                        &context->dataChannel, FTP_CLIENT_TLS_TX_BUFFER_SIZE,
                        FTP_CLIENT_MAX_TLS_RX_BUFFER_SIZE);
                  }

                  //Check status code
                  if(!error)
                  {
                     //Perform TLS handshake
                     ftpClientChangeState(context, FTP_CLIENT_STATE_CONNECTING_TLS);
                  }
               }
               else
#endif
               {
                  //Update FTP client state
                  ftpClientChangeState(context, FTP_CLIENT_STATE_SUB_COMMAND_9);
               }
            }
         }
         else
         {
            //Report an error
            error = ERROR_UNEXPECTED_RESPONSE;
         }
      }
   }
   else if(context->state == FTP_CLIENT_STATE_ACCEPTING_TCP)
   {
      //Wait for the server to connect back to the client's data port
      socket = socketAccept(context->dataChannel.socket, NULL, NULL);

      //Valid socket handle?
      if(socket != NULL)
      {
         //Close the listening socket
         socketClose(context->dataChannel.socket);
         //Save socket handle
         context->dataChannel.socket = socket;

         //Set timeout
         error = socketSetTimeout(context->dataChannel.socket,
            context->timeout);

#if (FTP_CLIENT_TLS_SUPPORT == ENABLED)
         //TLS-secured connection?
         if(context->controlChannel.tlsContext != NULL)
         {
            //Check status code
            if(!error)
            {
               //Check data transfer direction
               if(direction)
               {
                  //TLS initialization
                  error = ftpClientOpenSecureChannel(context,
                     &context->dataChannel, FTP_CLIENT_TLS_TX_BUFFER_SIZE,
                     FTP_CLIENT_MIN_TLS_RX_BUFFER_SIZE);
               }
               else
               {
                  //TLS initialization
                  error = ftpClientOpenSecureChannel(context,
                     &context->dataChannel, FTP_CLIENT_TLS_TX_BUFFER_SIZE,
                     FTP_CLIENT_MAX_TLS_RX_BUFFER_SIZE);
               }
            }

            //Check status code
            if(!error)
            {
               //Perform TLS handshake
               ftpClientChangeState(context, FTP_CLIENT_STATE_CONNECTING_TLS);
            }
         }
         else
#endif
         {
            //Update FTP client state
            ftpClientChangeState(context, FTP_CLIENT_STATE_SUB_COMMAND_9);
         }
      }
      else
      {
         //Report an error
         error = ERROR_WOULD_BLOCK;
      }
   }
   else if(context->state == FTP_CLIENT_STATE_CONNECTING_TLS)
   {
      //Perform TLS handshake
      error = ftpClientEstablishSecureChannel(&context->dataChannel);

      //Check status code
      if(!error)
      {
         //The content of the file can be transferred via the data connection
         ftpClientChangeState(context, FTP_CLIENT_STATE_SUB_COMMAND_9);
      }
   }
   else
   {
      //Invalid state
      error = ERROR_WRONG_STATE;
   }

   //Return status code
   return error;
}


/**
 * @brief Terminate data transfer
 * @param[in] context Pointer to the FTP client context
 * @return Error code
 **/

error_t ftpClientTerminateDataTransfer(FtpClientContext *context)
{
   error_t error;

   //Initialize status code
   error = NO_ERROR;

   //Execute FTP command sequence
   while(!error)
   {
      //Check current state
      if(context->state == FTP_CLIENT_STATE_WRITING_DATA ||
         context->state == FTP_CLIENT_STATE_READING_DATA)
      {
         //Update FTP client state
         ftpClientChangeState(context, FTP_CLIENT_STATE_DISCONNECTING_1);
      }
      else if(context->state == FTP_CLIENT_STATE_DISCONNECTING_1)
      {
         //Shutdown data connection
         error = ftpClientShutdownChannel(&context->dataChannel);

         //Check status code
         if(!error)
         {
            //Close data connection
            ftpClientCloseChannel(&context->dataChannel);

            //Flush buffer
            context->bufferPos = 0;
            context->commandLen = 0;
            context->replyLen = 0;

            //Wait for the transfer status
            ftpClientChangeState(context, FTP_CLIENT_STATE_SUB_COMMAND_1);
         }
      }
      else if(context->state == FTP_CLIENT_STATE_SUB_COMMAND_1)
      {
         //Wait for the transfer status
         error = ftpClientSendCommand(context);

         //Check status code
         if(!error)
         {
            //Check FTP response code
            if(FTP_REPLY_CODE_2YZ(context->replyCode))
            {
               //Update FTP client state
               ftpClientChangeState(context, FTP_CLIENT_STATE_CONNECTED);
            }
            else
            {
               //Report an error
               error = ERROR_UNEXPECTED_RESPONSE;
            }
         }
      }
      else if(context->state == FTP_CLIENT_STATE_CONNECTED)
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
      error = ftpClientCheckTimeout(context);
   }

   //Failed to close file?
   if(error != NO_ERROR && error != ERROR_WOULD_BLOCK)
   {
      //Close data connection
      ftpClientCloseChannel(&context->dataChannel);
      //Update FTP client state
      ftpClientChangeState(context, FTP_CLIENT_STATE_CONNECTED);
   }

   //Return status code
   return error;
}


/**
 * @brief Determine whether a timeout error has occurred
 * @param[in] context Pointer to the FTP client context
 * @return Error code
 **/

error_t ftpClientCheckTimeout(FtpClientContext *context)
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
