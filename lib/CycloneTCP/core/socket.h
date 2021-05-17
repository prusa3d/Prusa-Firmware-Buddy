/**
 * @file socket.h
 * @brief Socket API
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

#ifndef _SOCKET_H
#define _SOCKET_H

//Forward declaration of Socket structure
struct _Socket;
#define Socket struct _Socket

//Dependencies
#include "core/net.h"
#include "core/ethernet.h"
#include "core/ip.h"
#include "core/tcp.h"

//Number of sockets that can be opened simultaneously
#ifndef SOCKET_MAX_COUNT
   #define SOCKET_MAX_COUNT 16
#elif (SOCKET_MAX_COUNT < 1)
   #error SOCKET_MAX_COUNT parameter is not valid
#endif

//Dynamic port range (lower limit)
#ifndef SOCKET_EPHEMERAL_PORT_MIN
   #define SOCKET_EPHEMERAL_PORT_MIN 49152
#elif (SOCKET_EPHEMERAL_PORT_MIN < 1024)
   #error SOCKET_EPHEMERAL_PORT_MIN parameter is not valid
#endif

//Dynamic port range (upper limit)
#ifndef SOCKET_EPHEMERAL_PORT_MAX
   #define SOCKET_EPHEMERAL_PORT_MAX 65535
#elif (SOCKET_EPHEMERAL_PORT_MAX <= SOCKET_EPHEMERAL_PORT_MIN || SOCKET_EPHEMERAL_PORT_MAX > 65535)
   #error SOCKET_EPHEMERAL_PORT_MAX parameter is not valid
#endif

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Socket types
 **/

typedef enum
{
   SOCKET_TYPE_UNUSED  = 0,
   SOCKET_TYPE_STREAM  = 1,
   SOCKET_TYPE_DGRAM   = 2,
   SOCKET_TYPE_RAW_IP  = 3,
   SOCKET_TYPE_RAW_ETH = 4
} SocketType;


/**
 * @brief IP protocols
 **/

typedef enum
{
   SOCKET_IP_PROTO_ICMP   = 1,
   SOCKET_IP_PROTO_IGMP   = 2,
   SOCKET_IP_PROTO_TCP    = 6,
   SOCKET_IP_PROTO_UDP    = 17,
   SOCKET_IP_PROTO_ICMPV6 = 58
} SocketIpProtocol;


/**
 * @brief Ethernet protocols
 **/

typedef enum
{
   SOCKET_ETH_PROTO_ALL  = 0x0000,
   SOCKET_ETH_PROTO_LLC  = 0x05DC,
   SOCKET_ETH_PROTO_IPV4 = 0x0800,
   SOCKET_ETH_PROTO_ARP  = 0x0806,
   SOCKET_ETH_PROTO_IPV6 = 0x86DD
} SocketEthProtocol;


/**
 * @brief Flags used by I/O functions
 **/

typedef enum
{
   SOCKET_FLAG_PEEK       = 0x0200,
   SOCKET_FLAG_DONT_ROUTE = 0x0400,
   SOCKET_FLAG_WAIT_ALL   = 0x0800,
   SOCKET_FLAG_DONT_WAIT  = 0x0100,
   SOCKET_FLAG_BREAK_CHAR = 0x1000,
   SOCKET_FLAG_BREAK_CRLF = 0x100A,
   SOCKET_FLAG_WAIT_ACK   = 0x2000,
   SOCKET_FLAG_NO_DELAY   = 0x4000,
   SOCKET_FLAG_DELAY      = 0x8000
} SocketFlags;


//The SOCKET_FLAG_BREAK macro causes the I/O functions to stop reading
//data whenever the specified break character is encountered
#define SOCKET_FLAG_BREAK(c) (SOCKET_FLAG_BREAK_CHAR | LSB(c))


/**
 * @brief Flags used by shutdown function
 **/

typedef enum
{
   SOCKET_SD_RECEIVE = 0,
   SOCKET_SD_SEND    = 1,
   SOCKET_SD_BOTH    = 2
} SocketShutdownFlags;


/**
 * @brief Socket events
 **/

typedef enum
{
   SOCKET_EVENT_NONE         = 0x0000,
   SOCKET_EVENT_ACCEPT       = 0x0001,
   SOCKET_EVENT_CONNECTED    = 0x0002,
   SOCKET_EVENT_CLOSED       = 0x0004,
   SOCKET_EVENT_TX_READY     = 0x0008,
   SOCKET_EVENT_TX_DONE      = 0x0010,
   SOCKET_EVENT_TX_ACKED     = 0x0020,
   SOCKET_EVENT_TX_SHUTDOWN  = 0x0040,
   SOCKET_EVENT_RX_READY     = 0x0080,
   SOCKET_EVENT_RX_SHUTDOWN  = 0x0100,
   SOCKET_EVENT_LINK_UP      = 0x0200,
   SOCKET_EVENT_LINK_DOWN    = 0x0400
} SocketEvent;


/**
 * @brief Host types
 **/

typedef enum
{
   HOST_TYPE_ANY  = 0,
   HOST_TYPE_IPV4 = 16,
   HOST_TYPE_IPV6 = 32
} HostType;


/**
 * @brief Name resolution protocols
 **/

typedef enum
{
   HOST_NAME_RESOLVER_ANY   = 0,
   HOST_NAME_RESOLVER_DNS   = 1,
   HOST_NAME_RESOLVER_MDNS  = 2,
   HOST_NAME_RESOLVER_NBNS  = 4,
   HOST_NAME_RESOLVER_LLMNR = 8
} HostnameResolver;


/**
 * @brief Message and ancillary data
 **/

typedef struct
{
   void *data;              ///<Pointer to the payload
   size_t size;             ///<Size of the payload, in bytes
   size_t length;           ///<Actual length of the payload, in bytes
   uint8_t ttl;             ///<Time-to-live value
   NetInterface *interface; ///<Underlying network interface
   IpAddr srcIpAddr;        ///<Source IP address
   uint16_t srcPort;        ///<Source port
   IpAddr destIpAddr;       ///<Destination IP address
   uint16_t destPort;       ///<Destination port
#if (ETH_SUPPORT == ENABLED)
   MacAddr srcMacAddr;      ///<Source MAC address
   MacAddr destMacAddr;     ///<Destination MAC address
#endif
#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
   uint8_t switchPort;      ///<Switch port identifier
#endif
#if (ETH_TIMESTAMP_SUPPORT == ENABLED)
   int32_t timestampId;     ///<Unique identifier for hardware time stamping
   NetTimestamp timestamp;  ///<Captured time stamp
#endif
} SocketMsg;


/**
 * @brief Receive queue item
 **/

typedef struct _SocketQueueItem
{
   struct _SocketQueueItem *next;
   NetInterface *interface;
   IpAddr srcIpAddr;
   uint16_t srcPort;
   IpAddr destIpAddr;
   NetBuffer *buffer;
   size_t offset;
   NetRxAncillary ancillary;
} SocketQueueItem;


/**
 * @brief Structure describing a socket
 **/

struct _Socket
{
   uint_t descriptor;
   uint_t type;
   uint_t protocol;
   NetInterface *interface;
   IpAddr localIpAddr;
   uint16_t localPort;
   IpAddr remoteIpAddr;
   uint16_t remotePort;
   systime_t timeout;
   uint8_t ttl;                   ///<Time-to-live value for unicast datagrams
   uint8_t multicastTtl;          ///<Time-to-live value for multicast datagrams
#if (IP_DIFF_SERV_SUPPORT == ENABLED)
   uint8_t dscp;                  ///<Differentiated services codepoint
#endif
#if (ETH_VLAN_SUPPORT == ENABLED)
   int8_t vlanPcp;                ///<VLAN priority (802.1Q)
   int8_t vlanDei;                ///<Drop eligible indicator
#endif
#if (ETH_VMAN_SUPPORT == ENABLED)
   int8_t vmanPcp;                ///<VMAN priority (802.1ad)
   int8_t vmanDei;                ///<Drop eligible indicator
#endif
   int_t errnoCode;
   OsEvent event;
   uint_t eventMask;
   uint_t eventFlags;
   OsEvent *userEvent;

//TCP specific variables
#if (TCP_SUPPORT == ENABLED)
   TcpState state;                ///<Current state of the TCP finite state machine
   bool_t ownedFlag;              ///<The user is the owner of the TCP socket
   bool_t closedFlag;             ///<The connection has been closed properly
   bool_t resetFlag;              ///<The connection has been reset

   uint16_t smss;                 ///<Sender maximum segment size
   uint16_t rmss;                 ///<Receiver maximum segment size
   uint32_t iss;                  ///<Initial send sequence number
   uint32_t irs;                  ///<Initial receive sequence number

   uint32_t sndUna;               ///<Data that have been sent but not yet acknowledged
   uint32_t sndNxt;               ///<Sequence number of the next byte to be sent
   uint16_t sndUser;              ///<Amount of data buffered but not yet sent
   uint16_t sndWnd;               ///<Size of the send window
   uint16_t maxSndWnd;            ///<Maximum send window it has seen so far on the connection
   uint32_t sndWl1;               ///<Segment sequence number used for last window update
   uint32_t sndWl2;               ///<Segment acknowledgment number used for last window update

   uint32_t rcvNxt;               ///<Receive next sequence number
   uint16_t rcvUser;              ///<Number of data received but not yet consumed
   uint16_t rcvWnd;               ///<Receive window

   bool_t rttBusy;                ///<RTT measurement is being performed
   uint32_t rttSeqNum;            ///<Sequence number identifying a TCP segment
   systime_t rttStartTime;        ///<Round-trip start time
   systime_t srtt;                ///<Smoothed round-trip time
   systime_t rttvar;              ///<Round-trip time variation
   systime_t rto;                 ///<Retransmission timeout

#if (TCP_CONGEST_CONTROL_SUPPORT == ENABLED)
   TcpCongestState congestState;  ///<Congestion state
   uint16_t cwnd;                 ///<Congestion window
   uint16_t ssthresh;             ///<Slow start threshold
   uint_t dupAckCount;            ///<Number of consecutive duplicate ACKs
   uint_t n;                      ///<Number of bytes acknowledged during the whole round-trip
   uint32_t recover;              ///<NewReno modification to TCP's fast recovery algorithm
#endif

   TcpTxBuffer txBuffer;          ///<Send buffer
   size_t txBufferSize;           ///<Size of the send buffer
   TcpRxBuffer rxBuffer;          ///<Receive buffer
   size_t rxBufferSize;           ///<Size of the receive buffer

   TcpQueueItem *retransmitQueue; ///<Retransmission queue
   NetTimer retransmitTimer;      ///<Retransmission timer
   uint_t retransmitCount;        ///<Number of retransmissions

   TcpSynQueueItem *synQueue;     ///<SYN queue for listening sockets
   uint_t synQueueSize;           ///<Maximum number of pending connections for listening sockets

   uint_t wndProbeCount;          ///<Zero window probe counter
   systime_t wndProbeInterval;    ///<Interval between successive probes

   NetTimer persistTimer;         ///<Persist timer
   NetTimer overrideTimer;        ///<Override timer
   NetTimer finWait2Timer;        ///<FIN-WAIT-2 timer
   NetTimer timeWaitTimer;        ///<2MSL timer

#if (TCP_KEEP_ALIVE_SUPPORT == ENABLED)
   bool_t keepAliveEnabled;       ///<Specifies whether TCP keep-alive mechanism is enabled
   systime_t keepAliveIdle;       ///<Keep-alive idle time
   systime_t keepAliveInterval;   ///<Time interval between subsequent keep-alive probes
   uint_t keepAliveMaxProbes;     ///<Number of keep-alive probes
   uint_t keepAliveProbeCount;    ///<Keep-alive probe counter
   systime_t keepAliveTimestamp;  ///<Keep-alive timestamp
#endif

   bool_t sackPermitted;                        ///<SACK Permitted option received
   TcpSackBlock sackBlock[TCP_MAX_SACK_BLOCKS]; ///<List of non-contiguous blocks that have been received
   uint_t sackBlockCount;                       ///<Number of non-contiguous blocks that have been received
#endif

//UDP specific variables
#if (UDP_SUPPORT == ENABLED || RAW_SOCKET_SUPPORT == ENABLED)
   SocketQueueItem *receiveQueue;
#endif
};


/**
 * @brief Structure describing socket events
 **/

typedef struct
{
   Socket *socket;    ///<Handle to a socket to monitor
   uint_t eventMask;  ///<Requested events
   uint_t eventFlags; ///<Returned events
} SocketEventDesc;


//Global constants
extern const SocketMsg SOCKET_DEFAULT_MSG;

//Global variables
extern Socket socketTable[SOCKET_MAX_COUNT];

//Socket related functions
error_t socketInit(void);

Socket *socketOpen(uint_t type, uint_t protocol);

error_t socketSetTimeout(Socket *socket, systime_t timeout);

error_t socketSetTtl(Socket *socket, uint8_t ttl);
error_t socketSetMulticastTtl(Socket *socket, uint8_t ttl);

error_t socketSetDscp(Socket *socket, uint8_t dscp);

error_t socketSetVlanPcp(Socket *socket, uint8_t pcp);
error_t socketSetVlanDei(Socket *socket, bool_t dei);
error_t socketSetVmanPcp(Socket *socket, uint8_t pcp);
error_t socketSetVmanDei(Socket *socket, bool_t dei);

error_t socketEnableKeepAlive(Socket *socket, bool_t enabled);

error_t socketSetKeepAliveParams(Socket *socket, systime_t idle,
   systime_t interval, uint_t maxProbes);

error_t socketSetTxBufferSize(Socket *socket, size_t size);
error_t socketSetRxBufferSize(Socket *socket, size_t size);

error_t socketSetInterface(Socket *socket, NetInterface *interface);
NetInterface *socketGetInterface(Socket *socket);

error_t socketBind(Socket *socket, const IpAddr *localIpAddr, uint16_t localPort);
error_t socketConnect(Socket *socket, const IpAddr *remoteIpAddr, uint16_t remotePort);
error_t socketListen(Socket *socket, uint_t backlog);
Socket *socketAccept(Socket *socket, IpAddr *clientIpAddr, uint16_t *clientPort);

error_t socketSend(Socket *socket, const void *data, size_t length,
   size_t *written, uint_t flags);

error_t socketSendTo(Socket *socket, const IpAddr *destIpAddr, uint16_t destPort,
   const void *data, size_t length, size_t *written, uint_t flags);

error_t socketSendMsg(Socket *socket, const SocketMsg *message, uint_t flags);

error_t socketReceive(Socket *socket, void *data,
   size_t size, size_t *received, uint_t flags);

error_t socketReceiveFrom(Socket *socket, IpAddr *srcIpAddr, uint16_t *srcPort,
   void *data, size_t size, size_t *received, uint_t flags);

error_t socketReceiveEx(Socket *socket, IpAddr *srcIpAddr, uint16_t *srcPort,
   IpAddr *destIpAddr, void *data, size_t size, size_t *received, uint_t flags);

error_t socketReceiveMsg(Socket *socket, SocketMsg *message, uint_t flags);

error_t socketGetLocalAddr(Socket *socket, IpAddr *localIpAddr, uint16_t *localPort);
error_t socketGetRemoteAddr(Socket *socket, IpAddr *remoteIpAddr, uint16_t *remotePort);

error_t socketShutdown(Socket *socket, uint_t how);
void socketClose(Socket *socket);

error_t socketPoll(SocketEventDesc *eventDesc, uint_t size, OsEvent *extEvent,
   systime_t timeout);

error_t getHostByName(NetInterface *interface,
   const char_t *name, IpAddr *ipAddr, uint_t flags);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
