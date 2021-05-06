/**
 * @file snmp_message.c
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

//Switch to the appropriate trace level
#define TRACE_LEVEL SNMP_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "snmp/snmp_agent.h"
#include "snmp/snmp_agent_message.h"
#include "mibs/snmp_mpd_mib_module.h"
#include "core/crypto.h"
#include "encoding/asn1.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (SNMP_AGENT_SUPPORT == ENABLED)


/**
 * @brief Initialize a SNMP message
 * @param[in] message Pointer to the SNMP message
 **/

void snmpInitMessage(SnmpMessage *message)
{
   //Current position in the message
   message->pos = NULL;
   //Length of the message
   message->length = 0;

   //SNMP version identifier
   message->version = SNMP_VERSION_1;

#if (SNMP_V1_SUPPORT == ENABLED || SNMP_V2C_SUPPORT == ENABLED)
   //Initialize community name
   message->community = NULL;
   message->communityLen = 0;
#endif

#if (SNMP_V3_SUPPORT == ENABLED)
   //Initialize msgGlobalData fields
   message->msgId = 0;
   message->msgMaxSize = 0;
   message->msgFlags = 0;
   message->msgSecurityModel = 0;

   //Initialize msgSecurityParameters fields
   message->msgAuthEngineId = NULL;
   message->msgAuthEngineIdLen = 0;
   message->msgAuthEngineBoots = 0;
   message->msgAuthEngineTime = 0;
   message->msgUserName = NULL;
   message->msgUserNameLen = 0;
   message->msgAuthParameters = NULL;
   message->msgAuthParametersLen = 0;
   message->msgPrivParameters = NULL;
   message->msgPrivParametersLen = 0;

   //Initialize scopedPDU fields
   message->contextEngineId = NULL;
   message->contextEngineIdLen = 0;
   message->contextName = NULL;
   message->contextNameLen = 0;
#endif

   //Initialize PDU fields
   message->pduType = SNMP_PDU_GET_REQUEST;
   message->requestId = 0;
   message->errorStatus = SNMP_ERROR_NONE;
   message->errorIndex = 0;

#if (SNMP_V1_SUPPORT == ENABLED)
   message->enterpriseOid = NULL;
   message->enterpriseOidLen = 0;
   message->agentAddr = IPV4_UNSPECIFIED_ADDR;
   message->genericTrapType = 0;
   message->specificTrapCode = 0;
   message->timestamp = 0;
#endif

#if (SNMP_V2C_SUPPORT == ENABLED || SNMP_V3_SUPPORT == ENABLED)
   message->nonRepeaters = 0;
   message->maxRepetitions = 0;
#endif

   //Initialize the list of variable bindings
   message->varBindList = NULL;
   message->varBindListLen = 0;
   message->varBindListMaxLen = 0;
   message->oidLen = 0;
}


/**
 * @brief Initialize a GetResponse-PDU
 * @param[in] context Pointer to the SNMP agent context
 * @return Error code
 **/

error_t snmpInitResponse(SnmpAgentContext *context)
{
   error_t error;

   //SNMP version identifier
   context->response.version = context->request.version;

#if (SNMP_V1_SUPPORT == ENABLED || SNMP_V2C_SUPPORT == ENABLED)
   //Community name
   context->response.community = context->request.community;
   context->response.communityLen = context->request.communityLen;
#endif

#if (SNMP_V3_SUPPORT == ENABLED)
   //Message identifier
   context->response.msgId = context->request.msgId;
   //Maximum message size supported by the sender
   context->response.msgMaxSize = SNMP_MAX_MSG_SIZE;

   //Bit fields which control processing of the message
   context->response.msgFlags = context->request.msgFlags &
      (SNMP_MSG_FLAG_AUTH | SNMP_MSG_FLAG_PRIV);

   //Security model used by the sender
   context->response.msgSecurityModel = context->request.msgSecurityModel;

   //Authoritative engine identifier
   context->response.msgAuthEngineId = context->contextEngine;
   context->response.msgAuthEngineIdLen = context->contextEngineLen;

   //Number of times the SNMP engine has rebooted
   context->response.msgAuthEngineBoots = context->engineBoots;
   //Number of seconds since last reboot
   context->response.msgAuthEngineTime = context->engineTime;

   //User name
   context->response.msgUserName = context->request.msgUserName;
   context->response.msgUserNameLen = context->request.msgUserNameLen;

   //Authentication parameters
   context->response.msgAuthParameters = NULL;
   context->response.msgAuthParametersLen = context->request.msgAuthParametersLen;

   //Privacy parameters
   context->response.msgPrivParameters = context->privParameters;
   context->response.msgPrivParametersLen = context->request.msgPrivParametersLen;

   //Context engine identifier
   context->response.contextEngineId = context->contextEngine;
   context->response.contextEngineIdLen = context->contextEngineLen;

   //Context name
   context->response.contextName = context->request.contextName;
   context->response.contextNameLen = context->request.contextNameLen;
#endif

   //PDU type
   context->response.pduType = SNMP_PDU_GET_RESPONSE;
   //Request identifier
   context->response.requestId = context->request.requestId;

   //Make room for the message header at the beginning of the buffer
   error = snmpComputeMessageOverhead(&context->response);
   //Return status code
   return error;
}


/**
 * @brief Compute SNMP message overhead
 * @param[in] message Pointer to the SNMP message
 **/

error_t snmpComputeMessageOverhead(SnmpMessage *message)
{
   size_t n;

#if (SNMP_V1_SUPPORT == ENABLED)
   //SNMPv1 version?
   if(message->version == SNMP_VERSION_1)
   {
      //SNMPv1 message header overhead
      n = SNMP_V1_MSG_HEADER_OVERHEAD;
      //Take into consideration variable-length fields
      n += message->communityLen + message->enterpriseOidLen;
   }
   else
#endif
#if (SNMP_V2C_SUPPORT == ENABLED)
   //SNMPv2c version?
   if(message->version == SNMP_VERSION_2C)
   {
      //SNMPv2c message header overhead
      n = SNMP_V2C_MSG_HEADER_OVERHEAD;
      //Take into consideration variable-length fields
      n += message->communityLen;
   }
   else
#endif
#if (SNMP_V3_SUPPORT == ENABLED)
   //SNMPv3 version?
   if(message->version == SNMP_VERSION_3)
   {
      //SNMPv3 message header overhead
      n = SNMP_V3_MSG_HEADER_OVERHEAD;

      //Take into consideration variable-length fields
      n += message->msgAuthEngineIdLen + message->msgUserNameLen +
         message->msgAuthParametersLen + message->msgPrivParametersLen +
         message->contextEngineIdLen + message->contextNameLen;
   }
   else
#endif
   //Invalid SNMP version?
   {
      //Report an error
      return ERROR_INVALID_VERSION;
   }

   //Sanity check
   if(n > (SNMP_MAX_MSG_SIZE - SNMP_MSG_ENCRYPTION_OVERHEAD))
      return ERROR_FAILURE;

   //Make room for the message header at the beginning of the buffer
   message->varBindList = message->buffer + n;
   //Maximum length of the variable binding list
   message->varBindListMaxLen = (SNMP_MAX_MSG_SIZE - SNMP_MSG_ENCRYPTION_OVERHEAD) - n;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse SNMP message header
 * @param[in,out] message Pointer to the incoming SNMP message
 * @return Error code
 **/

error_t snmpParseMessageHeader(SnmpMessage *message)
{
   error_t error;
   size_t length;
   const uint8_t *p;
   Asn1Tag tag;

   //Point to the first byte of the SNMP message
   p = message->buffer;
   //Retrieve the length of the SNMP message
   length = message->bufferLen;

   //The SNMP message is encapsulated within a sequence
   error = asn1ReadSequence(p, length, &tag);
   //Failed to decode ASN.1 tag?
   if(error)
      return error;

   //Point to the first field of the sequence
   p = tag.value;
   length = tag.length;

   //Read version identifier
   error = asn1ReadInt32(p, length, &tag, &message->version);
   //Failed to decode ASN.1 tag?
   if(error)
      return error;

   //Make sure the SNMP version identifier is valid
   if(message->version != SNMP_VERSION_1 &&
      message->version != SNMP_VERSION_2C &&
      message->version != SNMP_VERSION_3)
   {
      //The SNMP version is not acceptable
      return ERROR_INVALID_VERSION;
   }

   //Advance data pointer
   message->pos = (uint8_t *) p + tag.totalLength;
   //Remaining bytes to process
   message->length = length - tag.totalLength;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Format SNMP message header
 * @param[in,out] message Pointer to the outgoing SNMP message
 * @return Error code
 **/

error_t snmpWriteMessageHeader(SnmpMessage *message)
{
   error_t error;
   size_t n;
   Asn1Tag tag;

   //SNMPv1 or SNMPv2c version?
   if(message->version == SNMP_VERSION_1 ||
      message->version == SNMP_VERSION_2C)
   {
      //Write the community name
      error = snmpWriteCommunity(message);
      //Any error to report?
      if(error)
         return error;
   }
   //SNMPv3 version?
   else if(message->version == SNMP_VERSION_3)
   {
      //Write msgSecurityParameters field
      error = snmpWriteSecurityParameters(message);
      //Any error to report?
      if(error)
         return error;

      //Write msgGlobalData field
      error = snmpWriteGlobalData(message);
      //Any error to report?
      if(error)
         return error;
   }
   //Invalid version?
   else
   {
      //Report an error
      return ERROR_INVALID_VERSION;
   }

   //Write version identifier
   error = asn1WriteInt32(message->version, TRUE, message->pos, &n);
   //Any error to report?
   if(error)
      return error;

   //Move backward
   message->pos -= n;
   //Update the length of the message
   message->length += n;

   //The SNMP message is encapsulated within a sequence
   tag.constructed = TRUE;
   tag.objClass = ASN1_CLASS_UNIVERSAL;
   tag.objType = ASN1_TYPE_SEQUENCE;
   tag.length = message->length;
   tag.value = NULL;

   //Write the corresponding ASN.1 tag
   error = asn1WriteTag(&tag, TRUE, message->pos, &n);
   //Any error to report?
   if(error)
      return error;

   //Move backward
   message->pos -= n;
   //Total length of the SNMP message
   message->length += n;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse community name
 * @param[in,out] message Pointer to the incoming SNMP message
 * @return Error code
 **/

error_t snmpParseCommunity(SnmpMessage *message)
{
#if (SNMP_V1_SUPPORT == ENABLED || SNMP_V2C_SUPPORT == ENABLED)
   error_t error;
   Asn1Tag tag;

   //Read community name
   error = asn1ReadTag(message->pos, message->length, &tag);
   //Failed to decode ASN.1 tag?
   if(error)
      return error;

   //Enforce encoding, class and type
   error = asn1CheckTag(&tag, FALSE, ASN1_CLASS_UNIVERSAL, ASN1_TYPE_OCTET_STRING);
   //The tag does not match the criteria?
   if(error)
      return error;

   //Save community name
   message->community = (char_t *) tag.value;
   message->communityLen = tag.length;

   //Advance data pointer
   message->pos += tag.totalLength;
   //Remaining bytes to process
   message->length -= tag.totalLength;

   //No error to report
   return NO_ERROR;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Format community name
 * @param[in,out] message Pointer to the outgoing SNMP message
 * @return Error code
 **/

error_t snmpWriteCommunity(SnmpMessage *message)
{
#if (SNMP_V1_SUPPORT == ENABLED || SNMP_V2C_SUPPORT == ENABLED)
   error_t error;
   size_t n;
   Asn1Tag tag;

   //The community name is an octet string
   tag.constructed = FALSE;
   tag.objClass = ASN1_CLASS_UNIVERSAL;
   tag.objType = ASN1_TYPE_OCTET_STRING;
   tag.length = message->communityLen;
   tag.value = (uint8_t *) message->community;

   //Write the corresponding ASN.1 tag
   error = asn1WriteTag(&tag, TRUE, message->pos, &n);
   //Any error to report?
   if(error)
      return error;

   //Point to the first byte of the community name
   message->pos -= n;
   //Total length of the message
   message->length += n;

   //Successful processing
   return NO_ERROR;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Parse msgGlobalData field
 * @param[in,out] message Pointer to the incoming SNMP message
 * @return Error code
 **/

error_t snmpParseGlobalData(SnmpMessage *message)
{
#if (SNMP_V3_SUPPORT == ENABLED)
   error_t error;
   size_t length;
   const uint8_t *p;
   Asn1Tag tag;

   //Read the msgGlobalData field
   error = asn1ReadSequence(message->pos, message->length, &tag);
   //Failed to decode ASN.1 tag?
   if(error)
      return error;

   //Advance pointer over the msgGlobalData field
   message->pos += tag.totalLength;
   //Remaining bytes to process
   message->length -= tag.totalLength;

   //Point to the first field of the sequence
   p = tag.value;
   length = tag.length;

   //The msgID is used between two SNMP entities to coordinate request
   //messages and responses
   error = asn1ReadInt32(p, length, &tag, &message->msgId);
   //Failed to decode ASN.1 tag?
   if(error)
      return error;

   //Make sure the value is in the range 0..2147483647
   if(message->msgId < 0)
      return ERROR_WRONG_ENCODING;

   //Point to the next field
   p += tag.totalLength;
   length -= tag.totalLength;

   //The msgMaxSize field of the message conveys the maximum message size
   //supported by the sender of the message
   error = asn1ReadInt32(p, length, &tag, &message->msgMaxSize);
   //Failed to decode ASN.1 tag?
   if(error)
      return error;

   //Make sure the value is in the range 484..2147483647
   if(message->msgMaxSize < 484)
      return ERROR_WRONG_ENCODING;

   //Point to the next field
   p += tag.totalLength;
   length -= tag.totalLength;

   //The msgFlags field of the message contains several bit fields which
   //control processing of the message
   error = asn1ReadTag(p, length, &tag);
   //Failed to decode ASN.1 tag?
   if(error)
      return error;

   //Enforce encoding, class and type
   error = asn1CheckTag(&tag, FALSE, ASN1_CLASS_UNIVERSAL, ASN1_TYPE_OCTET_STRING);
   //The tag does not match the criteria?
   if(error)
      return error;

   //The msgFlags field consists of a single byte
   if(tag.length != sizeof(uint8_t))
      return ERROR_WRONG_ENCODING;

   //Save the bit field
   message->msgFlags = tag.value[0];

   //If the authFlag is not set and privFlag is set, then the snmpInvalidMsgs
   //counter is incremented and the message is silently discarded
   if((message->msgFlags & SNMP_MSG_FLAG_AUTH) == 0 &&
      (message->msgFlags & SNMP_MSG_FLAG_PRIV) != 0)
   {
      //Total number of packets received by the SNMP engine which were dropped
      //because there were invalid or inconsistent components in the message
      SNMP_MPD_MIB_INC_COUNTER32(snmpInvalidMsgs, 1);

      //The message is discarded without further processing
      return ERROR_FAILURE;
   }

   //Point to the next field
   p += tag.totalLength;
   length -= tag.totalLength;

   //The msgSecurityModel field identifies which security model was used
   //by the sender to generate the message
   error = asn1ReadInt32(p, length, &tag, &message->msgSecurityModel);
   //Failed to decode ASN.1 tag?
   if(error)
      return error;

   //Make sure the value is in the range 1..2147483647
   if(message->msgSecurityModel < 1)
      return ERROR_WRONG_ENCODING;

   //Successful processing
   return NO_ERROR;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Format msgGlobalData field
 * @param[in,out] message Pointer to the outgoing SNMP message
 * @return Error code
 **/

error_t snmpWriteGlobalData(SnmpMessage *message)
{
#if (SNMP_V3_SUPPORT == ENABLED)
   error_t error;
   size_t n;
   size_t length;
   uint8_t *p;
   Asn1Tag tag;

   //The msgGlobalData field is encoded in reverse order
   p = message->pos;
   //Length of the msgGlobalData field
   length = 0;

   //Write msgSecurityModel field
   error = asn1WriteInt32(message->msgSecurityModel, TRUE, p, &n);
   //Any error to report?
   if(error)
      return error;

   //Move backward
   p -= n;
   //Update the length of the msgGlobalData field
   length += n;

   //The msgFlags field consists of a single byte
   tag.constructed = FALSE;
   tag.objClass = ASN1_CLASS_UNIVERSAL;
   tag.objType = ASN1_TYPE_OCTET_STRING;
   tag.length = sizeof(uint8_t);
   tag.value = &message->msgFlags;

   //Write the corresponding ASN.1 tag
   error = asn1WriteTag(&tag, TRUE, p, &n);
   //Any error to report?
   if(error)
      return error;

   //Move backward
   p -= n;
   //Update the length of the msgGlobalData field
   length += n;

   //Write msgMaxSize field
   error = asn1WriteInt32(message->msgMaxSize, TRUE, p, &n);
   //Any error to report?
   if(error)
      return error;

   //Move backward
   p -= n;
   //Update the length of the msgGlobalData field
   length += n;

   //Write msgID field
   error = asn1WriteInt32(message->msgId, TRUE, p, &n);
   //Any error to report?
   if(error)
      return error;

   //Move backward
   p -= n;
   //Update the length of the msgGlobalData field
   length += n;

   //The parameters are encapsulated within a sequence
   tag.constructed = TRUE;
   tag.objClass = ASN1_CLASS_UNIVERSAL;
   tag.objType = ASN1_TYPE_SEQUENCE;
   tag.length = length;
   tag.value = NULL;

   //Write the corresponding ASN.1 tag
   error = asn1WriteTag(&tag, TRUE, p, &n);
   //Any error to report?
   if(error)
      return error;

   //Point to the first byte of the msgGlobalData field
   message->pos = p - n;
   //Total length of the message
   message->length += length + n;

   //Successful processing
   return NO_ERROR;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Parse msgSecurityParameters field
 * @param[in,out] message Pointer to the incoming SNMP message
 * @return Error code
 **/

error_t snmpParseSecurityParameters(SnmpMessage *message)
{
#if (SNMP_V3_SUPPORT == ENABLED)
   error_t error;
   size_t length;
   const uint8_t *p;
   Asn1Tag tag;

   //Read the msgSecurityParameters field
   error = asn1ReadTag(message->pos, message->length, &tag);
   //Failed to decode ASN.1 tag?
   if(error)
      return error;

   //Enforce encoding, class and type
   error = asn1CheckTag(&tag, FALSE, ASN1_CLASS_UNIVERSAL, ASN1_TYPE_OCTET_STRING);
   //The tag does not match the criteria?
   if(error)
      return error;

   //Advance pointer over the msgSecurityParameters field
   message->pos += tag.totalLength;
   //Remaining bytes to process
   message->length -= tag.totalLength;

   //Point to the very first field of the sequence
   p = tag.value;
   length = tag.length;

   //User-based security model?
   if(message->msgSecurityModel == SNMP_SECURITY_MODEL_USM)
   {
      //The USM security parameters are encapsulated within a sequence
      error = asn1ReadSequence(p, length, &tag);
      //Failed to decode ASN.1 tag?
      if(error)
         return error;

      //Point to the first field of the sequence
      p = tag.value;
      length = tag.length;

      //Read the msgAuthoritativeEngineID field
      error = asn1ReadTag(p, length, &tag);
      //Failed to decode ASN.1 tag?
      if(error)
         return error;

      //Enforce encoding, class and type
      error = asn1CheckTag(&tag, FALSE, ASN1_CLASS_UNIVERSAL, ASN1_TYPE_OCTET_STRING);
      //The tag does not match the criteria?
      if(error)
         return error;

      //Save authoritative engine identifier
      message->msgAuthEngineId = tag.value;
      message->msgAuthEngineIdLen = tag.length;

      //Point to the next field
      p += tag.totalLength;
      length -= tag.totalLength;

      //Read the msgAuthoritativeEngineBoots field
      error = asn1ReadInt32(p, length, &tag,
         &message->msgAuthEngineBoots);
      //Failed to decode ASN.1 tag?
      if(error)
         return error;

      //Point to the next field
      p += tag.totalLength;
      length -= tag.totalLength;

      //Read the msgAuthoritativeEngineTime field
      error = asn1ReadInt32(p, length, &tag,
         &message->msgAuthEngineTime);
      //Failed to decode ASN.1 tag?
      if(error)
         return error;

      //Point to the next field
      p += tag.totalLength;
      length -= tag.totalLength;

      //Read the msgUserName field
      error = asn1ReadTag(p, length, &tag);
      //Failed to decode ASN.1 tag?
      if(error)
         return error;

      //Enforce encoding, class and type
      error = asn1CheckTag(&tag, FALSE, ASN1_CLASS_UNIVERSAL, ASN1_TYPE_OCTET_STRING);
      //The tag does not match the criteria?
      if(error)
         return error;

      //Check the length of the user name
      if(tag.length > 32)
         return ERROR_WRONG_ENCODING;

      //Save user name
      message->msgUserName = (char_t *) tag.value;
      message->msgUserNameLen = tag.length;

      //Point to the next field
      p += tag.totalLength;
      length -= tag.totalLength;

      //Read the msgAuthenticationParameters field
      error = asn1ReadTag(p, length, &tag);
      //Failed to decode ASN.1 tag?
      if(error)
         return error;

      //Enforce encoding, class and type
      error = asn1CheckTag(&tag, FALSE, ASN1_CLASS_UNIVERSAL, ASN1_TYPE_OCTET_STRING);
      //The tag does not match the criteria?
      if(error)
         return error;

      //Save authentication parameters
      message->msgAuthParameters = (uint8_t *) tag.value;
      message->msgAuthParametersLen = tag.length;

      //Point to the next field
      p += tag.totalLength;
      length -= tag.totalLength;

      //Read the msgPrivacyParameters field
      error = asn1ReadTag(p, length, &tag);
      //Failed to decode ASN.1 tag?
      if(error)
         return error;

      //Enforce encoding, class and type
      error = asn1CheckTag(&tag, FALSE, ASN1_CLASS_UNIVERSAL, ASN1_TYPE_OCTET_STRING);
      //The tag does not match the criteria?
      if(error)
         return error;

      //Save privacy parameters
      message->msgPrivParameters = tag.value;
      message->msgPrivParametersLen = tag.length;
   }
   else
   {
      //Total number of packets received by the SNMP engine which were dropped
      //because they referenced a securityModel that was not known to or
      //supported by the SNMP engine
      SNMP_MPD_MIB_INC_COUNTER32(snmpUnknownSecurityModels, 1);

      //The message is discarded without further processing
      return ERROR_FAILURE;
   }

   //Successful processing
   return NO_ERROR;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Format msgSecurityParameters field
 * @param[in,out] message Pointer to the outgoing SNMP message
 * @return Error code
 **/

error_t snmpWriteSecurityParameters(SnmpMessage *message)
{
#if (SNMP_V3_SUPPORT == ENABLED)
   error_t error;
   size_t n;
   size_t length;
   uint8_t *p;
   Asn1Tag tag;

   //The msgSecurityParameters field is encoded in reverse order
   p = message->pos;
   //Length of the msgSecurityParameters field
   length = 0;

   //User-based security model?
   if(message->msgSecurityModel == SNMP_SECURITY_MODEL_USM)
   {
      //Encode the msgPrivacyParameters field as an octet string
      tag.constructed = FALSE;
      tag.objClass = ASN1_CLASS_UNIVERSAL;
      tag.objType = ASN1_TYPE_OCTET_STRING;
      tag.length = message->msgPrivParametersLen;
      tag.value = message->msgPrivParameters;

      //Write the corresponding ASN.1 tag
      error = asn1WriteTag(&tag, TRUE, p, &n);
      //Any error to report?
      if(error)
         return error;

      //Move backward
      p -= n;
      //Update the length of the msgSecurityParameters field
      length += n;

      //Authentication required?
      if(message->msgAuthParametersLen > 0)
      {
         //Make room for the message digest
         p -= message->msgAuthParametersLen;
         //Update the length of the msgSecurityParameters field
         length += message->msgAuthParametersLen;

         //Clear the message digest
         osMemset(p, 0, message->msgAuthParametersLen);
      }

      //Save the location of the msgAuthenticationParameters field
      message->msgAuthParameters = p;

      //Encoded the msgAuthenticationParameters field as an octet string
      tag.constructed = FALSE;
      tag.objClass = ASN1_CLASS_UNIVERSAL;
      tag.objType = ASN1_TYPE_OCTET_STRING;
      tag.length = message->msgAuthParametersLen;
      tag.value = NULL;

      //Write the corresponding ASN.1 tag
      error = asn1WriteTag(&tag, TRUE, p, &n);
      //Any error to report?
      if(error)
         return error;

      //Move backward
      p -= n;
      //Update the length of the msgSecurityParameters field
      length += n;

      //Encode the msgUserName field as an octet string
      tag.constructed = FALSE;
      tag.objClass = ASN1_CLASS_UNIVERSAL;
      tag.objType = ASN1_TYPE_OCTET_STRING;
      tag.length = message->msgUserNameLen;
      tag.value = (uint8_t *) message->msgUserName;

      //Write the corresponding ASN.1 tag
      error = asn1WriteTag(&tag, TRUE, p, &n);
      //Any error to report?
      if(error)
         return error;

      //Move backward
      p -= n;
      //Update the length of the msgSecurityParameters field
      length += n;

      //Write the msgAuthoritativeEngineTime field
      error = asn1WriteInt32(message->msgAuthEngineTime, TRUE, p, &n);
      //Any error to report?
      if(error)
         return error;

      //Move backward
      p -= n;
      //Update the length of the msgSecurityParameters field
      length += n;

      //Write the msgAuthoritativeEngineBoots field
      error = asn1WriteInt32(message->msgAuthEngineBoots, TRUE, p, &n);
      //Any error to report?
      if(error)
         return error;

      //Move backward
      p -= n;
      //Update the length of the msgSecurityParameters field
      length += n;

      //The msgAuthoritativeEngineID field is an octet string
      tag.constructed = FALSE;
      tag.objClass = ASN1_CLASS_UNIVERSAL;
      tag.objType = ASN1_TYPE_OCTET_STRING;
      tag.length = message->msgAuthEngineIdLen;
      tag.value = message->msgAuthEngineId;

      //Write the corresponding ASN.1 tag
      error = asn1WriteTag(&tag, TRUE, p, &n);
      //Any error to report?
      if(error)
         return error;

      //Move backward
      p -= n;
      //Update the length of the msgSecurityParameters field
      length += n;

      //The USM security parameters are encapsulated within a sequence
      tag.constructed = TRUE;
      tag.objClass = ASN1_CLASS_UNIVERSAL;
      tag.objType = ASN1_TYPE_SEQUENCE;
      tag.length = length;
      tag.value = NULL;

      //Write the corresponding ASN.1 tag
      error = asn1WriteTag(&tag, TRUE, p, &n);
      //Any error to report?
      if(error)
         return error;

      //Move backward
      p -= n;
      //Update the length of the msgSecurityParameters field
      length += n;
   }
   else
   {
      //The security model is not supported
      return ERROR_FAILURE;
   }

   //The security parameters are encapsulated within an octet string
   tag.constructed = FALSE;
   tag.objClass = ASN1_CLASS_UNIVERSAL;
   tag.objType = ASN1_TYPE_OCTET_STRING;
   tag.length = length;
   tag.value = NULL;

   //Write the corresponding ASN.1 tag
   error = asn1WriteTag(&tag, TRUE, p, &n);
   //Any error to report?
   if(error)
      return error;

   //Point to the first byte of the msgSecurityParameters field
   message->pos = p - n;
   //Total length of the message
   message->length += length + n;

   //Successful processing
   return NO_ERROR;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Parse scopedPDU field
 * @param[in,out] message Pointer to the incoming SNMP message
 * @return Error code
 **/

error_t snmpParseScopedPdu(SnmpMessage *message)
{
#if (SNMP_V3_SUPPORT == ENABLED)
   error_t error;
   size_t length;
   const uint8_t *p;
   Asn1Tag tag;

   //Read the scopedPDU field
   error = asn1ReadSequence(message->pos, message->length, &tag);
   //Failed to decode ASN.1 tag?
   if(error)
      return error;

   //Point to the first field of the sequence
   p = tag.value;
   length = tag.length;

   //Read contextEngineID field
   error = asn1ReadTag(p, length, &tag);
   //Failed to decode ASN.1 tag?
   if(error)
      return error;

   //Enforce encoding, class and type
   error = asn1CheckTag(&tag, FALSE, ASN1_CLASS_UNIVERSAL, ASN1_TYPE_OCTET_STRING);
   //The tag does not match the criteria?
   if(error)
      return error;

   //Save context engine identifier
   message->contextEngineId = tag.value;
   message->contextEngineIdLen = tag.length;

   //Point to the next field
   p += tag.totalLength;
   length -= tag.totalLength;

   //Read contextName field
   error = asn1ReadTag(p, length, &tag);
   //Failed to decode ASN.1 tag?
   if(error)
      return error;

   //Enforce encoding, class and type
   error = asn1CheckTag(&tag, FALSE, ASN1_CLASS_UNIVERSAL, ASN1_TYPE_OCTET_STRING);
   //The tag does not match the criteria?
   if(error)
      return error;

   //Save context name
   message->contextName = (char_t *) tag.value;
   message->contextNameLen = tag.length;

   //Point to the first byte of the PDU
   message->pos = (uint8_t *) p + tag.totalLength;
   //Length of the PDU
   message->length = length - tag.totalLength;

   //Return status code
   return error;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Format scopedPDU
 * @param[in,out] message Pointer to the outgoing SNMP message
 * @return Error code
 **/

error_t snmpWriteScopedPdu(SnmpMessage *message)
{
#if (SNMP_V3_SUPPORT == ENABLED)
   error_t error;
   size_t n;
   size_t length;
   uint8_t *p;
   Asn1Tag tag;

   //Point to the first byte of the PDU
   p = message->pos;
   //Retrieve the length of the PDU
   length = message->length;

   //The contextName is an octet string
   tag.constructed = FALSE;
   tag.objClass = ASN1_CLASS_UNIVERSAL;
   tag.objType = ASN1_TYPE_OCTET_STRING;
   tag.length = message->contextNameLen;
   tag.value = (uint8_t *) message->contextName;

   //Write the corresponding ASN.1 tag
   error = asn1WriteTag(&tag, TRUE, p, &n);
   //Any error to report?
   if(error)
      return error;

   //Move backward
   p -= n;
   //Update the length of the scopedPduData
   length += n;

   //The contextEngineID is an octet string
   tag.constructed = FALSE;
   tag.objClass = ASN1_CLASS_UNIVERSAL;
   tag.objType = ASN1_TYPE_OCTET_STRING;
   tag.length = message->contextEngineIdLen;
   tag.value = message->contextEngineId;

   //Write the corresponding ASN.1 tag
   error = asn1WriteTag(&tag, TRUE, p, &n);
   //Any error to report?
   if(error)
      return error;

   //Move backward
   p -= n;
   //Update the length of the scopedPduData
   length += n;

   //The scopedPduData is encapsulated within a sequence
   tag.constructed = TRUE;
   tag.objClass = ASN1_CLASS_UNIVERSAL;
   tag.objType = ASN1_TYPE_SEQUENCE;
   tag.length = length;
   tag.value = NULL;

   //Write the corresponding ASN.1 tag
   error = asn1WriteTag(&tag, TRUE, p, &n);
   //Any error to report?
   if(error)
      return error;

   //Point to the first byte of the scopedPDU
   message->pos = p - n;
   //Length of the scopedPDU
   message->length = length + n;

   //Successful processing
   return NO_ERROR;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Parse PDU header
 * @param[in,out] message Pointer to the incoming SNMP message
 * @return Error code
 **/

error_t snmpParsePduHeader(SnmpMessage *message)
{
   error_t error;
   size_t length;
   const uint8_t *p;
   Asn1Tag tag;

   //The PDU is encapsulated within a sequence
   error = asn1ReadTag(message->pos, message->length, &tag);
   //Failed to decode ASN.1 tag?
   if(error)
      return error;

   //Check encoding
   if(tag.constructed != TRUE)
      return ERROR_WRONG_ENCODING;
   //Enforce class
   if(tag.objClass != ASN1_CLASS_CONTEXT_SPECIFIC)
      return ERROR_INVALID_CLASS;

   //Save PDU type
   message->pduType = (SnmpPduType) tag.objType;

   //Point to the first field
   p = tag.value;
   //Remaining bytes to process
   length = tag.length;

   //Read request-id field
   error = asn1ReadInt32(p, length, &tag, &message->requestId);
   //Failed to decode ASN.1 tag?
   if(error)
      return error;

   //Point to the next field
   p += tag.totalLength;
   length -= tag.totalLength;

#if (SNMP_V2C_SUPPORT == ENABLED || SNMP_V3_SUPPORT == ENABLED)
   //GetBulkRequest-PDU?
   if(message->pduType == SNMP_PDU_GET_BULK_REQUEST)
   {
      //Read non-repeaters field
      error = asn1ReadInt32(p, length, &tag, &message->nonRepeaters);
      //Failed to decode ASN.1 tag?
      if(error)
         return error;

      //If the value in the non-repeaters field is less than zero, then the
      //value of the field is set to zero
      if(message->nonRepeaters < 0)
         message->nonRepeaters = 0;

      //Point to the next field
      p += tag.totalLength;
      length -= tag.totalLength;

      //Read max-repetitions field
      error = asn1ReadInt32(p, length, &tag, &message->maxRepetitions);
      //Failed to decode ASN.1 tag?
      if(error)
         return error;

      //If the value in the max-repetitions field is less than zero, then the
      //value of the field is set to zero
      if(message->maxRepetitions < 0)
         message->maxRepetitions = 0;
   }
   else
#endif
   {
      //Read error-status field
      error = asn1ReadInt32(p, length, &tag, &message->errorStatus);
      //Failed to decode ASN.1 tag?
      if(error)
         return error;

      //Point to the next field
      p += tag.totalLength;
      length -= tag.totalLength;

      //Read error-index field
      error = asn1ReadInt32(p, length, &tag, &message->errorIndex);
      //Failed to decode ASN.1 tag?
      if(error)
         return error;
   }

   //Point to the next field
   p += tag.totalLength;
   length -= tag.totalLength;

   //The variable bindings are encapsulated within a sequence
   error = asn1ReadSequence(p, length, &tag);
   //Failed to decode ASN.1 tag?
   if(error)
      return error;

   //Save the location of the variable binding list
   message->varBindList = (uint8_t *) tag.value;
   message->varBindListLen = tag.length;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Format PDU header
 * @param[in,out] message Pointer to the outgoing SNMP message
 * @return Error code
 **/

error_t snmpWritePduHeader(SnmpMessage *message)
{
   error_t error;
   size_t n;
   size_t length;
   uint8_t *p;
   Asn1Tag tag;

   //The PDU header will be encoded in reverse order...
   p = message->varBindList;
   //Length of the PDU
   length = message->varBindListLen;

   //The variable bindings are encapsulated within a sequence
   tag.constructed = TRUE;
   tag.objClass = ASN1_CLASS_UNIVERSAL;
   tag.objType = ASN1_TYPE_SEQUENCE;
   tag.length = length;
   tag.value = NULL;

   //Write the corresponding ASN.1 tag
   error = asn1WriteTag(&tag, TRUE, p, &n);
   //Any error to report?
   if(error)
      return error;

   //Move backward
   p -= n;
   //Update the length of the PDU
   length += n;

   //GetRequest-PDU, GetResponse-PDU, InformRequest-PDU, SNMPv2-Trap-PDU
   //or Report-PDU?
   if(message->pduType == SNMP_PDU_GET_REQUEST ||
      message->pduType == SNMP_PDU_GET_RESPONSE ||
      message->pduType == SNMP_PDU_INFORM_REQUEST ||
      message->pduType == SNMP_PDU_TRAP_V2 ||
      message->pduType == SNMP_PDU_REPORT)
   {
      //Write error index
      error = asn1WriteInt32(message->errorIndex, TRUE, p, &n);
      //Any error to report?
      if(error)
         return error;

      //Move backward
      p -= n;
      //Update the length of the PDU
      length += n;

      //Write error status
      error = asn1WriteInt32(message->errorStatus, TRUE, p, &n);
      //Any error to report?
      if(error)
         return error;

      //Move backward
      p -= n;
      //Update the length of the PDU
      length += n;

      //Write request identifier
      error = asn1WriteInt32(message->requestId, TRUE, p, &n);
      //Any error to report?
      if(error)
         return error;

      //Move backward
      p -= n;
      //Update the length of the PDU
      length += n;
   }
#if (SNMP_V1_SUPPORT == ENABLED)
   //Trap-PDU?
   else if(message->pduType == SNMP_PDU_TRAP)
   {
      //Encode the object value using ASN.1 rules
      error = snmpEncodeUnsignedInt32(message->timestamp, message->buffer, &n);
      //Any error to report?
      if(error)
         return error;

      //The time stamp is encoded in ASN.1 format
      tag.constructed = FALSE;
      tag.objClass = ASN1_CLASS_APPLICATION;
      tag.objType = MIB_TYPE_TIME_TICKS;
      tag.length = n;
      tag.value = message->buffer;

      //Write the corresponding ASN.1 tag
      error = asn1WriteTag(&tag, TRUE, p, &n);
      //Any error to report?
      if(error)
         return error;

      //Move backward
      p -= n;
      //Update the length of the PDU
      length += n;

      //Write specific trap code
      error = asn1WriteInt32(message->specificTrapCode, TRUE, p, &n);
      //Any error to report?
      if(error)
         return error;

      //Move backward
      p -= n;
      //Update the length of the PDU
      length += n;

      //Write generic trap type
      error = asn1WriteInt32(message->genericTrapType, TRUE, p, &n);
      //Any error to report?
      if(error)
         return error;

      //Move backward
      p -= n;
      //Update the length of the PDU
      length += n;

      //The agent address is encoded in ASN.1 format
      tag.constructed = FALSE;
      tag.objClass = ASN1_CLASS_APPLICATION;
      tag.objType = MIB_TYPE_IP_ADDRESS;
      tag.length = sizeof(Ipv4Addr);
      tag.value = (uint8_t *) &message->agentAddr;

      //Write the corresponding ASN.1 tag
      error = asn1WriteTag(&tag, TRUE, p, &n);
      //Any error to report?
      if(error)
         return error;

      //Move backward
      p -= n;
      //Update the length of the PDU
      length += n;

      //The enterprise OID is encoded in ASN.1 format
      tag.constructed = FALSE;
      tag.objClass = ASN1_CLASS_UNIVERSAL;
      tag.objType = ASN1_TYPE_OBJECT_IDENTIFIER;
      tag.length = message->enterpriseOidLen;
      tag.value = message->enterpriseOid;

      //Write the corresponding ASN.1 tag
      error = asn1WriteTag(&tag, TRUE, p, &n);
      //Any error to report?
      if(error)
         return error;

      //Move backward
      p -= n;
      //Update the length of the PDU
      length += n;
   }
#endif
   //Unknown PDU type?
   else
   {
      //Report an error
      return ERROR_FAILURE;
   }

   //The PDU is encapsulated within a sequence
   tag.constructed = TRUE;
   tag.objClass = ASN1_CLASS_CONTEXT_SPECIFIC;
   tag.objType = message->pduType;
   tag.length = length;
   tag.value = NULL;

   //Write the corresponding ASN.1 tag
   error = asn1WriteTag(&tag, TRUE, p, &n);
   //Any error to report?
   if(error)
      return error;

   //Point to the first byte of the PDU
   message->pos = p - n;
   //Total length of the PDU
   message->length = length + n;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Encode a 32-bit signed integer
 * @param[in] value Integer value
 * @param[out] dest Buffer where to encode the integer
 * @param[out] length Total number of bytes that have been written
 * @return Error code
 **/

error_t snmpEncodeInt32(int32_t value, uint8_t *dest, size_t *length)
{
   size_t i;
   size_t j;
   uint8_t *src;

   //Check parameters
   if(dest == NULL || length == NULL)
      return ERROR_INVALID_PARAMETER;

   //The integer is encoded MSB first
   value = (int32_t) htobe32(value);
   //Cast the integer to byte array
   src = (uint8_t *) &value;

   //An integer value is always encoded in the smallest possible number of octets
   for(i = 0; i < 3; i++)
   {
      //The upper 9 bits shall not have the same value (all 0 or all 1)
      if((src[i] != 0x00 || (src[i + 1] & 0x80) != 0x00) &&
         (src[i] != 0xFF || (src[i + 1] & 0x80) != 0x80))
      {
         break;
      }
   }

   //Point to the beginning of the output buffer
   j = 0;

   //Copy integer value
   while(i < 4)
   {
      dest[j++] = src[i++];
   }

   //Total number of bytes that have been written
   *length = j;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Encode a 32-bit unsigned integer
 * @param[in] value Integer value
 * @param[out] dest Buffer where to encode the integer
 * @param[out] length Total number of bytes that have been written
 * @return Error code
 **/

error_t snmpEncodeUnsignedInt32(uint32_t value, uint8_t *dest, size_t *length)
{
   size_t i;
   size_t j;
   uint8_t *src;

   //Check parameters
   if(dest == NULL || length == NULL)
      return ERROR_INVALID_PARAMETER;

   //The integer is encoded MSB first
   value = htobe32(value);
   //Cast the integer to byte array
   src = (uint8_t *) &value;

   //An integer value is always encoded in the smallest possible number of octets
   for(i = 0; i < 3; i++)
   {
      //Check the upper 8 bits
      if(src[i] != 0x00)
         break;
   }

   //Point to the beginning of the output buffer
   j = 0;

   //Check the most significant bit
   if(src[i] & 0x80)
      dest[j++] = 0;

   //Copy integer value
   while(i < 4)
   {
      dest[j++] = src[i++];
   }

   //Total number of bytes that have been written
   *length = j;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Encode a 64-bit unsigned integer
 * @param[in] value Integer value
 * @param[out] dest Buffer where to encode the integer
 * @param[out] length Total number of bytes that have been written
 * @return Error code
 **/

error_t snmpEncodeUnsignedInt64(uint64_t value, uint8_t *dest, size_t *length)
{
   size_t i;
   size_t j;
   uint8_t *src;

   //Check parameters
   if(dest == NULL || length == NULL)
      return ERROR_INVALID_PARAMETER;

   //The integer is encoded MSB first
   value = htobe64(value);
   //Cast the integer to byte array
   src = (uint8_t *) &value;

   //An integer value is always encoded in the smallest possible number of octets
   for(i = 0; i < 7; i++)
   {
      //Check the upper 8 bits
      if(src[i] != 0x00)
         break;
   }

   //Point to the beginning of the output buffer
   j = 0;

   //Check the most significant bit
   if(src[i] & 0x80)
   {
      dest[j++] = 0;
   }

   //Copy integer value
   while(i < 8)
   {
      dest[j++] = src[i++];
   }

   //Total number of bytes that have been written
   *length = j;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Decode a 32-bit signed integer
 * @param[in] src Buffer that contains the encoded value
 * @param[in] length Number of bytes to be processed
 * @param[out] value Resulting integer value
 * @return Error code
 **/

error_t snmpDecodeInt32(const uint8_t *src, size_t length, int32_t *value)
{
   size_t i;

   //Check parameters
   if(src == NULL || value == NULL)
      return ERROR_INVALID_PARAMETER;
   if(length < 1)
      return ERROR_INVALID_PARAMETER;

   //The contents octets shall be a two's complement binary
   //number equal to the integer value
   *value = (src[0] & 0x80) ? -1 : 0;

   //Process contents octets
   for(i = 0; i < length; i++)
   {
      //Rotate left operation
      *value <<= 8;
      //Reconstruct integer value
      *value |= src[i];
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Decode a 32-bit unsigned integer
 * @param[in] src Buffer that contains the encoded value
 * @param[in] length Number of bytes to be processed
 * @param[out] value Resulting integer value
 * @return Error code
 **/

error_t snmpDecodeUnsignedInt32(const uint8_t *src, size_t length, uint32_t *value)
{
   size_t i;

   //Check parameters
   if(src == NULL || value == NULL)
      return ERROR_INVALID_PARAMETER;
   if(length < 1)
      return ERROR_INVALID_PARAMETER;

   //Only accept non-negative numbers
   if(src[0] & 0x80)
      return ERROR_FAILURE;

   //Initialize integer value
   *value = 0;

   //Process contents octets
   for(i = 0; i < length; i++)
   {
      //Rotate left operation
      *value <<= 8;
      //Reconstruct integer value
      *value |= src[i];
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Decode a 64-bit unsigned integer
 * @param[in] src Buffer that contains the encoded value
 * @param[in] length Number of bytes to be processed
 * @param[out] value Resulting integer value
 * @return Error code
 **/

error_t snmpDecodeUnsignedInt64(const uint8_t *src, size_t length, uint64_t *value)
{
   size_t i;

   //Check parameters
   if(src == NULL || value == NULL)
      return ERROR_INVALID_PARAMETER;
   if(length < 1)
      return ERROR_INVALID_PARAMETER;

   //Only accept non-negative numbers
   if(src[0] & 0x80)
      return ERROR_FAILURE;

   //Initialize integer value
   *value = 0;

   //Process contents octets
   for(i = 0; i < length; i++)
   {
      //Rotate left operation
      *value <<= 8;
      //Reconstruct integer value
      *value |= src[i];
   }

   //Successful processing
   return NO_ERROR;
}

#endif
