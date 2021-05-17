/**
 * @file snmp_community_mib_impl.c
 * @brief SNMP COMMUNITY MIB module implementation
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

//Switch to the appropriate trace level
#define TRACE_LEVEL SNMP_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "mibs/mib_common.h"
#include "mibs/snmp_community_mib_module.h"
#include "mibs/snmp_community_mib_impl.h"
#include "snmp/snmp_agent.h"
#include "snmp/snmp_agent_misc.h"
#include "core/crypto.h"
#include "encoding/asn1.h"
#include "encoding/oid.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (SNMP_COMMUNITY_MIB_SUPPORT == ENABLED)


/**
 * @brief SNMP COMMUNITY MIB module initialization
 * @return Error code
 **/

error_t snmpCommunityMibInit(void)
{
   //Debug message
   TRACE_INFO("Initializing SNMP COMMUNITY MIB base...\r\n");

   //Clear SNMP COMMUNITY MIB base
   osMemset(&snmpCommunityMibBase, 0, sizeof(snmpCommunityMibBase));

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Load SNMP COMMUNITY MIB module
 * @param[in] context Pointer to the SNMP agent context
 * @return Error code
 **/

error_t snmpCommunityMibLoad(void *context)
{
   //Register SNMP agent context
   snmpCommunityMibBase.context = context;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Unload SNMP COMMUNITY MIB module
 * @param[in] context Pointer to the SNMP agent context
 **/

void snmpCommunityMibUnload(void *context)
{
   //Unregister SNMP agent context
   snmpCommunityMibBase.context = NULL;
}


/**
 * @brief Lock SNMP COMMUNITY MIB base
 **/

void snmpCommunityMibLock(void)
{
   //Clear temporary community
   osMemset(&snmpCommunityMibBase.tempCommunity, 0, sizeof(SnmpUserEntry));
}


/**
 * @brief Unlock SNMP COMMUNITY MIB base
 **/

void snmpCommunityMibUnlock(void)
{
   //Clear temporary user
   osMemset(&snmpCommunityMibBase.tempCommunity, 0, sizeof(SnmpUserEntry));
}


/**
 * @brief Set snmpCommunityEntry object value
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier (object name and instance identifier)
 * @param[in] oidLen Length of the OID, in bytes
 * @param[in] value Object value
 * @param[in] valueLen Length of the object value, in bytes
 * @param[in] commit This flag tells whether the changes shall be committed
 *   to the MIB base
 * @return Error code
 **/

error_t snmpCommunityMibSetCommunityEntry(const MibObject *object, const uint8_t *oid,
   size_t oidLen, const MibVariant *value, size_t valueLen, bool_t commit)
{
#if (SNMP_COMMUNITY_MIB_SET_SUPPORT == ENABLED)
   error_t error;
   size_t n;
   char_t index[SNMP_MAX_USER_NAME_LEN + 1];
   SnmpAgentContext *context;
   SnmpUserEntry *community;

   //Point to the instance identifier
   n = object->oidLen;

   //snmpCommunityIndex is used as instance identifier
   error = mibDecodeString(oid, oidLen, &n, index,
      SNMP_MAX_USER_NAME_LEN, TRUE);
   //Invalid instance identifier?
   if(error)
      return error;

   //Sanity check
   if(n != oidLen)
      return ERROR_INSTANCE_NOT_FOUND;

   //Point to the SNMP agent context
   context = (SnmpAgentContext *) snmpCommunityMibBase.context;
   //Sanity check
   if(context == NULL)
      return ERROR_INSTANCE_NOT_FOUND;

   //Search the community table for the specified community string
   community = snmpFindCommunityEntry(context, index, osStrlen(index));

   //snmpCommunityName object?
   if(!osStrcmp(object->name, "snmpCommunityName"))
   {
      //Ensure the length of the community string is valid
      if(valueLen > SNMP_MAX_USER_NAME_LEN)
         return ERROR_WRONG_LENGTH;

      //Test if the conceptual row exists in the agent
      if(community != NULL)
      {
         //Commit phase?
         if(commit)
         {
            //Set community string
            osMemcpy(community->name, value->octetString, valueLen);
            //Properly terminate the string with a NULL character
            community->name[valueLen] = '\0';
         }
      }
      else
      {
         //Prepare phase?
         if(!commit)
         {
            //Save the community string for later use
            osMemcpy(snmpCommunityMibBase.tempCommunity.name,
               value->octetString, valueLen);

            //Properly terminate the string with a NULL character
            snmpCommunityMibBase.tempCommunity.name[valueLen] = '\0';
         }
      }
   }
   //snmpCommunitySecurityName object?
   else if(!osStrcmp(object->name, "snmpCommunitySecurityName"))
   {
      //Write access is not required
   }
   //snmpCommunityContextEngineID object?
   else if(!osStrcmp(object->name, "snmpCommunityContextEngineID"))
   {
      //Write access is not required
   }
   //snmpCommunityContextName object?
   else if(!osStrcmp(object->name, "snmpCommunityContextName"))
   {
      //Write access is not required
   }
   //snmpCommunityTransportTag object?
   else if(!osStrcmp(object->name, "snmpCommunityTransportTag"))
   {
      //Write access is not required
   }
   //snmpCommunityStorageType object?
   else if(!osStrcmp(object->name, "snmpCommunityStorageType"))
   {
      //The snmpCommunityStorageType object specifies the storage type
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
   //snmpCommunityStatus object?
   else if(!osStrcmp(object->name, "snmpCommunityStatus"))
   {
      MibRowStatus status;

      //Get row status
      status = (MibRowStatus) value->integer;

      //Check the value specified by the set operation
      if(status == MIB_ROW_STATUS_ACTIVE ||
         status == MIB_ROW_STATUS_NOT_IN_SERVICE)
      {
         //No matching row found?
         if(community == NULL)
            return ERROR_INCONSISTENT_VALUE;

         //Commit phase?
         if(commit)
         {
            //Valid community string specified?
            if(snmpCommunityMibBase.tempCommunity.name[0] != '\0')
               osStrcpy(community->name, snmpCommunityMibBase.tempCommunity.name);

            //A newly created row cannot be made active until a value has been
            //set for snmpCommunityName
            if(community->name[0] == '\0')
               return ERROR_INCONSISTENT_VALUE;

            //Update the status of the conceptual row
            community->status = status;
         }
      }
      else if(status == MIB_ROW_STATUS_CREATE_AND_GO)
      {
         //Row already instantiated?
         if(community != NULL)
            return ERROR_INCONSISTENT_VALUE;

         //Create a new row
         community = snmpCreateCommunityEntry(context);
         //Row creation failed?
         if(community == NULL)
            return ERROR_WRITE_FAILED;

         //Commit phase?
         if(commit)
         {
            //Valid community string specified?
            if(snmpCommunityMibBase.tempCommunity.name[0] != '\0')
            {
               //Save community string
               osStrcpy(community->name, snmpCommunityMibBase.tempCommunity.name);
               //Set default access rights
               community->mode = SNMP_ACCESS_READ_WRITE;

               //The conceptual row is now available for use by the managed device
               community->status = MIB_ROW_STATUS_ACTIVE;
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
         if(community != NULL)
            return ERROR_INCONSISTENT_VALUE;

         //Create a new row
         community = snmpCreateCommunityEntry(context);
         //Row creation failed?
         if(community == NULL)
            return ERROR_WRITE_FAILED;

         //Commit phase?
         if(commit)
         {
            //Ensure the community string is valid
            if(snmpCommunityMibBase.tempCommunity.name[0] == '\0')
               return ERROR_INCONSISTENT_VALUE;

            //Copy the community string
            osStrcpy(community->name, snmpCommunityMibBase.tempCommunity.name);
            //Set default access rights
            community->mode = SNMP_ACCESS_READ_WRITE;

            //Instances of all corresponding columns are now configured
            community->status = MIB_ROW_STATUS_NOT_IN_SERVICE;
         }
      }
      else if(status == MIB_ROW_STATUS_DESTROY)
      {
         //Test if the conceptual row exists in the agent
         if(community != NULL)
         {
            //Commit phase?
            if(commit)
            {
               //Delete the conceptual row from the table
               community->status = MIB_ROW_STATUS_UNUSED;
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
 * @brief Get snmpCommunityEntry object value
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier (object name and instance identifier)
 * @param[in] oidLen Length of the OID, in bytes
 * @param[out] value Object value
 * @param[in,out] valueLen Length of the object value, in bytes
 * @return Error code
 **/

error_t snmpCommunityMibGetCommunityEntry(const MibObject *object, const uint8_t *oid,
   size_t oidLen, MibVariant *value, size_t *valueLen)
{
   error_t error;
   size_t n;
   char_t index[SNMP_MAX_USER_NAME_LEN + 1];
   SnmpAgentContext *context;
   SnmpUserEntry *community;

   //Point to the instance identifier
   n = object->oidLen;

   //snmpCommunityIndex is used as instance identifier
   error = mibDecodeString(oid, oidLen, &n, index,
      SNMP_MAX_USER_NAME_LEN, TRUE);
   //Invalid instance identifier?
   if(error)
      return error;

   //Sanity check
   if(n != oidLen)
      return ERROR_INSTANCE_NOT_FOUND;

   //Point to the SNMP agent context
   context = (SnmpAgentContext *) snmpCommunityMibBase.context;
   //Sanity check
   if(context == NULL)
      return ERROR_INSTANCE_NOT_FOUND;

   //Search the community table for the specified community string
   community = snmpFindCommunityEntry(context, index, osStrlen(index));
   //Unknown community string?
   if(community == NULL)
      return ERROR_INSTANCE_NOT_FOUND;

   //snmpCommunityName object?
   if(!osStrcmp(object->name, "snmpCommunityName"))
   {
      //Retrieve the length of the community string
      n = osStrlen(community->name);

      //Make sure the buffer is large enough to hold the entire object
      if(*valueLen >= n)
      {
         //Copy object value
         osMemcpy(value->octetString, community->name, n);
         //Return object length
         *valueLen = n;
      }
      else
      {
         //Report an error
         error = ERROR_BUFFER_OVERFLOW;
      }
   }
   //snmpCommunitySecurityName object?
   else if(!osStrcmp(object->name, "snmpCommunitySecurityName"))
   {
      //Retrieve the length of the community string
      n = osStrlen(community->name);

      //Make sure the buffer is large enough to hold the entire object
      if(*valueLen >= n)
      {
         //Copy object value
         osMemcpy(value->octetString, community->name, n);
         //Return object length
         *valueLen = n;
      }
      else
      {
         //Report an error
         error = ERROR_BUFFER_OVERFLOW;
      }
   }
   //snmpCommunityContextEngineID object?
   else if(!osStrcmp(object->name, "snmpCommunityContextEngineID"))
   {
      //Retrieve the length of the context engine identifier
      n = context->contextEngineLen;

      //Make sure the buffer is large enough to hold the entire object
      if(*valueLen >= n)
      {
         //Copy object value
         osMemcpy(value->octetString, context->contextEngine, n);
         //Return object length
         *valueLen = n;
      }
      else
      {
         //Report an error
         error = ERROR_BUFFER_OVERFLOW;
      }
   }
   //snmpCommunityContextName object?
   else if(!osStrcmp(object->name, "snmpCommunityContextName"))
   {
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
   }
   //snmpCommunityTransportTag object?
   else if(!osStrcmp(object->name, "snmpCommunityTransportTag"))
   {
      //The default value is the empty string
      *valueLen = 0;
   }
   //snmpCommunityStorageType object?
   else if(!osStrcmp(object->name, "snmpCommunityStorageType"))
   {
      //Get the storage type for this conceptual row
      value->integer = MIB_STORAGE_TYPE_VOLATILE;
   }
   //snmpCommunityStatus object?
   else if(!osStrcmp(object->name, "snmpCommunityStatus"))
   {
      //Get the status of this conceptual row
      value->integer = community->status;
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
 * @brief Get next snmpCommunityEntry object
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier
 * @param[in] oidLen Length of the OID, in bytes
 * @param[out] nextOid OID of the next object in the MIB
 * @param[out] nextOidLen Length of the next object identifier, in bytes
 * @return Error code
 **/

error_t snmpCommunityMibGetNextCommunityEntry(const MibObject *object, const uint8_t *oid,
   size_t oidLen, uint8_t *nextOid, size_t *nextOidLen)
{
   error_t error;
   uint_t i;
   size_t n;
   bool_t acceptable;
   SnmpAgentContext *context;
   SnmpUserEntry *entry;
   SnmpUserEntry *nextEntry;

   //Initialize variables
   nextEntry = NULL;

   //Point to the SNMP agent context
   context = (SnmpAgentContext *) snmpCommunityMibBase.context;
   //Sanity check
   if(context == NULL)
      return ERROR_OBJECT_NOT_FOUND;

   //Make sure the buffer is large enough to hold the OID prefix
   if(*nextOidLen < object->oidLen)
      return ERROR_BUFFER_OVERFLOW;

   //Copy OID prefix
   osMemcpy(nextOid, object->oid, object->oidLen);

   //Loop through the list of community strings
   for(i = 0; i < SNMP_AGENT_MAX_COMMUNITIES; i++)
   {
      //Point to the current entry
      entry = &context->communityTable[i];

      //Check the status of the row
      if(entry->status != MIB_ROW_STATUS_UNUSED)
      {
         //Append the instance identifier to the OID prefix
         n = object->oidLen;

         //snmpCommunityIndex is used as instance identifier
         error = mibEncodeString(nextOid, *nextOidLen, &n, entry->name, TRUE);
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
            else if(osStrcmp(entry->name, nextEntry->name) < 0)
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

   //snmpCommunityIndex is used as instance identifier
   error = mibEncodeString(nextOid, *nextOidLen, &n, nextEntry->name, TRUE);
   //Any error to report?
   if(error)
      return error;

   //Save the length of the resulting object identifier
   *nextOidLen = n;
   //Next object found
   return NO_ERROR;
}

#endif
