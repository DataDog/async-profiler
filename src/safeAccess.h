/*
 * Copyright The async-profiler authors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SAFEACCESS_H
#define _SAFEACCESS_H

#include <stdint.h>

#ifdef __clang__
#  define NOINLINE __attribute__((noinline))
#else
#  define NOINLINE __attribute__((noinline,noclone))
#endif

class StackFrame;

class SafeAccess {
  public:
    NOINLINE __attribute__((aligned(16)))
    static void* load(void** ptr, void* default_value = nullptr);

    NOINLINE __attribute__((aligned(16)))
    static int32_t load32(int32_t* ptr, int32_t default_value = 0);

    NOINLINE __attribute__((aligned(16)))
    static int loadInt(int* ptr, int default_value = 0);

    static bool checkFault(StackFrame& frame);
};

#endif // _SAFEACCESS_H
