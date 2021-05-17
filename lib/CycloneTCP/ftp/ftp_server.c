/**
 * @file ftp_server.c
 * @brief FTP server (File Transfer Protocol)
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
 * - RFC 3659: Extensions to FTP
 * - RFC 2428: FTP Extensions for IPv6 and NATs
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

//Switch to the appropriate trace level
#define TRACE_LEVEL FTP_TRACE_LEVEL

//Dependencies
#include "ftp/ftp_server.h"
#include "ftp/ftp_server_control.h"
#include "ftp/ftp_server_data.h"
#include "ftp/ftp_server_misc.h"
#include "path.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (FTP_SERVER_SUPPORT == ENABLED)


/**
 * @brief Initialize settings with default values
 * @param[out] settings Structure that contains FTP server settings
 **/

void ftpServerGetDefaultSettings(FtpServerSettings *settings)
{
   //The FTP server is not bound to any interface
   settings->interface = NULL;

   //FTP command port number
   settings->port = FTP_PORT;
   //FTP data port number
   settings->dataPort = FTP_DATA_PORT;

   //Passive port range
   settings->passivePortMin = FTP_SERVER_PASSIVE_PORT_MIN;
   settings->passivePortMax = FTP_SERVER_PASSIVE_PORT_MAX;

   //Public IPv4 address to be used in PASV replies
   settings->publicIpv4Addr = IPV4_UNSPECIFIED_ADDR;

   //Default security mode (no security)
   settings->mode = FTP_SERVER_MODE_PLAINTEXT;

   //Client connections
   settings->maxConnections = 0;
   settings->connections = NULL;

   //Set root directory
   osStrcpy(settings->rootDir, "/");

   //Connection callback function
   settings->connectCallback = NULL;
   //Disconnection callback function
   settings->disconnectCallback = NULL;

#if (FTP_SERVER_TLS_SUPPORT == ENABLED)
   //TLS initialization callback function
   settings->tlsInitCallback = NULL;
#endif

   //User verification callback function
   settings->checkUserCallback = NULL;
   //Password verification callback function
   settings->checkPasswordCallback = NULL;
   //Callback used to retrieve file permissions
   settings->getFilePermCallback = NULL;
   //Unknown command callback function
   settings->unknownCommandCallback = NULL;
}


/**
 * @brief FTP server initialization
 * @param[in] context Pointer to the FTP server context
 * @param[in] settings FTP server specific settings
 * @return Error code
 **/

error_t ftpServerInit(FtpServerContext *context,
   const FtpServerSettings *settings)
{
   error_t error;
   uint_t i;

   //Debug message
   TRACE_INFO("Initializing FTP server...\r\n");

   //Ensure the parameters are valid
   if(context == NULL || settings == NULL)
      return ERROR_INVALID_PARAMETER;

   //Sanity check
   if(settings->passivePortMax <= settings->passivePortMin)
   {
      return ERROR_INVALID_PARAMETER;
   }

   //Invalid number of client connections?
   if(settings->maxConnections < 1 ||
      settings->maxConnections > FTP_SERVER_MAX_CONNECTIONS)
   {
      return ERROR_INVALID_PARAMETER;
   }

   //Invalid pointer?
   if(settings->connections == NULL)
      return ERROR_INVALID_PARAMETER;

   //Clear the FTP server context
   osMemset(context, 0, sizeof(FtpServerContext));

   //Save user settings
   context->settings = *settings;
   //Client connections
   context->connections = settings->connections;

   //Clean the root directory path
   pathCanonicalize(context->settings.rootDir);
   pathRemoveSlash(context->settings.rootDir);

   //Loop through client connections
   for(i = 0; i < context->settings.maxConnections; i++)
   {
      //Initialize the structure representing the client connection
      osMemset(&context->connections[i], 0, sizeof(FtpClientConnection));
   }

   //Initialize status code
   error = NO_ERROR;

   //Create an event object to poll the state of sockets
   if(!osCreateEvent(&context->event))
   {
      //Failed to create event
      error = ERROR_OUT_OF_RESOURCES;
   }

#if (FTP_SERVER_TLS_SUPPORT == ENABLED && TLS_TICKET_SUPPORT == ENABLED)
   //Check status code
   if(!error)
   {
      //Initialize ticket encryption context
      error = tlsInitTicketContext(&context->tlsTicketContext);
   }
#endif

   //Any error to report?
   if(error)
   {
      //Clean up side effects
      ftpServerDeinit(context);
   }

   //Return status code
   return error;
}


/**
 * @brief Start FTP server
 * @param[in] context Pointer to the FTP server context
 * @return Error code
 **/

error_t ftpServerStart(FtpServerContext *context)
{
   error_t error;
   OsTask *task;

   //Make sure the FTP server context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Debug message
   TRACE_INFO("Starting FTP server...\r\n");

   //Make sure the FTP server is not already running
   if(context->running)
      return ERROR_ALREADY_RUNNING;

   //Start of exception handling block
   do
   {
      //Open a TCP socket
      context->socket = socketOpen(SOCKET_TYPE_STREAM, SOCKET_IP_PROTO_TCP);
      //Failed to open socket?
      if(context->socket == NULL)
      {
         //Report an error
         error = ERROR_OPEN_FAILED;
         break;
      }

      //Force the socket to operate in non-blocking mode
      error = socketSetTimeout(context->socket, 0);
      //Any error to report?
      if(error)
         break;

      //Adjust the size of the TX buffer
      error = socketSetTxBufferSize(context->socket,
         FTP_SERVER_MIN_TCP_BUFFER_SIZE);
      //Any error to report?
      if(error)
         break;

      //Adjust the size of the RX buffer
      error = socketSetRxBufferSize(context->socket,
         FTP_SERVER_MIN_TCP_BUFFER_SIZE);
      //Any error to report?
      if(error)
         break;

      //Associate the socket with the relevant interface
      error = socketBindToInterface(context->socket,
         context->settings.interface);
      //Any error to report?
      if(error)
         break;

      //The FTP server listens for connection requests on port 21
      error = socketBind(context->socket, &IP_ADDR_ANY,
         context->settings.port);
      //Any error to report?
      if(error)
         break;

      //Place socket in listening state
      error = socketListen(context->socket, FTP_SERVER_BACKLOG);
      //Any failure to report?
      if(error)
         break;

      //Start the FTP server
      context->stop = FALSE;
      context->running = TRUE;

      //Create the FTP server task
      task = osCreateTask("FTP Server", (OsTaskCode) ftpServerTask, context,
         FTP_SERVER_STACK_SIZE, FTP_SERVER_PRIORITY);
      //Failed to create task?
      if(task == OS_INVALID_HANDLE)
      {
         //Report an error
         error = ERROR_OUT_OF_RESOURCES;
         break;
      }

      //End of exception handling block
   } while(0);

   //Any error to report?
   if(error)
   {
      //Clean up side effects
      context->running = FALSE;
      //Close listening socket
      socketClose(context->socket);
   }

   //Return status code
   return error;
}


/**
 * @brief Stop FTP server
 * @param[in] context Pointer to the FTP server context
 * @return Error code
 **/

error_t ftpServerStop(FtpServerContext *context)
{
   uint_t i;

   //Make sure the FTP server context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Debug message
   TRACE_INFO("Stopping FTP server...\r\n");

   //Check whether the FTP server is running
   if(context->running)
   {
      //Stop the FTP server
      context->stop = TRUE;
      //Send a signal to the task to abort any blocking operation
      osSetEvent(&context->event);

      //Wait for the task to terminate
      while(context->running)
      {
         osDelayTask(1);
      }

      //Loop through the connection table
      for(i = 0; i < context->settings.maxConnections; i++)
      {
         //Close client connection
         ftpServerCloseConnection(&context->connections[i]);
      }

      //Close listening socket
      socketClose(context->socket);
      context->socket = NULL;
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set home directory
 * @param[in] connection Pointer to the client connection
 * @param[in] homeDir NULL-terminated string specifying the home directory
 * @return Error code
 **/

error_t ftpServerSetHomeDir(FtpClientConnection *connection,
   const char_t *homeDir)
{
   //Check parameters
   if(connection == NULL || homeDir == NULL)
      return ERROR_INVALID_PARAMETER;

   //Set home directory
   pathCombine(connection->homeDir, homeDir, FTP_SERVER_MAX_HOME_DIR_LEN);

   //Clean the resulting path
   pathCanonicalize(connection->homeDir);
   pathRemoveSlash(connection->homeDir);

   //Set current directory
   osStrcpy(connection->currentDir, connection->homeDir);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief FTP server task
 * @param[in] context Pointer to the FTP server context
 **/

void ftpServerTask(FtpServerContext *context)
{
   error_t error;
   uint_t i;
   systime_t time;
   systime_t timeout;
   FtpClientConnection *connection;

#if (NET_RTOS_SUPPORT == ENABLED)
   //Task prologue
   osEnterTask();

   //Process events
   while(1)
   {
#endif
      //Set polling timeout
      timeout = FTP_SERVER_TICK_INTERVAL;

      //Clear event descriptor set
      osMemset(context->eventDesc, 0, sizeof(context->eventDesc));

      //Specify the events the application is interested in
      for(i = 0; i < context->settings.maxConnections; i++)
      {
         //Point to the structure describing the current connection
         connection = &context->connections[i];

         //Check whether the control connection is active
         if(connection->controlChannel.socket != NULL)
         {
            //Register the events related to the control connection
            ftpServerRegisterControlChannelEvents(connection,
               &context->eventDesc[2 * i]);

            //Check whether the socket is ready for I/O operation
            if(context->eventDesc[2 * i].eventFlags != 0)
            {
               //No need to poll the underlying socket for incoming traffic
               timeout = 0;
            }
         }

         //Check whether the data connection is active
         if(connection->dataChannel.socket != NULL)
         {
            //Register the events related to the data connection
            ftpServerRegisterDataChannelEvents(connection,
               &context->eventDesc[2 * i + 1]);

            //Check whether the socket is ready for I/O operation
            if(context->eventDesc[2 * i + 1].eventFlags != 0)
            {
               //No need to poll the underlying socket for incoming traffic
               timeout = 0;
            }
         }
      }

      //Accept connection request events
      context->eventDesc[2 * i].socket = context->socket;
      context->eventDesc[2 * i].eventMask = SOCKET_EVENT_RX_READY;

      //Wait for one of the set of sockets to become ready to perform I/O
      error = socketPoll(context->eventDesc,
         2 * context->settings.maxConnections + 1, &context->event, timeout);

      //Get current time
      time = osGetSystemTime();

      //Check status code
      if(error == NO_ERROR || error == ERROR_TIMEOUT)
      {
         //Stop request?
         if(context->stop)
         {
            //Stop FTP server operation
            context->running = FALSE;
            //Kill ourselves
            osDeleteTask(NULL);
         }

         //Event-driven processing
         for(i = 0; i < context->settings.maxConnections; i++)
         {
            //Point to the structure describing the current connection
            connection = &context->connections[i];

            //Check whether the control connection is active
            if(connection->controlChannel.socket != NULL)
            {
               //Check whether the control socket is to ready to perform I/O
               if(context->eventDesc[2 * i].eventFlags)
               {
                  //Update time stamp
                  connection->timestamp = time;

                  //Control connection event handler
                  ftpServerProcessControlChannelEvents(connection,
                     context->eventDesc[2 * i].eventFlags);
               }
            }

            //Check whether the data connection is active
            if(connection->dataChannel.socket != NULL)
            {
               //Check whether the data socket is ready to perform I/O
               if(context->eventDesc[2 * i + 1].eventFlags)
               {
                  //Update time stamp
                  connection->timestamp = time;

                  //Data connection event handler
                  ftpServerProcessDataChannelEvents(connection,
                     context->eventDesc[2 * i + 1].eventFlags);
               }
            }
         }

         //Check the state of the listening socket
         if(context->eventDesc[2 * i].eventFlags & SOCKET_EVENT_RX_READY)
         {
            //Accept connection request
            ftpServerAcceptControlChannel(context);
         }
      }

      //Handle periodic operations
      ftpServerTick(context);

#if (NET_RTOS_SUPPORT == ENABLED)
   }
#endif
}


/**
 * @brief Release FTP server context
 * @param[in] context Pointer to the FTP server context
 **/

void ftpServerDeinit(FtpServerContext *context)
{
   //Make sure the FTP server context is valid
   if(context != NULL)
   {
      //Free previously allocated resources
      osDeleteEvent(&context->event);

#if (FTP_SERVER_TLS_SUPPORT == ENABLED && TLS_TICKET_SUPPORT == ENABLED)
      //Release ticket encryption context
      tlsFreeTicketContext(&context->tlsTicketContext);
#endif

      //Clear FTP server context
      osMemset(context, 0, sizeof(FtpServerContext));
   }
}

#endif
