/*
 * Copyright The async-profiler authors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SAFEACCESS_H
#define _SAFEACCESS_H

#include <stdint.h>
#include "arch.h"

class StackFrame;

class SafeAccess {
  public:
    static void* load(void** ptr, void* default_value = nullptr);
    static u32 load32(u32* ptr, u32 default_value = 0);

    static bool checkFault(StackFrame& frame);
};

#endif // _SAFEACCESS_H
