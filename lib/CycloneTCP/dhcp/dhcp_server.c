/**
 * @file dhcp_server.c
 * @brief DHCP server (Dynamic Host Configuration Protocol)
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
 * The Dynamic Host Configuration Protocol is used to provide configuration
 * parameters to hosts. Refer to the following RFCs for complete details:
 * - RFC 2131: Dynamic Host Configuration Protocol
 * - RFC 2132: DHCP Options and BOOTP Vendor Extensions
 * - RFC 4039: Rapid Commit Option for the DHCP version 4
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

//Switch to the appropriate trace level
#define TRACE_LEVEL DHCP_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "dhcp/dhcp_server.h"
#include "dhcp/dhcp_common.h"
#include "dhcp/dhcp_debug.h"
#include "date_time.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (IPV4_SUPPORT == ENABLED && DHCP_SERVER_SUPPORT == ENABLED)

//Tick counter to handle periodic operations
systime_t dhcpServerTickCounter;


/**
 * @brief Initialize settings with default values
 * @param[out] settings Structure that contains DHCP server settings
 **/

void dhcpServerGetDefaultSettings(DhcpServerSettings *settings)
{
   uint_t i;

   //Use default interface
   settings->interface = netGetDefaultInterface();
   //Index of the IP address assigned to the DHCP server
   settings->ipAddrIndex = 0;

   //Support for quick configuration using rapid commit
   settings->rapidCommit = FALSE;
   //Lease time, in seconds, assigned to the DHCP clients
   settings->leaseTime = DHCP_SERVER_DEFAULT_LEASE_TIME;

   //Lowest and highest IP addresses in the pool that are available
   //for dynamic address assignment
   settings->ipAddrRangeMin = IPV4_UNSPECIFIED_ADDR;
   settings->ipAddrRangeMax = IPV4_UNSPECIFIED_ADDR;

   //Subnet mask
   settings->subnetMask = IPV4_UNSPECIFIED_ADDR;
   //Default gateway
   settings->defaultGateway = IPV4_UNSPECIFIED_ADDR;

   //DNS servers
   for(i = 0; i < DHCP_SERVER_MAX_DNS_SERVERS; i++)
      settings->dnsServer[i] = IPV4_UNSPECIFIED_ADDR;
}


/**
 * @brief DHCP server initialization
 * @param[in] context Pointer to the DHCP server context
 * @param[in] settings DHCP server specific settings
 * @return Error code
 **/

error_t dhcpServerInit(DhcpServerContext *context,
   const DhcpServerSettings *settings)
{
   error_t error;
   NetInterface *interface;

   //Debug message
   TRACE_INFO("Initializing DHCP server...\r\n");

   //Ensure the parameters are valid
   if(context == NULL || settings == NULL)
      return ERROR_INVALID_PARAMETER;

   //Valid network interface?
   if(settings->interface == NULL)
      return ERROR_INVALID_PARAMETER;

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Point to the underlying network interface
   interface = settings->interface;

   //Clear the DHCP server context
   osMemset(context, 0, sizeof(DhcpServerContext));
   //Save user settings
   context->settings = *settings;

   //Next IP address that will be assigned by the DHCP server
   context->nextIpAddr = settings->ipAddrRangeMin;
   //DHCP server is currently suspended
   context->running = FALSE;

   //Callback function to be called when a DHCP message is received
   error = udpAttachRxCallback(interface, DHCP_SERVER_PORT,
      dhcpServerProcessMessage, context);

   //Check status code
   if(!error)
   {
      //Attach the DHCP server context to the network interface
      interface->dhcpServerContext = context;
   }

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Return status code
   return error;
}


/**
 * @brief Release DHCP server context
 * @param[in] context Pointer to the DHCP server context
 **/

void dhcpServerDeinit(DhcpServerContext *context)
{
   NetInterface *interface;

   //Make sure the DHCP server context is valid
   if(context != NULL)
   {
      //Get exclusive access
      osAcquireMutex(&netMutex);

      //Point to the underlying network interface
      interface = context->settings.interface;

      //Valid network interface?
      if(interface != NULL)
      {
         //Detach the DHCP server context from the network interface
         interface->dhcpServerContext = NULL;

         //Unregister callback function
         udpDetachRxCallback(interface, DHCP_SERVER_PORT);
      }

      //Clear the DHCP server context
      osMemset(context, 0, sizeof(DhcpServerContext));

      //Release exclusive access
      osReleaseMutex(&netMutex);
   }
}


/**
 * @brief Start DHCP server
 * @param[in] context Pointer to the DHCP server context
 * @return Error code
 **/

error_t dhcpServerStart(DhcpServerContext *context)
{
   //Make sure the DHCP server context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Debug message
   TRACE_INFO("Starting DHCP server...\r\n");

   //Get exclusive access
   osAcquireMutex(&netMutex);
   //Start DHCP server
   context->running = TRUE;
   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Stop DHCP server
 * @param[in] context Pointer to the DHCP server context
 * @return Error code
 **/

error_t dhcpServerStop(DhcpServerContext *context)
{
   //Make sure the DHCP server context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Debug message
   TRACE_INFO("Stopping DHCP server...\r\n");

   //Get exclusive access
   osAcquireMutex(&netMutex);
   //Stop DHCP server
   context->running = FALSE;
   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief DHCP server timer handler
 *
 * This routine must be periodically called by the TCP/IP stack to
 * manage DHCP server operation
 *
 * @param[in] context Pointer to the DHCP server context
 **/

void dhcpServerTick(DhcpServerContext *context)
{
   uint_t i;
   systime_t time;
   systime_t leaseTime;
   DhcpServerBinding *binding;

   //Make sure the DHCP server has been properly instantiated
   if(context == NULL)
      return;

   //Get current time
   time = osGetSystemTime();

   //Convert the lease time to milliseconds
   if(context->settings.leaseTime < (MAX_DELAY / 1000))
      leaseTime = context->settings.leaseTime * 1000;
   else
      leaseTime = MAX_DELAY;

   //Loop through the list of bindings
   for(i = 0; i < DHCP_SERVER_MAX_CLIENTS; i++)
   {
      //Point to the current binding
      binding = &context->clientBinding[i];

      //Valid binding?
      if(!macCompAddr(&binding->macAddr, &MAC_UNSPECIFIED_ADDR))
      {
         //Check whether the network address has been committed
         if(binding->validLease)
         {
            //Check whether the lease has expired
            if(timeCompare(time, binding->timestamp + leaseTime) >= 0)
            {
               //The address lease is not more valid
               binding->validLease = FALSE;
            }
         }
      }
   }
}


/**
 * @brief Process incoming DHCP message
 * @param[in] interface Underlying network interface
 * @param[in] pseudoHeader UDP pseudo header
 * @param[in] udpHeader UDP header
 * @param[in] buffer Multi-part buffer containing the incoming DHCP message
 * @param[in] offset Offset to the first byte of the DHCP message
 * @param[in] ancillary Additional options passed to the stack along with
 *   the packet
 * @param[in] param Pointer to the DHCP server context
 **/

void dhcpServerProcessMessage(NetInterface *interface,
   const IpPseudoHeader *pseudoHeader, const UdpHeader *udpHeader,
   const NetBuffer *buffer, size_t offset, const NetRxAncillary *ancillary,
   void *param)
{
   size_t length;
   DhcpServerContext *context;
   DhcpMessage *message;
   DhcpOption *option;

   //Point to the DHCP server context
   context = (DhcpServerContext *) param;

   //Retrieve the length of the DHCP message
   length = netBufferGetLength(buffer) - offset;

   //Make sure the DHCP message is valid
   if(length < sizeof(DhcpMessage))
      return;
   if(length > DHCP_MAX_MSG_SIZE)
      return;

   //Point to the beginning of the DHCP message
   message = netBufferAt(buffer, offset);
   //Sanity check
   if(message == NULL)
      return;

   //Debug message
   TRACE_DEBUG("\r\n%s: DHCP message received (%" PRIuSIZE " bytes)...\r\n",
      formatSystemTime(osGetSystemTime(), NULL), length);

   //Dump the contents of the message for debugging purpose
   dhcpDumpMessage(message, length);

   //Check opcode
   if(message->op != DHCP_OPCODE_BOOTREQUEST)
      return;
   //Enforce hardware type
   if(message->htype != DHCP_HARDWARE_TYPE_ETH)
      return;
   //Check the length of the hardware address
   if(message->hlen != sizeof(MacAddr))
      return;
   //Check magic cookie
   if(message->magicCookie != HTONL(DHCP_MAGIC_COOKIE))
      return;

   //Retrieve DHCP Message Type option
   option = dhcpGetOption(message, length, DHCP_OPT_DHCP_MESSAGE_TYPE);

   //Failed to retrieve specified option?
   if(option == NULL || option->length != 1)
      return;

   //Check message type
   switch(option->value[0])
   {
   case DHCP_MESSAGE_TYPE_DISCOVER:
      //Parse DHCPDISCOVER message
      dhcpServerParseDiscover(context, message, length);
      break;
   case DHCP_MESSAGE_TYPE_REQUEST:
      //Parse DHCPREQUEST message
      dhcpServerParseRequest(context, message, length);
      break;
   case DHCP_MESSAGE_TYPE_DECLINE:
      //Parse DHCPDECLINE message
      dhcpServerParseDecline(context, message, length);
      break;
   case DHCP_MESSAGE_TYPE_RELEASE:
      //Parse DHCPRELEASE message
      dhcpServerParseRelease(context, message, length);
      break;
   case DHCP_MESSAGE_TYPE_INFORM:
      //Parse DHCPINFORM message
      dhcpServerParseInform(context, message, length);
      break;
   default:
      //Silently drop incoming message
      break;
   }
}


/**
 * @brief Parse DHCPDISCOVER message
 * @param[in] context Pointer to the DHCP server context
 * @param[in] message Pointer to the incoming DHCP message
 * @param[in] length Length of the incoming message to parse
 **/

void dhcpServerParseDiscover(DhcpServerContext *context,
   const DhcpMessage *message, size_t length)
{
   error_t error;
   uint_t i;
   NetInterface *interface;
   Ipv4Addr requestedIpAddr;
   DhcpOption *option;
   DhcpServerBinding *binding;

   //Point to the underlying network interface
   interface = context->settings.interface;
   //Index of the IP address assigned to the DHCP server
   i = context->settings.ipAddrIndex;

   //Retrieve Server Identifier option
   option = dhcpGetOption(message, length, DHCP_OPT_SERVER_IDENTIFIER);

   //Option found?
   if(option != NULL && option->length == 4)
   {
      //Unexpected server identifier?
      if(!ipv4CompAddr(option->value, &interface->ipv4Context.addrList[i].addr))
         return;
   }

   //Retrieve Requested IP Address option
   option = dhcpGetOption(message, length, DHCP_OPT_REQUESTED_IP_ADDRESS);

   //The client may include the 'requested IP address' option to suggest
   //that a particular IP address be assigned
   if(option != NULL && option->length == 4)
      ipv4CopyAddr(&requestedIpAddr, option->value);
   else
      requestedIpAddr = IPV4_UNSPECIFIED_ADDR;

   //Search the list for a matching binding
   binding = dhcpServerFindBindingByMacAddr(context, &message->chaddr);

   //Matching binding found?
   if(binding != NULL)
   {
      //Different IP address than cached?
      if(requestedIpAddr != binding->ipAddr)
      {
         //Ensure the IP address is in the server's pool of available addresses
         if(ntohl(requestedIpAddr) >= ntohl(context->settings.ipAddrRangeMin) &&
            ntohl(requestedIpAddr) <= ntohl(context->settings.ipAddrRangeMax))
         {
            //Make sure the IP address is not already allocated
            if(!dhcpServerFindBindingByIpAddr(context, requestedIpAddr))
            {
               //Record IP address
               binding->ipAddr = requestedIpAddr;
               //Get current time
               binding->timestamp = osGetSystemTime();
            }
         }
      }

      //Successful processing
      error = NO_ERROR;
   }
   else
   {
      //Create a new binding
      binding = dhcpServerCreateBinding(context);

      //Binding successfully created
      if(binding != NULL)
      {
         //Ensure the IP address is in the server's pool of available addresses
         if(ntohl(requestedIpAddr) >= ntohl(context->settings.ipAddrRangeMin) &&
            ntohl(requestedIpAddr) <= ntohl(context->settings.ipAddrRangeMax))
         {
            //Make sure the IP address is not already allocated
            if(!dhcpServerFindBindingByIpAddr(context, requestedIpAddr))
            {
               //Record IP address
               binding->ipAddr = requestedIpAddr;
               //Successful processing
               error = NO_ERROR;
            }
            else
            {
               //Retrieve the next available IP address from the pool of addresses
               error = dhcpServerGetNextIpAddr(context, &binding->ipAddr);
            }
         }
         else
         {
            //Retrieve the next available IP address from the pool of addresses
            error = dhcpServerGetNextIpAddr(context, &binding->ipAddr);
         }

         //Check status code
         if(!error)
         {
            //Record MAC address
            binding->macAddr = message->chaddr;
            //Get current time
            binding->timestamp = osGetSystemTime();
         }
      }
      else
      {
         //Failed to create a new binding
         error = ERROR_FAILURE;
      }
   }

   //Check status code
   if(!error)
   {
      //The server responds with a DHCPOFFER message that includes an
      //available network address in the 'yiaddr' field (and other
      //configuration parameters in DHCP options)
      dhcpServerSendReply(context, DHCP_MESSAGE_TYPE_OFFER,
         binding->ipAddr, message, length);
   }
}


/**
 * @brief Parse DHCPREQUEST message
 * @param[in] context Pointer to the DHCP server context
 * @param[in] message Pointer to the incoming DHCP message
 * @param[in] length Length of the incoming message to parse
 **/

void dhcpServerParseRequest(DhcpServerContext *context,
   const DhcpMessage *message, size_t length)
{
   uint_t i;
   NetInterface *interface;
   Ipv4Addr clientIpAddr;
   DhcpOption *option;
   DhcpServerBinding *binding;

   //Point to the underlying network interface
   interface = context->settings.interface;
   //Index of the IP address assigned to the DHCP server
   i = context->settings.ipAddrIndex;

   //Retrieve Server Identifier option
   option = dhcpGetOption(message, length, DHCP_OPT_SERVER_IDENTIFIER);

   //Option found?
   if(option != NULL && option->length == 4)
   {
      //Unexpected server identifier?
      if(!ipv4CompAddr(option->value, &interface->ipv4Context.addrList[i].addr))
         return;
   }

   //Check the 'ciaddr' field
   if(message->ciaddr != IPV4_UNSPECIFIED_ADDR)
   {
      //Save client's network address
      clientIpAddr = message->ciaddr;
   }
   else
   {
      //Retrieve Requested IP Address option
      option = dhcpGetOption(message, length, DHCP_OPT_REQUESTED_IP_ADDRESS);

      //Option found?
      if(option != NULL && option->length == 4)
         ipv4CopyAddr(&clientIpAddr, option->value);
      else
         clientIpAddr = IPV4_UNSPECIFIED_ADDR;
   }

   //Valid client IP address?
   if(clientIpAddr != IPV4_UNSPECIFIED_ADDR)
   {
      //Search the list for a matching binding
      binding = dhcpServerFindBindingByMacAddr(context, &message->chaddr);

      //Matching binding found?
      if(binding != NULL)
      {
         //Make sure the client's IP address is valid
         if(clientIpAddr == binding->ipAddr)
         {
            //Commit network address
            binding->validLease = TRUE;
            //Save lease start time
            binding->timestamp = osGetSystemTime();

            //The server responds with a DHCPACK message containing the
            //configuration parameters for the requesting client
            dhcpServerSendReply(context, DHCP_MESSAGE_TYPE_ACK,
               binding->ipAddr, message, length);

            //Exit immediately
            return;
         }
      }
      else
      {
         //Ensure the IP address is in the server's pool of available addresses
         if(ntohl(clientIpAddr) >= ntohl(context->settings.ipAddrRangeMin) &&
            ntohl(clientIpAddr) <= ntohl(context->settings.ipAddrRangeMax))
         {
            //Make sure the IP address is not already allocated
            if(!dhcpServerFindBindingByIpAddr(context, clientIpAddr))
            {
               //Create a new binding
               binding = dhcpServerCreateBinding(context);

               //Binding successfully created
               if(binding != NULL)
               {
                  //Record MAC address
                  binding->macAddr = message->chaddr;
                  //Record IP address
                  binding->ipAddr = clientIpAddr;
                  //Commit network address
                  binding->validLease = TRUE;
                  //Get current time
                  binding->timestamp = osGetSystemTime();

                  //The server responds with a DHCPACK message containing the
                  //configuration parameters for the requesting client
                  dhcpServerSendReply(context, DHCP_MESSAGE_TYPE_ACK,
                     binding->ipAddr, message, length);

                  //Exit immediately
                  return;
               }
            }
         }
      }
   }

   //If the server is unable to satisfy the DHCPREQUEST message, the
   //server should respond with a DHCPNAK message
   dhcpServerSendReply(context, DHCP_MESSAGE_TYPE_NAK,
      IPV4_UNSPECIFIED_ADDR, message, length);
}


/**
 * @brief Parse DHCPDECLINE message
 * @param[in] context Pointer to the DHCP server context
 * @param[in] message Pointer to the incoming DHCP message
 * @param[in] length Length of the incoming message to parse
 **/

void dhcpServerParseDecline(DhcpServerContext *context,
   const DhcpMessage *message, size_t length)
{
   DhcpOption *option;
   DhcpServerBinding *binding;
   Ipv4Addr requestedIpAddr;

   //Retrieve Requested IP Address option
   option = dhcpGetOption(message, length, DHCP_OPT_REQUESTED_IP_ADDRESS);

   //Option found?
   if(option != NULL && option->length == 4)
   {
      //Copy the requested IP address
      ipv4CopyAddr(&requestedIpAddr, option->value);

      //Search the list for a matching binding
      binding = dhcpServerFindBindingByMacAddr(context, &message->chaddr);

      //Matching binding found?
      if(binding != NULL)
      {
         //Check the IP address against the requested IP address
         if(binding->ipAddr == requestedIpAddr)
         {
            //Remote the binding from the list
            osMemset(binding, 0, sizeof(DhcpServerBinding));
         }
      }
   }
}


/**
 * @brief Parse DHCPRELEASE message
 * @param[in] context Pointer to the DHCP server context
 * @param[in] message Pointer to the incoming DHCP message
 * @param[in] length Length of the incoming message to parse
 **/

void dhcpServerParseRelease(DhcpServerContext *context,
   const DhcpMessage *message, size_t length)
{
   DhcpServerBinding *binding;

   //Search the list for a matching binding
   binding = dhcpServerFindBindingByMacAddr(context, &message->chaddr);

   //Matching binding found?
   if(binding != NULL)
   {
      //Check the IP address against the client IP address
      if(binding->ipAddr == message->ciaddr)
      {
         //Release the network address and cancel remaining lease
         binding->validLease = FALSE;
      }
   }
}


/**
 * @brief Parse DHCPINFORM message
 * @param[in] context Pointer to the DHCP server context
 * @param[in] message Pointer to the incoming DHCP message
 * @param[in] length Length of the incoming message to parse
 **/

void dhcpServerParseInform(DhcpServerContext *context,
   const DhcpMessage *message, size_t length)
{
   //Make sure the client IP address is valid
   if(message->ciaddr != IPV4_UNSPECIFIED_ADDR)
   {
      //Servers receiving a DHCPINFORM message construct a DHCPACK message
      //with any local configuration parameters appropriate for the client
      dhcpServerSendReply(context, DHCP_MESSAGE_TYPE_ACK,
         IPV4_UNSPECIFIED_ADDR, message, length);
   }
}


/**
 * @brief Send DHCP reply message
 * @param[in] context Pointer to the DHCP server context
 * @param[in] type DHCP message type (DHCPOFFER, DHCPACK or DHCPNAK)
 * @param[in] yourIpAddr The IP address to be placed in the 'yiaddr' field
 * @param[in] request Pointer to DHCP message received from the client
 * @param[in] length Length of the DHCP message received from the client
 * @return Error code
 **/

error_t dhcpServerSendReply(DhcpServerContext *context, uint8_t type,
   Ipv4Addr yourIpAddr, const DhcpMessage *request, size_t length)
{
   error_t error;
   uint_t i;
   uint_t n;
   uint32_t value;
   size_t offset;
   NetBuffer *buffer;
   NetInterface *interface;
   DhcpMessage *reply;
   IpAddr srcIpAddr;
   IpAddr destIpAddr;
   uint16_t destPort;
   NetTxAncillary ancillary;

   //Point to the underlying network interface
   interface = context->settings.interface;
   //Index of the IP address assigned to the DHCP server
   i = context->settings.ipAddrIndex;

   //Allocate a memory buffer to hold the DHCP message
   buffer = udpAllocBuffer(DHCP_MIN_MSG_SIZE, &offset);
   //Failed to allocate buffer?
   if(buffer == NULL)
      return ERROR_OUT_OF_MEMORY;

   //Point to the beginning of the DHCP message
   reply = netBufferAt(buffer, offset);
   //Clear memory buffer contents
   osMemset(reply, 0, DHCP_MIN_MSG_SIZE);

   //Format DHCP reply message
   reply->op = DHCP_OPCODE_BOOTREPLY;
   reply->htype = DHCP_HARDWARE_TYPE_ETH;
   reply->hlen = sizeof(MacAddr);
   reply->xid = request->xid;
   reply->secs = 0;
   reply->flags = request->flags;
   reply->ciaddr = IPV4_UNSPECIFIED_ADDR;
   reply->yiaddr = yourIpAddr;
   reply->siaddr = IPV4_UNSPECIFIED_ADDR;
   reply->giaddr = request->giaddr;
   reply->chaddr = request->chaddr;

   //Write magic cookie before setting any option
   reply->magicCookie = HTONL(DHCP_MAGIC_COOKIE);
   //Properly terminate options field
   reply->options[0] = DHCP_OPT_END;

   //Add DHCP Message Type option
   dhcpAddOption(reply, DHCP_OPT_DHCP_MESSAGE_TYPE,
      &type, sizeof(type));

   //Add Server Identifier option
   dhcpAddOption(reply, DHCP_OPT_SERVER_IDENTIFIER,
      &interface->ipv4Context.addrList[i].addr, sizeof(Ipv4Addr));

   //DHCPOFFER or DHCPACK message?
   if(type == DHCP_MESSAGE_TYPE_OFFER || type == DHCP_MESSAGE_TYPE_ACK)
   {
      //Convert the lease time to network byte order
      value = htonl(context->settings.leaseTime);

      //When responding to a DHCPINFORM message, the server must not
      //send a lease expiration time to the client
      if(yourIpAddr != IPV4_UNSPECIFIED_ADDR)
      {
         //Add IP Address Lease Time option
         dhcpAddOption(reply, DHCP_OPT_IP_ADDRESS_LEASE_TIME,
            &value, sizeof(value));
      }

      //Add Subnet Mask option
      if(context->settings.subnetMask != IPV4_UNSPECIFIED_ADDR)
      {
         dhcpAddOption(reply, DHCP_OPT_SUBNET_MASK,
            &context->settings.subnetMask, sizeof(Ipv4Addr));
      }

      //Add Router option
      if(context->settings.defaultGateway != IPV4_UNSPECIFIED_ADDR)
      {
         dhcpAddOption(reply, DHCP_OPT_ROUTER,
            &context->settings.defaultGateway, sizeof(Ipv4Addr));
      }

      //Retrieve the number of DNS servers
      for(n = 0; n < DHCP_SERVER_MAX_DNS_SERVERS; n++)
      {
         //Check whether the current DNS server is valid
         if(context->settings.dnsServer[n] == IPV4_UNSPECIFIED_ADDR)
            break;
      }

      //Add DNS Server option
      if(n > 0)
      {
         dhcpAddOption(reply, DHCP_OPT_DNS_SERVER,
            context->settings.dnsServer, n * sizeof(Ipv4Addr));
      }
   }

   //A server with multiple network address (e.g. a multi-homed host) may
   //use any of its network addresses in outgoing DHCP messages (refer to
   //RFC 2131, section 4.1)
   srcIpAddr.length = sizeof(Ipv4Addr);
   srcIpAddr.ipv4Addr = interface->ipv4Context.addrList[i].addr;

   //Check whether the 'giaddr' field is non-zero
   if(request->giaddr != IPV4_UNSPECIFIED_ADDR)
   {
      //If the 'giaddr' field in a DHCP message from a client is non-zero,
      //the server sends any return messages to the 'DHCP server' port
      destPort = DHCP_SERVER_PORT;

      //The DHCP message is sent to the BOOTP relay agent whose address
      //appears in 'giaddr'
      destIpAddr.length = sizeof(Ipv4Addr);
      destIpAddr.ipv4Addr = request->giaddr;
   }
   else
   {
      //If the 'giaddr' field in a DHCP message from a client is zero,
      //the server sends any return messages to the 'DHCP client'
      destPort = DHCP_CLIENT_PORT;

      //DHCPOFFER or DHCPACK message?
      if(type == DHCP_MESSAGE_TYPE_OFFER || type == DHCP_MESSAGE_TYPE_ACK)
      {
         //Check whether the 'giaddr' field is non-zero
         if(request->ciaddr != IPV4_UNSPECIFIED_ADDR)
         {
            //If the 'giaddr' field is zero and the 'ciaddr' field is nonzero,
            //then the server unicasts DHCPOFFER and DHCPACK messages to the
            //address in 'ciaddr'
            destIpAddr.length = sizeof(Ipv4Addr);
            destIpAddr.ipv4Addr = request->ciaddr;
         }
         else
         {
            //Check whether the broadcast bit is set
            if(ntohs(request->flags) & DHCP_FLAG_BROADCAST)
            {
               //If 'giaddr' is zero and 'ciaddr' is zero, and the broadcast bit is
               //set, then the server broadcasts DHCPOFFER and DHCPACK messages
               destIpAddr.length = sizeof(Ipv4Addr);
               destIpAddr.ipv4Addr = IPV4_BROADCAST_ADDR;
            }
            else
            {
               //If 'giaddr' is zero and 'ciaddr' is zero, and the broadcast bit is
               //not set, then the server unicasts DHCPOFFER and DHCPACK messages
               //to the client's hardware address and 'yiaddr' address
               destIpAddr.length = sizeof(Ipv4Addr);
               destIpAddr.ipv4Addr = IPV4_BROADCAST_ADDR;
            }
         }
      }
      //DHCPNAK message?
      else
      {
         //In all cases, when 'giaddr' is zero, the server broadcasts any
         //DHCPNAK messages
         destIpAddr.length = sizeof(Ipv4Addr);
         destIpAddr.ipv4Addr = IPV4_BROADCAST_ADDR;
      }
   }

   //Debug message
   TRACE_DEBUG("\r\n%s: Sending DHCP message (%" PRIuSIZE " bytes)...\r\n",
      formatSystemTime(osGetSystemTime(), NULL), DHCP_MIN_MSG_SIZE);

   //Dump the contents of the message for debugging purpose
   dhcpDumpMessage(reply, DHCP_MIN_MSG_SIZE);

   //Additional options can be passed to the stack along with the packet
   ancillary = NET_DEFAULT_TX_ANCILLARY;

   //Send DHCP reply
   error = udpSendBuffer(interface, &srcIpAddr, DHCP_SERVER_PORT, &destIpAddr,
      destPort, buffer, offset, &ancillary);

   //Free previously allocated memory
   netBufferFree(buffer);
   //Return status code
   return error;
}


/**
 * @brief Create a new binding
 * @param[in] context Pointer to the DHCP server context
 * @return Pointer to the newly created binding
 **/

DhcpServerBinding *dhcpServerCreateBinding(DhcpServerContext *context)
{
   uint_t i;
   systime_t time;
   DhcpServerBinding *binding;
   DhcpServerBinding *oldestBinding;

   //Get current time
   time = osGetSystemTime();

   //Keep track of the oldest binding
   oldestBinding = NULL;

   //Loop through the list of bindings
   for(i = 0; i < DHCP_SERVER_MAX_CLIENTS; i++)
   {
      //Point to the current binding
      binding = &context->clientBinding[i];

      //Check whether the binding is available
      if(macCompAddr(&binding->macAddr, &MAC_UNSPECIFIED_ADDR))
      {
         //Erase contents
         osMemset(binding, 0, sizeof(DhcpServerBinding));
         //Return a pointer to the newly created binding
         return binding;
      }
      else
      {
         //Bindings that have been committed cannot be removed
         if(!binding->validLease)
         {
            //Keep track of the oldest binding in the list
            if(oldestBinding == NULL)
            {
               oldestBinding = binding;
            }
            else if((time - binding->timestamp) > (time - oldestBinding->timestamp))
            {
               oldestBinding = binding;
            }
         }
      }
   }

   //Any binding available in the list?
   if(oldestBinding != NULL)
   {
      //Erase contents
      osMemset(oldestBinding, 0, sizeof(DhcpServerBinding));
   }

   //Return a pointer to the oldest binding
   return oldestBinding;
}


/**
 * @brief Search the list of bindings for a given MAC address
 * @param[in] context Pointer to the DHCP server context
 * @param[in] macAddr MAC address
 * @return Pointer to the corresponding DHCP binding
 **/

DhcpServerBinding *dhcpServerFindBindingByMacAddr(DhcpServerContext *context,
   const MacAddr *macAddr)
{
   uint_t i;
   DhcpServerBinding *binding;

   //Loop through the list of bindings
   for(i = 0; i < DHCP_SERVER_MAX_CLIENTS; i++)
   {
      //Point to the current binding
      binding = &context->clientBinding[i];

      //Valid binding?
      if(!macCompAddr(&binding->macAddr, &MAC_UNSPECIFIED_ADDR))
      {
         //Check whether the current binding matches the specified MAC address
         if(macCompAddr(&binding->macAddr, macAddr))
         {
            //Return the pointer to the corresponding binding
            return binding;
         }
      }
   }

   //No matching binding...
   return NULL;
}


/**
 * @brief Search the list of bindings for a given IP address
 * @param[in] context Pointer to the DHCP server context
 * @param[in] ipAddr IP address
 * @return Pointer to the corresponding DHCP binding
 **/

DhcpServerBinding *dhcpServerFindBindingByIpAddr(DhcpServerContext *context,
   Ipv4Addr ipAddr)
{
   uint_t i;
   DhcpServerBinding *binding;

   //Loop through the list of bindings
   for(i = 0; i < DHCP_SERVER_MAX_CLIENTS; i++)
   {
      //Point to the current binding
      binding = &context->clientBinding[i];

      //Valid binding?
      if(!macCompAddr(&binding->macAddr, &MAC_UNSPECIFIED_ADDR))
      {
         //Check whether the current binding matches the specified IP address
         if(binding->ipAddr == ipAddr)
         {
            //Return the pointer to the corresponding binding
            return binding;
         }
      }
   }

   //No matching binding...
   return NULL;
}


/**
 * @brief Retrieve the next IP address to be used
 * @param[in] context Pointer to the DHCP server context
 * @param[out] ipAddr Next IP address to be used
 * @return Error code
 **/

error_t dhcpServerGetNextIpAddr(DhcpServerContext *context, Ipv4Addr *ipAddr)
{
   uint_t i;
   DhcpServerBinding *binding;

   //Search the pool for any available IP address
   for(i = 0; i < DHCP_SERVER_MAX_CLIENTS; i++)
   {
      //Check whether the current IP address is already allocated
      binding = dhcpServerFindBindingByIpAddr(context, context->nextIpAddr);

      //If the IP address is available, then it can be assigned to a new client
      if(binding == NULL)
         *ipAddr = context->nextIpAddr;

      //Compute the next IP address that will be assigned by the DHCP server
      if(ntohl(context->nextIpAddr) >= ntohl(context->settings.ipAddrRangeMax))
      {
         //Wrap around to the beginning of the pool
         context->nextIpAddr = context->settings.ipAddrRangeMin;
      }
      else
      {
         //Increment IP address
         context->nextIpAddr = htonl(ntohl(context->nextIpAddr) + 1);
      }

      //If the IP address is available, we are done
      if(binding == NULL)
         return NO_ERROR;
   }

   //No available addresses in the pool...
   return ERROR_NO_ADDRESS;
}

#endif
