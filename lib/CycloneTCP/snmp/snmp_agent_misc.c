/**
 * @file snmp_agent_misc.c
  * @brief Helper functions for SNMP agent
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
#include <limits.h>
#include "core/net.h"
#include "snmp/snmp_agent.h"
#include "snmp/snmp_agent_misc.h"
#include "snmp/snmp_agent_object.h"
#include "mibs/mib2_module.h"
#include "core/crypto.h"
#include "encoding/asn1.h"
#include "encoding/oid.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (SNMP_AGENT_SUPPORT == ENABLED)

//sysUpTime.0 object (1.3.6.1.2.1.1.3.0)
static const uint8_t sysUpTimeObject[] = {43, 6, 1, 2, 1, 1, 3, 0};
//snmpTrapOID.0 object (1.3.6.1.6.3.1.1.4.1.0)
static const uint8_t snmpTrapOidObject[] = {43, 6, 1, 6, 3, 1, 1, 4, 1, 0};
//snmpTraps object (1.3.6.1.6.3.1.1.5)
static const uint8_t snmpTrapsObject[] = {43, 6, 1, 6, 3, 1, 1, 5};


/**
 * @brief Lock MIB bases
 * @param[in] context Pointer to the SNMP agent context
 **/

void snmpLockMib(SnmpAgentContext *context)
{
   uint_t i;
   bool_t flag;

   //Initialize flag
   flag = FALSE;

   //Loop through MIBs
   for(i = 0; i < SNMP_AGENT_MAX_MIBS; i++)
   {
      //Valid MIB?
      if(context->mibTable[i] != NULL)
      {
         //Any registered callback?
         if(context->mibTable[i]->lock != NULL)
         {
            //Lock access to the MIB
            context->mibTable[i]->lock();
         }
         else
         {
            //The MIB does not feature any lock/unlock mechanism
            flag = TRUE;
         }
      }
   }

   //Default lock/unlock sequence?
   if(flag)
   {
      //Get exclusive access
      osAcquireMutex(&netMutex);
   }
}


/**
 * @brief Unlock MIB bases
 * @param[in] context Pointer to the SNMP agent context
 **/

void snmpUnlockMib(SnmpAgentContext *context)
{
   uint_t i;
   bool_t flag;

   //Initialize flag
   flag = FALSE;

   //Loop through MIBs
   for(i = 0; i < SNMP_AGENT_MAX_MIBS; i++)
   {
      //Valid MIB?
      if(context->mibTable[i] != NULL)
      {
         //Any registered callback?
         if(context->mibTable[i]->unlock != NULL)
         {
            //Unlock access to the MIB
            context->mibTable[i]->unlock();
         }
         else
         {
            //The MIB does not feature any lock/unlock mechanism
            flag = TRUE;
         }
      }
   }

   //Default lock/unlock sequence?
   if(flag)
   {
      //Release exclusive access
      osReleaseMutex(&netMutex);
   }
}


/**
 * @brief Create a new community entry
 * @param[in] context Pointer to the SNMP agent context
 * @return Pointer to the newly created entry
 **/

SnmpUserEntry *snmpCreateCommunityEntry(SnmpAgentContext *context)
{
   SnmpUserEntry *entry;

   //Initialize pointer
   entry = NULL;

#if (SNMP_V1_SUPPORT == ENABLED || SNMP_V2C_SUPPORT == ENABLED)
   //Sanity check
   if(context != NULL)
   {
      uint_t i;

      //Loop through the list of community strings
      for(i = 0; i < SNMP_AGENT_MAX_COMMUNITIES; i++)
      {
         //Check current status
         if(context->communityTable[i].status == MIB_ROW_STATUS_UNUSED)
         {
            //An unused entry has been found
            entry = &context->communityTable[i];
            //We are done
            break;
         }
      }

      //Check whether the table runs out of space
      if(entry == NULL)
      {
         //Loop through the list of community strings
         for(i = 0; i < SNMP_AGENT_MAX_COMMUNITIES; i++)
         {
            //Check current status
            if(context->communityTable[i].status == MIB_ROW_STATUS_NOT_READY)
            {
               //Reuse the current entry
               entry = &context->communityTable[i];
               //We are done
               break;
            }
         }
      }
   }
#endif

   //Return a pointer to the newly created entry
   return entry;
}


/**
 * @brief Search the community table for a given community string
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] community Pointer to the community string
 * @param[in] length Length of the community string
 * @return Pointer to the matching entry
 **/

SnmpUserEntry *snmpFindCommunityEntry(SnmpAgentContext *context,
   const char_t *community, size_t length)
{
   SnmpUserEntry *entry;

   //Initialize pointer
   entry = NULL;

#if (SNMP_V1_SUPPORT == ENABLED || SNMP_V2C_SUPPORT == ENABLED)
   //Sanity check
   if(context != NULL && community != NULL)
   {
      uint_t i;

      //Loop through the list of community string
      for(i = 0; i < SNMP_AGENT_MAX_COMMUNITIES; i++)
      {
         //Check current status
         if(context->communityTable[i].status != MIB_ROW_STATUS_UNUSED)
         {
            //Check the length of the community string
            if(osStrlen(context->communityTable[i].name) == length)
            {
               //Compare community strings
               if(!osStrncmp(context->communityTable[i].name, community, length))
               {
                  //A matching entry has been found
                  entry = &context->communityTable[i];
                  //We are done
                  break;
               }
            }
         }
      }
   }
#endif

   //Return a pointer to the matching entry
   return entry;
}


/**
 * @brief Parse variable binding
 * @param[in] p Input stream where to read the variable binding
 * @param[in] length Number of bytes available in the input stream
 * @param[out] var Variable binding
 * @param[out] consumed Total number of bytes that have been consumed
 * @return Error code
 **/

error_t snmpParseVarBinding(const uint8_t *p,
   size_t length, SnmpVarBind *var, size_t *consumed)
{
   error_t error;
   Asn1Tag tag;

   //The variable binding is encapsulated within a sequence
   error = asn1ReadTag(p, length, &tag);
   //Failed to decode ASN.1 tag?
   if(error)
      return error;

   //Enforce encoding, class and type
   error = asn1CheckTag(&tag, TRUE, ASN1_CLASS_UNIVERSAL, ASN1_TYPE_SEQUENCE);
   //The tag does not match the criteria?
   if(error)
      return error;

   //Total number of bytes that have been consumed
   *consumed = tag.totalLength;

   //Point to the first item of the sequence
   p = tag.value;
   length = tag.length;

   //Read object name
   error = asn1ReadTag(p, length, &tag);
   //Failed to decode ASN.1 tag?
   if(error)
      return error;

   //Enforce encoding, class and type
   error = asn1CheckTag(&tag, FALSE, ASN1_CLASS_UNIVERSAL, ASN1_TYPE_OBJECT_IDENTIFIER);
   //The tag does not match the criteria?
   if(error)
      return error;

   //Save object identifier
   var->oid = tag.value;
   var->oidLen = tag.length;

   //Point to the next item
   p += tag.totalLength;
   length -= tag.totalLength;

   //Read object value
   error = asn1ReadTag(p, length, &tag);
   //Failed to decode ASN.1 tag?
   if(error)
      return error;

   //Make sure that the tag is valid
   if(tag.constructed)
      return ERROR_INVALID_TAG;

   //Save object class
   var->objClass = tag.objClass;
   //Save object type
   var->objType = tag.objType;
   //Save object value
   var->value = tag.value;
   var->valueLen = tag.length;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Write variable binding
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] var Variable binding
 * @return Error code
 **/

error_t snmpWriteVarBinding(SnmpAgentContext *context, const SnmpVarBind *var)
{
   error_t error;
   size_t m;
   size_t n;
   uint8_t *p;
   Asn1Tag tag;

   //The object's name is encoded in ASN.1 format
   tag.constructed = FALSE;
   tag.objClass = ASN1_CLASS_UNIVERSAL;
   tag.objType = ASN1_TYPE_OBJECT_IDENTIFIER;
   tag.length = var->oidLen;
   tag.value = var->oid;

   //Calculate the total length of the ASN.1 tag
   error = asn1WriteTag(&tag, FALSE, NULL, &m);
   //Any error to report?
   if(error)
      return error;

   //The object's value is encoded in ASN.1 format
   tag.constructed = FALSE;
   tag.objClass = var->objClass;
   tag.objType = var->objType;
   tag.length = var->valueLen;
   tag.value = var->value;

   //Calculate the total length of the ASN.1 tag
   error = asn1WriteTag(&tag, FALSE, NULL, &n);
   //Any error to report?
   if(error)
      return error;

   //The variable binding is encapsulated within a sequence
   tag.constructed = TRUE;
   tag.objClass = ASN1_CLASS_UNIVERSAL;
   tag.objType = ASN1_TYPE_SEQUENCE;
   tag.length = m + n;
   tag.value = NULL;

   //The first pass computes the total length of the sequence
   error = asn1WriteTag(&tag, FALSE, NULL, NULL);
   //Any error to report?
   if(error)
      return error;

   //Make sure the buffer is large enough to hold the whole sequence
   if((context->response.varBindListLen + tag.totalLength) >
      context->response.varBindListMaxLen)
   {
      //Report an error
      return ERROR_BUFFER_OVERFLOW;
   }

   //The second pass encodes the sequence in reverse order
   p = context->response.varBindList + context->response.varBindListLen +
      tag.totalLength;

   //Encode the object's value using ASN.1
   tag.constructed = FALSE;
   tag.objClass = var->objClass;
   tag.objType = var->objType;
   tag.length = var->valueLen;
   tag.value = var->value;

   //Write the corresponding ASN.1 tag
   error = asn1WriteTag(&tag, TRUE, p, &m);
   //Any error to report?
   if(error)
      return error;

   //Move backward
   p -= m;

   //Encode the object's name using ASN.1
   tag.constructed = FALSE;
   tag.objClass = ASN1_CLASS_UNIVERSAL;
   tag.objType = ASN1_TYPE_OBJECT_IDENTIFIER;
   tag.length = var->oidLen;
   tag.value = var->oid;

   //Write the corresponding ASN.1 tag
   error = asn1WriteTag(&tag, TRUE, p, &n);
   //Any error to report?
   if(error)
      return error;

   //Move backward
   p -= n;

   //The variable binding is encapsulated within a sequence
   tag.constructed = TRUE;
   tag.objClass = ASN1_CLASS_UNIVERSAL;
   tag.objType = ASN1_TYPE_SEQUENCE;
   tag.length = m + n;
   tag.value = NULL;

   //Write the corresponding ASN.1 tag
   error = asn1WriteTag(&tag, TRUE, p, NULL);
   //Any error to report?
   if(error)
      return error;

   //Update the length of the list
   context->response.varBindListLen += tag.totalLength;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Copy the list of variable bindings
 * @param[in] context Pointer to the SNMP agent context
 * @return Error code
 **/

error_t snmpCopyVarBindingList(SnmpAgentContext *context)
{
   //Sanity check
   if(context->request.varBindListLen > context->response.varBindListMaxLen)
      return ERROR_BUFFER_OVERFLOW;

   //Copy the list of variable bindings to the response buffer
   osMemcpy(context->response.varBindList, context->request.varBindList,
      context->request.varBindListLen);

   //Save the length of the list
   context->response.varBindListLen = context->request.varBindListLen;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Format the variable binding list for Trap-PDU or SNMPv2-Trap-PDU
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] genericTrapType Generic trap type
 * @param[in] specificTrapCode Specific code
 * @param[in] objectList List of object names
 * @param[in] objectListSize Number of entries in the list
 * @return Error code
 **/

error_t snmpWriteTrapVarBindingList(SnmpAgentContext *context,
   uint_t genericTrapType, uint_t specificTrapCode,
   const SnmpTrapObject *objectList, uint_t objectListSize)
{
   error_t error;
   uint_t i;
   size_t n;
   systime_t time;
   SnmpMessage *message;
   SnmpVarBind var;

   //Point to the SNMP message
   message = &context->response;

#if (SNMP_V2C_SUPPORT == ENABLED || SNMP_V3_SUPPORT == ENABLED)
   //SNMPv2c or SNMPv3 version?
   if(message->version == SNMP_VERSION_2C || message->version == SNMP_VERSION_3)
   {
      //Get current time
      time = osGetSystemTime() / 10;

      //Encode the object value using ASN.1 rules
      error = snmpEncodeUnsignedInt32(time, message->buffer, &n);
      //Any error to report?
      if(error)
         return error;

      //The first two variable bindings in the variable binding list of an
      //SNMPv2-Trap-PDU are sysUpTime.0 and snmpTrapOID.0 respectively
      var.oid = sysUpTimeObject;
      var.oidLen = sizeof(sysUpTimeObject);
      var.objClass = ASN1_CLASS_APPLICATION;
      var.objType = MIB_TYPE_TIME_TICKS;
      var.value = message->buffer;
      var.valueLen = n;

      //Append sysUpTime.0 to the variable binding list
      error = snmpWriteVarBinding(context, &var);
      //Any error to report?
      if(error)
         return error;

      //Generic or enterprise-specific trap?
      if(genericTrapType < SNMP_TRAP_ENTERPRISE_SPECIFIC)
      {
         //Retrieve the length of the snmpTraps OID
         n = sizeof(snmpTrapsObject);
         //Copy the OID
         osMemcpy(message->buffer, snmpTrapsObject, n);

         //For generic traps, the SNMPv2 snmpTrapOID parameter shall be
         //the corresponding trap as defined in section 2 of RFC 3418
         message->buffer[n] = genericTrapType + 1;

         //Update the length of the snmpTrapOID parameter
         n++;
      }
      else
      {
         //Retrieve the length of the enterprise OID
         n = context->enterpriseOidLen;

         //For enterprise specific traps, the SNMPv2 snmpTrapOID parameter shall
         //be the concatenation of the SNMPv1 enterprise OID and two additional
         //sub-identifiers, '0' and the SNMPv1 specific trap parameter. Refer
         //to RFC 3584, section 3.1 and RFC 2578, section 8.5
         osMemcpy(message->buffer, context->enterpriseOid, n);

         //Concatenate the '0' sub-identifier
         message->buffer[n++] = 0;

         //Concatenate the specific trap parameter
         message->buffer[n] = specificTrapCode % 128;

         //Loop as long as necessary
         for(i = 1; specificTrapCode > 128; i++)
         {
            //Split the binary representation into 7 bit chunks
            specificTrapCode /= 128;
            //Make room for the new chunk
            osMemmove(message->buffer + n + 1, message->buffer + n, i);
            //Set the most significant bit in the current chunk
            message->buffer[n] = OID_MORE_FLAG | (specificTrapCode % 128);
         }

         //Update the length of the snmpTrapOID parameter
         n += i;
      }

      //The snmpTrapOID.0 variable occurs as the second variable
      //binding in every SNMPv2-Trap-PDU
      var.oid = snmpTrapOidObject;
      var.oidLen = sizeof(snmpTrapOidObject);
      var.objClass = ASN1_CLASS_UNIVERSAL;
      var.objType = ASN1_TYPE_OBJECT_IDENTIFIER;
      var.value = message->buffer;
      var.valueLen = n;

      //Append snmpTrapOID.0 to the variable binding list
      error = snmpWriteVarBinding(context, &var);
      //Any error to report?
      if(error)
         return error;
   }
#endif

   //Loop through the list of objects
   for(i = 0; i < objectListSize; i++)
   {
      //Get object identifier
      var.oid = objectList[i].oid;
      var.oidLen = objectList[i].oidLen;

      //Retrieve object value
      error = snmpGetObjectValue(context, message, &var);
      //Any error to report?
      if(error)
         return error;

      //Append variable binding to the list
      error = snmpWriteVarBinding(context, &var);
      //Any error to report?
      if(error)
         return error;
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Translate status code
 * @param[in,out] message Pointer to the outgoing SNMP message
 * @param[in] status Status code
 * @param[in] index Index of the variable binding in the list that caused an exception
 * @return error code
 **/

error_t snmpTranslateStatusCode(SnmpMessage *message, error_t status, uint_t index)
{
   //SNMPv1 version?
   if(message->version == SNMP_VERSION_1)
   {
      //Set error-status and error-index fields (refer to RFC 2576, section 4.3)
      switch(status)
      {
      case NO_ERROR:
         //Return noError status code
         message->errorStatus = SNMP_ERROR_NONE;
         message->errorIndex = 0;
         break;

      case ERROR_BUFFER_OVERFLOW:
         //Return tooBig status code
         message->errorStatus = SNMP_ERROR_TOO_BIG;
         message->errorIndex = 0;

         //Total number of SNMP PDUs which were generated by the SNMP protocol
         //entity and for which the value of the error-status field is tooBig
         MIB2_INC_COUNTER32(snmpGroup.snmpOutTooBigs, 1);
         break;

      case ERROR_OBJECT_NOT_FOUND:
      case ERROR_INSTANCE_NOT_FOUND:
      case ERROR_ACCESS_DENIED:
      case ERROR_AUTHORIZATION_FAILED:
         //Return noSuchName status code
         message->errorStatus = SNMP_ERROR_NO_SUCH_NAME;
         message->errorIndex = index;

         //Total number of SNMP PDUs which were generated by the SNMP protocol
         //entity and for which the value of the error-status field is noSuchName
         MIB2_INC_COUNTER32(snmpGroup.snmpOutNoSuchNames, 1);
         break;

      case ERROR_WRONG_TYPE:
      case ERROR_WRONG_LENGTH:
      case ERROR_WRONG_ENCODING:
      case ERROR_WRONG_VALUE:
      case ERROR_INCONSISTENT_VALUE:
         //Return badValue status code
         message->errorStatus = SNMP_ERROR_BAD_VALUE;
         message->errorIndex = index;

         //Total number of SNMP PDUs which were generated by the SNMP protocol
         //entity and for which the value of the error-status field is badValue
         MIB2_INC_COUNTER32(snmpGroup.snmpOutBadValues, 1);
         break;

      case ERROR_READ_FAILED:
      case ERROR_WRITE_FAILED:
      case ERROR_NOT_WRITABLE:
         //Return genError status code
         message->errorStatus = SNMP_ERROR_GENERIC;
         message->errorIndex = index;

         //Total number of SNMP PDUs which were generated by the SNMP protocol
         //entity and for which the value of the error-status field is genError
         MIB2_INC_COUNTER32(snmpGroup.snmpOutGenErrs, 1);
         break;

      default:
         //If the parsing of the request fails, the SNMP agent discards
         //the message and performs no further actions
         return status;
      }
   }
   //SNMPv2c or SNMPv3 version?
   else
   {
      //Set error-status and error-index fields
      switch(status)
      {
      case NO_ERROR:
         //Return noError status code
         message->errorStatus = SNMP_ERROR_NONE;
         message->errorIndex = 0;
         break;

      case ERROR_BUFFER_OVERFLOW:
         //Return tooBig status code
         message->errorStatus = SNMP_ERROR_TOO_BIG;
         message->errorIndex = 0;

         //Total number of SNMP PDUs which were generated by the SNMP protocol
         //entity and for which the value of the error-status field is tooBig
         MIB2_INC_COUNTER32(snmpGroup.snmpOutTooBigs, 1);
         break;

      case ERROR_READ_FAILED:
      case ERROR_WRITE_FAILED:
         //Return genError status code
         message->errorStatus = SNMP_ERROR_GENERIC;
         message->errorIndex = index;

         //Total number of SNMP PDUs which were generated by the SNMP protocol
         //entity and for which the value of the error-status field is genError
         MIB2_INC_COUNTER32(snmpGroup.snmpOutGenErrs, 1);
         break;

      case ERROR_OBJECT_NOT_FOUND:
      case ERROR_INSTANCE_NOT_FOUND:
      case ERROR_ACCESS_DENIED:
         //Return noAccess status code
         message->errorStatus = SNMP_ERROR_NO_ACCESS;
         message->errorIndex = index;
         break;

      case ERROR_WRONG_TYPE:
         //Return wrongType status code
         message->errorStatus = SNMP_ERROR_WRONG_TYPE;
         message->errorIndex = index;
         break;

      case ERROR_WRONG_LENGTH:
         //Return wrongLength status code
         message->errorStatus = SNMP_ERROR_WRONG_LENGTH;
         message->errorIndex = index;
         break;

      case ERROR_WRONG_ENCODING:
         //Return wrongEncoding status code
         message->errorStatus = SNMP_ERROR_WRONG_ENCODING;
         message->errorIndex = index;
         break;

      case ERROR_WRONG_VALUE:
         //Return wrongValue status code
         message->errorStatus = SNMP_ERROR_WRONG_VALUE;
         message->errorIndex = index;
         break;

      case ERROR_INCONSISTENT_VALUE:
         //Return inconsistentValue status code
         message->errorStatus = SNMP_ERROR_INCONSISTENT_VALUE;
         message->errorIndex = index;
         break;

      case ERROR_AUTHORIZATION_FAILED:
         //Return authorizationError status code
         message->errorStatus = SNMP_ERROR_AUTHORIZATION;
         message->errorIndex = 0;
         break;

      case ERROR_NOT_WRITABLE:
         //Return notWritable status code
         message->errorStatus = SNMP_ERROR_NOT_WRITABLE;
         message->errorIndex = index;
         break;

      default:
         //If the parsing of the request fails, the SNMP agent discards
         //the message and performs no further actions
         return status;
      }
   }

   //Successful processing
   return NO_ERROR;
}

#endif
