/**
 * @file net.c
 * @brief TCP/IP stack core
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
#define TRACE_LEVEL NIC_TRACE_LEVEL

//Dependencies
#include <stdlib.h>
#include "core/net.h"
#include "core/socket.h"
#include "core/raw_socket.h"
#include "core/tcp_timer.h"
#include "core/tcp_misc.h"
#include "core/ethernet.h"
#include "ipv4/arp.h"
#include "ipv4/ipv4.h"
#include "ipv4/ipv4_routing.h"
#include "ipv4/igmp.h"
#include "ipv6/ipv6.h"
#include "ipv6/ipv6_routing.h"
#include "ipv6/mld.h"
#include "ipv6/ndp.h"
#include "ipv6/ndp_router_adv.h"
#include "dhcp/dhcp_client.h"
#include "dhcp/dhcp_server.h"
#include "dns/dns_cache.h"
#include "dns/dns_client.h"
#include "mdns/mdns_client.h"
#include "mdns/mdns_responder.h"
#include "mdns/mdns_common.h"
#include "dns_sd/dns_sd.h"
#include "netbios/nbns_client.h"
#include "netbios/nbns_responder.h"
#include "netbios/nbns_common.h"
#include "llmnr/llmnr_responder.h"
#include "str.h"
#include "debug.h"

#if (WEB_SOCKET_SUPPORT == ENABLED)
   #include "web_socket/web_socket.h"
#endif

//TCP/IP stack context
NetContext netContext;


/**
 * @brief TCP/IP stack initialization
 * @return Error code
 **/

error_t netInit(void)
{
   error_t error;
   uint_t i;
   NetInterface *interface;

   //Clear TCP/IP stack context
   osMemset(&netContext, 0, sizeof(NetContext));

   //The TCP/IP process is currently suspended
   netTaskRunning = FALSE;
   //Get current time
   netTimestamp = osGetSystemTime();

   //Create a mutex to prevent simultaneous access to the TCP/IP stack
   if(!osCreateMutex(&netMutex))
   {
      //Failed to create mutex
      return ERROR_OUT_OF_RESOURCES;
   }

   //Create a event object to receive notifications from device drivers
   if(!osCreateEvent(&netEvent))
   {
      //Failed to create mutex
      return ERROR_OUT_OF_RESOURCES;
   }

   //Memory pool initialization
   error = memPoolInit();
   //Any error to report?
   if(error)
      return error;

   //Clear configuration data for each interface
   osMemset(netInterface, 0, sizeof(netInterface));

   //Loop through network interfaces
   for(i = 0; i < NET_INTERFACE_COUNT; i++)
   {
      //Point to the current interface
      interface = &netInterface[i];

      //Default interface name
      osSprintf(interface->name, "eth%u", i);

      //Zero-based index
      interface->index = i;
      //Unique number identifying the interface
      interface->id = i;

#if (ETH_SUPPORT == ENABLED)
      //Default PHY address
      interface->phyAddr = UINT8_MAX;
#endif
   }

   //Socket related initialization
   error = socketInit();
   //Any error to report?
   if(error)
      return error;

#if (WEB_SOCKET_SUPPORT == ENABLED)
   //WebSocket related initialization
   webSocketInit();
#endif

#if (IPV4_SUPPORT == ENABLED && IPV4_ROUTING_SUPPORT == ENABLED)
   //Initialize IPv4 routing table
   error = ipv4InitRouting();
   //Any error to report?
   if(error)
      return error;
#endif

#if (IPV6_SUPPORT == ENABLED && IPV6_ROUTING_SUPPORT == ENABLED)
   //Initialize IPv6 routing table
   error = ipv6InitRouting();
   //Any error to report?
   if(error)
      return error;
#endif

#if (UDP_SUPPORT == ENABLED)
   //UDP related initialization
   error = udpInit();
   //Any error to report?
   if(error)
      return error;
#endif

#if (TCP_SUPPORT == ENABLED)
   //TCP related initialization
   error = tcpInit();
   //Any error to report?
   if(error)
      return error;
#endif

#if (DNS_CLIENT_SUPPORT == ENABLED || MDNS_CLIENT_SUPPORT == ENABLED || \
   NBNS_CLIENT_SUPPORT == ENABLED)
   //DNS cache initialization
   error = dnsInit();
   //Any error to report?
   if(error)
      return error;
#endif

   //Initialize tick counters
   nicTickCounter = 0;

#if (PPP_SUPPORT == ENABLED)
   pppTickCounter = 0;
#endif
#if (IPV4_SUPPORT == ENABLED && ETH_SUPPORT == ENABLED)
   arpTickCounter = 0;
#endif
#if (IPV4_SUPPORT == ENABLED && IPV4_FRAG_SUPPORT == ENABLED)
   ipv4FragTickCounter = 0;
#endif
#if (IPV4_SUPPORT == ENABLED && IGMP_SUPPORT == ENABLED)
   igmpTickCounter = 0;
#endif
#if (IPV4_SUPPORT == ENABLED && AUTO_IP_SUPPORT == ENABLED)
   autoIpTickCounter = 0;
#endif
#if (IPV4_SUPPORT == ENABLED && DHCP_CLIENT_SUPPORT == ENABLED)
   dhcpClientTickCounter = 0;
#endif
#if (IPV4_SUPPORT == ENABLED && DHCP_SERVER_SUPPORT == ENABLED)
   dhcpServerTickCounter = 0;
#endif
#if (IPV6_SUPPORT == ENABLED && IPV6_FRAG_SUPPORT == ENABLED)
   ipv6FragTickCounter = 0;
#endif
#if (IPV6_SUPPORT == ENABLED && MLD_SUPPORT == ENABLED)
   mldTickCounter = 0;
#endif
#if (IPV6_SUPPORT == ENABLED && NDP_SUPPORT == ENABLED)
   ndpTickCounter = 0;
#endif
#if (IPV6_SUPPORT == ENABLED && NDP_ROUTER_ADV_SUPPORT == ENABLED)
   ndpRouterAdvTickCounter = 0;
#endif
#if (IPV6_SUPPORT == ENABLED && DHCPV6_CLIENT_SUPPORT == ENABLED)
   dhcpv6ClientTickCounter = 0;
#endif
#if (TCP_SUPPORT == ENABLED)
   tcpTickCounter = 0;
#endif
#if (DNS_CLIENT_SUPPORT == ENABLED || MDNS_CLIENT_SUPPORT == ENABLED || \
   NBNS_CLIENT_SUPPORT == ENABLED)
   dnsTickCounter = 0;
#endif
#if (MDNS_RESPONDER_SUPPORT == ENABLED)
   mdnsResponderTickCounter = 0;
#endif
#if (DNS_SD_SUPPORT == ENABLED)
   dnsSdTickCounter = 0;
#endif

#if (NET_STATIC_OS_RESOURCES == ENABLED)
   //Create a task to handle TCP/IP events
   osCreateStaticTask(&netTaskInstance, "TCP/IP Stack", (OsTaskCode) netTask,
      NULL, netTaskStack, NET_TASK_STACK_SIZE, NET_TASK_PRIORITY);
#else
   //Create a task to handle TCP/IP events
   netTaskHandle = osCreateTask("TCP/IP Stack", (OsTaskCode) netTask,
      NULL, NET_TASK_STACK_SIZE, NET_TASK_PRIORITY);

   //Unable to create the task?
   if(netTaskHandle == OS_INVALID_HANDLE)
      return ERROR_OUT_OF_RESOURCES;
#endif

#if (NET_RTOS_SUPPORT == DISABLED)
   //The TCP/IP process is now running
   netTaskRunning = TRUE;
#endif

   //Successful initialization
   return NO_ERROR;
}


/**
 * @brief Seed the pseudo-random number generator
 * @param[in] seed Pointer to the random seed
 * @param[in] length Length of the random seed, in bytes
 * @return Error code
 **/

error_t netSeedRand(const uint8_t *seed, size_t length)
{
   size_t i;
   size_t j;

   //Check parameters
   if(seed == NULL || length == 0)
      return ERROR_INVALID_PARAMETER;

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Save random seed
   for(i = 0, j = 0; i < NET_RAND_SEED_SIZE; i++)
   {
      //Copy current byte
      netContext.randSeed[i] = seed[j];

      //Increment index and wrap around if necessary
      if(++j >= length)
      {
         j = 0;
      }
   }

   //Initialize pseudo-random generator
   netInitRand();

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Get default network interface
 * @return Pointer to the default network interface to be used
 **/

NetInterface *netGetDefaultInterface(void)
{
   //Default network interface
   return &netInterface[0];
}


/**
 * @brief Set MAC address
 * @param[in] interface Pointer to the desired network interface
 * @param[in] macAddr MAC address
 * @return Error code
 **/

error_t netSetMacAddr(NetInterface *interface, const MacAddr *macAddr)
{
#if (ETH_SUPPORT == ENABLED)
   //Check parameters
   if(interface == NULL || macAddr == NULL)
      return ERROR_INVALID_PARAMETER;

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Set MAC address
   interface->macAddr = *macAddr;

   //Generate the 64-bit interface identifier
   macAddrToEui64(macAddr, &interface->eui64);

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Retrieve MAC address
 * @param[in] interface Pointer to the desired network interface
 * @param[out] macAddr MAC address
 * @return Error code
 **/

error_t netGetMacAddr(NetInterface *interface, MacAddr *macAddr)
{
#if (ETH_SUPPORT == ENABLED)
   NetInterface *logicalInterface;

   //Check parameters
   if(interface == NULL || macAddr == NULL)
      return ERROR_INVALID_PARAMETER;

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Point to the logical interface
   logicalInterface = nicGetLogicalInterface(interface);

   //Get MAC address
   *macAddr = logicalInterface->macAddr;

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Set EUI-64 interface identifier
 * @param[in] interface Pointer to the desired network interface
 * @param[in] eui64 Interface identifier
 * @return Error code
 **/

error_t netSetEui64(NetInterface *interface, const Eui64 *eui64)
{
   //Check parameters
   if(interface == NULL || eui64 == NULL)
      return ERROR_INVALID_PARAMETER;

   //Get exclusive access
   osAcquireMutex(&netMutex);
   //Set interface identifier
   interface->eui64 = *eui64;
   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Retrieve EUI-64 interface identifier
 * @param[in] interface Pointer to the desired network interface
 * @param[out] eui64 Interface identifier
 * @return Error code
 **/

error_t netGetEui64(NetInterface *interface, Eui64 *eui64)
{
   NetInterface *logicalInterface;

   //Check parameters
   if(interface == NULL || eui64 == NULL)
      return ERROR_INVALID_PARAMETER;

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Point to the logical interface
   logicalInterface = nicGetLogicalInterface(interface);

   //Get interface identifier
   *eui64 = logicalInterface->eui64;

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set interface identifier
 * @param[in] interface Pointer to the desired network interface
 * @param[in] id Unique number identifying the interface
 * @return Error code
 **/

error_t netSetInterfaceId(NetInterface *interface, uint32_t id)
{
   //Check parameters
   if(interface == NULL)
      return ERROR_INVALID_PARAMETER;

   //Get exclusive access
   osAcquireMutex(&netMutex);
   //Set interface identifier
   interface->id = id;
   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set interface name
 * @param[in] interface Pointer to the desired network interface
 * @param[in] name NULL-terminated string that contains the interface name
 * @return Error code
 **/

error_t netSetInterfaceName(NetInterface *interface, const char_t *name)
{
   //Check parameters
   if(interface == NULL || name == NULL)
      return ERROR_INVALID_PARAMETER;

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Set interface name
   strSafeCopy(interface->name, name, NET_MAX_IF_NAME_LEN);

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set host name
 * @param[in] interface Pointer to the desired network interface
 * @param[in] name NULL-terminated string that contains the host name
 * @return Error code
 **/

error_t netSetHostname(NetInterface *interface, const char_t *name)
{
   //Check parameters
   if(interface == NULL || name == NULL)
      return ERROR_INVALID_PARAMETER;

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Set host name
   strSafeCopy(interface->hostname, name, NET_MAX_HOSTNAME_LEN);

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Specify VLAN identifier (802.1Q)
 * @param[in] interface Pointer to the desired network interface
 * @param[in] vlanId VLAN identifier
 * @return Error code
 **/

error_t netSetVlanId(NetInterface *interface, uint16_t vlanId)
{
#if (ETH_VLAN_SUPPORT == ENABLED)
   //Make sure the network interface is valid
   if(interface == NULL)
      return ERROR_INVALID_PARAMETER;

   //The VID value FFF is reserved
   if((vlanId & VLAN_VID_MASK) == VLAN_VID_MASK)
      return ERROR_INVALID_PARAMETER;

   //Get exclusive access
   osAcquireMutex(&netMutex);
   //Set VLAN identifier
   interface->vlanId = vlanId;
   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Specify VMAN identifier (802.1ad)
 * @param[in] interface Pointer to the desired network interface
 * @param[in] vmanId VMAN identifier
 * @return Error code
 **/

error_t netSetVmanId(NetInterface *interface, uint16_t vmanId)
{
#if (ETH_VMAN_SUPPORT == ENABLED)
   //Make sure the network interface is valid
   if(interface == NULL)
      return ERROR_INVALID_PARAMETER;

   //The VID value FFF is reserved
   if((vmanId & VLAN_VID_MASK) == VLAN_VID_MASK)
      return ERROR_INVALID_PARAMETER;

   //Get exclusive access
   osAcquireMutex(&netMutex);
   //Set VMAN identifier
   interface->vmanId = vmanId;
   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Attach a virtual interface to a given physical interface
 * @param[in] interface Pointer to the virtual interface
 * @param[in] physicalInterface physical interface on top of which the virtual
 *   interface will run
 * @return Error code
 **/

error_t netSetParentInterface(NetInterface *interface,
   NetInterface *physicalInterface)
{
#if (ETH_VIRTUAL_IF_SUPPORT == ENABLED || ETH_VLAN_SUPPORT == ENABLED || \
   ETH_PORT_TAGGING_SUPPORT == ENABLED)
   //Make sure the network interface is valid
   if(interface == NULL)
      return ERROR_INVALID_PARAMETER;

   //Get exclusive access
   osAcquireMutex(&netMutex);
   //Bind the virtual interface to the physical interface
   interface->parent = physicalInterface;
   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Set Ethernet MAC driver
 * @param[in] interface Pointer to the desired network interface
 * @param[in] driver Ethernet MAC driver
 * @return Error code
 **/

error_t netSetDriver(NetInterface *interface, const NicDriver *driver)
{
   //Check parameters
   if(interface == NULL || driver == NULL)
      return ERROR_INVALID_PARAMETER;

   //Get exclusive access
   osAcquireMutex(&netMutex);
   //Set Ethernet MAC driver
   interface->nicDriver = driver;
   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set Ethernet PHY driver
 * @param[in] interface Pointer to the desired network interface
 * @param[in] driver Ethernet PHY driver (can be NULL for MAC + PHY controller)
 * @return Error code
 **/

error_t netSetPhyDriver(NetInterface *interface, const PhyDriver *driver)
{
#if (ETH_SUPPORT == ENABLED)
   //Check parameters
   if(interface == NULL || driver == NULL)
      return ERROR_INVALID_PARAMETER;

   //Get exclusive access
   osAcquireMutex(&netMutex);
   //Set Ethernet PHY driver
   interface->phyDriver = driver;
   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Specify Ethernet PHY address
 * @param[in] interface Pointer to the desired network interface
 * @param[in] phyAddr PHY address
 * @return Error code
 **/

error_t netSetPhyAddr(NetInterface *interface, uint8_t phyAddr)
{
#if (ETH_SUPPORT == ENABLED)
   //Make sure the network interface is valid
   if(interface == NULL)
      return ERROR_INVALID_PARAMETER;

   //Make sure the PHY address is valid
   if(phyAddr >= 32)
      return ERROR_OUT_OF_RANGE;

   //Get exclusive access
   osAcquireMutex(&netMutex);
   //Set PHY address
   interface->phyAddr = phyAddr;
   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Set Ethernet switch driver
 * @param[in] interface Pointer to the desired network interface
 * @param[in] driver Ethernet switch driver
 * @return Error code
 **/

error_t netSetSwitchDriver(NetInterface *interface, const SwitchDriver *driver)
{
#if (ETH_SUPPORT == ENABLED)
   //Check parameters
   if(interface == NULL || driver == NULL)
      return ERROR_INVALID_PARAMETER;

   //Get exclusive access
   osAcquireMutex(&netMutex);
   //Set Ethernet switch driver
   interface->switchDriver = driver;
   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Specify switch port
 * @param[in] interface Pointer to the desired network interface
 * @param[in] port Switch port identifier
 * @return Error code
 **/

error_t netSetSwitchPort(NetInterface *interface, uint8_t port)
{
#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
   //Make sure the network interface is valid
   if(interface == NULL)
      return ERROR_INVALID_PARAMETER;

   //Get exclusive access
   osAcquireMutex(&netMutex);
   //Set switch port identifier
   interface->port = port;
   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Set SMI driver
 * @param[in] interface Pointer to the desired network interface
 * @param[in] driver Underlying SMI driver
 * @return Error code
 **/

error_t netSetSmiDriver(NetInterface *interface, const SmiDriver *driver)
{
#if (ETH_SUPPORT == ENABLED)
   //Check parameters
   if(interface == NULL || driver == NULL)
      return ERROR_INVALID_PARAMETER;

   //Get exclusive access
   osAcquireMutex(&netMutex);
   //Set SMI driver
   interface->smiDriver = driver;
   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
#else
   //Not implemented
   return ERROR_NOT_IMPLEMENTED;
#endif
}


/**
 * @brief Set SPI driver
 * @param[in] interface Pointer to the desired network interface
 * @param[in] driver Underlying SPI driver
 * @return Error code
 **/

error_t netSetSpiDriver(NetInterface *interface, const SpiDriver *driver)
{
   //Check parameters
   if(interface == NULL || driver == NULL)
      return ERROR_INVALID_PARAMETER;

   //Get exclusive access
   osAcquireMutex(&netMutex);
   //Set SPI driver
   interface->spiDriver = driver;
   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set UART driver
 * @param[in] interface Pointer to the desired network interface
 * @param[in] driver Underlying UART driver
 * @return Error code
 **/

error_t netSetUartDriver(NetInterface *interface, const UartDriver *driver)
{
   //Check parameters
   if(interface == NULL || driver == NULL)
      return ERROR_INVALID_PARAMETER;

   //Get exclusive access
   osAcquireMutex(&netMutex);
   //Set UART driver
   interface->uartDriver = driver;
   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set external interrupt line driver
 * @param[in] interface Pointer to the desired network interface
 * @param[in] driver Underlying SPI driver
 * @return Error code
 **/

error_t netSetExtIntDriver(NetInterface *interface, const ExtIntDriver *driver)
{
   //Check parameters
   if(interface == NULL || driver == NULL)
      return ERROR_INVALID_PARAMETER;

   //Get exclusive access
   osAcquireMutex(&netMutex);
   //Set external interrupt line driver
   interface->extIntDriver = driver;
   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set administrative link state
 * @param[in] interface Pointer to the desired network interface
 * @param[in] linkState Administrative link state (up, down or auto)
 * @return Error code
 **/

error_t netSetLinkState(NetInterface *interface, NicLinkState linkState)
{
   //Make sure the network interface is valid
   if(interface == NULL)
      return ERROR_INVALID_PARAMETER;

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Any change detected?
   if(linkState != interface->linkState)
   {
      //Update link state
      interface->linkState = linkState;
      //Process link state change event
      netProcessLinkChange(interface);
   }

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Get link state
 * @param[in] interface Pointer to the desired network interface
 * @return Link state
 **/

bool_t netGetLinkState(NetInterface *interface)
{
   bool_t linkState;

   //Make sure the network interface is valid
   if(interface == NULL)
      return FALSE;

   //Get exclusive access
   osAcquireMutex(&netMutex);
   //Retrieve link state
   linkState = interface->linkState;
   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Return link state
   return linkState;
}


/**
 * @brief Configure network interface
 * @param[in] interface Network interface to configure
 * @return Error code
 **/

error_t netConfigInterface(NetInterface *interface)
{
   error_t error;

   //Make sure the network interface is valid
   if(interface == NULL)
      return ERROR_INVALID_PARAMETER;

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Disable hardware interrupts
   if(interface->nicDriver != NULL)
      interface->nicDriver->disableIrq(interface);

   //Start of exception handling block
   do
   {
      //Receive notifications when the transmitter is ready to send
      if(!osCreateEvent(&interface->nicTxEvent))
      {
         //Failed to create event object
         error = ERROR_OUT_OF_RESOURCES;
         //Stop immediately
         break;
      }

      //Valid NIC driver?
      if(interface->nicDriver != NULL)
      {
         //Network controller initialization
         error = interface->nicDriver->init(interface);
         //Any error to report?
         if(error)
            break;
      }
      else
      {
#if (ETH_VIRTUAL_IF_SUPPORT == ENABLED || ETH_PORT_TAGGING_SUPPORT == ENABLED)
         NetInterface *physicalInterface;

         //Point to the physical interface
         physicalInterface = nicGetPhysicalInterface(interface);

         //Check whether the network interface is a virtual interface
         if(physicalInterface != interface)
         {
            //Valid MAC address assigned to the virtual interface?
            if(!macCompAddr(&interface->macAddr, &MAC_UNSPECIFIED_ADDR))
            {
               //Configure the physical interface to accept the MAC address of
               //the virtual interface
               error = ethAcceptMacAddr(physicalInterface, &interface->macAddr);
               //Any error to report?
               if(error)
                  break;
            }
         }
#endif
      }

#if (ETH_SUPPORT == ENABLED)
      //Ethernet related initialization
      error = ethInit(interface);
      //Any error to report?
      if(error)
         break;
#endif

#if (IPV4_SUPPORT == ENABLED)
      //IPv4 initialization
      error = ipv4Init(interface);
      //Any error to report?
      if(error)
         break;

#if (ETH_SUPPORT == ENABLED)
      //ARP cache initialization
      error = arpInit(interface);
      //Any error to report?
      if(error)
         break;
#endif

#if (IGMP_SUPPORT == ENABLED)
      //IGMP related initialization
      error = igmpInit(interface);
      //Any error to report?
      if(error)
         break;

      //Join the all-systems group
      error = ipv4JoinMulticastGroup(interface, IGMP_ALL_SYSTEMS_ADDR);
      //Any error to report?
      if(error)
         break;
#endif

#if (NBNS_CLIENT_SUPPORT == ENABLED || NBNS_RESPONDER_SUPPORT == ENABLED)
      //NetBIOS Name Service related initialization
      error = nbnsInit(interface);
      //Any error to report?
      if(error)
         break;
#endif
#endif

#if (IPV6_SUPPORT == ENABLED)
      //IPv6 initialization
      error = ipv6Init(interface);
      //Any error to report?
      if(error)
         break;

#if (NDP_SUPPORT == ENABLED)
      //NDP related initialization
      error = ndpInit(interface);
      //Any error to report?
      if(error)
         break;
#endif

#if (MLD_SUPPORT == ENABLED)
      //MLD related initialization
      error = mldInit(interface);
      //Any error to report?
      if(error)
         break;
#endif

      //Join the All-Nodes multicast address
      error = ipv6JoinMulticastGroup(interface, &IPV6_LINK_LOCAL_ALL_NODES_ADDR);
      //Any error to report?
      if(error)
         break;
#endif

#if (MDNS_CLIENT_SUPPORT == ENABLED || MDNS_RESPONDER_SUPPORT == ENABLED)
      //mDNS related initialization
      error = mdnsInit(interface);
      //Any error to report?
      if(error)
         break;
#endif

#if (LLMNR_RESPONDER_SUPPORT == ENABLED)
      //LLMNR responder initialization
      error = llmnrResponderInit(interface);
      //Any error to report?
      if(error)
         break;
#endif

      //End of exception handling block
   } while(0);

   //Check status code
   if(!error)
   {
      //Initialize pseudo-random generator
      netInitRand();

      //The network interface is now fully configured
      interface->configured = TRUE;

      //Check whether the TCP/IP process is running
      if(netTaskRunning)
      {
         //Interrupts can be safely enabled
         if(interface->nicDriver != NULL)
            interface->nicDriver->enableIrq(interface);
      }
   }
   else
   {
      //Clean up side effects before returning
      osDeleteEvent(&interface->nicTxEvent);
   }

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Return status code
   return error;
}


/**
 * @brief Start network interface
 * @param[in] interface Network interface to start
 * @return Error code
 **/

error_t netStartInterface(NetInterface *interface)
{
   error_t error;

   //Make sure the network interface is valid
   if(interface == NULL)
      return ERROR_INVALID_PARAMETER;

   //Initialize status code
   error = NO_ERROR;

   //Get exclusive access
   osAcquireMutex(&netMutex);

#if (ETH_SUPPORT == ENABLED)
   //Check whether the interface is enabled for operation
   if(!interface->configured)
   {
      NetInterface *physicalInterface;

      //Point to the physical interface
      physicalInterface = nicGetPhysicalInterface(interface);

      //Virtual interface?
      if(interface != physicalInterface)
      {
         //Valid MAC address assigned to the virtual interface?
         if(!macCompAddr(&interface->macAddr, &MAC_UNSPECIFIED_ADDR))
         {
            //Configure the physical interface to accept the MAC address of
            //the virtual interface
            error = ethAcceptMacAddr(physicalInterface, &interface->macAddr);
         }
      }
      else
      {
#if (ETH_PORT_TAGGING_SUPPORT == ENABLED)
         //Valid switch driver?
         if(interface->switchDriver != NULL &&
            interface->switchDriver->init != NULL)
         {
            //Reconfigure switch operation
            error = interface->switchDriver->init(interface);
         }
#endif
         //Check status code
         if(!error)
         {
            //Update the MAC filter
            error = nicUpdateMacAddrFilter(interface);
         }
      }
   }
#endif

   //Enable network interface
   interface->configured = TRUE;

   //Check whether the TCP/IP process is running
   if(netTaskRunning)
   {
      //Interrupts can be safely enabled
      if(interface->nicDriver != NULL)
         interface->nicDriver->enableIrq(interface);
   }

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Return status code
   return error;
}


/**
 * @brief Stop network interface
 * @param[in] interface Network interface to stop
 * @return Error code
 **/

error_t netStopInterface(NetInterface *interface)
{
   NetInterface *physicalInterface;

   //Make sure the network interface is valid
   if(interface == NULL)
      return ERROR_INVALID_PARAMETER;

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Point to the physical interface
   physicalInterface = nicGetPhysicalInterface(interface);

   //Check whether the interface is enabled for operation
   if(interface->configured)
   {
      //Update link state
      interface->linkState = FALSE;
      //Process link state change event
      netProcessLinkChange(interface);

      //Disable hardware interrupts
      if(interface->nicDriver != NULL)
         interface->nicDriver->disableIrq(interface);

      //Disable network interface
      interface->configured = FALSE;

      //Virtual interface?
      if(interface != physicalInterface)
      {
#if (ETH_SUPPORT == ENABLED)
         //Valid MAC address assigned to the virtual interface?
         if(!macCompAddr(&interface->macAddr, &MAC_UNSPECIFIED_ADDR))
         {
            //Drop the corresponding address from the MAC filter table of
            //the physical interface
            ethDropMacAddr(physicalInterface, &interface->macAddr);
         }
#endif
      }
   }

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Successful operation
   return NO_ERROR;
}


/**
 * @brief TCP/IP events handling
 **/

void netTask(void)
{
   uint_t i;
   bool_t status;
   systime_t time;
   systime_t timeout;
   NetInterface *interface;

#if (NET_RTOS_SUPPORT == ENABLED)
   //Task prologue
   osEnterTask();

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //The TCP/IP process is now running
   netTaskRunning = TRUE;

   //Loop through network interfaces
   for(i = 0; i < NET_INTERFACE_COUNT; i++)
   {
      //Point to the current network interface
      interface = &netInterface[i];

      //Check whether the interface is fully configured
      if(interface->configured)
      {
         //Interrupts can be safely enabled
         if(interface->nicDriver != NULL)
         {
            interface->nicDriver->enableIrq(interface);
         }
      }
   }

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Main loop
   while(1)
   {
#endif
      //Get current time
      time = osGetSystemTime();

      //Compute the maximum blocking time when waiting for an event
      if(timeCompare(time, netTimestamp) < 0)
         timeout = netTimestamp - time;
      else
         timeout = 0;

      //Receive notifications when a frame has been received, or the
      //link state of any network interfaces has changed
      status = osWaitForEvent(&netEvent, timeout);

      //Check whether the specified event is in signaled state
      if(status)
      {
         //Get exclusive access
         osAcquireMutex(&netMutex);

         //Process events
         for(i = 0; i < NET_INTERFACE_COUNT; i++)
         {
            //Point to the current network interface
            interface = &netInterface[i];

            //Check whether a NIC event is pending
            if(interface->nicEvent)
            {
               //Acknowledge the event by clearing the flag
               interface->nicEvent = FALSE;

               //Valid NIC driver?
               if(interface->nicDriver != NULL)
               {
                  //Disable hardware interrupts
                  interface->nicDriver->disableIrq(interface);
                  //Handle NIC events
                  interface->nicDriver->eventHandler(interface);
                  //Re-enable hardware interrupts
                  interface->nicDriver->enableIrq(interface);
               }
            }

#if (ETH_SUPPORT == ENABLED)
            //Check whether a PHY event is pending
            if(interface->phyEvent)
            {
               //Acknowledge the event by clearing the flag
               interface->phyEvent = FALSE;

               //Valid NIC driver?
               if(interface->nicDriver != NULL)
               {
                  //Disable hardware interrupts
                  interface->nicDriver->disableIrq(interface);

                  //Valid Ethernet PHY or switch driver?
                  if(interface->phyDriver != NULL)
                  {
                     //Handle events
                     interface->phyDriver->eventHandler(interface);
                  }
                  else if(interface->switchDriver != NULL)
                  {
                     //Handle events
                     interface->switchDriver->eventHandler(interface);
                  }
                  else
                  {
                     //The interface is not properly configured
                  }

                  //Re-enable hardware interrupts
                  interface->nicDriver->enableIrq(interface);
               }
            }
#endif
         }

         //Release exclusive access
         osReleaseMutex(&netMutex);
      }

      //Get current time
      time = osGetSystemTime();

      //Check current time
      if(timeCompare(time, netTimestamp) >= 0)
      {
         //Get exclusive access
         osAcquireMutex(&netMutex);
         //Handle periodic operations
         netTick();
         //Release exclusive access
         osReleaseMutex(&netMutex);

         //Next event
         netTimestamp = time + NET_TICK_INTERVAL;
      }
#if (NET_RTOS_SUPPORT == ENABLED)
   }
#endif
}
