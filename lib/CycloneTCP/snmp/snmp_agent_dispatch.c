/**
 * @file snmp_agent_dispatch.c
 * @brief SNMP message dispatching
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
#include "snmp/snmp_agent_dispatch.h"
#include "snmp/snmp_agent_pdu.h"
#include "snmp/snmp_agent_misc.h"
#include "mibs/mib2_module.h"
#include "mibs/snmp_mib_module.h"
#include "core/crypto.h"
#include "encoding/asn1.h"
#include "encoding/oid.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (SNMP_AGENT_SUPPORT == ENABLED)


/**
 * @brief Process incoming SNMP message
 * @param[in] context Pointer to the SNMP agent context
 * @return Error code
 **/

error_t snmpProcessMessage(SnmpAgentContext *context)
{
   error_t error;

   //Total number of messages delivered to the SNMP entity from the
   //transport service
   MIB2_INC_COUNTER32(snmpGroup.snmpInPkts, 1);
   SNMP_MIB_INC_COUNTER32(snmpGroup.snmpInPkts, 1);

#if (SNMP_V3_SUPPORT == ENABLED)
   //Refresh SNMP engine time
   snmpRefreshEngineTime(context);
#endif

   //Message parsing initialization
   snmpInitMessage(&context->request);

   //Parse SNMP message header
   error = snmpParseMessageHeader(&context->request);
   //Any error to report?
   if(error)
      return error;

   //The SNMP agent verifies the version number. If there is a mismatch,
   //it discards the datagram and performs no further actions
   if(context->request.version < context->settings.versionMin ||
      context->request.version > context->settings.versionMax)
   {
      //Debug message
      TRACE_WARNING("  Invalid SNMP version!\r\n");
      //Discard incoming SNMP message
      return ERROR_INVALID_VERSION;
   }

#if (SNMP_V1_SUPPORT == ENABLED)
   //SNMPv1 version?
   if(context->request.version == SNMP_VERSION_1)
   {
      //Process incoming SNMPv1 message
      error = snmpv1ProcessMessage(context);
   }
   else
#endif
#if (SNMP_V2C_SUPPORT == ENABLED)
   //SNMPv2c version?
   if(context->request.version == SNMP_VERSION_2C)
   {
      //Process incoming SNMPv2c message
      error = snmpv2cProcessMessage(context);
   }
   else
#endif
#if (SNMP_V3_SUPPORT == ENABLED)
   //SNMPv3 version?
   if(context->request.version == SNMP_VERSION_3)
   {
      //Process incoming SNMPv3 message
      error = snmpv3ProcessMessage(context);
   }
   else
#endif
   //Invalid SNMP version?
   {
      //Debug message
      TRACE_WARNING("  Invalid SNMP version!\r\n");

      //Total number of SNMP messages which were delivered to the SNMP
      //protocol entity and were for an unsupported SNMP version
      MIB2_INC_COUNTER32(snmpGroup.snmpInBadVersions, 1);
      SNMP_MIB_INC_COUNTER32(snmpGroup.snmpInBadVersions, 1);

      //Discard incoming SNMP message
      error = ERROR_INVALID_VERSION;
   }

   //Check status code
   if(error == NO_ERROR)
   {
      //Total number of messages which were passed from the SNMP protocol
      //entity to the transport service
      MIB2_INC_COUNTER32(snmpGroup.snmpOutPkts, 1);
   }
   else if(error == ERROR_INVALID_TAG)
   {
      //Total number of ASN.1 or BER errors encountered by the SNMP protocol
      //entity when decoding received SNMP messages
      MIB2_INC_COUNTER32(snmpGroup.snmpInASNParseErrs, 1);
      SNMP_MIB_INC_COUNTER32(snmpGroup.snmpInASNParseErrs, 1);
   }
   else if(error == ERROR_BUFFER_OVERFLOW)
   {
      //Total number of PDUs delivered to the SNMP entity which were silently
      //dropped because the size of the reply was greater than the maximum
      //message size
      SNMP_MIB_INC_COUNTER32(snmpGroup.snmpSilentDrops, 1);
   }

   //Return status code
   return error;
}


/**
 * @brief Process incoming SNMPv1 message
 * @param[in] context Pointer to the SNMP agent context
 * @return Error code
 **/

error_t snmpv1ProcessMessage(SnmpAgentContext *context)
{
#if (SNMP_V1_SUPPORT == ENABLED)
   error_t error;
   SnmpUserEntry *community;

   //Parse community name
   error = snmpParseCommunity(&context->request);
   //Any error to report?
   if(error)
      return error;

   //Information about the community name is extracted from the local
   //configuration datastore
   community = snmpFindCommunityEntry(context, context->request.community,
      context->request.communityLen);

   //Invalid community name?
   if(community == NULL || community->status != MIB_ROW_STATUS_ACTIVE)
   {
      //Debug message
      TRACE_WARNING("  Invalid community name!\r\n");

      //Total number of SNMP messages delivered to the SNMP protocol entity
      //which used a SNMP community name not known to said entity
      MIB2_INC_COUNTER32(snmpGroup.snmpInBadCommunityNames, 1);
      SNMP_MIB_INC_COUNTER32(snmpGroup.snmpInBadCommunityNames, 1);

      //Report an error
      return ERROR_UNKNOWN_USER_NAME;
   }

   //Save the security profile associated with the current community
   context->user = *community;

   //Process PDU
   error = snmpProcessPdu(context);
   //Any error to report?
   if(error)
      return error;

   //Any response?
   if(context->response.length > 0)
   {
      //Format SNMP message header
      error = snmpWriteMessageHeader(&context->response);
   }

   //Return status code
   return error;
#else
   //Report an error
   return ERROR_INVALID_VERSION;
#endif
}


/**
 * @brief Process incoming SNMPv2c message
 * @param[in] context Pointer to the SNMP agent context
 * @return Error code
 **/

error_t snmpv2cProcessMessage(SnmpAgentContext *context)
{
#if (SNMP_V2C_SUPPORT == ENABLED)
   error_t error;
   SnmpUserEntry *community;

   //Parse community name
   error = snmpParseCommunity(&context->request);
   //Any error to report?
   if(error)
      return error;

   //Information about the community name is extracted from the local
   //configuration datastore
   community = snmpFindCommunityEntry(context, context->request.community,
      context->request.communityLen);

   //Invalid community name?
   if(community == NULL || community->status != MIB_ROW_STATUS_ACTIVE)
   {
      //Debug message
      TRACE_WARNING("  Invalid community name!\r\n");

      //Total number of SNMP messages delivered to the SNMP protocol entity
      //which used a SNMP community name not known to said entity
      MIB2_INC_COUNTER32(snmpGroup.snmpInBadCommunityNames, 1);
      SNMP_MIB_INC_COUNTER32(snmpGroup.snmpInBadCommunityNames, 1);

      //Report an error
      return ERROR_UNKNOWN_USER_NAME;
   }

   //Save the security profile associated with the current community
   context->user = *community;

   //Process PDU
   error = snmpProcessPdu(context);
   //Any error to report?
   if(error)
      return error;

   //Any response?
   if(context->response.length > 0)
   {
      //Format SNMP message header
      error = snmpWriteMessageHeader(&context->response);
   }

   //Return status code
   return error;
#else
   //Report an error
   return ERROR_INVALID_VERSION;
#endif
}


/**
 * @brief Process incoming SNMPv3 message
 * @param[in] context Pointer to the SNMP agent context
 * @return Error code
 **/

error_t snmpv3ProcessMessage(SnmpAgentContext *context)
{
#if (SNMP_V3_SUPPORT == ENABLED)
   error_t error;
   SnmpUserEntry *user;

   //Parse msgGlobalData field
   error = snmpParseGlobalData(&context->request);
   //Any error to report?
   if(error)
      return error;

   //Parse msgSecurityParameters field
   error = snmpParseSecurityParameters(&context->request);
   //Any error to report?
   if(error)
      return error;

   //Start of exception handling block
   do
   {
#if (SNMP_AGENT_INFORM_SUPPORT == ENABLED)
      if(context->request.msgUserNameLen == 0 && context->request.msgFlags == 0)
      {
         //Clear the security profile
         osMemset(&context->user, 0, sizeof(SnmpUserEntry));
      }
      else if(context->informContextEngineLen > 0 &&
         !oidComp(context->request.msgAuthEngineId, context->request.msgAuthEngineIdLen,
         context->informContextEngine, context->informContextEngineLen))
      {
         //Information about the value of the msgUserName field is extracted
         //from the local configuration datastore
         user = snmpFindUserEntry(context, context->request.msgUserName,
            context->request.msgUserNameLen);

         //Check security parameters
         error = snmpCheckSecurityParameters(user, &context->request,
            context->informContextEngine, context->informContextEngineLen);
         //Invalid security parameters?
         if(error)
            break;

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
               break;
         }

         //Localize the privacy key with the engine ID of the remote
         //SNMP device
         if(context->user.privProtocol != SNMP_AUTH_PROTOCOL_NONE)
         {
            //Key localization algorithm
            error = snmpLocalizeKey(context->user.authProtocol,
               context->informContextEngine, context->informContextEngineLen,
               &context->user.rawPrivKey, &context->user.localizedPrivKey);
            //Any error to report?
            if(error)
               break;
         }
      }
      else
#endif
      {
         //Information about the value of the msgUserName field is extracted
         //from the local configuration datastore
         user = snmpFindUserEntry(context, context->request.msgUserName,
            context->request.msgUserNameLen);

         //Check security parameters
         error = snmpCheckSecurityParameters(user, &context->request,
            context->contextEngine, context->contextEngineLen);
         //Invalid security parameters?
         if(error)
            break;

         //Save the security profile associated with the current user
         context->user = *user;
      }

      //Check whether the authFlag is set
      if((context->request.msgFlags & SNMP_MSG_FLAG_AUTH) != 0)
      {
         //Authenticate incoming SNMP message
         error = snmpAuthIncomingMessage(&context->user, &context->request);
         //Data authentication failed?
         if(error)
            break;

         //Replay protection
         error = snmpCheckEngineTime(context, &context->request);
         //Message outside of the time window?
         if(error)
            break;
      }

      //Check whether the privFlag is set
      if((context->request.msgFlags & SNMP_MSG_FLAG_PRIV) != 0)
      {
         //Decrypt data
         error = snmpDecryptData(&context->user, &context->request);
         //Data decryption failed?
         if(error)
            break;
      }

      //Parse scopedPDU
      error = snmpParseScopedPdu(&context->request);
      //Any error to report?
      if(error)
         break;

      //Process PDU
      error = snmpProcessPdu(context);
      //Any error to report?
      if(error)
        break;

      //End of exception handling block
   } while(0);

   //Check error indication
   if(error == ERROR_UNSUPPORTED_SECURITY_LEVEL ||
      error == ERROR_NOT_IN_TIME_WINDOW ||
      error == ERROR_UNKNOWN_USER_NAME ||
      error == ERROR_UNKNOWN_ENGINE_ID ||
      error == ERROR_AUTHENTICATION_FAILED ||
      error == ERROR_DECRYPTION_FAILED ||
      error == ERROR_UNAVAILABLE_CONTEXT ||
      error == ERROR_UNKNOWN_CONTEXT)
   {
      //When the reportable flag is used, if its value is one, a Report-PDU
      //must be returned to the sender
      if((context->request.msgFlags & SNMP_MSG_FLAG_REPORTABLE) != 0)
         error = snmpFormatReportPdu(context, error);

      //Any error to report?
      if(error)
         return error;
   }
   else if(error == NO_ERROR)
   {
      //Continue processing
   }
   else
   {
      //Stop processing
      return error;
   }

   //Any response?
   if(context->response.length > 0)
   {
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

      //Format SNMP message header
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

   //Successful processing
   return NO_ERROR;
#else
   //Report an error
   return ERROR_INVALID_VERSION;
#endif
}

#endif
