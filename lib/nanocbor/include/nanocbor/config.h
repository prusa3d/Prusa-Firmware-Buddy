/*
 * SPDX-License-Identifier: CC0-1.0
 */

/**
 * @defgroup    nanocbor_config NanoCBOR configuration header
 * @brief       Provides compile-time configuration for nanocbor
 *
 * @{
 *
 * @file
 *
 * @author      Koen Zandberg <koen@bergzand.net>
 */

#ifndef NANOCBOR_CONFIG_H
#define NANOCBOR_CONFIG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Recursion limit when using @ref nanocbor_skip.
 */
#ifndef NANOCBOR_RECURSION_MAX
#  define NANOCBOR_RECURSION_MAX 10
#endif

/**
 * @brief configuration for size_t SIZE_MAX equivalent
 */
#ifndef NANOCBOR_SIZE_SIZET
#  if (SIZE_MAX == UINT16_MAX)
#    define NANOCBOR_SIZE_SIZET NANOCBOR_SIZE_SHORT
#  elif (SIZE_MAX == UINT32_MAX)
#    define NANOCBOR_SIZE_SIZET NANOCBOR_SIZE_WORD
#  elif (SIZE_MAX == UINT64_MAX)
#    define NANOCBOR_SIZE_SIZET NANOCBOR_SIZE_LONG
#  else
#    error "ERROR: unable to determine maximum size of size_t"
#  endif
#endif

#ifdef __cplusplus
}
#endif

#endif /* NANOCBOR_CONFIG_H */
/** @} */
