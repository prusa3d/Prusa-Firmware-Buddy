/**
 * @file slaac.h
 * @brief IPv6 Stateless Address Autoconfiguration
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

#ifndef _SLAAC_H
#define _SLAAC_H

//Dependencies
#include "core/net.h"
#include "ipv6/ndp.h"

//SLAAC support
#ifndef SLAAC_SUPPORT
   #define SLAAC_SUPPORT ENABLED
#elif (SLAAC_SUPPORT != ENABLED && SLAAC_SUPPORT != DISABLED)
   #error SLAAC_SUPPORT parameter is not valid
#endif

//Time constant
#define SLAAC_LIFETIME_2_HOURS (2 * 3600 * 1000)

//Forward declaration of SlaacContext structure
struct _SlaacContext;
#define SlaacContext struct _SlaacContext

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Link state change callback
 **/

typedef void (*SlaacLinkChangeCallback)(SlaacContext *context,
   NetInterface *interface, bool_t linkState);


/**
 * @brief Router Advertisement parsing callback
 **/

typedef void (*SlaacParseRouterAdvCallback)(SlaacContext *context,
   NdpRouterAdvMessage *message, size_t length);


/**
 * @brief SLAAC settings
 **/

typedef struct
{
   NetInterface *interface;                            ///<Network interface to configure
   bool_t manualDnsConfig;                             ///<Force manual DNS configuration
   SlaacLinkChangeCallback linkChangeEvent;            ///<Link state change event
   SlaacParseRouterAdvCallback parseRouterAdvCallback; ///<Router Advertisement parsing callback
} SlaacSettings;


/**
 * @brief SLAAC context
 **/

struct _SlaacContext
{
   SlaacSettings settings; ///<SLAAC settings
   bool_t running;         ///<SLAAC is currently running
   bool_t configUpdated;   ///<This flag is set when IPv6 configuration has been updated
};


//SLAAC related functions
void slaacGetDefaultSettings(SlaacSettings *settings);
error_t slaacInit(SlaacContext *context, const SlaacSettings *settings);
error_t slaacStart(SlaacContext *context);
error_t slaacStop(SlaacContext *context);

void slaacLinkChangeEvent(SlaacContext *context);

void slaacParseRouterAdv(SlaacContext *context,
   NdpRouterAdvMessage *message, size_t length);

void slaacParsePrefixInfoOption(SlaacContext *context,
   NdpPrefixInfoOption *option);

error_t slaacGenerateLinkLocalAddr(SlaacContext *context);

void slaacDumpConfig(SlaacContext *context);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
