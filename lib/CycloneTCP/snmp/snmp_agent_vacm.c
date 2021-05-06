/**
 * @file snmp_agent_vacm.c
 * @brief View-based Access Control Model (VACM) for SNMP
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
 * This module implements the View-based Access Control Model (VACM) for Simple
 * Network Management Protocol (SNMP). Refer to RFC 3415 for complete details
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

//Switch to the appropriate trace level
#define TRACE_LEVEL SNMP_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "snmp/snmp_agent.h"
#include "snmp/snmp_agent_vacm.h"
#include "core/crypto.h"
#include "encoding/asn1.h"
#include "encoding/oid.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (SNMP_AGENT_SUPPORT == ENABLED && SNMP_AGENT_VACM_SUPPORT == ENABLED)


/**
 * @brief Access control verification
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] message Pointer to the received SNMP message
 * @param[in] oid OID for the managed object
 * @param[in] oidLen Length of the OID, in bytes
 * @return Error code
 **/

error_t snmpIsAccessAllowed(SnmpAgentContext *context,
   const SnmpMessage *message, const uint8_t *oid, size_t oidLen)
{
   SnmpSecurityModel securityModel;
   SnmpSecurityLevel securityLevel;
   const char_t *securityName;
   size_t securityNameLen;
   const char_t *contextName;
   size_t contextNameLen;
   const char_t *viewName;
   const SnmpGroupEntry *groupEntry;
   const SnmpAccessEntry *accessEntry;
   const SnmpViewEntry *viewEntry;

#if (SNMP_V1_SUPPORT == ENABLED)
   //SNMPv1 version?
   if(message->version == SNMP_VERSION_1)
   {
      //Set security parameters
      securityModel = SNMP_SECURITY_MODEL_V1;
      securityLevel = SNMP_SECURITY_LEVEL_NO_AUTH_NO_PRIV;
      securityName = message->community;
      securityNameLen = message->communityLen;
      contextName = context->contextName;
      contextNameLen = osStrlen(context->contextName);
   }
   else
#endif
#if (SNMP_V2C_SUPPORT == ENABLED)
   //SNMPv2c version?
   if(message->version == SNMP_VERSION_2C)
   {
      //Set security parameters
      securityModel = SNMP_SECURITY_MODEL_V2C;
      securityLevel = SNMP_SECURITY_LEVEL_NO_AUTH_NO_PRIV;
      securityName = message->community;
      securityNameLen = message->communityLen;
      contextName = context->contextName;
      contextNameLen = osStrlen(context->contextName);
   }
   else
#endif
#if (SNMP_V3_SUPPORT == ENABLED)
   //SNMPv3 version?
   if(message->version == SNMP_VERSION_3)
   {
      //Set security parameters
      securityModel = (SnmpSecurityModel) message->msgSecurityModel;
      securityLevel = SNMP_SECURITY_LEVEL_NO_AUTH_NO_PRIV;
      securityName = message->msgUserName;
      securityNameLen = message->msgUserNameLen;
      contextName = message->contextName;
      contextNameLen = message->contextNameLen;

      //Check whether the authFlag is set
      if((message->msgFlags & SNMP_MSG_FLAG_AUTH) != 0)
      {
         //Check whether the privFlag is set
         if((message->msgFlags & SNMP_MSG_FLAG_PRIV) != 0)
         {
            securityLevel = SNMP_SECURITY_LEVEL_AUTH_PRIV;
         }
         else
         {
            securityLevel = SNMP_SECURITY_LEVEL_AUTH_NO_PRIV;
         }
      }
   }
   else
#endif
   //Invalid SNMP version?
   {
      //Report an error
      return ERROR_INVALID_VERSION;
   }

   //The vacmContextTable is consulted for information about the SNMP
   //context identified by the contextName. If information about this
   //SNMP context is absent from the table, then an errorIndication
   //(noSuchContext) is returned to the calling module
   if(contextNameLen != osStrlen(context->contextName))
      return ERROR_UNKNOWN_CONTEXT;

   //Check context name
   if(osStrncmp(contextName, context->contextName, contextNameLen))
      return ERROR_UNKNOWN_CONTEXT;

   //The vacmSecurityToGroupTable is consulted for mapping the securityModel
   //and securityName to a groupName
   groupEntry = snmpFindGroupEntry(context, securityModel,
      securityName, securityNameLen);

   //If the information about this combination is absent from the table, then
   //an errorIndication (noGroupName) is returned to the calling module
   if(groupEntry == NULL)
      return ERROR_AUTHORIZATION_FAILED;

   //The vacmAccessTable is consulted for information about the groupName,
   //contextName, securityModel and securityLevel
   accessEntry = snmpSelectAccessEntry(context, groupEntry->groupName,
      contextName, contextNameLen, securityModel, securityLevel);

   //If information about this combination is absent from the table, then
   //an errorIndication (noAccessEntry) is returned to the calling module
   if(accessEntry == NULL)
      return ERROR_AUTHORIZATION_FAILED;

   //Select the proper MIB view
   if(message->pduType == SNMP_PDU_GET_REQUEST ||
      message->pduType == SNMP_PDU_GET_NEXT_REQUEST ||
      message->pduType == SNMP_PDU_GET_BULK_REQUEST)
   {
      //The read view is used for checking access rights
      viewName = accessEntry->readViewName;
   }
   else if(message->pduType == SNMP_PDU_SET_REQUEST)
   {
      //The write view is used for checking access rights
      viewName = accessEntry->writeViewName;
   }
   else if(message->pduType == SNMP_PDU_TRAP ||
      message->pduType == SNMP_PDU_TRAP_V2 ||
      message->pduType == SNMP_PDU_INFORM_REQUEST)
   {
      //The notify view is used for checking access rights
      viewName = accessEntry->notifyViewName;
   }
   else
   {
      //Report an error
      return ERROR_AUTHORIZATION_FAILED;
   }

   //If the view to be used is the empty view (zero length viewName) then
   //an errorIndication (noSuchView) is returned to the calling module
   if(viewName[0] == '\0')
      return ERROR_AUTHORIZATION_FAILED;

   //Check whether the specified variableName is in the MIB view
   viewEntry = snmpSelectViewEntry(context, viewName, oid, oidLen);

   //If there is no view configured for the specified viewType, then an
   //errorIndication (noSuchView) is returned to the calling module
   if(viewEntry == NULL)
      return ERROR_AUTHORIZATION_FAILED;

   //If the specified variableName (object instance) is not in the MIB view,
   //then an errorIndication (notInView) is returned to the calling module
   if(viewEntry->type != SNMP_VIEW_TYPE_INCLUDED)
      return ERROR_AUTHORIZATION_FAILED;

   //Otherwise, the specified variableName is in the MIB view
   return NO_ERROR;
}


/**
 * @brief Create a new group entry
 * @param[in] context Pointer to the SNMP agent context
 * @return Pointer to the newly created entry
 **/

SnmpGroupEntry *snmpCreateGroupEntry(SnmpAgentContext *context)
{
   uint_t i;
   SnmpGroupEntry *entry;

   //Initialize pointer
   entry = NULL;

   //Loop through the list of groups
   for(i = 0; i < SNMP_AGENT_GROUP_TABLE_SIZE; i++)
   {
      //Check current status
      if(context->groupTable[i].status == MIB_ROW_STATUS_UNUSED)
      {
         //An unused entry has been found
         entry = &context->groupTable[i];
         //We are done
         break;
      }
   }

   //Check whether the group table runs out of space
   if(entry == NULL)
   {
      //Loop through the list of groups
      for(i = 0; i < SNMP_AGENT_GROUP_TABLE_SIZE; i++)
      {
         //Check current status
         if(context->groupTable[i].status == MIB_ROW_STATUS_NOT_READY)
         {
            //Reuse the current entry
            entry = &context->groupTable[i];
            //We are done
            break;
         }
      }
   }

   //Return a pointer to the newly created entry
   return entry;
}


/**
 * @brief Search the group table
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] securityModel Security model
 * @param[in] securityName Pointer to the security name
 * @param[in] securityNameLen Length of the security name
 * @return Pointer to the matching entry
 **/

SnmpGroupEntry *snmpFindGroupEntry(SnmpAgentContext *context,
   uint_t securityModel, const char_t *securityName, size_t securityNameLen)
{
   uint_t i;
   SnmpGroupEntry *entry;

   //Loop through the list of groups
   for(i = 0; i < SNMP_AGENT_GROUP_TABLE_SIZE; i++)
   {
      //Point to the current entry
      entry = &context->groupTable[i];

      //Check current status
      if(entry->status != MIB_ROW_STATUS_UNUSED)
      {
         //Compare security model
         if(entry->securityModel == securityModel)
         {
            //Check the length of the security name
            if(osStrlen(entry->securityName) == securityNameLen)
            {
               //Compare security name
               if(!osStrncmp(entry->securityName, securityName, securityNameLen))
               {
                  //A matching entry has been found
                  break;
               }
            }
         }
      }
   }

   //Any matching entry found?
   if(i < SNMP_AGENT_GROUP_TABLE_SIZE)
      return entry;
   else
      return NULL;
}


/**
 * @brief Create a new access entry
 * @param[in] context Pointer to the SNMP agent context
 * @return Pointer to the newly created entry
 **/

SnmpAccessEntry *snmpCreateAccessEntry(SnmpAgentContext *context)
{
   uint_t i;
   SnmpAccessEntry *entry;

   //Initialize pointer
   entry = NULL;

   //Loop through the list of access rights
   for(i = 0; i < SNMP_AGENT_ACCESS_TABLE_SIZE; i++)
   {
      //Check current status
      if(context->accessTable[i].status == MIB_ROW_STATUS_UNUSED)
      {
         //An unused entry has been found
         entry = &context->accessTable[i];
         //We are done
         break;
      }
   }

   //Check whether the group table runs out of space
   if(entry == NULL)
   {
      //Loop through the list of access rights
      for(i = 0; i < SNMP_AGENT_ACCESS_TABLE_SIZE; i++)
      {
         //Check current status
         if(context->accessTable[i].status == MIB_ROW_STATUS_NOT_READY)
         {
            //Reuse the current entry
            entry = &context->accessTable[i];
            //We are done
            break;
         }
      }
   }

   //Return a pointer to the newly created entry
   return entry;
}


/**
 * @brief Search the access table for a given entry
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] groupName Group name
 * @param[in] contextPrefix Context name prefix
 * @param[in] securityModel Security model
 * @param[in] securityLevel Security level
 * @return Pointer to the matching entry
 **/

SnmpAccessEntry *snmpFindAccessEntry(SnmpAgentContext *context,
   const char_t *groupName, const char_t *contextPrefix,
   uint_t securityModel, uint_t securityLevel)
{
   uint_t i;
   SnmpAccessEntry *entry;

   //Loop through the list of access rights
   for(i = 0; i < SNMP_AGENT_ACCESS_TABLE_SIZE; i++)
   {
      //Point to the current entry
      entry = &context->accessTable[i];

      //Check current status
      if(entry->status != MIB_ROW_STATUS_UNUSED)
      {
         //Compare group name
         if(!osStrcmp(entry->groupName, groupName))
         {
            //Compare context name prefix
            if(!osStrcmp(entry->contextPrefix, contextPrefix))
            {
               //Compare security model and security level
               if(entry->securityModel == securityModel &&
                  entry->securityLevel == securityLevel)
               {
                  //A matching entry has been found
                  break;
               }
            }
         }
      }
   }

   //Return a pointer to the matching entry
   if(i < SNMP_AGENT_ACCESS_TABLE_SIZE)
      return entry;
   else
      return NULL;
}


/**
 * @brief Find an access entry that matches the selection criteria
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] groupName NULL-terminated string that contains the group name
 * @param[in] contextName Pointer to the context name
 * @param[in] contextNameLen Length of the context name
 * @param[in] securityModel Security model
 * @param[in] securityLevel Security level
 * @return Pointer to the matching entry
 **/

SnmpAccessEntry *snmpSelectAccessEntry(SnmpAgentContext *context,
   const char_t *groupName, const char_t *contextName, size_t contextNameLen,
   SnmpSecurityModel securityModel, SnmpSecurityLevel securityLevel)
{
   uint_t i;
   size_t n;
   bool_t acceptable;
   SnmpAccessEntry *entry;
   SnmpAccessEntry *selectedEntry;

   //Initialize pointer
   selectedEntry = NULL;

   //Loop through the list of access rights
   for(i = 0; i < SNMP_AGENT_ACCESS_TABLE_SIZE; i++)
   {
      //Point to the current entry
      entry = &context->accessTable[i];

      //Check current status
      if(entry->status == MIB_ROW_STATUS_UNUSED)
         continue;

      //Compare group name
      if(osStrcmp(entry->groupName, groupName))
         continue;

      //Compare security model
      if(entry->securityModel != SNMP_SECURITY_MODEL_ANY)
      {
         if(entry->securityModel != securityModel)
            continue;
      }

      //Compare security level
      if(entry->securityLevel > securityLevel)
         continue;

      //Retrieve the length of the context name prefix
      n = osStrlen(entry->contextPrefix);

      //Check the length of the context name prefix
      if(n > contextNameLen)
         continue;

      //Compare context name prefix
      if(osStrncmp(entry->contextPrefix, contextName, n))
         continue;

      //Exact match?
      if(entry->contextMatch == SNMP_CONTEXT_MATCH_EXACT)
      {
         //The contextName must match exactly
         if(n != contextNameLen)
            continue;
      }

      //If this set has only one member, we're done otherwise, it comes down
      //to deciding how to weight the preferences between ContextPrefixes,
      //SecurityModels, and SecurityLevels (refer to RFC 3415, section 4)
      if(selectedEntry == NULL)
         acceptable = TRUE;
      else if(entry->securityModel == SNMP_SECURITY_MODEL_ANY)
         acceptable = FALSE;
      else if(selectedEntry->securityModel == SNMP_SECURITY_MODEL_ANY)
         acceptable = TRUE;
      else if(osStrlen(selectedEntry->contextPrefix) == contextNameLen)
         acceptable = FALSE;
      else if(osStrlen(entry->contextPrefix) == contextNameLen)
         acceptable = TRUE;
      else if(osStrlen(selectedEntry->contextPrefix) > osStrlen(entry->contextPrefix))
         acceptable = FALSE;
      else if(osStrlen(entry->contextPrefix) > osStrlen(selectedEntry->contextPrefix))
         acceptable = TRUE;
      else if(selectedEntry->securityLevel >= entry->securityLevel)
         acceptable = FALSE;
      else
         acceptable = TRUE;

      //Select the proper entry
      if(acceptable)
         selectedEntry = entry;
   }

   //Return a pointer to the matching entry
   return selectedEntry;
}


/**
 * @brief Create a new view entry
 * @param[in] context Pointer to the SNMP agent context
 * @return Pointer to the newly created entry
 **/

SnmpViewEntry *snmpCreateViewEntry(SnmpAgentContext *context)
{
   uint_t i;
   SnmpViewEntry *entry;

   //Initialize pointer
   entry = NULL;

   //Loop through the list of MIB views
   for(i = 0; i < SNMP_AGENT_VIEW_TABLE_SIZE; i++)
   {
      //Check current status
      if(context->viewTable[i].status == MIB_ROW_STATUS_UNUSED)
      {
         //An unused entry has been found
         entry = &context->viewTable[i];
         //We are done
         break;
      }
   }

   //Check whether the group table runs out of space
   if(entry == NULL)
   {
      //Loop through the list of MIB views
      for(i = 0; i < SNMP_AGENT_VIEW_TABLE_SIZE; i++)
      {
         //Check current status
         if(context->viewTable[i].status == MIB_ROW_STATUS_NOT_READY)
         {
            //Reuse the current entry
            entry = &context->viewTable[i];
            //We are done
            break;
         }
      }
   }

   //Return a pointer to the newly created entry
   return entry;
}


/**
 * @brief Search the view table for a given entry
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] viewName View name
 * @param[in] subtree Pointer to the MIB subtree
 * @param[in] subtreeLen Length of the MIB subtree
 * @return Pointer to the matching entry
 **/

SnmpViewEntry *snmpFindViewEntry(SnmpAgentContext *context,
   const char_t *viewName, const uint8_t *subtree, size_t subtreeLen)
{
   uint_t i;
   SnmpViewEntry *entry;

   //Loop through the list of MIB views
   for(i = 0; i < SNMP_AGENT_VIEW_TABLE_SIZE; i++)
   {
      //Point to the current entry
      entry = &context->viewTable[i];

      //Check current status
      if(entry->status != MIB_ROW_STATUS_UNUSED)
      {
         //Compare view name
         if(!osStrcmp(entry->viewName, viewName))
         {
            //Check the length of the subtree
            if(entry->subtreeLen == subtreeLen)
            {
               //Compare subtree
               if(!osMemcmp(entry->subtree, subtree, subtreeLen))
               {
                  //A matching entry has been found
                  break;
               }
            }
         }
      }
   }

   //Return a pointer to the matching entry
   if(i < SNMP_AGENT_VIEW_TABLE_SIZE)
      return entry;
   else
      return NULL;
}


/**
 * @brief Find a view entry that matches the selection criteria
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] viewName NULL-terminated string that contains the view name
 * @param[in] oid OID for the managed object
 * @param[in] oidLen Length of the OID, in bytes
 * @return Pointer to the matching entry
 **/

SnmpViewEntry *snmpSelectViewEntry(SnmpAgentContext *context,
   const char_t *viewName, const uint8_t *oid, size_t oidLen)
{
   uint_t i;
   uint_t subtreeLen;
   uint_t selectedSubtreeLen;
   bool_t acceptable;
   SnmpViewEntry *entry;
   SnmpViewEntry *selectedEntry;

   //Initialize pointer
   selectedEntry = NULL;

   //Loop through the list of MIB views
   for(i = 0; i < SNMP_AGENT_VIEW_TABLE_SIZE; i++)
   {
      //Point to the current entry
      entry = &context->viewTable[i];

      //Check current status
      if(entry->status == MIB_ROW_STATUS_UNUSED)
         continue;

      //Compare view name
      if(osStrcmp(entry->viewName, viewName))
         continue;

      //Check whether the OID matches the subtree (the mask allows for a
      //simple form of wildcarding)
      if(!oidMatch(oid, oidLen, entry->subtree, entry->subtreeLen,
         entry->mask, entry->maskLen))
      {
         continue;
      }

      //First matching entry?
      if(selectedEntry == NULL)
      {
         acceptable = TRUE;
      }
      else
      {
         //Calculate the number of sub-identifiers of the subtree
         subtreeLen = oidCountSubIdentifiers(entry->subtree,
            entry->subtreeLen);

         //Calculate the number of sub-identifiers of the currently selected
         //subtree
         selectedSubtreeLen = oidCountSubIdentifiers(selectedEntry->subtree,
            selectedEntry->subtreeLen);

         //If multiple entries match, then select the entry whose value of
         //vacmViewTreeFamilySubtree has the most sub-identifiers. If multiple
         //entries match and have the same number of sub-identifiers, then the
         //lexicographically greatest instance of vacmViewTreeFamilyType is
         //selected
         if(selectedSubtreeLen > subtreeLen)
         {
            acceptable = FALSE;
         }
         else if(subtreeLen > selectedSubtreeLen)
         {
            acceptable = TRUE;
         }
         else if(oidComp(selectedEntry->subtree, selectedEntry->subtreeLen,
            entry->subtree, entry->subtreeLen) > 0)
         {
            acceptable = FALSE;
         }
         else if(oidComp(entry->subtree, entry->subtreeLen,
            selectedEntry->subtree, selectedEntry->subtreeLen) > 0)
         {
            acceptable = TRUE;
         }
         else if(selectedEntry->type >= entry->type)
         {
            acceptable = FALSE;
         }
         else
         {
            acceptable = TRUE;
         }
      }

      //Select the proper entry
      if(acceptable)
         selectedEntry = entry;
   }

   //Return a pointer to the matching entry
   return selectedEntry;
}

#endif
