/*
 * Copyright The async-profiler authors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STACKFRAME_H
#define _STACKFRAME_H

#include <stddef.h>
#include <stdint.h>
#include <ucontext.h>
#include "arch.h"


class NMethod;

class StackFrame {
  private:
    ucontext_t* _ucontext;

    static bool withinCurrentStack(uintptr_t address) {
        // Check that the address is not too far from the stack pointer of current context
        void* real_sp;
        return address - (uintptr_t)&real_sp <= 0xffff;
    }

  public:
    StackFrame(void* ucontext) {
        _ucontext = (ucontext_t*)ucontext;
    }

    void restore(uintptr_t saved_pc, uintptr_t saved_sp, uintptr_t saved_fp) {
        if (_ucontext != NULL) {
            pc() = saved_pc;
            sp() = saved_sp;
            fp() = saved_fp;
        }
    }

    uintptr_t stackAt(int slot) {
        return ((uintptr_t*)sp())[slot];
    }

    uintptr_t& pc();
    uintptr_t& sp();
    uintptr_t& fp();

    uintptr_t& retval();
    uintptr_t link();
    uintptr_t arg0();
    uintptr_t arg1();
    uintptr_t arg2();
    uintptr_t arg3();
    uintptr_t jarg0();
    uintptr_t method();
    uintptr_t senderSP();

    void ret();

    bool unwindStub(instruction_t* entry, const char* name) {
        return unwindStub(entry, name, pc(), sp(), fp());
    }

    bool unwindCompiled(NMethod* nm) {
        return unwindCompiled(nm, pc(), sp(), fp());
    }

    bool unwindStub(instruction_t* entry, const char* name, uintptr_t& pc, uintptr_t& sp, uintptr_t& fp);
    bool unwindCompiled(NMethod* nm, uintptr_t& pc, uintptr_t& sp, uintptr_t& fp);
    bool unwindAtomicStub(const void*& pc);

    /* ===========================================================================
     *  ONE-STEP **LEAF-FRAME** UNWIND  —  no prologue, no frame-pointer
     *
     *  Applies only when we interrupt inside a *true* leaf (PLT veneer, 1-inst
     *  syscall stub, musl clone entry, etc.).  The callee has **not** saved
     *  FP/LR to memory, so the *only* recoverable information is the caller’s
     *  return address that the hardware or ABI guarantees to exist.
     *
     *  ┌─ prerequisites ─────────────────────────────────────────────────────────
     *  │ •  sp   still points to the caller’s stack frame
     *  │ •  regs = live register set from ucontext  (if the ABI keeps LR in reg)
     *  │ •  sanity check **AFTER** each step:
     *  │       – pc inside executable   – sp stays inside thread stack
     *  │       – sp keeps natural alignment (4- or 8-byte)
     *  └─────────────────────────────────────────────────────────────────────────
     *
     *  x86-32  SysV
     *      pc = ((void**)sp)[0];   // return RIP pushed by CALL
     *      sp += 4;
     *      fp = *(uint32_t*)sp;    // may be 0
     *
     *  x86-64  SysV
     *      pc = ((void**)sp)[0];   // return RIP pushed by CALL
     *      sp += 8;
     *      fp = *(uint64_t*)sp;    // may be 0
     *
     *  AArch64  (AAPCS64, musl & glibc)
     *      pc = regs->regs[30];    // LR (x30) – ONLY place it lives
     *      // sp & fp (x29) unchanged; fp may be 0
     *
     *  armv7 (EABI)
     *      pc = regs->arm_lr;      // r14
     *      // sp & fp (r11) unchanged
     *
     *  RISC-V  RV64   (LoongArch64 identical)
     *      pc = regs->ra;          // x1
     *      // sp unchanged; fp (s0) unchanged
     *
     *  PPC64  ELFv2
     *      // Leaf functions never build the 32-byte save area and never push LR
     *      // → LR exists only in r0 while we execute.  A memory-only unwinder
     *      // cannot advance → record one native leaf (“ppc64_leaf”) and stop.
     *
     * ========================================================================= */
    bool unwindFramelessLeaf(const void*& pc, uintptr_t& sp, uintptr_t& fp);

    void adjustSP(const void* entry, const void* pc, uintptr_t& sp);

    bool skipFaultInstruction();

    bool checkInterruptedSyscall();

    // Check if PC points to a syscall instruction
    static bool isSyscall(instruction_t* pc);
};

#endif // _STACKFRAME_H
