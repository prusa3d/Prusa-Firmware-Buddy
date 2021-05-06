/**
 * @file snmp_community_mib_module.h
 * @brief SNMP COMMUNITY MIB module
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
 * The SNMP-MIB describes managed objects which describe the behavior
 * of an SNMP entity. Refer to RFC 3418 for more details
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

#ifndef _SNMP_COMMUNITY_MIB_MODULE_H
#define _SNMP_COMMUNITY_MIB_MODULE_H

//Dependencies
#include "mibs/mib_common.h"
#include "snmp/snmp_agent.h"

//SNMP COMMUNITY MIB module support
#ifndef SNMP_COMMUNITY_MIB_SUPPORT
   #define SNMP_COMMUNITY_MIB_SUPPORT DISABLED
#elif (SNMP_COMMUNITY_MIB_SUPPORT != ENABLED && SNMP_COMMUNITY_MIB_SUPPORT != DISABLED)
   #error SNMP_COMMUNITY_MIB_SUPPORT parameter is not valid
#endif

//Support for SET operations
#ifndef SNMP_COMMUNITY_MIB_SET_SUPPORT
   #define SNMP_COMMUNITY_MIB_SET_SUPPORT DISABLED
#elif (SNMP_COMMUNITY_MIB_SET_SUPPORT != ENABLED && SNMP_COMMUNITY_MIB_SET_SUPPORT != DISABLED)
   #error SNMP_COMMUNITY_MIB_SET_SUPPORT parameter is not valid
#endif

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief SNMP COMMUNITY MIB base
 **/

typedef struct
{
   SnmpAgentContext *context;
   SnmpUserEntry tempCommunity;
} SnmpCommunityMibBase;


//SNMP COMMUNITY MIB related constants
extern SnmpCommunityMibBase snmpCommunityMibBase;
extern const MibObject snmpCommunityMibObjects[];
extern const MibModule snmpCommunityMibModule;

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
