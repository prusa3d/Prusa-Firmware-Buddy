/**
 * @file snmp_agent.c
 * @brief SNMP agent (Simple Network Management Protocol)
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
 * SNMP is a simple protocol by which management information for a network
 * element may be inspected or altered by logically remote users. Refer
 * to the following RFCs for complete details:
 * - RFC 1157: A Simple Network Management Protocol (SNMP)
 * - RFC 1905: Protocol Operations for Version 2 of the Simple Network
 *     Management Protocol (SNMPv2)
 * - RFC 3410: Introduction and Applicability Statements for Internet
 *     Standard Management Framework
 * - RFC 3411: An Architecture for Describing SNMP Management Frameworks
 * - RFC 3412: Message Processing and Dispatching for the SNMP
 * - RFC 3413: Simple Network Management Protocol (SNMP) Applications
 * - RFC 3584: Coexistence between Version 1, Version 2, and Version 3 of
 *     SNMP Framework
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
#include "snmp/snmp_agent_trap.h"
#include "snmp/snmp_agent_inform.h"
#include "mibs/mib2_module.h"
#include "core/crypto.h"
#include "encoding/asn1.h"
#include "encoding/oid.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (SNMP_AGENT_SUPPORT == ENABLED)


/**
 * @brief Initialize settings with default values
 * @param[out] settings Structure that contains SNMP agent settings
 **/

void snmpAgentGetDefaultSettings(SnmpAgentSettings *settings)
{
   //The SNMP agent is not bound to any interface
   settings->interface = NULL;

   //Minimum version accepted by the SNMP agent
   settings->versionMin = SNMP_VERSION_1;
   //Maximum version accepted by the SNMP agent
   settings->versionMax = SNMP_VERSION_3;

   //SNMP port number
   settings->port = SNMP_PORT;
   //SNMP trap port number
   settings->trapPort = SNMP_TRAP_PORT;

   //Random data generation callback function
   settings->randCallback = NULL;
}


/**
 * @brief SNMP agent initialization
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] settings SNMP agent specific settings
 * @return Error code
 **/

error_t snmpAgentInit(SnmpAgentContext *context, const SnmpAgentSettings *settings)
{
   error_t error;

   //Debug message
   TRACE_INFO("Initializing SNMP agent...\r\n");

   //Ensure the parameters are valid
   if(context == NULL || settings == NULL)
      return ERROR_INVALID_PARAMETER;

   //Check minimum and maximum SNMP versions
   if(settings->versionMin > settings->versionMax)
      return ERROR_INVALID_PARAMETER;

   //Clear the SNMP agent context
   osMemset(context, 0, sizeof(SnmpAgentContext));

   //Save user settings
   context->settings = *settings;

   //Initialize request identifier
   context->requestId = netGetRandRange(1, INT32_MAX);

#if (SNMP_V3_SUPPORT == ENABLED)
   //Get current time
   context->systemTime = osGetSystemTime();

   //Each SNMP engine maintains two values, snmpEngineBoots and snmpEngineTime,
   //which taken together provide an indication of time at that SNMP engine
   context->engineBoots = 1;
   context->engineTime = 0;

   //Initialize message identifier
   context->msgId = netGetRandRange(1, INT32_MAX);

   //Check whether SNMPv3 is supported
   if(settings->versionMin <= SNMP_VERSION_3 &&
      settings->versionMax >= SNMP_VERSION_3)
   {
      //Make sure a random number generator has been registered
      if(settings->randCallback == NULL)
         return ERROR_INVALID_PARAMETER;

      //The salt integer is initialized to an arbitrary value at boot time
      error = settings->randCallback((uint8_t *) &context->salt, sizeof(context->salt));
      //Any error to report?
      if(error)
         return error;
   }
#endif

   //Create a mutex to prevent simultaneous access to SNMP agent context
   if(!osCreateMutex(&context->mutex))
   {
      //Failed to create mutex
      return ERROR_OUT_OF_RESOURCES;
   }

#if (SNMP_AGENT_INFORM_SUPPORT == ENABLED)
   //Create an event object to manage inform request retransmissions
   if(!osCreateEvent(&context->informEvent))
   {
      //Clean up side effects
      osDeleteMutex(&context->mutex);
      //Failed to event object
      return ERROR_OUT_OF_RESOURCES;
   }
#endif

   //Open a UDP socket
   context->socket = socketOpen(SOCKET_TYPE_DGRAM, SOCKET_IP_PROTO_UDP);

   //Failed to open socket?
   if(context->socket == NULL)
   {
      //Clean up side effects
      osDeleteMutex(&context->mutex);
#if (SNMP_AGENT_INFORM_SUPPORT == ENABLED)
      osDeleteEvent(&context->informEvent);
#endif
      //Report an error
      return ERROR_OPEN_FAILED;
   }

   //Start of exception handling block
   do
   {
      //Explicitly associate the socket with the relevant interface
      error = socketBindToInterface(context->socket, settings->interface);
      //Unable to bind the socket to the desired interface?
      if(error)
         break;

      //The SNMP agent listens for messages on port 161
      error = socketBind(context->socket, &IP_ADDR_ANY, settings->port);
      //Unable to bind the socket to the desired port?
      if(error)
         break;

      //End of exception handling block
   } while(0);

   //Any error to report?
   if(error)
   {
      //Clean up side effects
      osDeleteMutex(&context->mutex);
#if (SNMP_AGENT_INFORM_SUPPORT == ENABLED)
      osDeleteEvent(&context->informEvent);
#endif
      //Close underlying socket
      socketClose(context->socket);
   }

   //Return status code
   return error;
}


/**
 * @brief Start SNMP agent
 * @param[in] context Pointer to the SNMP agent context
 * @return Error code
 **/

error_t snmpAgentStart(SnmpAgentContext *context)
{
   OsTask *task;

   //Make sure the SNMP agent context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Debug message
   TRACE_INFO("Starting SNMP agent...\r\n");

   //Start the SNMP agent service
   task = osCreateTask("SNMP Agent", (OsTaskCode) snmpAgentTask,
      context, SNMP_AGENT_STACK_SIZE, SNMP_AGENT_PRIORITY);

   //Unable to create the task?
   if(task == OS_INVALID_HANDLE)
      return ERROR_OUT_OF_RESOURCES;

   //The SNMP agent has successfully started
   return NO_ERROR;
}


/**
 * @brief Load a MIB module
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] module Pointer the MIB module to be loaded
 * @return Error code
 **/

error_t snmpAgentLoadMib(SnmpAgentContext *context, const MibModule *module)
{
   error_t error;
   uint_t i;

   //Check parameters
   if(context == NULL || module == NULL)
      return ERROR_INVALID_PARAMETER;
   if(module->numObjects < 1)
      return ERROR_INVALID_PARAMETER;

   //Acquire exclusive access to the SNMP agent context
   osAcquireMutex(&context->mutex);

   //Loop through existing MIBs
   for(i = 0; i < SNMP_AGENT_MAX_MIBS; i++)
   {
      //Check whether the specified MIB module is already loaded
      if(context->mibTable[i] == module)
         break;
   }

   //MIB module found?
   if(i < SNMP_AGENT_MAX_MIBS)
   {
      //Prevent the SNMP agent from loading the same MIB multiple times
      error = NO_ERROR;
   }
   else
   {
      //Loop through existing MIBs
      for(i = 0; i < SNMP_AGENT_MAX_MIBS; i++)
      {
         //Check if the current entry is available
         if(context->mibTable[i] == NULL)
            break;
      }

      //Make sure there is enough room to add the specified MIB
      if(i < SNMP_AGENT_MAX_MIBS)
      {
         //Invoke user callback, if any
         if(module->load != NULL)
            error = module->load(context);
         else
            error = NO_ERROR;

         //Check status code
         if(!error)
         {
            //Add the MIB to the list
            context->mibTable[i] = module;
         }
      }
      else
      {
         //Failed to load the specified MIB
         error = ERROR_OUT_OF_RESOURCES;
      }
   }

   //Release exclusive access to the SNMP agent context
   osReleaseMutex(&context->mutex);

   //Return status code
   return error;
}


/**
 * @brief Unload a MIB module
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] module Pointer the MIB module to be unloaded
 * @return Error code
 **/

error_t snmpAgentUnloadMib(SnmpAgentContext *context, const MibModule *module)
{
   error_t error;
   uint_t i;

   //Check parameters
   if(context == NULL || module == NULL)
      return ERROR_INVALID_PARAMETER;

   //Acquire exclusive access to the SNMP agent context
   osAcquireMutex(&context->mutex);

   //Loop through existing MIBs
   for(i = 0; i < SNMP_AGENT_MAX_MIBS; i++)
   {
      //Check whether the specified MIB module is already loaded
      if(context->mibTable[i] == module)
         break;
   }

   //MIB module found?
   if(i < SNMP_AGENT_MAX_MIBS)
   {
      //Any registered callback?
      if(context->mibTable[i]->unload != NULL)
      {
         //Invoke user callback function
         context->mibTable[i]->unload(context);
      }

      //Remove the MIB from the list
      context->mibTable[i] = NULL;

      //Successful processing
      error = NO_ERROR;
   }
   else
   {
      //Failed to unload the specified MIB
      error = ERROR_NOT_FOUND;
   }

   //Release exclusive access to the SNMP agent context
   osReleaseMutex(&context->mutex);

   //Return status code
   return error;
}


/**
 * @brief Set minimum and maximum versions permitted
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] versionMin Minimum version accepted by the SNMP agent
 * @param[in] versionMax Maximum version accepted by the SNMP agent
 * @return Error code
 **/

error_t snmpAgentSetVersion(SnmpAgentContext *context,
   SnmpVersion versionMin, SnmpVersion versionMax)
{
   //Check parameters
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;
   if(versionMin > versionMax)
      return ERROR_INVALID_PARAMETER;

   //Acquire exclusive access to the SNMP agent context
   osAcquireMutex(&context->mutex);

   //Set minimum and maximum versions permitted
   context->settings.versionMin = versionMin;
   context->settings.versionMax = versionMax;

   //Release exclusive access to the SNMP agent context
   osReleaseMutex(&context->mutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set the value of the snmpEngineBoots variable
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] engineBoots Number of times the SNMP engine has re-booted
 * @return Error code
 **/

error_t snmpAgentSetEngineBoots(SnmpAgentContext *context, int32_t engineBoots)
{
#if (SNMP_V3_SUPPORT == ENABLED)
   //Check parameters
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;
   if(engineBoots < 0)
      return ERROR_OUT_OF_RANGE;

   //Acquire exclusive access to the SNMP agent context
   osAcquireMutex(&context->mutex);

   //Get current time
   context->systemTime = osGetSystemTime();

   //Set the value of the snmpEngineBoots
   context->engineBoots = engineBoots;
   //The snmpEngineTime is reset to zero
   context->engineTime = 0;

   //Release exclusive access to the SNMP agent context
   osReleaseMutex(&context->mutex);

   //Successful processing
   return NO_ERROR;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Get the value of the snmpEngineBoots variable
 * @param[in] context Pointer to the SNMP agent context
 * @param[out] engineBoots Number of times the SNMP engine has re-booted
 * @return Error code
 **/

error_t snmpAgentGetEngineBoots(SnmpAgentContext *context, int32_t *engineBoots)
{
#if (SNMP_V3_SUPPORT == ENABLED)
   //Check parameters
   if(context == NULL || engineBoots == NULL)
      return ERROR_INVALID_PARAMETER;

   //Acquire exclusive access to the SNMP agent context
   osAcquireMutex(&context->mutex);
   //Get the current value of the snmpEngineBoots
   *engineBoots = context->engineBoots;
   //Release exclusive access to the SNMP agent context
   osReleaseMutex(&context->mutex);

   //Successful processing
   return NO_ERROR;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Set enterprise OID
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] enterpriseOid Pointer to the enterprise OID
 * @param[in] enterpriseOidLen Length of the enterprise OID
 * @return Error code
 **/

error_t snmpAgentSetEnterpriseOid(SnmpAgentContext *context,
   const uint8_t *enterpriseOid, size_t enterpriseOidLen)
{
   //Check parameters
   if(context == NULL || enterpriseOid == NULL)
      return ERROR_INVALID_PARAMETER;
   if(enterpriseOidLen > SNMP_MAX_OID_SIZE)
      return ERROR_INVALID_PARAMETER;

   //Acquire exclusive access to the SNMP agent context
   osAcquireMutex(&context->mutex);

   //Set enterprise OID
   osMemcpy(context->enterpriseOid, enterpriseOid, enterpriseOidLen);
   //Save the length of the enterprise OID
   context->enterpriseOidLen = enterpriseOidLen;

   //Release exclusive access to the SNMP agent context
   osReleaseMutex(&context->mutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set context engine identifier
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] contextEngine Pointer to the context engine identifier
 * @param[in] contextEngineLen Length of the context engine identifier
 * @return Error code
 **/

error_t snmpAgentSetContextEngine(SnmpAgentContext *context,
   const void *contextEngine, size_t contextEngineLen)
{
#if (SNMP_V3_SUPPORT == ENABLED)
   //Check parameters
   if(context == NULL || contextEngine == NULL)
      return ERROR_INVALID_PARAMETER;
   if(contextEngineLen > SNMP_MAX_CONTEXT_ENGINE_SIZE)
      return ERROR_INVALID_PARAMETER;

   //Acquire exclusive access to the SNMP agent context
   osAcquireMutex(&context->mutex);

   //Set context engine identifier
   osMemcpy(context->contextEngine, contextEngine, contextEngineLen);
   //Save the length of the context engine identifier
   context->contextEngineLen = contextEngineLen;

   //Release exclusive access to the SNMP agent context
   osReleaseMutex(&context->mutex);

   //Successful processing
   return NO_ERROR;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Set context name
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] contextName NULL-terminated string that contains the context name
 * @return Error code
 **/

error_t snmpAgentSetContextName(SnmpAgentContext *context,
   const char_t *contextName)
{
#if (SNMP_V3_SUPPORT == ENABLED)
   size_t n;

   //Check parameters
   if(context == NULL || contextName == NULL)
      return ERROR_INVALID_PARAMETER;

   //Retrieve the length of the context name
   n = osStrlen(contextName);

   //Make sure the context name is valid
   if(n > SNMP_MAX_CONTEXT_NAME_LEN)
      return ERROR_INVALID_LENGTH;

   //Acquire exclusive access to the SNMP agent context
   osAcquireMutex(&context->mutex);
   //Set context name
   osStrcpy(context->contextName, contextName);
   //Release exclusive access to the SNMP agent context
   osReleaseMutex(&context->mutex);

   //Successful processing
   return NO_ERROR;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Create a new community string
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] community NULL-terminated string that contains the community name
 * @param[in] mode Access rights
 * @return Error code
 **/

error_t snmpAgentCreateCommunity(SnmpAgentContext *context,
   const char_t *community, SnmpAccess mode)
{
#if (SNMP_V1_SUPPORT == ENABLED || SNMP_V2C_SUPPORT == ENABLED)
   error_t error;
   size_t n;
   SnmpUserEntry *entry;

   //Check parameters
   if(context == NULL || community == NULL)
      return ERROR_INVALID_PARAMETER;

   //Retrieve the length of the community string
   n = osStrlen(community);

   //Make sure the community string is valid
   if(n == 0 || n > SNMP_MAX_USER_NAME_LEN)
      return ERROR_INVALID_LENGTH;

   //Acquire exclusive access to the SNMP agent context
   osAcquireMutex(&context->mutex);

   //Check whether the community string already exists
   entry = snmpFindCommunityEntry(context, community, osStrlen(community));

   //If the specified community string does not exist, then a new entry
   //should be created
   if(entry == NULL)
   {
      //Create a new entry
      entry = snmpCreateCommunityEntry(context);
   }

   //Any entry available?
   if(entry != NULL)
   {
      //Clear the contents
      osMemset(entry, 0, sizeof(SnmpUserEntry));

      //Save community string
      osStrcpy(entry->name, community);
      //Set access rights
      entry->mode = mode;
      //The entry is now available for use
      entry->status = MIB_ROW_STATUS_ACTIVE;

      //Successful processing
      error = NO_ERROR;
   }
   else
   {
      //The table runs out of space
      error = ERROR_OUT_OF_RESOURCES;
   }

   //Release exclusive access to the SNMP agent context
   osReleaseMutex(&context->mutex);

   //Return error code
   return error;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Remove a community string
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] community NULL-terminated string that contains the community name
 * @return Error code
 **/

error_t snmpAgentDeleteCommunity(SnmpAgentContext *context, const char_t *community)
{
#if (SNMP_V1_SUPPORT == ENABLED || SNMP_V2C_SUPPORT == ENABLED)
   error_t error;
   SnmpUserEntry *entry;

   //Check parameters
   if(context == NULL || community == NULL)
      return ERROR_INVALID_PARAMETER;

   //Acquire exclusive access to the SNMP agent context
   osAcquireMutex(&context->mutex);

   //Search the community table for the specified community string
   entry = snmpFindCommunityEntry(context, community, osStrlen(community));

   //Any matching entry found?
   if(entry != NULL)
   {
      //Clear the contents
      osMemset(entry, 0, sizeof(SnmpUserEntry));
      //Now mark the entry as free
      entry->status = MIB_ROW_STATUS_UNUSED;

      //Successful processing
      error = NO_ERROR;
   }
   else
   {
      //The specified community string does not exist
      error = ERROR_NOT_FOUND;
   }

   //Release exclusive access to the SNMP agent context
   osReleaseMutex(&context->mutex);

   //Return status code
   return error;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Create a new user
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] userName NULL-terminated string that contains the user name
 * @param[in] mode Access rights
 * @param[in] keyFormat Key format (ASCII password or raw key)
 * @param[in] authProtocol Authentication type
 * @param[in] authKey Key to be used for data authentication
 * @param[in] privProtocol Privacy type
 * @param[in] privKey Key to be used for data encryption
 * @return Error code
 **/

error_t snmpAgentCreateUser(SnmpAgentContext *context,
   const char_t *userName, SnmpAccess mode, SnmpKeyFormat keyFormat,
   SnmpAuthProtocol authProtocol, const void *authKey,
   SnmpPrivProtocol privProtocol, const void *privKey)
{
#if (SNMP_V3_SUPPORT == ENABLED)
   error_t error;
   size_t n;
   SnmpUserEntry *entry;

   //Check parameters
   if(context == NULL || userName == NULL)
      return ERROR_INVALID_PARAMETER;

   //Data authentication?
   if(authProtocol != SNMP_AUTH_PROTOCOL_NONE)
   {
      //Check key format
      if(keyFormat != SNMP_KEY_FORMAT_TEXT &&
         keyFormat != SNMP_KEY_FORMAT_RAW &&
         keyFormat != SNMP_KEY_FORMAT_LOCALIZED)
      {
         return ERROR_INVALID_PARAMETER;
      }

      //Data authentication requires a key
      if(authKey == NULL)
         return ERROR_INVALID_PARAMETER;
   }

   //Data confidentiality?
   if(privProtocol != SNMP_PRIV_PROTOCOL_NONE)
   {
      //Check key format
      if(keyFormat != SNMP_KEY_FORMAT_TEXT && keyFormat != SNMP_KEY_FORMAT_RAW)
         return ERROR_INVALID_PARAMETER;

      //Data confidentiality requires a key
      if(privKey == NULL)
         return ERROR_INVALID_PARAMETER;

      //There is no provision for data confidentiality without data authentication
      if(authProtocol == SNMP_AUTH_PROTOCOL_NONE)
         return ERROR_INVALID_PARAMETER;
   }

   //Retrieve the length of the user name
   n = osStrlen(userName);

   //Make sure the user name is valid
   if(n == 0 || n > SNMP_MAX_USER_NAME_LEN)
      return ERROR_INVALID_LENGTH;

   //Acquire exclusive access to the SNMP agent context
   osAcquireMutex(&context->mutex);

   //Check whether the user name already exists
   entry = snmpFindUserEntry(context, userName, osStrlen(userName));

   //If the specified user name does not exist, then a new entry
   //should be created
   if(entry == NULL)
   {
      //Create a security profile for the new user
      entry = snmpCreateUserEntry(context);
   }

   //Any entry available?
   if(entry != NULL)
   {
      //Clear the security profile of the user
      osMemset(entry, 0, sizeof(SnmpUserEntry));

      //Save user name
      osStrcpy(entry->name, userName);
      //Access rights
      entry->mode = mode;
      //Authentication protocol
      entry->authProtocol = authProtocol;
      //Privacy protocol
      entry->privProtocol = privProtocol;

      //Initialize status code
      error = NO_ERROR;

      //Data authentication?
      if(authProtocol != SNMP_AUTH_PROTOCOL_NONE)
      {
         //ASCII password or raw key?
         if(keyFormat == SNMP_KEY_FORMAT_TEXT)
         {
            //Generate the authentication key from the provided password
            error = snmpGenerateKey(authProtocol, authKey, &entry->rawAuthKey);

            //Check status code
            if(!error)
            {
               //Localize the key with the engine ID
               error = snmpLocalizeKey(authProtocol,
                  context->contextEngine, context->contextEngineLen,
                  &entry->rawAuthKey, &entry->localizedAuthKey);
            }
         }
         else if(keyFormat == SNMP_KEY_FORMAT_RAW)
         {
            //Save the authentication key
            osMemcpy(&entry->rawAuthKey, authKey, sizeof(SnmpKey));

            //Now localize the key with the engine ID
            error = snmpLocalizeKey(authProtocol,
               context->contextEngine, context->contextEngineLen,
               &entry->rawAuthKey, &entry->localizedAuthKey);
         }
         else
         {
            //The authentication key is already localized
            osMemcpy(&entry->localizedAuthKey, authKey, sizeof(SnmpKey));
         }
      }

      //Check status code
      if(!error)
      {
         //Data confidentiality?
         if(privProtocol != SNMP_PRIV_PROTOCOL_NONE)
         {
            //ASCII password or raw key?
            if(keyFormat == SNMP_KEY_FORMAT_TEXT)
            {
               //Generate the privacy key from the provided password
               error = snmpGenerateKey(authProtocol, privKey, &entry->rawPrivKey);

               //Check status code
               if(!error)
               {
                  //Localize the key with the engine ID
                  error = snmpLocalizeKey(authProtocol,
                     context->contextEngine, context->contextEngineLen,
                     &entry->rawPrivKey, &entry->localizedPrivKey);
               }
            }
            else if(keyFormat == SNMP_KEY_FORMAT_RAW)
            {
               //Save the privacy key
               osMemcpy(&entry->rawPrivKey, privKey, sizeof(SnmpKey));

               //Now localize the key with the engine ID
               error = snmpLocalizeKey(authProtocol,
                  context->contextEngine, context->contextEngineLen,
                  &entry->rawPrivKey, &entry->localizedPrivKey);
            }
            else
            {
               //The privacy key is already localized
               osMemcpy(&entry->localizedPrivKey, privKey, sizeof(SnmpKey));
            }
         }
      }

      //Check status code
      if(!error)
      {
         //The entry is now available for use
         entry->status = MIB_ROW_STATUS_ACTIVE;
      }
      else
      {
         //Clean up side effects
         osMemset(entry, 0, sizeof(SnmpUserEntry));
         //Now mark the entry as free
         entry->status = MIB_ROW_STATUS_UNUSED;
      }
   }
   else
   {
      //The user table runs out of space
      error = ERROR_OUT_OF_RESOURCES;
   }

   //Release exclusive access to the SNMP agent context
   osReleaseMutex(&context->mutex);

   //Return error code
   return error;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Remove existing user
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] userName NULL-terminated string that contains the user name
 * @return Error code
 **/

error_t snmpAgentDeleteUser(SnmpAgentContext *context, const char_t *userName)
{
#if (SNMP_V3_SUPPORT == ENABLED)
   error_t error;
   SnmpUserEntry *entry;

   //Check parameters
   if(context == NULL || userName == NULL)
      return ERROR_INVALID_PARAMETER;

   //Acquire exclusive access to the SNMP agent context
   osAcquireMutex(&context->mutex);

   //Search the user table for the specified user name
   entry = snmpFindUserEntry(context, userName, osStrlen(userName));

   //Any matching entry found?
   if(entry != NULL)
   {
      //Clear the security profile of the user
      osMemset(entry, 0, sizeof(SnmpUserEntry));
      //Now mark the entry as free
      entry->status = MIB_ROW_STATUS_UNUSED;

      //Successful processing
      error = NO_ERROR;
   }
   else
   {
      //The specified user name does not exist
      error = ERROR_NOT_FOUND;
   }

   //Release exclusive access to the SNMP agent context
   osReleaseMutex(&context->mutex);

   //Return status code
   return error;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Join a group of users
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] userName NULL-terminated string that contains the user name
 * @param[in] securityModel Security model
 * @param[in] groupName NULL-terminated string that contains the group name
 * @return Error code
 **/

error_t snmpAgentJoinGroup(SnmpAgentContext *context, const char_t *userName,
   SnmpSecurityModel securityModel, const char_t *groupName)
{
#if (SNMP_AGENT_VACM_SUPPORT == ENABLED)
   error_t error;
   size_t n;
   SnmpGroupEntry *entry;

   //Check parameters
   if(context == NULL || userName == NULL || groupName == NULL)
      return ERROR_INVALID_PARAMETER;

   //Check security model
   if(securityModel != SNMP_SECURITY_MODEL_V1 &&
      securityModel != SNMP_SECURITY_MODEL_V2C &&
      securityModel != SNMP_SECURITY_MODEL_USM)
   {
      return ERROR_INVALID_PARAMETER;
   }

   //Retrieve the length of the user name
   n = osStrlen(userName);

   //Make sure the user name is valid
   if(n == 0 || n > SNMP_MAX_USER_NAME_LEN)
      return ERROR_INVALID_LENGTH;

   //Retrieve the length of the group name
   n = osStrlen(groupName);

   //Make sure the group name is valid
   if(n == 0 || n > SNMP_MAX_GROUP_NAME_LEN)
      return ERROR_INVALID_LENGTH;

   //Acquire exclusive access to the SNMP agent context
   osAcquireMutex(&context->mutex);

   //Search the group table for a matching entry
   entry = snmpFindGroupEntry(context, securityModel, userName,
      osStrlen(userName));

   //No matching entry found?
   if(entry == NULL)
   {
      //Create a new entry in the group table
      entry = snmpCreateGroupEntry(context);
   }

   //Any entry available?
   if(entry != NULL)
   {
      //Clear entry
      osMemset(entry, 0, sizeof(SnmpGroupEntry));

      //Save security model
      entry->securityModel = securityModel;
      //Save user name
      osStrcpy(entry->securityName, userName);
      //Save group name
      osStrcpy(entry->groupName, groupName);

      //The entry is now available for use
      entry->status = MIB_ROW_STATUS_ACTIVE;

      //Successful processing
      error = NO_ERROR;
   }
   else
   {
      //The group table runs out of space
      error = ERROR_OUT_OF_RESOURCES;
   }

   //Release exclusive access to the SNMP agent context
   osReleaseMutex(&context->mutex);

   //Return status code
   return error;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Leave a group of users
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] userName NULL-terminated string that contains the user name
 * @param[in] securityModel Security model
 * @return Error code
 **/

error_t snmpAgentLeaveGroup(SnmpAgentContext *context,
   const char_t *userName, SnmpSecurityModel securityModel)
{
#if (SNMP_AGENT_VACM_SUPPORT == ENABLED)
   error_t error;
   SnmpGroupEntry *entry;

   //Check parameters
   if(context == NULL || userName == NULL)
      return ERROR_INVALID_PARAMETER;

   //Acquire exclusive access to the SNMP agent context
   osAcquireMutex(&context->mutex);

   //Search the group table for a matching entry
   entry = snmpFindGroupEntry(context, securityModel, userName,
      osStrlen(userName));

   //Any matching entry found?
   if(entry != NULL)
   {
      //Clear the entry
      osMemset(entry, 0, sizeof(SnmpGroupEntry));
      //Now mark the entry as free
      entry->status = MIB_ROW_STATUS_UNUSED;

      //Successful processing
      error = NO_ERROR;
   }
   else
   {
      //The specified entry does not exist
      error = ERROR_NOT_FOUND;
   }

   //Release exclusive access to the SNMP agent context
   osReleaseMutex(&context->mutex);

   //Return status code
   return error;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Create access policy for the specified group name
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] groupName NULL-terminated string that contains the group name
 * @param[in] securityModel Security model
 * @param[in] securityLevel Security level
 * @param[in] contextPrefix NULL-terminated string that contains the context name prefix
 * @param[in] contextMatch Context match
 * @param[in] readViewName NULL-terminated string that contains the read view name
 * @param[in] writeViewName NULL-terminated string that contains the write view name
 * @param[in] notifyViewName NULL-terminated string that contains the notify view name
 * @return Error code
 **/

error_t snmpAgentCreateAccess(SnmpAgentContext *context,
   const char_t *groupName, SnmpSecurityModel securityModel,
   SnmpSecurityLevel securityLevel, const char_t *contextPrefix,
   SnmpContextMatch contextMatch, const char_t *readViewName,
   const char_t *writeViewName, const char_t *notifyViewName)
{
#if (SNMP_AGENT_VACM_SUPPORT == ENABLED)
   error_t error;
   size_t n;
   SnmpAccessEntry *entry;

   //Check parameters
   if(context == NULL || groupName == NULL || contextPrefix == NULL)
      return ERROR_INVALID_PARAMETER;
   if(readViewName == NULL || writeViewName == NULL || notifyViewName == NULL)
      return ERROR_INVALID_PARAMETER;

   //Check security model
   if(securityModel != SNMP_SECURITY_MODEL_ANY &&
      securityModel != SNMP_SECURITY_MODEL_V1 &&
      securityModel != SNMP_SECURITY_MODEL_V2C &&
      securityModel != SNMP_SECURITY_MODEL_USM)
   {
      return ERROR_INVALID_PARAMETER;
   }

   //Check security level
   if(securityLevel != SNMP_SECURITY_LEVEL_NO_AUTH_NO_PRIV &&
      securityLevel != SNMP_SECURITY_LEVEL_AUTH_NO_PRIV &&
      securityLevel != SNMP_SECURITY_LEVEL_AUTH_PRIV)
   {
      return ERROR_INVALID_PARAMETER;
   }

   //Check context match
   if(contextMatch != SNMP_CONTEXT_MATCH_EXACT &&
      contextMatch != SNMP_CONTEXT_MATCH_PREFIX)
   {
      return ERROR_INVALID_PARAMETER;
   }

   //Retrieve the length of the group name
   n = osStrlen(groupName);

   //Make sure the group name is valid
   if(n == 0 || n > SNMP_MAX_GROUP_NAME_LEN)
      return ERROR_INVALID_LENGTH;

   //Make sure the context name prefix is valid
   if(osStrlen(contextPrefix) > SNMP_MAX_CONTEXT_NAME_LEN)
      return ERROR_INVALID_LENGTH;

   //Make sure the read view name is valid
   if(osStrlen(readViewName) > SNMP_MAX_VIEW_NAME_LEN)
      return ERROR_INVALID_LENGTH;

   //Make sure the write view name is valid
   if(osStrlen(writeViewName) > SNMP_MAX_VIEW_NAME_LEN)
      return ERROR_INVALID_LENGTH;

   //Make sure the notify view name is valid
   if(osStrlen(notifyViewName) > SNMP_MAX_VIEW_NAME_LEN)
      return ERROR_INVALID_LENGTH;

   //Acquire exclusive access to the SNMP agent context
   osAcquireMutex(&context->mutex);

   //Search the access table for a matching entry
   entry = snmpFindAccessEntry(context, groupName, contextPrefix,
      securityModel, securityLevel);

   //No matching entry found?
   if(entry == NULL)
   {
      //Create a new entry in the access table
      entry = snmpCreateAccessEntry(context);
   }

   //Any entry available?
   if(entry != NULL)
   {
      //Clear entry
      osMemset(entry, 0, sizeof(SnmpAccessEntry));

      //Save group name
      osStrcpy(entry->groupName, groupName);
      //Save context name prefix
      osStrcpy(entry->contextPrefix, contextPrefix);
      //Save security model
      entry->securityModel = securityModel;
      //Save security level
      entry->securityLevel = securityLevel;
      //Save context match
      entry->contextMatch = contextMatch;
      //Save read view name
      osStrcpy(entry->readViewName, readViewName);
      //Save write view name
      osStrcpy(entry->writeViewName, writeViewName);
      //Save notify view name
      osStrcpy(entry->notifyViewName, notifyViewName);

      //The entry is now available for use
      entry->status = MIB_ROW_STATUS_ACTIVE;

      //Successful processing
      error = NO_ERROR;
   }
   else
   {
      //The access table runs out of space
      error = ERROR_OUT_OF_RESOURCES;
   }

   //Release exclusive access to the SNMP agent context
   osReleaseMutex(&context->mutex);

   //Return status code
   return error;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Delete an existing access policy
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] groupName NULL-terminated string that contains the group name
 * @param[in] securityModel Security model
 * @param[in] securityLevel Security level
 * @param[in] contextPrefix NULL-terminated string that contains the context name prefix
 * @return Error code
 **/

error_t snmpAgentDeleteAccess(SnmpAgentContext *context,
   const char_t *groupName, SnmpSecurityModel securityModel,
   SnmpSecurityLevel securityLevel, const char_t *contextPrefix)
{
#if (SNMP_AGENT_VACM_SUPPORT == ENABLED)
   error_t error;
   SnmpAccessEntry *entry;

   //Check parameters
   if(context == NULL || groupName == NULL || contextPrefix == NULL)
      return ERROR_INVALID_PARAMETER;

   //Acquire exclusive access to the SNMP agent context
   osAcquireMutex(&context->mutex);

   //Search the access table for a matching entry
   entry = snmpFindAccessEntry(context, groupName, contextPrefix,
      securityModel, securityLevel);

   //Any matching entry found?
   if(entry != NULL)
   {
      //Clear the entry
      osMemset(entry, 0, sizeof(SnmpAccessEntry));
      //Now mark the entry as free
      entry->status = MIB_ROW_STATUS_UNUSED;

      //Successful processing
      error = NO_ERROR;
   }
   else
   {
      //The specified entry does not exist
      error = ERROR_NOT_FOUND;
   }

   //Release exclusive access to the SNMP agent context
   osReleaseMutex(&context->mutex);

   //Return status code
   return error;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Create a new MIB view
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] viewName NULL-terminated string that contains the view name
 * @param[in] subtree Pointer to the subtree
 * @param[in] subtreeLen Length of the subtree, in bytes
 * @param[in] mask Pointer to the bit mask
 * @param[in] maskLen Length of the bit mask
 * @param[in] type View type
 * @return Error code
 **/

error_t snmpAgentCreateView(SnmpAgentContext *context,
   const char_t *viewName, const uint8_t *subtree, size_t subtreeLen,
   const uint8_t *mask, size_t maskLen, SnmpViewType type)
{
#if (SNMP_AGENT_VACM_SUPPORT == ENABLED)
   error_t error;
   size_t n;
   SnmpViewEntry *entry;

   //Check parameters
   if(context == NULL || viewName == NULL || subtree == NULL)
      return ERROR_INVALID_PARAMETER;

   //Check view type
   if(type != SNMP_VIEW_TYPE_INCLUDED &&
      type != SNMP_VIEW_TYPE_EXCLUDED)
   {
      return ERROR_INVALID_PARAMETER;
   }

   //Retrieve the length of the view name
   n = osStrlen(viewName);

   //Make sure the view name is valid
   if(n == 0 || n > SNMP_MAX_VIEW_NAME_LEN)
      return ERROR_INVALID_LENGTH;

   //Make sure the subtree is valid
   if(subtreeLen == 0 || subtreeLen > MIB_MAX_OID_SIZE)
      return ERROR_INVALID_PARAMETER;

   //Make sure the bit mask is valid
   if(maskLen > 0 && mask == NULL)
      return ERROR_INVALID_PARAMETER;
   if(maskLen > SNMP_MAX_BIT_MASK_SIZE)
      return ERROR_INVALID_PARAMETER;

   //Acquire exclusive access to the SNMP agent context
   osAcquireMutex(&context->mutex);

   //Search the view table for a matching entry
   entry = snmpFindViewEntry(context, viewName, subtree, subtreeLen);

   //No matching entry found?
   if(entry == NULL)
   {
      //Create a new entry in the view table
      entry = snmpCreateViewEntry(context);
   }

   //Any entry available?
   if(entry != NULL)
   {
      //Clear entry
      osMemset(entry, 0, sizeof(SnmpViewEntry));

      //Save view name
      osStrcpy(entry->viewName, viewName);
      //Save subtree
      osMemcpy(entry->subtree, subtree, subtreeLen);
      //Save the length of the subtree
      entry->subtreeLen = subtreeLen;
      //Save bit mask
      osMemcpy(entry->mask, mask, maskLen);
      //Save the length of the bit mask
      entry->maskLen = maskLen;
      //Save type
      entry->type = type;

      //The entry is now available for use
      entry->status = MIB_ROW_STATUS_ACTIVE;

      //Successful processing
      error = NO_ERROR;
   }
   else
   {
      //The view table runs out of space
      error = ERROR_OUT_OF_RESOURCES;
   }

   //Release exclusive access to the SNMP agent context
   osReleaseMutex(&context->mutex);

   //Return status code
   return error;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Delete an existing MIB view
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] viewName NULL-terminated string that contains the view name
 * @param[in] subtree Pointer to the subtree
 * @param[in] subtreeLen Length of the subtree, in bytes
 * @return Error code
 **/

error_t snmpAgentDeleteView(SnmpAgentContext *context,
   const char_t *viewName, const uint8_t *subtree, size_t subtreeLen)
{
#if (SNMP_AGENT_VACM_SUPPORT == ENABLED)
   error_t error;
   SnmpViewEntry *entry;

   //Check parameters
   if(context == NULL || viewName == NULL || subtree == NULL)
      return ERROR_INVALID_PARAMETER;

   //Acquire exclusive access to the SNMP agent context
   osAcquireMutex(&context->mutex);

   //Search the view table for a matching entry
   entry = snmpFindViewEntry(context, viewName, subtree, subtreeLen);

   //Any matching entry found?
   if(entry != NULL)
   {
      //Clear the entry
      osMemset(entry, 0, sizeof(SnmpViewEntry));
      //Now mark the entry as free
      entry->status = MIB_ROW_STATUS_UNUSED;

      //Successful processing
      error = NO_ERROR;
   }
   else
   {
      //The specified entry does not exist
      error = ERROR_NOT_FOUND;
   }

   //Release exclusive access to the SNMP agent context
   osReleaseMutex(&context->mutex);

   //Return status code
   return error;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Send SNMP trap notification
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] destIpAddr Destination IP address
 * @param[in] version SNMP version identifier
 * @param[in] userName User name or community name
 * @param[in] genericTrapType Generic trap type
 * @param[in] specificTrapCode Specific code
 * @param[in] objectList List of object names
 * @param[in] objectListSize Number of entries in the list
 * @return Error code
 **/

error_t snmpAgentSendTrap(SnmpAgentContext *context, const IpAddr *destIpAddr,
   SnmpVersion version, const char_t *userName, uint_t genericTrapType,
   uint_t specificTrapCode, const SnmpTrapObject *objectList, uint_t objectListSize)
{
#if (SNMP_AGENT_TRAP_SUPPORT == ENABLED)
   error_t error;

   //Check parameters
   if(context == NULL || destIpAddr == NULL || userName == NULL)
      return ERROR_INVALID_PARAMETER;

   //Make sure the list of objects is valid
   if(objectListSize > 0 && objectList == NULL)
      return ERROR_INVALID_PARAMETER;

   //Acquire exclusive access to the SNMP agent context
   osAcquireMutex(&context->mutex);

#if (SNMP_V3_SUPPORT == ENABLED)
   //Refresh SNMP engine time
   snmpRefreshEngineTime(context);
#endif

   //Format Trap message
   error = snmpFormatTrapMessage(context, version, userName,
      genericTrapType, specificTrapCode, objectList, objectListSize);

   //Check status code
   if(!error)
   {
      //Total number of messages which were passed from the SNMP protocol
      //entity to the transport service
      MIB2_INC_COUNTER32(snmpGroup.snmpOutPkts, 1);

      //Debug message
      TRACE_INFO("Sending SNMP message to %s port %" PRIu16
         " (%" PRIuSIZE " bytes)...\r\n",
         ipAddrToString(destIpAddr, NULL),
         context->settings.trapPort, context->response.length);

      //Display the contents of the SNMP message
      TRACE_DEBUG_ARRAY("  ", context->response.pos, context->response.length);
      //Display ASN.1 structure
      asn1DumpObject(context->response.pos, context->response.length, 0);

      //Send SNMP message
      error = socketSendTo(context->socket, destIpAddr, context->settings.trapPort,
         context->response.pos, context->response.length, NULL, 0);
   }

   //Release exclusive access to the SNMP agent context
   osReleaseMutex(&context->mutex);

   //Return status code
   return error;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Send SNMP inform request
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] destIpAddr Destination IP address
 * @param[in] version SNMP version identifier
 * @param[in] userName User name or community name
 * @param[in] genericTrapType Generic trap type
 * @param[in] specificTrapCode Specific code
 * @param[in] objectList List of object names
 * @param[in] objectListSize Number of entries in the list
 * @return Error code
 **/

error_t snmpAgentSendInform(SnmpAgentContext *context, const IpAddr *destIpAddr,
   SnmpVersion version, const char_t *userName, uint_t genericTrapType,
   uint_t specificTrapCode, const SnmpTrapObject *objectList, uint_t objectListSize)
{
#if (SNMP_AGENT_INFORM_SUPPORT == ENABLED)
   error_t error;
   bool_t status;

   //Check parameters
   if(context == NULL || destIpAddr == NULL || userName == NULL)
      return ERROR_INVALID_PARAMETER;

   //Make sure the list of objects is valid
   if(objectListSize > 0 && objectList == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Acquire exclusive access to the SNMP agent context
   osAcquireMutex(&context->mutex);

   //Send an inform request and wait for the acknowledgment to be received
   while(!error)
   {
      //Check current state
      if(context->informState == SNMP_AGENT_STATE_IDLE)
      {
         //Reset event object
         osResetEvent(&context->informEvent);
         //Initialize retransmission counter
         context->informRetransmitCount = 0;

#if (SNMP_V3_SUPPORT == ENABLED)
         //SNMPv3 version?
         if(version == SNMP_VERSION_3)
         {
            //The User-based Security Model (USM) of SNMPv3 provides a mechanism
            //to discover the snmpEngineID of the remote SNMP engine
            context->informContextEngineLen = 0;
            context->informEngineBoots = 0;
            context->informEngineTime = 0;

            //Perform discovery process
            context->informState = SNMP_AGENT_STATE_SENDING_GET_REQ;
         }
         else
#endif
         //SNMPv2c version?
         {
            //Send an inform request message
            context->informState = SNMP_AGENT_STATE_SENDING_INFORM_REQ;
         }
      }
#if (SNMP_V3_SUPPORT == ENABLED)
      else if(context->informState == SNMP_AGENT_STATE_SENDING_GET_REQ)
      {
         //Format GetRequest message
         error = snmpFormatGetRequestMessage(context, version);

         //Check status
         if(!error)
         {
            //Total number of messages which were passed from the SNMP protocol
            //entity to the transport service
            MIB2_INC_COUNTER32(snmpGroup.snmpOutPkts, 1);

            //Debug message
            TRACE_INFO("Sending SNMP message to %s port %" PRIu16
               " (%" PRIuSIZE " bytes)...\r\n",
               ipAddrToString(destIpAddr, NULL),
               context->settings.trapPort, context->response.length);

            //Display the contents of the SNMP message
            TRACE_DEBUG_ARRAY("  ", context->response.pos, context->response.length);
            //Display ASN.1 structure
            asn1DumpObject(context->response.pos, context->response.length, 0);

            //Send SNMP message
            error = socketSendTo(context->socket, destIpAddr, context->settings.trapPort,
               context->response.pos, context->response.length, NULL, 0);
         }

         //Check status code
         if(!error)
         {
            //Save the time at which the GetResponse-PDU was sent
            context->informTimestamp = osGetSystemTime();
            //Increment retransmission counter
            context->informRetransmitCount++;
            //Wait for an Report-PDU to be received
            context->informState = SNMP_AGENT_STATE_WAITING_REPORT;
         }
         else
         {
            //Back to default state
            context->informState = SNMP_AGENT_STATE_IDLE;
         }
      }
      else if(context->informState == SNMP_AGENT_STATE_WAITING_REPORT)
      {
         //Release exclusive access to the SNMP agent context
         osReleaseMutex(&context->mutex);

         //Wait for a matching Report-PDU to be received
         status = osWaitForEvent(&context->informEvent,
            SNMP_AGENT_INFORM_TIMEOUT);

         //Acquire exclusive access to the SNMP agent context
         osAcquireMutex(&context->mutex);

         //Any Report-PDU received?
         if(status && context->informContextEngineLen > 0)
         {
            //Reset event object
            osResetEvent(&context->informEvent);
            //Initialize retransmission counter
            context->informRetransmitCount = 0;
            //Send an inform request message
            context->informState = SNMP_AGENT_STATE_SENDING_INFORM_REQ;
         }
         else
         {
#if (NET_RTOS_SUPPORT == DISABLED)
            //Get current time
            systime_t time = osGetSystemTime();

            //Check current time
            if(timeCompare(time, context->informTimestamp + SNMP_AGENT_INFORM_TIMEOUT) < 0)
            {
               //Exit immediately
               error = ERROR_WOULD_BLOCK;
            }
            else
#endif
            {
               //The request should be retransmitted if no corresponding response
               //is received in an appropriate time interval
               if(context->informRetransmitCount < SNMP_AGENT_INFORM_MAX_RETRIES)
               {
                  //Retransmit the request
                  context->informState = SNMP_AGENT_STATE_SENDING_GET_REQ;
               }
               else
               {
                  //Back to default state
                  context->informState = SNMP_AGENT_STATE_IDLE;
                  //Report a timeout error
                  error = ERROR_TIMEOUT;
               }
            }
         }
      }
#endif
      else if(context->informState == SNMP_AGENT_STATE_SENDING_INFORM_REQ)
      {
         //Format InformRequest message
         error = snmpFormatInformRequestMessage(context, version, userName,
            genericTrapType, specificTrapCode, objectList, objectListSize);

         //Check status code
         if(!error)
         {
            //Total number of messages which were passed from the SNMP protocol
            //entity to the transport service
            MIB2_INC_COUNTER32(snmpGroup.snmpOutPkts, 1);

            //Debug message
            TRACE_INFO("Sending SNMP message to %s port %" PRIu16
               " (%" PRIuSIZE " bytes)...\r\n",
               ipAddrToString(destIpAddr, NULL),
               context->settings.trapPort, context->response.length);

            //Display the contents of the SNMP message
            TRACE_DEBUG_ARRAY("  ", context->response.pos, context->response.length);
            //Display ASN.1 structure
            asn1DumpObject(context->response.pos, context->response.length, 0);

            //Send SNMP message
            error = socketSendTo(context->socket, destIpAddr, context->settings.trapPort,
               context->response.pos, context->response.length, NULL, 0);
         }

         //Check status code
         if(!error)
         {
            //Save the time at which the InformRequest-PDU was sent
            context->informTimestamp = osGetSystemTime();
            //Increment retransmission counter
            context->informRetransmitCount++;
            //Wait for a GetResponse-PDU to be received
            context->informState = SNMP_AGENT_STATE_WAITING_GET_RESP;
         }
         else
         {
            //Back to default state
            context->informState = SNMP_AGENT_STATE_IDLE;
         }
      }
      else if(context->informState == SNMP_AGENT_STATE_WAITING_GET_RESP)
      {
         //Release exclusive access to the SNMP agent context
         osReleaseMutex(&context->mutex);

         //Wait for a matching GetResponse-PDU to be received
         status = osWaitForEvent(&context->informEvent,
            SNMP_AGENT_INFORM_TIMEOUT);

         //Acquire exclusive access to the SNMP agent context
         osAcquireMutex(&context->mutex);

         //Any GetResponse-PDU received?
         if(status)
         {
            //Back to default state
            context->informState = SNMP_AGENT_STATE_IDLE;
            //The inform request has been acknowledged
            error = NO_ERROR;
            //We are done
            break;
         }
         else
         {
#if (NET_RTOS_SUPPORT == DISABLED)
            //Get current time
            systime_t time = osGetSystemTime();

            //Check current time
            if(timeCompare(time, context->informTimestamp + SNMP_AGENT_INFORM_TIMEOUT) < 0)
            {
               //Exit immediately
               error = ERROR_WOULD_BLOCK;
            }
            else
#endif
            {
               //The request should be retransmitted if no corresponding response
               //is received in an appropriate time interval
               if(context->informRetransmitCount < SNMP_AGENT_INFORM_MAX_RETRIES)
               {
                  //Retransmit the request
                  context->informState = SNMP_AGENT_STATE_SENDING_INFORM_REQ;
               }
               else
               {
                  //Back to default state
                  context->informState = SNMP_AGENT_STATE_IDLE;
                  //Report a timeout error
                  error = ERROR_TIMEOUT;
               }
            }
         }
      }
      else
      {
         //Back to default state
         context->informState = SNMP_AGENT_STATE_IDLE;
         //Report an error
         error = ERROR_WRONG_STATE;
      }
   }

   //Release exclusive access to the SNMP agent context
   osReleaseMutex(&context->mutex);

   //Return status code
   return error;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief SNMP agent task
 * @param[in] context Pointer to the SNMP agent context
 **/

void snmpAgentTask(SnmpAgentContext *context)
{
   error_t error;

#if (NET_RTOS_SUPPORT == ENABLED)
   //Task prologue
   osEnterTask();

   //Main loop
   while(1)
   {
#endif
      //Wait for an incoming datagram
      error = socketReceiveFrom(context->socket, &context->remoteIpAddr,
         &context->remotePort, context->request.buffer,
         SNMP_MAX_MSG_SIZE, &context->request.bufferLen, 0);

      //Any datagram received?
      if(!error)
      {
         //Acquire exclusive access to the SNMP agent context
         osAcquireMutex(&context->mutex);

         //Debug message
         TRACE_INFO("\r\nSNMP message received from %s port %" PRIu16
            " (%" PRIuSIZE " bytes)...\r\n",
            ipAddrToString(&context->remoteIpAddr, NULL),
            context->remotePort, context->request.bufferLen);

         //Display the contents of the SNMP message
         TRACE_DEBUG_ARRAY("  ", context->request.buffer, context->request.bufferLen);
         //Dump ASN.1 structure
         asn1DumpObject(context->request.buffer, context->request.bufferLen, 0);

         //Process incoming SNMP message
         error = snmpProcessMessage(context);

         //Check status code
         if(!error)
         {
            //Any response?
            if(context->response.length > 0)
            {
               //Debug message
               TRACE_INFO("Sending SNMP message to %s port %" PRIu16
                  " (%" PRIuSIZE " bytes)...\r\n",
                  ipAddrToString(&context->remoteIpAddr, NULL),
                  context->remotePort, context->response.length);

               //Display the contents of the SNMP message
               TRACE_DEBUG_ARRAY("  ", context->response.pos, context->response.length);
               //Display ASN.1 structure
               asn1DumpObject(context->response.pos, context->response.length, 0);

               //Send SNMP response message
               socketSendTo(context->socket, &context->remoteIpAddr,
                  context->remotePort, context->response.pos,
                  context->response.length, NULL, 0);
            }
         }

         //Release exclusive access to the SNMP agent context
         osReleaseMutex(&context->mutex);
      }
#if (NET_RTOS_SUPPORT == ENABLED)
   }
#endif
}

#endif
