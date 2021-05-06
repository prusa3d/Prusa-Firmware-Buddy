/**
 * @file dhcpv6_client.h
 * @brief DHCPv6 client (Dynamic Host Configuration Protocol for IPv6)
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

#ifndef _DHCPV6_CLIENT_H
#define _DHCPV6_CLIENT_H

//Dependencies
#include "dhcpv6/dhcpv6_common.h"
#include "core/socket.h"

//DHCPv6 client support
#ifndef DHCPV6_CLIENT_SUPPORT
   #define DHCPV6_CLIENT_SUPPORT DISABLED
#elif (DHCPV6_CLIENT_SUPPORT != ENABLED && DHCPV6_CLIENT_SUPPORT != DISABLED)
   #error DHCPV6_CLIENT_SUPPORT parameter is not valid
#endif

//DHCPv6 client tick interval
#ifndef DHCPV6_CLIENT_TICK_INTERVAL
   #define DHCPV6_CLIENT_TICK_INTERVAL 200
#elif (DHCPV6_CLIENT_TICK_INTERVAL < 10)
   #error DHCPV6_CLIENT_TICK_INTERVAL parameter is not valid
#endif

//Maximum number of IPv6 addresses in the client's IA
#ifndef DHCPV6_CLIENT_ADDR_LIST_SIZE
   #define DHCPV6_CLIENT_ADDR_LIST_SIZE 2
#elif (DHCPV6_CLIENT_ADDR_LIST_SIZE < 1)
   #error DHCPV6_CLIENT_ADDR_LIST_SIZE parameter is not valid
#endif

//Maximum size of the client's FQDN
#ifndef DHCPV6_CLIENT_MAX_FQDN_SIZE
   #define DHCPV6_CLIENT_MAX_FQDN_SIZE 16
#elif (DHCPV6_CLIENT_MAX_FQDN_SIZE < 1)
   #error DHCPV6_CLIENT_MAX_FQDN_SIZE parameter is not valid
#endif

//Max delay of first Solicit
#ifndef DHCPV6_CLIENT_SOL_MAX_DELAY
   #define DHCPV6_CLIENT_SOL_MAX_DELAY 1000
#elif (DHCPV6_CLIENT_SOL_MAX_DELAY < 100)
   #error DHCPV6_CLIENT_SOL_MAX_DELAY parameter is not valid
#endif

//Initial Solicit timeout
#ifndef DHCPV6_CLIENT_SOL_TIMEOUT
   #define DHCPV6_CLIENT_SOL_TIMEOUT 1000
#elif (DHCPV6_CLIENT_SOL_TIMEOUT < 100)
   #error DHCPV6_CLIENT_SOL_TIMEOUT parameter is not valid
#endif

//Max Solicit timeout value
#ifndef DHCPV6_CLIENT_SOL_MAX_RT
   #define DHCPV6_CLIENT_SOL_MAX_RT 120000
#elif (DHCPV6_CLIENT_SOL_MAX_RT < 100)
   #error DHCPV6_CLIENT_SOL_MAX_RT parameter is not valid
#endif

//Initial Request timeout
#ifndef DHCPV6_CLIENT_REQ_TIMEOUT
   #define DHCPV6_CLIENT_REQ_TIMEOUT 1000
#elif (DHCPV6_CLIENT_REQ_TIMEOUT < 100)
   #error DHCPV6_CLIENT_REQ_TIMEOUT parameter is not valid
#endif

//Max Request timeout value
#ifndef DHCPV6_CLIENT_REQ_MAX_RT
   #define DHCPV6_CLIENT_REQ_MAX_RT 30000
#elif (DHCPV6_CLIENT_REQ_MAX_RT < 100)
   #error DHCPV6_CLIENT_REQ_MAX_RT parameter is not valid
#endif

//Max Request retry attempts
#ifndef DHCPV6_CLIENT_REQ_MAX_RC
   #define DHCPV6_CLIENT_REQ_MAX_RC 10
#elif (DHCPV6_CLIENT_REQ_MAX_RC < 1)
   #error DHCPV6_CLIENT_REQ_MAX_RC parameter is not valid
#endif

//Max delay of first Confirm
#ifndef DHCPV6_CLIENT_CNF_MAX_DELAY
   #define DHCPV6_CLIENT_CNF_MAX_DELAY 1000
#elif (DHCPV6_CLIENT_CNF_MAX_DELAY < 100)
   #error DHCPV6_CLIENT_CNF_MAX_DELAY parameter is not valid
#endif

//Initial Confirm timeout
#ifndef DHCPV6_CLIENT_CNF_TIMEOUT
   #define DHCPV6_CLIENT_CNF_TIMEOUT 1000
#elif (DHCPV6_CLIENT_CNF_TIMEOUT < 100)
   #error DHCPV6_CLIENT_CNF_TIMEOUT parameter is not valid
#endif

//Max Confirm timeout
#ifndef DHCPV6_CLIENT_CNF_MAX_RT
   #define DHCPV6_CLIENT_CNF_MAX_RT 4000
#elif (DHCPV6_CLIENT_CNF_MAX_RT < 100)
   #error DHCPV6_CLIENT_CNF_MAX_RT parameter is not valid
#endif

//Max Confirm duration
#ifndef DHCPV6_CLIENT_CNF_MAX_RD
   #define DHCPV6_CLIENT_CNF_MAX_RD 10000
#elif (DHCPV6_CLIENT_CNF_MAX_RD < 100)
   #error DHCPV6_CLIENT_CNF_MAX_RD parameter is not valid
#endif

//Initial Renew timeout
#ifndef DHCPV6_CLIENT_REN_TIMEOUT
   #define DHCPV6_CLIENT_REN_TIMEOUT 10000
#elif (DHCPV6_CLIENT_REN_TIMEOUT < 100)
   #error DHCPV6_CLIENT_REN_TIMEOUT parameter is not valid
#endif

//Max Renew timeout value
#ifndef DHCPV6_CLIENT_REN_MAX_RT
   #define DHCPV6_CLIENT_REN_MAX_RT 600000
#elif (DHCPV6_CLIENT_REN_MAX_RT < 100)
   #error DHCPV6_CLIENT_REN_MAX_RT parameter is not valid
#endif

//Initial Rebind timeout
#ifndef DHCPV6_CLIENT_REB_TIMEOUT
   #define DHCPV6_CLIENT_REB_TIMEOUT 10000
#elif (DHCPV6_CLIENT_REB_TIMEOUT < 100)
   #error DHCPV6_CLIENT_REB_TIMEOUT parameter is not valid
#endif

//Max Rebind timeout value
#ifndef DHCPV6_CLIENT_REB_MAX_RT
   #define DHCPV6_CLIENT_REB_MAX_RT 600000
#elif (DHCPV6_CLIENT_REB_MAX_RT < 100)
   #error DHCPV6_CLIENT_REB_MAX_RT parameter is not valid
#endif

//Max delay of first Information-request
#ifndef DHCPV6_CLIENT_INF_MAX_DELAY
   #define DHCPV6_CLIENT_INF_MAX_DELAY 1000
#elif (DHCPV6_CLIENT_INF_MAX_DELAY < 100)
   #error DHCPV6_CLIENT_INF_MAX_DELAY parameter is not valid
#endif

//Initial Information-request timeout
#ifndef DHCPV6_CLIENT_INF_TIMEOUT
   #define DHCPV6_CLIENT_INF_TIMEOUT 1000
#elif (DHCPV6_CLIENT_INF_TIMEOUT < 100)
   #error DHCPV6_CLIENT_INF_TIMEOUT parameter is not valid
#endif

//Max Information-request timeout value
#ifndef DHCPV6_CLIENT_INF_MAX_RT
   #define DHCPV6_CLIENT_INF_MAX_RT 120000
#elif (DHCPV6_CLIENT_INF_MAX_RT < 1000)
   #error DHCPV6_CLIENT_INF_MAX_RT parameter is not valid
#endif

//Initial Release timeout
#ifndef DHCPV6_CLIENT_REL_TIMEOUT
   #define DHCPV6_CLIENT_REL_TIMEOUT 1000
#elif (DHCPV6_CLIENT_REL_TIMEOUT < 100)
   #error DHCPV6_CLIENT_REL_TIMEOUT parameter is not valid
#endif

//Max Release attempts
#ifndef DHCPV6_CLIENT_REL_MAX_RC
   #define DHCPV6_CLIENT_REL_MAX_RC 5
#elif (DHCPV6_CLIENT_REL_MAX_RC < 1)
   #error DHCPV6_CLIENT_REL_MAX_RC parameter is not valid
#endif

//Initial Decline timeout
#ifndef DHCPV6_CLIENT_DEC_TIMEOUT
   #define DHCPV6_CLIENT_DEC_TIMEOUT 1000
#elif (DHCPV6_CLIENT_DEC_TIMEOUT < 100)
   #error DHCPV6_CLIENT_DEC_TIMEOUT parameter is not valid
#endif

//Max Decline attempts
#ifndef DHCPV6_CLIENT_DEC_MAX_RC
   #define DHCPV6_CLIENT_DEC_MAX_RC 5
#elif (DHCPV6_CLIENT_DEC_MAX_RC < 1)
   #error DHCPV6_CLIENT_DEC_MAX_RC parameter is not valid
#endif

//Initial Reconfigure timeout
#ifndef DHCPV6_CLIENT_REC_TIMEOUT
   #define DHCPV6_CLIENT_REC_TIMEOUT 2000
#elif (DHCPV6_CLIENT_REC_TIMEOUT < 100)
   #error DHCPV6_CLIENT_REC_TIMEOUT parameter is not valid
#endif

//Max Reconfigure attempts
#ifndef DHCPV6_CLIENT_REC_MAX_RC
   #define DHCPV6_CLIENT_REC_MAX_RC 8
#elif (DHCPV6_CLIENT_REC_MAX_RC < 1)
   #error DHCPV6_CLIENT_REC_MAX_RC parameter is not valid
#endif

//Forward declaration of Dhcpv6ClientContext structure
struct _Dhcpv6ClientContext;
#define Dhcpv6ClientContext struct _Dhcpv6ClientContext

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief DHCPv6 client FSM states
 **/

typedef enum
{
   DHCPV6_STATE_INIT         = 0,
   DHCPV6_STATE_SOLICIT      = 1,
   DHCPV6_STATE_REQUEST      = 2,
   DHCPV6_STATE_INIT_CONFIRM = 3,
   DHCPV6_STATE_CONFIRM      = 4,
   DHCPV6_STATE_DAD          = 5,
   DHCPV6_STATE_BOUND        = 6,
   DHCPV6_STATE_RENEW        = 7,
   DHCPV6_STATE_REBIND       = 8,
   DHCPV6_STATE_RELEASE      = 9,
   DHCPV6_STATE_DECLINE      = 10
} Dhcpv6State;


/**
 * @brief DHCPv6 configuration timeout callback
 **/

typedef void (*Dhcpv6TimeoutCallback)(Dhcpv6ClientContext *context,
   NetInterface *interface);


/**
 * @brief Link state change callback
 **/

typedef void (*Dhcpv6LinkChangeCallback)(Dhcpv6ClientContext *context,
   NetInterface *interface, bool_t linkState);


/**
 * @brief FSM state change callback
 **/

typedef void (*Dhcpv6StateChangeCallback)(Dhcpv6ClientContext *context,
   NetInterface *interface, Dhcpv6State state);


/**
 * @brief DHCPv6 client settings
 **/

typedef struct
{
   NetInterface *interface;                    ///<Network interface to configure
   bool_t rapidCommit;                         ///<Quick configuration using rapid commit
   bool_t manualDnsConfig;                     ///<Force manual DNS configuration
   systime_t timeout;                          ///<DHCPv6 configuration timeout
   Dhcpv6TimeoutCallback timeoutEvent;         ///<DHCPv6 configuration timeout event
   Dhcpv6LinkChangeCallback linkChangeEvent;   ///<Link state change event
   Dhcpv6StateChangeCallback stateChangeEvent; ///<FSM state change event
} Dhcpv6ClientSettings;


/**
 * @brief IA address entry
 **/

typedef struct
{
   Ipv6Addr addr;              ///<IPv6 address
   uint32_t validLifetime;     ///<Valid lifetime
   uint32_t preferredLifetime; ///<Preferred lifetime
} Dhcpv6ClientAddrEntry;


/**
 * @brief Client's IA (Identity Association)
 **/

typedef struct
{
   uint32_t t1;                                                  ///<T1 parameter
   uint32_t t2;                                                  ///<T2 parameter
   Dhcpv6ClientAddrEntry addrList[DHCPV6_CLIENT_ADDR_LIST_SIZE]; ///<Set of IPv6 addresses
} Dhcpv6ClientIa;


/**
 * @brief DHCPv6 client context
 **/

struct _Dhcpv6ClientContext
{
   Dhcpv6ClientSettings settings;                   ///<DHCPv6 client settings
   bool_t running;                                  ///<This flag tells whether the DHCP client is running or not
   Dhcpv6State state;                               ///<Current state of the FSM
   bool_t timeoutEventDone;                         ///<Timeout callback function has been called
   systime_t timestamp;                             ///<Timestamp to manage retransmissions
   systime_t timeout;                               ///<Timeout value
   uint_t retransmitCount;                          ///<Retransmission counter
   uint8_t clientId[DHCPV6_MAX_DUID_SIZE];          ///<Client DUID
   size_t clientIdLength;                           ///<Length of the client DUID
   uint8_t clientFqdn[DHCPV6_CLIENT_MAX_FQDN_SIZE]; ///<Client's fully qualified domain name
   size_t clientFqdnLength;                         ///<Length of the client's FQDN
   uint8_t serverId[DHCPV6_MAX_DUID_SIZE];          ///<Server DUID
   size_t serverIdLength;                           ///<Length of the server DUID
   int_t serverPreference;                          ///<Preference value for the server
   uint32_t transactionId;                          ///<Value to match requests with replies
   systime_t configStartTime;                       ///<Address acquisition or renewal process start time
   systime_t exchangeStartTime;                     ///<Time at which the client sent the first message
   systime_t leaseStartTime;                        ///<Lease start time
   Dhcpv6ClientIa ia;                               ///<Identity association
};


//Tick counter to handle periodic operations
extern systime_t dhcpv6ClientTickCounter;

//DHCPv6 client related functions
void dhcpv6ClientGetDefaultSettings(Dhcpv6ClientSettings *settings);
error_t dhcpv6ClientInit(Dhcpv6ClientContext *context, const Dhcpv6ClientSettings *settings);
error_t dhcpv6ClientStart(Dhcpv6ClientContext *context);
error_t dhcpv6ClientStop(Dhcpv6ClientContext *context);
error_t dhcpv6ClientRelease(Dhcpv6ClientContext *context);
Dhcpv6State dhcpv6ClientGetState(Dhcpv6ClientContext *context);

void dhcpv6ClientTick(Dhcpv6ClientContext *context);
void dhcpv6ClientLinkChangeEvent(Dhcpv6ClientContext *context);

void dhcpv6ClientStateInit(Dhcpv6ClientContext *context);
void dhcpv6ClientStateSolicit(Dhcpv6ClientContext *context);
void dhcpv6ClientStateRequest(Dhcpv6ClientContext *context);
void dhcpv6ClientStateInitConfirm(Dhcpv6ClientContext *context);
void dhcpv6ClientStateConfirm(Dhcpv6ClientContext *context);
void dhcpv6ClientStateDad(Dhcpv6ClientContext *context);
void dhcpv6ClientStateBound(Dhcpv6ClientContext *context);
void dhcpv6ClientStateRenew(Dhcpv6ClientContext *context);
void dhcpv6ClientStateRebind(Dhcpv6ClientContext *context);
void dhcpv6ClientStateRelease(Dhcpv6ClientContext *context);
void dhcpv6ClientStateDecline(Dhcpv6ClientContext *context);

error_t dhcpv6ClientSendMessage(Dhcpv6ClientContext *context,
   Dhcpv6MessageType type);

void dhcpv6ClientProcessMessage(NetInterface *interface,
   const IpPseudoHeader *pseudoHeader, const UdpHeader *udpHeader,
   const NetBuffer *buffer, size_t offset, const NetRxAncillary *ancillary,
   void *param);

void dhcpv6ClientParseAdvertise(Dhcpv6ClientContext *context,
   const Dhcpv6Message *message, size_t length);

void dhcpv6ClientParseReply(Dhcpv6ClientContext *context,
   const Dhcpv6Message *message, size_t length);

error_t dhcpv6ClientParseIaNaOption(Dhcpv6ClientContext *context,
   const Dhcpv6Option *option);

error_t dhcpv6ClientParseIaAddrOption(Dhcpv6ClientContext *context,
   const Dhcpv6Option *option);

void dhcpv6ClientAddAddr(Dhcpv6ClientContext *context, const Ipv6Addr *addr,
   uint32_t validLifetime, uint32_t preferredLifetime);

void dhcpv6ClientRemoveAddr(Dhcpv6ClientContext *context, const Ipv6Addr *addr);

void dhcpv6ClientFlushAddrList(Dhcpv6ClientContext *context);

error_t dhcpv6ClientGenerateDuid(Dhcpv6ClientContext *context);
error_t dhcpv6ClientGenerateFqdn(Dhcpv6ClientContext *context);
error_t dhcpv6ClientGenerateLinkLocalAddr(Dhcpv6ClientContext *context);

bool_t dhcpv6ClientCheckServerId(Dhcpv6ClientContext *context,
   Dhcpv6Option *serverIdOption);

void dhcpv6ClientCheckTimeout(Dhcpv6ClientContext *context);

uint16_t dhcpv6ClientComputeElapsedTime(Dhcpv6ClientContext *context);

void dhcpv6ClientChangeState(Dhcpv6ClientContext *context,
   Dhcpv6State newState, systime_t delay);

void dhcpv6ClientDumpConfig(Dhcpv6ClientContext *context);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
