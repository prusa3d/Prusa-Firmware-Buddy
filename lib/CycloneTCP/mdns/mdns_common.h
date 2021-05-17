/**
 * @file mdns_common.h
 * @brief Definitions common to mDNS client and mDNS responder
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

#ifndef _MDNS_COMMON_H
#define _MDNS_COMMON_H

//Dependencies
#include "core/net.h"
#include "dns/dns_common.h"

//Maximum size of DNS messages
#ifndef MDNS_MESSAGE_MAX_SIZE
   #define MDNS_MESSAGE_MAX_SIZE 1024
#elif (MDNS_MESSAGE_MAX_SIZE < 1)
   #error MDNS_MESSAGE_MAX_SIZE parameter is not valid
#endif

//Default resource record TTL (cache lifetime)
#ifndef MDNS_DEFAULT_RR_TTL
   #define MDNS_DEFAULT_RR_TTL 120
#elif (MDNS_DEFAULT_RR_TTL < 1)
   #error MDNS_DEFAULT_RR_TTL parameter is not valid
#endif

//mDNS port number
#define MDNS_PORT 5353
//Default IP TTL value
#define MDNS_DEFAULT_IP_TTL 255
//Maximum RR TTL in legacy unicast responses
#define MDNS_LEGACY_UNICAST_RR_TTL 10

//QU flag
#define MDNS_QCLASS_QU 0x8000
//Cache Flush flag
#define MDNS_RCLASS_CACHE_FLUSH 0x8000

//mDNS IPv4 multicast group
#define MDNS_IPV4_MULTICAST_ADDR IPV4_ADDR(224, 0, 0, 251)

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief mDNS message
 **/

typedef struct
{
   NetBuffer *buffer;
   size_t offset;
   size_t length;
   const IpPseudoHeader *pseudoHeader;
   const UdpHeader *udpHeader;
   DnsHeader *dnsHeader;
   systime_t timestamp;
   systime_t timeout;
   uint_t sharedRecordCount;
} MdnsMessage;


//mDNS IPv6 multicast group
extern const Ipv6Addr MDNS_IPV6_MULTICAST_ADDR;

//mDNS related functions
error_t mdnsInit(NetInterface *interface);

void mdnsProcessMessage(NetInterface *interface,
   const IpPseudoHeader *pseudoHeader, const UdpHeader *udpHeader,
   const NetBuffer *buffer, size_t offset, const NetRxAncillary *ancillary,
   void *param);

void mdnsProcessResponse(NetInterface *interface, MdnsMessage *response);

bool_t mdnsCheckSourceAddr(NetInterface *interface,
   const IpPseudoHeader *pseudoHeader);

error_t mdnsCreateMessage(MdnsMessage *message, bool_t queryResponse);
void mdnsDeleteMessage(MdnsMessage *message);

error_t mdnsSendMessage(NetInterface *interface, const MdnsMessage *message,
   const IpAddr *destIpAddr, uint_t destPort);

size_t mdnsEncodeName(const char_t *instance, const char_t *service,
   const char_t *domain, uint8_t *dest);

int_t mdnsCompareName(const DnsHeader *message, size_t length, size_t pos,
   const char_t *instance, const char_t *service, const char_t *domain, uint_t level);

int_t mdnsCompareRecord(const MdnsMessage *message1, size_t offset1,
   const DnsResourceRecord *record1, const MdnsMessage *message2,
   size_t offset2, const DnsResourceRecord *record2);

bool_t mdnsCheckDuplicateRecord(const MdnsMessage *message, const char_t *instance,
   const char_t *service, const char_t *domain, uint16_t rtype);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
