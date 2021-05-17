/**
 * @file snmp_agent_vacm.h
 * @brief View-based Access Control Model (VACM) for SNMP
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

#ifndef _SNMP_AGENT_VACM_H
#define _SNMP_AGENT_VACM_H

//Dependencies
#include "core/net.h"
#include "snmp/snmp_agent.h"
#include "mibs/mib_common.h"
#include "core/crypto.h"

//VACM support
#ifndef SNMP_AGENT_VACM_SUPPORT
   #define SNMP_AGENT_VACM_SUPPORT DISABLED
#elif (SNMP_AGENT_VACM_SUPPORT != ENABLED && SNMP_AGENT_VACM_SUPPORT != DISABLED)
   #error SNMP_AGENT_VACM_SUPPORT parameter is not valid
#endif

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Context match
 **/

typedef enum
{
   SNMP_CONTEXT_MATCH_INVALID = 0,
   SNMP_CONTEXT_MATCH_EXACT   = 1,
   SNMP_CONTEXT_MATCH_PREFIX  = 2
} SnmpContextMatch;


/**
 * @brief View type
 **/

typedef enum
{
   SNMP_VIEW_TYPE_INVALID  = 0,
   SNMP_VIEW_TYPE_INCLUDED = 1,
   SNMP_VIEW_TYPE_EXCLUDED = 2
} SnmpViewType;


/**
 * @brief Group table entry
 **/

typedef struct
{
   MibRowStatus status;
   SnmpSecurityModel securityModel;
   char_t securityName[SNMP_MAX_GROUP_NAME_LEN + 1];
   char_t groupName[SNMP_MAX_GROUP_NAME_LEN + 1];
} SnmpGroupEntry;


/**
 * @brief Access table entry
 **/

typedef struct
{
   MibRowStatus status;
   char_t groupName[SNMP_MAX_GROUP_NAME_LEN + 1];
   char_t contextPrefix[SNMP_MAX_CONTEXT_NAME_LEN + 1];
   SnmpSecurityModel securityModel;
   SnmpSecurityLevel securityLevel;
   SnmpContextMatch contextMatch;
   char_t readViewName[SNMP_MAX_VIEW_NAME_LEN + 1];
   char_t writeViewName[SNMP_MAX_VIEW_NAME_LEN + 1];
   char_t notifyViewName[SNMP_MAX_VIEW_NAME_LEN + 1];
} SnmpAccessEntry;


/**
 * @brief View table entry
 **/

typedef struct
{
   MibRowStatus status;
   char_t viewName[SNMP_MAX_VIEW_NAME_LEN + 1];
   uint8_t subtree[SNMP_MAX_OID_SIZE];
   size_t subtreeLen;
   uint8_t mask[SNMP_MAX_BIT_MASK_SIZE];
   size_t maskLen;
   SnmpViewType type;
} SnmpViewEntry;


//VACM related functions
error_t snmpIsAccessAllowed(SnmpAgentContext *context,
   const SnmpMessage *message, const uint8_t *oid, size_t oidLen);

SnmpGroupEntry *snmpCreateGroupEntry(SnmpAgentContext *context);

SnmpGroupEntry *snmpFindGroupEntry(SnmpAgentContext *context,
   uint_t securityModel, const char_t *securityName, size_t securityNameLen);

SnmpAccessEntry *snmpCreateAccessEntry(SnmpAgentContext *context);

SnmpAccessEntry *snmpFindAccessEntry(SnmpAgentContext *context,
   const char_t *groupName, const char_t *contextPrefix,
   uint_t securityModel, uint_t securityLevel);

SnmpAccessEntry *snmpSelectAccessEntry(SnmpAgentContext *context,
   const char_t *groupName, const char_t *contextName, size_t contextNameLen,
   SnmpSecurityModel securityModel, SnmpSecurityLevel securityLevel);

SnmpViewEntry *snmpCreateViewEntry(SnmpAgentContext *context);

SnmpViewEntry *snmpFindViewEntry(SnmpAgentContext *context,
   const char_t *viewName, const uint8_t *subtree, size_t subtreeLen);

SnmpViewEntry *snmpSelectViewEntry(SnmpAgentContext *context,
   const char_t *viewName, const uint8_t *oid, size_t oidLen);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
