/**
 * @file snmp_agent_trap.h
 * @brief SNMP trap notifications
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

#ifndef _SNMP_AGENT_TRAP_H
#define _SNMP_AGENT_TRAP_H

//Dependencies
#include "core/net.h"
#include "snmp/snmp_agent.h"

//Trap notification support
#ifndef SNMP_AGENT_TRAP_SUPPORT
   #define SNMP_AGENT_TRAP_SUPPORT ENABLED
#elif (SNMP_AGENT_TRAP_SUPPORT != ENABLED && SNMP_AGENT_TRAP_SUPPORT != DISABLED)
   #error SNMP_AGENT_TRAP_SUPPORT parameter is not valid
#endif

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Object descriptor for trap notifications
 **/

typedef struct
{
   uint8_t oid[SNMP_MAX_OID_SIZE];
   size_t oidLen;
} SnmpTrapObject;


//SNMP trap related functions
error_t snmpFormatTrapMessage(SnmpAgentContext *context, SnmpVersion version,
   const char_t *userName, uint_t genericTrapType, uint_t specificTrapCode,
   const SnmpTrapObject *objectList, uint_t objectListSize);

error_t snmpFormatTrapPdu(SnmpAgentContext *context, SnmpVersion version,
   const char_t *userName, uint_t genericTrapType, uint_t specificTrapCode,
   const SnmpTrapObject *objectList, uint_t objectListSize);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
