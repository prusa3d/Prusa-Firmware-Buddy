/**
 * @file mdns_responder.h
 * @brief mDNS responder (Multicast DNS)
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

#ifndef _MDNS_RESPONDER_H
#define _MDNS_RESPONDER_H

//Dependencies
#include "core/net.h"
#include "core/udp.h"
#include "dns/dns_common.h"
#include "mdns/mdns_common.h"

//mDNS responder support
#ifndef MDNS_RESPONDER_SUPPORT
   #define MDNS_RESPONDER_SUPPORT DISABLED
#elif (MDNS_RESPONDER_SUPPORT != ENABLED && MDNS_RESPONDER_SUPPORT != DISABLED)
   #error MDNS_RESPONDER_SUPPORT parameter is not valid
#endif

//mDNS responder tick interval
#ifndef MDNS_RESPONDER_TICK_INTERVAL
   #define MDNS_RESPONDER_TICK_INTERVAL 250
#elif (MDNS_RESPONDER_TICK_INTERVAL < 10)
   #error MDNS_RESPONDER_TICK_INTERVAL parameter is not valid
#endif

//Maximum length of host name
#ifndef MDNS_RESPONDER_MAX_HOSTNAME_LEN
   #define MDNS_RESPONDER_MAX_HOSTNAME_LEN 32
#elif (MDNS_RESPONDER_MAX_HOSTNAME_LEN < 1)
   #error MDNS_RESPONDER_MAX_HOSTNAME_LEN parameter is not valid
#endif

//Maximum waiting delay
#ifndef MDNS_MAX_WAITING_DELAY
   #define MDNS_MAX_WAITING_DELAY 10000
#elif (MDNS_MAX_WAITING_DELAY < 0)
   #error MDNS_MAX_WAITING_DELAY parameter is not valid
#endif

//Initial random delay (minimum value)
#ifndef MDNS_RAND_DELAY_MIN
   #define MDNS_RAND_DELAY_MIN 0
#elif (MDNS_RAND_DELAY_MIN < 0)
   #error MDNS_RAND_DELAY_MIN parameter is not valid
#endif

//Initial random delay (maximum value)
#ifndef MDNS_RAND_DELAY_MAX
   #define MDNS_RAND_DELAY_MAX 250
#elif (MDNS_RAND_DELAY_MAX < 0)
   #error MDNS_RAND_DELAY_MAX parameter is not valid
#endif

//Number of probe packets
#ifndef MDNS_PROBE_NUM
   #define MDNS_PROBE_NUM 3
#elif (MDNS_PROBE_NUM < 1)
   #error MDNS_PROBE_NUM parameter is not valid
#endif

//Time interval between subsequent probe packets
#ifndef MDNS_PROBE_DELAY
   #define MDNS_PROBE_DELAY 250
#elif (MDNS_PROBE_DELAY < 100)
   #error MDNS_PROBE_DELAY parameter is not valid
#endif

//Delay before probing again when deferring to the winning host
#ifndef MDNS_PROBE_DEFER
   #define MDNS_PROBE_DEFER 1000
#elif (MDNS_PROBE_DEFER < 100)
   #error MDNS_PROBE_DEFER parameter is not valid
#endif

//Number of announcement packets
#ifndef MDNS_ANNOUNCE_NUM
   #define MDNS_ANNOUNCE_NUM 2
#elif (MDNS_ANNOUNCE_NUM < 1)
   #error MDNS_ANNOUNCE_NUM parameter is not valid
#endif

//Time interval between subsequent announcement packets
#ifndef MDNS_ANNOUNCE_DELAY
   #define MDNS_ANNOUNCE_DELAY 1000
#elif (MDNS_ANNOUNCE_DELAY < 100)
   #error MDNS_ANNOUNCE_DELAY parameter is not valid
#endif

//Forward declaration of DnsSdContext structure
struct _MdnsResponderContext;
#define MdnsResponderContext struct _MdnsResponderContext

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief mDNS responder states
 **/

typedef enum
{
   MDNS_STATE_INIT       = 0,
   MDNS_STATE_WAITING    = 1,
   MDNS_STATE_PROBING    = 2,
   MDNS_STATE_ANNOUNCING = 3,
   MDNS_STATE_IDLE       = 4
} MdnsState;


/**
 * @brief FSM state change callback
 **/

typedef void (*MdnsResponderStateChangeCallback)(MdnsResponderContext *context,
   NetInterface *interface, MdnsState state);


/**
 * @brief mDNS responder settings
 **/

typedef struct
{
   NetInterface *interface;                           ///<Underlying network interface
   uint_t numAnnouncements;                           ///<Number of announcement packets
   uint32_t ttl;                                      ///<TTL resource record
   MdnsResponderStateChangeCallback stateChangeEvent; ///<FSM state change event
} MdnsResponderSettings;


/**
 * @brief mDNS responder context
 **/

struct _MdnsResponderContext
{
   MdnsResponderSettings settings;                            ///<DNS-SD settings
   bool_t running;                                            ///<mDNS responder is currently running
   MdnsState state;                                           ///<FSM state
   bool_t conflict;                                           ///<Conflict detected
   bool_t tieBreakLost;                                       ///<Tie-break lost
   systime_t timestamp;                                       ///<Timestamp to manage retransmissions
   systime_t timeout;                                         ///<Timeout value
   uint_t retransmitCount;                                    ///<Retransmission counter
   char_t hostname[MDNS_RESPONDER_MAX_HOSTNAME_LEN + 1];      ///<Hostname
#if (IPV4_SUPPORT == ENABLED)
   char_t ipv4ReverseName[DNS_MAX_IPV4_REVERSE_NAME_LEN + 1]; ///<Reverse DNS lookup for IPv4
   MdnsMessage ipv4Response;                                  ///<IPv4 response message
#endif
#if (IPV6_SUPPORT == ENABLED)
   char_t ipv6ReverseName[DNS_MAX_IPV6_REVERSE_NAME_LEN + 1]; ///<Reverse DNS lookup for IPv6
   MdnsMessage ipv6Response;                                  ///<IPv6 response message
#endif
};


//Tick counter to handle periodic operations
extern systime_t mdnsResponderTickCounter;

//mDNS related functions
void mdnsResponderGetDefaultSettings(MdnsResponderSettings *settings);

error_t mdnsResponderInit(MdnsResponderContext *context,
   const MdnsResponderSettings *settings);

error_t mdnsResponderStart(MdnsResponderContext *context);
error_t mdnsResponderStop(MdnsResponderContext *context);
MdnsState mdnsResponderGetState(MdnsResponderContext *context);

error_t mdnsResponderSetHostname(MdnsResponderContext *context,
   const char_t *hostname);

error_t mdnsResponderSetIpv4ReverseName(MdnsResponderContext *context);
error_t mdnsResponderSetIpv6ReverseName(MdnsResponderContext *context);

error_t mdnsResponderStartProbing(MdnsResponderContext *context);

void mdnsResponderTick(MdnsResponderContext *context);
void mdnsResponderLinkChangeEvent(MdnsResponderContext *context);

void mdnsResponderChangeState(MdnsResponderContext *context,
   MdnsState newState, systime_t delay);

void mdnsResponderChangeHostname(MdnsResponderContext *context);

error_t mdnsResponderSendProbe(MdnsResponderContext *context);
error_t mdnsResponderSendAnnouncement(MdnsResponderContext *context);
error_t mdnsResponderSendGoodbye(MdnsResponderContext *context);

void mdnsResponderProcessQuery(NetInterface *interface, MdnsMessage *query);

error_t mdnsResponderParseQuestion(NetInterface *interface, const MdnsMessage *query,
   size_t offset, const DnsQuestion *question, MdnsMessage *response);

void mdnsResponderParseKnownAnRecord(NetInterface *interface, const MdnsMessage *query,
   size_t queryOffset, const DnsResourceRecord *queryRecord, MdnsMessage *response);

void mdnsResponderParseNsRecord(NetInterface *interface,
   const MdnsMessage *query, size_t offset, const DnsResourceRecord *record);

void mdnsResponderParseAnRecord(NetInterface *interface,
   const MdnsMessage *response, size_t offset, const DnsResourceRecord *record);

void mdnsResponderGenerateAdditionalRecords(NetInterface *interface,
   MdnsMessage *response, bool_t legacyUnicast);

error_t mdnsResponderAddIpv4AddrRecord(NetInterface *interface,
   MdnsMessage *message, bool_t cacheFlush, uint32_t ttl);

error_t mdnsResponderAddIpv6AddrRecord(NetInterface *interface,
   MdnsMessage *message, bool_t cacheFlush, uint32_t ttl);

error_t mdnsResponderAddIpv4ReversePtrRecord(NetInterface *interface,
   MdnsMessage *message, bool_t cacheFlush, uint32_t ttl);

error_t mdnsResponderAddIpv6ReversePtrRecord(NetInterface *interface,
   MdnsMessage *message, bool_t cacheFlush, uint32_t ttl);

error_t mdnsResponderAddNsecRecord(NetInterface *interface,
   MdnsMessage *message, bool_t cacheFlush, uint32_t ttl);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
