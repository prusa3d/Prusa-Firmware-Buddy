/**
 * @file slaac.c
 * @brief IPv6 Stateless Address Autoconfiguration
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
 * Stateless Address Autoconfiguration is a facility to allow devices to
 * configure themselves independently. Refer to the following RFCs for
 * complete details:
 * - RFC 4862: IPv6 Stateless Address Autoconfiguration
 * - RFC 6106: IPv6 Router Advertisement Options for DNS Configuration
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

//Switch to the appropriate trace level
#define TRACE_LEVEL SLAAC_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "core/ethernet.h"
#include "ipv6/ipv6.h"
#include "ipv6/ipv6_misc.h"
#include "ipv6/slaac.h"
#include "ipv6/ndp.h"
#include "ipv6/ndp_misc.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (IPV6_SUPPORT == ENABLED && SLAAC_SUPPORT == ENABLED)


/**
 * @brief Initialize settings with default values
 * @param[out] settings Structure that contains SLAAC settings
 **/

void slaacGetDefaultSettings(SlaacSettings *settings)
{
   //Use default interface
   settings->interface = netGetDefaultInterface();

   //Use the DNS servers specified by the RDNSS option
   settings->manualDnsConfig = FALSE;
   //Link state change event
   settings->linkChangeEvent = NULL;
   //Router Advertisement parsing callback
   settings->parseRouterAdvCallback = NULL;
}


/**
 * @brief SLAAC initialization
 * @param[in] context Pointer to the SLAAC context
 * @param[in] settings SLAAC specific settings
 * @return Error code
 **/

error_t slaacInit(SlaacContext *context, const SlaacSettings *settings)
{
   NetInterface *interface;

   //Debug message
   TRACE_INFO("Initializing SLAAC...\r\n");

   //Ensure the parameters are valid
   if(context == NULL || settings == NULL)
      return ERROR_INVALID_PARAMETER;

   //The SLAAC service must be bound to a valid interface
   if(settings->interface == NULL)
      return ERROR_INVALID_PARAMETER;

   //Point to the underlying network interface
   interface = settings->interface;

   //Clear the SLAAC context
   osMemset(context, 0, sizeof(SlaacContext));
   //Save user settings
   context->settings = *settings;

   //SLAAC operation is currently suspended
   context->running = FALSE;

   //Attach the SLAAC context to the network interface
   interface->slaacContext = context;

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief Start SLAAC process
 * @param[in] context Pointer to the SLAAC context
 * @return Error code
 **/

error_t slaacStart(SlaacContext *context)
{
   NetInterface *interface;

   //Make sure the SLAAC context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Debug message
   TRACE_INFO("Starting SLAAC...\r\n");

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Point to the underlying network interface
   interface = context->settings.interface;

   //Clear the list of IPv6 addresses
   ipv6FlushAddrList(interface);

   //Automatic DNS server configuration?
   if(!context->settings.manualDnsConfig)
   {
      //Clear the list of DNS servers
      ipv6FlushDnsServerList(interface);
   }

   //Check if the link is up?
   if(interface->linkState)
   {
      //A link-local address is formed by combining the well-known
      //link-local prefix fe80::/10 with the interface identifier
      slaacGenerateLinkLocalAddr(context);
   }

   //Start SLAAC operation
   context->running = TRUE;

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Stop SLAAC process
 * @param[in] context Pointer to the SLAAC context
 * @return Error code
 **/

error_t slaacStop(SlaacContext *context)
{
   //Make sure the SLAAC context is valid
   if(context == NULL)
      return ERROR_INVALID_PARAMETER;

   //Debug message
   TRACE_INFO("Stopping SLAAC...\r\n");

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Suspend SLAAC operation
   context->running = FALSE;

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Callback function for link change event
 * @param[in] context Pointer to the SLAAC context
 **/

void slaacLinkChangeEvent(SlaacContext *context)
{
   NetInterface *interface;

   //Make sure the SLAAC service has been properly instantiated
   if(context == NULL)
      return;

   //Point to the underlying network interface
   interface = context->settings.interface;

   //Check whether SLAAC is enabled
   if(context->running)
   {
      //Automatic DNS server configuration?
      if(!context->settings.manualDnsConfig)
      {
         //Clear the list of DNS servers
         ipv6FlushDnsServerList(interface);
      }

      //Link-up event?
      if(interface->linkState)
      {
         //A link-local address is formed by combining the well-known
         //link-local prefix fe80::/10 with the interface identifier
         slaacGenerateLinkLocalAddr(context);
      }
   }

   //Any registered callback?
   if(context->settings.linkChangeEvent != NULL)
   {
      //Release exclusive access
      osReleaseMutex(&netMutex);
      //Invoke user callback function
      context->settings.linkChangeEvent(context, interface, interface->linkState);
      //Get exclusive access
      osAcquireMutex(&netMutex);
   }
}


/**
 * @brief Parse Router Advertisement message
 * @param[in] context Pointer to the SLAAC context
 * @param[in] message Pointer to the Router Advertisement message
 * @param[in] length Length of the message, in bytes
 **/

void slaacParseRouterAdv(SlaacContext *context,
   NdpRouterAdvMessage *message, size_t length)
{
   uint_t i;
   uint_t n;
   NetInterface *interface;
   NdpPrefixInfoOption *prefixInfoOption;
   NdpRdnssOption *rdnssOption;

   //Point to the underlying network interface
   interface = context->settings.interface;

   //Check whether SLAAC is enabled
   if(!context->running)
      return;

   //Make sure that a valid link-local address has been assigned to the interface
   if(ipv6GetLinkLocalAddrState(interface) != IPV6_ADDR_STATE_PREFERRED)
      return;

   //Calculate the length of the Options field
   length -= sizeof(NdpRouterAdvMessage);

   //This flag tracks changes in IPv6 configuration
   context->configUpdated = FALSE;

   //Point to the beginning of the Options field
   n = 0;

   //Parse Options field
   while(1)
   {
      //Search the Options field for any Prefix Information options
      prefixInfoOption = ndpGetOption(message->options + n,
         length - n, NDP_OPT_PREFIX_INFORMATION);

      //No more option of the specified type?
      if(prefixInfoOption == NULL)
         break;

      //Parse the Prefix Information Option
      slaacParsePrefixInfoOption(context, prefixInfoOption);

      //Retrieve the offset to the current position
      n = (uint8_t *) prefixInfoOption - message->options;
      //Jump to the next option
      n += prefixInfoOption->length * 8;
   }

   //Automatic DNS server configuration?
   if(!context->settings.manualDnsConfig)
   {
      //Search for the Recursive DNS Server (RDNSS) option
      rdnssOption = ndpGetOption(message->options, length,
         NDP_OPT_RECURSIVE_DNS_SERVER);

      //RDNSS option found?
      if(rdnssOption != NULL && rdnssOption->length >= 1)
      {
         //Retrieve the number of addresses
         n = (rdnssOption->length - 1) / 2;

         //Loop through the list of DNS servers
         for(i = 0; i < n && i < IPV6_DNS_SERVER_LIST_SIZE; i++)
         {
            //Record DNS server address
            interface->ipv6Context.dnsServerList[i] = rdnssOption->address[i];
         }
      }
   }

   //Any registered callback?
   if(context->settings.parseRouterAdvCallback != NULL)
   {
      //Invoke user callback function
      context->settings.parseRouterAdvCallback(context, message, length);
   }

   //Check whether a new IPv6 address has been assigned to the interface
   if(context->configUpdated)
   {
      //Dump current IPv6 configuration for debugging purpose
      slaacDumpConfig(context);
   }
}


/**
 * @brief Parse Prefix Information Option
 * @param[in] context Pointer to the SLAAC context
 * @param[in] option Pointer to the Prefix Information option
 **/

void slaacParsePrefixInfoOption(SlaacContext *context,
   NdpPrefixInfoOption *option)
{
   uint_t i;
   bool_t found;
   systime_t time;
   systime_t validLifetime;
   systime_t preferredLifetime;
   systime_t remainingLifetime;
   NetInterface *interface;
   NetInterface *logicalInterface;
   Ipv6AddrEntry *entry;
   Ipv6Addr addr;

   //Make sure the Prefix Information option is valid
   if(option == NULL || option->length != 4)
      return;

   //If the Autonomous flag is not set, silently ignore the Prefix
   //Information option
   if(!option->a)
      return;

   //If the prefix is the link-local prefix, silently ignore the
   //Prefix Information option
   if(ipv6CompPrefix(&option->prefix, &IPV6_LINK_LOCAL_ADDR_PREFIX, 10))
      return;

   //Check whether the valid lifetime is zero
   if(ntohl(option->validLifetime) == 0)
      return;

   //If the preferred lifetime is greater than the valid lifetime,
   //silently ignore the Prefix Information option
   if(ntohl(option->preferredLifetime) > ntohl(option->validLifetime))
      return;

   //If the sum of the prefix length and interface identifier length does
   //not equal 128 bits, the Prefix Information option must be ignored
   if(option->prefixLength != 64)
      return;

   //Get current time
   time = osGetSystemTime();

   //Point to the underlying network interface
   interface = context->settings.interface;

   //Point to the logical interface
   logicalInterface = nicGetLogicalInterface(interface);

   //Form an address by combining the advertised prefix with the interface
   //identifier
   addr.w[0] = option->prefix.w[0];
   addr.w[1] = option->prefix.w[1];
   addr.w[2] = option->prefix.w[2];
   addr.w[3] = option->prefix.w[3];
   addr.w[4] = logicalInterface->eui64.w[0];
   addr.w[5] = logicalInterface->eui64.w[1];
   addr.w[6] = logicalInterface->eui64.w[2];
   addr.w[7] = logicalInterface->eui64.w[3];

   //Convert Valid Lifetime to host byte order
   validLifetime = ntohl(option->validLifetime);

   //Check the valid lifetime
   if(validLifetime != NDP_INFINITE_LIFETIME)
   {
      //The length of time in seconds that the prefix is valid for the
      //purpose of on-link determination
      if(validLifetime < (MAX_DELAY / 1000))
         validLifetime *= 1000;
      else
         validLifetime = MAX_DELAY;
   }
   else
   {
      //A value of all one bits (0xffffffff) represents infinity
      validLifetime = INFINITE_DELAY;
   }

   //Convert Preferred Lifetime to host byte order
   preferredLifetime = ntohl(option->preferredLifetime);

   //Check the preferred lifetime
   if(preferredLifetime != NDP_INFINITE_LIFETIME)
   {
      //The length of time in seconds that addresses generated from the
      //prefix via stateless address autoconfiguration remain preferred
      if(preferredLifetime < (MAX_DELAY / 1000))
         preferredLifetime *= 1000;
      else
         preferredLifetime = MAX_DELAY;
   }
   else
   {
      //A value of all one bits (0xffffffff) represents infinity
      preferredLifetime = INFINITE_DELAY;
   }

   //This flag will be set if the advertised prefix matches an address
   //assigned to the interface
   found = FALSE;

   //Loop through the list of IPv6 addresses
   for(i = 1; i < IPV6_ADDR_LIST_SIZE; i++)
   {
      //Point to the current entry
      entry = &interface->ipv6Context.addrList[i];

      //Check whether the advertised prefix is equal to the prefix of an
      //address configured by stateless autoconfiguration in the list
      if(ipv6CompPrefix(&entry->addr, &option->prefix, option->prefixLength))
      {
         //Valid address?
         if(entry->state == IPV6_ADDR_STATE_PREFERRED ||
            entry->state == IPV6_ADDR_STATE_DEPRECATED)
         {
            //Set flag
            found = TRUE;

            //The preferred lifetime of the address is reset to the Preferred
            //Lifetime in the received advertisement
            entry->preferredLifetime = preferredLifetime;

            //Compute the remaining time to the valid lifetime expiration
            //of the previously autoconfigured address
            if(timeCompare(time, entry->timestamp + entry->validLifetime) < 0)
               remainingLifetime = entry->timestamp + entry->validLifetime - time;
            else
               remainingLifetime = 0;

            //The specific action to perform for the valid lifetime of the
            //address depends on the Valid Lifetime in the received Router
            //Advertisement and the remaining time
            if(validLifetime > SLAAC_LIFETIME_2_HOURS ||
               validLifetime > remainingLifetime)
            {
               //If the received Valid Lifetime is greater than 2 hours or
               //greater than remaining lifetime, set the valid lifetime of
               //the corresponding address to the advertised Valid Lifetime
               entry->validLifetime = validLifetime;

               //Save current time
               entry->timestamp = time;
               //Update the state of the IPv6 address
               entry->state = IPV6_ADDR_STATE_PREFERRED;
            }
            else if(remainingLifetime <= SLAAC_LIFETIME_2_HOURS)
            {
               //If remaining lifetime is less than or equal to 2 hours, ignore
               //the Prefix Information option with regards to the valid lifetime
            }
            else
            {
               //Otherwise, reset the valid lifetime of the corresponding
               //address to 2 hours
               entry->validLifetime = SLAAC_LIFETIME_2_HOURS;

               //Save current time
               entry->timestamp = time;
               //Update the state of the IPv6 address
               entry->state = IPV6_ADDR_STATE_PREFERRED;
            }
         }
         //Tentative address?
         else if(entry->state == IPV6_ADDR_STATE_TENTATIVE)
         {
            //Do not update the preferred and valid lifetimes of the address
            //when Duplicate Address Detection is being performed
            found = TRUE;
         }
      }
   }

   //The IPv6 address is not yet in the list?
   if(!found)
   {
      //Loop through the list of IPv6 addresses
      for(i = 1; i < IPV6_ADDR_LIST_SIZE; i++)
      {
         //Point to the current entry
         entry = &interface->ipv6Context.addrList[i];

         //Check the state of the IPv6 address
         if(entry->state == IPV6_ADDR_STATE_INVALID)
         {
            //If an address is formed successfully and the address is not yet
            //in the list, the host adds it to the list of addresses assigned
            //to the interface, initializing its preferred and valid lifetime
            //values from the Prefix Information option
            if(interface->ndpContext.dupAddrDetectTransmits > 0)
            {
               //Use the IPv6 address as a tentative address
               ipv6SetAddr(interface, i, &addr, IPV6_ADDR_STATE_TENTATIVE,
                  validLifetime, preferredLifetime, FALSE);
            }
            else
            {
               //The use of the IPv6 address is now unrestricted
               ipv6SetAddr(interface, i, &addr, IPV6_ADDR_STATE_PREFERRED,
                  validLifetime, preferredLifetime, FALSE);
            }

            //A new IPv6 address has just been assigned to the interface
            context->configUpdated = TRUE;
            //We are done
            break;
         }
      }
   }
}


/**
 * @brief Generate a link-local address
 * @param[in] context Pointer to the SLAAC context
 * @return Error code
 **/

error_t slaacGenerateLinkLocalAddr(SlaacContext *context)
{
   error_t error;
   NetInterface *interface;
   NetInterface *logicalInterface;
   Ipv6Addr addr;

   //Point to the underlying network interface
   interface = context->settings.interface;

   //Point to the logical interface
   logicalInterface = nicGetLogicalInterface(interface);

   //Check whether a link-local address has been manually assigned
   if(interface->ipv6Context.addrList[0].state != IPV6_ADDR_STATE_INVALID &&
      interface->ipv6Context.addrList[0].permanent)
   {
      //Keep using the current link-local address
      error = NO_ERROR;
   }
   else
   {
      //A link-local address is formed by combining the well-known
      //link-local prefix fe80::/10 with the interface identifier
      ipv6GenerateLinkLocalAddr(&logicalInterface->eui64, &addr);

      //Check whether Duplicate Address Detection should be performed
      if(interface->ndpContext.dupAddrDetectTransmits > 0)
      {
         //Use the link-local address as a tentative address
         error = ipv6SetAddr(interface, 0, &addr, IPV6_ADDR_STATE_TENTATIVE,
            NDP_INFINITE_LIFETIME, NDP_INFINITE_LIFETIME, FALSE);
      }
      else
      {
         //The use of the link-local address is now unrestricted
         error = ipv6SetAddr(interface, 0, &addr, IPV6_ADDR_STATE_PREFERRED,
            NDP_INFINITE_LIFETIME, NDP_INFINITE_LIFETIME, FALSE);
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Dump IPv6 configuration for debugging purpose
 * @param[in] context Pointer to the SLAAC context
 **/

void slaacDumpConfig(SlaacContext *context)
{
#if (SLAAC_TRACE_LEVEL >= TRACE_LEVEL_INFO)
   uint_t i;
   NetInterface *interface;
   Ipv6Context *ipv6Context;

   //Point to the underlying network interface
   interface = context->settings.interface;
   //Point to the IPv6 context
   ipv6Context = &interface->ipv6Context;

   //Debug message
   TRACE_INFO("\r\n");
   TRACE_INFO("SLAAC configuration:\r\n");

   //Link-local address
   TRACE_INFO("  Link-local Address = %s\r\n",
      ipv6AddrToString(&ipv6Context->addrList[0].addr, NULL));

   //Global addresses
   for(i = 1; i < IPV6_ADDR_LIST_SIZE; i++)
   {
      TRACE_INFO("  Global Address %u = %s\r\n", i,
         ipv6AddrToString(&ipv6Context->addrList[i].addr, NULL));
   }

   //IPv6 prefixes
   for(i = 0; i < IPV6_PREFIX_LIST_SIZE; i++)
   {
      TRACE_INFO("  Prefix %u = %s/%" PRIu8 "\r\n", i + 1,
         ipv6AddrToString(&ipv6Context->prefixList[i].prefix, NULL),
         ipv6Context->prefixList[i].prefixLen);
   }

   //Default routers
   for(i = 0; i < IPV6_ROUTER_LIST_SIZE; i++)
   {
      TRACE_INFO("  Default Router %u = %s\r\n", i + 1,
         ipv6AddrToString(&ipv6Context->routerList[i].addr, NULL));
   }

   //DNS servers
   for(i = 0; i < IPV6_DNS_SERVER_LIST_SIZE; i++)
   {
      TRACE_INFO("  DNS Server %u = %s\r\n", i + 1,
         ipv6AddrToString(&ipv6Context->dnsServerList[i], NULL));
   }

   //Maximum transmit unit
   TRACE_INFO("  MTU = %" PRIuSIZE "\r\n", ipv6Context->linkMtu);
   TRACE_INFO("\r\n");
#endif
}

#endif
