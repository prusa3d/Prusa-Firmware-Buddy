/**
 * @file snmp_mib_impl.h
 * @brief SNMP MIB module implementation
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

#ifndef _SNMP_MIB_IMPL_H
#define _SNMP_MIB_IMPL_H

//Dependencies
#include "mibs/mib_common.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//SNMP MIB related functions
error_t snmpMibInit(void);
error_t snmpMibLoad(void *context);
void snmpMibUnload(void *context);
void snmpMibLock(void);
void snmpMibUnlock(void);

error_t snmpMibGetSysDescr(const MibObject *object, const uint8_t *oid,
   size_t oidLen, MibVariant *value, size_t *valueLen);

error_t snmpMibGetSysObjectID(const MibObject *object, const uint8_t *oid,
   size_t oidLen, MibVariant *value, size_t *valueLen);

error_t snmpMibGetSysUpTime(const MibObject *object, const uint8_t *oid,
   size_t oidLen, MibVariant *value, size_t *valueLen);

error_t snmpMibSetSysContact(const MibObject *object, const uint8_t *oid,
   size_t oidLen, const MibVariant *value, size_t valueLen, bool_t commit);

error_t snmpMibGetSysContact(const MibObject *object, const uint8_t *oid,
   size_t oidLen, MibVariant *value, size_t *valueLen);

error_t snmpMibSetSysName(const MibObject *object, const uint8_t *oid,
   size_t oidLen, const MibVariant *value, size_t valueLen, bool_t commit);

error_t snmpMibGetSysName(const MibObject *object, const uint8_t *oid,
   size_t oidLen, MibVariant *value, size_t *valueLen);

error_t snmpMibSetSysLocation(const MibObject *object, const uint8_t *oid,
   size_t oidLen, const MibVariant *value, size_t valueLen, bool_t commit);

error_t snmpMibGetSysLocation(const MibObject *object, const uint8_t *oid,
   size_t oidLen, MibVariant *value, size_t *valueLen);

error_t snmpMibGetSysOREntry(const MibObject *object, const uint8_t *oid,
   size_t oidLen, MibVariant *value, size_t *valueLen);

error_t snmpMibGetNextSysOREntry(const MibObject *object, const uint8_t *oid,
   size_t oidLen, uint8_t *nextOid, size_t *nextOidLen);

error_t snmpv2MibGetSnmpTrapOID(const MibObject *object, const uint8_t *oid,
   size_t oidLen, MibVariant *value, size_t *valueLen);

error_t snmpv2MibGetSnmpTrapEnterprise(const MibObject *object, const uint8_t *oid,
   size_t oidLen, MibVariant *value, size_t *valueLen);

error_t snmpMibSetSnmpSetSerialNo(const MibObject *object, const uint8_t *oid,
   size_t oidLen, const MibVariant *value, size_t valueLen, bool_t commit);

error_t snmpMibGetSnmpSetSerialNo(const MibObject *object, const uint8_t *oid,
   size_t oidLen, MibVariant *value, size_t *valueLen);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
