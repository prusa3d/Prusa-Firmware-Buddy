/**
 * @file snmp_mpd_mib_impl.c
 * @brief SNMP MPD MIB module implementation
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
 * @brief SNMP MPD MIB module initialization
 * @return Error code
 **/

error_t snmpMpdMibInit(void)
{
   //Debug message
   TRACE_INFO("Initializing SNMP MPD MIB base...\r\n");

   //Clear SNMP MPD MIB base
   osMemset(&snmpMpdMibBase, 0, sizeof(snmpMpdMibBase));

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Load SNMP MPD MIB module
 * @param[in] context Pointer to the SNMP agent context
 * @return Error code
 **/

error_t snmpMpdMibLoad(void *context)
{
   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Unload SNMP MPD MIB module
 * @param[in] context Pointer to the SNMP agent context
 **/

void snmpMpdMibUnload(void *context)
{
}


/**
 * @brief Lock SNMP MPD MIB base
 **/

void snmpMpdMibLock(void)
{
}


/**
 * @brief Unlock SNMP MPD MIB base
 **/

void snmpMpdMibUnlock(void)
{
}

#endif
