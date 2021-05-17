/**
 * @file snmp_message.h
 * @brief SNMP message formatting and parsing
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

#ifndef _SNMP_AGENT_MESSAGE_H
#define _SNMP_AGENT_MESSAGE_H

//Dependencies
#include "core/net.h"
#include "snmp/snmp_agent.h"

//SNMPv1 message header overhead
#define SNMP_V1_MSG_HEADER_OVERHEAD 48
//SNMPv2c message header overhead
#define SNMP_V2C_MSG_HEADER_OVERHEAD 37
//SNMPv3 message header overhead
#define SNMP_V3_MSG_HEADER_OVERHEAD 105

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief SNMP message
 **/

typedef struct
{
   uint8_t buffer[SNMP_MAX_MSG_SIZE]; ///<Buffer that holds the message
   size_t bufferLen;                  ///<Original length of the message
   uint8_t *pos;                      ///<Current position
   size_t length;                     ///<Length of the message
   int32_t version;                   ///<SNMP version identifier
#if (SNMP_V1_SUPPORT == ENABLED || SNMP_V2C_SUPPORT == ENABLED)
   const char_t *community;           ///<Community name
   size_t communityLen;               ///<Length of the community name
#endif
#if (SNMP_V3_SUPPORT == ENABLED)
   int32_t msgId;                     ///<Message identifier
   int32_t msgMaxSize;                ///<Maximum message size supported by the sender
   uint8_t msgFlags;                  ///<Bit fields which control processing of the message
   int32_t msgSecurityModel;          ///<Security model used by the sender
   const uint8_t *msgAuthEngineId;    ///<Authoritative engine identifier
   size_t msgAuthEngineIdLen;         ///<Length of the authoritative engine identifier
   int32_t msgAuthEngineBoots;        ///<Number of times the SNMP engine has rebooted
   int32_t msgAuthEngineTime;         ///<Number of seconds since last reboot
   const char_t *msgUserName;         ///<User name
   size_t msgUserNameLen;             ///<Length of the user name
   uint8_t *msgAuthParameters;        ///<Authentication parameters
   size_t msgAuthParametersLen;       ///<Length of the authentication parameters
   const uint8_t *msgPrivParameters;  ///<Privacy parameters
   size_t msgPrivParametersLen;       ///<Length of the privacy parameters
   const uint8_t *contextEngineId;    ///<Context engine identifier
   size_t contextEngineIdLen;         ///<Length of the context engine identifier
   const char_t *contextName;         ///<Context name
   size_t contextNameLen;             ///<Length of the context name
#endif
   SnmpPduType pduType;               ///<PDU type
   int32_t requestId;                 ///<Request identifier
   int32_t errorStatus;               ///<Error status
   int32_t errorIndex;                ///<Error index
#if (SNMP_V1_SUPPORT == ENABLED)
   const uint8_t *enterpriseOid;      ///<Type of object generating trap
   size_t enterpriseOidLen;           ///<Length of the enterprise OID
   Ipv4Addr agentAddr;                ///<Address of object generating trap
   int32_t genericTrapType;           ///<Generic trap type
   int32_t specificTrapCode;          ///<Specific trap code
   uint32_t timestamp;                ///<Timestamp
#endif
#if (SNMP_V2C_SUPPORT == ENABLED || SNMP_V3_SUPPORT == ENABLED)
   int32_t nonRepeaters;              ///<GetBulkRequest-PDU specific parameter
   int32_t maxRepetitions;            ///<GetBulkRequest-PDU specific parameter
#endif
   uint8_t *varBindList;              ///<List of variable bindings
   size_t varBindListLen;             ///<Length of the list in bytes
   size_t varBindListMaxLen;          ///<Maximum length of the list in bytes
   size_t oidLen;                     ///<Length of the object identifier
} SnmpMessage;


/**
 * @brief Variable binding
 **/

typedef struct
{
   const uint8_t *oid;
   size_t oidLen;
   uint_t objClass;
   uint_t objType;
   const uint8_t *value;
   size_t valueLen;
} SnmpVarBind;


//SNMP related functions
void snmpInitMessage(SnmpMessage *message);
error_t snmpInitResponse(SnmpAgentContext *context);

error_t snmpComputeMessageOverhead(SnmpMessage *message);

error_t snmpParseMessageHeader(SnmpMessage *message);
error_t snmpWriteMessageHeader(SnmpMessage *message);

error_t snmpParseCommunity(SnmpMessage *message);
error_t snmpWriteCommunity(SnmpMessage *message);

error_t snmpParseGlobalData(SnmpMessage *message);
error_t snmpWriteGlobalData(SnmpMessage *message);

error_t snmpParseSecurityParameters(SnmpMessage *message);
error_t snmpWriteSecurityParameters(SnmpMessage *message);

error_t snmpParseScopedPdu(SnmpMessage *message);
error_t snmpWriteScopedPdu(SnmpMessage *message);

error_t snmpParsePduHeader(SnmpMessage *message);
error_t snmpWritePduHeader(SnmpMessage *message);

error_t snmpEncodeInt32(int32_t value, uint8_t *dest, size_t *length);
error_t snmpEncodeUnsignedInt32(uint32_t value, uint8_t *dest, size_t *length);
error_t snmpEncodeUnsignedInt64(uint64_t value, uint8_t *dest, size_t *length);

error_t snmpDecodeInt32(const uint8_t *src, size_t length, int32_t *value);
error_t snmpDecodeUnsignedInt32(const uint8_t *src, size_t length, uint32_t *value);
error_t snmpDecodeUnsignedInt64(const uint8_t *src, size_t length, uint64_t *value);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
