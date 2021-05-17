/**
 * @file ndp.h
 * @brief NDP (Neighbor Discovery Protocol)
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

#ifndef _NDP_H
#define _NDP_H

//Dependencies
#include "core/net.h"

//NDP support
#ifndef NDP_SUPPORT
   #define NDP_SUPPORT ENABLED
#elif (NDP_SUPPORT != ENABLED && NDP_SUPPORT != DISABLED)
   #error NDP_SUPPORT parameter is not valid
#endif

//NDP tick interval
#ifndef NDP_TICK_INTERVAL
   #define NDP_TICK_INTERVAL 200
#elif (NDP_TICK_INTERVAL < 10)
   #error NDP_TICK_INTERVAL parameter is not valid
#endif

//Neighbor cache size
#ifndef NDP_NEIGHBOR_CACHE_SIZE
   #define NDP_NEIGHBOR_CACHE_SIZE 8
#elif (NDP_NEIGHBOR_CACHE_SIZE < 1)
   #error NDP_NEIGHBOR_CACHE_SIZE parameter is not valid
#endif

//Destination cache size
#ifndef NDP_DEST_CACHE_SIZE
   #define NDP_DEST_CACHE_SIZE 8
#elif (NDP_DEST_CACHE_SIZE < 1)
   #error NDP_DEST_CACHE_SIZE parameter is not valid
#endif

//Maximum number of packets waiting for address resolution to complete
#ifndef NDP_MAX_PENDING_PACKETS
   #define NDP_MAX_PENDING_PACKETS 2
#elif (NDP_MAX_PENDING_PACKETS < 1)
   #error NDP_MAX_PENDING_PACKETS parameter is not valid
#endif

//Maximum time interval between Router Advertisements
#ifndef NDP_MAX_RTR_ADVERT_INTERVAL
   #define NDP_MAX_RTR_ADVERT_INTERVAL 600000
#elif (NDP_MAX_RTR_ADVERT_INTERVAL < 1000)
   #error NDP_MAX_RTR_ADVERT_INTERVAL parameter is not valid
#endif

//Maximum time interval between initial Router Advertisements
#ifndef NDP_MAX_INITIAL_RTR_ADVERT_INTERVAL
   #define NDP_MAX_INITIAL_RTR_ADVERT_INTERVAL 16000
#elif (NDP_MAX_INITIAL_RTR_ADVERT_INTERVAL < 1000)
   #error NDP_MAX_INITIAL_RTR_ADVERT_INTERVAL parameter is not valid
#endif

//Maximum number of initial Router Advertisements
#ifndef NDP_MAX_INITIAL_RTR_ADVERTISEMENTS
   #define NDP_MAX_INITIAL_RTR_ADVERTISEMENTS 3
#elif (NDP_MAX_INITIAL_RTR_ADVERTISEMENTS < 1)
   #error NDP_MAX_INITIAL_RTR_ADVERTISEMENTS parameter is not valid
#endif

//Maximum number of final Router Advertisements
#ifndef NDP_MAX_FINAL_RTR_ADVERTISEMENTS
   #define NDP_MAX_FINAL_RTR_ADVERTISEMENTS 3
#elif (NDP_MAX_FINAL_RTR_ADVERTISEMENTS < 1)
   #error NDP_MAX_FINAL_RTR_ADVERTISEMENTS parameter is not valid
#endif

//Minimum delay between Router Advertisements
#ifndef NDP_MIN_DELAY_BETWEEN_RAS
   #define NDP_MIN_DELAY_BETWEEN_RAS 3000
#elif (NDP_MIN_DELAY_BETWEEN_RAS < 1000)
   #error NDP_MIN_DELAY_BETWEEN_RAS parameter is not valid
#endif

//Maximum delay for Router Advertisements sent in response to a Router Solicitation
#ifndef NDP_MAX_RA_DELAY_TIME
   #define NDP_MAX_RA_DELAY_TIME 500
#elif (NDP_MAX_RA_DELAY_TIME < 100)
   #error NDP_MAX_RA_DELAY_TIME parameter is not valid
#endif

//Minimum delay before transmitting the first Router Solicitation message
#ifndef NDP_MIN_RTR_SOLICITATION_DELAY
   #define NDP_MIN_RTR_SOLICITATION_DELAY 0
#elif (NDP_MIN_RTR_SOLICITATION_DELAY < 0)
   #error NDP_MIN_RTR_SOLICITATION_DELAY parameter is not valid
#endif

//Maximum delay before transmitting the first Router Solicitation message
#ifndef NDP_MAX_RTR_SOLICITATION_DELAY
   #define NDP_MAX_RTR_SOLICITATION_DELAY 1000
#elif (NDP_MAX_RTR_SOLICITATION_DELAY < 0)
   #error NDP_MAX_RTR_SOLICITATION_DELAY parameter is not valid
#endif

//The time between retransmissions of Router Solicitation messages
#ifndef NDP_RTR_SOLICITATION_INTERVAL
   #define NDP_RTR_SOLICITATION_INTERVAL 4000
#elif (NDP_RTR_SOLICITATION_INTERVAL < 1000)
   #error NDP_RTR_SOLICITATION_INTERVAL parameter is not valid
#endif

//Number of retransmissions for Router Solicitation messages
#ifndef NDP_MAX_RTR_SOLICITATIONS
   #define NDP_MAX_RTR_SOLICITATIONS 3
#elif (NDP_MAX_RTR_SOLICITATIONS < 1)
   #error NDP_MAX_RTR_SOLICITATIONS parameter is not valid
#endif

//Number of retransmissions for multicast Neighbor Solicitation messages
#ifndef NDP_MAX_MULTICAST_SOLICIT
   #define NDP_MAX_MULTICAST_SOLICIT 3
#elif (NDP_MAX_MULTICAST_SOLICIT < 1)
   #error NDP_MAX_MULTICAST_SOLICIT parameter is not valid
#endif

//Number of retransmissions for unicast Neighbor Solicitation messages
#ifndef NDP_MAX_UNICAST_SOLICIT
   #define NDP_MAX_UNICAST_SOLICIT 3
#elif (NDP_MAX_UNICAST_SOLICIT < 1)
   #error NDP_MAX_UNICAST_SOLICIT parameter is not valid
#endif

//Maximum number of Neighbor Solicitation messages sent while performing DAD
#ifndef NDP_DUP_ADDR_DETECT_TRANSMITS
   #define NDP_DUP_ADDR_DETECT_TRANSMITS 1
#elif (NDP_DUP_ADDR_DETECT_TRANSMITS < 0)
   #error NDP_DUP_ADDR_DETECT_TRANSMITS parameter is not valid
#endif

//Delay before sending Neighbor Advertisements if the target address is an anycast address
#ifndef NDP_MAX_ANYCAST_DELAY_TIME
   #define NDP_MAX_ANYCAST_DELAY_TIME 1000
#elif (NDP_MAX_ANYCAST_DELAY_TIME < 100)
   #error NDP_MAX_ANYCAST_DELAY_TIME parameter is not valid
#endif

//Maximum number of unsolicited Neighbor Advertisements
#ifndef NDP_MAX_NEIGHBOR_ADVERTISEMENT
   #define NDP_MAX_NEIGHBOR_ADVERTISEMENT 3
#elif (NDP_MAX_NEIGHBOR_ADVERTISEMENT < 0)
   #error NDP_MAX_NEIGHBOR_ADVERTISEMENT parameter is not valid
#endif

//The time a neighbor is considered reachable after receiving a reachability confirmation
#ifndef NDP_REACHABLE_TIME
   #define NDP_REACHABLE_TIME 30000
#elif (NDP_REACHABLE_TIME < 1000)
   #error NDP_REACHABLE_TIME parameter is not valid
#endif

//The time between retransmissions of Neighbor Solicitation messages
#ifndef NDP_RETRANS_TIMER
   #define NDP_RETRANS_TIMER 1000
#elif (NDP_RETRANS_TIMER < 100)
   #error NDP_RETRANS_TIMER parameter is not valid
#endif

//Delay before sending the first probe
#ifndef NDP_DELAY_FIRST_PROBE_TIME
   #define NDP_DELAY_FIRST_PROBE_TIME 5000
#elif (NDP_DELAY_FIRST_PROBE_TIME < 1000)
   #error NDP_DELAY_FIRST_PROBE_TIME parameter is not valid
#endif

//Hop Limit used by NDP messages
#define NDP_HOP_LIMIT 255

//Infinite lifetime
#define NDP_INFINITE_LIFETIME 0xFFFFFFFF

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Neighbor Discovery options
 **/

typedef enum
{
   NDP_OPT_SOURCE_LINK_LAYER_ADDR = 1,
   NDP_OPT_TARGET_LINK_LAYER_ADDR = 2,
   NDP_OPT_PREFIX_INFORMATION     = 3,
   NDP_OPT_REDIRECTED_HEADER      = 4,
   NDP_OPT_MTU                    = 5,
   NDP_OPT_ROUTE_INFORMATION      = 24,
   NDP_OPT_RECURSIVE_DNS_SERVER   = 25,
   NDP_OPT_DNS_SEARCH_LIST        = 31,
   NDP_OPT_6LOWPAN_CONTEXT        = 34,
   NDP_OPT_CAPTIVE_PORTAL         = 37,
   NDP_OPT_ANY                    = 255
} NdpOptionType;


/**
 * @brief Router selection preferences
 **/

typedef enum
{
   NDP_ROUTER_SEL_PREFERENCE_MEDIUM   = 0,
   NDP_ROUTER_SEL_PREFERENCE_HIGH     = 1,
   NDP_ROUTER_SEL_PREFERENCE_RESERVED = 2,
   NDP_ROUTER_SEL_PREFERENCE_LOW      = 3
} NdpRouterSelPreference;


/**
 * @brief Neighbor cache entry states
 **/

typedef enum
{
   NDP_STATE_NONE       = 0,
   NDP_STATE_INCOMPLETE = 1,
   NDP_STATE_REACHABLE  = 2,
   NDP_STATE_STALE      = 3,
   NDP_STATE_DELAY      = 4,
   NDP_STATE_PROBE      = 5,
   NDP_STATE_PERMANENT  = 6
} NdpState;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(push, 1)
#endif


/**
 * @brief Router Solicitation message
 **/

typedef __start_packed struct
{
   uint8_t type;      //0
   uint8_t code;      //1
   uint16_t checksum; //2-3
   uint32_t reserved; //4-7
   uint8_t options[]; //8
} __end_packed NdpRouterSolMessage;


/**
 * @brief Router Advertisement message
 **/

typedef __start_packed struct
{
   uint8_t type;                  //0
   uint8_t code;                  //1
   uint16_t checksum;             //2-3
   uint8_t curHopLimit;           //4
#if defined(_CPU_BIG_ENDIAN) && !defined(__IAR_SYSTEMS_ICC__)
   uint8_t m : 1;                 //5
   uint8_t o : 1;
   uint8_t h : 1;
   uint8_t prf : 2;
   uint8_t p : 1;
   uint8_t reserved : 2;
#else
   uint8_t reserved : 2;          //5
   uint8_t p : 1;
   uint8_t prf : 2;
   uint8_t h : 1;
   uint8_t o : 1;
   uint8_t m : 1;
#endif
   uint16_t routerLifetime;       //6-7
   uint32_t reachableTime;        //8-11
   uint32_t retransTimer;         //12-15
   uint8_t options[];             //16
} __end_packed NdpRouterAdvMessage;


/**
 * @brief Neighbor Solicitation message
 **/

typedef __start_packed struct
{
   uint8_t type;        //0
   uint8_t code;        //1
   uint16_t checksum;   //2-3
   uint32_t reserved;   //4-7
   Ipv6Addr targetAddr; //8-23
   uint8_t options[];   //24
} __end_packed NdpNeighborSolMessage;


/**
 * @brief Neighbor Advertisement message
 **/

typedef __start_packed struct
{
   uint8_t type;          //0
   uint8_t code;          //1
   uint16_t checksum;     //2-3
#if defined(_CPU_BIG_ENDIAN) && !defined(__IAR_SYSTEMS_ICC__)
   uint8_t r : 1;         //4
   uint8_t s : 1;
   uint8_t o : 1;
   uint8_t reserved1 : 5;
#else
   uint8_t reserved1 : 5; //4
   uint8_t o : 1;
   uint8_t s : 1;
   uint8_t r : 1;
#endif
   uint8_t reserved2[3];  //5-7
   Ipv6Addr targetAddr;   //8-23
   uint8_t options[];     //24
} __end_packed NdpNeighborAdvMessage;


/**
 * @brief Redirect message
 **/

typedef __start_packed struct
{
   uint8_t type;        //0
   uint8_t code;        //1
   uint16_t checksum;   //2-3
   uint32_t reserved;   //4-7
   Ipv6Addr targetAddr; //8-23
   Ipv6Addr destAddr;   //24-39
   uint8_t options[];   //40
} __end_packed NdpRedirectMessage;


/**
 * @brief Neighbor Discovery option general format
 **/

typedef __start_packed struct
{
   uint8_t type;    //0
   uint8_t length;  //1
   uint8_t value[]; //2
} __end_packed NdpOption;


/**
 * @brief Source/Target Link-Layer Address option
 **/

typedef __start_packed struct
{
   uint8_t type;          //0
   uint8_t length;        //1
   MacAddr linkLayerAddr; //2-7
} __end_packed NdpLinkLayerAddrOption;


/**
 * @brief Prefix Information option (PIO)
 **/

typedef __start_packed struct
{
   uint8_t type;               //0
   uint8_t length;             //1
   uint8_t prefixLength;       //2
#if defined(_CPU_BIG_ENDIAN) && !defined(__IAR_SYSTEMS_ICC__)
   uint8_t l : 1;              //3
   uint8_t a : 1;
   uint8_t r : 1;
   uint8_t reserved1 : 5;
#else
   uint8_t reserved1 : 5;      //3
   uint8_t r : 1;
   uint8_t a : 1;
   uint8_t l : 1;
#endif
   uint32_t validLifetime;     //4-7
   uint32_t preferredLifetime; //8-11
   uint32_t reserved2;         //12-15
   Ipv6Addr prefix;            //16-31
} __end_packed NdpPrefixInfoOption;


/**
 * @brief Redirected Header option (RHO)
 **/

typedef __start_packed struct
{
   uint8_t type;       //0
   uint8_t length;     //1
   uint16_t reserved1; //2-3
   uint32_t reserved2; //4-7
   uint8_t ipPacket[]; //8
} __end_packed NdpRedirectedHeaderOption;


/**
 * @brief MTU option
 **/

typedef __start_packed struct
{
   uint8_t type;      //0
   uint8_t length;    //1
   uint16_t reserved; //2-3
   uint32_t mtu;      //4-7
} __end_packed NdpMtuOption;


/**
 * @brief Route Information option (RIO)
 **/

typedef __start_packed struct
{
   uint8_t type;           //0
   uint8_t length;         //1
   uint8_t prefixLength;   //2
#if defined(_CPU_BIG_ENDIAN) && !defined(__IAR_SYSTEMS_ICC__)
   uint8_t reserved1 : 3;  //3
   uint8_t prf : 2;
   uint8_t reserved2 : 3;
#else
   uint8_t reserved2 : 3;  //3
   uint8_t prf : 2;
   uint8_t reserved1 : 3;
#endif
   uint32_t routeLifetime; //4-7
   Ipv6Addr prefix;        //8
} __end_packed NdpRouteInfoOption;


/**
 * @brief Recursive DNS Server option (RDNSS)
 **/

typedef __start_packed struct
{
   uint8_t type;       //0
   uint8_t length;     //1
   uint16_t reserved;  //2-3
   uint32_t lifetime;  //4-7
   Ipv6Addr address[]; //8
} __end_packed NdpRdnssOption;


/**
 * @brief DNS Search List option (DNSSL)
 **/

typedef __start_packed struct
{
   uint8_t type;          //0
   uint8_t length;        //1
   uint16_t reserved;     //2-3
   uint32_t lifetime;     //4-7
   uint8_t domainNames[]; //8
} __end_packed NdpDnsslOption;


/**
 * @brief 6LoWPAN Context option (6CO)
 **/

typedef __start_packed struct
{
   uint8_t type;           //0
   uint8_t length;         //1
   uint8_t contextLength;  //2
#if defined(_CPU_BIG_ENDIAN) && !defined(__IAR_SYSTEMS_ICC__)
   uint8_t reserved1 : 3;  //3
   uint8_t c : 1;
   uint8_t cid : 4;
#else
   uint8_t cid : 4;        //3
   uint8_t c : 1;
   uint8_t reserved1 : 3;
#endif
   uint16_t reserved2;     //4-5
   uint16_t validLifetime; //6-7
   Ipv6Addr contextPrefix; //8
} __end_packed NdpContextOption;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(pop)
#endif


/**
 * @brief NDP queue item
 **/

typedef struct
{
   NetInterface *srcInterface; ///<Interface from which the packet has been received
   NetBuffer *buffer;          ///<Packet waiting for address resolution
   size_t offset;              ///<Offset to the first byte of the packet
   NetTxAncillary ancillary;   ///<Additional options
} NdpQueueItem;


/**
 * @brief Neighbor cache entry
 **/

typedef struct
{
   NdpState state;                              ///<Reachability state
   Ipv6Addr ipAddr;                             ///<Unicast IPv6 address
   MacAddr macAddr;                             ///<Link layer address associated with the IPv6 address
   bool_t isRouter;                             ///<A flag indicating whether the neighbor is a router or a host
   systime_t timestamp;                         ///<Timestamp to manage entry lifetime
   systime_t timeout;                           ///<Timeout value
   uint_t retransmitCount;                      ///<Retransmission counter
   NdpQueueItem queue[NDP_MAX_PENDING_PACKETS]; ///<Packets waiting for address resolution to complete
   uint_t queueSize;                            ///<Number of queued packets
} NdpNeighborCacheEntry;


/**
 * @brief Destination cache entry
 **/

typedef struct
{
   Ipv6Addr destAddr;   ///<Destination IPv6 address
   Ipv6Addr nextHop;    ///<IPv6 address of the next-hop neighbor
   size_t pathMtu;      ///<Path MTU
   systime_t timestamp; ///<Timestamp to manage entry lifetime
} NdpDestCacheEntry;


/**
 * @brief NDP context
 **/

typedef struct
{
   uint32_t reachableTime;                                       ///<The time a node assumes a neighbor is reachable
   uint32_t retransTimer;                                        ///<The time between retransmissions of NS messages
   uint_t dupAddrDetectTransmits;                                ///<Maximum number of NS messages sent while performing DAD
   systime_t minRtrSolicitationDelay;                            ///<Minimum delay before transmitting the first RS message
   systime_t maxRtrSolicitationDelay;                            ///<Maximum delay before transmitting the first RS message
   systime_t rtrSolicitationInterval;                            ///<Time interval between retransmissions of RS messages
   uint_t maxRtrSolicitations;                                   ///<Number of retransmissions for RS messages
   uint_t rtrSolicitationCount;                                  ///<Retransmission counter for RS messages
   bool_t rtrAdvReceived;                                        ///<Valid RA message received
   systime_t timestamp;                                          ///<Timestamp to manage retransmissions
   systime_t timeout;                                            ///<Timeout value
   NdpNeighborCacheEntry neighborCache[NDP_NEIGHBOR_CACHE_SIZE]; ///<Neighbor cache
   NdpDestCacheEntry destCache[NDP_DEST_CACHE_SIZE];             ///<Destination cache
} NdpContext;


//Tick counter to handle periodic operations
extern systime_t ndpTickCounter;

//NDP related functions
error_t ndpInit(NetInterface *interface);

error_t ndpResolve(NetInterface *interface, const Ipv6Addr *ipAddr,
   MacAddr *macAddr);

error_t ndpEnqueuePacket(NetInterface *srcInterface,
   NetInterface *destInterface, const Ipv6Addr *ipAddr, NetBuffer *buffer,
      size_t offset, NetTxAncillary *ancillary);

void ndpTick(NetInterface *interface);
void ndpLinkChangeEvent(NetInterface *interface);

void ndpProcessRouterAdv(NetInterface *interface, Ipv6PseudoHeader *pseudoHeader,
   const NetBuffer *buffer, size_t offset, uint8_t hopLimit);

void ndpProcessNeighborSol(NetInterface *interface, Ipv6PseudoHeader *pseudoHeader,
   const NetBuffer *buffer, size_t offset, uint8_t hopLimit);

void ndpProcessNeighborAdv(NetInterface *interface, Ipv6PseudoHeader *pseudoHeader,
   const NetBuffer *buffer, size_t offset, uint8_t hopLimit);

void ndpProcessRedirect(NetInterface *interface, Ipv6PseudoHeader *pseudoHeader,
   const NetBuffer *buffer, size_t offset, uint8_t hopLimit);

error_t ndpSendRouterSol(NetInterface *interface);

error_t ndpSendNeighborSol(NetInterface *interface,
   const Ipv6Addr *targetIpAddr, bool_t multicast);

error_t ndpSendNeighborAdv(NetInterface *interface,
   const Ipv6Addr *targetIpAddr, const Ipv6Addr *destIpAddr);

error_t ndpSendRedirect(NetInterface *interface, const Ipv6Addr *targetAddr,
   const NetBuffer *ipPacket, size_t ipPacketOffset);

void ndpDumpRouterSolMessage(const NdpRouterSolMessage *message);
void ndpDumpRouterAdvMessage(const NdpRouterAdvMessage *message);
void ndpDumpNeighborSolMessage(const NdpNeighborSolMessage *message);
void ndpDumpNeighborAdvMessage(const NdpNeighborAdvMessage *message);
void ndpDumpRedirectMessage(const NdpRedirectMessage *message);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
