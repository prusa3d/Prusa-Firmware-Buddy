/**
 * @file snmp_framework_mib_impl.c
 * @brief SNMP FRAMEWORK MIB module implementation
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
#include "mibs/snmp_framework_mib_module.h"
#include "mibs/snmp_framework_mib_impl.h"
#include "core/crypto.h"
#include "encoding/asn1.h"
#include "encoding/oid.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (SNMP_FRAMEWORK_MIB_SUPPORT == ENABLED)


/**
 * @brief SNMP FRAMEWORK MIB module initialization
 * @return Error code
 **/

error_t snmpFrameworkMibInit(void)
{
   //Debug message
   TRACE_INFO("Initializing SNMP FRAMEWORK MIB base...\r\n");

   //Clear SNMP FRAMEWORK MIB base
   osMemset(&snmpFrameworkMibBase, 0, sizeof(snmpFrameworkMibBase));

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Load SNMP FRAMEWORK MIB module
 * @param[in] context Pointer to the SNMP agent context
 * @return Error code
 **/

error_t snmpFrameworkMibLoad(void *context)
{
   //Register SNMP agent context
   snmpFrameworkMibBase.context = context;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Unload SNMP FRAMEWORK MIB module
 * @param[in] context Pointer to the SNMP agent context
 **/

void snmpFrameworkMibUnload(void *context)
{
   //Unregister SNMP agent context
   snmpFrameworkMibBase.context = NULL;
}


/**
 * @brief Lock SNMP FRAMEWORK MIB base
 **/

void snmpFrameworkMibLock(void)
{
}


/**
 * @brief Unlock SNMP FRAMEWORK MIB base
 **/

void snmpFrameworkMibUnlock(void)
{
}


/**
 * @brief Get snmpEngineID object value
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier (object name and instance identifier)
 * @param[in] oidLen Length of the OID, in bytes
 * @param[out] value Object value
 * @param[in,out] valueLen Length of the object value, in bytes
 * @return Error code
 **/

error_t snmpFrameworkMibGetSnmpEngineID(const MibObject *object,
   const uint8_t *oid, size_t oidLen, MibVariant *value, size_t *valueLen)
{
   SnmpAgentContext *context;

   //Point to the SNMP agent context
   context = (SnmpAgentContext *) snmpFrameworkMibBase.context;
   //Sanity check
   if(context == NULL)
      return ERROR_OBJECT_NOT_FOUND;

   //Make sure the buffer is large enough to hold the entire object
   if(*valueLen < context->contextEngineLen)
      return ERROR_BUFFER_OVERFLOW;

   //Copy object value
   osMemcpy(value->octetString, context->contextEngine, context->contextEngineLen);
   //Return object length
   *valueLen = context->contextEngineLen;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Get snmpEngineBoots object value
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier (object name and instance identifier)
 * @param[in] oidLen Length of the OID, in bytes
 * @param[out] value Object value
 * @param[in,out] valueLen Length of the object value, in bytes
 * @return Error code
 **/

error_t snmpFrameworkMibGetSnmpEngineBoots(const MibObject *object,
   const uint8_t *oid, size_t oidLen, MibVariant *value, size_t *valueLen)
{
   SnmpAgentContext *context;

   //Point to the SNMP agent context
   context = (SnmpAgentContext *) snmpFrameworkMibBase.context;
   //Sanity check
   if(context == NULL)
      return ERROR_OBJECT_NOT_FOUND;

   //Get object value
   value->integer = context->engineBoots;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Get snmpEngineTime object value
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier (object name and instance identifier)
 * @param[in] oidLen Length of the OID, in bytes
 * @param[out] value Object value
 * @param[in,out] valueLen Length of the object value, in bytes
 * @return Error code
 **/

error_t snmpFrameworkMibGetSnmpEngineTime(const MibObject *object,
   const uint8_t *oid, size_t oidLen, MibVariant *value, size_t *valueLen)
{
   SnmpAgentContext *context;

   //Point to the SNMP agent context
   context = (SnmpAgentContext *) snmpFrameworkMibBase.context;
   //Sanity check
   if(context == NULL)
      return ERROR_OBJECT_NOT_FOUND;

   //Get object value
   value->integer = context->engineTime;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Get snmpEngineMaxMessageSize object value
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier (object name and instance identifier)
 * @param[in] oidLen Length of the OID, in bytes
 * @param[out] value Object value
 * @param[in,out] valueLen Length of the object value, in bytes
 * @return Error code
 **/

error_t snmpFrameworkMibGetSnmpEngineMaxMessageSize(const MibObject *object,
   const uint8_t *oid, size_t oidLen, MibVariant *value, size_t *valueLen)
{
   //Get object value
   value->integer = SNMP_MAX_MSG_SIZE;

   //Successful processing
   return NO_ERROR;
}

#endif
