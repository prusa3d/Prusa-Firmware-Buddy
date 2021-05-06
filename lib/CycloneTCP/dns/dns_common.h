/**
 * @file dns_common.h
 * @brief Common DNS routines
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

#ifndef _DNS_COMMON_H
#define _DNS_COMMON_H

//Dependencies
#include "core/net.h"

//Maximum recursion limit when parsing domain names
#ifndef DNS_NAME_MAX_RECURSION
   #define DNS_NAME_MAX_RECURSION 4
#elif (DNS_NAME_MAX_RECURSION < 1 || DNS_NAME_MAX_RECURSION > 8)
   #error DNS_NAME_MAX_RECURSION parameter is not valid
#endif

//Maximum size of DNS messages
#define DNS_MESSAGE_MAX_SIZE 512
//Maximum size of names
#define DNS_NAME_MAX_SIZE 255
//Maximum size of labels
#define DNS_LABEL_MAX_SIZE 63

//Maximum length of reverse DNS names (IPv4)
#define DNS_MAX_IPV4_REVERSE_NAME_LEN 15
//Maximum length of reverse DNS names (IPv6)
#define DNS_MAX_IPV6_REVERSE_NAME_LEN 63

//DNS port number
#define DNS_PORT 53

//Label compression tag
#define DNS_COMPRESSION_TAG 0xC0

//Macro definition
#define DNS_GET_QUESTION(message, offset) (DnsQuestion *) ((uint8_t *) (message) + (offset))
#define DNS_GET_RESOURCE_RECORD(message, offset) (DnsResourceRecord *) ((uint8_t *) (message) + (offset))

#define DNS_SET_NSEC_BITMAP(bitmap, type) bitmap[(type) / 8] |= 0x80 >> ((type) % 8)
#define DNS_CLR_NSEC_BITMAP(bitmap, type) bitmap[(type) / 8] &= ~(0x80 >> ((type) % 8))

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief DNS opcodes
 **/

typedef enum
{
   DNS_OPCODE_QUERY         = 0,
   DNS_OPCODE_INVERSE_QUERY = 1,
   DNS_OPCODE_STATUS        = 2,
   DNS_OPCODE_NOTIFY        = 4,
   DNS_OPCODE_UPDATE        = 5
} DnsOpcode;


/**
 * @brief DNS return codes
 **/

typedef enum
{
   DNS_RCODE_NO_ERROR        = 0,
   DNS_RCODE_FORMAT_ERROR    = 1,
   DNS_RCODE_SERVER_FAILURE  = 2,
   DNS_RCODE_NAME_ERROR      = 3,
   DNS_RCODE_NOT_IMPLEMENTED = 4,
   DNS_RCODE_QUERY_REFUSED   = 5
} DnsReturnCode;


/**
 * @brief DNS resource record classes
 **/

typedef enum
{
   DNS_RR_CLASS_IN  = 1,  ///<Internet
   DNS_RR_CLASS_CH  = 3,  ///<Chaos
   DNS_RR_CLASS_HS  = 4,  ///<Hesiod
   DNS_RR_CLASS_ANY = 255 ///<Any class
} DnsResourceRecordClass;


/**
 * @brief DNS resource record types
 **/

typedef enum
{
   DNS_RR_TYPE_A     = 1,   ///<Host address
   DNS_RR_TYPE_NS    = 2,   ///<Authoritative name server
   DNS_RR_TYPE_CNAME = 5,   ///<Canonical name for an alias
   DNS_RR_TYPE_SOA   = 6,   ///<Start of a zone of authority
   DNS_RR_TYPE_WKS   = 11,  ///<Well known service description
   DNS_RR_TYPE_PTR   = 12,  ///<Domain name pointer
   DNS_RR_TYPE_HINFO = 13,  ///<Host information
   DNS_RR_TYPE_MINFO = 14,  ///<Mailbox or mail list information
   DNS_RR_TYPE_MX    = 15,  ///<Mail exchange
   DNS_RR_TYPE_TXT   = 16,  ///<Text strings
   DNS_RR_TYPE_AAAA  = 28,  ///<IPv6 address
   DNS_RR_TYPE_NB    = 32,  ///<NetBIOS name service
   DNS_RR_TYPE_SRV   = 33,  ///<Server selection
   DNS_RR_TYPE_NAPTR = 35,  ///<Naming authority pointer
   DNS_RR_TYPE_NSEC  = 47,  ///<NSEC record
   DNS_RR_TYPE_EUI48 = 108, ///<EUI-48 address
   DNS_RR_TYPE_EUI64 = 109, ///<EUI-64 address
   DNS_RR_TYPE_AXFR  = 252, ///<Transfer of an entire zone
   DNS_RR_TYPE_ANY   = 255, ///<A request for all records
   DNS_RR_TYPE_URI   = 256  ///<Uniform resource identifier
} DnsResourceRecordType;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(push, 1)
#endif


/**
 * @brief DNS message header
 **/

typedef __start_packed struct
{
   uint16_t id;         //0-1
#if defined(_CPU_BIG_ENDIAN) && !defined(__IAR_SYSTEMS_ICC__)
   uint8_t qr : 1;      //2
   uint8_t opcode : 4;
   uint8_t aa : 1;
   uint8_t tc : 1;
   uint8_t rd : 1;
   uint8_t ra : 1;      //3
   uint8_t z : 3;
   uint8_t rcode : 4;
#else
   uint8_t rd : 1;      //2
   uint8_t tc : 1;
   uint8_t aa : 1;
   uint8_t opcode : 4;
   uint8_t qr : 1;
   uint8_t rcode : 4;   //3
   uint8_t z : 3;
   uint8_t ra : 1;
#endif
   uint16_t qdcount;    //4-5
   uint16_t ancount;    //6-7
   uint16_t nscount;    //8-9
   uint16_t arcount;    //10-11
   uint8_t questions[]; //12
} __end_packed DnsHeader;


/**
 * @brief Question format
 **/

typedef __start_packed struct
{
   uint16_t qtype;
   uint16_t qclass;
} __end_packed DnsQuestion;


/**
 * @brief Resource record format
 **/

typedef __start_packed struct
{
   uint16_t rtype;    //0-1
   uint16_t rclass;   //2-3
   uint32_t ttl;      //4-7
   uint16_t rdlength; //8-9
   uint8_t rdata[];   //10
} __end_packed DnsResourceRecord;


/**
 * @brief SRV resource record format
 **/

typedef __start_packed struct
{
   uint16_t rtype;    //0-1
   uint16_t rclass;   //2-3
   uint32_t ttl;      //4-7
   uint16_t rdlength; //8-9
   uint16_t priority; //10-11
   uint16_t weight;   //12-13
   uint16_t port;     //14-15
   uint8_t target[];  //16
} __end_packed DnsSrvResourceRecord;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(pop)
#endif


//DNS related functions
size_t dnsEncodeName(const char_t *src, uint8_t *dest);

size_t dnsParseName(const DnsHeader *message,
   size_t length, size_t pos, char_t *dest, uint_t level);

int_t dnsCompareName(const DnsHeader *message, size_t length,
   size_t pos, const char_t *name, uint_t level);

int_t dnsCompareEncodedName(const DnsHeader *message1, size_t length1, size_t pos1,
   const DnsHeader *message2, size_t length2, size_t pos2, uint_t level);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
