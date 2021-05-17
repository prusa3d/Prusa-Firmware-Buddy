/**
 * @file ndp_router_adv.h
 * @brief Router advertisement service
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

#ifndef _NDP_ROUTER_ADV_H
#define _NDP_ROUTER_ADV_H

//Dependencies
#include "core/net.h"
#include "ipv6/ipv6.h"

//RA service support
#ifndef NDP_ROUTER_ADV_SUPPORT
   #define NDP_ROUTER_ADV_SUPPORT DISABLED
#elif (NDP_ROUTER_ADV_SUPPORT != ENABLED && NDP_ROUTER_ADV_SUPPORT != DISABLED)
   #error NDP_ROUTER_ADV_SUPPORT parameter is not valid
#endif

//RA service tick interval
#ifndef NDP_ROUTER_ADV_TICK_INTERVAL
   #define NDP_ROUTER_ADV_TICK_INTERVAL 100
#elif (NDP_ROUTER_ADV_TICK_INTERVAL < 10)
   #error NDP_ROUTER_ADV_TICK_INTERVAL parameter is not valid
#endif

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief IPv6 prefix information
 **/

typedef struct
{
   Ipv6Addr prefix;
   uint8_t length;
   bool_t onLinkFlag;
   bool_t autonomousFlag;
   uint32_t validLifetime;
   uint32_t preferredLifetime;
} NdpRouterAdvPrefixInfo;


/**
 * @brief Route information
 **/

typedef struct
{
   Ipv6Addr prefix;
   uint8_t length;
   uint8_t preference;
   uint32_t routeLifetime;
} NdpRouterAdvRouteInfo;


/**
 * @brief Context information for 6LoWPAN header compression
 **/

typedef struct
{
   uint8_t cid;
   Ipv6Addr prefix;
   uint8_t length;
   bool_t compression;
   uint16_t validLifetime;
} NdpRouterAdvContextInfo;


/**
 * @brief RA service settings
 **/

typedef struct
{
   NetInterface *interface;
   systime_t maxRtrAdvInterval;
   systime_t minRtrAdvInterval;
   uint8_t curHopLimit;
   bool_t managedFlag;
   bool_t otherConfigFlag;
   bool_t homeAgentFlag;
   uint8_t preference;
   bool_t proxyFlag;
   uint16_t defaultLifetime;
   uint32_t reachableTime;
   uint32_t retransTimer;
   uint32_t linkMtu;
   NdpRouterAdvPrefixInfo *prefixList;
   uint_t prefixListLength;
   NdpRouterAdvRouteInfo *routeList;
   uint_t routeListLength;
   NdpRouterAdvContextInfo *contextList;
   uint_t contextListLength;
} NdpRouterAdvSettings;


/**
 * @brief RA service context
 **/

typedef struct
{
   NdpRouterAdvSettings settings; ///<RA service settings
   bool_t running;                ///<This flag tells whether the RA service is running
   systime_t timestamp;           ///<Timestamp to manage retransmissions
   systime_t timeout;             ///<Timeout value
   uint_t routerAdvCount;         ///<Router Advertisement message counter
} NdpRouterAdvContext;


//Tick counter to handle periodic operations
extern systime_t ndpRouterAdvTickCounter;

//RA service related functions
void ndpRouterAdvGetDefaultSettings(NdpRouterAdvSettings *settings);

error_t ndpRouterAdvInit(NdpRouterAdvContext *context,
   const NdpRouterAdvSettings *settings);

error_t ndpRouterAdvStart(NdpRouterAdvContext *context);
error_t ndpRouterAdvStop(NdpRouterAdvContext *context);

void ndpRouterAdvTick(NdpRouterAdvContext *context);
void ndpRouterAdvLinkChangeEvent(NdpRouterAdvContext *context);

void ndpProcessRouterSol(NetInterface *interface, Ipv6PseudoHeader *pseudoHeader,
   const NetBuffer *buffer, size_t offset, uint8_t hopLimit);

error_t ndpSendRouterAdv(NdpRouterAdvContext *context, uint16_t routerLifetime);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
