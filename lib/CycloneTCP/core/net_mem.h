/**
 * @file net_mem.h
 * @brief Memory management
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

#ifndef _NET_MEM_H
#define _NET_MEM_H

//Dependencies
#include "net_config.h"
#include "os_port.h"
#include "error.h"

//Use fixed-size blocks allocation?
#ifndef NET_MEM_POOL_SUPPORT
   #define NET_MEM_POOL_SUPPORT DISABLED
#elif (NET_MEM_POOL_SUPPORT != ENABLED && NET_MEM_POOL_SUPPORT != DISABLED)
   #error NET_MEM_POOL_SUPPORT parameter is not valid
#endif

//Number of buffers available
#ifndef NET_MEM_POOL_BUFFER_COUNT
   #define NET_MEM_POOL_BUFFER_COUNT 32
#elif (NET_MEM_POOL_BUFFER_COUNT < 1)
   #error NET_MEM_POOL_BUFFER_COUNT parameter is not valid
#endif

//Size of the buffers
#ifndef NET_MEM_POOL_BUFFER_SIZE
   #define NET_MEM_POOL_BUFFER_SIZE 1536
#elif (NET_MEM_POOL_BUFFER_SIZE < 128)
   #error NET_MEM_POOL_BUFFER_SIZE parameter is not valid
#endif

//Size of the header part of the buffer
#define CHUNKED_BUFFER_HEADER_SIZE (sizeof(NetBuffer) + MAX_CHUNK_COUNT * sizeof(ChunkDesc))

//Helper macro for defining a buffer
#define N(size) (((size) + NET_MEM_POOL_BUFFER_SIZE - 1) / NET_MEM_POOL_BUFFER_SIZE)

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Structure describing a chunk of data
 **/

typedef struct
{
   void *address;
   uint16_t length;
   uint16_t size;
} ChunkDesc;


/**
 * @brief Structure describing a buffer that spans multiple chunks
 **/

typedef struct
{
   uint_t chunkCount;
   uint_t maxChunkCount;
   ChunkDesc chunk[];
} NetBuffer;


typedef struct
{
   uint_t chunkCount;
   uint_t maxChunkCount;
   ChunkDesc chunk[1];
} NetBuffer1;


//Memory management functions
error_t memPoolInit(void);
void *memPoolAlloc(size_t size);
void memPoolFree(void *p);
void memPoolGetStats(uint_t *currentUsage, uint_t *maxUsage, uint_t *size);

NetBuffer *netBufferAlloc(size_t length);
void netBufferFree(NetBuffer *buffer);

size_t netBufferGetLength(const NetBuffer *buffer);
error_t netBufferSetLength(NetBuffer *buffer, size_t length);

void *netBufferAt(const NetBuffer *buffer, size_t offset);

error_t netBufferConcat(NetBuffer *dest,
   const NetBuffer *src, size_t srcOffset, size_t length);

error_t netBufferCopy(NetBuffer *dest, size_t destOffset,
   const NetBuffer *src, size_t srcOffset, size_t length);

error_t netBufferAppend(NetBuffer *dest, const void *src, size_t length);

size_t netBufferWrite(NetBuffer *dest,
   size_t destOffset, const void *src, size_t length);

size_t netBufferRead(void *dest, const NetBuffer *src,
   size_t srcOffset, size_t length);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
