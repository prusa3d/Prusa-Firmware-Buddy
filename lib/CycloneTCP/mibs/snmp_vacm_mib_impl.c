/**
 * @file snmp_vacm_mib_impl.c
 * @brief SNMP VACM MIB module implementation
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
#include "mibs/mib_common.h"
#include "mibs/snmp_vacm_mib_module.h"
#include "mibs/snmp_vacm_mib_impl.h"
#include "snmp/snmp_agent.h"
#include "snmp/snmp_agent_misc.h"
#include "core/crypto.h"
#include "encoding/asn1.h"
#include "encoding/oid.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (SNMP_VACM_MIB_SUPPORT == ENABLED)


/**
 * @brief SNMP VACM MIB module initialization
 * @return Error code
 **/

error_t snmpVacmMibInit(void)
{
   //Debug message
   TRACE_INFO("Initializing SNMP VACM MIB base...\r\n");

   //Clear SNMP VACM MIB base
   osMemset(&snmpVacmMibBase, 0, sizeof(snmpVacmMibBase));

   //usmUserSpinLock object
   snmpVacmMibBase.vacmViewSpinLock = netGetRandRange(1, INT32_MAX);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Load SNMP VACM MIB module
 * @param[in] context Pointer to the SNMP agent context
 * @return Error code
 **/

error_t snmpVacmMibLoad(void *context)
{
   //Register SNMP agent context
   snmpVacmMibBase.context = context;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Unload SNMP VACM MIB module
 * @param[in] context Pointer to the SNMP agent context
 **/

void snmpVacmMibUnload(void *context)
{
   //Unregister SNMP agent context
   snmpVacmMibBase.context = NULL;
}


/**
 * @brief Lock SNMP VACM MIB base
 **/

void snmpVacmMibLock(void)
{
   //Clear temporary objects
   osMemset(&snmpVacmMibBase.tempGroupEntry, 0, sizeof(SnmpGroupEntry));
   osMemset(&snmpVacmMibBase.tempAccessEntry, 0, sizeof(SnmpAccessEntry));
   osMemset(&snmpVacmMibBase.tempViewEntry, 0, sizeof(SnmpViewEntry));
}


/**
 * @brief Unlock SNMP VACM MIB base
 **/

void snmpVacmMibUnlock(void)
{
   //Clear temporary objects
   osMemset(&snmpVacmMibBase.tempGroupEntry, 0, sizeof(SnmpGroupEntry));
   osMemset(&snmpVacmMibBase.tempAccessEntry, 0, sizeof(SnmpAccessEntry));
   osMemset(&snmpVacmMibBase.tempViewEntry, 0, sizeof(SnmpViewEntry));
}


/**
 * @brief Get vacmContextEntry object value
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier (object name and instance identifier)
 * @param[in] oidLen Length of the OID, in bytes
 * @param[out] value Object value
 * @param[in,out] valueLen Length of the object value, in bytes
 * @return Error code
 **/

error_t snmpVacmMibGetContextEntry(const MibObject *object,
   const uint8_t *oid, size_t oidLen, MibVariant *value, size_t *valueLen)
{
   error_t error;
   size_t n;
   char_t contextName[SNMP_MAX_CONTEXT_NAME_LEN + 1];
   SnmpAgentContext *context;

   //Point to the instance identifier
   n = object->oidLen;

   //vacmContextName is used as instance identifier
   error = mibDecodeString(oid, oidLen, &n, contextName,
      SNMP_MAX_CONTEXT_NAME_LEN, FALSE);
   //Invalid instance identifier?
   if(error)
      return error;

   //Sanity check
   if(n != oidLen)
      return ERROR_INSTANCE_NOT_FOUND;

   //Point to the SNMP agent context
   context = (SnmpAgentContext *) snmpVacmMibBase.context;
   //Sanity check
   if(context == NULL)
      return ERROR_INSTANCE_NOT_FOUND;

   //Check context name
   if(osStrcmp(contextName, context->contextName))
      return ERROR_INSTANCE_NOT_FOUND;

   //vacmContextName object?
   if(!osStrcmp(object->name, "vacmContextName"))
   {
#if (SNMP_V3_SUPPORT == ENABLED)
      //Retrieve the length of the context name
      n = osStrlen(context->contextName);

      //Make sure the buffer is large enough to hold the entire object
      if(*valueLen >= n)
      {
         //Copy object value
         osMemcpy(value->octetString, context->contextName, n);
         //Return object length
         *valueLen = n;
      }
      else
      {
         //Report an error
         error = ERROR_BUFFER_OVERFLOW;
      }
#else
      //The empty contextName (zero length) represents the default context
      *valueLen = 0;
#endif
   }
   //Unknown object?
   else
   {
      //The specified object does not exist
      error = ERROR_OBJECT_NOT_FOUND;
   }

   //Return status code
   return error;
}


/**
 * @brief Get next vacmContextEntry object
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier
 * @param[in] oidLen Length of the OID, in bytes
 * @param[out] nextOid OID of the next object in the MIB
 * @param[out] nextOidLen Length of the next object identifier, in bytes
 * @return Error code
 **/

error_t snmpVacmMibGetNextContextEntry(const MibObject *object,
   const uint8_t *oid, size_t oidLen, uint8_t *nextOid, size_t *nextOidLen)
{
   error_t error;
   size_t n;
   SnmpAgentContext *context;

   //Point to the SNMP agent context
   context = (SnmpAgentContext *) snmpVacmMibBase.context;
   //Sanity check
   if(context == NULL)
      return ERROR_OBJECT_NOT_FOUND;

   //Make sure the buffer is large enough to hold the OID prefix
   if(*nextOidLen < object->oidLen)
      return ERROR_BUFFER_OVERFLOW;

   //Copy OID prefix
   osMemcpy(nextOid, object->oid, object->oidLen);

   //Append the instance identifier to the OID prefix
   n = object->oidLen;

#if (SNMP_V3_SUPPORT == ENABLED)
   //vacmContextName is used as instance identifier
   error = mibEncodeString(nextOid, *nextOidLen, &n,
      context->contextName, FALSE);
#else
   //The empty contextName (zero length) represents the default context
   error = mibEncodeString(nextOid, *nextOidLen, &n, "", FALSE);
#endif

   //Any error to report?
   if(error)
      return error;

   //Check whether the resulting object identifier lexicographically
   //follows the specified OID
   if(oidComp(nextOid, n, oid, oidLen) > 0)
   {
      //Save the length of the resulting object identifier
      *nextOidLen = n;
      //Next object found
      return NO_ERROR;
   }

   //The specified OID does not lexicographically precede the name
   //of some object
   return ERROR_OBJECT_NOT_FOUND;
}


/**
 * @brief Set vacmSecurityToGroupEntry object value
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier (object name and instance identifier)
 * @param[in] oidLen Length of the OID, in bytes
 * @param[in] value Object value
 * @param[in] valueLen Length of the object value, in bytes
 * @param[in] commit This flag tells whether the changes shall be committed
 *   to the MIB base
 * @return Error code
 **/

error_t snmpVacmMibSetSecurityToGroupEntry(const MibObject *object, const uint8_t *oid,
   size_t oidLen, const MibVariant *value, size_t valueLen, bool_t commit)
{
#if (SNMP_VACM_MIB_SET_SUPPORT == ENABLED)
   error_t error;
   size_t n;
   uint_t securityModel;
   char_t securityName[SNMP_MAX_USER_NAME_LEN + 1];
   SnmpAgentContext *context;
   SnmpGroupEntry *entry;

   //Point to the instance identifier
   n = object->oidLen;

   //vacmSecurityModel is used as 1st instance identifier
   error = mibDecodeIndex(oid, oidLen, &n, &securityModel);
   //Invalid instance identifier?
   if(error)
      return error;

   //vacmSecurityName is used as 2nd instance identifier
   error = mibDecodeString(oid, oidLen, &n, securityName,
      SNMP_MAX_USER_NAME_LEN, FALSE);
   //Invalid instance identifier?
   if(error)
      return error;

   //Sanity check
   if(n != oidLen)
      return ERROR_INSTANCE_NOT_FOUND;

   //Make sure the security model is valid
   if(securityModel != SNMP_SECURITY_MODEL_V1 &&
      securityModel != SNMP_SECURITY_MODEL_V2C &&
      securityModel != SNMP_SECURITY_MODEL_USM &&
      securityModel != SNMP_SECURITY_MODEL_TSM)
   {
      //The security model is not supported
      return ERROR_INSTANCE_NOT_FOUND;
   }

   //Make sure the security name is valid
   if(securityName[0] == '\0')
      return ERROR_INSTANCE_NOT_FOUND;

   //Point to the SNMP agent context
   context = (SnmpAgentContext *) snmpVacmMibBase.context;
   //Sanity check
   if(context == NULL)
      return ERROR_INSTANCE_NOT_FOUND;

   //Search the table for a matching row
   entry = snmpFindGroupEntry(context, securityModel, securityName,
      osStrlen(securityName));

   //vacmGroupName object?
   if(!osStrcmp(object->name, "vacmGroupName"))
   {
      //Ensure the length of the group name is valid
      if(valueLen > SNMP_MAX_GROUP_NAME_LEN)
         return ERROR_WRONG_LENGTH;

      //Test if the conceptual row exists in the agent
      if(entry != NULL)
      {
         //Commit phase?
         if(commit)
         {
            //Set group name
            osMemcpy(entry->groupName, value->octetString, valueLen);
            //Properly terminate the string with a NULL character
            entry->groupName[valueLen] = '\0';
         }
      }
      else
      {
         //Prepare phase?
         if(!commit)
         {
            //Save the group name for later use
            osMemcpy(snmpVacmMibBase.tempGroupEntry.groupName,
               value->octetString, valueLen);

            //Properly terminate the string with a NULL character
            snmpVacmMibBase.tempGroupEntry.groupName[valueLen] = '\0';
         }
      }
   }
   //vacmSecurityToGroupStorageType object?
   else if(!osStrcmp(object->name, "vacmSecurityToGroupStorageType"))
   {
      //The vacmSecurityToGroupStorageType object specifies the storage type
      //for this conceptual row
      if(value->integer != MIB_STORAGE_TYPE_OTHER &&
         value->integer != MIB_STORAGE_TYPE_VOLATILE &&
         value->integer != MIB_STORAGE_TYPE_NON_VOLATILE &&
         value->integer != MIB_STORAGE_TYPE_PERMANENT &&
         value->integer != MIB_STORAGE_TYPE_READ_ONLY)
      {
         return ERROR_WRONG_VALUE;
      }
   }
   //vacmSecurityToGroupStatus object?
   else if(!osStrcmp(object->name, "vacmSecurityToGroupStatus"))
   {
      MibRowStatus status;

      //Get row status
      status = (MibRowStatus) value->integer;

      //Check the value specified by the set operation
      if(status == MIB_ROW_STATUS_ACTIVE ||
         status == MIB_ROW_STATUS_NOT_IN_SERVICE)
      {
         //No matching row found?
         if(entry == NULL)
            return ERROR_INCONSISTENT_VALUE;

         //Commit phase?
         if(commit)
         {
            //Valid group name specified?
            if(snmpVacmMibBase.tempGroupEntry.groupName[0] != '\0')
               osStrcpy(entry->groupName, snmpVacmMibBase.tempGroupEntry.groupName);

            //A newly created row cannot be made active until a value has been
            //set for vacmGroupName
            if(entry->groupName[0] == '\0')
               return ERROR_INCONSISTENT_VALUE;

            //Update the status of the conceptual row
            entry->status = status;
         }
      }
      else if(status == MIB_ROW_STATUS_CREATE_AND_GO)
      {
         //Row already instantiated?
         if(entry != NULL)
            return ERROR_INCONSISTENT_VALUE;

         //Create a new row
         entry = snmpCreateGroupEntry(context);
         //Row creation failed?
         if(entry == NULL)
            return ERROR_WRITE_FAILED;

         //Commit phase?
         if(commit)
         {
            //Valid group name specified?
            if(snmpVacmMibBase.tempGroupEntry.groupName[0] != '\0')
            {
               //Save security model
               entry->securityModel = (SnmpSecurityModel) securityModel;
               //Save security name
               osStrcpy(entry->securityName, securityName);
               //Copy the group name
               osStrcpy(entry->groupName, snmpVacmMibBase.tempGroupEntry.groupName);

               //The conceptual row is now available for use by the managed device
               entry->status = MIB_ROW_STATUS_ACTIVE;
            }
            else
            {
               //The newly created row cannot be made active
               return ERROR_INCONSISTENT_VALUE;
            }
         }
      }
      else if(status == MIB_ROW_STATUS_CREATE_AND_WAIT)
      {
         //Row already instantiated?
         if(entry != NULL)
            return ERROR_INCONSISTENT_VALUE;

         //Create a new row
         entry = snmpCreateGroupEntry(context);
         //Row creation failed?
         if(entry == NULL)
            return ERROR_WRITE_FAILED;

         //Commit phase?
         if(commit)
         {
            //Save security model
            entry->securityModel = (SnmpSecurityModel) securityModel;
            //Save security name
            osStrcpy(entry->securityName, securityName);

            //Valid group name specified?
            if(snmpVacmMibBase.tempGroupEntry.groupName[0] != '\0')
            {
               //Copy the group name
               osStrcpy(entry->groupName, snmpVacmMibBase.tempGroupEntry.groupName);

               //Instances of all corresponding columns are now configured
               entry->status = MIB_ROW_STATUS_NOT_IN_SERVICE;
            }
            else
            {
               //Initialize columns with default values
               entry->groupName[0] = '\0';

               //Until instances of all corresponding columns are appropriately
               //configured, the value of the corresponding instance of the
               //vacmSecurityToGroupStatus column is notReady
               entry->status = MIB_ROW_STATUS_NOT_READY;
            }
         }
      }
      else if(status == MIB_ROW_STATUS_DESTROY)
      {
         //Test if the conceptual row exists in the agent
         if(entry != NULL)
         {
            //Commit phase?
            if(commit)
            {
               //Delete the conceptual row from the table
               entry->status = MIB_ROW_STATUS_UNUSED;
            }
         }
      }
      else
      {
         //Unsupported action
         return ERROR_INCONSISTENT_VALUE;
      }
   }
   //Unknown object?
   else
   {
      //The specified object does not exist
      error = ERROR_OBJECT_NOT_FOUND;
   }

   //Return status code
   return error;
#else
   //SET operation is not supported
   return ERROR_WRITE_FAILED;
#endif
}


/**
 * @brief Get vacmSecurityToGroupEntry object value
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier (object name and instance identifier)
 * @param[in] oidLen Length of the OID, in bytes
 * @param[out] value Object value
 * @param[in,out] valueLen Length of the object value, in bytes
 * @return Error code
 **/

error_t snmpVacmMibGetSecurityToGroupEntry(const MibObject *object,
   const uint8_t *oid, size_t oidLen, MibVariant *value, size_t *valueLen)
{
   error_t error;
   size_t n;
   uint_t securityModel;
   char_t securityName[SNMP_MAX_USER_NAME_LEN + 1];
   SnmpAgentContext *context;
   SnmpGroupEntry *entry;

   //Point to the instance identifier
   n = object->oidLen;

   //vacmSecurityModel is used as 1st instance identifier
   error = mibDecodeIndex(oid, oidLen, &n, &securityModel);
   //Invalid instance identifier?
   if(error)
      return error;

   //vacmSecurityName is used as 2nd instance identifier
   error = mibDecodeString(oid, oidLen, &n, securityName,
      SNMP_MAX_USER_NAME_LEN, FALSE);
   //Invalid instance identifier?
   if(error)
      return error;

   //Sanity check
   if(n != oidLen)
      return ERROR_INSTANCE_NOT_FOUND;

   //Point to the SNMP agent context
   context = (SnmpAgentContext *) snmpVacmMibBase.context;
   //Sanity check
   if(context == NULL)
      return ERROR_INSTANCE_NOT_FOUND;

   //Search the table for a matching row
   entry = snmpFindGroupEntry(context, securityModel, securityName,
      osStrlen(securityName));
   //No matching row found?
   if(entry == NULL)
      return ERROR_INSTANCE_NOT_FOUND;

   //vacmGroupName object?
   if(!osStrcmp(object->name, "vacmGroupName"))
   {
      //Retrieve the length of the group name
      n = osStrlen(entry->groupName);

      //Make sure the buffer is large enough to hold the entire object
      if(*valueLen >= n)
      {
         //Copy object value
         osMemcpy(value->octetString, entry->groupName, n);
         //Return object length
         *valueLen = n;
      }
      else
      {
         //Report an error
         error = ERROR_BUFFER_OVERFLOW;
      }
   }
   //vacmSecurityToGroupStorageType object?
   else if(!osStrcmp(object->name, "vacmSecurityToGroupStorageType"))
   {
      //Get the storage type for this conceptual row
      value->integer = MIB_STORAGE_TYPE_VOLATILE;
   }
   //vacmSecurityToGroupStatus object?
   else if(!osStrcmp(object->name, "vacmSecurityToGroupStatus"))
   {
      //Get the status of this conceptual row
      value->integer = entry->status;
   }
   //Unknown object?
   else
   {
      //The specified object does not exist
      error = ERROR_OBJECT_NOT_FOUND;
   }

   //Return status code
   return error;
}


/**
 * @brief Get next vacmSecurityToGroupEntry object
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier
 * @param[in] oidLen Length of the OID, in bytes
 * @param[out] nextOid OID of the next object in the MIB
 * @param[out] nextOidLen Length of the next object identifier, in bytes
 * @return Error code
 **/

error_t snmpVacmMibGetNextSecurityToGroupEntry(const MibObject *object,
   const uint8_t *oid, size_t oidLen, uint8_t *nextOid, size_t *nextOidLen)
{
   error_t error;
   uint_t i;
   size_t n;
   bool_t acceptable;
   SnmpAgentContext *context;
   SnmpGroupEntry *entry;
   SnmpGroupEntry *nextEntry;

   //Initialize variables
   nextEntry = NULL;

   //Point to the SNMP agent context
   context = (SnmpAgentContext *) snmpVacmMibBase.context;
   //Sanity check
   if(context == NULL)
      return ERROR_OBJECT_NOT_FOUND;

   //Make sure the buffer is large enough to hold the OID prefix
   if(*nextOidLen < object->oidLen)
      return ERROR_BUFFER_OVERFLOW;

   //Copy OID prefix
   osMemcpy(nextOid, object->oid, object->oidLen);

   //Loop through the list of groups
   for(i = 0; i < SNMP_AGENT_GROUP_TABLE_SIZE; i++)
   {
      //Point to the current entry
      entry = &context->groupTable[i];

      //Check the status of the row
      if(entry->status != MIB_ROW_STATUS_UNUSED)
      {
         //Append the instance identifier to the OID prefix
         n = object->oidLen;

         //vacmSecurityModel is used as 1st instance identifier
         error = mibEncodeIndex(nextOid, *nextOidLen, &n, entry->securityModel);
         //Any error to report?
         if(error)
            return error;

         //vacmSecurityName is used as 2nd instance identifier
         error = mibEncodeString(nextOid, *nextOidLen, &n, entry->securityName,
            FALSE);
         //Any error to report?
         if(error)
            return error;

         //Check whether the resulting object identifier lexicographically
         //follows the specified OID
         if(oidComp(nextOid, n, oid, oidLen) > 0)
         {
            //Perform lexicographic comparison
            if(nextEntry == NULL)
            {
               acceptable = TRUE;
            }
            else if(entry->securityModel < nextEntry->securityModel)
            {
               acceptable = TRUE;
            }
            else if(entry->securityModel > nextEntry->securityModel)
            {
               acceptable = FALSE;
            }
            else if(osStrlen(entry->securityName) < osStrlen(nextEntry->securityName))
            {
               acceptable = TRUE;
            }
            else if(osStrlen(entry->securityName) > osStrlen(nextEntry->securityName))
            {
               acceptable = FALSE;
            }
            else if(osStrcmp(entry->securityName, nextEntry->securityName) < 0)
            {
               acceptable = TRUE;
            }
            else
            {
               acceptable = FALSE;
            }

            //Save the closest object identifier that follows the specified
            //OID in lexicographic order
            if(acceptable)
               nextEntry = entry;
         }
      }
   }

   //The specified OID does not lexicographically precede the name
   //of some object?
   if(nextEntry == NULL)
      return ERROR_OBJECT_NOT_FOUND;

   //Append the instance identifier to the OID prefix
   n = object->oidLen;

   //vacmSecurityModel is used as 1st instance identifier
   error = mibEncodeIndex(nextOid, *nextOidLen, &n, nextEntry->securityModel);
   //Any error to report?
   if(error)
      return error;

   //vacmSecurityName is used as 2nd instance identifier
   error = mibEncodeString(nextOid, *nextOidLen, &n,
      nextEntry->securityName, FALSE);
   //Any error to report?
   if(error)
      return error;

   //Save the length of the resulting object identifier
   *nextOidLen = n;
   //Next object found
   return NO_ERROR;
}


/**
 * @brief Set vacmAccessEntry object value
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier (object name and instance identifier)
 * @param[in] oidLen Length of the OID, in bytes
 * @param[in] value Object value
 * @param[in] valueLen Length of the object value, in bytes
 * @param[in] commit This flag tells whether the changes shall be committed
 *   to the MIB base
 * @return Error code
 **/

error_t snmpVacmMibSetAccessEntry(const MibObject *object, const uint8_t *oid,
   size_t oidLen, const MibVariant *value, size_t valueLen, bool_t commit)
{
#if (SNMP_VACM_MIB_SET_SUPPORT == ENABLED)
   error_t error;
   size_t n;
   char_t groupName[SNMP_MAX_GROUP_NAME_LEN + 1];
   char_t contextPrefix[SNMP_MAX_CONTEXT_NAME_LEN + 1];
   uint_t securityModel;
   uint_t securityLevel;
   SnmpAgentContext *context;
   SnmpAccessEntry *entry;

   //Point to the instance identifier
   n = object->oidLen;

   //vacmGroupName is used as 1st instance identifier
   error = mibDecodeString(oid, oidLen, &n, groupName,
      SNMP_MAX_GROUP_NAME_LEN, FALSE);
   //Invalid instance identifier?
   if(error)
      return error;

   //vacmAccessContextPrefix is used as 2nd instance identifier
   error = mibDecodeString(oid, oidLen, &n, contextPrefix,
      SNMP_MAX_CONTEXT_NAME_LEN, FALSE);
   //Invalid instance identifier?
   if(error)
      return error;

   //vacmAccessSecurityModel is used as 3rd instance identifier
   error = mibDecodeIndex(oid, oidLen, &n, &securityModel);
   //Invalid instance identifier?
   if(error)
      return error;

   //vacmAccessSecurityLevel is used as 4th instance identifier
   error = mibDecodeIndex(oid, oidLen, &n, &securityLevel);
   //Invalid instance identifier?
   if(error)
      return error;

   //Sanity check
   if(n != oidLen)
      return ERROR_INSTANCE_NOT_FOUND;

   //Make sure the group name is valid
   if(groupName[0] == '\0')
      return ERROR_INSTANCE_NOT_FOUND;

   //Make sure the security model is valid
   if(securityModel != SNMP_SECURITY_MODEL_ANY &&
      securityModel != SNMP_SECURITY_MODEL_V1 &&
      securityModel != SNMP_SECURITY_MODEL_V2C &&
      securityModel != SNMP_SECURITY_MODEL_USM &&
      securityModel != SNMP_SECURITY_MODEL_TSM)
   {
      //The security model is not supported
      return ERROR_INSTANCE_NOT_FOUND;
   }

   //Make sure the security level is valid
   if(securityLevel != SNMP_SECURITY_LEVEL_NO_AUTH_NO_PRIV &&
      securityLevel != SNMP_SECURITY_LEVEL_AUTH_NO_PRIV &&
      securityLevel != SNMP_SECURITY_LEVEL_AUTH_PRIV)
   {
      //The security level is not supported
      return ERROR_INSTANCE_NOT_FOUND;
   }

   //Point to the SNMP agent context
   context = (SnmpAgentContext *) snmpVacmMibBase.context;
   //Sanity check
   if(context == NULL)
      return ERROR_INSTANCE_NOT_FOUND;

   //Search the table for a matching row
   entry = snmpFindAccessEntry(context, groupName, contextPrefix,
      securityModel, securityLevel);

   //vacmAccessContextMatch object?
   if(!osStrcmp(object->name, "vacmAccessContextMatch"))
   {
      //Ensure the value of the object is acceptable
      if(value->integer != SNMP_CONTEXT_MATCH_EXACT &&
         value->integer != SNMP_CONTEXT_MATCH_PREFIX)
      {
         return ERROR_WRONG_VALUE;
      }

      //Test if the conceptual row exists in the agent
      if(entry != NULL)
      {
         //Commit phase?
         if(commit)
         {
            //Set the value of the column
            entry->contextMatch = (SnmpContextMatch) value->integer;
         }
      }
      else
      {
         //Prepare phase?
         if(!commit)
         {
            //Save the value of the column for later use
            snmpVacmMibBase.tempAccessEntry.contextMatch = (SnmpContextMatch) value->integer;
         }
      }
   }
   //vacmAccessReadViewName object?
   else if(!osStrcmp(object->name, "vacmAccessReadViewName"))
   {
      //Ensure the length of the read view name is valid
      if(valueLen > SNMP_MAX_VIEW_NAME_LEN)
         return ERROR_WRONG_LENGTH;

      //Test if the conceptual row exists in the agent
      if(entry != NULL)
      {
         //Commit phase?
         if(commit)
         {
            //Set read view name
            osMemcpy(entry->readViewName, value->octetString, valueLen);
            //Properly terminate the string with a NULL character
            entry->readViewName[valueLen] = '\0';
         }
      }
      else
      {
         //Prepare phase?
         if(!commit)
         {
            //Save the read view name for later use
            osMemcpy(snmpVacmMibBase.tempAccessEntry.readViewName,
               value->octetString, valueLen);

            //Properly terminate the string with a NULL character
            snmpVacmMibBase.tempAccessEntry.readViewName[valueLen] = '\0';
         }
      }
   }
   //vacmAccessWriteViewName object?
   else if(!osStrcmp(object->name, "vacmAccessWriteViewName"))
   {
      //Ensure the length of the write view name is valid
      if(valueLen > SNMP_MAX_VIEW_NAME_LEN)
         return ERROR_WRONG_LENGTH;

      //Test if the conceptual row exists in the agent
      if(entry != NULL)
      {
         //Commit phase?
         if(commit)
         {
            //Set write view name
            osMemcpy(entry->writeViewName, value->octetString, valueLen);
            //Properly terminate the string with a NULL character
            entry->writeViewName[valueLen] = '\0';
         }
      }
      else
      {
         //Prepare phase?
         if(!commit)
         {
            //Save the write view name for later use
            osMemcpy(snmpVacmMibBase.tempAccessEntry.writeViewName,
               value->octetString, valueLen);

            //Properly terminate the string with a NULL character
            snmpVacmMibBase.tempAccessEntry.writeViewName[valueLen] = '\0';
         }
      }
   }
   //vacmAccessNotifyViewName object?
   else if(!osStrcmp(object->name, "vacmAccessNotifyViewName"))
   {
      //Ensure the length of the notify view name is valid
      if(valueLen > SNMP_MAX_VIEW_NAME_LEN)
         return ERROR_WRONG_LENGTH;

      //Test if the conceptual row exists in the agent
      if(entry != NULL)
      {
         //Commit phase?
         if(commit)
         {
            //Set notify view name
            osMemcpy(entry->notifyViewName, value->octetString, valueLen);
            //Properly terminate the string with a NULL character
            entry->notifyViewName[valueLen] = '\0';
         }
      }
      else
      {
         //Prepare phase?
         if(!commit)
         {
            //Save the notify view name for later use
            osMemcpy(snmpVacmMibBase.tempAccessEntry.notifyViewName,
               value->octetString, valueLen);

            //Properly terminate the string with a NULL character
            snmpVacmMibBase.tempAccessEntry.notifyViewName[valueLen] = '\0';
         }
      }
   }
   //vacmAccessStorageType object?
   else if(!osStrcmp(object->name, "vacmAccessStorageType"))
   {
      //The vacmAccessStorageType object specifies the storage type
      //for this conceptual row
      if(value->integer != MIB_STORAGE_TYPE_OTHER &&
         value->integer != MIB_STORAGE_TYPE_VOLATILE &&
         value->integer != MIB_STORAGE_TYPE_NON_VOLATILE &&
         value->integer != MIB_STORAGE_TYPE_PERMANENT &&
         value->integer != MIB_STORAGE_TYPE_READ_ONLY)
      {
         return ERROR_WRONG_VALUE;
      }
   }
   //vacmAccessStatus object?
   else if(!osStrcmp(object->name, "vacmAccessStatus"))
   {
      MibRowStatus status;

      //Get row status
      status = (MibRowStatus) value->integer;

      //Check the value specified by the set operation
      if(status == MIB_ROW_STATUS_ACTIVE ||
         status == MIB_ROW_STATUS_NOT_IN_SERVICE)
      {
         //No matching row found?
         if(entry == NULL)
            return ERROR_INCONSISTENT_VALUE;

         //Commit phase?
         if(commit)
         {
            //Update the status of the conceptual row
            entry->status = status;
         }
      }
      else if(status == MIB_ROW_STATUS_CREATE_AND_GO ||
         status == MIB_ROW_STATUS_CREATE_AND_WAIT)
      {
         //Row already instantiated?
         if(entry != NULL)
            return ERROR_INCONSISTENT_VALUE;

         //Create a new row
         entry = snmpCreateAccessEntry(context);
         //Row creation failed?
         if(entry == NULL)
            return ERROR_WRITE_FAILED;

         //Commit phase?
         if(commit)
         {
            //Save group name
            osStrcpy(entry->groupName, groupName);
            //Save context name prefix
            osStrcpy(entry->contextPrefix, contextPrefix);
            //Save security model
            entry->securityModel = (SnmpSecurityModel) securityModel;
            //Save security level
            entry->securityLevel = (SnmpSecurityLevel) securityLevel;

            //Initialize columns with default values
            entry->contextMatch = SNMP_CONTEXT_MATCH_EXACT;
            entry->readViewName[0] = '\0';
            entry->writeViewName[0] = '\0';
            entry->notifyViewName[0] = '\0';

            //Valid context match specified?
            if(snmpVacmMibBase.tempAccessEntry.contextMatch != SNMP_CONTEXT_MATCH_INVALID)
               entry->contextMatch = snmpVacmMibBase.tempAccessEntry.contextMatch;

            //Valid read view name specified?
            if(snmpVacmMibBase.tempAccessEntry.readViewName[0] != '\0')
               osStrcpy(entry->readViewName, snmpVacmMibBase.tempAccessEntry.readViewName);

            //Valid write view name specified?
            if(snmpVacmMibBase.tempAccessEntry.writeViewName[0] != '\0')
               osStrcpy(entry->writeViewName, snmpVacmMibBase.tempAccessEntry.writeViewName);

            //Valid notify notify name specified?
            if(snmpVacmMibBase.tempAccessEntry.notifyViewName[0] != '\0')
               osStrcpy(entry->notifyViewName, snmpVacmMibBase.tempAccessEntry.notifyViewName);

            //The conceptual row has been successfully created
            if(status == MIB_ROW_STATUS_CREATE_AND_GO)
            {
               entry->status = MIB_ROW_STATUS_ACTIVE;
            }
            else
            {
               entry->status = MIB_ROW_STATUS_NOT_IN_SERVICE;
            }
         }
      }
      else if(status == MIB_ROW_STATUS_DESTROY)
      {
         //Test if the conceptual row exists in the agent
         if(entry != NULL)
         {
            //Commit phase?
            if(commit)
            {
               //Delete the conceptual row from the table
               entry->status = MIB_ROW_STATUS_UNUSED;
            }
         }
      }
      else
      {
         //Unsupported action
         return ERROR_INCONSISTENT_VALUE;
      }
   }
   //Unknown object?
   else
   {
      //The specified object does not exist
      return ERROR_OBJECT_NOT_FOUND;
   }

   //Successful processing
   return NO_ERROR;
#else
   //SET operation is not supported
   return ERROR_WRITE_FAILED;
#endif
}


/**
 * @brief Get vacmAccessEntry object value
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier (object name and instance identifier)
 * @param[in] oidLen Length of the OID, in bytes
 * @param[out] value Object value
 * @param[in,out] valueLen Length of the object value, in bytes
 * @return Error code
 **/

error_t snmpVacmMibGetAccessEntry(const MibObject *object,
   const uint8_t *oid, size_t oidLen, MibVariant *value, size_t *valueLen)
{
   error_t error;
   size_t n;
   char_t groupName[SNMP_MAX_GROUP_NAME_LEN + 1];
   char_t contextPrefix[SNMP_MAX_CONTEXT_NAME_LEN + 1];
   uint_t securityModel;
   uint_t securityLevel;
   SnmpAgentContext *context;
   SnmpAccessEntry *entry;

   //Point to the instance identifier
   n = object->oidLen;

   //vacmGroupName is used as 1st instance identifier
   error = mibDecodeString(oid, oidLen, &n, groupName,
      SNMP_MAX_GROUP_NAME_LEN, FALSE);
   //Invalid instance identifier?
   if(error)
      return error;

   //vacmAccessContextPrefix is used as 2nd instance identifier
   error = mibDecodeString(oid, oidLen, &n, contextPrefix,
      SNMP_MAX_CONTEXT_NAME_LEN, FALSE);
   //Invalid instance identifier?
   if(error)
      return error;

   //vacmAccessSecurityModel is used as 3rd instance identifier
   error = mibDecodeIndex(oid, oidLen, &n, &securityModel);
   //Invalid instance identifier?
   if(error)
      return error;

   //vacmAccessSecurityLevel is used as 4th instance identifier
   error = mibDecodeIndex(oid, oidLen, &n, &securityLevel);
   //Invalid instance identifier?
   if(error)
      return error;

   //Sanity check
   if(n != oidLen)
      return ERROR_INSTANCE_NOT_FOUND;

   //Point to the SNMP agent context
   context = (SnmpAgentContext *) snmpVacmMibBase.context;
   //Sanity check
   if(context == NULL)
      return ERROR_INSTANCE_NOT_FOUND;

   //Search the table for a matching row
   entry = snmpFindAccessEntry(context, groupName, contextPrefix,
      securityModel, securityLevel);
   //No matching row found?
   if(entry == NULL)
      return ERROR_INSTANCE_NOT_FOUND;

   //vacmAccessContextMatch object?
   if(!osStrcmp(object->name, "vacmAccessContextMatch"))
   {
      //Get object value
      value->integer = entry->contextMatch;
   }
   //vacmAccessReadViewName object?
   else if(!osStrcmp(object->name, "vacmAccessReadViewName"))
   {
      //Retrieve the length of the read view name
      n = osStrlen(entry->readViewName);

      //Make sure the buffer is large enough to hold the entire object
      if(*valueLen >= n)
      {
         //Copy object value
         osMemcpy(value->octetString, entry->readViewName, n);
         //Return object length
         *valueLen = n;
      }
      else
      {
         //Report an error
         error = ERROR_BUFFER_OVERFLOW;
      }
   }
   //vacmAccessWriteViewName object?
   else if(!osStrcmp(object->name, "vacmAccessWriteViewName"))
   {
      //Retrieve the length of the write view name
      n = osStrlen(entry->writeViewName);

      //Make sure the buffer is large enough to hold the entire object
      if(*valueLen >= n)
      {
         //Copy object value
         osMemcpy(value->octetString, entry->writeViewName, n);
         //Return object length
         *valueLen = n;
      }
      else
      {
         //Report an error
         error = ERROR_BUFFER_OVERFLOW;
      }
   }
   //vacmAccessNotifyViewName object?
   else if(!osStrcmp(object->name, "vacmAccessNotifyViewName"))
   {
      //Retrieve the length of the notify view name
      n = osStrlen(entry->notifyViewName);

      //Make sure the buffer is large enough to hold the entire object
      if(*valueLen >= n)
      {
         //Copy object value
         osMemcpy(value->octetString, entry->notifyViewName, n);
         //Return object length
         *valueLen = n;
      }
      else
      {
         //Report an error
         error = ERROR_BUFFER_OVERFLOW;
      }
   }
   //vacmAccessStorageType object?
   else if(!osStrcmp(object->name, "vacmAccessStorageType"))
   {
      //Get the storage type for this conceptual row
      value->integer = MIB_STORAGE_TYPE_VOLATILE;
   }
   //vacmAccessStatus object?
   else if(!osStrcmp(object->name, "vacmAccessStatus"))
   {
      //Get the status of this conceptual row
      value->integer = entry->status;
   }
   //Unknown object?
   else
   {
      //The specified object does not exist
      error = ERROR_OBJECT_NOT_FOUND;
   }

   //Return status code
   return error;
}


/**
 * @brief Get next vacmAccessEntry object
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier
 * @param[in] oidLen Length of the OID, in bytes
 * @param[out] nextOid OID of the next object in the MIB
 * @param[out] nextOidLen Length of the next object identifier, in bytes
 * @return Error code
 **/

error_t snmpVacmMibGetNextAccessEntry(const MibObject *object,
   const uint8_t *oid, size_t oidLen, uint8_t *nextOid, size_t *nextOidLen)
{
   error_t error;
   uint_t i;
   size_t n;
   bool_t acceptable;
   SnmpAgentContext *context;
   SnmpAccessEntry *entry;
   SnmpAccessEntry *nextEntry;

   //Initialize variables
   nextEntry = NULL;

   //Point to the SNMP agent context
   context = (SnmpAgentContext *) snmpVacmMibBase.context;
   //Sanity check
   if(context == NULL)
      return ERROR_OBJECT_NOT_FOUND;

   //Make sure the buffer is large enough to hold the OID prefix
   if(*nextOidLen < object->oidLen)
      return ERROR_BUFFER_OVERFLOW;

   //Copy OID prefix
   osMemcpy(nextOid, object->oid, object->oidLen);

   //Loop through the list of access rights
   for(i = 0; i < SNMP_AGENT_ACCESS_TABLE_SIZE; i++)
   {
      //Point to the current entry
      entry = &context->accessTable[i];

      //Check the status of the row
      if(entry->status != MIB_ROW_STATUS_UNUSED)
      {
         //Append the instance identifier to the OID prefix
         n = object->oidLen;

         //vacmGroupName is used as 1st instance identifier
         error = mibEncodeString(nextOid, *nextOidLen, &n,
            entry->groupName, FALSE);
         //Invalid instance identifier?
         if(error)
            return error;

         //vacmAccessContextPrefix is used as 2nd instance identifier
         error = mibEncodeString(nextOid, *nextOidLen, &n,
            entry->contextPrefix, FALSE);
         //Invalid instance identifier?
         if(error)
            return error;

         //vacmAccessSecurityModel is used as 3rd instance identifier
         error = mibEncodeIndex(nextOid, *nextOidLen, &n,
            entry->securityModel);
         //Invalid instance identifier?
         if(error)
            return error;

         //vacmAccessSecurityLevel is used as 4th instance identifier
         error = mibEncodeIndex(nextOid, *nextOidLen, &n,
            entry->securityLevel);
         //Invalid instance identifier?
         if(error)
            return error;

         //Check whether the resulting object identifier lexicographically
         //follows the specified OID
         if(oidComp(nextOid, n, oid, oidLen) > 0)
         {
            //Perform lexicographic comparison
            if(nextEntry == NULL)
            {
               acceptable = TRUE;
            }
            else if(osStrlen(entry->groupName) < osStrlen(nextEntry->groupName))
            {
               acceptable = TRUE;
            }
            else if(osStrlen(entry->groupName) > osStrlen(nextEntry->groupName))
            {
               acceptable = FALSE;
            }
            else if(osStrcmp(entry->groupName, nextEntry->groupName) < 0)
            {
               acceptable = TRUE;
            }
            else if(osStrcmp(entry->groupName, nextEntry->groupName) > 0)
            {
               acceptable = FALSE;
            }
            else if(osStrlen(entry->contextPrefix) < osStrlen(nextEntry->contextPrefix))
            {
               acceptable = TRUE;
            }
            else if(osStrlen(entry->contextPrefix) > osStrlen(nextEntry->contextPrefix))
            {
               acceptable = FALSE;
            }
            else if(osStrcmp(entry->contextPrefix, nextEntry->contextPrefix) < 0)
            {
               acceptable = TRUE;
            }
            else if(osStrcmp(entry->contextPrefix, nextEntry->contextPrefix) > 0)
            {
               acceptable = FALSE;
            }
            else if(entry->securityModel < nextEntry->securityModel)
            {
               acceptable = TRUE;
            }
            else if(entry->securityModel > nextEntry->securityModel)
            {
               acceptable = FALSE;
            }
            else if(entry->securityLevel < nextEntry->securityLevel)
            {
               acceptable = TRUE;
            }
            else
            {
               acceptable = FALSE;
            }

            //Save the closest object identifier that follows the specified
            //OID in lexicographic order
            if(acceptable)
               nextEntry = entry;
         }
      }
   }

   //The specified OID does not lexicographically precede the name
   //of some object?
   if(nextEntry == NULL)
      return ERROR_OBJECT_NOT_FOUND;

   //Append the instance identifier to the OID prefix
   n = object->oidLen;

   //vacmGroupName is used as 1st instance identifier
   error = mibEncodeString(nextOid, *nextOidLen, &n,
      nextEntry->groupName, FALSE);
   //Invalid instance identifier?
   if(error)
      return error;

   //vacmAccessContextPrefix is used as 2nd instance identifier
   error = mibEncodeString(nextOid, *nextOidLen, &n,
      nextEntry->contextPrefix, FALSE);
   //Invalid instance identifier?
   if(error)
      return error;

   //vacmAccessSecurityModel is used as 3rd instance identifier
   error = mibEncodeIndex(nextOid, *nextOidLen, &n, nextEntry->securityModel);
   //Invalid instance identifier?
   if(error)
      return error;

   //vacmAccessSecurityLevel is used as 4th instance identifier
   error = mibEncodeIndex(nextOid, *nextOidLen, &n, nextEntry->securityLevel);
   //Invalid instance identifier?
   if(error)
      return error;

   //Save the length of the resulting object identifier
   *nextOidLen = n;
   //Next object found
   return NO_ERROR;
}


/**
 * @brief Set vacmViewSpinLock object value
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier (object name and instance identifier)
 * @param[in] oidLen Length of the OID, in bytes
 * @param[in] value Object value
 * @param[in] valueLen Length of the object value, in bytes
 * @param[in] commit This flag tells whether the changes shall be committed
 *   to the MIB base
 * @return Error code
 **/

error_t snmpVacmMibSetViewSpinLock(const MibObject *object, const uint8_t *oid,
   size_t oidLen, const MibVariant *value, size_t valueLen, bool_t commit)
{
   //Test and increment spin lock
   return mibTestAndIncSpinLock(&snmpVacmMibBase.vacmViewSpinLock,
      value->integer, commit);
}


/**
 * @brief Get vacmViewSpinLock object value
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier (object name and instance identifier)
 * @param[in] oidLen Length of the OID, in bytes
 * @param[out] value Object value
 * @param[in,out] valueLen Length of the object value, in bytes
 * @return Error code
 **/

error_t snmpVacmMibGetViewSpinLock(const MibObject *object,
   const uint8_t *oid, size_t oidLen, MibVariant *value, size_t *valueLen)
{
   //Get the current value of the spin lock
   value->integer = snmpVacmMibBase.vacmViewSpinLock;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set vacmViewTreeFamilyEntry object value
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier (object name and instance identifier)
 * @param[in] oidLen Length of the OID, in bytes
 * @param[in] value Object value
 * @param[in] valueLen Length of the object value, in bytes
 * @param[in] commit This flag tells whether the changes shall be committed
 *   to the MIB base
 * @return Error code
 **/

error_t snmpVacmMibSetViewTreeFamilyEntry(const MibObject *object, const uint8_t *oid,
   size_t oidLen, const MibVariant *value, size_t valueLen, bool_t commit)
{
#if (SNMP_VACM_MIB_SET_SUPPORT == ENABLED)
   error_t error;
   size_t n;
   char_t viewName[SNMP_MAX_VIEW_NAME_LEN + 1];
   uint8_t subtree[SNMP_MAX_OID_SIZE];
   size_t subtreeLen;
   SnmpAgentContext *context;
   SnmpViewEntry *entry;

   //Point to the instance identifier
   n = object->oidLen;

   //vacmViewTreeFamilyViewName is used as 1st instance identifier
   error = mibDecodeString(oid, oidLen, &n, viewName,
      SNMP_MAX_VIEW_NAME_LEN, FALSE);
   //Invalid instance identifier?
   if(error)
      return error;

   //vacmViewTreeFamilySubtree is used as 2nd instance identifier
   error = mibDecodeObjectIdentifier(oid, oidLen, &n, subtree,
      SNMP_MAX_OID_SIZE, &subtreeLen, FALSE);
   //Invalid instance identifier?
   if(error)
      return error;

   //Sanity check
   if(n != oidLen)
      return ERROR_INSTANCE_NOT_FOUND;

   //Make sure the view name is valid
   if(viewName[0] == '\0')
      return ERROR_INSTANCE_NOT_FOUND;

   //Point to the SNMP agent context
   context = (SnmpAgentContext *) snmpVacmMibBase.context;
   //Sanity check
   if(context == NULL)
      return ERROR_INSTANCE_NOT_FOUND;

   //Search the table for a matching row
   entry = snmpFindViewEntry(context, viewName, subtree, subtreeLen);

   //vacmViewTreeFamilyMask object?
   if(!osStrcmp(object->name, "vacmViewTreeFamilyMask"))
   {
      //Ensure the length of the bit mask is valid
      if(valueLen > SNMP_MAX_BIT_MASK_SIZE)
         return ERROR_WRONG_LENGTH;

      //Test if the conceptual row exists in the agent
      if(entry != NULL)
      {
         //Commit phase?
         if(commit)
         {
            //Set the bit mask
            osMemcpy(entry->mask, value->octetString, valueLen);
            //Set the length of the bit mask
            entry->maskLen = valueLen;
         }
      }
      else
      {
         //Prepare phase?
         if(!commit)
         {
            //Save the bit mask for later use
            osMemcpy(snmpVacmMibBase.tempViewEntry.mask, value->octetString, valueLen);
            //Save the length of the bit mask
            snmpVacmMibBase.tempViewEntry.maskLen = valueLen;
         }
      }
   }
   //vacmViewTreeFamilyType object?
   else if(!osStrcmp(object->name, "vacmViewTreeFamilyType"))
   {
      //Ensure the value of the object is acceptable
      if(value->integer != SNMP_VIEW_TYPE_INCLUDED &&
         value->integer != SNMP_VIEW_TYPE_EXCLUDED)
      {
         return ERROR_WRONG_VALUE;
      }

      //Test if the conceptual row exists in the agent
      if(entry != NULL)
      {
         //Commit phase?
         if(commit)
         {
            //Set the value of the column
            entry->type = (SnmpViewType) value->integer;
         }
      }
      else
      {
         //Prepare phase?
         if(!commit)
         {
            //Save the value of the column for later use
            snmpVacmMibBase.tempViewEntry.type = (SnmpViewType) value->integer;
         }
      }
   }
   //vacmViewTreeFamilyStorageType object?
   else if(!osStrcmp(object->name, "vacmViewTreeFamilyStorageType"))
   {
      //The vacmViewTreeFamilyStorageType object specifies the storage type
      //for this conceptual row
      if(value->integer != MIB_STORAGE_TYPE_OTHER &&
         value->integer != MIB_STORAGE_TYPE_VOLATILE &&
         value->integer != MIB_STORAGE_TYPE_NON_VOLATILE &&
         value->integer != MIB_STORAGE_TYPE_PERMANENT &&
         value->integer != MIB_STORAGE_TYPE_READ_ONLY)
      {
         return ERROR_WRONG_VALUE;
      }
   }
   //vacmViewTreeFamilyStatus object?
   else if(!osStrcmp(object->name, "vacmViewTreeFamilyStatus"))
   {
      MibRowStatus status;

      //Get row status
      status = (MibRowStatus) value->integer;

      //Check the value specified by the set operation
      if(status == MIB_ROW_STATUS_ACTIVE ||
         status == MIB_ROW_STATUS_NOT_IN_SERVICE)
      {
         //No matching row found?
         if(entry == NULL)
            return ERROR_INCONSISTENT_VALUE;

         //Commit phase?
         if(commit)
         {
            //Update the status of the conceptual row
            entry->status = status;
         }
      }
      else if(status == MIB_ROW_STATUS_CREATE_AND_GO ||
         status == MIB_ROW_STATUS_CREATE_AND_WAIT)
      {
         //Row already instantiated?
         if(entry != NULL)
            return ERROR_INCONSISTENT_VALUE;

         //Create a new row
         entry = snmpCreateViewEntry(context);
         //Row creation failed?
         if(entry == NULL)
            return ERROR_WRITE_FAILED;

         //Commit phase?
         if(commit)
         {
            //Save view name
            osStrcpy(entry->viewName, viewName);
            //Save subtree
            osMemcpy(entry->subtree, subtree, subtreeLen);
            //Save the length of the subtree
            entry->subtreeLen = subtreeLen;

            //Initialize columns with default values
            entry->maskLen = 0;
            entry->type = SNMP_VIEW_TYPE_INCLUDED;

            //Valid bit mask specified?
            if(snmpVacmMibBase.tempViewEntry.maskLen != 0)
            {
               //Copy the bit mask
               osMemcpy(entry->mask, snmpVacmMibBase.tempViewEntry.mask,
                  snmpVacmMibBase.tempViewEntry.maskLen);

               //Set the length of the bit mask
               entry->maskLen = snmpVacmMibBase.tempViewEntry.maskLen;
            }

            //Valid type specified?
            if(snmpVacmMibBase.tempViewEntry.type != SNMP_VIEW_TYPE_INVALID)
               entry->type = snmpVacmMibBase.tempViewEntry.type;

            //The conceptual row has been successfully created
            if(status == MIB_ROW_STATUS_CREATE_AND_GO)
            {
               entry->status = MIB_ROW_STATUS_ACTIVE;
            }
            else
            {
               entry->status = MIB_ROW_STATUS_NOT_IN_SERVICE;
            }
         }
      }
      else if(status == MIB_ROW_STATUS_DESTROY)
      {
         //Test if the conceptual row exists in the agent
         if(entry != NULL)
         {
            //Commit phase?
            if(commit)
            {
               //Delete the conceptual row from the table
               entry->status = MIB_ROW_STATUS_UNUSED;
            }
         }
      }
      else
      {
         //Unsupported action
         return ERROR_INCONSISTENT_VALUE;
      }
   }
   //Unknown object?
   else
   {
      //The specified object does not exist
      return ERROR_OBJECT_NOT_FOUND;
   }

   //Successful processing
   return NO_ERROR;
#else
   //SET operation is not supported
   return ERROR_WRITE_FAILED;
#endif
}


/**
 * @brief Get vacmViewTreeFamilyEntry object value
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier (object name and instance identifier)
 * @param[in] oidLen Length of the OID, in bytes
 * @param[out] value Object value
 * @param[in,out] valueLen Length of the object value, in bytes
 * @return Error code
 **/

error_t snmpVacmMibGetViewTreeFamilyEntry(const MibObject *object,
   const uint8_t *oid, size_t oidLen, MibVariant *value, size_t *valueLen)
{
   error_t error;
   size_t n;
   char_t viewName[SNMP_MAX_VIEW_NAME_LEN + 1];
   uint8_t subtree[SNMP_MAX_OID_SIZE];
   size_t subtreeLen;
   SnmpAgentContext *context;
   SnmpViewEntry *entry;

   //Point to the instance identifier
   n = object->oidLen;

   //vacmViewTreeFamilyViewName is used as 1st instance identifier
   error = mibDecodeString(oid, oidLen, &n, viewName,
      SNMP_MAX_VIEW_NAME_LEN, FALSE);
   //Invalid instance identifier?
   if(error)
      return error;

   //vacmViewTreeFamilySubtree is used as 2nd instance identifier
   error = mibDecodeObjectIdentifier(oid, oidLen, &n, subtree,
      SNMP_MAX_OID_SIZE, &subtreeLen, FALSE);
   //Invalid instance identifier?
   if(error)
      return error;

   //Sanity check
   if(n != oidLen)
      return ERROR_INSTANCE_NOT_FOUND;

   //Point to the SNMP agent context
   context = (SnmpAgentContext *) snmpVacmMibBase.context;
   //Sanity check
   if(context == NULL)
      return ERROR_INSTANCE_NOT_FOUND;

   //Search the table for a matching row
   entry = snmpFindViewEntry(context, viewName, subtree, subtreeLen);
   //No matching row found?
   if(entry == NULL)
      return ERROR_INSTANCE_NOT_FOUND;

   //vacmViewTreeFamilyMask object?
   if(!osStrcmp(object->name, "vacmViewTreeFamilyMask"))
   {
      //Make sure the buffer is large enough to hold the entire object
      if(*valueLen >= entry->maskLen)
      {
         //Copy object value
         osMemcpy(value->octetString, entry->mask, entry->maskLen);
         //Return object length
         *valueLen = entry->maskLen;
      }
      else
      {
         //Report an error
         error = ERROR_BUFFER_OVERFLOW;
      }
   }
   //vacmViewTreeFamilyType object?
   else if(!osStrcmp(object->name, "vacmViewTreeFamilyType"))
   {
      //This object indicates whether the corresponding instances of
      //vacmViewTreeFamilySubtree and vacmViewTreeFamilyMask define a family
      //of view subtrees which is included in or excluded from the MIB view
      value->integer = entry->type;
   }
   //vacmViewTreeFamilyStorageType object?
   else if(!osStrcmp(object->name, "vacmViewTreeFamilyStorageType"))
   {
      //Get the storage type for this conceptual row
      value->integer = MIB_STORAGE_TYPE_VOLATILE;
   }
   //vacmViewTreeFamilyStatus object?
   else if(!osStrcmp(object->name, "vacmViewTreeFamilyStatus"))
   {
      //Get the status of this conceptual row
      value->integer = entry->status;
   }
   //Unknown object?
   else
   {
      //The specified object does not exist
      error = ERROR_OBJECT_NOT_FOUND;
   }

   //Return status code
   return error;
}


/**
 * @brief Get next vacmViewTreeFamilyEntry object
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier
 * @param[in] oidLen Length of the OID, in bytes
 * @param[out] nextOid OID of the next object in the MIB
 * @param[out] nextOidLen Length of the next object identifier, in bytes
 * @return Error code
 **/

error_t snmpVacmMibGetNextViewTreeFamilyEntry(const MibObject *object,
   const uint8_t *oid, size_t oidLen, uint8_t *nextOid, size_t *nextOidLen)
{
   error_t error;
   uint_t i;
   size_t n;
   bool_t acceptable;
   SnmpAgentContext *context;
   SnmpViewEntry *entry;
   SnmpViewEntry *nextEntry;

   //Initialize variables
   nextEntry = NULL;

   //Point to the SNMP agent context
   context = (SnmpAgentContext *) snmpVacmMibBase.context;
   //Sanity check
   if(context == NULL)
      return ERROR_OBJECT_NOT_FOUND;

   //Make sure the buffer is large enough to hold the OID prefix
   if(*nextOidLen < object->oidLen)
      return ERROR_BUFFER_OVERFLOW;

   //Copy OID prefix
   osMemcpy(nextOid, object->oid, object->oidLen);

   //Loop through the list of MIB views
   for(i = 0; i < SNMP_AGENT_VIEW_TABLE_SIZE; i++)
   {
      //Point to the current entry
      entry = &context->viewTable[i];

      //Check the status of the row
      if(entry->status != MIB_ROW_STATUS_UNUSED)
      {
         //Append the instance identifier to the OID prefix
         n = object->oidLen;

         //vacmViewTreeFamilyViewName is used as 1st instance identifier
         error = mibEncodeString(nextOid, *nextOidLen, &n,
            entry->viewName, FALSE);
         //Invalid instance identifier?
         if(error)
            return error;

         //vacmViewTreeFamilySubtree is used as 2nd instance identifier
         error = mibEncodeObjectIdentifier(nextOid, *nextOidLen, &n,
            entry->subtree, entry->subtreeLen, FALSE);
         //Invalid instance identifier?
         if(error)
            return error;

         //Check whether the resulting object identifier lexicographically
         //follows the specified OID
         if(oidComp(nextOid, n, oid, oidLen) > 0)
         {
            //Perform lexicographic comparison
            if(nextEntry == NULL)
            {
               acceptable = TRUE;
            }
            else if(osStrlen(entry->viewName) < osStrlen(nextEntry->viewName))
            {
               acceptable = TRUE;
            }
            else if(osStrlen(entry->viewName) > osStrlen(nextEntry->viewName))
            {
               acceptable = FALSE;
            }
            else if(osStrcmp(entry->viewName, nextEntry->viewName) < 0)
            {
               acceptable = TRUE;
            }
            else if(osStrcmp(entry->viewName, nextEntry->viewName) > 0)
            {
               acceptable = FALSE;
            }
            else if(entry->subtreeLen < nextEntry->subtreeLen)
            {
               acceptable = TRUE;
            }
            else if(entry->subtreeLen > nextEntry->subtreeLen)
            {
               acceptable = FALSE;
            }
            else if(osMemcmp(entry->subtree, nextEntry->subtree, nextEntry->subtreeLen) < 0)
            {
               acceptable = TRUE;
            }
            else
            {
               acceptable = FALSE;
            }

            //Save the closest object identifier that follows the specified
            //OID in lexicographic order
            if(acceptable)
               nextEntry = entry;
         }
      }
   }

   //The specified OID does not lexicographically precede the name
   //of some object?
   if(nextEntry == NULL)
      return ERROR_OBJECT_NOT_FOUND;

   //Append the instance identifier to the OID prefix
   n = object->oidLen;

   //vacmViewTreeFamilyViewName is used as 1st instance identifier
   error = mibEncodeString(nextOid, *nextOidLen, &n,
      nextEntry->viewName, FALSE);
   //Invalid instance identifier?
   if(error)
      return error;

   //vacmViewTreeFamilySubtree is used as 2nd instance identifier
   error = mibEncodeObjectIdentifier(nextOid, *nextOidLen, &n,
      nextEntry->subtree, nextEntry->subtreeLen, FALSE);
   //Invalid instance identifier?
   if(error)
      return error;

   //Save the length of the resulting object identifier
   *nextOidLen = n;
   //Next object found
   return NO_ERROR;
}

#endif
