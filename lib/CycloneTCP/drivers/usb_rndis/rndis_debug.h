/**
 * @file rndis_debug.h
 * @brief RNDIS (Remote Network Driver Interface Specification)
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

#ifndef _RNDIS_DEBUG_H
#define _RNDIS_DEBUG_H

//Dependencies
#include "rndis.h"


/**
 * @brief Value/name pair
 **/

typedef struct
{
   uint32_t value;
   const char_t *name;
} RndisValueName;


//RNDIS related functions
error_t rndisDumpMsg(const RndisMsg *message, size_t length);
error_t rndisDumpPacketMsg(const RndisPacketMsg *message, size_t length);
error_t rndisDumpInitializeMsg(const RndisInitializeMsg *message, size_t length);
error_t rndisDumpHaltMsg(const RndisHaltMsg *message, size_t length);
error_t rndisDumpQueryMsg(const RndisQueryMsg *message, size_t length);
error_t rndisDumpSetMsg(const RndisSetMsg *message, size_t length);
error_t rndisDumpResetMsg(const RndisResetMsg *message, size_t length);
error_t rndisDumpIndicateStatusMsg(const RndisIndicateStatusMsg *message, size_t length);
error_t rndisDumpKeepAliveMsg(const RndisKeepAliveMsg *message, size_t length);

error_t rndisDumpInitializeCmplt(const RndisInitializeCmplt *message, size_t length);
error_t rndisDumpQueryCmplt(const RndisQueryCmplt *message, size_t length);
error_t rndisDumpSetCmplt(const RndisSetCmplt *message, size_t length);
error_t rndisDumpResetCmplt(const RndisResetCmplt *message, size_t length);
error_t rndisDumpKeepAliveCmplt(const RndisKeepAliveCmplt *message, size_t length);

const char_t *rndisFindName(uint32_t value, const RndisValueName *table, size_t size);

#endif
