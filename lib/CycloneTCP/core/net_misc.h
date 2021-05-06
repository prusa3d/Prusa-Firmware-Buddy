/**
 * @file net_misc.h
 * @brief Helper functions for TCP/IP stack
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

#ifndef _NET_MISC_H
#define _NET_MISC_H

//Forward declaration of NetTxAncillary structure
struct _NetTxAncillary;
#define NetTxAncillary struct _NetTxAncillary

//Forward declaration of NetRxAncillary structure
struct _NetRxAncillary;
#define NetRxAncillary struct _NetRxAncillary

//Dependencies
#include "core/net.h"
#include "core/ethernet.h"
#include "core/ip.h"

//Get a given bit of the PRNG internal state
#define NET_RAND_GET_BIT(s, n) ((s[(n - 1) / 8] >> ((n - 1) % 8)) & 1)

//Set a given bit of the PRNG internal state
#define NET_RAND_STATE_SET_BIT(s, n, v) s[(n - 1) / 8] = \
   (s[(n - 1) / 8] & ~(1 << ((n - 1) % 8))) | (v) << ((n - 1) % 8)

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Link change callback
 **/

typedef void (*NetLinkChangeCallback)(NetInterface *interface,
   bool_t linkState, void *param);


/**
 * @brief Link change callback entry
 **/

typedef struct
{
   NetInterface *interface;
   NetLinkChangeCallback callback;
   void *param;
} NetLinkChangeCallbackEntry;


/**
 * @brief Timer callback
 **/

typedef void (*NetTimerCallback)(void *param);


/**
 * @brief Timer callback entry
 **/

typedef struct
{
   systime_t timerValue;
   systime_t timerPeriod;
   NetTimerCallback callback;
   void *param;
} NetTimerCallbackEntry;


/**
 * @brief Timestamp
 **/

typedef struct
{
   uint32_t s;
   uint32_t ns;
} NetTimestamp;


/**
 * @brief Additional options passed to the stack (TX path)
 **/

struct _NetTxAncillary
{
   uint8_t ttl;         ///<Time-to-live value
   bool_t dontRoute;    ///<Do not send the packet via a router
   bool_t routerAlert;  ///<Add an IP Router Alert option
#if (IP_DIFF_SERV_SUPPORT == ENABLED)
   uint8_t dscp;        ///<Differentiated services codepoint
#endif
#if (ETH_SUPPORT == ENABLED)
   MacAddr srcMacAddr;  ///<Source MAC address
   MacAddr destMacAddr; ///<Destination MAC address
#endif
#if (ETH_VLAN_SUPPORT == ENABLED)
   int8_t vlanPcp;      ///<VLAN priority (802.1Q)
   int8_t vlanDei;      ///<Drop eligible indicator
#endif
#if (ETH_VMAN_SUPPORT == ENABLED)
   int8_t vmanPcp;      ///<VMAN priority (802.1ad)
   int8_t vmanDei;      ///<Drop eligible indicator
#endif
#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
   uint8_t port;        ///<Switch port identifier
#endif
#if (ETH_TIMESTAMP_SUPPORT == ENABLED)
   int32_t timestampId; ///<Unique identifier for hardware time stamping
#endif
};


/**
 * @brief Additional options passed to the stack (RX path)
 **/

struct _NetRxAncillary
{
   uint8_t ttl;            ///<Time-to-live value
#if (ETH_SUPPORT == ENABLED)
   MacAddr srcMacAddr;     ///<Source MAC address
   MacAddr destMacAddr;    ///<Destination MAC address
#endif
#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
   uint8_t port;           ///<Switch port identifier
#endif
#if (ETH_TIMESTAMP_SUPPORT == ENABLED)
   NetTimestamp timestamp; ///<Captured time stamp
#endif
};


/**
 * @brief Timer
 **/

typedef struct
{
   bool_t running;
   systime_t startTime;
   systime_t interval;
} NetTimer;


/**
 * @brief Pseudo-random number generator state
 **/

typedef struct
{
   uint16_t counter;
   uint8_t s[36];
} NetRandState;


//Global constants
extern const NetTxAncillary NET_DEFAULT_TX_ANCILLARY;
extern const NetRxAncillary NET_DEFAULT_RX_ANCILLARY;

//TCP/IP stack related functions
error_t netAttachLinkChangeCallback(NetInterface *interface,
   NetLinkChangeCallback callback, void *param);

error_t netDetachLinkChangeCallback(NetInterface *interface,
   NetLinkChangeCallback callback, void *param);

void netProcessLinkChange(NetInterface *interface);

error_t netAttachTimerCallback(systime_t period, NetTimerCallback callback,
   void *param);

error_t netDetachTimerCallback(NetTimerCallback callback, void *param);

void netTick(void);

void netStartTimer(NetTimer *timer, systime_t interval);
void netStopTimer(NetTimer *timer);
bool_t netTimerRunning(NetTimer *timer);
bool_t netTimerExpired(NetTimer *timer);

void netInitRand(void);
uint32_t netGetRand(void);
int32_t netGetRandRange(int32_t min, int32_t max);
uint32_t netGetRandBit(NetRandState *state);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
