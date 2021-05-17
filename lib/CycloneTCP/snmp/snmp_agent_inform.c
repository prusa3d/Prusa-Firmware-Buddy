/**
 * @file snmp_agent_inform.c
 * @brief SNMP inform notifications
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
#include "snmp/snmp_agent_inform.h"
#include "mibs/mib2_module.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (SNMP_AGENT_SUPPORT == ENABLED && SNMP_AGENT_INFORM_SUPPORT == ENABLED)


/**
 * @brief Format SNMP InformRequest message
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] version SNMP version identifier
 * @param[in] userName User name or community name
 * @param[in] genericTrapType Generic trap type
 * @param[in] specificTrapCode Specific code
 * @param[in] objectList List of object names
 * @param[in] objectListSize Number of entries in the list
 * @return Error code
 **/

error_t snmpFormatInformRequestMessage(SnmpAgentContext *context,
   SnmpVersion version, const char_t *userName, uint_t genericTrapType,
   uint_t specificTrapCode, const SnmpTrapObject *objectList,
   uint_t objectListSize)
{
   error_t error;

#if (SNMP_V2C_SUPPORT == ENABLED)
   //SNMPv2c version?
   if(version == SNMP_VERSION_2C)
   {
      //Format InformRequest-PDU
      error = snmpFormatInformRequestPdu(context, version, userName,
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

      //Localize the authentication key with the engine ID of the
      //remote SNMP device
      if(context->user.authProtocol != SNMP_AUTH_PROTOCOL_NONE)
      {
         //Key localization algorithm
         error = snmpLocalizeKey(context->user.authProtocol,
            context->informContextEngine, context->informContextEngineLen,
            &context->user.rawAuthKey, &context->user.localizedAuthKey);
         //Any error to report?
         if(error)
            return error;
      }

      //Localize the privacy key with the engine ID of the remote
      //SNMP device
      if(context->user.privProtocol != SNMP_PRIV_PROTOCOL_NONE)
      {
         //Key localization algorithm
         error = snmpLocalizeKey(context->user.authProtocol,
            context->informContextEngine, context->informContextEngineLen,
            &context->user.rawPrivKey, &context->user.localizedPrivKey);
         //Any error to report?
         if(error)
            return error;
      }

      //Format InformRequest-PDU
      error = snmpFormatInformRequestPdu(context, version, userName,
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
 * @brief Format InformRequest-PDU
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] version SNMP version identifier
 * @param[in] userName User name or community name
 * @param[in] genericTrapType Generic trap type
 * @param[in] specificTrapCode Specific code
 * @param[in] objectList List of object names
 * @param[in] objectListSize Number of entries in the list
 * @return Error code
 **/

error_t snmpFormatInformRequestPdu(SnmpAgentContext *context,
   SnmpVersion version, const char_t *userName, uint_t genericTrapType,
   uint_t specificTrapCode, const SnmpTrapObject *objectList,
   uint_t objectListSize)
{
   error_t error;
   SnmpMessage *message;

   //Point to the SNMP message
   message = &context->response;
   //Initialize SNMP message
   snmpInitMessage(message);

   //SNMP version identifier
   message->version = version;

#if (SNMP_V2C_SUPPORT == ENABLED)
   //SNMPv2c version?
   if(version == SNMP_VERSION_2C)
   {
      //Community name
      message->community = userName;
      message->communityLen = osStrlen(userName);
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

      //Save the value of the msgId field
      context->informMsgId = message->msgId;

      //Maximum message size supported by the sender
      message->msgMaxSize = SNMP_MAX_MSG_SIZE;

      //Bit fields which control processing of the message
      if(context->user.authProtocol != SNMP_AUTH_PROTOCOL_NONE)
         message->msgFlags |= SNMP_MSG_FLAG_AUTH;
      if(context->user.privProtocol != SNMP_PRIV_PROTOCOL_NONE)
         message->msgFlags |= SNMP_MSG_FLAG_PRIV;

      //The reportable flag must always be set for acknowledged
      //notification-type PDUs (refer to RFC 3412, section 6.4)
      message->msgFlags |= SNMP_MSG_FLAG_REPORTABLE;

      //Security model used by the sender
      message->msgSecurityModel = SNMP_SECURITY_MODEL_USM;

      //Authoritative engine identifier
      message->msgAuthEngineId = context->informContextEngine;
      message->msgAuthEngineIdLen = context->informContextEngineLen;
      //Number of times the SNMP engine has rebooted
      message->msgAuthEngineBoots = context->informEngineBoots;
      //Number of seconds since last reboot
      message->msgAuthEngineTime = context->informEngineTime;
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
   }
   else
#endif
   //Invalid SNMP version?
   {
      //Report an error
      return ERROR_INVALID_VERSION;
   }

   //Prepare to send an InformRequest-PDU
   message->pduType = SNMP_PDU_INFORM_REQUEST;

   //Increment request identifier
   message->requestId = context->requestId++;
   //Wrap around if necessary
   if(context->requestId < 0)
      context->requestId = 0;

   //Save the value of the request-id field
   context->informRequestId = message->requestId;

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

   //Format PDU header
   error = snmpWritePduHeader(&context->response);
   //Return status code
   return error;
}


/**
 * @brief Format SNMP GetRequest message
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] version SNMP version identifier
 * @return Error code
 **/

error_t snmpFormatGetRequestMessage(SnmpAgentContext *context,
   SnmpVersion version)
{
   error_t error;

   //Clear the security profile
   osMemset(&context->user, 0, sizeof(SnmpUserEntry));

   //Format GetRequest-PDU
   error = snmpFormatGetRequestPdu(context, version);
   //Any error to report?
   if(error)
      return error;

   //Format scopedPDU
   error = snmpWriteScopedPdu(&context->response);
   //Any error to report?
   if(error)
      return error;

   //Format SMNP message header
   error = snmpWriteMessageHeader(&context->response);
   //Any error to report?
   if(error)
      return error;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Format GetRequest-PDU (engine ID discovery procedure)
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] version SNMP version identifier
 * @return Error code
 **/

error_t snmpFormatGetRequestPdu(SnmpAgentContext *context, SnmpVersion version)
{
   error_t error;
   SnmpMessage *message;

   //Point to the SNMP message
   message = &context->response;
   //Initialize SNMP message
   snmpInitMessage(message);

   //SNMP version identifier
   message->version = version;

#if (SNMP_V3_SUPPORT == ENABLED)
   //SNMPv3 version?
   if(version == SNMP_VERSION_3)
   {
      //Increment message identifier
      message->msgId = context->msgId++;
      //Wrap around if necessary
      if(context->msgId < 0)
         context->msgId = 0;

      //Save the value of the msgId field
      context->informMsgId = message->msgId;

      //Maximum message size supported by the sender
      message->msgMaxSize = SNMP_MAX_MSG_SIZE;

      //The reportable flag must always be set for request-type PDUs (refer
      //to RFC 3412, section 6.4)
      message->msgFlags |= SNMP_MSG_FLAG_REPORTABLE;

      //Security model used by the sender
      message->msgSecurityModel = SNMP_SECURITY_MODEL_USM;
   }
#endif

   //Prepare to send an GetRequest-PDU
   message->pduType = SNMP_PDU_GET_REQUEST;

   //Increment request identifier
   message->requestId = context->requestId++;
   //Wrap around if necessary
   if(context->requestId < 0)
      context->requestId = 0;

   //Save the value of the request-id field
   context->informRequestId = message->requestId;

   //Make room for the message header at the beginning of the buffer
   error = snmpComputeMessageOverhead(&context->response);
   //Any error to report?
   if(error)
      return error;

   //Format PDU header
   error = snmpWritePduHeader(&context->response);
   //Return status code
   return error;
}


/**
 * @brief Process GetResponse-PDU
 * @param[in] context Pointer to the SNMP agent context
 * @return Error code
 **/

error_t snmpProcessGetResponsePdu(SnmpAgentContext *context)
{
   SnmpMessage *message;

   //Debug message
   TRACE_INFO("Parsing GetResponse-PDU...\r\n");

   //Point to the incoming SNMP message
   message = &context->request;

   //Total number of SNMP Get-Response PDUs which have been accepted and
   //processed by the SNMP protocol entity
   MIB2_INC_COUNTER32(snmpGroup.snmpInGetResponses, 1);

   //Check the error-status field
   if(message->errorStatus == SNMP_ERROR_NONE)
   {
      //Compare the request-id against the expected identifier
      if(message->requestId == context->informRequestId)
      {
         //The inform request has been acknowledged
         osSetEvent(&context->informEvent);
      }
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Process Report-PDU
 * @param[in] context Pointer to the SNMP agent context
 * @return Error code
 **/

error_t snmpProcessReportPdu(SnmpAgentContext *context)
{
#if (SNMP_V3_SUPPORT == ENABLED)
   SnmpMessage *message;

   //Debug message
   TRACE_INFO("Parsing Report-PDU...\r\n");

   //Point to the incoming SNMP message
   message = &context->request;

   //Sanity check
   if(message->msgAuthEngineIdLen <= SNMP_MAX_CONTEXT_ENGINE_SIZE)
   {
      //Save the authoritative engine identifier
      osMemcpy(context->informContextEngine, message->msgAuthEngineId,
         message->msgAuthEngineIdLen);

      //Save the length of the engine identifier
      context->informContextEngineLen = message->msgAuthEngineIdLen;

      //Save the msgAuthoritativeEngineBoots field
      context->informEngineBoots = message->msgAuthEngineBoots;
      //Save the msgAuthoritativeEngineTime field
      context->informEngineTime = message->msgAuthEngineTime;

      //Check current state
      if(context->informState == SNMP_AGENT_STATE_WAITING_REPORT)
      {
         //Valid context engine identifier?
         if(context->informContextEngineLen > 0)
         {
            //The discovery process is complete
            osSetEvent(&context->informEvent);
         }
      }
   }

   //Successful processing
   return NO_ERROR;
#else
   //SNMPv3 is not supported
   return ERROR_NOT_IMPLEMENTED;
#endif
}

#endif
