/**
 * @file udp_mib_module.h
 * @brief UDP MIB module
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

#ifndef _UDP_MIB_MODULE_H
#define _UDP_MIB_MODULE_H

//Dependencies
#include "mibs/mib_common.h"

//UDP MIB module support
#ifndef UDP_MIB_SUPPORT
   #define UDP_MIB_SUPPORT DISABLED
#elif (UDP_MIB_SUPPORT != ENABLED && UDP_MIB_SUPPORT != DISABLED)
   #error UDP_MIB_SUPPORT parameter is not valid
#endif

//Macro definitions
#if (UDP_MIB_SUPPORT == ENABLED)
   #define UDP_MIB_INC_COUNTER32(name, value) udpMibBase.name += value
   #define UDP_MIB_INC_COUNTER64(name, value) udpMibBase.name += value
#else
   #define UDP_MIB_INC_COUNTER32(name, value)
   #define UDP_MIB_INC_COUNTER64(name, value)
#endif

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief UDP MIB base
 **/

typedef struct
{
   uint32_t udpInDatagrams;
   uint32_t udpNoPorts;
   uint32_t udpInErrors;
   uint32_t udpOutDatagrams;
   uint64_t udpHCInDatagrams;
   uint64_t udpHCOutDatagrams;
} UdpMibBase;


//UDP MIB related constants
extern UdpMibBase udpMibBase;
extern const MibObject udpMibObjects[];
extern const MibModule udpMibModule;

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
