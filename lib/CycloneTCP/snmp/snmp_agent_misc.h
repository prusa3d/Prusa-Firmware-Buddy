/**
 * @file snmp_agent_misc.h
 * @brief Helper functions for SNMP agent
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

#ifndef _SNMP_AGENT_MISC_H
#define _SNMP_AGENT_MISC_H

//Dependencies
#include "core/net.h"
#include "snmp/snmp_agent.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//SNMP agent related functions
void snmpLockMib(SnmpAgentContext *context);
void snmpUnlockMib(SnmpAgentContext *context);

SnmpUserEntry *snmpCreateCommunityEntry(SnmpAgentContext *context);

SnmpUserEntry *snmpFindCommunityEntry(SnmpAgentContext *context,
   const char_t *community, size_t length);

error_t snmpParseVarBinding(const uint8_t *p,
   size_t length, SnmpVarBind *var, size_t *consumed);

error_t snmpWriteVarBinding(SnmpAgentContext *context, const SnmpVarBind *var);
error_t snmpCopyVarBindingList(SnmpAgentContext *context);

error_t snmpWriteTrapVarBindingList(SnmpAgentContext *context,
   uint_t genericTrapType, uint_t specificTrapCode,
   const SnmpTrapObject *objectList, uint_t objectListSize);

error_t snmpTranslateStatusCode(SnmpMessage *message, error_t status, uint_t index);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
