#pragma once

#ifndef CONSTINIT
#ifndef __cpp_constinit
#if defined(__clang__)
#define CONSTINIT [[clang::require_constant_initialization]]
#else
#define CONSTINIT
#endif
#else
#define CONSTINIT constinit
#endif
#endif

#ifndef CONSTEVAL
#ifndef __cpp_consteval
#define CONSTEVAL constexpr
#else
#define CONSTEVAL consteval
#endif
#endif

#ifndef ALWAYS_INLINE
#define ALWAYS_INLINE inline __attribute__((always_inline))
#endif

#ifndef NEVER_INLINE
#define NEVER_INLINE __attribute__((noinline))
#endif

#ifndef MUSTTAIL
#if defined(__clang__)
#define MUSTTAIL [[clang::musttail]]
#else
#define MUSTTAIL
#endif
#endif
