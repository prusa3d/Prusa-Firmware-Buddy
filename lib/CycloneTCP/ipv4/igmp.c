/**
 * @file igmp.c
 * @brief IGMP (Internet Group Management Protocol)
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
 * IGMP is used by IP hosts to report their multicast group memberships
 * to routers. Refer to the following RFCs for complete details:
 * - RFC 1112: Host Extensions for IP Multicasting
 * - RFC 2236: Internet Group Management Protocol, Version 2
 * - RFC 3376: Internet Group Management Protocol, Version 3
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

//Switch to the appropriate trace level
#define TRACE_LEVEL IGMP_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "core/ip.h"
#include "ipv4/ipv4.h"
#include "ipv4/igmp.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (IPV4_SUPPORT == ENABLED && IGMP_SUPPORT == ENABLED)

//Tick counter to handle periodic operations
systime_t igmpTickCounter;


/**
 * @brief IGMP initialization
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t igmpInit(NetInterface *interface)
{
   //The default host compatibility mode is IGMPv2
   interface->igmpv1RouterPresent = FALSE;

   //Start IGMPv1 router present timer
   interface->igmpv1RouterPresentTimer =
      osGetSystemTime() + IGMP_V1_ROUTER_PRESENT_TIMEOUT;

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief Join the specified host group
 * @param[in] interface Underlying network interface
 * @param[in] entry IPv4 filter entry identifying the host group to join
 * @return Error code
 **/

error_t igmpJoinGroup(NetInterface *interface, Ipv4FilterEntry *entry)
{
   //The all-systems group (address 224.0.0.1) is handled as a special
   //case. The host starts in Idle Member state for that group on every
   //interface and never transitions to another state
   if(entry->addr == IGMP_ALL_SYSTEMS_ADDR)
   {
      //Clear flag
      entry->flag = FALSE;
      //Enter the Idle Member state
      entry->state = IGMP_STATE_IDLE_MEMBER;
   }
   else
   {
      //Link is up?
      if(interface->linkState)
      {
         //When a host joins a multicast group, it should immediately transmit
         //an unsolicited Membership Report for that group
         igmpSendReportMessage(interface, entry->addr);

         //Set flag
         entry->flag = TRUE;
         //Start timer
         entry->timer = osGetSystemTime() + IGMP_UNSOLICITED_REPORT_INTERVAL;
         //Enter the Delaying Member state
         entry->state = IGMP_STATE_DELAYING_MEMBER;
      }
      //Link is down?
      else
      {
         //Clear flag
         entry->flag = FALSE;
         //Enter the Idle Member state
         entry->state = IGMP_STATE_IDLE_MEMBER;
      }
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Leave the specified host group
 * @param[in] interface Underlying network interface
 * @param[in] entry IPv4 filter entry identifying the host group to leave
 * @return Error code
 **/

error_t igmpLeaveGroup(NetInterface *interface, Ipv4FilterEntry *entry)
{
   //Check link state
   if(interface->linkState)
   {
      //Send a Leave Group message if the flag is set
      if(entry->flag)
         igmpSendLeaveGroupMessage(interface, entry->addr);
   }

   //Switch to the Non-Member state
   entry->state = IGMP_STATE_NON_MEMBER;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief IGMP timer handler
 *
 * This routine must be periodically called by the TCP/IP stack to
 * handle IGMP related timers
 *
 * @param[in] interface Underlying network interface
 **/

void igmpTick(NetInterface *interface)
{
   uint_t i;
   systime_t time;
   Ipv4FilterEntry *entry;

   //Get current time
   time = osGetSystemTime();

   //Check IGMPv1 router present timer
   if(timeCompare(time, interface->igmpv1RouterPresentTimer) >= 0)
      interface->igmpv1RouterPresent = FALSE;

   //Go through the multicast filter table
   for(i = 0; i < IPV4_MULTICAST_FILTER_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->ipv4Context.multicastFilter[i];

      //Valid entry?
      if(entry->refCount > 0)
      {
         //Delaying Member state?
         if(entry->state == IGMP_STATE_DELAYING_MEMBER)
         {
            //Timer expired?
            if(timeCompare(time, entry->timer) >= 0)
            {
               //Send a Membership Report message for the group on the interface
               igmpSendReportMessage(interface, entry->addr);

               //Set flag
               entry->flag = TRUE;
               //Switch to the Idle Member state
               entry->state = IGMP_STATE_IDLE_MEMBER;
            }
         }
      }
   }
}


/**
 * @brief Callback function for link change event
 * @param[in] interface Underlying network interface
 **/

void igmpLinkChangeEvent(NetInterface *interface)
{
   uint_t i;
   systime_t time;
   Ipv4FilterEntry *entry;

   //Get current time
   time = osGetSystemTime();

   //Link up event?
   if(interface->linkState)
   {
      //The default host compatibility mode is IGMPv2
      interface->igmpv1RouterPresent = FALSE;
      //Start IGMPv1 router present timer
      interface->igmpv1RouterPresentTimer = time + IGMP_V1_ROUTER_PRESENT_TIMEOUT;

      //Go through the multicast filter table
      for(i = 0; i < IPV4_MULTICAST_FILTER_SIZE; i++)
      {
         //Point to the current entry
         entry = &interface->ipv4Context.multicastFilter[i];

         //Valid entry?
         if(entry->refCount > 0)
         {
            //The all-systems group (address 224.0.0.1) is handled as a special
            //case. The host starts in Idle Member state for that group on every
            //interface and never transitions to another state
            if(entry->addr != IGMP_ALL_SYSTEMS_ADDR)
            {
               //Send an unsolicited Membership Report for that group
               igmpSendReportMessage(interface, entry->addr);

               //Set flag
               entry->flag = TRUE;
               //Start timer
               entry->timer = time + IGMP_UNSOLICITED_REPORT_INTERVAL;
               //Enter the Delaying Member state
               entry->state = IGMP_STATE_DELAYING_MEMBER;
            }
         }
      }
   }
   //Link down event?
   else
   {
      //Go through the multicast filter table
      for(i = 0; i < IPV4_MULTICAST_FILTER_SIZE; i++)
      {
         //Point to the current entry
         entry = &interface->ipv4Context.multicastFilter[i];

         //Valid entry?
         if(entry->refCount > 0)
         {
            //Clear flag
            entry->flag = FALSE;
            //Enter the Idle Member state
            entry->state = IGMP_STATE_IDLE_MEMBER;
         }
      }
   }
}


/**
 * @brief Process incoming IGMP message
 * @param[in] interface Underlying network interface
 * @param[in] pseudoHeader IPv4 pseudo header
 * @param[in] buffer Multi-part buffer containing the incoming IGMP message
 * @param[in] offset Offset to the first byte of the IGMP message
 **/

void igmpProcessMessage(NetInterface *interface, Ipv4PseudoHeader *pseudoHeader,
   const NetBuffer *buffer, size_t offset)
{
   size_t length;
   IgmpMessage *message;

   //Retrieve the length of the IGMP message
   length = netBufferGetLength(buffer) - offset;

   //Ensure the message length is correct
   if(length < sizeof(IgmpMessage))
   {
      //Debug message
      TRACE_WARNING("IGMP message length is invalid!\r\n");
      //Silently discard incoming message
      return;
   }

   //Point to the beginning of the IGMP message
   message = netBufferAt(buffer, offset);
   //Sanity check
   if(message == NULL)
      return;

   //Debug message
   TRACE_INFO("IGMP message received (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump message contents for debugging purpose
   igmpDumpMessage(message);

   //Verify checksum value
   if(ipCalcChecksumEx(buffer, offset, length) != 0x0000)
   {
      //Debug message
      TRACE_WARNING("Wrong IGMP header checksum!\r\n");
      //Drop incoming message
      return;
   }

   //Check the type field
   switch(message->type)
   {
   //Membership Query message?
   case IGMP_TYPE_MEMBERSHIP_QUERY:
      //Process Membership Query message
      igmpProcessQueryMessage(interface, message, length);
      break;
   //Membership Report message?
   case IGMP_TYPE_MEMBERSHIP_REPORT_V1:
   case IGMP_TYPE_MEMBERSHIP_REPORT_V2:
      //Process Membership Query message
      igmpProcessReportMessage(interface, message, length);
      break;
   //Unknown type?
   default:
      //Debug message
      TRACE_WARNING("Unknown IGMP message type!\r\n");
      //Discard incoming IGMP message
      break;
   }
}


/**
 * @brief Process incoming Membership Query message
 * @param[in] interface Underlying network interface
 * @param[in] message Incoming Membership Query message
 * @param[in] length Message length
 **/

void igmpProcessQueryMessage(NetInterface *interface,
   const IgmpMessage *message, size_t length)
{
   uint_t i;
   systime_t time;
   systime_t maxRespTime;
   Ipv4FilterEntry *entry;

   //Get current time
   time = osGetSystemTime();

   //IGMPv1 Membership Query message?
   if(message->maxRespTime == 0)
   {
      //The host receives a query with the Max Response Time field set to 0
      interface->igmpv1RouterPresent = TRUE;
      //Restart IGMPv1 router present timer
      interface->igmpv1RouterPresentTimer = time + IGMP_V1_ROUTER_PRESENT_TIMEOUT;
      //The maximum response time is 10 seconds by default
      maxRespTime = IGMP_V1_MAX_RESPONSE_TIME;
   }
   //IGMPv2 Membership Query message?
   else
   {
      //The Max Resp Time field specifies the maximum time allowed
      //before sending a responding report
      maxRespTime = message->maxRespTime * 100;
   }

   //Go through the multicast filter table
   for(i = 0; i < IPV4_MULTICAST_FILTER_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->ipv4Context.multicastFilter[i];

      //Valid entry?
      if(entry->refCount > 0)
      {
         //The all-systems group (224.0.0.1) is handled as a special case. The
         //host starts in Idle Member state for that group on every interface
         //and never transitions to another state
         if(entry->addr != IGMP_ALL_SYSTEMS_ADDR)
         {
            //A General Query applies to all memberships on the interface from which
            //the Query is received. A Group-Specific Query applies to membership
            //in a single group on the interface from which the Query is received
            if(message->groupAddr == IPV4_UNSPECIFIED_ADDR ||
               message->groupAddr == entry->addr)
            {
               //Delaying Member state?
               if(entry->state == IGMP_STATE_DELAYING_MEMBER)
               {
                  //The timer has not yet expired?
                  if(timeCompare(time, entry->timer) < 0)
                  {
                     //If a timer for the group is already running, it is reset to
                     //the random value only if the requested Max Response Time is
                     //less than the remaining value of the running timer
                     if(maxRespTime < (entry->timer - time))
                     {
                        //Restart delay timer
                        entry->timer = time + igmpRand(maxRespTime);
                     }
                  }
               }
               //Idle Member state?
               else if(entry->state == IGMP_STATE_IDLE_MEMBER)
               {
                  //Switch to the Delaying Member state
                  entry->state = IGMP_STATE_DELAYING_MEMBER;
                  //Delay the response by a random amount of time
                  entry->timer = time + igmpRand(maxRespTime);
               }
            }
         }
      }
   }
}


/**
 * @brief Process incoming Membership Report message
 * @param[in] interface Underlying network interface
 * @param[in] message Incoming Membership Report message
 * @param[in] length Message length
 **/

void igmpProcessReportMessage(NetInterface *interface,
   const IgmpMessage *message, size_t length)
{
   uint_t i;
   Ipv4FilterEntry *entry;

   //Go through the multicast filter table
   for(i = 0; i < IPV4_MULTICAST_FILTER_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->ipv4Context.multicastFilter[i];

      //Valid entry?
      if(entry->refCount > 0)
      {
         //Report messages are ignored for memberships in
         //the Non-Member or Idle Member state
         if(entry->state == IGMP_STATE_DELAYING_MEMBER)
         {
            //The Membership Report message matches the current entry?
            if(message->groupAddr == entry->addr)
            {
               //Clear flag
               entry->flag = FALSE;
               //Switch to the Idle Member state
               entry->state = IGMP_STATE_IDLE_MEMBER;
            }
         }
      }
   }
}


/**
 * @brief Send Membership Report message
 * @param[in] interface Underlying network interface
 * @param[in] ipAddr IPv4 address specifying the group address
 * @return Error code
 **/

error_t igmpSendReportMessage(NetInterface *interface, Ipv4Addr ipAddr)
{
   error_t error;
   size_t offset;
   IgmpMessage *message;
   NetBuffer *buffer;
   Ipv4PseudoHeader pseudoHeader;
   NetTxAncillary ancillary;

   //Make sure the specified address is a valid multicast address
   if(!ipv4IsMulticastAddr(ipAddr))
      return ERROR_INVALID_ADDRESS;

   //The all-systems group (224.0.0.1) is handled as a special case.
   //The host never sends a report for that group
   if(ipAddr == IGMP_ALL_SYSTEMS_ADDR)
      return ERROR_INVALID_ADDRESS;

   //Allocate a memory buffer to hold an IGMP message
   buffer = ipAllocBuffer(sizeof(IgmpMessage), &offset);
   //Failed to allocate memory?
   if(buffer == NULL)
      return ERROR_OUT_OF_MEMORY;

   //Point to the beginning of the IGMP message
   message = netBufferAt(buffer, offset);

   //The type of report is determined by the state of the interface
   if(interface->igmpv1RouterPresent)
   {
      message->type = IGMP_TYPE_MEMBERSHIP_REPORT_V1;
   }
   else
   {
      message->type = IGMP_TYPE_MEMBERSHIP_REPORT_V2;
   }

   //Format the Membership Report message
   message->maxRespTime = 0;
   message->checksum = 0;
   message->groupAddr = ipAddr;

   //Message checksum calculation
   message->checksum = ipCalcChecksumEx(buffer, offset, sizeof(IgmpMessage));

   //Format IPv4 pseudo header
   pseudoHeader.srcAddr = interface->ipv4Context.addrList[0].addr;
   pseudoHeader.destAddr = ipAddr;
   pseudoHeader.reserved = 0;
   pseudoHeader.protocol = IPV4_PROTOCOL_IGMP;
   pseudoHeader.length = HTONS(sizeof(IgmpMessage));

   //Debug message
   TRACE_INFO("Sending IGMP message (%" PRIuSIZE " bytes)...\r\n", sizeof(IgmpMessage));
   //Dump message contents for debugging purpose
   igmpDumpMessage(message);

   //Additional options can be passed to the stack along with the packet
   ancillary = NET_DEFAULT_TX_ANCILLARY;

   //All IGMP messages are sent with an IP TTL of 1 and contain an IP Router
   //Alert option in their IP header (refer to RFC 2236, section 2)
   ancillary.ttl = IGMP_TTL;
   ancillary.routerAlert = TRUE;

   //The Membership Report message is sent to the group being reported
   error = ipv4SendDatagram(interface, &pseudoHeader, buffer, offset,
      &ancillary);

   //Free previously allocated memory
   netBufferFree(buffer);
   //Return status code
   return error;
}


/**
 * @brief Send Leave Group message
 * @param[in] interface Underlying network interface
 * @param[in] ipAddr IPv4 address specifying the group address being left
 * @return Error code
 **/

error_t igmpSendLeaveGroupMessage(NetInterface *interface, Ipv4Addr ipAddr)
{
   error_t error;
   size_t offset;
   NetBuffer *buffer;
   IgmpMessage *message;
   Ipv4PseudoHeader pseudoHeader;
   NetTxAncillary ancillary;

   //Make sure the specified address is a valid multicast address
   if(!ipv4IsMulticastAddr(ipAddr))
      return ERROR_INVALID_ADDRESS;

   //The all-systems group (224.0.0.1) is handled as a special case.
   //The host never sends a Leave Group message for that group
   if(ipAddr == IGMP_ALL_SYSTEMS_ADDR)
      return ERROR_INVALID_ADDRESS;

   //If the interface state says the querier is running
   //IGMPv1, this action should be skipped
   if(interface->igmpv1RouterPresent)
      return NO_ERROR;

   //Allocate a memory buffer to hold an IGMP message
   buffer = ipAllocBuffer(sizeof(IgmpMessage), &offset);
   //Failed to allocate memory?
   if(buffer == NULL)
      return ERROR_OUT_OF_MEMORY;

   //Point to the beginning of the IGMP message
   message = netBufferAt(buffer, offset);

   //Format the Leave Group message
   message->type = IGMP_TYPE_LEAVE_GROUP;
   message->maxRespTime = 0;
   message->checksum = 0;
   message->groupAddr = ipAddr;

   //Message checksum calculation
   message->checksum = ipCalcChecksumEx(buffer, offset, sizeof(IgmpMessage));

   //Format IPv4 pseudo header
   pseudoHeader.srcAddr = interface->ipv4Context.addrList[0].addr;
   pseudoHeader.destAddr = IGMP_ALL_ROUTERS_ADDR;
   pseudoHeader.reserved = 0;
   pseudoHeader.protocol = IPV4_PROTOCOL_IGMP;
   pseudoHeader.length = HTONS(sizeof(IgmpMessage));

   //Debug message
   TRACE_INFO("Sending IGMP message (%" PRIuSIZE " bytes)...\r\n", sizeof(IgmpMessage));
   //Dump message contents for debugging purpose
   igmpDumpMessage(message);

   //Additional options can be passed to the stack along with the packet
   ancillary = NET_DEFAULT_TX_ANCILLARY;

   //All IGMP messages are sent with an IP TTL of 1 and contain an IP Router
   //Alert option in their IP header (refer to RFC 2236, section 2)
   ancillary.ttl = IGMP_TTL;
   ancillary.routerAlert = TRUE;

   //The Leave Group message is sent to the all-routers multicast group
   error = ipv4SendDatagram(interface, &pseudoHeader, buffer, offset,
      &ancillary);

   //Free previously allocated memory
   netBufferFree(buffer);
   //Return status code
   return error;
}


/**
 * @brief Get a random value in the specified range
 * @param[in] max Upper bound
 * @return Random value in the specified range
 **/

uint32_t igmpRand(uint32_t max)
{
   //Return a random value in the given range
   return netGetRand() % (max + 1);
}


/**
 * @brief Dump IGMP message for debugging purpose
 * @param[in] message Pointer to the IGMP message
 **/

void igmpDumpMessage(const IgmpMessage *message)
{
   //Dump IGMP message
   TRACE_DEBUG("  Type = 0x%02" PRIX8 "\r\n", message->type);
   TRACE_DEBUG("  Max Resp Time = %" PRIu8 "\r\n", message->maxRespTime);
   TRACE_DEBUG("  Checksum = 0x%04" PRIX16 "\r\n", ntohs(message->checksum));
   TRACE_DEBUG("  Group Address = %s\r\n", ipv4AddrToString(message->groupAddr, NULL));
}

#endif
