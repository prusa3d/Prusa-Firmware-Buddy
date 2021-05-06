/**
 * @file tcp_timer.c
 * @brief TCP timer management
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
#include "core/net.h"
#include "core/socket.h"
#include "core/tcp.h"
#include "core/tcp_misc.h"
#include "core/tcp_timer.h"
#include "date_time.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (TCP_SUPPORT == ENABLED)


/**
 * @brief TCP timer handler
 *
 * This routine must be periodically called by the TCP/IP stack to
 * handle retransmissions and TCP related timers (persist timer,
 * FIN-WAIT-2 timer and TIME-WAIT timer)
 *
 **/

void tcpTick(void)
{
   uint_t i;
   Socket *socket;

   //Loop through opened sockets
   for(i = 0; i < SOCKET_MAX_COUNT; i++)
   {
      //Point to the current socket
      socket = &socketTable[i];

      //TCP socket?
      if(socket->type == SOCKET_TYPE_STREAM)
      {
         //Check current TCP state
         if(socket->state != TCP_STATE_CLOSED)
         {
            //Check retransmission timer
            tcpCheckRetransmitTimer(socket);
            //Check persist timer
            tcpCheckPersistTimer(socket);
            //Check TCP keep-alive timer
            tcpCheckKeepAliveTimer(socket);
            //Check override timer
            tcpCheckOverrideTimer(socket);
            //Check FIN-WAIT-2 timer
            tcpCheckFinWait2Timer(socket);
            //Check 2MSL timer
            tcpCheckTimeWaitTimer(socket);
         }
      }
   }
}


/**
 * @brief Check retransmission timer
 * @param[in] socket Handle referencing the socket
 **/

void tcpCheckRetransmitTimer(Socket *socket)
{
   //Check current TCP state
   if(socket->state != TCP_STATE_CLOSED)
   {
      //Any packet in the retransmission queue?
      if(socket->retransmitQueue != NULL)
      {
         //Retransmission timeout?
         if(netTimerExpired(&socket->retransmitTimer))
         {
#if (TCP_CONGEST_CONTROL_SUPPORT == ENABLED)
            //When a TCP sender detects segment loss using the retransmission
            //timer and the given segment has not yet been resent by way of
            //the retransmission timer, the value of ssthresh must be updated
            if(socket->retransmitCount == 0)
            {
               uint32_t flightSize;

               //Amount of data that has been sent but not yet acknowledged
               flightSize = socket->sndNxt - socket->sndUna;
               //Adjust ssthresh value
               socket->ssthresh = MAX(flightSize / 2, 2 * socket->smss);
            }

            //Furthermore, upon a timeout cwnd must be set to no more than the
            //loss window, LW, which equals 1 full-sized segment
            socket->cwnd = MIN(TCP_LOSS_WINDOW * socket->smss,
               socket->txBufferSize);

            //After a retransmit timeout, record the highest sequence number
            //transmitted in the variable recover
            socket->recover = socket->sndNxt - 1;

            //Enter the fast loss recovery procedure
            socket->congestState = TCP_CONGEST_STATE_LOSS_RECOVERY;
#endif
            //Make sure the maximum number of retransmissions has not been
            //reached
            if(socket->retransmitCount < TCP_MAX_RETRIES)
            {
               //Debug message
               TRACE_INFO("%s: TCP segment retransmission #%u (%u data bytes)...\r\n",
                  formatSystemTime(osGetSystemTime(), NULL),
                  socket->retransmitCount + 1,
                  socket->retransmitQueue->length);

               //Retransmit the earliest segment that has not been acknowledged
               //by the TCP receiver
               tcpRetransmitSegment(socket);

               //Use exponential back-off algorithm to calculate the new RTO
               socket->rto = MIN(socket->rto * 2, TCP_MAX_RTO);
               //Restart retransmission timer
               netStartTimer(&socket->retransmitTimer, socket->rto);
               //Increment retransmission counter
               socket->retransmitCount++;
            }
            else
            {
               //Send a reset segment
               tcpSendResetSegment(socket, socket->sndNxt);
               //Turn off the retransmission timer
               netStopTimer(&socket->retransmitTimer);
               //The maximum number of retransmissions has been exceeded
               tcpChangeState(socket, TCP_STATE_CLOSED);
            }

            //TCP must use Karn's algorithm for taking RTT samples. That is, RTT
            //samples must not be made using segments that were retransmitted
            socket->rttBusy = FALSE;
         }
      }
   }
}


/**
 * @brief Check persist timer
 *
 * The TCP persist timer is set by one end of a connection when it has data to
 * send, but has been stopped because the other end has advertised a zero-sized
 * window
 *
 * @param[in] socket Handle referencing the socket
 **/

void tcpCheckPersistTimer(Socket *socket)
{
   //Check current TCP state
   if(socket->state != TCP_STATE_CLOSED)
   {
      //Check whether the remote host advertises a window size of zero
      if(socket->sndWnd == 0 && socket->wndProbeInterval != 0)
      {
         //Persist timer expired?
         if(netTimerExpired(&socket->persistTimer))
         {
            //Make sure the maximum number of retransmissions has not been
            //reached
            if(socket->wndProbeCount < TCP_MAX_RETRIES)
            {
               //Debug message
               TRACE_INFO("%s: TCP zero window probe #%u...\r\n",
                  formatSystemTime(osGetSystemTime(), NULL), socket->wndProbeCount + 1);

               //Zero window probes usually have the sequence number one less
               //than expected
               tcpSendSegment(socket, TCP_FLAG_ACK, socket->sndUna - 1,
                  socket->rcvNxt, 0, FALSE);

               //The interval between successive probes should be increased
               //exponentially
               socket->wndProbeInterval = MIN(socket->wndProbeInterval * 2,
                  TCP_MAX_PROBE_INTERVAL);

               //Restart the persist timer
               netStartTimer(&socket->persistTimer, socket->wndProbeInterval);
               //Increment window probe counter
               socket->wndProbeCount++;
            }
            else
            {
               //Send a reset segment
               tcpSendResetSegment(socket, socket->sndNxt);
               //Enter CLOSED state
               tcpChangeState(socket, TCP_STATE_CLOSED);
            }
         }
      }
   }
}


/**
 * @brief Check TCP keep-alive timer
 *
 * The TCP keep-alive timer feature provides a mechanism to identify dead
 * connections. The other useful goal of keep-alive is to prevent inactivity
 * from disconnecting the channel
 *
 * @param[in] socket Handle referencing the socket
 **/

void tcpCheckKeepAliveTimer(Socket *socket)
{
#if (TCP_KEEP_ALIVE_SUPPORT == ENABLED)
   systime_t time;

   //Check current TCP state
   if(socket->state == TCP_STATE_ESTABLISHED)
   {
      //Check whether TCP keep-alive mechanism is enabled
      if(socket->keepAliveEnabled)
      {
         //Get current time
         time = osGetSystemTime();

         //Idle condition?
         if(socket->keepAliveProbeCount == 0)
         {
            //Check keep-alive idle timer
            if(timeCompare(time, socket->keepAliveTimestamp +
               socket->keepAliveIdle) >= 0)
            {
               //Keep-alive probes are sent with a sequence number one less than
               //the sequence number the receiver is expecting
               tcpSendSegment(socket, TCP_FLAG_ACK, socket->sndUna - 1,
                  socket->rcvNxt, 0, FALSE);

               //Initialize window probe counter
               socket->keepAliveProbeCount = 1;
               //Restart keep-alive timer
               socket->keepAliveTimestamp = time;
            }
         }
         else
         {
            //Check keep-alive probe timer
            if(timeCompare(time, socket->keepAliveTimestamp +
               MIN(socket->keepAliveInterval, socket->keepAliveIdle)) >= 0)
            {
               //Check the number of unacknowledged keep-alive probes
               if(socket->keepAliveProbeCount < socket->keepAliveMaxProbes)
               {
                  //Keep-alive probes are sent with a sequence number one less
                  //than the sequence number the receiver is expecting
                  tcpSendSegment(socket, TCP_FLAG_ACK, socket->sndUna - 1,
                     socket->rcvNxt, 0, FALSE);

                  //Increment window probe counter
                  socket->keepAliveProbeCount++;
                  //Restart keep-alive timer
                  socket->keepAliveTimestamp = time;
               }
               else
               {
                  //Debug message
                  TRACE_WARNING("%s: TCP dead peer detected...\r\n",
                     formatSystemTime(osGetSystemTime(), NULL));

                  //Send a reset segment
                  tcpSendResetSegment(socket, socket->sndNxt);
                  //The TCP connection is dead
                  tcpChangeState(socket, TCP_STATE_CLOSED);
               }
            }
         }
      }
   }
#endif
}


/**
 * @brief Check override timer
 *
 * To avoid a deadlock, it is necessary to have a timeout to force transmission
 * of data, overriding the SWS avoidance algorithm. In practice, this timeout
 * should seldom occur (refer to RFC 1122, section 4.2.3.4)
 *
 * @param[in] socket Handle referencing the socket
 **/

void tcpCheckOverrideTimer(Socket *socket)
{
   error_t error;
   uint32_t n;
   uint32_t u;

   //Check current TCP state
   if(socket->state == TCP_STATE_ESTABLISHED ||
      socket->state == TCP_STATE_CLOSE_WAIT)
   {
      //Override timer expired?
      if(socket->sndUser && netTimerExpired(&socket->overrideTimer))
      {
         //The amount of data that can be sent at any given time is limited by
         //the receiver window and the congestion window
         n = MIN(socket->sndWnd, socket->txBufferSize);

#if (TCP_CONGEST_CONTROL_SUPPORT == ENABLED)
         //Check the congestion window
         n = MIN(n, socket->cwnd);
#endif
         //Retrieve the size of the usable window
         u = n - (socket->sndNxt - socket->sndUna);

         //Send as much data as possible
         while(socket->sndUser > 0)
         {
            //The usable window size may become zero or negative, preventing
            //packet transmission
            if((int32_t) u <= 0)
               break;

            //Calculate the number of bytes to send at a time
            n = MIN(u, socket->sndUser);
            n = MIN(n, socket->smss);

            //Send TCP segment
            error = tcpSendSegment(socket, TCP_FLAG_PSH | TCP_FLAG_ACK,
               socket->sndNxt, socket->rcvNxt, n, TRUE);
            //Failed to send TCP segment?
            if(error)
               break;

            //Advance SND.NXT pointer
            socket->sndNxt += n;
            //Adjust the number of bytes buffered but not yet sent
            socket->sndUser -= n;
            //Update the size of the usable window
            u -= n;
         }

         //Check whether the transmitter can accept more data
         tcpUpdateEvents(socket);

         //Restart override timer if necessary
         if(socket->sndUser > 0)
         {
            netStartTimer(&socket->overrideTimer, TCP_OVERRIDE_TIMEOUT);
         }
      }
   }
}


/**
 * @brief Check FIN-WAIT-2 timer
 *
 * The FIN-WAIT-2 timer prevents the connection from staying in the FIN-WAIT-2
 * state forever
 *
 * @param[in] socket Handle referencing the socket
 **/

void tcpCheckFinWait2Timer(Socket *socket)
{
   //Check current TCP state
   if(socket->state == TCP_STATE_FIN_WAIT_2)
   {
      //FIN-WAIT-2 timer expired?
      if(netTimerExpired(&socket->finWait2Timer))
      {
         //Debug message
         TRACE_INFO("TCP FIN-WAIT-2 timer elapsed...\r\n");
         //Enter CLOSED state
         tcpChangeState(socket, TCP_STATE_CLOSED);
      }
   }
}


/**
 * @brief Check 2MSL timer
 *
 * The purpose of the TIME-WAIT timer is to prevent delayed packets from one
 * connection from being accepted by a later connection
 *
 * @param[in] socket Handle referencing the socket
 **/

void tcpCheckTimeWaitTimer(Socket *socket)
{
   //Check current TCP state
   if(socket->state == TCP_STATE_TIME_WAIT)
   {
      //2MSL timer expired?
      if(netTimerExpired(&socket->timeWaitTimer))
      {
         //Debug message
         TRACE_INFO("TCP 2MSL timer elapsed...\r\n");
         //Enter CLOSED state
         tcpChangeState(socket, TCP_STATE_CLOSED);

         //Dispose the socket if the user does not have the ownership anymore
         if(!socket->ownedFlag)
         {
            //Delete the TCB
            tcpDeleteControlBlock(socket);
            //Mark the socket as closed
            socket->type = SOCKET_TYPE_UNUSED;
         }
      }
   }
}

#endif
