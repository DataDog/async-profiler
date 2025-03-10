/*
 * Copyright The async-profiler authors
 * SPDX-License-Identifier: Apache-2.0
 */

package one.profiler.test;

public enum Arch {
    X64,
    X86,
    ARM64,
    ARM32,
    PPC64LE,
    RISCV64,
    LOONGARCH64;

    public static Arch current() {
        String arch = System.getProperty("os.arch");
        if (arch.contains("x86_64") || arch.contains("amd64")) {
            return Arch.X64;
        } else if (arch.contains("aarch64")) {
            return Arch.ARM64;
        } else if (arch.contains("arm")) {
            return Arch.ARM32;
        } else if (arch.contains("ppc64le")) {
            return Arch.PPC64LE;
        } else if (arch.contains("riscv64")) {
            return Arch.RISCV64;
        } else if (arch.contains("loongarch64")) {
            return Arch.LOONGARCH64;
        } else if (arch.endsWith("86")) {
            return Arch.X86;
        }
        throw new IllegalStateException("Unknown CPU architecture");
    }
}
