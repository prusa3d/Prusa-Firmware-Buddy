/**
 * @file dhcpv6_relay.c
 * @brief DHCPv6 relay agent (Dynamic Host Configuration Protocol for IPv6)
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
 * DHCPv6 Relay-Agents are deployed to forward DHCPv6 messages between clients
 * and servers when they are not on the same IPv6 link and are often implemented
 * alongside a routing function in a common node. Refer to RFC 3315
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

//Switch to the appropriate trace level
#define TRACE_LEVEL DHCPV6_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "dhcpv6_relay.h"
#include "dhcpv6/dhcpv6_common.h"
#include "dhcpv6/dhcpv6_debug.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (IPV6_SUPPORT == ENABLED && DHCPV6_RELAY_SUPPORT == ENABLED)


/**
 * @brief Start DHCPv6 relay agent
 * @param[in] context Pointer to the DHCPv6 relay agent context
 * @param[in] settings DHCPv6 relay agent specific settings
 * @return Error code
 **/

error_t dhcpv6RelayStart(Dhcpv6RelayContext *context, const Dhcpv6RelaySettings *settings)
{
   error_t error;
   uint_t i;
   OsTask *task;

   //Debug message
   TRACE_INFO("Starting DHCPv6 relay agent...\r\n");

   //Ensure the parameters are valid
   if(!context || !settings)
      return ERROR_INVALID_PARAMETER;
   //The pointer to the network-facing interface shall be valid
   if(!settings->serverInterface)
      return ERROR_INVALID_INTERFACE;
   //Check the number of client-facing interfaces
   if(!settings->clientInterfaceCount)
      return ERROR_INVALID_PARAMETER;
   if(settings->clientInterfaceCount >= DHCPV6_RELAY_MAX_CLIENT_IF)
      return ERROR_INVALID_PARAMETER;

   //Loop through the client-facing interfaces
   for(i = 0; i < settings->clientInterfaceCount; i++)
   {
      //A valid pointer is required for each interface
      if(!settings->clientInterface[i])
         return ERROR_INVALID_INTERFACE;
   }

   //Check the address to be used when forwarding messages to the server
   if(ipv6CompAddr(&settings->serverAddress, &IPV6_UNSPECIFIED_ADDR))
      return ERROR_INVALID_ADDRESS;

   //Clear the DHCPv6 relay agent context
   osMemset(context, 0, sizeof(Dhcpv6RelayContext));

   //Save the network-facing interface
   context->serverInterface = settings->serverInterface;
   //Save the number of client-facing interfaces
   context->clientInterfaceCount = settings->clientInterfaceCount;

   //Save all the client-facing interfaces
   for(i = 0; i < context->clientInterfaceCount; i++)
      context->clientInterface[i] = settings->clientInterface[i];

   //Save the address to be used when relaying client messages to the server
   context->serverAddress = settings->serverAddress;

   //Join the All_DHCP_Relay_Agents_and_Servers multicast group
   //for each client-facing interface
   error = dhcpv6RelayJoinMulticastGroup(context);
   //Any error to report?
   if(error)
      return error;

   //Start of exception handling block
   do
   {
      //Open a UDP socket to handle the network-facing interface
      context->serverSocket = socketOpen(SOCKET_TYPE_DGRAM, SOCKET_IP_PROTO_UDP);
      //Failed to open socket?
      if(!context->serverSocket)
      {
         //Report an error
         error = ERROR_OPEN_FAILED;
         //Stop processing
         break;
      }

      //Explicitly associate the socket with the relevant interface
      error = socketBindToInterface(context->serverSocket, context->serverInterface);
      //Unable to bind the socket to the desired interface?
      if(error)
         break;

      //Relay agents listen for DHCPv6 messages on UDP port 547
      error = socketBind(context->serverSocket, &IP_ADDR_ANY, DHCPV6_SERVER_PORT);
      //Unable to bind the socket to the desired port?
      if(error)
         break;

      //Only accept datagrams with source port number 547
      error = socketConnect(context->serverSocket, &IP_ADDR_ANY, DHCPV6_SERVER_PORT);
      //Any error to report?
      if(error)
         break;

      //If the relay agent relays messages to the All_DHCP_Servers address
      //or other multicast addresses, it sets the Hop Limit field to 32

      //Loop through the client-facing interfaces
      for(i = 0; i < context->clientInterfaceCount; i++)
      {
         //Open a UDP socket to handle the current interface
         context->clientSocket[i] = socketOpen(SOCKET_TYPE_DGRAM, SOCKET_IP_PROTO_UDP);
         //Failed to open socket?
         if(!context->clientSocket[i])
         {
            //Report an error
            error = ERROR_OPEN_FAILED;
            //Stop processing
            break;
         }

         //Explicitly associate the socket with the relevant interface
         error = socketBindToInterface(context->clientSocket[i], context->clientInterface[i]);
         //Unable to bind the socket to the desired interface?
         if(error)
            break;

         //Relay agents listen for DHCPv6 messages on UDP port 547
         error = socketBind(context->clientSocket[i], &IP_ADDR_ANY, DHCPV6_SERVER_PORT);
         //Unable to bind the socket to the desired port?
         if(error)
            break;

         //Only accept datagrams with source port number 546
         error = socketConnect(context->clientSocket[i], &IP_ADDR_ANY, DHCPV6_CLIENT_PORT);
         //Any error to report?
         if(error)
            break;
      }

      //Propagate exception if necessary
      if(error)
         break;

      //Initialize event object
      if(!osCreateEvent(&context->event))
      {
         //Failed to create event
         error = ERROR_OUT_OF_RESOURCES;
         //Stop processing
         break;
      }

      //Initialize ACK event object
      if(!osCreateEvent(&context->ackEvent))
      {
         //Failed to create event
         error = ERROR_OUT_OF_RESOURCES;
         //Stop processing
         break;
      }

      //The DHCPv6 relay agent is now running
      context->running = TRUE;

      //Start the DHCPv6 relay agent service
      task = osCreateTask("DHCPv6 Relay", dhcpv6RelayTask,
         context, DHCPV6_RELAY_STACK_SIZE, DHCPV6_RELAY_PRIORITY);

      //Unable to create the task?
      if(task == OS_INVALID_HANDLE)
         error = ERROR_OUT_OF_RESOURCES;

      //End of exception handling block
   } while(0);

   //Did we encounter an error?
   if(error)
   {
      //Close the socket associated with the network-facing interface
      socketClose(context->serverSocket);

      //Close the socket associated with each client-facing interface
      for(i = 0; i < context->clientInterfaceCount; i++)
         socketClose(context->clientSocket[i]);

      //Leave the All_DHCP_Relay_Agents_and_Servers multicast group
      //for each client-facing interface
      dhcpv6RelayLeaveMulticastGroup(context);

      //Delete event objects
      osDeleteEvent(&context->event);
      osDeleteEvent(&context->ackEvent);
   }

   //Return status code
   return error;
}


/**
 * @brief Stop DHCPv6 relay agent
 * @param[in] context Pointer to the DHCPv6 relay agent context
 * @return Error code
 **/

error_t dhcpv6RelayStop(Dhcpv6RelayContext *context)
{
   uint_t i;

   //Make sure the DHCPv6 relay agent context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Debug message
   TRACE_INFO("Stopping DHCPv6 relay agent...\r\n");

   //Check DHCPv6 relay agent state
   if(!context->running)
      return ERROR_WRONG_STATE;

   //Reset ACK event before sending the kill signal
   osResetEvent(&context->ackEvent);
   //Stop the DHCPv6 relay agent task
   context->stopRequest = TRUE;
   //Send a signal to the task in order to abort any blocking operation
   osSetEvent(&context->event);

   //Wait for the process to terminate...
   osWaitForEvent(&context->ackEvent, INFINITE_DELAY);

   //Leave the All_DHCP_Relay_Agents_and_Servers multicast group
   //for each client-facing interface
   dhcpv6RelayLeaveMulticastGroup(context);

   //Close the socket that carries traffic towards the DHCPv6 server
   socketClose(context->serverSocket);

   //Properly dispose the sockets that carry traffic towards the DHCPv6 clients
   for(i = 0; i < context->clientInterfaceCount; i++)
      socketClose(context->clientSocket[i]);

   //Delete event objects
   osDeleteEvent(&context->event);
   osDeleteEvent(&context->ackEvent);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Join All_DHCP_Relay_Agents_and_Servers multicast group
 * @param[in] context Pointer to the DHCPv6 relay agent context
 **/

error_t dhcpv6RelayJoinMulticastGroup(Dhcpv6RelayContext *context)
{
   uint_t i;
   uint_t j;

   //Initialize status code
   error_t error = NO_ERROR;

   //Loop through the client-facing interfaces
   for(i = 0; i < context->clientInterfaceCount; i++)
   {
      //Join the All_DHCP_Relay_Agents_and_Servers multicast
      //group for each interface
      error = ipv6JoinMulticastGroup(context->clientInterface[i],
         &DHCPV6_ALL_RELAY_AGENTS_AND_SERVERS_ADDR);
      //Unable to join the specified multicast group?
      if(error)
         break;
   }

   //Did we encounter an error?
   if(error)
   {
      //Clean up side effects before returning...
      for(j = 0; j < i; j++)
      {
         //Leave the multicast group for each interface
         ipv6LeaveMulticastGroup(context->clientInterface[j],
            &DHCPV6_ALL_RELAY_AGENTS_AND_SERVERS_ADDR);
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Leave All_DHCP_Relay_Agents_and_Servers multicast group
 * @param[in] context Pointer to the DHCPv6 relay agent context
 **/

error_t dhcpv6RelayLeaveMulticastGroup(Dhcpv6RelayContext *context)
{
   uint_t i;

   //Loop through the client-facing interfaces
   for(i = 0; i < context->clientInterfaceCount; i++)
   {
      //Leave the All_DHCP_Relay_Agents_and_Servers multicast
      //group for each interface
      ipv6LeaveMulticastGroup(context->clientInterface[i],
         &DHCPV6_ALL_RELAY_AGENTS_AND_SERVERS_ADDR);
   }

   //Successsful processing
   return NO_ERROR;
}


/**
 * @brief DHCPv6 relay agent task
 * @param[in] param Pointer to the DHCPv6 relay agent context
 **/

void dhcpv6RelayTask(void *param)
{
   error_t error;
   uint_t i;
   Dhcpv6RelayContext *context;

   //Task prologue
   osEnterTask();

   //Point to the DHCPv6 relay agent context
   context = (Dhcpv6RelayContext *) param;

   //Specify the events the application is interested in for
   //each client-facing sockets
   for(i = 0; i < context->clientInterfaceCount; i++)
   {
      context->eventDesc[i].socket = context->clientSocket[i];
      context->eventDesc[i].eventMask = SOCKET_EVENT_RX_READY;
   }

   //Specify the events the application is interested in for
   //the network-facing socket
   context->eventDesc[i].socket = context->serverSocket;
   context->eventDesc[i].eventMask = SOCKET_EVENT_RX_READY;

   //Main loop
   while(1)
   {
      //Wait for incoming packets on network-facing or client-facing interfaces
      error = socketPoll(context->eventDesc, context->clientInterfaceCount + 1,
         &context->event, INFINITE_DELAY);

      //Stop DHCPv6 relay agent?
      if(context->stopRequest)
      {
         //The DHCPv6 relay agent is about to stop
         context->stopRequest = FALSE;
         context->running = FALSE;
         //Acknowledge the reception of the user request
         osSetEvent(&context->ackEvent);
         //Kill ourselves
         osDeleteTask(NULL);
      }

      //Verify status code
      if(!error)
      {
         //Check the state of each client-facing socket
         for(i = 0; i < context->clientInterfaceCount; i++)
         {
            //Relay client messages if applicable
            if(context->eventDesc[i].eventFlags & SOCKET_EVENT_RX_READY)
               dhcpv6ForwardClientMessage(context, i);
         }

         //Check the state of the network-facing socket
         if(context->eventDesc[i].eventFlags & SOCKET_EVENT_RX_READY)
         {
            //Forward Relay-Reply messages from the network
            dhcpv6ForwardRelayReplyMessage(context);
         }
      }
   }
}


/**
 * @brief Forward client message
 * @param[in] context Pointer to the DHCPv6 relay agent context
 * @param[in] index Index identifying the interface on which the message was received
 * @return Error code
 **/

error_t dhcpv6ForwardClientMessage(Dhcpv6RelayContext *context, uint_t index)
{
   error_t error;
   uint32_t interfaceId;
   size_t inputMessageLen;
   size_t outputMessageLen;
   Dhcpv6RelayMessage *inputMessage;
   Dhcpv6RelayMessage *outputMessage;
   Dhcpv6Option *option;
   IpAddr ipAddr;

   //Point to the buffer where to store the incoming DHCPv6 message
   inputMessage = (Dhcpv6RelayMessage *) (context->buffer + DHCPV6_RELAY_FORW_OVERHEAD);
   //Message that will be forwarded by the DHCPv6 relay agent
   outputMessage = (Dhcpv6RelayMessage *) context->buffer;

   //Read incoming message
   error = socketReceiveFrom(context->clientSocket[index], &ipAddr, NULL, inputMessage,
      DHCPV6_MAX_MSG_SIZE - DHCPV6_RELAY_FORW_OVERHEAD, &inputMessageLen, 0);
   //Any error to report?
   if(error)
      return error;

   //Debug message
   TRACE_INFO("\r\nDHCPv6 message received on client-facing interface %s (%" PRIuSIZE " bytes)...\r\n",
      context->clientInterface[index]->name, inputMessageLen);

   //Dump the contents of the message for debugging purpose
   dhcpv6DumpMessage(inputMessage, inputMessageLen);

   //The source address must be a valid IPv6 address
   if(ipAddr.length != sizeof(Ipv6Addr))
      return ERROR_INVALID_ADDRESS;
   //Check the length of the DHCPv6 message
   if(inputMessageLen < sizeof(Dhcpv6Message))
      return ERROR_INVALID_MESSAGE;

   //When the relay agent receives a valid message to be relayed,
   //it constructs a new Relay-Forward message
   outputMessage->msgType = DHCPV6_MSG_TYPE_RELAY_FORW;

   //Inspect the message type
   switch(inputMessage->msgType)
   {
   //Message received from a client?
   case DHCPV6_MSG_TYPE_SOLICIT:
   case DHCPV6_MSG_TYPE_REQUEST:
   case DHCPV6_MSG_TYPE_CONFIRM:
   case DHCPV6_MSG_TYPE_RENEW:
   case DHCPV6_MSG_TYPE_REBIND:
   case DHCPV6_MSG_TYPE_RELEASE:
   case DHCPV6_MSG_TYPE_DECLINE:
   case DHCPV6_MSG_TYPE_INFO_REQUEST:
      //If the relay agent received the message to be relayed from a client
      //the hop-count in the Relay-Forward message is set to 0
      outputMessage->hopCount = 0;
      //Continue processing
      break;

   //Message received from another relay agent?
   case DHCPV6_MSG_TYPE_RELAY_FORW:
      //If the message received by the relay agent is a Relay-Forward message
      //and the hop-count in the message is greater than or equal to 32, the
      //relay agent discards the received message
      if(inputMessage->hopCount >= DHCPV6_HOP_COUNT_LIMIT)
         return ERROR_INVALID_MESSAGE;
      //Set the hop-count field to the value of the hop-count field in
      //the received message incremented by 1
      outputMessage->hopCount = inputMessage->hopCount + 1;
      //Continue processing
      break;

   //Message received from a server?
   default:
      //Discard ADVERTISE, REPLY, RECONFIGURE and RELAY-REPL messages
      return ERROR_INVALID_MESSAGE;
   }

   //Set the link-address field to the unspecified address
   outputMessage->linkAddress = IPV6_UNSPECIFIED_ADDR;
   //Copy the source address from the header of the IP datagram in
   //which the message was received to the peer-address field
   outputMessage->peerAddress = ipAddr.ipv6Addr;
   //Size of the Relay-Forward message
   outputMessageLen = sizeof(Dhcpv6RelayMessage);

   //Get the interface identifier
   interfaceId = context->clientInterface[index]->id;
   //Convert the 32-bit integer to network byte order
   interfaceId = htonl(interfaceId);

   //If the relay agent cannot use the address in the link-address field
   //to identify the interface through which the response to the client
   //will be relayed, the relay agent must include an Interface ID option
   dhcpv6AddOption(outputMessage, &outputMessageLen,
      DHCPV6_OPTION_INTERFACE_ID, &interfaceId, sizeof(interfaceId));

   //Copy the received DHCPv6 message into a Relay Message option
   option = dhcpv6AddOption(outputMessage, &outputMessageLen,
      DHCPV6_OPTION_RELAY_MSG, NULL, 0);

   //Set the appropriate length of the option
   option->length = htons(inputMessageLen);
   //Adjust the length of the Relay-Forward message
   outputMessageLen += inputMessageLen;

   //Debug message
   TRACE_INFO("Forwarding DHCPv6 message on network-facing interface %s (%" PRIuSIZE " bytes)...\r\n",
      context->serverInterface->name, outputMessageLen);

   //Dump the contents of the message for debugging purpose
   dhcpv6DumpMessage(outputMessage, outputMessageLen);

   //Destination address to be used when relaying the client message
   ipAddr.length = sizeof(Ipv6Addr);
   ipAddr.ipv6Addr = context->serverAddress;

   //Relay the client message to the server
   return socketSendTo(context->serverSocket, &ipAddr,
      DHCPV6_SERVER_PORT, outputMessage, outputMessageLen, NULL, 0);
}


/**
 * @brief Forward Relay-Reply message
 * @param[in] context Pointer to the DHCPv6 relay agent context
 * @return Error code
 **/

error_t dhcpv6ForwardRelayReplyMessage(Dhcpv6RelayContext *context)
{
   error_t error;
   uint_t i;
   uint32_t interfaceId;
   size_t inputMessageLen;
   size_t outputMessageLen;
   Dhcpv6RelayMessage *inputMessage;
   Dhcpv6Message *outputMessage;
   Dhcpv6Option *option;
   IpAddr ipAddr;
   uint16_t port;

   //Point to the buffer where to store the incoming DHCPv6 message
   inputMessage = (Dhcpv6RelayMessage *) context->buffer;

   //Read incoming message
   error = socketReceiveFrom(context->serverSocket, &ipAddr, &port,
      inputMessage, DHCPV6_MAX_MSG_SIZE, &inputMessageLen, 0);
   //Any error to report?
   if(error)
      return error;

   //Debug message
   TRACE_INFO("\r\nDHCPv6 message received on network-facing interface %s (%" PRIuSIZE " bytes)...\r\n",
      context->serverInterface->name, inputMessageLen);

   //Dump the contents of the message for debugging purpose
   dhcpv6DumpMessage(inputMessage, inputMessageLen);

   //Check the length of the DHCPv6 message
   if(inputMessageLen < sizeof(Dhcpv6RelayMessage))
      return ERROR_INVALID_MESSAGE;

   //Inspect the message type and only forward Relay-Reply messages.
   //Other DHCPv6 message types must be silently discarded
   if(inputMessage->msgType != DHCPV6_MSG_TYPE_RELAY_REPL)
      return ERROR_INVALID_MESSAGE;

   //Get the length of the Options field
   inputMessageLen -= sizeof(Dhcpv6Message);

   //Check whether an Interface ID option is included in the Relay-Reply
   option = dhcpv6GetOption(inputMessage->options, inputMessageLen, DHCPV6_OPTION_INTERFACE_ID);
   //Failed to retrieve specified option?
   if(option == NULL || ntohs(option->length) != sizeof(interfaceId))
      return ERROR_INVALID_MESSAGE;

   //Read the Interface ID option contents
   osMemcpy(&interfaceId, option->value, sizeof(interfaceId));
   //Convert the 32-bit integer from network byte order
   interfaceId = ntohl(interfaceId);

   //The Relay-Reply message must include a Relay Message option
   option = dhcpv6GetOption(inputMessage->options, inputMessageLen, DHCPV6_OPTION_RELAY_MSG);
   //Failed to retrieve specified option?
   if(option == NULL || ntohs(option->length) < sizeof(Dhcpv6Message))
      return ERROR_INVALID_MESSAGE;

   //Extract the message from the Relay Message option
   outputMessage = (Dhcpv6Message *) option->value;
   //Save the length of the message
   outputMessageLen = ntohs(option->length);

   //Loop through client-facing interfaces
   for(i = 0; i < context->clientInterfaceCount; i++)
   {
      //Check whether the current interface matches the Interface ID option
      if(context->clientInterface[i]->id == interfaceId)
      {
         //Debug message
         TRACE_INFO("Forwarding DHCPv6 message on client-facing interface %s (%" PRIuSIZE " bytes)...\r\n",
            context->clientInterface[i]->name, outputMessageLen);

         //Dump the contents of the message for debugging purpose
         dhcpv6DumpMessage(outputMessage, outputMessageLen);

         //Copy the peer-address into the destination IP address
         ipAddr.length = sizeof(Ipv6Addr);
         ipAddr.ipv6Addr = inputMessage->peerAddress;

         //Select the relevant port number to use
         if(outputMessage->msgType == DHCPV6_MSG_TYPE_RELAY_REPL)
            port = DHCPV6_SERVER_PORT;
         else
            port = DHCPV6_CLIENT_PORT;

         //Relay the DHCPv6 message to the client on the link
         //identified by the Interface ID option
         return socketSendTo(context->clientSocket[i], &ipAddr,
            port, outputMessage, outputMessageLen, NULL, 0);
      }
   }

   //Unknown interface identifier...
   return ERROR_INVALID_OPTION;
}

#endif
