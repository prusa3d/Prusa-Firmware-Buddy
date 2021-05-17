/**
 * @file snmp_agent_object.c
 * @brief MIB object access
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
#include "snmp/snmp_agent_object.h"
#include "core/crypto.h"
#include "encoding/asn1.h"
#include "encoding/oid.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (SNMP_AGENT_SUPPORT == ENABLED)


/**
 * @brief Assign object value
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] message Pointer to the received SNMP message
 * @param[in] var Variable binding
 * @param[in] commit This flag specifies whether the changes should be
 *   committed to the MIB base
 * @return Error code
 **/

error_t snmpSetObjectValue(SnmpAgentContext *context,
   const SnmpMessage *message, SnmpVarBind *var, bool_t commit)
{
   error_t error;
   size_t n;
   MibVariant *value;
   const MibObject *object;

   //Search the MIB for the specified object
   error = snmpFindMibObject(context, var->oid, var->oidLen, &object);
   //Cannot found the specified object?
   if(error)
      return error;

   //Debug message
   TRACE_INFO("  %s\r\n", object->name);

   //Make sure the specified object is available for set operations
   if(object->access != MIB_ACCESS_WRITE_ONLY &&
      object->access != MIB_ACCESS_READ_WRITE &&
      object->access != MIB_ACCESS_READ_CREATE)
   {
      //Report an error
      return ERROR_NOT_WRITABLE;
   }

#if (SNMP_AGENT_VACM_SUPPORT == ENABLED)
   //Access control verification
   error = snmpIsAccessAllowed(context, message, var->oid, var->oidLen);
   //Access denied?
   if(error)
      return error;
#endif

   //Check class
   if(var->objClass != object->objClass)
      return ERROR_WRONG_TYPE;
   //Check type
   if(var->objType != object->objType)
      return ERROR_WRONG_TYPE;

   //Point to the object value
   value = (MibVariant *) var->value;
   //Get the length of the object value
   n = var->valueLen;

   //Check object class
   if(object->objClass == ASN1_CLASS_UNIVERSAL)
   {
      //Check object type
      if(object->objType == ASN1_TYPE_INTEGER)
      {
         int32_t val;

         //Integer objects use ASN.1 encoding rules
         error = snmpDecodeInt32(var->value, n, &val);
         //Conversion failed?
         if(error)
            return ERROR_WRONG_ENCODING;

         //Point to the scratch buffer
         value = (MibVariant *) context->response.buffer;
         //Save resulting value
         value->integer = val;
         //Integer size
         n = sizeof(int32_t);
      }
   }
   else if(object->objClass == ASN1_CLASS_APPLICATION)
   {
      //Check object type
      if(object->objType == MIB_TYPE_IP_ADDRESS)
      {
         //IpAddress objects have fixed size
         if(n != object->valueSize)
            return ERROR_WRONG_LENGTH;
      }
      else if(object->objType == MIB_TYPE_COUNTER32 ||
         object->objType == MIB_TYPE_GAUGE32 ||
         object->objType == MIB_TYPE_TIME_TICKS)
      {
         uint32_t val;

         //Counter32, Gauge32 and TimeTicks objects use ASN.1 encoding rules
         error = snmpDecodeUnsignedInt32(var->value, n, &val);
         //Conversion failed?
         if(error)
            return ERROR_WRONG_ENCODING;

         //Point to the scratch buffer
         value = (MibVariant *) context->response.buffer;
         //Save resulting value
         value->counter32 = val;
         //Integer size
         n = sizeof(uint32_t);
      }
      else if(object->objType == MIB_TYPE_COUNTER64)
      {
         uint64_t val;

#if (SNMP_V1_SUPPORT == ENABLED)
         //Any SNMPv1 request which contains a variable binding with a
         //Counter64 value is ill-formed and shall be discarded (refer
         //to RFC 3584, section 4.2.2.1)
         if(message->version == SNMP_VERSION_1)
            return ERROR_INVALID_TAG;
#endif
         //Counter64 objects use ASN.1 encoding rules
         error = snmpDecodeUnsignedInt64(var->value, n, &val);
         //Conversion failed?
         if(error)
            return ERROR_WRONG_ENCODING;

         //Point to the scratch buffer
         value = (MibVariant *) context->response.buffer;
         //Save resulting value
         value->counter64 = val;
         //Integer size
         n = sizeof(uint64_t);
      }
   }

   //Objects can be assigned a value using a callback function
   if(object->setValue != NULL)
   {
      //Invoke callback function to assign object value
      error = object->setValue(object, var->oid, var->oidLen, value, n, commit);
   }
   //Simple scalar objects can also be attached to a variable
   else if(object->value != NULL)
   {
      //Check the length of the object
      if(n <= object->valueSize)
      {
         //Check whether the changes shall be committed to the MIB base
         if(commit)
         {
            //Record the length of the object value
            if(object->valueLen != NULL)
               *object->valueLen = n;

            //Set object value
            osMemcpy(object->value, value, n);
         }

         //Successful write operation
         error = NO_ERROR;
      }
      else
      {
         //Invalid length
         error = ERROR_WRONG_LENGTH;
      }
   }
   else
   {
      //Report an error
      error = ERROR_WRITE_FAILED;
   }

   //Return status code
   return error;
}


/**
 * @brief Retrieve object value
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] message Pointer to the received SNMP message
 * @param[out] var Variable binding
 * @return Error code
 **/

error_t snmpGetObjectValue(SnmpAgentContext *context,
   const SnmpMessage *message, SnmpVarBind *var)
{
   error_t error;
   size_t n;
   MibVariant *value;
   const MibObject *object;

   //Search the MIB for the specified object
   error = snmpFindMibObject(context, var->oid, var->oidLen, &object);
   //Cannot found the specified object?
   if(error)
      return error;

   //Debug message
   TRACE_INFO("  %s\r\n", object->name);

   //Check the maximal level of access for the specified object
   if(object->access == MIB_ACCESS_READ_ONLY ||
      object->access == MIB_ACCESS_READ_WRITE ||
      object->access == MIB_ACCESS_READ_CREATE)
   {
      //The object is available for get operations
   }
   else if(object->access == MIB_ACCESS_FOR_NOTIFY)
   {
      //The object is accessible only via a notification
      if(message->pduType != SNMP_PDU_TRAP &&
         message->pduType != SNMP_PDU_TRAP_V2 &&
         message->pduType != SNMP_PDU_INFORM_REQUEST)
      {
         //Report an error
         return ERROR_ACCESS_DENIED;
      }
   }
   else
   {
      //The object is not available for get operations
      return ERROR_ACCESS_DENIED;
   }

#if (SNMP_AGENT_VACM_SUPPORT == ENABLED)
   //Access control verification
   error = snmpIsAccessAllowed(context, message, var->oid, var->oidLen);
   //Access denied?
   if(error)
      return error;
#endif

   //Buffer where to store the object value
   value = (MibVariant *) (context->response.varBindList +
      context->response.varBindListLen + context->response.oidLen);

   //Number of bytes available in the buffer
   n = context->response.varBindListMaxLen -
      (context->response.varBindListLen + context->response.oidLen);

   //Check object class
   if(object->objClass == ASN1_CLASS_UNIVERSAL)
   {
      //Check object type
      if(object->objType == ASN1_TYPE_INTEGER)
      {
         //Make sure the buffer is large enough
         if(n < object->valueSize)
            return ERROR_BUFFER_OVERFLOW;

         //Integer objects have fixed size
         n = object->valueSize;
      }
   }
   else if(object->objClass == ASN1_CLASS_APPLICATION)
   {
      //Check object type
      if(object->objType == MIB_TYPE_IP_ADDRESS ||
         object->objType == MIB_TYPE_COUNTER32 ||
         object->objType == MIB_TYPE_GAUGE32 ||
         object->objType == MIB_TYPE_TIME_TICKS ||
         object->objType == MIB_TYPE_COUNTER64)
      {
         //Make sure the buffer is large enough
         if(n < object->valueSize)
            return ERROR_BUFFER_OVERFLOW;

         //IpAddress, Counter32, Gauge32, TimeTicks and
         //Counter64 objects have fixed size
         n = object->valueSize;
      }
   }

   //Object value can be retrieved using a callback function
   if(object->getValue != NULL)
   {
      //Invoke callback function to retrieve object value
      error = object->getValue(object, var->oid, var->oidLen, value, &n);
   }
   //Simple scalar objects can also be attached to a variable
   else if(object->value != NULL)
   {
      //Get the length of the object value
      if(object->valueLen != NULL)
         n = *object->valueLen;

      //Retrieve object value
      osMemcpy(value, object->value, n);
      //Successful read operation
      error = NO_ERROR;
   }
   else
   {
      //Report an error
      error = ERROR_READ_FAILED;
   }

   //Unable to retrieve object value?
   if(error)
      return error;

   //Check object class
   if(object->objClass == ASN1_CLASS_UNIVERSAL)
   {
      //Check object type
      if(object->objType == ASN1_TYPE_INTEGER)
      {
         //Encode Integer objects using ASN.1 rules
         error = snmpEncodeInt32(value->integer, (uint8_t *) value, &n);
      }
      else
      {
         //No conversion required for OctetString and ObjectIdentifier objects
         error = NO_ERROR;
      }
   }
   else if(object->objClass == ASN1_CLASS_APPLICATION)
   {
      //Check object type
      if(object->objType == MIB_TYPE_COUNTER32 ||
         object->objType == MIB_TYPE_GAUGE32 ||
         object->objType == MIB_TYPE_TIME_TICKS)
      {
         //Encode Counter32, Gauge32 and TimeTicks objects using ASN.1 rules
         error = snmpEncodeUnsignedInt32(value->counter32, (uint8_t *) value, &n);
      }
      else if(object->objType == MIB_TYPE_COUNTER64)
      {
#if (SNMP_V1_SUPPORT == ENABLED)
         //On receipt of an SNMPv1 GetRequest-PDU containing a variable binding
         //whose name field points to an object instance of type Counter64, a
         //GetResponse-PDU SHALL be returned, with an error-status of noSuchName
         if(message->version == SNMP_VERSION_1)
         {
            //Report an error
            error = ERROR_OBJECT_NOT_FOUND;
         }
         else
#endif
         {
            //Encode Counter64 objects using ASN.1 rules
            error = snmpEncodeUnsignedInt64(value->counter64,
               (uint8_t *) value, &n);
         }
      }
      else
      {
         //No conversion required for Opaque objects
         error = NO_ERROR;
      }
   }

   //Check status code
   if(!error)
   {
      //Save object class and type
      var->objClass = object->objClass;
      var->objType = object->objType;

      //Save object value
      var->value = (uint8_t *) value;
      var->valueLen = n;
   }

   //Return status code
   return error;
}


/**
 * @brief Search MIBs for the next object
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] message Pointer to the received SNMP message
 * @param[in] var Variable binding
 * @return Error pointer
 **/

error_t snmpGetNextObject(SnmpAgentContext *context,
   const SnmpMessage *message, SnmpVarBind *var)
{
   error_t error;
   uint_t i;
   uint_t j;
   size_t n;
   uint_t numObjects;
   size_t bufferLen;
   uint8_t *curOid;
   size_t curOidLen;
   uint8_t *nextOid;
   size_t nextOidLen;
   uint8_t *tempOid;
   size_t tempOidLen;
   const MibObject *object;
   const MibObject *nextObject;

   //Initialize status code
   error = NO_ERROR;

   //Calculate the length of the buffer
   bufferLen = context->response.varBindListMaxLen -
      context->response.varBindListLen;

   //Initialize pointer
   nextObject = NULL;

   //Buffer where to store the resulting OID
   nextOid = context->response.varBindList + context->response.varBindListLen;
   nextOidLen = 0;

   //Loop through MIBs
   for(i = 0; i < SNMP_AGENT_MAX_MIBS; i++)
   {
      //Valid MIB?
      if(context->mibTable[i] != NULL &&
         context->mibTable[i]->numObjects > 0)
      {
         //Get the total number of objects
         numObjects = context->mibTable[i]->numObjects;

         //Point to the last object of the MIB
         object = &context->mibTable[i]->objects[numObjects - 1];

         //Discard instance sub-identifier
         n = MIN(var->oidLen, object->oidLen);

         //Perform lexicographical comparison
         if(oidComp(var->oid, n, object->oid, object->oidLen) <= 0)
         {
            //Sanity check
            if((nextOidLen + var->oidLen) > bufferLen)
            {
               //Report an error
               error = ERROR_BUFFER_OVERFLOW;
               //Exit immediately
               break;
            }

            //Copy the OID from the specified variable binding
            curOid = nextOid + nextOidLen;
            curOidLen = var->oidLen;
            osMemcpy(curOid, var->oid, var->oidLen);

            //Loop through objects
            for(j = 0; j < numObjects; )
            {
               //Point to the current object
               object = &context->mibTable[i]->objects[j];

               //Buffer where to store the OID of the next object
               tempOid = curOid + curOidLen;

               //Make sure the current object is accessible
               if(object->access == MIB_ACCESS_READ_ONLY ||
                  object->access == MIB_ACCESS_READ_WRITE ||
                  object->access == MIB_ACCESS_READ_CREATE)
               {
                  //Scalar or tabular object?
                  if(object->getNext == NULL)
                  {
                     //Perform lexicographical comparison
                     if(oidComp(curOid, curOidLen, object->oid, object->oidLen) <= 0)
                     {
                        //Take in account the instance sub-identifier to determine
                        //the length of the OID
                        tempOidLen = object->oidLen + 1;

                        //Make sure the buffer is large enough to hold the entire OID
                        if((nextOidLen + curOidLen + tempOidLen) <= bufferLen)
                        {
                           //Copy object identifier
                           osMemcpy(tempOid, object->oid, object->oidLen);
                           //Append instance sub-identifier
                           tempOid[tempOidLen - 1] = 0;

                           //Successful processing
                           error = NO_ERROR;
                        }
                        else
                        {
                           //Report an error
                           error = ERROR_BUFFER_OVERFLOW;
                        }
                     }
                     else
                     {
                        //The specified OID does not lexicographically precede
                        //the name of the current object
                        error = ERROR_OBJECT_NOT_FOUND;
                     }
                  }
                  else
                  {
                     //Discard instance sub-identifier
                     n = MIN(curOidLen, object->oidLen);

                     //Perform lexicographical comparison
                     if(oidComp(curOid, n, object->oid, object->oidLen) <= 0)
                     {
                        //Maximum acceptable size of the OID
                        tempOidLen = bufferLen - nextOidLen - curOidLen;

                        //Search the MIB for the next object
                        error = object->getNext(object, curOid, curOidLen,
                           tempOid, &tempOidLen);
                     }
                     else
                     {
                        //The specified OID does not lexicographically precede
                        //the name of the current object
                        error = ERROR_OBJECT_NOT_FOUND;
                     }
                  }

#if (SNMP_V1_SUPPORT == ENABLED)
                  //Check status code
                  if(error == NO_ERROR)
                  {
                     //On receipt of an SNMPv1 GetNextRequest-PDU, any object
                     //instance which contains a syntax of Counter64 shall be
                     //skipped (refer to RFC 3584, section 4.2.2.1)
                     if(message->version == SNMP_VERSION_1)
                     {
                        //Counter64 type?
                        if(object->objClass == ASN1_CLASS_APPLICATION &&
                           object->objType == MIB_TYPE_COUNTER64)
                        {
                           //Skip current object
                           error = ERROR_OBJECT_NOT_FOUND;
                        }
                     }
                  }
#endif
#if (SNMP_AGENT_VACM_SUPPORT == ENABLED)
                  //Check status code
                  if(error == NO_ERROR)
                  {
                     //Access control verification
                     error = snmpIsAccessAllowed(context, message, tempOid,
                        tempOidLen);
                  }
#endif
                  //Check status code
                  if(error == NO_ERROR)
                  {
                     //Save the closest object identifier that follows the
                     //specified OID
                     if(nextObject == NULL)
                     {
                        nextObject = object;
                        nextOidLen = tempOidLen;
                        osMemmove(nextOid, tempOid, tempOidLen);
                     }
                     else if(oidComp(tempOid, tempOidLen, nextOid, nextOidLen) < 0)
                     {
                        nextObject = object;
                        nextOidLen = tempOidLen;
                        osMemmove(nextOid, tempOid, tempOidLen);
                     }

                     //We are done
                     break;
                  }
                  else if(error == ERROR_OBJECT_NOT_FOUND)
                  {
                     //Catch exception
                     error = NO_ERROR;

                     //Jump to the next object in the MIB
                     j++;
                  }
                  else if(error == ERROR_UNKNOWN_CONTEXT ||
                     error == ERROR_AUTHORIZATION_FAILED)
                  {
                     //Catch exception
                     error = NO_ERROR;

                     //Check the next instance of the same object
                     curOidLen = tempOidLen;
                     osMemmove(curOid, tempOid, tempOidLen);
                  }
                  else
                  {
                     //Exit immediately
                     break;
                  }
               }
               else
               {
                  //The current object is not accessible
                  j++;
               }
            }
         }
      }

      //Any error to report?
      if(error)
         break;
   }

   //Check status code
   if(!error)
   {
      //Next object found?
      if(nextObject != NULL)
      {
         //Replace the original OID with the name of the next object
         var->oid = nextOid;
         var->oidLen = nextOidLen;

         //Save the length of the OID
         context->response.oidLen = nextOidLen;
      }
      else
      {
         //The specified OID does not lexicographically precede the
         //name of some object
         error = ERROR_OBJECT_NOT_FOUND;
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Search MIBs for the given object
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] oid Object identifier
 * @param[in] oidLen Length of the OID
 * @param[out] object Pointer the MIB object descriptor
 * @return Error code
 **/

error_t snmpFindMibObject(SnmpAgentContext *context,
   const uint8_t *oid, size_t oidLen, const MibObject **object)
{
   error_t error;
   int_t left;
   int_t right;
   int_t mid;
   int_t res;
   uint_t i;
   size_t n;
   const MibObject *objects;

   //Initialize variables
   res = -1;
   mid = 0;
   objects = NULL;

   //Loop through MIBs
   for(i = 0; i < SNMP_AGENT_MAX_MIBS && res != 0; i++)
   {
      //Valid MIB?
      if(context->mibTable[i] != NULL &&
         context->mibTable[i]->numObjects > 0)
      {
         //Point to the list of objects
         objects = context->mibTable[i]->objects;

         //Index of the first item
         left = 0;
         //Index of the last item
         right = context->mibTable[i]->numObjects - 1;

         //Discard instance sub-identifier
         n = MIN(oidLen, objects[right].oidLen);

         //Check object identifier
         if(oidComp(oid, oidLen, objects[left].oid, objects[left].oidLen) >= 0 &&
            oidComp(oid, n, objects[right].oid, objects[right].oidLen) <= 0)
         {
            //Binary search algorithm
            while(left <= right && res != 0)
            {
               //Calculate the index of the middle item
               mid = left + (right - left) / 2;

               //Discard instance sub-identifier
               n = MIN(oidLen, objects[mid].oidLen);

               //Perform lexicographic comparison
               res = oidComp(oid, n, objects[mid].oid, objects[mid].oidLen);

               //Check the result of the comparison
               if(res > 0)
               {
                  left = mid + 1;
               }
               else if(res < 0)
               {
                  right = mid - 1;
               }
            }
         }
      }
   }

   //Object identifier found?
   if(res == 0)
   {
      //Scalar object?
      if(objects[mid].getNext == NULL)
      {
         //The instance sub-identifier shall be 0 for scalar objects
         if(oidLen == (objects[mid].oidLen + 1) && oid[oidLen - 1] == 0)
         {
            //Return a pointer to the matching object
            *object = &objects[mid];
            //No error to report
            error = NO_ERROR;
         }
         else
         {
            //No such instance...
            error = ERROR_INSTANCE_NOT_FOUND;
         }
      }
      //Tabular object?
      else
      {
         //Check the length of the OID
         if(oidLen > objects[mid].oidLen)
         {
            //Return a pointer to the matching object
            *object = &objects[mid];
            //No error to report
            error = NO_ERROR;
         }
         else
         {
            //No such instance...
            error = ERROR_INSTANCE_NOT_FOUND;
         }
      }
   }
   else
   {
      //No such object...
      error = ERROR_OBJECT_NOT_FOUND;
   }

   //Return status code
   return error;
}

#endif
