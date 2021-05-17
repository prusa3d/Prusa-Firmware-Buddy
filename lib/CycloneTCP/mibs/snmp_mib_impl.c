/**
 * @file snmp_mib_impl.c
 * @brief SNMP MIB module implementation
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
#include "mibs/snmp_mib_module.h"
#include "mibs/snmp_mib_impl.h"
#include "snmp/snmp_agent.h"
#include "core/crypto.h"
#include "encoding/asn1.h"
#include "encoding/oid.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (SNMP_MIB_SUPPORT == ENABLED)


/**
 * @brief SNMP MIB module initialization
 * @return Error code
 **/

error_t snmpMibInit(void)
{
   SnmpMibSysGroup *sysGroup;
   SnmpMibSnmpGroup *snmpGroup;
   SnmpMibSetGroup *setGroup;

   //Debug message
   TRACE_INFO("Initializing SNMP-MIB base...\r\n");

   //Clear SNMP MIB base
   osMemset(&snmpMibBase, 0, sizeof(snmpMibBase));

   //Point to the system group
   sysGroup = &snmpMibBase.sysGroup;

#if (SNMP_MIB_SYS_DESCR_SIZE > 0)
   //sysDescr object
   osStrcpy(sysGroup->sysDescr, "Description");
   sysGroup->sysDescrLen = osStrlen(sysGroup->sysDescr);
#endif

#if (SNMP_MIB_SYS_OBJECT_ID_SIZE > 0)
   //sysObjectID object
   sysGroup->sysObjectID[0] = 0;
   sysGroup->sysObjectIDLen = 1;
#endif

#if (SNMP_MIB_SYS_CONTACT_SIZE > 0)
   //sysContact object
   osStrcpy(sysGroup->sysContact, "Contact");
   sysGroup->sysContactLen = osStrlen(sysGroup->sysContact);
#endif

#if (SNMP_MIB_SYS_NAME_SIZE > 0)
   //sysName object
   osStrcpy(sysGroup->sysName, "Name");
   sysGroup->sysNameLen = osStrlen(sysGroup->sysName);
#endif

#if (SNMP_MIB_SYS_LOCATION_SIZE > 0)
   //sysLocation object
   osStrcpy(sysGroup->sysLocation, "Location");
   sysGroup->sysLocationLen = osStrlen(sysGroup->sysLocation);
#endif

   //sysServices object
   sysGroup->sysServices = SNMP_MIB_SYS_SERVICE_INTERNET;

   //Point to the SNMP group
   snmpGroup = &snmpMibBase.snmpGroup;
   //snmpEnableAuthenTraps object
   snmpGroup->snmpEnableAuthenTraps = SNMP_MIB_AUTHEN_TRAPS_DISABLED;

   //Point to the set group
   setGroup = &snmpMibBase.setGroup;
   //snmpSetSerialNo object
   setGroup->snmpSetSerialNo = netGetRandRange(1, INT32_MAX);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Load SNMP MIB module
 * @param[in] context Pointer to the SNMP agent context
 * @return Error code
 **/

error_t snmpMibLoad(void *context)
{
   //Register SNMP agent context
   snmpMibBase.context = context;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Unload SNMP MIB module
 * @param[in] context Pointer to the SNMP agent context
 **/

void snmpMibUnload(void *context)
{
   //Unregister SNMP agent context
   snmpMibBase.context = NULL;
}


/**
 * @brief Lock SNMP MIB base
 **/

void snmpMibLock(void)
{
}


/**
 * @brief Unlock SNMP MIB base
 **/

void snmpMibUnlock(void)
{
}


/**
 * @brief Get sysUpTime object value
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier (object name and instance identifier)
 * @param[in] oidLen Length of the OID, in bytes
 * @param[out] value Object value
 * @param[in,out] valueLen Length of the object value, in bytes
 * @return Error code
 **/

error_t snmpMibGetSysUpTime(const MibObject *object, const uint8_t *oid,
   size_t oidLen, MibVariant *value, size_t *valueLen)
{
   //Get object value
   value->timeTicks = osGetSystemTime() / 10;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Get sysOREntry object value
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier (object name and instance identifier)
 * @param[in] oidLen Length of the OID, in bytes
 * @param[out] value Object value
 * @param[in,out] valueLen Length of the object value, in bytes
 * @return Error code
 **/

error_t snmpMibGetSysOREntry(const MibObject *object, const uint8_t *oid,
   size_t oidLen, MibVariant *value, size_t *valueLen)
{
   error_t error;
   size_t n;
   uint_t index;
   SnmpAgentContext *context;
   const MibModule *mibModule;

   //Point to the instance identifier
   n = object->oidLen;

   //sysORIndex is used as instance identifier
   error = mibDecodeIndex(oid, oidLen, &n, &index);
   //Invalid instance identifier?
   if(error)
      return error;

   //Sanity check
   if(n != oidLen)
      return ERROR_INSTANCE_NOT_FOUND;

   //Check index range
   if(index < 1 || index > SNMP_AGENT_MAX_MIBS)
      return ERROR_INSTANCE_NOT_FOUND;

   //Point to the SNMP agent context
   context = (SnmpAgentContext *) snmpMibBase.context;
   //Sanity check
   if(context == NULL)
      return ERROR_INSTANCE_NOT_FOUND;

   //Point to the MIB
   mibModule = context->mibTable[index - 1];
   //Make sure the MIB is properly loaded
   if(mibModule == NULL)
      return ERROR_INSTANCE_NOT_FOUND;

   //sysORID object?
   if(!osStrcmp(object->name, "sysORID"))
   {
      //Make sure the buffer is large enough to hold the entire object
      if(*valueLen >= mibModule->oidLen)
      {
         //Copy object value
         osMemcpy(value->octetString, mibModule->oid, mibModule->oidLen);
         //Return object length
         *valueLen = mibModule->oidLen;
      }
      else
      {
         //Report an error
         error = ERROR_BUFFER_OVERFLOW;
      }
   }
   //sysORDescr object?
   else if(!osStrcmp(object->name, "sysORDescr"))
   {
      //Retrieve the length of the MIB name
      n = osStrlen(mibModule->name);

      //Make sure the buffer is large enough to hold the entire object
      if(*valueLen >= n)
      {
         //Copy object value
         osMemcpy(value->octetString, mibModule->name, n);
         //Return object length
         *valueLen = n;
      }
      else
      {
         //Report an error
         error = ERROR_BUFFER_OVERFLOW;
      }
   }
   //sysORUpTime object?
   else if(!osStrcmp(object->name, "sysORUpTime"))
   {
      //Get object value
      value->timeTicks = 0;
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
 * @brief Get next sysOREntry object
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier
 * @param[in] oidLen Length of the OID, in bytes
 * @param[out] nextOid OID of the next object in the MIB
 * @param[out] nextOidLen Length of the next object identifier, in bytes
 * @return Error code
 **/

error_t snmpMibGetNextSysOREntry(const MibObject *object, const uint8_t *oid,
   size_t oidLen, uint8_t *nextOid, size_t *nextOidLen)
{
   error_t error;
   size_t n;
   uint_t index;
   SnmpAgentContext *context;

   //Point to the SNMP agent context
   context = (SnmpAgentContext *) snmpMibBase.context;
   //Sanity check
   if(context == NULL)
      return ERROR_OBJECT_NOT_FOUND;

   //Make sure the buffer is large enough to hold the OID prefix
   if(*nextOidLen < object->oidLen)
      return ERROR_BUFFER_OVERFLOW;

   //Copy OID prefix
   osMemcpy(nextOid, object->oid, object->oidLen);

   //Loop through existing MIBs
   for(index = 1; index <= SNMP_AGENT_MAX_MIBS; index++)
   {
      //Make sure the MIB is properly loaded
      if(context->mibTable[index - 1] != NULL)
      {
         //Append the instance identifier to the OID prefix
         n = object->oidLen;

         //sysORIndex is used as instance identifier
         error = mibEncodeIndex(nextOid, *nextOidLen, &n, index);
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
      }
   }

   //The specified OID does not lexicographically precede the name
   //of some object
   return ERROR_OBJECT_NOT_FOUND;
}


/**
 * @brief Get snmpTrapOID object value
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier (object name and instance identifier)
 * @param[in] oidLen Length of the OID, in bytes
 * @param[out] value Object value
 * @param[in,out] valueLen Length of the object value, in bytes
 * @return Error code
 **/

error_t snmpv2MibGetSnmpTrapOID(const MibObject *object, const uint8_t *oid,
   size_t oidLen, MibVariant *value, size_t *valueLen)
{
   //Not implemented
   *valueLen = 0;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Get snmpTrapEnterprise object value
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier (object name and instance identifier)
 * @param[in] oidLen Length of the OID, in bytes
 * @param[out] value Object value
 * @param[in,out] valueLen Length of the object value, in bytes
 * @return Error code
 **/

error_t snmpv2MibGetSnmpTrapEnterprise(const MibObject *object, const uint8_t *oid,
   size_t oidLen, MibVariant *value, size_t *valueLen)
{
   SnmpAgentContext *context;

   //Point to the SNMP agent context
   context = (SnmpAgentContext *) snmpMibBase.context;
   //Sanity check
   if(context == NULL)
      return ERROR_OBJECT_NOT_FOUND;

   //Make sure the buffer is large enough to hold the enterprise OID
   if(*valueLen < context->enterpriseOidLen)
      return ERROR_BUFFER_OVERFLOW;

   //Copy enterprise OID
   osMemcpy(value->octetString, context->enterpriseOid,
      context->enterpriseOidLen);

   //Return object length
   *valueLen = context->enterpriseOidLen;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set snmpSetSerialNo object value
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier (object name and instance identifier)
 * @param[in] oidLen Length of the OID, in bytes
 * @param[in] value Object value
 * @param[in] valueLen Length of the object value, in bytes
 * @param[in] commit This flag tells whether the changes shall be committed
 *   to the MIB base
 * @return Error code
 **/

error_t snmpMibSetSnmpSetSerialNo(const MibObject *object, const uint8_t *oid,
   size_t oidLen, const MibVariant *value, size_t valueLen, bool_t commit)
{
   //Test and increment spin lock
   return mibTestAndIncSpinLock(&snmpMibBase.setGroup.snmpSetSerialNo,
      value->integer, commit);
}


/**
 * @brief Get snmpSetSerialNo object value
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier (object name and instance identifier)
 * @param[in] oidLen Length of the OID, in bytes
 * @param[out] value Object value
 * @param[in,out] valueLen Length of the object value, in bytes
 * @return Error code
 **/

error_t snmpMibGetSnmpSetSerialNo(const MibObject *object, const uint8_t *oid,
   size_t oidLen, MibVariant *value, size_t *valueLen)
{
   //Get the current value of the spin lock
   value->integer = snmpMibBase.setGroup.snmpSetSerialNo;

   //Successful processing
   return NO_ERROR;
}

#endif
