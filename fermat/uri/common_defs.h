// Copyright (C) 2026 Kumo inc. and its affiliates. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

/////////////////////////////////////////////////////////////////////////////////////
/// @file common_defs.h
/// @brief Common definitions for cross-platform compiler support.

#pragma once

#include <turbo/base/macros.h>
#include <turbo/log/logging.h>

#ifdef _MSC_VER
#define ADA_VISUAL_STUDIO 1
/**
 * We want to differentiate carefully between
 * clang under visual studio and regular visual
 * studio.
 */
#ifdef __clang__
// clang under visual studio
#define ADA_CLANG_VISUAL_STUDIO 1
#else
// just regular visual studio (best guess)
#define ADA_REGULAR_VISUAL_STUDIO 1
#endif  // __clang__
#endif  // _MSC_VER

#if defined(__GNUC__)
// Marks a block with a name so that MCA analysis can see it.
#define ADA_BEGIN_DEBUG_BLOCK(name) __asm volatile("# LLVM-MCA-BEGIN " #name);
#define ADA_END_DEBUG_BLOCK(name) __asm volatile("# LLVM-MCA-END " #name);
#define ADA_DEBUG_BLOCK(name, block) \
  BEGIN_DEBUG_BLOCK(name);           \
  block;                             \
  END_DEBUG_BLOCK(name);
#else
#define ADA_BEGIN_DEBUG_BLOCK(name)
#define ADA_END_DEBUG_BLOCK(name)
#define ADA_DEBUG_BLOCK(name, block)
#endif

// Align to N-byte boundary
#define ADA_ROUNDUP_N(a, n) (((a) + ((n)-1)) & ~((n)-1))
#define ADA_ROUNDDOWN_N(a, n) ((a) & ~((n)-1))

#define ADA_ISALIGNED_N(ptr, n) (((uintptr_t)(ptr) & ((n)-1)) == 0)

#if defined(ADA_REGULAR_VISUAL_STUDIO)

#define ADA_PUSH_DISABLE_WARNINGS __pragma(warning(push))
#define ADA_PUSH_DISABLE_ALL_WARNINGS __pragma(warning(push, 0))
#define ADA_DISABLE_VS_WARNING(WARNING_NUMBER) \
  __pragma(warning(disable : WARNING_NUMBER))
// Get rid of Intellisense-only warnings (Code Analysis)
// Though __has_include is C++17, it is supported in Visual Studio 2017 or
// better (_MSC_VER>=1910).
#ifdef __has_include
#if __has_include(<CppCoreCheck\Warnings.h>)
#include <CppCoreCheck\Warnings.h>
#define ADA_DISABLE_UNDESIRED_WARNINGS \
  ADA_DISABLE_VS_WARNING(ALL_CPPCORECHECK_WARNINGS)
#endif
#endif

#ifndef ADA_DISABLE_UNDESIRED_WARNINGS
#define ADA_DISABLE_UNDESIRED_WARNINGS
#endif

#define ADA_DISABLE_DEPRECATED_WARNING ADA_DISABLE_VS_WARNING(4996)
#define ADA_DISABLE_STRICT_OVERFLOW_WARNING
#define ADA_POP_DISABLE_WARNINGS __pragma(warning(pop))

#else  // ADA_REGULAR_VISUAL_STUDIO

#define ADA_PUSH_DISABLE_WARNINGS _Pragma("GCC diagnostic push")
// gcc doesn't seem to disable all warnings with all and extra, add warnings
// here as necessary
#define ADA_PUSH_DISABLE_ALL_WARNINGS               \
  ADA_PUSH_DISABLE_WARNINGS                         \
  ADA_DISABLE_GCC_WARNING("-Weffc++")               \
  ADA_DISABLE_GCC_WARNING("-Wall")                  \
  ADA_DISABLE_GCC_WARNING("-Wconversion")           \
  ADA_DISABLE_GCC_WARNING("-Wextra")                \
  ADA_DISABLE_GCC_WARNING("-Wattributes")           \
  ADA_DISABLE_GCC_WARNING("-Wimplicit-fallthrough") \
  ADA_DISABLE_GCC_WARNING("-Wnon-virtual-dtor")     \
  ADA_DISABLE_GCC_WARNING("-Wreturn-type")          \
  ADA_DISABLE_GCC_WARNING("-Wshadow")               \
  ADA_DISABLE_GCC_WARNING("-Wunused-parameter")     \
  ADA_DISABLE_GCC_WARNING("-Wunused-variable")
#define ADA_PRAGMA(P) _Pragma(#P)
#define ADA_DISABLE_GCC_WARNING(WARNING) \
  ADA_PRAGMA(GCC diagnostic ignored WARNING)
#if defined(ADA_CLANG_VISUAL_STUDIO)
#define ADA_DISABLE_UNDESIRED_WARNINGS \
  ADA_DISABLE_GCC_WARNING("-Wmicrosoft-include")
#else
#define ADA_DISABLE_UNDESIRED_WARNINGS
#endif
#define ADA_DISABLE_DEPRECATED_WARNING \
  ADA_DISABLE_GCC_WARNING("-Wdeprecated-declarations")
#define ADA_DISABLE_STRICT_OVERFLOW_WARNING \
  ADA_DISABLE_GCC_WARNING("-Wstrict-overflow")
#define ADA_POP_DISABLE_WARNINGS _Pragma("GCC diagnostic pop")

#endif  // MSC_VER

// __has_cpp_attribute is part of C++20
#if !defined(__has_cpp_attribute)
#define __has_cpp_attribute(x) 0
#endif

#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__)
#define ADA_IS_BIG_ENDIAN (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#elif defined(_WIN32)
#define ADA_IS_BIG_ENDIAN 0
#else
#if defined(__APPLE__) || \
    defined(__FreeBSD__)  // defined __BYTE_ORDER__ && defined
// __ORDER_BIG_ENDIAN__
#include <machine/endian.h>
#elif defined(sun) || \
    defined(__sun)  // defined(__APPLE__) || defined(__FreeBSD__)
#include <sys/byteorder.h>
#else  // defined(__APPLE__) || defined(__FreeBSD__)

#ifdef __has_include
#if __has_include(<endian.h>)
#include <endian.h>
#endif  //__has_include(<endian.h>)
#endif  //__has_include

#endif  // defined(__APPLE__) || defined(__FreeBSD__)

#ifndef !defined(__BYTE_ORDER__) || !defined(__ORDER_LITTLE_ENDIAN__)
#define ADA_IS_BIG_ENDIAN 0
#endif

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define ADA_IS_BIG_ENDIAN 0
#else  // __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define ADA_IS_BIG_ENDIAN 1
#endif  // __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

#endif  // defined __BYTE_ORDER__ && defined __ORDER_BIG_ENDIAN__

// Unless the programmer has already set ADA_DEVELOPMENT_CHECKS,
// we want to set it under debug builds. We detect a debug build
// under Visual Studio when the _DEBUG macro is set. Under the other
// compilers, we use the fact that they define __OPTIMIZE__ whenever
// they allow optimizations.
// It is possible that this could miss some cases where ADA_DEVELOPMENT_CHECKS
// is helpful, but the programmer can set the macro ADA_DEVELOPMENT_CHECKS.
// It could also wrongly set ADA_DEVELOPMENT_CHECKS (e.g., if the programmer
// sets _DEBUG in a release build under Visual Studio, or if some compiler fails
// to set the __OPTIMIZE__ macro).
#if !defined(ADA_DEVELOPMENT_CHECKS) && !defined(NDEBUG)
#ifdef _MSC_VER
// Visual Studio seems to set _DEBUG for debug builds.
#ifdef _DEBUG
#define ADA_DEVELOPMENT_CHECKS 1
#endif  // _DEBUG
#else   // _MSC_VER
// All other compilers appear to set __OPTIMIZE__ to a positive integer
// when the compiler is optimizing.
#ifndef __OPTIMIZE__
#define ADA_DEVELOPMENT_CHECKS 1
#endif  // __OPTIMIZE__
#endif  // _MSC_VER
#endif  // ADA_DEVELOPMENT_CHECKS

#if defined(__SSE2__) || defined(__x86_64__) || defined(__x86_64) || \
    (defined(_M_AMD64) || defined(_M_X64) ||                         \
     (defined(_M_IX86_FP) && _M_IX86_FP == 2))
#define ADA_SSE2 1
#endif

#if defined(__aarch64__) || defined(_M_ARM64)
#define ADA_NEON 1
#endif
