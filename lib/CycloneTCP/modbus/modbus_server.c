/**
 * @file modbus_server.c
 * @brief Modbus/TCP server
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
#define TRACE_LEVEL MODBUS_TRACE_LEVEL

//Dependencies
#include "modbus/modbus_server.h"
#include "modbus/modbus_server_transport.h"
#include "modbus/modbus_server_misc.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (MODBUS_SERVER_SUPPORT == ENABLED)


/**
 * @brief Initialize settings with default values
 * @param[out] settings Structure that contains Modbus/TCP server settings
 **/

void modbusServerGetDefaultSettings(ModbusServerSettings *settings)
{
   //The Modbus/TCP server is not bound to any interface
   settings->interface = NULL;

   //Modbus/TCP port number
   settings->port = MODBUS_TCP_PORT;
   //Default unit identifier
   settings->unitId = MODBUS_DEFAULT_UNIT_ID;

#if (MODBUS_SERVER_TLS_SUPPORT == ENABLED)
   //TLS initialization callback function
   settings->tlsInitCallback = NULL;
#endif

   //Lock Modbus table callback function
   settings->lockCallback = NULL;
   //Unlock Modbus table callback function
   settings->unlockCallback = NULL;
   //Get coil state callback function
   settings->readCoilCallback = NULL;
   //Get discrete input state callback function
   settings->readDiscreteInputCallback = NULL;
   //Set coil state callback function
   settings->writeCoilCallback = NULL;
   //Get register value callback function
   settings->readRegCallback = NULL;
   //Get holding register value callback function
   settings->readHoldingRegCallback = NULL;
   //Get input register value callback function
   settings->readInputRegCallback = NULL;
   //Set register value callback function
   settings->writeRegCallback = NULL;
   //PDU processing callback
   settings->processPduCallback = NULL;
}


/**
 * @brief Initialize Modbus/TCP server context
 * @param[in] context Pointer to the Modbus/TCP server context
 * @param[in] settings Modbus/TCP server specific settings
 * @return Error code
 **/

error_t modbusServerInit(ModbusServerContext *context,
   const ModbusServerSettings *settings)
{
   error_t error;

   //Debug message
   TRACE_INFO("Initializing Modbus/TCP server...\r\n");

   //Ensure the parameters are valid
   if(context == NULL || settings == NULL)
      return ERROR_INVALID_PARAMETER;

   //Clear Modbus/TCP server context
   osMemset(context, 0, sizeof(ModbusServerContext));

   //Save user settings
   context->settings = *settings;

   //Initialize status code
   error = NO_ERROR;

   //Create an event object to poll the state of sockets
   if(!osCreateEvent(&context->event))
   {
      //Failed to create event
      error = ERROR_OUT_OF_RESOURCES;
   }

#if (MODBUS_SERVER_TLS_SUPPORT == ENABLED && TLS_TICKET_SUPPORT == ENABLED)
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
      modbusServerDeinit(context);
   }

   //Return status code
   return error;
}


/**
 * @brief Start Modbus/TCP server
 * @param[in] context Pointer to the Modbus/TCP server context
 * @return Error code
 **/

error_t modbusServerStart(ModbusServerContext *context)
{
   error_t error;
   OsTask *task;

   //Make sure the Modbus/TCP server context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Debug message
   TRACE_INFO("Starting Modbus/TCP server...\r\n");

   //Make sure the Modbus/TCP server is not already running
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

      //Associate the socket with the relevant interface
      error = socketBindToInterface(context->socket,
        context->settings.interface);
      //Any error to report?
      if(error)
         break;

      //The Modbus/TCP server listens for connection requests on port 502
      error = socketBind(context->socket, &IP_ADDR_ANY, context->settings.port);
      //Any error to report?
      if(error)
         break;

      //Place socket in listening state
      error = socketListen(context->socket, 0);
      //Any error to report?
      if(error)
         break;

      //Start the Modbus/TCP server
      context->stop = FALSE;
      context->running = TRUE;

      //Create the Modbus/TCP server task
      task = osCreateTask("Modbus/TCP Server", (OsTaskCode) modbusServerTask,
         context, MODBUS_SERVER_STACK_SIZE, MODBUS_SERVER_PRIORITY);
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
 * @brief Stop Modbus/TCP server
 * @param[in] context Pointer to the Modbus/TCP server context
 * @return Error code
 **/

error_t modbusServerStop(ModbusServerContext *context)
{
   uint_t i;

   //Make sure the Modbus/TCP server context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Debug message
   TRACE_INFO("Stopping Modbus/TCP server...\r\n");

   //Check whether the Modbus/TCP server is running
   if(context->running)
   {
      //Stop the Modbus/TCP server
      context->stop = TRUE;
      //Send a signal to the task to abort any blocking operation
      osSetEvent(&context->event);

      //Wait for the task to terminate
      while(context->running)
      {
         osDelayTask(1);
      }

      //Loop through the connection table
      for(i = 0; i < MODBUS_SERVER_MAX_CONNECTIONS; i++)
      {
         //Close client connection
         modbusServerCloseConnection(&context->connection[i]);
      }

      //Close listening socket
      socketClose(context->socket);
      context->socket = NULL;
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Modbus/TCP server task
 * @param[in] context Pointer to the Modbus/TCP server context
 **/

void modbusServerTask(ModbusServerContext *context)
{
   error_t error;
   uint_t i;
   systime_t timeout;
   ModbusClientConnection *connection;
   SocketEventDesc eventDesc[MODBUS_SERVER_MAX_CONNECTIONS + 1];

#if (NET_RTOS_SUPPORT == ENABLED)
   //Task prologue
   osEnterTask();

   //Process events
   while(1)
   {
#endif
      //Set polling timeout
      timeout = MODBUS_SERVER_TICK_INTERVAL;

      //Clear event descriptor set
      osMemset(eventDesc, 0, sizeof(eventDesc));

      //Specify the events the application is interested in
      for(i = 0; i < MODBUS_SERVER_MAX_CONNECTIONS; i++)
      {
         //Point to the structure describing the current connection
         connection = &context->connection[i];

         //Loop through active connections only
         if(connection->state != MODBUS_CONNECTION_STATE_CLOSED)
         {
            //Register connection events
            modbusServerRegisterConnectionEvents(connection, &eventDesc[i]);

            //Check whether the socket is ready for I/O operation
            if(eventDesc[i].eventFlags != 0)
            {
               //No need to poll the underlying socket for incoming traffic
               timeout = 0;
            }
         }
      }

      //The Modbus/TCP server listens for connection requests on port 502
      eventDesc[i].socket = context->socket;
      eventDesc[i].eventMask = SOCKET_EVENT_RX_READY;

      //Wait for one of the set of sockets to become ready to perform I/O
      error = socketPoll(eventDesc, MODBUS_SERVER_MAX_CONNECTIONS + 1,
         &context->event, timeout);

      //Check status code
      if(error == NO_ERROR || error == ERROR_TIMEOUT)
      {
         //Stop request?
         if(context->stop)
         {
            //Stop Modbus/TCP server operation
            context->running = FALSE;
            //Kill ourselves
            osDeleteTask(NULL);
         }

         //Event-driven processing
         for(i = 0; i < MODBUS_SERVER_MAX_CONNECTIONS; i++)
         {
            //Point to the structure describing the current connection
            connection = &context->connection[i];

            //Loop through active connections only
            if(connection->state != MODBUS_CONNECTION_STATE_CLOSED)
            {
               //Check whether the socket is ready to perform I/O
               if(eventDesc[i].eventFlags != 0)
               {
                  //Connection event handler
                  modbusServerProcessConnectionEvents(connection);
               }
            }
         }

         //Any connection request received on port 502?
         if(eventDesc[i].eventFlags != 0)
         {
            //Accept connection request
            modbusServerAcceptConnection(context);
         }
      }

      //Handle periodic operations
      modbusServerTick(context);

#if (NET_RTOS_SUPPORT == ENABLED)
   }
#endif
}


/**
 * @brief Release Modbus/TCP server context
 * @param[in] context Pointer to the Modbus/TCP server context
 **/

void modbusServerDeinit(ModbusServerContext *context)
{
   //Make sure the Modbus/TCP server context is valid
   if(context != NULL)
   {
      //Free previously allocated resources
      osDeleteEvent(&context->event);

#if (MODBUS_SERVER_TLS_SUPPORT == ENABLED && TLS_TICKET_SUPPORT == ENABLED)
      //Release ticket encryption context
      tlsFreeTicketContext(&context->tlsTicketContext);
#endif

      //Clear Modbus/TCP server context
      osMemset(context, 0, sizeof(ModbusServerContext));
   }
}

#endif
