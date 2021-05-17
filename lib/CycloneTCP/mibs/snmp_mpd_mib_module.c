/**
 * @file snmp_mpd_mib_module.c
 * @brief SNMP MPD MIB module
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
 * The SNMP-MPD-MIB defines managed objects for SNMP message processing and
 * dispatching. Refer to RFC 3412 for more details
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

//Switch to the appropriate trace level
#define TRACE_LEVEL SNMP_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "mibs/mib_common.h"
#include "mibs/snmp_mpd_mib_module.h"
#include "mibs/snmp_mpd_mib_impl.h"
#include "core/crypto.h"
#include "encoding/asn1.h"
#include "encoding/oid.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (SNMP_MPD_MIB_SUPPORT == ENABLED)


/**
 * @brief SNMP MPD MIB base
 **/

SnmpMpdMibBase snmpMpdMibBase;


/**
 * @brief SNMP MPD MIB objects
 **/

const MibObject snmpMpdMibObjects[] =
{
   //snmpUnknownSecurityModels object (1.3.6.1.6.3.11.2.1.1)
   {
      "snmpUnknownSecurityModels",
      {43, 6, 1, 6, 3, 11, 2, 1, 1},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &snmpMpdMibBase.snmpUnknownSecurityModels,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //snmpInvalidMsgs object (1.3.6.1.6.3.11.2.1.2)
   {
      "snmpInvalidMsgs",
      {43, 6, 1, 6, 3, 11, 2, 1, 2},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &snmpMpdMibBase.snmpInvalidMsgs,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   },
   //snmpUnknownPDUHandlers object (1.3.6.1.6.3.11.2.1.3)
   {
      "snmpUnknownPDUHandlers",
      {43, 6, 1, 6, 3, 11, 2, 1, 3},
      9,
      ASN1_CLASS_APPLICATION,
      MIB_TYPE_COUNTER32,
      MIB_ACCESS_READ_ONLY,
      &snmpMpdMibBase.snmpUnknownPDUHandlers,
      NULL,
      sizeof(uint32_t),
      NULL,
      NULL,
      NULL
   }
};


/**
 * @brief SNMP MPD MIB module
 **/

const MibModule snmpMpdMibModule =
{
   "SNMP-MPD-MIB",
   {43, 6, 1, 6, 3, 11},
   6,
   snmpMpdMibObjects,
   arraysize(snmpMpdMibObjects),
   snmpMpdMibInit,
   snmpMpdMibLoad,
   snmpMpdMibUnload,
   snmpMpdMibLock,
   snmpMpdMibUnlock
};

#endif
