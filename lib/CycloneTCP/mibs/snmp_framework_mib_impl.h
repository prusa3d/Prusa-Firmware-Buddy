/**
 * @file snmp_framework_mib_impl.h
 * @brief SNMP FRAMEWORK MIB module implementation
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

#ifndef _SNMP_FRAMEWORK_MIB_IMPL_H
#define _SNMP_FRAMEWORK_MIB_IMPL_H

//Dependencies
#include "mibs/mib_common.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//SNMP FRAMEWORK MIB related functions
error_t snmpFrameworkMibInit(void);
error_t snmpFrameworkMibLoad(void *context);
void snmpFrameworkMibUnload(void *context);
void snmpFrameworkMibLock(void);
void snmpFrameworkMibUnlock(void);

error_t snmpFrameworkMibGetSnmpEngineID(const MibObject *object,
   const uint8_t *oid, size_t oidLen, MibVariant *value, size_t *valueLen);

error_t snmpFrameworkMibGetSnmpEngineBoots(const MibObject *object,
   const uint8_t *oid, size_t oidLen, MibVariant *value, size_t *valueLen);

error_t snmpFrameworkMibGetSnmpEngineTime(const MibObject *object,
   const uint8_t *oid, size_t oidLen, MibVariant *value, size_t *valueLen);

error_t snmpFrameworkMibGetSnmpEngineMaxMessageSize(const MibObject *object,
   const uint8_t *oid, size_t oidLen, MibVariant *value, size_t *valueLen);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
