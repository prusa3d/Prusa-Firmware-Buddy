/**
 * @file ftp_client.c
 * @brief FTP client (File Transfer Protocol)
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
 * File Transfer Protocol (FTP) is a standard network protocol used to
 * transfer files from one host to another host over a TCP-based network.
 * Refer to the following RFCs for complete details:
 * - RFC 959: File Transfer Protocol (FTP)
 * - RFC 2428: FTP Extensions for IPv6 and NATs
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
 * @brief Initialize FTP client context
 * @param[in] context Pointer to the FTP client context
 * @return Error code
 **/

error_t ftpClientInit(FtpClientContext *context)
{
#if (FTP_CLIENT_TLS_SUPPORT == ENABLED)
   error_t error;
#endif

   //Make sure the FTP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Clear FTP client context
   osMemset(context, 0, sizeof(FtpClientContext));

#if (FTP_CLIENT_TLS_SUPPORT == ENABLED)
   //Initialize TLS session state
   error = tlsInitSessionState(&context->tlsSession);
   //Any error to report?
   if(error)
      return error;
#endif

   //Initialize FTP client state
   context->state = FTP_CLIENT_STATE_DISCONNECTED;
   //Default timeout
   context->timeout = FTP_CLIENT_DEFAULT_TIMEOUT;

   //Successful initialization
   return NO_ERROR;
}


#if (FTP_CLIENT_TLS_SUPPORT == ENABLED)

/**
 * @brief Register TLS initialization callback function
 * @param[in] context Pointer to the FTP client context
 * @param[in] callback TLS initialization callback function
 * @return Error code
 **/

error_t ftpClientRegisterTlsInitCallback(FtpClientContext *context,
   FtpClientTlsInitCallback callback)
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
 * @param[in] context Pointer to the FTP client context
 * @param[in] timeout Timeout value, in milliseconds
 * @return Error code
 **/

error_t ftpClientSetTimeout(FtpClientContext *context, systime_t timeout)
{
   //Make sure the FTP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Save timeout value
   context->timeout = timeout;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Bind the FTP client to a particular network interface
 * @param[in] context Pointer to the FTP client context
 * @param[in] interface Network interface to be used
 * @return Error code
 **/

error_t ftpClientBindToInterface(FtpClientContext *context,
   NetInterface *interface)
{
   //Make sure the FTP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Explicitly associate the FTP client with the specified interface
   context->interface = interface;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Establish a connection with the specified FTP server
 * @param[in] context Pointer to the FTP client context
 * @param[in] serverIpAddr IP address of the FTP server to connect to
 * @param[in] serverPort Port number
 * @param[in] mode FTP connection mode
 * @return Error code
 **/

error_t ftpClientConnect(FtpClientContext *context,
   const IpAddr *serverIpAddr, uint16_t serverPort, uint_t mode)
{
   error_t error;

   //Check parameters
   if(context == NULL || serverIpAddr == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Establish connection with the FTP server
   while(!error)
   {
      //Check current state
      if(context->state == FTP_CLIENT_STATE_DISCONNECTED)
      {
         //Save the IP address of the FTP server
         context->serverIpAddr = *serverIpAddr;

         //Use passive mode?
         context->passiveMode = (mode & FTP_MODE_PASSIVE) ? TRUE : FALSE;

         //Open control socket
         error = ftpClientOpenChannel(context, &context->controlChannel,
            FTP_CLIENT_MIN_TCP_BUFFER_SIZE, FTP_CLIENT_MIN_TCP_BUFFER_SIZE);

         //Check status code
         if(!error)
         {
            //Establish TCP connection
            ftpClientChangeState(context, FTP_CLIENT_STATE_CONNECTING_TCP);
         }
      }
      else if(context->state == FTP_CLIENT_STATE_CONNECTING_TCP)
      {
         //Establish TCP connection
         error = socketConnect(context->controlChannel.socket, serverIpAddr,
            serverPort);

         //Check status code
         if(!error)
         {
            //Implicit FTPS?
            if((mode & FTP_MODE_IMPLICIT_TLS) != 0)
            {
               //TLS initialization
               error = ftpClientOpenSecureChannel(context,
                  &context->controlChannel, FTP_CLIENT_TLS_TX_BUFFER_SIZE,
                  FTP_CLIENT_MIN_TLS_RX_BUFFER_SIZE);

               //Check status code
               if(!error)
               {
                  //Perform TLS handshake
                  ftpClientChangeState(context, FTP_CLIENT_STATE_CONNECTING_TLS);
               }
            }
            else
            {
               //Flush buffer
               context->bufferPos = 0;
               context->commandLen = 0;
               context->replyLen = 0;

               //Wait for the connection greeting reply
               ftpClientChangeState(context, FTP_CLIENT_STATE_SUB_COMMAND_1);
            }
         }
      }
      else if(context->state == FTP_CLIENT_STATE_CONNECTING_TLS)
      {
         //Perform TLS handshake
         error = ftpClientEstablishSecureChannel(&context->controlChannel);

         //Check status code
         if(!error)
         {
            //Implicit FTPS?
            if((mode & FTP_MODE_IMPLICIT_TLS) != 0)
            {
               //Flush buffer
               context->bufferPos = 0;
               context->commandLen = 0;
               context->replyLen = 0;

               //Wait for the connection greeting reply
               ftpClientChangeState(context, FTP_CLIENT_STATE_SUB_COMMAND_1);
            }
            else
            {
               //The FTP client is connected
               ftpClientChangeState(context, FTP_CLIENT_STATE_CONNECTED);
            }
         }
      }
      else if(context->state == FTP_CLIENT_STATE_SUB_COMMAND_1)
      {
         //Wait for the connection greeting reply
         error = ftpClientSendCommand(context);

         //Check status code
         if(!error)
         {
            //Check FTP response code
            if(FTP_REPLY_CODE_2YZ(context->replyCode))
            {
               //Explicit FTPS?
               if((mode & FTP_MODE_EXPLICIT_TLS) != 0)
               {
                  //Format AUTH TLS command
                  error = ftpClientFormatCommand(context, "AUTH TLS", NULL);

                  //Check status code
                  if(!error)
                  {
                     //Send AUTH TLS command and wait for the server's response
                     ftpClientChangeState(context, FTP_CLIENT_STATE_SUB_COMMAND_2);
                  }
               }
               else
               {
                  //The FTP client is connected
                  ftpClientChangeState(context, FTP_CLIENT_STATE_CONNECTED);
               }
            }
            else
            {
               //Report an error
               error = ERROR_UNEXPECTED_RESPONSE;
            }
         }
      }
      else if(context->state == FTP_CLIENT_STATE_SUB_COMMAND_2)
      {
         //Send AUTH TLS command and wait for the server's response
         error = ftpClientSendCommand(context);

         //Check status code
         if(!error)
         {
            //Check FTP response code
            if(FTP_REPLY_CODE_2YZ(context->replyCode))
            {
               //TLS initialization
               error = ftpClientOpenSecureChannel(context,
                  &context->controlChannel, FTP_CLIENT_TLS_TX_BUFFER_SIZE,
                  FTP_CLIENT_MIN_TLS_RX_BUFFER_SIZE);

               //Check status code
               if(!error)
               {
                  //Perform TLS handshake
                  ftpClientChangeState(context, FTP_CLIENT_STATE_CONNECTING_TLS);
               }
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
         //The FTP client is connected
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

   //Failed to establish connection with the FTP server?
   if(error != NO_ERROR && error != ERROR_WOULD_BLOCK)
   {
      //Clean up side effects
      ftpClientCloseChannel(&context->controlChannel);
      //Update FTP client state
      ftpClientChangeState(context, FTP_CLIENT_STATE_DISCONNECTED);
   }

   //Return status code
   return error;
}


/**
 * @brief Login to the FTP server using the provided user name and password
 * @param[in] context Pointer to the FTP client context
 * @param[in] username NULL-terminated string containing the user name
 * @param[in] password NULL-terminated string containing the user's password
 * @return Error code
 **/

error_t ftpClientLogin(FtpClientContext *context, const char_t *username,
   const char_t *password)
{
   //The USER, PASS and ACCT commands specify access control identifiers
   return ftpClientLoginEx(context, username, password, "");
}


/**
 * @brief Login to the FTP server using user name, password and account
 * @param[in] context Pointer to the FTP client context
 * @param[in] username NULL-terminated string containing the user name
 * @param[in] password NULL-terminated string containing the user's password
 * @param[in] account NULL-terminated string containing the user's account
 * @return Error code
 **/

error_t ftpClientLoginEx(FtpClientContext *context, const char_t *username,
   const char_t *password, const char_t *account)
{
   error_t error;

   //Check parameters
   if(context == NULL || username == NULL || password == NULL || account == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Execute FTP command sequence
   while(!error)
   {
      //Check current state
      if(context->state == FTP_CLIENT_STATE_CONNECTED)
      {
         //Format USER command
         error = ftpClientFormatCommand(context, "USER", username);

         //Check status code
         if(!error)
         {
            //Send USER command and wait for the server's response
            ftpClientChangeState(context, FTP_CLIENT_STATE_SUB_COMMAND_1);
         }
      }
      else if(context->state == FTP_CLIENT_STATE_SUB_COMMAND_1)
      {
         //Send USER command and wait for the server's response
         error = ftpClientSendCommand(context);

         //Check status code
         if(!error)
         {
            //Check FTP response code
            if(FTP_REPLY_CODE_2YZ(context->replyCode))
            {
               //Update FTP client state
               ftpClientChangeState(context, FTP_CLIENT_STATE_CONNECTED);
               //Successful user identification
               break;
            }
            else if(FTP_REPLY_CODE_3YZ(context->replyCode))
            {
               //Format PASS command
               error = ftpClientFormatCommand(context, "PASS", password);

               //Check status code
               if(!error)
               {
                  //Send PASS command and wait for the server's response
                  ftpClientChangeState(context, FTP_CLIENT_STATE_SUB_COMMAND_2);
               }
            }
            else
            {
               //Update FTP client state
               ftpClientChangeState(context, FTP_CLIENT_STATE_CONNECTED);
               //Report an error
               error = ERROR_UNEXPECTED_RESPONSE;
            }
         }
      }
      else if(context->state == FTP_CLIENT_STATE_SUB_COMMAND_2)
      {
         //Send PASS command and wait for the server's response
         error = ftpClientSendCommand(context);

         //Check status code
         if(!error)
         {
            //Check FTP response code
            if(FTP_REPLY_CODE_2YZ(context->replyCode))
            {
               //Update FTP client state
               ftpClientChangeState(context, FTP_CLIENT_STATE_CONNECTED);
               //Successful user identification
               break;
            }
            else if(FTP_REPLY_CODE_3YZ(context->replyCode))
            {
               //Format ACCT command
               error = ftpClientFormatCommand(context, "ACCT", account);

               //Check status code
               if(!error)
               {
                  //Send ACCT command and wait for the server's response
                  ftpClientChangeState(context, FTP_CLIENT_STATE_SUB_COMMAND_3);
               }
            }
            else
            {
               //Update FTP client state
               ftpClientChangeState(context, FTP_CLIENT_STATE_CONNECTED);
               //Report an error
               error = ERROR_UNEXPECTED_RESPONSE;
            }
         }
      }
      else if(context->state == FTP_CLIENT_STATE_SUB_COMMAND_3)
      {
         //Send ACCT command and wait for the server's response
         error = ftpClientSendCommand(context);

         //Check status code
         if(!error)
         {
            //Check FTP response code
            if(!FTP_REPLY_CODE_2YZ(context->replyCode))
            {
               //Report an error
               error = ERROR_UNEXPECTED_RESPONSE;
            }

            //Update FTP client state
            ftpClientChangeState(context, FTP_CLIENT_STATE_CONNECTED);
            //We are done
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
      error = ftpClientCheckTimeout(context);
   }

   //Return status code
   return error;
}


/**
 * @brief Get current working directory
 * @param[in] context Pointer to the FTP client context
 * @param[out] path Output buffer where to store the current directory
 * @param[in] maxLen Maximum number of characters the buffer can hold
 * @return Error code
 **/

error_t ftpClientGetWorkingDir(FtpClientContext *context, char_t *path,
   size_t maxLen)
{
   error_t error;

   //Check parameters
   if(context == NULL || path == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Execute FTP command
   while(!error)
   {
      //Check current state
      if(context->state == FTP_CLIENT_STATE_CONNECTED)
      {
         //Format PWD command
         error = ftpClientFormatCommand(context, "PWD", NULL);

         //Check status code
         if(!error)
         {
            //Send PWD command and wait for the server's response
            ftpClientChangeState(context, FTP_CLIENT_STATE_SUB_COMMAND_1);
         }
      }
      else if(context->state == FTP_CLIENT_STATE_SUB_COMMAND_1)
      {
         //Send PWD command and wait for the server's response
         error = ftpClientSendCommand(context);

         //Check status code
         if(!error)
         {
            //Check FTP response code
            if(FTP_REPLY_CODE_2YZ(context->replyCode))
            {
               //Parse server's response
               error = ftpClientParsePwdReply(context, path, maxLen);
            }
            else
            {
               //Report an error
               error = ERROR_UNEXPECTED_RESPONSE;
            }

            //Update FTP client state
            ftpClientChangeState(context, FTP_CLIENT_STATE_CONNECTED);
            //We are done
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
      error = ftpClientCheckTimeout(context);
   }

   //Return status code
   return error;
}


/**
 * @brief Change working directory
 * @param[in] context Pointer to the FTP client context
 * @param[in] path New current working directory
 * @return Error code
 **/

error_t ftpClientChangeWorkingDir(FtpClientContext *context,
   const char_t *path)
{
   error_t error;

   //Check parameters
   if(context == NULL || path == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Execute FTP command
   while(!error)
   {
      //Check current state
      if(context->state == FTP_CLIENT_STATE_CONNECTED)
      {
         //Format CWD command
         error = ftpClientFormatCommand(context, "CWD", path);

         //Check status code
         if(!error)
         {
            //Send CWD command and wait for the server's response
            ftpClientChangeState(context, FTP_CLIENT_STATE_SUB_COMMAND_1);
         }
      }
      else if(context->state == FTP_CLIENT_STATE_SUB_COMMAND_1)
      {
         //Send CWD command and wait for the server's response
         error = ftpClientSendCommand(context);

         //Check status code
         if(!error)
         {
            //Check FTP response code
            if(!FTP_REPLY_CODE_2YZ(context->replyCode))
            {
               //Report an error
               error = ERROR_UNEXPECTED_RESPONSE;
            }

            //Update FTP client state
            ftpClientChangeState(context, FTP_CLIENT_STATE_CONNECTED);
            //We are done
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
      error = ftpClientCheckTimeout(context);
   }

   //Return status code
   return error;
}


/**
 * @brief Change to parent directory
 * @param[in] context Pointer to the FTP client context
 * @return Error code
 **/

error_t ftpClientChangeToParentDir(FtpClientContext *context)
{
   error_t error;

   //Make sure the FTP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Execute FTP command
   while(!error)
   {
      //Check current state
      if(context->state == FTP_CLIENT_STATE_CONNECTED)
      {
         //Format CDUP command
         error = ftpClientFormatCommand(context, "CDUP", NULL);

         //Check status code
         if(!error)
         {
            //Send CDUP command and wait for the server's response
            ftpClientChangeState(context, FTP_CLIENT_STATE_SUB_COMMAND_1);
         }
      }
      else if(context->state == FTP_CLIENT_STATE_SUB_COMMAND_1)
      {
         //Send CDUP command and wait for the server's response
         error = ftpClientSendCommand(context);

         //Check status code
         if(!error)
         {
            //Check FTP response code
            if(!FTP_REPLY_CODE_2YZ(context->replyCode))
            {
               //Report an error
               error = ERROR_UNEXPECTED_RESPONSE;
            }

            //Update FTP client state
            ftpClientChangeState(context, FTP_CLIENT_STATE_CONNECTED);
            //We are done
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
      error = ftpClientCheckTimeout(context);
   }

   //Return status code
   return error;
}


/**
 * @brief Open a directory
 * @param[in] context Pointer to the FTP client context
 * @param[in] path Path to the directory to be be opened
 * @return Directory handle
 **/

error_t ftpClientOpenDir(FtpClientContext *context, const char_t *path)
{
   error_t error;

   //Check parameters
   if(context == NULL || path == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Execute FTP command sequence
   while(!error)
   {
      //Check current state
      if(context->state == FTP_CLIENT_STATE_CONNECTED)
      {
         //The data transfer is over the data connection in type ASCII or
         //type EBCDIC (refer to RFC 959, section 4.1.3)
         error = ftpClientFormatCommand(context, "TYPE", "A");

         //Check status code
         if(!error)
         {
            //Send TYPE command and wait for the server's response
            ftpClientChangeState(context, FTP_CLIENT_STATE_SUB_COMMAND_1);
         }
      }
      else if(context->state == FTP_CLIENT_STATE_SUB_COMMAND_1)
      {
         //Send TYPE command and wait for the server's response
         error = ftpClientSendCommand(context);

         //Check status code
         if(!error)
         {
            //Check FTP response code
            if(FTP_REPLY_CODE_2YZ(context->replyCode))
            {
               //Update FTP client state
               ftpClientChangeState(context, FTP_CLIENT_STATE_SUB_COMMAND_2);
            }
            else
            {
               //Report an error
               error = ERROR_UNEXPECTED_RESPONSE;
            }
         }
      }
      else if(context->state == FTP_CLIENT_STATE_SUB_COMMAND_2 ||
         context->state == FTP_CLIENT_STATE_SUB_COMMAND_3 ||
         context->state == FTP_CLIENT_STATE_SUB_COMMAND_4 ||
         context->state == FTP_CLIENT_STATE_SUB_COMMAND_5 ||
         context->state == FTP_CLIENT_STATE_SUB_COMMAND_6 ||
         context->state == FTP_CLIENT_STATE_CONNECTING_TCP ||
         context->state == FTP_CLIENT_STATE_SUB_COMMAND_8 ||
         context->state == FTP_CLIENT_STATE_ACCEPTING_TCP ||
         context->state == FTP_CLIENT_STATE_CONNECTING_TLS)
      {
         //Initiate data transfer
         error = ftpClientInitDataTransfer(context, FALSE);
      }
      else if(context->state == FTP_CLIENT_STATE_SUB_COMMAND_7)
      {
         //Format LIST command
         if(!osStrcmp(path, "."))
         {
            ftpClientFormatCommand(context, "LIST", NULL);
         }
         else
         {
            ftpClientFormatCommand(context, "LIST", path);
         }

         //Check status code
         if(!error)
         {
            //Send LIST command and wait for the server's response
            ftpClientChangeState(context, FTP_CLIENT_STATE_SUB_COMMAND_8);
         }
      }
      else if(context->state == FTP_CLIENT_STATE_SUB_COMMAND_9)
      {
         //Flush buffer
         context->bufferPos = 0;
         context->commandLen = 0;
         context->replyLen = 0;

         //The content of the directory can be transferred via the data
         //connection
         ftpClientChangeState(context, FTP_CLIENT_STATE_READING_DATA);

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

   //Failed to open directory?
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
 * @brief Read an entry from the directory
 * @param[in] context Pointer to the FTP client context
 * @param[out] dirEntry Pointer to a directory entry
 * @return Error code
 **/

error_t ftpClientReadDir(FtpClientContext *context, FtpDirEntry *dirEntry)
{
   error_t error;
   size_t n;

   //Check parameters
   if(context == NULL || dirEntry == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Erase the contents of the entry
   osMemset(dirEntry, 0, sizeof(FtpDirEntry));

   //Check current state
   if(context->state == FTP_CLIENT_STATE_READING_DATA)
   {
      //Loop through directory entries
      while(!error)
      {
         //Determine whether more data should be collected
         if(context->replyLen < (FTP_CLIENT_BUFFER_SIZE - 1))
         {
            //Receive data from the FTP server
            error = ftpClientReadChannel(&context->dataChannel,
               context->buffer + context->replyLen,
               FTP_CLIENT_BUFFER_SIZE - 1 - context->replyLen,
               &n, SOCKET_FLAG_BREAK_CRLF);

            //Check status code
            if(error == NO_ERROR)
            {
               //Advance data pointer
               context->replyLen += n;

               //Check whether the string is terminated by a CRLF sequence
               if(context->replyLen != 0 &&
                  context->buffer[context->replyLen - 1] == '\n')
               {
                  //Save current time
                  context->timestamp = osGetSystemTime();

                  //Properly terminate the string with a NULL character
                  context->buffer[context->replyLen] = '\0';
                  //Flush buffer
                  context->replyLen = 0;

                  //Remove trailing whitespace characters
                  strRemoveTrailingSpace(context->buffer);

                  //Discard empty lines
                  if(context->buffer[0] != '\0')
                  {
                     //Parse current directory entry
                     error = ftpClientParseDirEntry(context->buffer, dirEntry);
                     //We are done
                     break;
                  }
               }
            }
            else if(error == ERROR_WOULD_BLOCK || error == ERROR_TIMEOUT)
            {
               //Check whether the timeout has elapsed
               error = ftpClientCheckTimeout(context);
            }
            else
            {
               //Communication error
            }
         }
         else
         {
            //Flush buffer
            context->replyLen = 0;
         }
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
 * @brief Close directory
 * @param[in] context Pointer to the FTP client context
 * @return Error code
 **/

error_t ftpClientCloseDir(FtpClientContext *context)
{
   //Make sure the FTP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Close data connection and get transfer status
   return ftpClientTerminateDataTransfer(context);
}


/**
 * @brief Create a new directory
 * @param[in] context Pointer to the FTP client context
 * @param[in] path Name of the new directory
 * @return Error code
 **/

error_t ftpClientCreateDir(FtpClientContext *context, const char_t *path)
{
   error_t error;

   //Check parameters
   if(context == NULL || path == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Execute FTP command
   while(!error)
   {
      //Check current state
      if(context->state == FTP_CLIENT_STATE_CONNECTED)
      {
         //Format MKD command
         error = ftpClientFormatCommand(context, "MKD", path);

         //Check status code
         if(!error)
         {
            //Send MKD command and wait for the server's response
            ftpClientChangeState(context, FTP_CLIENT_STATE_SUB_COMMAND_1);
         }
      }
      else if(context->state == FTP_CLIENT_STATE_SUB_COMMAND_1)
      {
         //Send MKD command and wait for the server's response
         error = ftpClientSendCommand(context);

         //Check status code
         if(!error)
         {
            //Check FTP response code
            if(!FTP_REPLY_CODE_2YZ(context->replyCode))
            {
               //Report an error
               error = ERROR_UNEXPECTED_RESPONSE;
            }

            //Update FTP client state
            ftpClientChangeState(context, FTP_CLIENT_STATE_CONNECTED);
            //We are done
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
      error = ftpClientCheckTimeout(context);
   }

   //Return status code
   return error;
}


/**
 * @brief Remove a directory
 * @param[in] context Pointer to the FTP client context
 * @param[in] path Path to the directory to be removed
 * @return Error code
 **/

error_t ftpClientDeleteDir(FtpClientContext *context, const char_t *path)
{
   error_t error;

   //Check parameters
   if(context == NULL || path == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Execute FTP command
   while(!error)
   {
      //Check current state
      if(context->state == FTP_CLIENT_STATE_CONNECTED)
      {
         //Format RMD command
         error = ftpClientFormatCommand(context, "RMD", path);

         //Check status code
         if(!error)
         {
            //Send RMD command and wait for the server's response
            ftpClientChangeState(context, FTP_CLIENT_STATE_SUB_COMMAND_1);
         }
      }
      else if(context->state == FTP_CLIENT_STATE_SUB_COMMAND_1)
      {
         //Send RMD command and wait for the server's response
         error = ftpClientSendCommand(context);

         //Check status code
         if(!error)
         {
            //Check FTP response code
            if(!FTP_REPLY_CODE_2YZ(context->replyCode))
            {
               //Report an error
               error = ERROR_UNEXPECTED_RESPONSE;
            }

            //Update FTP client state
            ftpClientChangeState(context, FTP_CLIENT_STATE_CONNECTED);
            //We are done
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
      error = ftpClientCheckTimeout(context);
   }

   //Return status code
   return error;
}


/**
 * @brief Open a file for reading, writing, or appending
 * @param[in] context Pointer to the FTP client context
 * @param[in] path Path to the file to be be opened
 * @param[in] mode File access mode
 * @return Error code
 **/

error_t ftpClientOpenFile(FtpClientContext *context, const char_t *path,
   uint_t mode)
{
   error_t error;

   //Check parameters
   if(context == NULL || path == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Execute FTP command sequence
   while(!error)
   {
      //Check current state
      if(context->state == FTP_CLIENT_STATE_CONNECTED)
      {
         //Set representation type
         if((mode & FTP_FILE_MODE_TEXT) != 0)
         {
            //Use ASCII type
            error = ftpClientFormatCommand(context, "TYPE", "A");
         }
         else
         {
            //Use image type
            error = ftpClientFormatCommand(context, "TYPE", "I");
         }

         //Check status code
         if(!error)
         {
            //Send TYPE command and wait for the server's response
            ftpClientChangeState(context, FTP_CLIENT_STATE_SUB_COMMAND_1);
         }
      }
      else if(context->state == FTP_CLIENT_STATE_SUB_COMMAND_1)
      {
         //Send TYPE command and wait for the server's response
         error = ftpClientSendCommand(context);

         //Check status code
         if(!error)
         {
            //Check FTP response code
            if(FTP_REPLY_CODE_2YZ(context->replyCode))
            {
               //Update FTP client state
               ftpClientChangeState(context, FTP_CLIENT_STATE_SUB_COMMAND_2);
            }
            else
            {
               //Report an error
               error = ERROR_UNEXPECTED_RESPONSE;
            }
         }
      }
      else if(context->state == FTP_CLIENT_STATE_SUB_COMMAND_2 ||
         context->state == FTP_CLIENT_STATE_SUB_COMMAND_3 ||
         context->state == FTP_CLIENT_STATE_SUB_COMMAND_4 ||
         context->state == FTP_CLIENT_STATE_SUB_COMMAND_5 ||
         context->state == FTP_CLIENT_STATE_SUB_COMMAND_6 ||
         context->state == FTP_CLIENT_STATE_CONNECTING_TCP ||
         context->state == FTP_CLIENT_STATE_SUB_COMMAND_8 ||
         context->state == FTP_CLIENT_STATE_ACCEPTING_TCP ||
         context->state == FTP_CLIENT_STATE_CONNECTING_TLS)
      {
         //Initiate data transfer
         if((mode & FTP_FILE_MODE_WRITE) != 0 ||
            (mode & FTP_FILE_MODE_APPEND) != 0)
         {
            error = ftpClientInitDataTransfer(context, TRUE);
         }
         else
         {
            error = ftpClientInitDataTransfer(context, FALSE);
         }
      }
      else if(context->state == FTP_CLIENT_STATE_SUB_COMMAND_7)
      {
         //Format STOR/APPE/RETR command
         if((mode & FTP_FILE_MODE_WRITE) != 0)
         {
            ftpClientFormatCommand(context, "STOR", path);
         }
         else if((mode & FTP_FILE_MODE_APPEND) != 0)
         {
            ftpClientFormatCommand(context, "APPE", path);
         }
         else
         {
            ftpClientFormatCommand(context, "RETR", path);
         }

         //Check status code
         if(!error)
         {
            //Send STOR/APPE/RETR command and wait for the server's response
            ftpClientChangeState(context, FTP_CLIENT_STATE_SUB_COMMAND_8);
         }
      }
      else if(context->state == FTP_CLIENT_STATE_SUB_COMMAND_9)
      {
         //Check data transfer direction
         if((mode & FTP_FILE_MODE_WRITE) != 0 ||
            (mode & FTP_FILE_MODE_APPEND) != 0)
         {
            //The content of the file can be written via the data connection
            ftpClientChangeState(context, FTP_CLIENT_STATE_WRITING_DATA);
         }
         else
         {
            //The content of the file can be read via the data connection
            ftpClientChangeState(context, FTP_CLIENT_STATE_READING_DATA);
         }

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

   //Failed to open file?
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
 * @brief Write to a remote file
 * @param[in] context Pointer to the FTP client context
 * @param[in] data Pointer to a buffer containing the data to be written
 * @param[in] length Number of data bytes to write
 * @param[in] written Number of bytes that have been written (optional parameter)
 * @param[in] flags Set of flags that influences the behavior of this function
 * @return Error code
 **/

error_t ftpClientWriteFile(FtpClientContext *context, const void *data,
   size_t length, size_t *written, uint_t flags)
{
   error_t error;
   size_t n;

   //Make sure the FTP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Check parameters
   if(data == NULL && length != 0)
      return ERROR_INVALID_PARAMETER;

   //Actual number of bytes written
   n = 0;

   //Check current state
   if(context->state == FTP_CLIENT_STATE_WRITING_DATA)
   {
      //Transmit data to the FTP server
      error = ftpClientWriteChannel(&context->dataChannel, data, length, &n,
         flags);

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
      error = ftpClientCheckTimeout(context);
   }

   //Total number of data that have been written
   if(written != NULL)
      *written = n;

   //Return status code
   return error;
}


/**
 * @brief Read from a remote file
 * @param[in] context Pointer to the FTP client context
 * @param[out] data Buffer where to store the incoming data
 * @param[in] size Maximum number of bytes that can be read
 * @param[out] received Actual number of bytes that have been read
 * @param[in] flags Set of flags that influences the behavior of this function
 * @return Error code
 **/

error_t ftpClientReadFile(FtpClientContext *context, void *data, size_t size,
   size_t *received, uint_t flags)
{
   error_t error;

   //Check parameters
   if(context == NULL || data == NULL || received == NULL)
      return ERROR_INVALID_PARAMETER;

   //Check current state
   if(context->state == FTP_CLIENT_STATE_READING_DATA)
   {
      //Receive data from the FTP server
      error = ftpClientReadChannel(&context->dataChannel, data, size,
         received, flags);

      //Check status code
      if(error == NO_ERROR)
      {
         //Save current time
         context->timestamp = osGetSystemTime();
      }
      else if(error == ERROR_WOULD_BLOCK || error == ERROR_TIMEOUT)
      {
         //Check whether the timeout has elapsed
         error = ftpClientCheckTimeout(context);
      }
      else
      {
         //Communication error
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
 * @brief Close file
 * @param[in] context Pointer to the FTP client context
 * @return Error code
 **/

error_t ftpClientCloseFile(FtpClientContext *context)
{
   //Make sure the FTP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Close data connection and get transfer status
   return ftpClientTerminateDataTransfer(context);
}


/**
 * @brief Rename a file
 * @param[in] context Pointer to the FTP client context
 * @param[in] oldPath Name of an existing file or directory
 * @param[in] newPath New name for the file or directory
 * @return Error code
 **/

error_t ftpClientRenameFile(FtpClientContext *context, const char_t *oldPath,
   const char_t *newPath)
{
   error_t error;

   //Check parameters
   if(context == NULL || oldPath == NULL || newPath == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Execute FTP command sequence
   while(!error)
   {
      //Check current state
      if(context->state == FTP_CLIENT_STATE_CONNECTED)
      {
         //Format RNFR command
         error = ftpClientFormatCommand(context, "RNFR", oldPath);

         //Check status code
         if(!error)
         {
            //Send USER command and wait for the server's response
            ftpClientChangeState(context, FTP_CLIENT_STATE_SUB_COMMAND_1);
         }
      }
      else if(context->state == FTP_CLIENT_STATE_SUB_COMMAND_1)
      {
         //Send RNFR command and wait for the server's response
         error = ftpClientSendCommand(context);

         //Check status code
         if(!error)
         {
            //Check FTP response code
            if(FTP_REPLY_CODE_3YZ(context->replyCode))
            {
               //Format RNTO command
               error = ftpClientFormatCommand(context, "RNTO", newPath);

               //Check status code
               if(!error)
               {
                  //Send RNTO command and wait for the server's response
                  ftpClientChangeState(context, FTP_CLIENT_STATE_SUB_COMMAND_2);
               }
            }
            else
            {
               //Update FTP client state
               ftpClientChangeState(context, FTP_CLIENT_STATE_CONNECTED);
               //Report an error
               error = ERROR_UNEXPECTED_RESPONSE;
            }
         }
      }
      else if(context->state == FTP_CLIENT_STATE_SUB_COMMAND_2)
      {
         //Send RNTO command and wait for the server's response
         error = ftpClientSendCommand(context);

         //Check status code
         if(!error)
         {
            //Check FTP response code
            if(!FTP_REPLY_CODE_2YZ(context->replyCode))
            {
               //Report an error
               error = ERROR_UNEXPECTED_RESPONSE;
            }

            //Update FTP client state
            ftpClientChangeState(context, FTP_CLIENT_STATE_CONNECTED);
            //We are done
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
      error = ftpClientCheckTimeout(context);
   }

   //Return status code
   return error;
}


/**
 * @brief Delete a file
 * @param[in] context Pointer to the FTP client context
 * @param[in] path Path to the file to be be deleted
 * @return Error code
 **/

error_t ftpClientDeleteFile(FtpClientContext *context, const char_t *path)
{
   error_t error;

   //Check parameters
   if(context == NULL || path == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Execute FTP command
   while(!error)
   {
      //Check current state
      if(context->state == FTP_CLIENT_STATE_CONNECTED)
      {
         //Format DELE command
         error = ftpClientFormatCommand(context, "DELE", path);

         //Check status code
         if(!error)
         {
            //Send DELE command and wait for the server's response
            ftpClientChangeState(context, FTP_CLIENT_STATE_SUB_COMMAND_1);
         }
      }
      else if(context->state == FTP_CLIENT_STATE_SUB_COMMAND_1)
      {
         //Send DELE command and wait for the server's response
         error = ftpClientSendCommand(context);

         //Check status code
         if(!error)
         {
            //Check FTP response code
            if(!FTP_REPLY_CODE_2YZ(context->replyCode))
            {
               //Report an error
               error = ERROR_UNEXPECTED_RESPONSE;
            }

            //Update FTP client state
            ftpClientChangeState(context, FTP_CLIENT_STATE_CONNECTED);
            //We are done
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
      error = ftpClientCheckTimeout(context);
   }

   //Return status code
   return error;
}


/**
 * @brief Retrieve server's reply code
 * @param[in] context Pointer to the FTP client context
 * @return FTP reply code
 **/

uint_t ftpClientGetReplyCode(FtpClientContext *context)
{
   uint_t replyCode;

   //Make sure the FTP client context is valid
   if(context != NULL)
   {
      //Get server's reply code
      replyCode = context->replyCode;
   }
   else
   {
      //The FTP client context is not valid
      replyCode = 0;
   }

   //Return FTP reply code
   return replyCode;
}


/**
 * @brief Gracefully disconnect from the FTP server
 * @param[in] context Pointer to the FTP client context
 * @return Error code
 **/

error_t ftpClientDisconnect(FtpClientContext *context)
{
   error_t error;

   //Make sure the FTP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Execute FTP command sequence
   while(!error)
   {
      //Check current state
      if(context->state == FTP_CLIENT_STATE_CONNECTED)
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
            //Update FTP client state
            ftpClientChangeState(context, FTP_CLIENT_STATE_DISCONNECTING_2);
         }
      }
      else if(context->state == FTP_CLIENT_STATE_DISCONNECTING_2)
      {
         //Shutdown control connection
         error = ftpClientShutdownChannel(&context->controlChannel);

         //Check status code
         if(!error)
         {
            //Close control connection
            ftpClientCloseChannel(&context->controlChannel);
            //Update FTP client state
            ftpClientChangeState(context, FTP_CLIENT_STATE_DISCONNECTED);
         }
      }
      else if(context->state == FTP_CLIENT_STATE_DISCONNECTED)
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

   //Failed to gracefully disconnect from the FTP server?
   if(error != NO_ERROR && error != ERROR_WOULD_BLOCK)
   {
      //Close data and control connections
      ftpClientCloseChannel(&context->dataChannel);
      ftpClientCloseChannel(&context->controlChannel);

      //Update FTP client state
      ftpClientChangeState(context, FTP_CLIENT_STATE_DISCONNECTED);
   }

   //Return status code
   return error;
}


/**
 * @brief Close the connection with the FTP server
 * @param[in] context Pointer to the FTP client context
 * @return Error code
 **/

error_t ftpClientClose(FtpClientContext *context)
{
   //Make sure the FTP client context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Close data and control connections
   ftpClientCloseChannel(&context->dataChannel);
   ftpClientCloseChannel(&context->controlChannel);

   //Update FTP client state
   ftpClientChangeState(context, FTP_CLIENT_STATE_DISCONNECTED);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Release FTP client context
 * @param[in] context Pointer to the FTP client context
 **/

void ftpClientDeinit(FtpClientContext *context)
{
   //Make sure the FTP client context is valid
   if(context != NULL)
   {
      //Close data and control connections
      ftpClientCloseChannel(&context->dataChannel);
      ftpClientCloseChannel(&context->controlChannel);

#if (FTP_CLIENT_TLS_SUPPORT == ENABLED)
      //Release TLS session state
      tlsFreeSessionState(&context->tlsSession);
#endif

      //Clear FTP client context
      osMemset(context, 0, sizeof(FtpClientContext));
   }
}

#endif
