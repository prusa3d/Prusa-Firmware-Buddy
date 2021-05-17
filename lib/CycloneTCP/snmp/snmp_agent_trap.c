/**
 * @file snmp_agent_trap.c
 * @brief SNMP trap notifications
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

//Switch to the appropriate trace level
#define TRACE_LEVEL SNMP_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "snmp/snmp_agent.h"
#include "snmp/snmp_agent_misc.h"
#include "snmp/snmp_agent_trap.h"
#include "mibs/mib2_module.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (SNMP_AGENT_SUPPORT == ENABLED && SNMP_AGENT_TRAP_SUPPORT == ENABLED)


/**
 * @brief Format SNMP Trap message
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] version SNMP version identifier
 * @param[in] userName User name or community name
 * @param[in] genericTrapType Generic trap type
 * @param[in] specificTrapCode Specific code
 * @param[in] objectList List of object names
 * @param[in] objectListSize Number of entries in the list
 * @return Error code
 **/

error_t snmpFormatTrapMessage(SnmpAgentContext *context, SnmpVersion version,
   const char_t *userName, uint_t genericTrapType, uint_t specificTrapCode,
   const SnmpTrapObject *objectList, uint_t objectListSize)
{
   error_t error;

#if (SNMP_V1_SUPPORT == ENABLED)
   //SNMPv1 version?
   if(version == SNMP_VERSION_1)
   {
      //Format Trap-PDU
      error = snmpFormatTrapPdu(context, version, userName,
         genericTrapType, specificTrapCode, objectList, objectListSize);
      //Any error to report?
      if(error)
         return error;

      //Format SMNP message header
      error = snmpWriteMessageHeader(&context->response);
      //Any error to report?
      if(error)
         return error;
   }
   else
#endif
#if (SNMP_V2C_SUPPORT == ENABLED)
   //SNMPv2c version?
   if(version == SNMP_VERSION_2C)
   {
      //Format SNMPv2-Trap-PDU
      error = snmpFormatTrapPdu(context, version, userName,
         genericTrapType, specificTrapCode, objectList, objectListSize);
      //Any error to report?
      if(error)
         return error;

      //Format SMNP message header
      error = snmpWriteMessageHeader(&context->response);
      //Any error to report?
      if(error)
         return error;
   }
   else
#endif
#if (SNMP_V3_SUPPORT == ENABLED)
   //SNMPv3 version?
   if(version == SNMP_VERSION_3)
   {
      SnmpUserEntry *user;

      //Information about the user name is extracted from the local
      //configuration datastore
      user = snmpFindUserEntry(context, userName, osStrlen(userName));

      //Invalid user name?
      if(user == NULL || user->status != MIB_ROW_STATUS_ACTIVE)
         return ERROR_UNKNOWN_USER_NAME;

      //Save the security profile associated with the current user
      context->user = *user;

      //Format SNMPv2-Trap-PDU
      error = snmpFormatTrapPdu(context, version, userName,
         genericTrapType, specificTrapCode, objectList, objectListSize);
      //Any error to report?
      if(error)
         return error;

      //Format scopedPDU
      error = snmpWriteScopedPdu(&context->response);
      //Any error to report?
      if(error)
         return error;

      //Check whether the privFlag is set
      if((context->response.msgFlags & SNMP_MSG_FLAG_PRIV) != 0)
      {
         //Encrypt data
         error = snmpEncryptData(&context->user, &context->response,
            &context->salt);
         //Any error to report?
         if(error)
            return error;
      }

      //Format SMNP message header
      error = snmpWriteMessageHeader(&context->response);
      //Any error to report?
      if(error)
         return error;

      //Check whether the authFlag is set
      if((context->response.msgFlags & SNMP_MSG_FLAG_AUTH) != 0)
      {
         //Authenticate outgoing SNMP message
         error = snmpAuthOutgoingMessage(&context->user, &context->response);
         //Any error to report?
         if(error)
            return error;
      }
   }
   else
#endif
   //Invalid SNMP version?
   {
      //Debug message
      TRACE_WARNING("  Invalid SNMP version!\r\n");
      //Report an error
      return ERROR_INVALID_VERSION;
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Format Trap-PDU or SNMPv2-Trap-PDU
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] version SNMP version identifier
 * @param[in] userName User name or community name
 * @param[in] genericTrapType Generic trap type
 * @param[in] specificTrapCode Specific code
 * @param[in] objectList List of object names
 * @param[in] objectListSize Number of entries in the list
 * @return Error code
 **/

error_t snmpFormatTrapPdu(SnmpAgentContext *context, SnmpVersion version,
   const char_t *userName, uint_t genericTrapType, uint_t specificTrapCode,
   const SnmpTrapObject *objectList, uint_t objectListSize)
{
   error_t error;
   SnmpMessage *message;

   //Point to the SNMP message
   message = &context->response;
   //Initialize SNMP message
   snmpInitMessage(message);

   //SNMP version identifier
   message->version = version;

#if (SNMP_V1_SUPPORT == ENABLED)
   //SNMPv1 version?
   if(version == SNMP_VERSION_1)
   {
#if (IPV4_SUPPORT == ENABLED)
      NetInterface *interface;

      //Point to the underlying network interface
      interface = context->settings.interface;
#endif

      //Community name
      message->community = userName;
      message->communityLen = osStrlen(userName);

      //Prepare to send a Trap-PDU
      message->pduType = SNMP_PDU_TRAP;
      //Type of object generating trap
      message->enterpriseOid = context->enterpriseOid;
      message->enterpriseOidLen = context->enterpriseOidLen;

#if (IPV4_SUPPORT == ENABLED)
      //Address of object generating trap
      if(interface != NULL)
         message->agentAddr = interface->ipv4Context.addrList[0].addr;
#endif

      //Generic trap type
      message->genericTrapType = genericTrapType;
      //Specific trap code
      message->specificTrapCode = specificTrapCode;
      //Timestamp
      message->timestamp = osGetSystemTime() / 10;
   }
   else
#endif
#if (SNMP_V2C_SUPPORT == ENABLED)
   //SNMPv2c version?
   if(version == SNMP_VERSION_2C)
   {
      //Community name
      message->community = userName;
      message->communityLen = osStrlen(userName);

      //Prepare to send a SNMPv2-Trap-PDU
      message->pduType = SNMP_PDU_TRAP_V2;
   }
   else
#endif
#if (SNMP_V3_SUPPORT == ENABLED)
   //SNMPv3 version?
   if(version == SNMP_VERSION_3)
   {
      //Increment message identifier
      message->msgId = context->msgId++;
      //Wrap around if necessary
      if(context->msgId < 0)
         context->msgId = 0;

      //Maximum message size supported by the sender
      message->msgMaxSize = SNMP_MAX_MSG_SIZE;

      //Bit fields which control processing of the message
      if(context->user.authProtocol != SNMP_AUTH_PROTOCOL_NONE)
         message->msgFlags |= SNMP_MSG_FLAG_AUTH;
      if(context->user.privProtocol != SNMP_PRIV_PROTOCOL_NONE)
         message->msgFlags |= SNMP_MSG_FLAG_PRIV;

      //Security model used by the sender
      message->msgSecurityModel = SNMP_SECURITY_MODEL_USM;

      //Authoritative engine identifier
      message->msgAuthEngineId = context->contextEngine;
      message->msgAuthEngineIdLen = context->contextEngineLen;
      //Number of times the SNMP engine has rebooted
      message->msgAuthEngineBoots = context->engineBoots;
      //Number of seconds since last reboot
      message->msgAuthEngineTime = context->engineTime;
      //User name
      message->msgUserName = userName;
      message->msgUserNameLen = osStrlen(userName);
      //Authentication parameters
      message->msgAuthParameters = NULL;
      //Length of the authentication parameters
      message->msgAuthParametersLen = snmpGetMacLength(context->user.authProtocol);
      //Privacy parameters
      message->msgPrivParameters = context->privParameters;

      //Length of the privacy parameters
      if(context->user.privProtocol == SNMP_PRIV_PROTOCOL_DES)
         message->msgPrivParametersLen = 8;
      else if(context->user.privProtocol == SNMP_PRIV_PROTOCOL_AES)
         message->msgPrivParametersLen = 8;
      else
         message->msgPrivParametersLen = 0;

      //Context engine identifier
      message->contextEngineId = context->contextEngine;
      message->contextEngineIdLen = context->contextEngineLen;
      //Context name
      message->contextName = context->contextName;
      message->contextNameLen = osStrlen(context->contextName);

      //Prepare to send a SNMPv2-Trap-PDU
      message->pduType = SNMP_PDU_TRAP_V2;
   }
   else
#endif
   //Invalid SNMP version?
   {
      //Report an error
      return ERROR_INVALID_VERSION;
   }

   //Increment request identifier
   message->requestId = context->requestId++;
   //Wrap around if necessary
   if(context->requestId < 0)
      context->requestId = 0;

   //Make room for the message header at the beginning of the buffer
   error = snmpComputeMessageOverhead(&context->response);
   //Any error to report?
   if(error)
      return error;

   //Format the list of variable bindings
   error = snmpWriteTrapVarBindingList(context, genericTrapType,
      specificTrapCode, objectList, objectListSize);
   //Any error to report?
   if(error)
      return error;

   //Total number of SNMP Trap PDUs which have been generated by
   //the SNMP protocol entity
   MIB2_INC_COUNTER32(snmpGroup.snmpOutTraps, 1);

   //Format PDU header
   error = snmpWritePduHeader(&context->response);
   //Return status code
   return error;
}

#endif
