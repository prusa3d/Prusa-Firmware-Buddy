/**
 * @file ftp_server_commands.c
 * @brief FTP server (command processing)
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
#include "ipv4/ipv4_misc.h"
#include "ftp/ftp_server.h"
#include "ftp/ftp_server_commands.h"
#include "ftp/ftp_server_data.h"
#include "ftp/ftp_server_misc.h"
#include "str.h"
#include "path.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (FTP_SERVER_SUPPORT == ENABLED)


/**
 * @brief FTP command processing
 * @param[in] connection Pointer to the client connection
 **/

void ftpServerProcessCommand(FtpClientConnection *connection)
{
   size_t n;
   char_t *p;

   //The CRLF sequence should be used to terminate the command line
   for(n = 0; n < connection->commandLen; n++)
   {
      if(connection->command[n] == '\n')
         break;
   }

   //Any command to process?
   if(n < connection->commandLen)
   {
      //Properly terminate the string with a NULL character
      connection->command[n] = '\0';
      //Remove trailing whitespace from the command line
      strRemoveTrailingSpace(connection->command);

      //Debug message
      TRACE_DEBUG("FTP client: %s\r\n", connection->command);

      //Command line too long?
      if(connection->controlChannel.state == FTP_CHANNEL_STATE_DISCARD)
      {
         //Switch to idle state
         connection->controlChannel.state = FTP_CHANNEL_STATE_IDLE;
         //Format response message
         osStrcpy(connection->response, "500 Command line too long\r\n");
      }
      else
      {
         //The command name and the arguments are separated by one or more
         //spaces
         for(p = connection->command; *p != '\0' && *p != ' '; p++)
         {
         }

         //Space character found?
         if(*p == ' ')
         {
            //Split the string at the first occurrence of the space character
            *(p++) = '\0';

            //Skip extra whitespace
            while(*p == ' ')
            {
               p++;
            }
         }

         //NOOP command received?
         if(!osStrcasecmp(connection->command, "NOOP"))
         {
            ftpServerProcessNoop(connection, p);
         }
         //SYST command received?
         else if(!osStrcasecmp(connection->command, "SYST"))
         {
            ftpServerProcessSyst(connection, p);
         }
         //FEAT command received?
         else if(!osStrcasecmp(connection->command, "FEAT"))
         {
            ftpServerProcessFeat(connection, p);
         }
#if (FTP_SERVER_TLS_SUPPORT == ENABLED)
         //AUTH command received?
         else if(!osStrcasecmp(connection->command, "AUTH"))
         {
            ftpServerProcessAuth(connection, p);
         }
         //PBSZ command received?
         else if(!osStrcasecmp(connection->command, "PBSZ"))
         {
            ftpServerProcessPbsz(connection, p);
         }
         //PROT command received?
         else if(!osStrcasecmp(connection->command, "PROT"))
         {
            ftpServerProcessProt(connection, p);
         }
#endif
         //TYPE command received?
         else if(!osStrcasecmp(connection->command, "TYPE"))
         {
            ftpServerProcessType(connection, p);
         }
         //STRU command received?
         else if(!osStrcasecmp(connection->command, "STRU"))
         {
            ftpServerProcessStru(connection, p);
         }
         //MODE command received?
         else if(!osStrcasecmp(connection->command, "MODE"))
         {
            ftpServerProcessMode(connection, p);
         }
         //USER command received?
         else if(!osStrcasecmp(connection->command, "USER"))
         {
            ftpServerProcessUser(connection, p);
         }
         //PASS command received?
         else if(!osStrcasecmp(connection->command, "PASS"))
         {
            ftpServerProcessPass(connection, p);
         }
         //REIN command received?
         else if(!osStrcasecmp(connection->command, "REIN"))
         {
            ftpServerProcessRein(connection, p);
         }
         //QUIT command received?
         else if(!osStrcasecmp(connection->command, "QUIT"))
         {
            ftpServerProcessQuit(connection, p);
         }
         //PORT command received?
         else if(!osStrcasecmp(connection->command, "PORT"))
         {
            ftpServerProcessPort(connection, p);
         }
         //EPRT command received?
         else if(!osStrcasecmp(connection->command, "EPRT"))
         {
            ftpServerProcessEprt(connection, p);
         }
         //PASV command received?
         else if(!osStrcasecmp(connection->command, "PASV"))
         {
            ftpServerProcessPasv(connection, p);
         }
         //EPSV command received?
         else if(!osStrcasecmp(connection->command, "EPSV"))
         {
            ftpServerProcessEpsv(connection, p);
         }
         //ABOR command received?
         else if(!osStrcasecmp(connection->command, "ABOR"))
         {
            ftpServerProcessAbor(connection, p);
         }
         //PWD command received?
         else if(!osStrcasecmp(connection->command, "PWD"))
         {
            ftpServerProcessPwd(connection, p);
         }
         //LIST command received?
         else if(!osStrcasecmp(connection->command, "LIST"))
         {
            ftpServerProcessList(connection, p);
         }
         //NLST command received?
         else if(!osStrcasecmp(connection->command, "NLST"))
         {
            ftpServerProcessNlst(connection, p);
         }
         //CWD command received?
         else if(!osStrcasecmp(connection->command, "CWD"))
         {
            ftpServerProcessCwd(connection, p);
         }
         //CDUP command received?
         else if(!osStrcasecmp(connection->command, "CDUP"))
         {
            ftpServerProcessCdup(connection, p);
         }
         //MKD command received?
         else if(!osStrcasecmp(connection->command, "MKD"))
         {
            ftpServerProcessMkd(connection, p);
         }
         //RMD command received?
         else if(!osStrcasecmp(connection->command, "RMD"))
         {
            ftpServerProcessRmd(connection, p);
         }
         //SIZE command received?
         else if(!osStrcasecmp(connection->command, "SIZE"))
         {
            ftpServerProcessSize(connection, p);
         }
         //RETR command received?
         else if(!osStrcasecmp(connection->command, "RETR"))
         {
            ftpServerProcessRetr(connection, p);
         }
         //STOR command received?
         else if(!osStrcasecmp(connection->command, "STOR"))
         {
            ftpServerProcessStor(connection, p);
         }
         //APPE command received?
         else if(!osStrcasecmp(connection->command, "APPE"))
         {
            ftpServerProcessAppe(connection, p);
         }
         //RNFR command received?
         else if(!osStrcasecmp(connection->command, "RNFR"))
         {
            ftpServerProcessRnfr(connection, p);
         }
         //RNTO command received?
         else if(!osStrcasecmp(connection->command, "RNTO"))
         {
            ftpServerProcessRnto(connection, p);
         }
         //DELE command received?
         else if(!osStrcasecmp(connection->command, "DELE"))
         {
            ftpServerProcessDele(connection, p);
         }
         //Unknown command received?
         else
         {
            ftpServerProcessUnknownCmd(connection, p);
         }
      }

      //Debug message
      TRACE_DEBUG("FTP server: %s", connection->response);

      //Number of bytes in the response buffer
      connection->responseLen = osStrlen(connection->response);
      connection->responsePos = 0;
   }
   else if(connection->commandLen >= FTP_SERVER_MAX_LINE_LEN)
   {
      //Drop incoming data
      connection->controlChannel.state = FTP_CHANNEL_STATE_DISCARD;
   }

   //Flush command line
   connection->commandLen = 0;
}


/**
 * @brief NOOP command processing
 *
 * The NOOP command does not affect any parameters or previously entered
 * commands. It specifies no action other than that the server send an OK reply
 *
 * @param[in] connection Pointer to the client connection
 * @param[in] param Command line parameters
 **/

void ftpServerProcessNoop(FtpClientConnection *connection, char_t *param)
{
   //Send an OK reply
   osStrcpy(connection->response, "200 Command okay\r\n");
}


/**
 * @brief SYST command processing
 *
 * The SYST command is used to find out the type of operating system
 * at the server side
 *
 * @param[in] connection Pointer to the client connection
 * @param[in] param Command line parameters
 **/

void ftpServerProcessSyst(FtpClientConnection *connection, char_t *param)
{
   //Format the response to the SYST command
   osStrcpy(connection->response, "215 UNIX Type: L8\r\n");
}


/**
 * @brief FEAT command processing
 *
 * The FEAT command allows a client to discover which optional
 * commands a server supports
 *
 * @param[in] connection Pointer to the client connection
 * @param[in] param Command line parameters
 **/

void ftpServerProcessFeat(FtpClientConnection *connection, char_t *param)
{
#if (FTP_SERVER_TLS_SUPPORT == ENABLED)
   FtpServerContext *context;

   //Point to the FTP server context
   context = connection->context;
#endif

   //Format the response to the FEAT command
   osStrcpy(connection->response, "211-Extensions supported:\r\n");

   //Each extension supported must be listed on a separate line
   osStrcat(connection->response, " SIZE\r\n");
   osStrcat(connection->response, " EPRT\r\n");
   osStrcat(connection->response, " EPSV\r\n");

#if (FTP_SERVER_TLS_SUPPORT == ENABLED)
   //TLS security mode supported by the server?
   if((context->settings.mode & FTP_SERVER_MODE_IMPLICIT_TLS) != 0 ||
      (context->settings.mode & FTP_SERVER_MODE_EXPLICIT_TLS) != 0)
   {
      //If a server supports the FEAT command, then it must advertise
      //supported AUTH, PBSZ, and PROT commands in the reply (refer to
      //RFC 4217, section 6)
      osStrcat(connection->response, " AUTH TLS\r\n");
      osStrcat(connection->response, " PBSZ\r\n");
      osStrcat(connection->response, " PROT\r\n");
   }
#endif

   //Terminate feature listing
   osStrcat(connection->response, "211 End\r\n");
}


/**
 * @brief AUTH command processing
 *
 * The AUTH command specifies the security mechanism
 *
 * @param[in] connection Pointer to the client connection
 * @param[in] param Command line parameters
 **/

void ftpServerProcessAuth(FtpClientConnection *connection, char_t *param)
{
#if (FTP_SERVER_TLS_SUPPORT == ENABLED)
   FtpServerContext *context;

   //Point to the FTP server context
   context = connection->context;

   //TLS security mode supported by the server?
   if((context->settings.mode & FTP_SERVER_MODE_IMPLICIT_TLS) != 0)
   {
      //When using implicit FTPS, a TLS connection is immediately established
      //via port 990 before any command is exchanged
      osStrcpy(connection->response, "534 Secure connection already negotiated\r\n");
   }
   else if((context->settings.mode & FTP_SERVER_MODE_EXPLICIT_TLS) != 0)
   {
      //The argument specifies the security mechanism
      if(*param != '\0')
      {
         //TLS security mechanism?
         if(!osStrcasecmp(param, "TLS"))
         {
            //If the server is willing to accept the named security mechanism,
            //and does not require any security data, it must respond with reply
            //code 234
            osStrcpy(connection->response, "234 AUTH TLS OK\r\n");

            //Establish a protected session
            connection->controlChannel.state = FTP_CHANNEL_STATE_AUTH_TLS_1;
         }
         else
         {
            //The security mechanism is unknown
            osStrcpy(connection->response, "504 Unknown security scheme\r\n");
         }
      }
      else
      {
         //The argument is missing
         osStrcpy(connection->response, "501 Missing parameter\r\n");
      }
   }
   else
#endif
   {
      //TLS security mode is not supported
      osStrcpy(connection->response, "502 Command not implemented\r\n");
   }
}


/**
 * @brief PBSZ command processing
 *
 * The PBSZ command specifies the maximum size, in bytes, of the encoded data
 * blocks to be sent or received during file transfer
 *
 * @param[in] connection Pointer to the client connection
 * @param[in] param Command line parameters
 **/

void ftpServerProcessPbsz(FtpClientConnection *connection, char_t *param)
{
#if (FTP_SERVER_TLS_SUPPORT == ENABLED)
   FtpServerContext *context;

   //Point to the FTP server context
   context = connection->context;

   //TLS security mode supported by the server?
   if((context->settings.mode & FTP_SERVER_MODE_IMPLICIT_TLS) != 0 ||
      (context->settings.mode & FTP_SERVER_MODE_EXPLICIT_TLS) != 0)
   {
      //The argument specifies the maximum size of the encoded data blocks
      if(*param != '\0')
      {
         //Format the response to the PBSZ command
         osStrcpy(connection->response, "200 PBSZ=0\r\n");
      }
      else
      {
         //The argument is missing
         osStrcpy(connection->response, "501 Missing parameter\r\n");
      }
   }
   else
#endif
   {
      //TLS security mode is not supported
      osStrcpy(connection->response, "502 Command not implemented\r\n");
   }
}


/**
 * @brief PROT command processing
 *
 * The PROT command specifies the data channel protection level
 *
 * @param[in] connection Pointer to the client connection
 * @param[in] param Command line parameters
 **/

void ftpServerProcessProt(FtpClientConnection *connection, char_t *param)
{
#if (FTP_SERVER_TLS_SUPPORT == ENABLED)
   FtpServerContext *context;

   //Point to the FTP server context
   context = connection->context;

   //TLS security mode supported by the server?
   if((context->settings.mode & FTP_SERVER_MODE_IMPLICIT_TLS) != 0 ||
      (context->settings.mode & FTP_SERVER_MODE_EXPLICIT_TLS) != 0)
   {
      //The argument specifies the data protection level
      if(*param != '\0')
      {
         //Private protection level?
         if(!osStrcasecmp(param, "P"))
         {
            //The server must reply with a 200 reply code to indicate that the
            //specified protection level is accepted
            osStrcpy(connection->response, "200 Data protection level set to P\r\n");
         }
         //Unknown security mechanism?
         else
         {
            //If the server does not understand the specified protection level,
            //it should respond with reply code 504
            osStrcpy(connection->response, "504 Unknown protection level\r\n");
         }
      }
      else
      {
         //The argument is missing
         osStrcpy(connection->response, "501 Missing parameter\r\n");
      }
   }
   else
#endif
   {
      //TLS security mode is not supported
      osStrcpy(connection->response, "502 Command not implemented\r\n");
   }
}


/**
 * @brief TYPE command processing
 *
 * The TYPE command specifies the representation type
 *
 * @param[in] connection Pointer to the client connection
 * @param[in] param Command line parameters
 **/

void ftpServerProcessType(FtpClientConnection *connection, char_t *param)
{
   //The argument specifies the representation type
   if(*param != '\0')
   {
      //ASCII type?
      if(!osStrcasecmp(param, "A"))
      {
         //Format the response to the TYPE command
         osStrcpy(connection->response, "200 Type set to A\r\n");
      }
      //Image type?
      else if(!osStrcasecmp(param, "I"))
      {
         //Format the response to the TYPE command
         osStrcpy(connection->response, "200 Type set to I\r\n");
      }
      //Unknown type?
      else
      {
         //Report an error
         osStrcpy(connection->response, "504 Unknown type\r\n");
      }
   }
   else
   {
      //The argument is missing
      osStrcpy(connection->response, "501 Missing parameter\r\n");
   }
}


/**
 * @brief STRU command processing
 *
 * The STRU command specifies the file structure
 *
 * @param[in] connection Pointer to the client connection
 * @param[in] param Command line parameters
 **/

void ftpServerProcessStru(FtpClientConnection *connection, char_t *param)
{
   //The argument specifies the file structure
   if(*param != '\0')
   {
      //No record structure?
      if(!osStrcasecmp(param, "F"))
      {
         //Format the response to the STRU command
         osStrcpy(connection->response, "200 Structure set to F\r\n");
      }
      //Unknown file structure?
      else
      {
         //Report an error
         osStrcpy(connection->response, "504 Unknown structure\r\n");
      }
   }
   else
   {
      //The argument is missing
      osStrcpy(connection->response, "501 Missing parameter\r\n");
   }
}


/**
 * @brief MODE command processing
 *
 * The MODE command specifies the data transfer mode
 *
 * @param[in] connection Pointer to the client connection
 * @param[in] param Command line parameters
 **/

void ftpServerProcessMode(FtpClientConnection *connection, char_t *param)
{
   //The argument specifies the data transfer mode
   if(*param != '\0')
   {
      //Stream mode?
      if(!osStrcasecmp(param, "S"))
      {
         //Format the response to the MODE command
         osStrcpy(connection->response, "200 Mode set to S\r\n");
      }
      //Unknown data transfer mode?
      else
      {
         //Report an error
         osStrcpy(connection->response, "504 Unknown mode\r\n");
      }
   }
   else
   {
      //The argument is missing
      osStrcpy(connection->response, "501 Missing parameter\r\n");
   }
}


/**
 * @brief USER command processing
 *
 * The USER command is used to identify the user
 *
 * @param[in] connection Pointer to the client connection
 * @param[in] param Command line parameters
 **/

void ftpServerProcessUser(FtpClientConnection *connection, char_t *param)
{
   uint_t status;
   FtpServerContext *context;

   //Point to the FTP server context
   context = connection->context;

   //The argument specifies the user name
   if(*param == '\0')
   {
      //The argument is missing
      osStrcpy(connection->response, "501 Missing parameter\r\n");
      //Exit immediately
      return;
   }

   //Check the length of the user name
   if(osStrlen(param) > FTP_SERVER_MAX_USERNAME_LEN)
   {
      //The specified user name is not valid...
      osStrcpy(connection->response, "501 Invalid parameter\r\n");
      //Exit immediately
      return;
   }

#if (FTP_SERVER_TLS_SUPPORT == ENABLED)
   //Cleartext FTP session?
   if(connection->controlChannel.tlsContext == NULL)
   {
      //If the server needs AUTH, then it refuses to accept certain commands
      //until it gets a successfully protected session (refer to RFC 4217,
      //section 11.1)
      if((context->settings.mode & FTP_SERVER_MODE_PLAINTEXT) == 0)
      {
         //Format response message
         osStrcpy(connection->response, "421 Cleartext sessions are not accepted\r\n");
         //Exit immediately
         return;
      }
   }
#endif

   //Save user name
   osStrcpy(connection->user, param);
   //Log out the user
   connection->userLoggedIn = FALSE;

   //Set home directory
   pathCopy(connection->homeDir, context->settings.rootDir,
      FTP_SERVER_MAX_HOME_DIR_LEN);

   //Set current directory
   pathCopy(connection->currentDir, context->settings.rootDir,
      FTP_SERVER_MAX_PATH_LEN);

   //Invoke user-defined callback, if any
   if(context->settings.checkUserCallback != NULL)
   {
      status = context->settings.checkUserCallback(connection, param);
   }
   else
   {
      status = FTP_ACCESS_ALLOWED;
   }

   //Access allowed?
   if(status == FTP_ACCESS_ALLOWED)
   {
      //The user is now logged in
      connection->userLoggedIn = TRUE;
      //Format response message
      osStrcpy(connection->response, "230 User logged in, proceed\r\n");
   }
   //Password required?
   else if(status == FTP_PASSWORD_REQUIRED)
   {
      //This command must be immediately followed by a PASS command
      connection->controlChannel.state = FTP_CHANNEL_STATE_USER;
      //Format response message
      osStrcpy(connection->response, "331 User name okay, need password\r\n");
   }
   //Access denied?
   else
   {
      //Format response message
      osStrcpy(connection->response, "530 Login authentication failed\r\n");
   }
}


/**
 * @brief PASS command processing
 *
 * The USER command specifies the user's password
 *
 * @param[in] connection Pointer to the client connection
 * @param[in] param Command line parameters
 **/

void ftpServerProcessPass(FtpClientConnection *connection, char_t *param)
{
   uint_t status;
   FtpServerContext *context;

   //Point to the FTP server context
   context = connection->context;

   //This command must immediately follow a USER command
   if(connection->controlChannel.state != FTP_CHANNEL_STATE_USER)
   {
      //Switch to idle state
      connection->controlChannel.state = FTP_CHANNEL_STATE_IDLE;
      //Report an error
      osStrcpy(connection->response, "503 Bad sequence of commands\r\n");
      //Exit immediately
      return;
   }

   //Switch to idle state
   connection->controlChannel.state = FTP_CHANNEL_STATE_IDLE;

   //The argument specifies the password
   if(*param == '\0')
   {
      //The argument is missing
      osStrcpy(connection->response, "501 Missing parameter\r\n");
      //Exit immediately
      return;
   }

   //Invoke user-defined callback, if any
   if(context->settings.checkPasswordCallback != NULL)
   {
      status = context->settings.checkPasswordCallback(connection,
         connection->user, param);
   }
   else
   {
      status = FTP_ACCESS_ALLOWED;
   }

   //Access allowed?
   if(status == FTP_ACCESS_ALLOWED)
   {
      //The user is now logged in
      connection->userLoggedIn = TRUE;
      //Format response message
      osStrcpy(connection->response, "230 User logged in, proceed\r\n");
   }
   //Access denied?
   else
   {
      //Format response message
      osStrcpy(connection->response, "530 Login authentication failed\r\n");
   }
}


/**
 * @brief REIN command processing
 *
 * The REIN command is used to reinitialize a user session
 *
 * @param[in] connection Pointer to the client connection
 * @param[in] param Command line parameters
 **/

void ftpServerProcessRein(FtpClientConnection *connection, char_t *param)
{
   //Close data connection
   ftpServerCloseDataChannel(connection);

   //Release previously allocated resources
   if(connection->file != NULL)
   {
      fsCloseFile(connection->file);
      connection->file = NULL;
   }

   if(connection->dir != NULL)
   {
      fsCloseDir(connection->dir);
      connection->dir = NULL;
   }

   //Clear account information
   connection->userLoggedIn = FALSE;

   //Format response message
   osStrcpy(connection->response, "220 Service ready for new user\r\n");
}


/**
 * @brief QUIT command processing
 *
 * The QUIT command is used to terminate a user session
 *
 * @param[in] connection Pointer to the client connection
 * @param[in] param Command line parameters
 **/

void ftpServerProcessQuit(FtpClientConnection *connection, char_t *param)
{
   //There are two cases to consider upon receipt of this command
   if(connection->dataChannel.state == FTP_CHANNEL_STATE_CLOSED)
   {
      //If the FTP service command was already completed, the server closes
      //the data connection (if it is open)...
      ftpServerCloseDataChannel(connection);

      //...and responds with a 221 reply
      osStrcpy(connection->response, "221 Service closing control connection\r\n");
   }
   else
   {
      //If the FTP service command is still in progress, the server aborts
      //the FTP service in progress and closes the data connection...
      ftpServerCloseDataChannel(connection);

      //...returning a 426 reply to indicate that the service request
      //terminated abnormally
      osStrcpy(connection->response, "426 Connection closed; transfer aborted\r\n");

      //The server then sends a 221 reply
      osStrcat(connection->response, "221 Service closing control connection\r\n");
   }

   //Release previously allocated resources
   if(connection->file != NULL)
   {
      fsCloseFile(connection->file);
      connection->file = NULL;
   }

   if(connection->dir != NULL)
   {
      fsCloseDir(connection->dir);
      connection->dir = NULL;
   }

   //Clear account information
   connection->userLoggedIn = FALSE;
   //Gracefully disconnect from the remote host
   connection->controlChannel.state = FTP_CHANNEL_STATE_WAIT_ACK;
}


/**
 * @brief PORT command processing
 *
 * The PORT command specifies the data port to be used for the data connection
 *
 * @param[in] connection Pointer to the client connection
 * @param[in] param Command line parameters
 **/

void ftpServerProcessPort(FtpClientConnection *connection, char_t *param)
{
   error_t error;
   size_t i;
   size_t j;
   char_t *p;
   char_t *token;
   char_t *end;

   //Ensure the user is logged in
   if(!connection->userLoggedIn)
   {
      //Format response message
      osStrcpy(connection->response, "530 Not logged in\r\n");
      //Exit immediately
      return;
   }

   //The argument is the concatenation of the IP address and the 16-bit
   //port number
   if(*param == '\0')
   {
      //The argument is missing
      osStrcpy(connection->response, "501 Missing parameter\r\n");
      //Exit immediately
      return;
   }

   //Close the data connection, if any
   ftpServerCloseDataChannel(connection);

   //Start of exception handling block
   do
   {
      //Assume an error condition...
      error = ERROR_INVALID_SYNTAX;

      //Parse the string
      for(i = 0, j = 1; param[i] != '\0'; i++)
      {
         //Change commas to dots
         if(param[i] == ',' && j < sizeof(Ipv4Addr))
         {
            param[i] = '.';
            j++;
         }
      }

      //Get the IP address to be used
      token = osStrtok_r(param, ",", &p);
      //Syntax error?
      if(token == NULL)
         break;

      //Convert the dot-decimal string to a binary IP address
      error = ipStringToAddr(token, &connection->remoteIpAddr);
      //Invalid IP address?
      if(error)
         break;

      //Assume an error condition...
      error = ERROR_INVALID_SYNTAX;

      //Get the most significant byte of the port number
      token = osStrtok_r(NULL, ",", &p);
      //Syntax error?
      if(token == NULL)
         break;

      //Convert the string representation to integer
      connection->remotePort = (uint16_t) osStrtoul(token, &end, 10) << 8;
      //Syntax error?
      if(*end != '\0')
         break;

      //Get the least significant byte of the port number
      token = osStrtok_r(NULL, ",", &p);
      //Syntax error?
      if(token == NULL)
         break;

      //Convert the string representation to integer
      connection->remotePort |= osStrtoul(token, &end, 10) & 0xFF;
      //Syntax error?
      if(*end != '\0')
         break;

      //Successful processing
      error = NO_ERROR;

      //End of exception handling block
   } while(0);

   //Any error to report?
   if(error)
   {
      //Re initialize data connection
      connection->passiveMode = FALSE;
      connection->remotePort = 0;

      //Format response message
      osStrcpy(connection->response, "501 Syntax error in parameters or arguments\r\n");
      //Exit immediately
      return;
   }

   //Successful processing
   osStrcpy(connection->response, "200 Command okay\r\n");
}


/**
 * @brief EPRT command processing
 *
 * The EPRT command allows for the specification of an extended address
 * for the data connection
 *
 * @param[in] connection Pointer to the client connection
 * @param[in] param Command line parameters
 **/

void ftpServerProcessEprt(FtpClientConnection *connection, char_t *param)
{
   error_t error;
   uint_t protocol;
   char_t *p;
   char_t *token;
   char_t *end;
   char_t delimiter[2];

   //Ensure the user is logged in
   if(!connection->userLoggedIn)
   {
      //Format response message
      osStrcpy(connection->response, "530 Not logged in\r\n");
      //Exit immediately
      return;
   }

   //The extended address must consist of the network protocol
   //as well as the IP address and the 16-bit port number
   if(*param == '\0')
   {
      //The argument is missing
      osStrcpy(connection->response, "501 Missing parameter\r\n");
      //Exit immediately
      return;
   }

   //Close the data connection, if any
   ftpServerCloseDataChannel(connection);

   //Start of exception handling block
   do
   {
      //A delimiter character must be specified
      delimiter[0] = param[0];
      delimiter[1] = '\0';
      //Skip delimiter character
      param++;

      //Assume an error condition...
      error = ERROR_INVALID_SYNTAX;

      //Retrieve the network protocol to be used
      token = osStrtok_r(param, delimiter, &p);
      //Syntax error?
      if(token == NULL)
         break;

      //Convert the string representation to integer
      protocol = osStrtoul(token, &end, 10);
      //Syntax error?
      if(*end != '\0')
         break;

      //Get the IP address to be used
      token = osStrtok_r(NULL, delimiter, &p);
      //Syntax error?
      if(token == NULL)
         break;

#if (IPV4_SUPPORT == ENABLED)
      //IPv4 address family?
      if(protocol == 1)
      {
         //IPv4 addresses are 4-byte long
         connection->remoteIpAddr.length = sizeof(Ipv4Addr);
         //Convert the string to IPv4 address
         error = ipv4StringToAddr(token, &connection->remoteIpAddr.ipv4Addr);
         //Invalid IP address?
         if(error)
            break;
      }
      else
#endif
#if (IPV6_SUPPORT == ENABLED)
      //IPv6 address family?
      if(protocol == 2)
      {
         //IPv6 addresses are 16-byte long
         connection->remoteIpAddr.length = sizeof(Ipv6Addr);
         //Convert the string to IPv6 address
         error = ipv6StringToAddr(token, &connection->remoteIpAddr.ipv6Addr);
         //Invalid IP address?
         if(error)
            break;
      }
      else
#endif
      //Unknown address family?
      {
         //Report an error
         error = ERROR_INVALID_ADDRESS;
         //Exit immediately
         break;
      }

      //Assume an error condition...
      error = ERROR_INVALID_SYNTAX;

      //Get the port number to be used
      token = osStrtok_r(NULL, delimiter, &p);
      //Syntax error?
      if(token == NULL)
         break;

      //Convert the string representation to integer
      connection->remotePort = (uint16_t) osStrtoul(token, &end, 10);
      //Syntax error?
      if(*end != '\0')
         break;

      //Successful processing
      error = NO_ERROR;

      //End of exception handling block
   } while(0);

   //Any error to report?
   if(error)
   {
      //Re initialize data connection
      connection->passiveMode = FALSE;
      connection->remotePort = 0;

      //Format response message
      osStrcpy(connection->response, "501 Syntax error in parameters or arguments\r\n");
      //Exit immediately
      return;
   }

   //Successful processing
   osStrcpy(connection->response, "200 Command okay\r\n");
}


/**
 * @brief PASV command processing
 *
 * The PASV command requests the server to listen on a data port and
 * to wait for a connection rather than initiate one upon receipt of
 * a transfer command
 *
 * @param[in] connection Pointer to the client connection
 * @param[in] param Command line parameters
 **/

void ftpServerProcessPasv(FtpClientConnection *connection, char_t *param)
{
   error_t error;
   size_t n;
   char_t *p;
   IpAddr ipAddr;
   uint16_t port;
   FtpServerContext *context;

   //Point to the FTP server context
   context = connection->context;

   //Ensure the user is logged in
   if(!connection->userLoggedIn)
   {
      //Format response message
      osStrcpy(connection->response, "530 Not logged in\r\n");
      //Exit immediately
      return;
   }

   //Close the data connection, if any
   ftpServerCloseDataChannel(connection);

   //Get the next passive port number to be used
   port = ftpServerGetPassivePort(context);

   //Start of exception handling block
   do
   {
      //Open data socket
      connection->dataChannel.socket = socketOpen(SOCKET_TYPE_STREAM,
         SOCKET_IP_PROTO_TCP);
      //Failed to open socket?
      if(!connection->dataChannel.socket)
      {
         //Report an error
         error = ERROR_OPEN_FAILED;
         break;
      }

      //Force the socket to operate in non-blocking mode
      error = socketSetTimeout(connection->dataChannel.socket, 0);
      //Any error to report?
      if(error)
         break;

      //Adjust the size of the TX buffer
      error = socketSetTxBufferSize(connection->dataChannel.socket,
         FTP_SERVER_MAX_TCP_BUFFER_SIZE);
      //Any error to report?
      if(error)
         break;

      //Adjust the size of the RX buffer
      error = socketSetRxBufferSize(connection->dataChannel.socket,
         FTP_SERVER_MAX_TCP_BUFFER_SIZE);
      //Any error to report?
      if(error)
         break;

      //Associate the socket with the relevant interface
      error = socketBindToInterface(connection->dataChannel.socket,
         connection->interface);
      //Unable to bind the socket to the desired interface?
      if(error)
         break;

      //Bind the socket to the passive port number
      error = socketBind(connection->dataChannel.socket, &IP_ADDR_ANY,
         port);
      //Failed to bind the socket to the desired port?
      if(error)
         break;

      //Place the data socket in the listening state
      error = socketListen(connection->dataChannel.socket, 1);
      //Any error to report?
      if(error)
         break;

      //Retrieve the IP address of the client
      error = socketGetRemoteAddr(connection->controlChannel.socket, &ipAddr,
         NULL);
      //Any error to report?
      if(error)
         break;

      //PASV command is limited to IPv4
      if(ipAddr.length != sizeof(Ipv4Addr))
      {
         //Report an error
         error = ERROR_INVALID_ADDRESS;
         break;
      }

      //If the server is behind a NAT router, make sure the server knows
      //its external IP address
      if(!ipv4IsOnLink(connection->interface, ipAddr.ipv4Addr) &&
         context->settings.publicIpv4Addr != IPV4_UNSPECIFIED_ADDR)
      {
         //The server must return the public IP address in the PASV reply
         ipAddr.ipv4Addr = context->settings.publicIpv4Addr;
      }
      else
      {
         //The server must return its own IP address in the PASV reply
         error = socketGetLocalAddr(connection->controlChannel.socket,
            &ipAddr, NULL);
         //Any error to report?
         if(error)
            break;

         //PASV command is limited to IPv4
         if(ipAddr.length != sizeof(Ipv4Addr))
         {
            //Report an error
            error = ERROR_INVALID_ADDRESS;
            break;
         }
      }

      //End of exception handling block
   } while(0);

   //Check status code
   if(!error)
   {
      //Use passive data transfer
      connection->passiveMode = TRUE;
      //Update data connection state
      connection->dataChannel.state = FTP_CHANNEL_STATE_LISTEN;

      //Format response message
      n = osSprintf(connection->response, "227 Entering passive mode (");

      //Append host address
      ipv4AddrToString(ipAddr.ipv4Addr, connection->response + n);
      //Change dots to commas
      strReplaceChar(connection->response, '.', ',');

      //Point to the end of the resulting string
      p = connection->response + osStrlen(connection->response);
      //Append port number
      osSprintf(p, ",%" PRIu8 ",%" PRIu8 ")\r\n", MSB(port), LSB(port));
   }
   else
   {
      //Clean up side effects
      ftpServerCloseDataChannel(connection);

      //Format response message
      osStrcpy(connection->response, "425 Can't enter passive mode\r\n");
   }
}


/**
 * @brief EPSV command processing
 *
 * The EPSV command requests that a server listen on a data port and
 * wait for a connection
 *
 * @param[in] connection Pointer to the client connection
 * @param[in] param Command line parameters
 **/

void ftpServerProcessEpsv(FtpClientConnection *connection, char_t *param)
{
   error_t error;
   uint16_t port;
   FtpServerContext *context;

   //Point to the FTP server context
   context = connection->context;

   //Ensure the user is logged in
   if(!connection->userLoggedIn)
   {
      //Format response message
      osStrcpy(connection->response, "530 Not logged in\r\n");
      //Exit immediately
      return;
   }

   //Close the data connection, if any
   ftpServerCloseDataChannel(connection);

   //Get the next passive port number to be used
   port = ftpServerGetPassivePort(context);

   //Start of exception handling block
   do
   {
      //Open data socket
      connection->dataChannel.socket = socketOpen(SOCKET_TYPE_STREAM,
         SOCKET_IP_PROTO_TCP);
      //Failed to open socket?
      if(!connection->dataChannel.socket)
      {
         //Report an error
         error = ERROR_OPEN_FAILED;
         //Exit immediately
         break;
      }

      //Force the socket to operate in non-blocking mode
      error = socketSetTimeout(connection->dataChannel.socket, 0);
      //Any error to report?
      if(error)
         break;

      //Adjust the size of the TX buffer
      error = socketSetTxBufferSize(connection->dataChannel.socket,
         FTP_SERVER_MAX_TCP_BUFFER_SIZE);
      //Any error to report?
      if(error)
         break;

      //Adjust the size of the RX buffer
      error = socketSetRxBufferSize(connection->dataChannel.socket,
         FTP_SERVER_MAX_TCP_BUFFER_SIZE);
      //Any error to report?
      if(error)
         break;

      //Associate the socket with the relevant interface
      error = socketBindToInterface(connection->dataChannel.socket,
         connection->interface);
      //Unable to bind the socket to the desired interface?
      if(error)
         break;

      //Bind the socket to the passive port number
      error = socketBind(connection->dataChannel.socket, &IP_ADDR_ANY, port);
      //Failed to bind the socket to the desired port?
      if(error)
         break;

      //Place the data socket in the listening state
      error = socketListen(connection->dataChannel.socket, 1);
      //Any error to report?
      if(error)
         break;

      //End of exception handling block
   } while(0);

   //Check status code
   if(!error)
   {
      //Use passive data transfer
      connection->passiveMode = TRUE;
      //Update data connection state
      connection->dataChannel.state = FTP_CHANNEL_STATE_LISTEN;

      //The response code for entering passive mode using an extended address
      //must be 229
      osSprintf(connection->response, "229 Entering extended passive mode (|||"
         "%" PRIu16 "|)\r\n", port);
   }
   else
   {
      //Clean up side effects
      ftpServerCloseDataChannel(connection);

      //Format response message
      osStrcpy(connection->response, "425 Can't enter passive mode\r\n");
   }
}


/**
 * @brief ABOR command processing
 *
 * The ABOR command tells the server to abort the previous FTP
 * service command and any associated transfer of data
 *
 * @param[in] connection Pointer to the client connection
 * @param[in] param Command line parameters
 **/

void ftpServerProcessAbor(FtpClientConnection *connection, char_t *param)
{
   //There are two cases to consider upon receipt of this command
   if(connection->dataChannel.state == FTP_CHANNEL_STATE_CLOSED)
   {
      //If the FTP service command was already completed, the server closes
      //the data connection (if it is open)...
      ftpServerCloseDataChannel(connection);

      //...and responds with a 226 reply, indicating that the abort command
      //was successfully processed
      osStrcpy(connection->response, "226 Abort command successful\r\n");
   }
   else
   {
      //If the FTP service command is still in progress, the server aborts
      //the FTP service in progress and closes the data connection...
      ftpServerCloseDataChannel(connection);

      //...returning a 426 reply to indicate that the service request
      //terminated abnormally
      osStrcpy(connection->response, "426 Connection closed; transfer aborted\r\n");

      //The server then sends a 226 reply, indicating that the abort command
      //was successfully processed
      osStrcat(connection->response, "226 Abort command successful\r\n");
   }

   //Release previously allocated resources
   if(connection->file != NULL)
   {
      fsCloseFile(connection->file);
      connection->file = NULL;
   }

   if(connection->dir != NULL)
   {
      fsCloseDir(connection->dir);
      connection->dir = NULL;
   }
}


/**
 * @brief PWD command processing
 *
 * The PWD command causes the name of the current working
 * directory to be returned in the reply
 *
 * @param[in] connection Pointer to the client connection
 * @param[in] param Command line parameters
 **/

void ftpServerProcessPwd(FtpClientConnection *connection, char_t *param)
{
   //Ensure the user is logged in
   if(!connection->userLoggedIn)
   {
      //Format response message
      osStrcpy(connection->response, "530 Not logged in\r\n");
      //Exit immediately
      return;
   }

   //A successful PWD command uses the 257 reply code
   osSprintf(connection->response, "257 \"%s\" is current directory\r\n",
      ftpServerStripHomeDir(connection, connection->currentDir));
}


/**
 * @brief CWD command processing
 *
 * The CWD command allows the user to work with a different
 * directory
 *
 * @param[in] connection Pointer to the client connection
 * @param[in] param Command line parameters
 **/

void ftpServerProcessCwd(FtpClientConnection *connection, char_t *param)
{
   error_t error;
   uint_t perm;

   //Ensure the user is logged in
   if(!connection->userLoggedIn)
   {
      //Format response message
      osStrcpy(connection->response, "530 Not logged in\r\n");
      //Exit immediately
      return;
   }

   //The argument specifies the pathname
   if(*param == '\0')
   {
      //The argument is missing
      osStrcpy(connection->response, "501 Missing parameter\r\n");
      //Exit immediately
      return;
   }

   //Retrieve the full pathname
   error = ftpServerGetPath(connection, param,
      connection->path, FTP_SERVER_MAX_PATH_LEN);

   //Make sure the pathname is valid
   if(error)
   {
      //Report an error
      osStrcpy(connection->response, "501 Invalid parameter\r\n");
      //Exit immediately
      return;
   }

   //Retrieve permissions for the specified directory
   perm = ftpServerGetFilePermissions(connection, connection->path);

   //Insufficient access rights?
   if((perm & FTP_FILE_PERM_READ) == 0)
   {
      //Report an error
      osStrcpy(connection->response, "550 Access denied\r\n");
      //Exit immediately
      return;
   }

   //Make sure the specified directory exists
   if(!fsDirExists(connection->path))
   {
      //Report an error
      osStrcpy(connection->response, "550 Directory not found\r\n");
      //Exit immediately
      return;
   }

   //Change current working directory
   osStrcpy(connection->currentDir, connection->path);

   //A successful PWD command uses the 250 reply code
   osSprintf(connection->response, "250 Directory changed to %s\r\n",
      ftpServerStripHomeDir(connection, connection->currentDir));
}


/**
 * @brief CDUP command processing
 *
 * The CDUP command allows the user to change to the parent directory
 *
 * @param[in] connection Pointer to the client connection
 * @param[in] param Command line parameters
 **/

void ftpServerProcessCdup(FtpClientConnection *connection, char_t *param)
{
   uint_t perm;

   //Ensure the user is logged in
   if(!connection->userLoggedIn)
   {
      //Format response message
      osStrcpy(connection->response, "530 Not logged in\r\n");
      //Exit immediately
      return;
   }

   //Get current directory
   osStrcpy(connection->path, connection->currentDir);

   //Change to the parent directory
   pathCombine(connection->path, "..", FTP_SERVER_MAX_PATH_LEN);
   pathCanonicalize(connection->path);

   //Retrieve permissions for the directory
   perm = ftpServerGetFilePermissions(connection, connection->path);

   //Check access rights
   if((perm & FTP_FILE_PERM_READ) != 0)
   {
      //Update current directory
      osStrcpy(connection->currentDir, connection->path);
   }

   //A successful PWD command uses the 250 reply code
   osSprintf(connection->response, "250 Directory changed to %s\r\n",
      ftpServerStripHomeDir(connection, connection->currentDir));
}


/**
 * @brief LIST command processing
 *
 * The LIST command is used to list the content of a directory
 *
 * @param[in] connection Pointer to the client connection
 * @param[in] param Command line parameters
 **/

void ftpServerProcessList(FtpClientConnection *connection, char_t *param)
{
   error_t error;
   uint_t perm;

   //Ensure the user is logged in
   if(!connection->userLoggedIn)
   {
      //Format response message
      osStrcpy(connection->response, "530 Not logged in\r\n");
      //Exit immediately
      return;
   }

   //Any option flags
   while(*param == '-')
   {
      //Skip option flags
      while(*param != ' ' && *param != '\0')
      {
         param++;
      }

      //Skip whitespace characters
      while(*param == ' ')
      {
         param++;
      }
   }

   //The pathname is optional
   if(*param == '\0')
   {
      //Use current directory if no pathname is specified
      osStrcpy(connection->path, connection->currentDir);
   }
   else
   {
      //Retrieve the full pathname
      error = ftpServerGetPath(connection, param,
         connection->path, FTP_SERVER_MAX_PATH_LEN);

      //Any error to report?
      if(error)
      {
         //The specified pathname is not valid...
         osStrcpy(connection->response, "501 Invalid parameter\r\n");
         //Exit immediately
         return;
      }
   }

   //Retrieve permissions for the specified directory
   perm = ftpServerGetFilePermissions(connection, connection->path);

   //Insufficient access rights?
   if((perm & FTP_FILE_PERM_READ) == 0)
   {
      //Report an error
      osStrcpy(connection->response, "550 Access denied\r\n");
      //Exit immediately
      return;
   }

   //Open the specified directory for reading
   connection->dir = fsOpenDir(connection->path);

   //Failed to open the directory?
   if(!connection->dir)
   {
      //Report an error
      osStrcpy(connection->response, "550 Directory not found\r\n");
      //Exit immediately
      return;
   }

   //Check current data transfer mode
   if(connection->passiveMode)
   {
      //Check whether the data connection is already opened
      if(connection->dataChannel.state == FTP_CHANNEL_STATE_IDLE)
         connection->dataChannel.state = FTP_CHANNEL_STATE_SEND;
   }
   else
   {
      //Open the data connection
      error = ftpServerOpenDataChannel(connection);

      //Any error to report?
      if(error)
      {
         //Clean up side effects
         fsCloseDir(connection->dir);
         //Format response
         osStrcpy(connection->response, "450 Can't open data connection\r\n");
         //Exit immediately
         return;
      }

      //The data connection is ready to send data
      connection->dataChannel.state = FTP_CHANNEL_STATE_SEND;
   }

   //Flush transmission buffer
   connection->bufferLength = 0;
   connection->bufferPos = 0;

   //LIST command is being processed
   connection->controlChannel.state = FTP_CHANNEL_STATE_LIST;

   //Format response message
   osStrcpy(connection->response, "150 Opening data connection\r\n");
}


/**
 * @brief NLST command processing
 *
 * The NLST command is used to list the content of a directory
 *
 * @param[in] connection Pointer to the client connection
 * @param[in] param Command line parameters
 **/

void ftpServerProcessNlst(FtpClientConnection *connection, char_t *param)
{
   error_t error;
   uint_t perm;

   //Ensure the user is logged in
   if(!connection->userLoggedIn)
   {
      //Format response message
      osStrcpy(connection->response, "530 Not logged in\r\n");
      //Exit immediately
      return;
   }

   //Any option flags
   while(*param == '-')
   {
      //Skip option flags
      while(*param != ' ' && *param != '\0')
      {
         param++;
      }

      //Skip whitespace characters
      while(*param == ' ')
      {
         param++;
      }
   }

   //The pathname is optional
   if(*param == '\0')
   {
      //Use current directory if no pathname is specified
      osStrcpy(connection->path, connection->currentDir);
   }
   else
   {
      //Retrieve the full pathname
      error = ftpServerGetPath(connection, param, connection->path,
         FTP_SERVER_MAX_PATH_LEN);

      //Any error to report?
      if(error)
      {
         //The specified pathname is not valid...
         osStrcpy(connection->response, "501 Invalid parameter\r\n");
         //Exit immediately
         return;
      }
   }

   //Retrieve permissions for the specified directory
   perm = ftpServerGetFilePermissions(connection, connection->path);

   //Insufficient access rights?
   if((perm & FTP_FILE_PERM_READ) == 0)
   {
      //Report an error
      osStrcpy(connection->response, "550 Access denied\r\n");
      //Exit immediately
      return;
   }

   //Open the specified directory for reading
   connection->dir = fsOpenDir(connection->path);

   //Failed to open the directory?
   if(!connection->dir)
   {
      //Report an error
      osStrcpy(connection->response, "550 Directory not found\r\n");
      //Exit immediately
      return;
   }

   //Check current data transfer mode
   if(connection->passiveMode)
   {
      //Check whether the data connection is already opened
      if(connection->dataChannel.state == FTP_CHANNEL_STATE_IDLE)
         connection->dataChannel.state = FTP_CHANNEL_STATE_SEND;
   }
   else
   {
      //Open the data connection
      error = ftpServerOpenDataChannel(connection);

      //Any error to report?
      if(error)
      {
         //Clean up side effects
         fsCloseDir(connection->dir);
         //Format response
         osStrcpy(connection->response, "450 Can't open data connection\r\n");
         //Exit immediately
         return;
      }

      //The data connection is ready to send data
      connection->dataChannel.state = FTP_CHANNEL_STATE_SEND;
   }

   //Flush transmission buffer
   connection->bufferLength = 0;
   connection->bufferPos = 0;

   //NLST command is being processed
   connection->controlChannel.state = FTP_CHANNEL_STATE_NLST;

   //Format response message
   osStrcpy(connection->response, "150 Opening data connection\r\n");
}


/**
 * @brief MKD command processing
 *
 * The MKD command causes the directory specified in the pathname
 * to be created as a directory
 *
 * @param[in] connection Pointer to the client connection
 * @param[in] param Command line parameters
 **/

void ftpServerProcessMkd(FtpClientConnection *connection, char_t *param)
{
   error_t error;
   uint_t perm;

   //Ensure the user is logged in
   if(!connection->userLoggedIn)
   {
      //Format response message
      osStrcpy(connection->response, "530 Not logged in\r\n");
      //Exit immediately
      return;
   }

   //The argument specifies the pathname
   if(*param == '\0')
   {
      //The argument is missing
      osStrcpy(connection->response, "501 Missing parameter\r\n");
      //Exit immediately
      return;
   }

   //Retrieve the full pathname
   error = ftpServerGetPath(connection, param,
      connection->path, FTP_SERVER_MAX_PATH_LEN);

   //Any error to report?
   if(error)
   {
      //The specified pathname is not valid...
      osStrcpy(connection->response, "501 Invalid parameter\r\n");
      //Exit immediately
      return;
   }

   //Retrieve permissions for the specified directory
   perm = ftpServerGetFilePermissions(connection, connection->path);

   //Insufficient access rights?
   if((perm & FTP_FILE_PERM_WRITE) == 0)
   {
      //Report an error
      osStrcpy(connection->response, "550 Access denied\r\n");
      //Exit immediately
      return;
   }

   //Create the specified directory
   error = fsCreateDir(connection->path);

   //Any error to report?
   if(error)
   {
      //The specified pathname is not valid...
      osStrcpy(connection->response, "550 Can't create directory\r\n");
      //Exit immediately
      return;
   }

   //The specified directory was successfully created
   osSprintf(connection->response, "257 \"%s\" created\r\n",
      ftpServerStripHomeDir(connection, connection->path));
}


/**
 * @brief RMD command processing
 *
 * The RMD command causes the directory specified in the pathname
 * to be removed
 *
 * @param[in] connection Pointer to the client connection
 * @param[in] param Command line parameters
 **/

void ftpServerProcessRmd(FtpClientConnection *connection, char_t *param)
{
   error_t error;
   uint_t perm;

   //Ensure the user is logged in
   if(!connection->userLoggedIn)
   {
      //Format response message
      osStrcpy(connection->response, "530 Not logged in\r\n");
      //Exit immediately
      return;
   }

   //The argument specifies the directory to be removed
   if(*param == '\0')
   {
      //The argument is missing
      osStrcpy(connection->response, "501 Missing parameter\r\n");
      //Exit immediately
      return;
   }

   //Retrieve the full pathname of the directory
   error = ftpServerGetPath(connection, param,
      connection->path, FTP_SERVER_MAX_PATH_LEN);

   //Any error to report?
   if(error)
   {
      //The specified pathname is not valid...
      osStrcpy(connection->response, "501 Invalid parameter\r\n");
      //Exit immediately
      return;
   }

   //Retrieve permissions for the specified directory
   perm = ftpServerGetFilePermissions(connection, connection->path);

   //Insufficient access rights?
   if((perm & FTP_FILE_PERM_WRITE) == 0)
   {
      //Report an error
      osStrcpy(connection->response, "550 Access denied\r\n");
      //Exit immediately
      return;
   }

   //Remove the specified directory
   error = fsRemoveDir(connection->path);

   //Any error to report?
   if(error)
   {
      //The specified directory cannot be deleted...
      osStrcpy(connection->response, "550 Can't remove directory\r\n");
      //Exit immediately
      return;
   }

   //The specified directory was successfully removed
   osStrcpy(connection->response, "250 Directory removed\r\n");
}


/**
 * @brief SIZE command processing
 *
 * The SIZE command is used to obtain the transfer size of the specified file
 *
 * @param[in] connection Pointer to the client connection
 * @param[in] param Command line parameters
 **/

void ftpServerProcessSize(FtpClientConnection *connection, char_t *param)
{
   error_t error;
   uint_t perm;
   uint32_t size;

   //Ensure the user is logged in
   if(!connection->userLoggedIn)
   {
      //Format response message
      osStrcpy(connection->response, "530 Not logged in\r\n");
      //Exit immediately
      return;
   }

   //The argument specifies the pathname of the file
   if(*param == '\0')
   {
      //The argument is missing
      osStrcpy(connection->response, "501 Missing parameter\r\n");
      //Exit immediately
      return;
   }

   //Retrieve the full pathname
   error = ftpServerGetPath(connection, param,
      connection->path, FTP_SERVER_MAX_PATH_LEN);

   //Any error to report?
   if(error)
   {
      //The specified pathname is not valid...
      osStrcpy(connection->response, "501 Invalid parameter\r\n");
      //Exit immediately
      return;
   }

   //Retrieve permissions for the specified directory
   perm = ftpServerGetFilePermissions(connection, connection->path);

   //Insufficient access rights?
   if((perm & FTP_FILE_PERM_LIST) == 0 && (perm & FTP_FILE_PERM_READ) == 0)
   {
      //Report an error
      osStrcpy(connection->response, "550 Access denied\r\n");
      //Exit immediately
      return;
   }

   //Retrieve the size of the specified file
   error = fsGetFileSize(connection->path, &size);

   //Any error to report?
   if(error)
   {
      //Report an error
      osStrcpy(connection->response, "550 File not found\r\n");
      //Exit immediately
      return;
   }

   //Format response message
   osSprintf(connection->response, "213 %" PRIu32 "\r\n", size);
}


/**
 * @brief RETR command processing
 *
 * The RETR command is used to retrieve the content of the specified file
 *
 * @param[in] connection Pointer to the client connection
 * @param[in] param Command line parameters
 **/

void ftpServerProcessRetr(FtpClientConnection *connection, char_t *param)
{
   error_t error;
   uint_t perm;

   //Ensure the user is logged in
   if(!connection->userLoggedIn)
   {
      //Format response message
      osStrcpy(connection->response, "530 Not logged in\r\n");
      //Exit immediately
      return;
   }

   //The argument specifies the pathname of the file to read
   if(*param == '\0')
   {
      //The argument is missing
      osStrcpy(connection->response, "501 Missing parameter\r\n");
      //Exit immediately
      return;
   }

   //Retrieve the full pathname
   error = ftpServerGetPath(connection, param,
      connection->path, FTP_SERVER_MAX_PATH_LEN);

   //Any error to report?
   if(error)
   {
      //The specified pathname is not valid...
      osStrcpy(connection->response, "501 Invalid parameter\r\n");
      //Exit immediately
      return;
   }

   //Retrieve permissions for the specified directory
   perm = ftpServerGetFilePermissions(connection, connection->path);

   //Insufficient access rights?
   if((perm & FTP_FILE_PERM_READ) == 0)
   {
      //Report an error
      osStrcpy(connection->response, "550 Access denied\r\n");
      //Exit immediately
      return;
   }

   //Open specified file for reading
   connection->file = fsOpenFile(connection->path, FS_FILE_MODE_READ);

   //Failed to open the file?
   if(!connection->file)
   {
      //Report an error
      osStrcpy(connection->response, "550 File not found\r\n");
      //Exit immediately
      return;
   }

   //Check current data transfer mode
   if(connection->passiveMode)
   {
      //Check whether the data connection is already opened
      if(connection->dataChannel.state == FTP_CHANNEL_STATE_IDLE)
         connection->dataChannel.state = FTP_CHANNEL_STATE_SEND;
   }
   else
   {
      //Open the data connection
      error = ftpServerOpenDataChannel(connection);

      //Any error to report?
      if(error)
      {
         //Clean up side effects
         fsCloseFile(connection->file);
         //Format response
         osStrcpy(connection->response, "450 Can't open data connection\r\n");
         //Exit immediately
         return;
      }

      //The data connection is ready to send data
      connection->dataChannel.state = FTP_CHANNEL_STATE_SEND;
   }

   //Flush transmission buffer
   connection->bufferLength = 0;
   connection->bufferPos = 0;

   //RETR command is being processed
   connection->controlChannel.state = FTP_CHANNEL_STATE_RETR;

   //Format response message
   osStrcpy(connection->response, "150 Opening data connection\r\n");
}


/**
 * @brief STOR command processing
 *
 * The STOR command is used to store data to the specified file
 *
 * @param[in] connection Pointer to the client connection
 * @param[in] param Command line parameters
 **/

void ftpServerProcessStor(FtpClientConnection *connection, char_t *param)
{
   error_t error;
   uint_t perm;

   //Ensure the user is logged in
   if(!connection->userLoggedIn)
   {
      //Format response message
      osStrcpy(connection->response, "530 Not logged in\r\n");
      //Exit immediately
      return;
   }

   //The argument specifies the pathname of the file to written
   if(*param == '\0')
   {
      //The argument is missing
      osStrcpy(connection->response, "501 Missing parameter\r\n");
      //Exit immediately
      return;
   }

   //Retrieve the full pathname
   error = ftpServerGetPath(connection, param,
      connection->path, FTP_SERVER_MAX_PATH_LEN);

   //Any error to report?
   if(error)
   {
      //The specified pathname is not valid...
      osStrcpy(connection->response, "501 Invalid parameter\r\n");
      //Exit immediately
      return;
   }

   //Retrieve permissions for the specified directory
   perm = ftpServerGetFilePermissions(connection, connection->path);

   //Insufficient access rights?
   if((perm & FTP_FILE_PERM_WRITE) == 0)
   {
      //Report an error
      osStrcpy(connection->response, "550 Access denied\r\n");
      //Exit immediately
      return;
   }

   //Open specified file for writing
   connection->file = fsOpenFile(connection->path,
      FS_FILE_MODE_WRITE | FS_FILE_MODE_CREATE | FS_FILE_MODE_TRUNC);

   //Failed to open the file?
   if(!connection->file)
   {
      //Report an error
      osStrcpy(connection->response, "550 File not found\r\n");
      //Exit immediately
      return;
   }

   //Check current data transfer mode
   if(connection->passiveMode)
   {
      //Check whether the data connection is already opened
      if(connection->dataChannel.state == FTP_CHANNEL_STATE_IDLE)
         connection->dataChannel.state = FTP_CHANNEL_STATE_RECEIVE;
   }
   else
   {
      //Open the data connection
      error = ftpServerOpenDataChannel(connection);

      //Any error to report?
      if(error)
      {
         //Clean up side effects
         fsCloseFile(connection->file);
         //Format response
         osStrcpy(connection->response, "450 Can't open data connection\r\n");
         //Exit immediately
         return;
      }

      //The data connection is ready to receive data
      connection->dataChannel.state = FTP_CHANNEL_STATE_RECEIVE;
   }

   //Flush reception buffer
   connection->bufferLength = 0;
   connection->bufferPos = 0;

   //STOR command is being processed
   connection->controlChannel.state = FTP_CHANNEL_STATE_STOR;

   //Format response message
   osStrcpy(connection->response, "150 Opening data connection\r\n");
}


/**
 * @brief APPE command processing
 *
 * The APPE command is used to append data to the specified file
 *
 * @param[in] connection Pointer to the client connection
 * @param[in] param Command line parameters
 **/

void ftpServerProcessAppe(FtpClientConnection *connection, char_t *param)
{
   error_t error;
   uint_t perm;

   //Ensure the user is logged in
   if(!connection->userLoggedIn)
   {
      //Format response message
      osStrcpy(connection->response, "530 Not logged in\r\n");
      //Exit immediately
      return;
   }

   //The argument specifies the pathname of the file to written
   if(*param == '\0')
   {
      //The argument is missing
      osStrcpy(connection->response, "501 Missing parameter\r\n");
      //Exit immediately
      return;
   }

   //Retrieve the full pathname
   error = ftpServerGetPath(connection, param,
      connection->path, FTP_SERVER_MAX_PATH_LEN);

   //Any error to report?
   if(error)
   {
      //The specified pathname is not valid...
      osStrcpy(connection->response, "501 Invalid parameter\r\n");
      //Exit immediately
      return;
   }

   //Retrieve permissions for the specified directory
   perm = ftpServerGetFilePermissions(connection, connection->path);

   //Insufficient access rights?
   if((perm & FTP_FILE_PERM_WRITE) == 0)
   {
      //Report an error
      osStrcpy(connection->response, "550 Access denied\r\n");
      //Exit immediately
      return;
   }

   //Open specified file for writing
   connection->file = fsOpenFile(connection->path,
      FS_FILE_MODE_WRITE | FS_FILE_MODE_CREATE);

   //Failed to open the file?
   if(!connection->file)
   {
      //Report an error
      osStrcpy(connection->response, "550 File not found\r\n");
      //Exit immediately
      return;
   }

   //Move to the end of the file
   error = fsSeekFile(connection->file, 0, FS_SEEK_END);

   //Any error to report?
   if(error)
   {
      //Clean up side effects
      fsCloseFile(connection->file);
      //Format response
      osStrcpy(connection->response, "550 File unavailable\r\n");
   }

   //Check current data transfer mode
   if(connection->passiveMode)
   {
      //Check whether the data connection is already opened
      if(connection->dataChannel.state == FTP_CHANNEL_STATE_IDLE)
         connection->dataChannel.state = FTP_CHANNEL_STATE_RECEIVE;
   }
   else
   {
      //Open the data connection
      error = ftpServerOpenDataChannel(connection);

      //Any error to report?
      if(error)
      {
         //Clean up side effects
         fsCloseFile(connection->file);
         //Format response
         osStrcpy(connection->response, "450 Can't open data connection\r\n");
         //Exit immediately
         return;
      }

      //The data connection is ready to receive data
      connection->dataChannel.state = FTP_CHANNEL_STATE_RECEIVE;
   }

   //Flush reception buffer
   connection->bufferLength = 0;
   connection->bufferPos = 0;

   //APPE command is being processed
   connection->controlChannel.state = FTP_CHANNEL_STATE_APPE;

   //Format response message
   osStrcpy(connection->response, "150 Opening data connection\r\n");
}


/**
 * @brief RNFR command processing
 *
 * The RNFR command specifies the old pathname of the file which is
 * to be renamed
 *
 * @param[in] connection Pointer to the client connection
 * @param[in] param Command line parameters
 **/

void ftpServerProcessRnfr(FtpClientConnection *connection, char_t *param)
{
   error_t error;
   uint_t perm;

   //Ensure the user is logged in
   if(!connection->userLoggedIn)
   {
      //Format response message
      osStrcpy(connection->response, "530 Not logged in\r\n");
      //Exit immediately
      return;
   }

   //The argument specifies the file to be renamed
   if(*param == '\0')
   {
      //The argument is missing
      osStrcpy(connection->response, "501 Missing parameter\r\n");
      //Exit immediately
      return;
   }

   //Retrieve the full pathname
   error = ftpServerGetPath(connection, param,
      connection->path, FTP_SERVER_MAX_PATH_LEN);

   //Any error to report?
   if(error)
   {
      //The specified pathname is not valid...
      osStrcpy(connection->response, "501 Invalid parameter\r\n");
      //Exit immediately
      return;
   }

   //Retrieve permissions for the specified directory
   perm = ftpServerGetFilePermissions(connection, connection->path);

   //Insufficient access rights?
   if((perm & FTP_FILE_PERM_WRITE) == 0)
   {
      //Report an error
      osStrcpy(connection->response, "550 Access denied\r\n");
      //Exit immediately
      return;
   }

   //Make sure the file exists
   if(!fsFileExists(connection->path) && !fsDirExists(connection->path))
   {
      //No such file or directory...
      osStrcpy(connection->response, "550 File not found\r\n");
      //Exit immediately
      return;
   }

   //This command must be immediately followed by a RNTO command
   connection->controlChannel.state = FTP_CHANNEL_STATE_RNFR;
   //Format the response message
   osStrcpy(connection->response, "350 File exists, ready for destination name\r\n");
}


/**
 * @brief RNTO command processing
 *
 * The RNTO command specifies the new pathname of the file specified
 * in the immediately preceding RNFR command
 *
 * @param[in] connection Pointer to the client connection
 * @param[in] param Command line parameters
 **/

void ftpServerProcessRnto(FtpClientConnection *connection, char_t *param)
{
   error_t error;
   uint_t perm;
   char_t newPath[FTP_SERVER_MAX_PATH_LEN];

   //Ensure the user is logged in
   if(!connection->userLoggedIn)
   {
      //Format response message
      osStrcpy(connection->response, "530 Not logged in\r\n");
      //Exit immediately
      return;
   }

   //This command must immediately follow a RNFR command
   if(connection->controlChannel.state != FTP_CHANNEL_STATE_RNFR)
   {
      //Switch to idle state
      connection->controlChannel.state = FTP_CHANNEL_STATE_IDLE;
      //Report an error
      osStrcpy(connection->response, "503 Bad sequence of commands\r\n");
      //Exit immediately
      return;
   }

   //Switch to idle state
   connection->controlChannel.state = FTP_CHANNEL_STATE_IDLE;

   //The argument specifies the new pathname
   if(*param == '\0')
   {
      //The argument is missing
      osStrcpy(connection->response, "501 Missing parameter\r\n");
      //Exit immediately
      return;
   }

   //Retrieve the full pathname
   error = ftpServerGetPath(connection, param,
      newPath, FTP_SERVER_MAX_PATH_LEN);

   //Any error to report?
   if(error)
   {
      //The specified pathname is not valid...
      osStrcpy(connection->response, "501 Invalid parameter\r\n");
      //Exit immediately
      return;
   }

   //Retrieve permissions for the specified directory
   perm = ftpServerGetFilePermissions(connection, newPath);

   //Insufficient access rights?
   if((perm & FTP_FILE_PERM_WRITE) == 0)
   {
      //Report an error
      osStrcpy(connection->response, "550 Access denied\r\n");
      //Exit immediately
      return;
   }

   //Check whether the file name already exists
   if(fsFileExists(newPath) || fsDirExists(newPath))
   {
      //Report an error
      osStrcpy(connection->response, "550 File already exists\r\n");
      //Exit immediately
      return;
   }

   //Rename the specified file
   error = fsRenameFile(connection->path, newPath);

   //Any error to report?
   if(error)
   {
      //The specified file cannot be renamed
      osStrcpy(connection->response, "550 Can't rename file\r\n");
      //Exit immediately
      return;
   }

   //The specified file was successfully deleted
   osStrcpy(connection->response, "250 File renamed\r\n");
}


/**
 * @brief DELE command processing
 *
 * The DELE command causes the file specified in the pathname to be
 * deleted at the server site
 *
 * @param[in] connection Pointer to the client connection
 * @param[in] param Command line parameters
 **/

void ftpServerProcessDele(FtpClientConnection *connection, char_t *param)
{
   error_t error;
   uint_t perm;

   //Ensure the user is logged in
   if(!connection->userLoggedIn)
   {
      //Format response message
      osStrcpy(connection->response, "530 Not logged in\r\n");
      //Exit immediately
      return;
   }

   //The argument specifies the file to be deleted
   if(*param == '\0')
   {
      //The argument is missing
      osStrcpy(connection->response, "501 Missing parameter\r\n");
      //Exit immediately
      return;
   }

   //Retrieve the full pathname of the file
   error = ftpServerGetPath(connection, param,
      connection->path, FTP_SERVER_MAX_PATH_LEN);

   //Any error to report?
   if(error)
   {
      //The specified pathname is not valid...
      osStrcpy(connection->response, "501 Invalid parameter\r\n");
      //Exit immediately
      return;
   }

   //Retrieve permissions for the specified directory
   perm = ftpServerGetFilePermissions(connection, connection->path);

   //Insufficient access rights?
   if((perm & FTP_FILE_PERM_WRITE) == 0)
   {
      //Report an error
      osStrcpy(connection->response, "550 Access denied\r\n");
      //Exit immediately
      return;
   }

   //Delete the specified file
   error = fsDeleteFile(connection->path);

   //Any error to report?
   if(error)
   {
      //The specified file cannot be deleted...
      osStrcpy(connection->response, "550 Can't delete file\r\n");
      //Exit immediately
      return;
   }

   //The specified file was successfully deleted
   osStrcpy(connection->response, "250 File deleted\r\n");
}


/**
 * @brief Unknown command processing
 * @param[in] connection Pointer to the client connection
 * @param[in] param Command line parameters
 **/

void ftpServerProcessUnknownCmd(FtpClientConnection *connection, char_t *param)
{
   error_t error;
   FtpServerContext *context;

   //Point to the FTP server context
   context = connection->context;

   //Invoke user-defined callback, if any
   if(context->settings.unknownCommandCallback != NULL)
   {
      //Custom command processing
      error = context->settings.unknownCommandCallback(connection,
         connection->command, param);
   }
   else
   {
      //Report an error
      error = ERROR_INVALID_COMMAND;
   }

   //Invalid command received?
   if(error == ERROR_INVALID_COMMAND)
   {
      //Format response message
      osStrcpy(connection->response, "500 Command unrecognized\r\n");
   }
}

#endif
