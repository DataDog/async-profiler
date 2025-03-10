/*
 * Copyright The async-profiler authors
 * SPDX-License-Identifier: Apache-2.0
 */

package one.profiler.test;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Arrays;

public enum Os {
    LINUX(checkMusl()),
    MACOS(false),
    WINDOWS(false);

    private boolean musl;
    Os(boolean isMusl) {
        this.musl = isMusl;
    }

    public static Os current() {
        String osName = System.getProperty("os.name").toLowerCase();
        if (osName.contains("linux")) {
            return LINUX;
        } else if (osName.contains("mac")) {
            return MACOS;
        } else if (osName.contains("windows")) {
            return WINDOWS;
        }
        throw new IllegalStateException("Unknown OS type: " + osName);
    }

    public boolean isMusl() {
        return musl;
    }

    public String getLibExt() {
        switch (this) {
            case LINUX:
                return "so";
            case MACOS:
                return "dylib";
            case WINDOWS:
                return "dll";
            default:
                throw new AssertionError();
        }
    }
    private static boolean checkMusl() {
        // check the Java executable first, then fall back to /proc/self/maps
        try {
            return isMuslJavaExecutable();
        } catch (IOException ignore) {
            try {
                return isMuslProcSelfMaps();
            } catch (IOException ignore1) {
            }
        }
        return false;
    }

    private static boolean isMuslProcSelfMaps() throws IOException {
        try (BufferedReader reader = new BufferedReader(new FileReader("/proc/self/maps"))) {
            String line;
            while ((line = reader.readLine()) != null) {
                if (line.contains("-musl-")) {
                    return true;
                }
                if (line.contains("/libc.")) {
                    return false;
                }
            }
        }
        return false;
    }

    /**
     * There is information about the linking in the ELF file. Since properly parsing ELF is not
     * trivial this code will attempt a brute-force approach and will scan the first 4096 bytes
     * of the 'java' program image for anything prefixed with `/ld-` - in practice this will contain
     * `/ld-musl` for musl systems and probably something else for non-musl systems (eg. `/ld-linux-...`).
     * However, if such string is missing should indicate that the system is not a musl one.
     */
    // package-private access for testing only
    private static boolean isMuslJavaExecutable() throws IOException {

        byte[] magic = new byte[]{(byte)0x7f, (byte)'E', (byte)'L', (byte)'F'};
        byte[] prefix = new byte[]{(byte)'/', (byte)'l', (byte)'d', (byte)'-'}; // '/ld-*'
        byte[] musl = new byte[]{(byte)'m', (byte)'u', (byte)'s', (byte)'l'}; // 'musl'

        Path binary = Paths.get(System.getProperty("java.home"), "bin", "java");
        byte[] buffer = new byte[4096];

        try (InputStream is = Files.newInputStream(binary)) {
            int read = is.read(buffer, 0, 4);
            if (read != 4 || !containsArray(buffer, 0, magic)) {
                throw new IOException(Arrays.toString(buffer));
            }
            read = is.read(buffer);
            if (read <= 0) {
                throw new IOException();
            }
            int prefixPos = 0;
            for (int i = 0; i < read; i++) {
                if (buffer[i] == prefix[prefixPos]) {
                    if (++prefixPos == prefix.length) {
                        return containsArray(buffer, i + 1, musl);
                    }
                } else {
                    prefixPos = 0;
                }
            }
        }
        return false;
    }

    private static boolean containsArray(byte[] container, int offset, byte[] contained) {
        for (int i = 0; i < contained.length; i++) {
            int leftPos = offset + i;
            if (leftPos >= container.length) {
                return false;
            }
            if (container[leftPos] != contained[i]) {
                return false;
            }
        }
        return true;
    }
}
