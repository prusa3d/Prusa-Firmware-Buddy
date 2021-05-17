/**
 * @file snmp_mpd_mib_module.h
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
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

#ifndef _SNMP_MPD_MIB_MODULE_H
#define _SNMP_MPD_MIB_MODULE_H

//Dependencies
#include "mibs/mib_common.h"

//SNMP MPD MIB module support
#ifndef SNMP_MPD_MIB_SUPPORT
   #define SNMP_MPD_MIB_SUPPORT DISABLED
#elif (SNMP_MPD_MIB_SUPPORT != ENABLED && SNMP_MPD_MIB_SUPPORT != DISABLED)
   #error SNMP_MPD_MIB_SUPPORT parameter is not valid
#endif

//Macro definitions
#if (SNMP_MPD_MIB_SUPPORT == ENABLED)
   #define SNMP_MPD_MIB_INC_COUNTER32(name, value) snmpMpdMibBase.name += value
#else
   #define SNMP_MPD_MIB_INC_COUNTER32(name, value)
#endif

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief SNMP MPD MIB base
 **/

typedef struct
{
   uint32_t snmpUnknownSecurityModels;
   uint32_t snmpInvalidMsgs;
   uint32_t snmpUnknownPDUHandlers;
} SnmpMpdMibBase;


//SNMP MPD MIB related constants
extern SnmpMpdMibBase snmpMpdMibBase;
extern const MibObject snmpMpdMibObjects[];
extern const MibModule snmpMpdMibModule;

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
