/**
 * @file snmp_agent.h
 * @brief SNMP agent (Simple Network Management Protocol)
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

#ifndef _SNMP_AGENT_H
#define _SNMP_AGENT_H

//Forward declaration of SnmpAgentContext structure
struct _SnmpAgentContext;
#define SnmpAgentContext struct _SnmpAgentContext

//Dependencies
#include "core/net.h"
#include "snmp/snmp_common.h"
#include "snmp/snmp_agent_message.h"
#include "snmp/snmp_agent_trap.h"
#include "snmp/snmp_agent_inform.h"
#include "snmp/snmp_agent_usm.h"
#include "snmp/snmp_agent_vacm.h"
#include "mibs/mib_common.h"

//SNMP agent support
#ifndef SNMP_AGENT_SUPPORT
   #define SNMP_AGENT_SUPPORT DISABLED
#elif (SNMP_AGENT_SUPPORT != ENABLED && SNMP_AGENT_SUPPORT != DISABLED)
   #error SNMP_AGENT_SUPPORT parameter is not valid
#endif

//Stack size required to run the SNMP agent
#ifndef SNMP_AGENT_STACK_SIZE
   #define SNMP_AGENT_STACK_SIZE 550
#elif (SNMP_AGENT_STACK_SIZE < 1)
   #error SNMP_AGENT_STACK_SIZE parameter is not valid
#endif

//Priority at which the SNMP agent should run
#ifndef SNMP_AGENT_PRIORITY
   #define SNMP_AGENT_PRIORITY OS_TASK_PRIORITY_NORMAL
#endif

//Maximum number of MIBs
#ifndef SNMP_AGENT_MAX_MIBS
   #define SNMP_AGENT_MAX_MIBS 8
#elif (SNMP_AGENT_MAX_MIBS < 1)
   #error SNMP_AGENT_MAX_MIBS parameter is not valid
#endif

//Maximum number of community strings
#ifndef SNMP_AGENT_MAX_COMMUNITIES
   #define SNMP_AGENT_MAX_COMMUNITIES 3
#elif (SNMP_AGENT_MAX_COMMUNITIES < 1)
   #error SNMP_AGENT_MAX_COMMUNITIES parameter is not valid
#endif

//Maximum number of users
#ifndef SNMP_AGENT_MAX_USERS
   #define SNMP_AGENT_MAX_USERS 8
#elif (SNMP_AGENT_MAX_USERS < 1)
   #error SNMP_AGENT_MAX_USERS parameter is not valid
#endif

//Size of the group table
#ifndef SNMP_AGENT_GROUP_TABLE_SIZE
   #define SNMP_AGENT_GROUP_TABLE_SIZE 8
#elif (SNMP_AGENT_GROUP_TABLE_SIZE < 1)
   #error SNMP_AGENT_GROUP_TABLE_SIZE parameter is not valid
#endif

//Size of the access table
#ifndef SNMP_AGENT_ACCESS_TABLE_SIZE
   #define SNMP_AGENT_ACCESS_TABLE_SIZE 8
#elif (SNMP_AGENT_ACCESS_TABLE_SIZE < 1)
   #error SNMP_AGENT_ACCESS_TABLE_SIZE parameter is not valid
#endif

//Size of the view table
#ifndef SNMP_AGENT_VIEW_TABLE_SIZE
   #define SNMP_AGENT_VIEW_TABLE_SIZE 8
#elif (SNMP_AGENT_VIEW_TABLE_SIZE < 1)
   #error SNMP_AGENT_VIEW_TABLE_SIZE parameter is not valid
#endif

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Random data generation callback function
 **/

typedef error_t (*SnmpAgentRandCallback)(uint8_t *data, size_t length);


/**
 * @brief SNMP agent settings
 **/

typedef struct
{
   NetInterface *interface;                        ///<Network interface to configure
   SnmpVersion versionMin;                         ///<Minimum version accepted by the SNMP agent
   SnmpVersion versionMax;                         ///<Maximum version accepted by the SNMP agent
   uint16_t port;                                  ///<SNMP port number
   uint16_t trapPort;                              ///<SNMP trap port number
   SnmpAgentRandCallback randCallback;             ///<Random data generation callback function
} SnmpAgentSettings;


/**
 * @brief SNMP agent context
 **/

struct _SnmpAgentContext
{
   SnmpAgentSettings settings;                                ///<SNMP agent settings
   OsMutex mutex;                                             ///<Mutex preventing simultaneous access to SNMP agent context
   uint8_t enterpriseOid[SNMP_MAX_OID_SIZE];                  ///<Enterprise OID
   size_t enterpriseOidLen;                                   ///<Length of the enterprise OID
   const MibModule *mibTable[SNMP_AGENT_MAX_MIBS];            ///<MIB modules
#if (SNMP_V1_SUPPORT == ENABLED || SNMP_V2C_SUPPORT == ENABLED)
   SnmpUserEntry communityTable[SNMP_AGENT_MAX_COMMUNITIES];  ///<Community strings
#endif
#if (SNMP_V3_SUPPORT == ENABLED)
   SnmpUserEntry userTable[SNMP_AGENT_MAX_USERS];             ///<List of users
#endif
#if (SNMP_AGENT_VACM_SUPPORT == ENABLED)
   SnmpGroupEntry groupTable[SNMP_AGENT_GROUP_TABLE_SIZE];    ///<List of groups
   SnmpAccessEntry accessTable[SNMP_AGENT_ACCESS_TABLE_SIZE]; ///<Access rights for groups
   SnmpViewEntry viewTable[SNMP_AGENT_VIEW_TABLE_SIZE];       ///<Families of subtrees within MIB views
#endif
   Socket *socket;                                            ///<Underlying socket
   IpAddr remoteIpAddr;                                       ///<IP address of the remote SNMP engine
   uint16_t remotePort;                                       ///<Source port used by the remote SNMP engine
   int32_t requestId;                                         ///<Request identifier
   SnmpMessage request;                                       ///<SNMP request message
   SnmpMessage response;                                      ///<SNMP response message
   SnmpUserEntry user;                                        ///<Security profile of current user
#if (SNMP_V3_SUPPORT == ENABLED)
   uint8_t contextEngine[SNMP_MAX_CONTEXT_ENGINE_SIZE];       ///<Context engine identifier
   size_t contextEngineLen;                                   ///<Length of the context engine identifier
   char_t contextName[SNMP_MAX_CONTEXT_NAME_LEN + 1];         ///<Context name
   systime_t systemTime;                                      ///<System time
   int32_t engineBoots;                                       ///<Number of times that the SNMP engine has rebooted
   int32_t engineTime;                                        ///<SNMP engine time
   int32_t msgId;                                             ///<Message identifier
   uint64_t salt;                                             ///<Integer initialized to a random value at boot time
   uint8_t privParameters[8];                                 ///<Privacy parameters
#endif
#if (SNMP_AGENT_INFORM_SUPPORT == ENABLED)
   SnmpAgentState informState;                                ///<State of the inform sending process
   int32_t informRequestId;                                   ///<Inform request identifier
   systime_t informTimestamp;                                 ///<Timestamp to manage retransmissions
   uint_t informRetransmitCount;                              ///<Retransmission counter
   OsEvent informEvent;                                       ///<Event object
#if (SNMP_V3_SUPPORT == ENABLED)
   uint8_t informContextEngine[SNMP_MAX_CONTEXT_ENGINE_SIZE]; ///<Context engine identifier of the remote application
   size_t informContextEngineLen;                             ///<Length of the context engine identifier
   int32_t informEngineBoots;                                 ///<Number of times that the remote SNMP engine has rebooted
   int32_t informEngineTime;                                  ///<SNMP engine time of the remote application
   int32_t informMsgId;                                       ///<Message identifier
#endif
#endif
};


//SNMP agent related functions
void snmpAgentGetDefaultSettings(SnmpAgentSettings *settings);
error_t snmpAgentInit(SnmpAgentContext *context, const SnmpAgentSettings *settings);
error_t snmpAgentStart(SnmpAgentContext *context);

error_t snmpAgentLoadMib(SnmpAgentContext *context, const MibModule *module);
error_t snmpAgentUnloadMib(SnmpAgentContext *context, const MibModule *module);

error_t snmpAgentSetVersion(SnmpAgentContext *context,
   SnmpVersion versionMin, SnmpVersion versionMax);

error_t snmpAgentSetEngineBoots(SnmpAgentContext *context, int32_t engineBoots);
error_t snmpAgentGetEngineBoots(SnmpAgentContext *context, int32_t *engineBoots);

error_t snmpAgentSetEnterpriseOid(SnmpAgentContext *context,
   const uint8_t *enterpriseOid, size_t enterpriseOidLen);

error_t snmpAgentSetContextEngine(SnmpAgentContext *context,
   const void *contextEngine, size_t contextEngineLen);

error_t snmpAgentSetContextName(SnmpAgentContext *context,
   const char_t *contextName);

error_t snmpAgentCreateCommunity(SnmpAgentContext *context,
   const char_t *community, SnmpAccess mode);

error_t snmpAgentDeleteCommunity(SnmpAgentContext *context,
   const char_t *community);

error_t snmpAgentCreateUser(SnmpAgentContext *context,
   const char_t *userName, SnmpAccess mode, SnmpKeyFormat keyFormat,
   SnmpAuthProtocol authProtocol, const void *authKey,
   SnmpPrivProtocol privProtocol, const void *privKey);

error_t snmpAgentDeleteUser(SnmpAgentContext *context, const char_t *userName);

error_t snmpAgentJoinGroup(SnmpAgentContext *context, const char_t *userName,
   SnmpSecurityModel securityModel, const char_t *groupName);

error_t snmpAgentLeaveGroup(SnmpAgentContext *context,
   const char_t *userName, SnmpSecurityModel securityModel);

error_t snmpAgentCreateAccess(SnmpAgentContext *context,
   const char_t *groupName, SnmpSecurityModel securityModel,
   SnmpSecurityLevel securityLevel, const char_t *contextPrefix,
   SnmpContextMatch contextMatch, const char_t *readViewName,
   const char_t *writeViewName, const char_t *notifyViewName);

error_t snmpAgentDeleteAccess(SnmpAgentContext *context,
   const char_t *groupName, SnmpSecurityModel securityModel,
   SnmpSecurityLevel securityLevel, const char_t *contextPrefix);

error_t snmpAgentCreateView(SnmpAgentContext *context,
   const char_t *viewName, const uint8_t *subtree, size_t subtreeLen,
   const uint8_t *mask, size_t maskLen, SnmpViewType type);

error_t snmpAgentDeleteView(SnmpAgentContext *context,
   const char_t *viewName, const uint8_t *subtree, size_t subtreeLen);

error_t snmpAgentSendTrap(SnmpAgentContext *context, const IpAddr *destIpAddr,
   SnmpVersion version, const char_t *userName, uint_t genericTrapType,
   uint_t specificTrapCode, const SnmpTrapObject *objectList, uint_t objectListSize);

error_t snmpAgentSendInform(SnmpAgentContext *context, const IpAddr *destIpAddr,
   SnmpVersion version, const char_t *userName, uint_t genericTrapType,
   uint_t specificTrapCode, const SnmpTrapObject *objectList, uint_t objectListSize);

void snmpAgentTask(SnmpAgentContext *context);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
