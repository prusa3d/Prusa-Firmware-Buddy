/**
 * @file tcp.c
 * @brief TCP (Transmission Control Protocol)
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
#define TRACE_LEVEL TCP_TRACE_LEVEL

//Dependencies
#include <string.h>
#include "core/net.h"
#include "core/socket.h"
#include "core/socket_misc.h"
#include "core/tcp.h"
#include "core/tcp_misc.h"
#include "core/tcp_timer.h"
#include "mibs/mib2_module.h"
#include "mibs/tcp_mib_module.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (TCP_SUPPORT == ENABLED)

//Tick counter to handle periodic operations
systime_t tcpTickCounter;

//Ephemeral ports are used for dynamic port assignment
static uint16_t tcpDynamicPort;


/**
 * @brief TCP related initialization
 * @return Error code
 **/

error_t tcpInit(void)
{
   //Reset ephemeral port number
   tcpDynamicPort = 0;

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief Get an ephemeral port number
 * @return Ephemeral port
 **/

uint16_t tcpGetDynamicPort(void)
{
   uint_t port;

   //Retrieve current port number
   port = tcpDynamicPort;

   //Invalid port number?
   if(port < SOCKET_EPHEMERAL_PORT_MIN || port > SOCKET_EPHEMERAL_PORT_MAX)
   {
      //Generate a random port number
      port = SOCKET_EPHEMERAL_PORT_MIN + netGetRand() %
         (SOCKET_EPHEMERAL_PORT_MAX - SOCKET_EPHEMERAL_PORT_MIN + 1);
   }

   //Next dynamic port to use
   if(port < SOCKET_EPHEMERAL_PORT_MAX)
   {
      //Increment port number
      tcpDynamicPort = port + 1;
   }
   else
   {
      //Wrap around if necessary
      tcpDynamicPort = SOCKET_EPHEMERAL_PORT_MIN;
   }

   //Return an ephemeral port number
   return port;
}


/**
 * @brief Establish a TCP connection
 * @param[in] socket Handle to an unconnected socket
 * @param[in] remoteIpAddr IP address of the remote host
 * @param[in] remotePort Remote port number that will be used to establish the connection
 * @return Error code
 **/

error_t tcpConnect(Socket *socket, const IpAddr *remoteIpAddr, uint16_t remotePort)
{
   error_t error;
   uint_t event;

   //Check current TCP state
   if(socket->state == TCP_STATE_CLOSED)
   {
      //Save port number and IP address of the remote host
      socket->remoteIpAddr = *remoteIpAddr;
      socket->remotePort = remotePort;

      //Select the source address and the relevant network interface
      //to use when establishing the connection
      error = ipSelectSourceAddr(&socket->interface,
         &socket->remoteIpAddr, &socket->localIpAddr);
      //Any error to report?
      if(error)
         return error;

      //Make sure the source address is valid
      if(ipIsUnspecifiedAddr(&socket->localIpAddr))
         return ERROR_NOT_CONFIGURED;

      //The user owns the socket
      socket->ownedFlag = TRUE;

      //Number of chunks that comprise the TX and the RX buffers
      socket->txBuffer.maxChunkCount = arraysize(socket->txBuffer.chunk);
      socket->rxBuffer.maxChunkCount = arraysize(socket->rxBuffer.chunk);

      //Allocate transmit buffer
      error = netBufferSetLength((NetBuffer *) &socket->txBuffer,
         socket->txBufferSize);

      //Allocate receive buffer
      if(!error)
      {
         error = netBufferSetLength((NetBuffer *) &socket->rxBuffer,
            socket->rxBufferSize);
      }

      //Failed to allocate memory?
      if(error)
      {
         //Free any previously allocated memory
         tcpDeleteControlBlock(socket);
         //Report an error to the caller
         return error;
      }

      //The SMSS is the size of the largest segment that the sender can
      //transmit
      socket->smss = MIN(TCP_DEFAULT_MSS, TCP_MAX_MSS);

      //The RMSS is the size of the largest segment the receiver is willing
      //to accept
      socket->rmss = MIN(socket->rxBufferSize, TCP_MAX_MSS);

      //Generate the initial sequence number
      socket->iss = tcpGenerateInitialSeqNum(&socket->localIpAddr,
         socket->localPort, &socket->remoteIpAddr, socket->remotePort);

      //Initialize TCP control block
      socket->sndUna = socket->iss;
      socket->sndNxt = socket->iss + 1;
      socket->rcvUser = 0;
      socket->rcvWnd = socket->rxBufferSize;

      //Default retransmission timeout
      socket->rto = TCP_INITIAL_RTO;

#if (TCP_CONGEST_CONTROL_SUPPORT == ENABLED)
      //Default congestion state
      socket->congestState = TCP_CONGEST_STATE_IDLE;
      //Initial congestion window
      socket->cwnd = MIN(TCP_INITIAL_WINDOW * socket->smss, socket->txBufferSize);
      //Slow start threshold should be set arbitrarily high
      socket->ssthresh = UINT16_MAX;
      //Recover is set to the initial send sequence number
      socket->recover = socket->iss;
#endif

      //Send a SYN segment
      error = tcpSendSegment(socket, TCP_FLAG_SYN, socket->iss, 0, 0, TRUE);
      //Failed to send TCP segment?
      if(error)
         return error;

      //Switch to the SYN-SENT state
      tcpChangeState(socket, TCP_STATE_SYN_SENT);

      //Number of times TCP connections have made a direct transition to
      //the SYN-SENT state from the CLOSED state
      MIB2_INC_COUNTER32(tcpGroup.tcpActiveOpens, 1);
      TCP_MIB_INC_COUNTER32(tcpActiveOpens, 1);
   }

   //Wait for the connection to be established
   event = tcpWaitForEvents(socket, SOCKET_EVENT_CONNECTED |
      SOCKET_EVENT_CLOSED, socket->timeout);

   //Connection successfully established?
   if(event == SOCKET_EVENT_CONNECTED)
      return NO_ERROR;
   //Failed to establish connection?
   else if(event == SOCKET_EVENT_CLOSED)
      return ERROR_CONNECTION_FAILED;
   //Timeout exception?
   else
      return ERROR_TIMEOUT;
}


/**
 * @brief Place a socket in the listening state
 *
 * Place a socket in a state in which it is listening for an incoming connection
 *
 * @param[in] socket Socket to place in the listening state
 * @param[in] backlog backlog The maximum length of the pending connection queue.
 *   If this parameter is zero, then the default backlog value is used instead
 * @return Error code
 **/

error_t tcpListen(Socket *socket, uint_t backlog)
{
   //Socket already connected?
   if(socket->state != TCP_STATE_CLOSED)
      return ERROR_ALREADY_CONNECTED;

   //Set the size of the SYN queue
   socket->synQueueSize = (backlog > 0) ? backlog : TCP_DEFAULT_SYN_QUEUE_SIZE;
   //Limit the number of pending connections
   socket->synQueueSize = MIN(socket->synQueueSize, TCP_MAX_SYN_QUEUE_SIZE);

   //Place the socket in the listening state
   tcpChangeState(socket, TCP_STATE_LISTEN);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Permit an incoming connection attempt on a TCP socket
 * @param[in] socket Handle to a socket previously placed in a listening state
 * @param[out] clientIpAddr IP address of the client
 * @param[out] clientPort Port number used by the client
 * @return Handle to the socket in which the actual connection is made
 **/

Socket *tcpAccept(Socket *socket, IpAddr *clientIpAddr, uint16_t *clientPort)
{
   error_t error;
   Socket *newSocket;
   TcpSynQueueItem *queueItem;

   //Ensure the socket was previously placed in the listening state
   if(tcpGetState(socket) != TCP_STATE_LISTEN)
      return NULL;

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Wait for an connection attempt
   while(1)
   {
      //The SYN queue is empty?
      if(socket->synQueue == NULL)
      {
         //Set the events the application is interested in
         socket->eventMask = SOCKET_EVENT_RX_READY;
         //Reset the event object
         osResetEvent(&socket->event);

         //Release exclusive access
         osReleaseMutex(&netMutex);
         //Wait until a SYN message is received from a client
         osWaitForEvent(&socket->event, socket->timeout);
         //Get exclusive access
         osAcquireMutex(&netMutex);
      }

      //Check whether the queue is still empty
      if(socket->synQueue == NULL)
      {
         //Timeout error
         newSocket = NULL;
         //Exit immediately
         break;
      }

      //Point to the first item in the SYN queue
      queueItem = socket->synQueue;

      //The function optionally returns the IP address of the client
      if(clientIpAddr != NULL)
      {
         *clientIpAddr = queueItem->srcAddr;
      }

      //The function optionally returns the port number used by the client
      if(clientPort != NULL)
      {
         *clientPort = queueItem->srcPort;
      }

      //Create a new socket to handle the incoming connection request
      newSocket = socketAllocate(SOCKET_TYPE_STREAM, SOCKET_IP_PROTO_TCP);

      //Socket successfully created?
      if(newSocket != NULL)
      {
         //The user owns the socket
         newSocket->ownedFlag = TRUE;

         //Inherit parameters from the listening socket
         newSocket->txBufferSize = socket->txBufferSize;
         newSocket->rxBufferSize = socket->rxBufferSize;

#if (TCP_KEEP_ALIVE_SUPPORT == ENABLED)
         //Inherit keep-alive parameters from the listening socket
         newSocket->keepAliveEnabled = socket->keepAliveEnabled;
         newSocket->keepAliveIdle = socket->keepAliveIdle;
         newSocket->keepAliveInterval = socket->keepAliveInterval;
         newSocket->keepAliveMaxProbes = socket->keepAliveMaxProbes;
#endif
         //Number of chunks that comprise the TX and the RX buffers
         newSocket->txBuffer.maxChunkCount = arraysize(newSocket->txBuffer.chunk);
         newSocket->rxBuffer.maxChunkCount = arraysize(newSocket->rxBuffer.chunk);

         //Allocate transmit buffer
         error = netBufferSetLength((NetBuffer *) &newSocket->txBuffer,
            newSocket->txBufferSize);

         //Check status code
         if(!error)
         {
            //Allocate receive buffer
            error = netBufferSetLength((NetBuffer *) &newSocket->rxBuffer,
               newSocket->rxBufferSize);
         }

         //Transmit and receive buffers successfully allocated?
         if(!error)
         {
            //Bind the newly created socket to the appropriate interface
            newSocket->interface = queueItem->interface;

            //Bind the socket to the specified address
            newSocket->localIpAddr = queueItem->destAddr;
            newSocket->localPort = socket->localPort;

            //Save the port number and the IP address of the remote host
            newSocket->remoteIpAddr = queueItem->srcAddr;
            newSocket->remotePort = queueItem->srcPort;

            //The SMSS is the size of the largest segment that the sender can
            //transmit
            newSocket->smss = queueItem->mss;

            //The RMSS is the size of the largest segment the receiver is
            //willing to accept
            newSocket->rmss = MIN(newSocket->rxBufferSize, TCP_MAX_MSS);

            //Generate the initial sequence number
            newSocket->iss = tcpGenerateInitialSeqNum(&socket->localIpAddr,
               socket->localPort, &socket->remoteIpAddr, socket->remotePort);

            //Initialize TCP control block
            newSocket->irs = queueItem->isn;
            newSocket->sndUna = newSocket->iss;
            newSocket->sndNxt = newSocket->iss + 1;
            newSocket->rcvNxt = newSocket->irs + 1;
            newSocket->rcvUser = 0;
            newSocket->rcvWnd = newSocket->rxBufferSize;

            //Default retransmission timeout
            newSocket->rto = TCP_INITIAL_RTO;

#if (TCP_CONGEST_CONTROL_SUPPORT == ENABLED)
            //Default congestion state
            newSocket->congestState = TCP_CONGEST_STATE_IDLE;
            //Initial congestion window
            newSocket->cwnd = MIN(TCP_INITIAL_WINDOW * newSocket->smss, newSocket->txBufferSize);
            //Slow start threshold should be set arbitrarily high
            newSocket->ssthresh = UINT16_MAX;
            //Recover is set to the initial send sequence number
            newSocket->recover = newSocket->iss;
#endif
            //The connection state should be changed to SYN-RECEIVED
            tcpChangeState(newSocket, TCP_STATE_SYN_RECEIVED);

            //Number of times TCP connections have made a direct transition to
            //the SYN-RECEIVED state from the LISTEN state
            MIB2_INC_COUNTER32(tcpGroup.tcpPassiveOpens, 1);
            TCP_MIB_INC_COUNTER32(tcpPassiveOpens, 1);

            //Send a SYN ACK control segment
            error = tcpSendSegment(newSocket, TCP_FLAG_SYN | TCP_FLAG_ACK,
               newSocket->iss, newSocket->rcvNxt, 0, TRUE);

            //TCP segment successfully sent?
            if(!error)
            {
               //Remove the item from the SYN queue
               socket->synQueue = queueItem->next;
               //Deallocate memory buffer
               memPoolFree(queueItem);
               //Update the state of events
               tcpUpdateEvents(socket);

               //We are done
               break;
            }
         }

         //Dispose the socket
         tcpAbort(newSocket);
      }

      //Debug message
      TRACE_WARNING("Cannot accept TCP connection!\r\n");

      //Remove the item from the SYN queue
      socket->synQueue = queueItem->next;
      //Deallocate memory buffer
      memPoolFree(queueItem);

      //Wait for the next connection attempt
   }

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Return a handle to the newly created socket
   return newSocket;
}


/**
 * @brief Send data to a connected socket
 * @param[in] socket Handle that identifies a connected socket
 * @param[in] data Pointer to a buffer containing the data to be transmitted
 * @param[in] length Number of bytes to be transmitted
 * @param[out] written Actual number of bytes written (optional parameter)
 * @param[in] flags Set of flags that influences the behavior of this function
 * @return Error code
 **/

error_t tcpSend(Socket *socket, const uint8_t *data,
   size_t length, size_t *written, uint_t flags)
{
   uint_t n;
   uint_t totalLength;
   uint_t event;

   //Check whether the socket is in the listening state
   if(socket->state == TCP_STATE_LISTEN)
      return ERROR_NOT_CONNECTED;

   //Actual number of bytes written
   totalLength = 0;

   //Send as much data as possible
   do
   {
      //Wait until there is more room in the send buffer
      event = tcpWaitForEvents(socket, SOCKET_EVENT_TX_READY, socket->timeout);

      //A timeout exception occurred?
      if(event != SOCKET_EVENT_TX_READY)
         return ERROR_TIMEOUT;

      //Check current TCP state
      switch(socket->state)
      {
      //ESTABLISHED or CLOSE-WAIT state?
      case TCP_STATE_ESTABLISHED:
      case TCP_STATE_CLOSE_WAIT:
         //The send buffer is now available for writing
         break;

      //LAST-ACK, FIN-WAIT-1, FIN-WAIT-2, CLOSING or TIME-WAIT state?
      case TCP_STATE_LAST_ACK:
      case TCP_STATE_FIN_WAIT_1:
      case TCP_STATE_FIN_WAIT_2:
      case TCP_STATE_CLOSING:
      case TCP_STATE_TIME_WAIT:
         //The connection is being closed
         return ERROR_CONNECTION_CLOSING;

      //CLOSED state?
      default:
         //The connection was reset by remote side?
         return (socket->resetFlag) ? ERROR_CONNECTION_RESET : ERROR_NOT_CONNECTED;
      }

      //Determine the actual number of bytes in the send buffer
      n = socket->sndUser + socket->sndNxt - socket->sndUna;
      //Exit immediately if the transmission buffer is full (sanity check)
      if(n >= socket->txBufferSize)
         return ERROR_FAILURE;

      //Number of bytes available for writing
      n = socket->txBufferSize - n;
      //Calculate the number of bytes to copy at a time
      n = MIN(n, length - totalLength);

      //Any data to copy?
      if(n > 0)
      {
         //Copy user data to send buffer
         tcpWriteTxBuffer(socket, socket->sndNxt + socket->sndUser, data, n);

         //Update the number of data buffered but not yet sent
         socket->sndUser += n;
         //Advance data pointer
         data += n;
         //Update byte counter
         totalLength += n;

         //Total number of data that have been written
         if(written != NULL)
            *written = totalLength;

         //Update TX events
         tcpUpdateEvents(socket);

         //To avoid a deadlock, it is necessary to have a timeout to force
         //transmission of data, overriding the SWS avoidance algorithm. In
         //practice, this timeout should seldom occur (refer to RFC 1122,
         //section 4.2.3.4)
         if(socket->sndUser == n)
         {
            netStartTimer(&socket->overrideTimer, TCP_OVERRIDE_TIMEOUT);
         }
      }

      //The Nagle algorithm should be implemented to coalesce
      //short segments (refer to RFC 1122 4.2.3.4)
      tcpNagleAlgo(socket, flags);

      //Send as much data as possible
   } while(totalLength < length);

   //The SOCKET_FLAG_WAIT_ACK flag causes the function to
   //wait for acknowledgment from the remote side
   if((flags & SOCKET_FLAG_WAIT_ACK) != 0)
   {
      //Wait for the data to be acknowledged
      event = tcpWaitForEvents(socket, SOCKET_EVENT_TX_ACKED, socket->timeout);

      //A timeout exception occurred?
      if(event != SOCKET_EVENT_TX_ACKED)
         return ERROR_TIMEOUT;

      //The connection was closed before an acknowledgment was received?
      if(socket->state != TCP_STATE_ESTABLISHED && socket->state != TCP_STATE_CLOSE_WAIT)
         return ERROR_NOT_CONNECTED;
   }

   //Successful write operation
   return NO_ERROR;
}


/**
 * @brief Receive data from a connected socket
 * @param[in] socket Handle that identifies a connected socket
 * @param[out] data Buffer where to store the incoming data
 * @param[in] size Maximum number of bytes that can be received
 * @param[out] received Number of bytes that have been received
 * @param[in] flags Set of flags that influences the behavior of this function
 * @return Error code
 **/

error_t tcpReceive(Socket *socket, uint8_t *data,
   size_t size, size_t *received, uint_t flags)
{
   uint_t i;
   uint_t n;
   uint_t event;
   uint32_t seqNum;
   systime_t timeout;

   //Retrieve the break character code
   char_t c = LSB(flags);
   //No data has been read yet
   *received = 0;

   //Check whether the socket is in the listening state
   if(socket->state == TCP_STATE_LISTEN)
      return ERROR_NOT_CONNECTED;

   //Read as much data as possible
   while(*received < size)
   {
      //The SOCKET_FLAG_DONT_WAIT enables non-blocking operation
      timeout = (flags & SOCKET_FLAG_DONT_WAIT) ? 0 : socket->timeout;
      //Wait for data to be available for reading
      event = tcpWaitForEvents(socket, SOCKET_EVENT_RX_READY, timeout);

      //A timeout exception occurred?
      if(event != SOCKET_EVENT_RX_READY)
         return ERROR_TIMEOUT;

      //Check current TCP state
      switch(socket->state)
      {
      //ESTABLISHED, FIN-WAIT-1 or FIN-WAIT-2 state?
      case TCP_STATE_ESTABLISHED:
      case TCP_STATE_FIN_WAIT_1:
      case TCP_STATE_FIN_WAIT_2:
         //Sequence number of the first byte to read
         seqNum = socket->rcvNxt - socket->rcvUser;
         //Data is available in the receive buffer
         break;

      //CLOSE-WAIT, LAST-ACK, CLOSING or TIME-WAIT state?
      case TCP_STATE_CLOSE_WAIT:
      case TCP_STATE_LAST_ACK:
      case TCP_STATE_CLOSING:
      case TCP_STATE_TIME_WAIT:
         //The user must be satisfied with data already on hand
         if(socket->rcvUser == 0)
         {
            if(*received > 0)
               return NO_ERROR;
            else
               return ERROR_END_OF_STREAM;
         }

         //Sequence number of the first byte to read
         seqNum = (socket->rcvNxt - 1) - socket->rcvUser;
         //Data is available in the receive buffer
         break;

      //CLOSED state?
      default:
         //The connection was reset by remote side?
         if(socket->resetFlag)
            return ERROR_CONNECTION_RESET;
         //The connection has not yet been established?
         if(!socket->closedFlag)
            return ERROR_NOT_CONNECTED;

         //The user must be satisfied with data already on hand
         if(socket->rcvUser == 0)
         {
            if(*received > 0)
               return NO_ERROR;
            else
               return ERROR_END_OF_STREAM;
         }

         //Sequence number of the first byte to read
         seqNum = (socket->rcvNxt - 1) - socket->rcvUser;
         //Data is available in the receive buffer
         break;
      }

      //Sanity check
      if(socket->rcvUser == 0)
         return ERROR_FAILURE;

      //Calculate the number of bytes to read at a time
      n = MIN(socket->rcvUser, size - *received);
      //Copy data from circular buffer
      tcpReadRxBuffer(socket, seqNum, data, n);

      //Read data until a break character is encountered?
      if((flags & SOCKET_FLAG_BREAK_CHAR) != 0)
      {
         //Search for the specified break character
         for(i = 0; i < n && data[i] != c; i++);
         //Adjust the number of data to read
         n = MIN(n, i + 1);
      }

      //Total number of data that have been read
      *received += n;
      //Remaining data still available in the receive buffer
      socket->rcvUser -= n;

      //Update the receive window
      tcpUpdateReceiveWindow(socket);
      //Update RX event state
      tcpUpdateEvents(socket);

      //The SOCKET_FLAG_BREAK_CHAR flag causes the function to stop reading
      //data as soon as the specified break character is encountered
      if((flags & SOCKET_FLAG_BREAK_CHAR) != 0)
      {
         //Check whether a break character has been found
         if(data[n - 1] == c)
            break;
      }
      //The SOCKET_FLAG_WAIT_ALL flag causes the function to return
      //only when the requested number of bytes have been read
      else if((flags & SOCKET_FLAG_WAIT_ALL) == 0)
      {
         break;
      }

      //Advance data pointer
      data += n;
   }

   //Successful read operation
   return NO_ERROR;
}


/**
 * @brief Shutdown gracefully reception, transmission, or both
 *
 * Note that socketShutdown() does not close the socket, and resources attached
 * to the socket will not be freed until socketClose() is invoked
 *
 * @param[in] socket Handle to a socket
 * @param[in] how Flag that describes what types of operation will no longer be allowed
 * @return Error code
 **/

error_t tcpShutdown(Socket *socket, uint_t how)
{
   error_t error;
   uint_t event;

   //Disable transmission?
   if(how == SOCKET_SD_SEND || how == SOCKET_SD_BOTH)
   {
      //Check current state
      switch(socket->state)
      {
      //CLOSED or LISTEN state?
      case TCP_STATE_CLOSED:
      case TCP_STATE_LISTEN:
         //The connection does not exist
         return ERROR_NOT_CONNECTED;

      //SYN-RECEIVED or ESTABLISHED state?
      case TCP_STATE_SYN_RECEIVED:
      case TCP_STATE_ESTABLISHED:
         //Flush the send buffer
         error = tcpSend(socket, NULL, 0, NULL, SOCKET_FLAG_NO_DELAY);
         //Any error to report?
         if(error)
            return error;

         //Make sure all the data has been sent out
         event = tcpWaitForEvents(socket, SOCKET_EVENT_TX_DONE, socket->timeout);
         //Timeout error?
         if(event != SOCKET_EVENT_TX_DONE)
            return ERROR_TIMEOUT;

         //Send a FIN segment
         error = tcpSendSegment(socket, TCP_FLAG_FIN | TCP_FLAG_ACK,
            socket->sndNxt, socket->rcvNxt, 0, TRUE);
         //Failed to send FIN segment?
         if(error)
            return error;

         //Sequence number expected to be received
         socket->sndNxt++;
         //Switch to the FIN-WAIT1 state
         tcpChangeState(socket, TCP_STATE_FIN_WAIT_1);

         //Wait for the FIN to be acknowledged
         event = tcpWaitForEvents(socket, SOCKET_EVENT_TX_SHUTDOWN, socket->timeout);
         //Timeout interval elapsed?
         if(event != SOCKET_EVENT_TX_SHUTDOWN)
            return ERROR_TIMEOUT;

         //Continue processing
         break;

      //CLOSE-WAIT state?
      case TCP_STATE_CLOSE_WAIT:
         //Flush the send buffer
         error = tcpSend(socket, NULL, 0, NULL, SOCKET_FLAG_NO_DELAY);
         //Any error to report?
         if(error)
            return error;

         //Make sure all the data has been sent out
         event = tcpWaitForEvents(socket, SOCKET_EVENT_TX_DONE, socket->timeout);
         //Timeout error?
         if(event != SOCKET_EVENT_TX_DONE)
            return ERROR_TIMEOUT;

         //Send a FIN segment
         error = tcpSendSegment(socket, TCP_FLAG_FIN | TCP_FLAG_ACK,
            socket->sndNxt, socket->rcvNxt, 0, TRUE);
         //Failed to send FIN segment?
         if(error)
            return error;

         //Sequence number expected to be received
         socket->sndNxt++;
         //Switch to the LAST-ACK state
         tcpChangeState(socket, TCP_STATE_LAST_ACK);

         //Wait for the FIN to be acknowledged
         event = tcpWaitForEvents(socket, SOCKET_EVENT_TX_SHUTDOWN, socket->timeout);
         //Timeout interval elapsed?
         if(event != SOCKET_EVENT_TX_SHUTDOWN)
            return ERROR_TIMEOUT;

         //Continue processing
         break;

      //FIN-WAIT-1, CLOSING or LAST-ACK state?
      case TCP_STATE_FIN_WAIT_1:
      case TCP_STATE_CLOSING:
      case TCP_STATE_LAST_ACK:
         //Wait for the FIN to be acknowledged
         event = tcpWaitForEvents(socket, SOCKET_EVENT_TX_SHUTDOWN, socket->timeout);
         //Timeout interval elapsed?
         if(event != SOCKET_EVENT_TX_SHUTDOWN)
            return ERROR_TIMEOUT;

         //Continue processing
         break;

      //SYN-SENT, FIN-WAIT-2 or TIME-WAIT state?
      default:
         //Continue processing
         break;
      }
   }

   //Disable reception?
   if(how == SOCKET_SD_RECEIVE || how == SOCKET_SD_BOTH)
   {
      //Check current state
      switch(socket->state)
      {
      //LISTEN state?
      case TCP_STATE_LISTEN:
         //The connection does not exist
         return ERROR_NOT_CONNECTED;

      //SYN-SENT, SYN-RECEIVED, ESTABLISHED, FIN-WAIT-1 or FIN-WAIT-2 state?
      case TCP_STATE_SYN_SENT:
      case TCP_STATE_SYN_RECEIVED:
      case TCP_STATE_ESTABLISHED:
      case TCP_STATE_FIN_WAIT_1:
      case TCP_STATE_FIN_WAIT_2:
         //Wait for a FIN to be received
         event = tcpWaitForEvents(socket, SOCKET_EVENT_RX_SHUTDOWN, socket->timeout);
         //Timeout interval elapsed?
         if(event != SOCKET_EVENT_RX_SHUTDOWN)
            return ERROR_TIMEOUT;
         //A FIN segment has been received
         break;

      //CLOSING, TIME-WAIT, CLOSE-WAIT, LAST-ACK or CLOSED state?
      default:
         //A FIN segment has already been received
         break;
      }
   }

   //Successful operation
   return NO_ERROR;
}


/**
 * @brief Abort an existing TCP connection
 * @param[in] socket Handle identifying the socket to close
 * @return Error code
 **/

error_t tcpAbort(Socket *socket)
{
   error_t error;

   //Check current state
   switch(socket->state)
   {
   //SYN-RECEIVED, ESTABLISHED, FIN-WAIT-1
   //FIN-WAIT-2 or CLOSE-WAIT state?
   case TCP_STATE_SYN_RECEIVED:
   case TCP_STATE_ESTABLISHED:
   case TCP_STATE_FIN_WAIT_1:
   case TCP_STATE_FIN_WAIT_2:
   case TCP_STATE_CLOSE_WAIT:
      //Send a reset segment
      error = tcpSendResetSegment(socket, socket->sndNxt);
      //Enter CLOSED state
      tcpChangeState(socket, TCP_STATE_CLOSED);
      //Delete TCB
      tcpDeleteControlBlock(socket);
      //Mark the socket as closed
      socket->type = SOCKET_TYPE_UNUSED;
      //Return status code
      return error;

   //TIME-WAIT state?
   case TCP_STATE_TIME_WAIT:
#if (TCP_2MSL_TIMER > 0)
      //The user doe not own the socket anymore...
      socket->ownedFlag = FALSE;
      //TCB will be deleted and socket will be closed
      //when the 2MSL timer will elapse
      return NO_ERROR;
#else
      //Enter CLOSED state
      tcpChangeState(socket, TCP_STATE_CLOSED);
      //Delete TCB
      tcpDeleteControlBlock(socket);
      //Mark the socket as closed
      socket->type = SOCKET_TYPE_UNUSED;
      //No error to report
      return NO_ERROR;
#endif

   //Any other state?
   default:
      //Enter CLOSED state
      tcpChangeState(socket, TCP_STATE_CLOSED);
      //Delete TCB
      tcpDeleteControlBlock(socket);
      //Mark the socket as closed
      socket->type = SOCKET_TYPE_UNUSED;
      //No error to report
      return NO_ERROR;
   }
}


/**
 * @brief Get the current state of the TCP FSM
 * @param[in] socket Handle identifying the socket
 * @return TCP FSM state
 **/

TcpState tcpGetState(Socket *socket)
{
   TcpState state;

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Get TCP FSM current state
   state = socket->state;

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Return current state
   return state;
}


/**
 * @brief Kill the oldest socket in the TIME-WAIT state
 * @return Handle identifying the oldest TCP connection in the TIME-WAIT state.
 *   NULL is returned if no socket is currently in the TIME-WAIT state
 **/

Socket *tcpKillOldestConnection(void)
{
   uint_t i;
   systime_t time;
   Socket *socket;
   Socket *oldestSocket;

   //Get current time
   time = osGetSystemTime();

   //Keep track of the oldest socket in the TIME-WAIT state
   oldestSocket = NULL;

   //Loop through socket descriptors
   for(i = 0; i < SOCKET_MAX_COUNT; i++)
   {
      //Point to the current socket descriptor
      socket = &socketTable[i];

      //TCP connection found?
      if(socket->type == SOCKET_TYPE_STREAM)
      {
         //Check current state
         if(socket->state == TCP_STATE_TIME_WAIT)
         {
            //Keep track of the oldest socket in the TIME-WAIT state
            if(oldestSocket == NULL)
            {
               //Save socket handle
               oldestSocket = socket;
            }
            if((time - socket->timeWaitTimer.startTime) >
               (time - oldestSocket->timeWaitTimer.startTime))
            {
               //Save socket handle
               oldestSocket = socket;
            }
         }
      }
   }

   //Any connection in the TIME-WAIT state?
   if(oldestSocket != NULL)
   {
      //Enter CLOSED state
      tcpChangeState(oldestSocket, TCP_STATE_CLOSED);
      //Delete TCB
      tcpDeleteControlBlock(oldestSocket);
      //Mark the socket as closed
      oldestSocket->type = SOCKET_TYPE_UNUSED;
   }

   //The oldest connection in the TIME-WAIT state can be reused
   //when the socket table runs out of space
   return oldestSocket;
}

#endif
