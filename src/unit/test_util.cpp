/*
 * Copyright (c) Valkey Contributors
 * All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "generated_wrappers.hpp"

#include <climits>
#include <cstring>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

extern "C" {
#include "config.h"
#include "fmacros.h"
#include "util.h"

extern bool valgrind;
/* From main.cpp: non-NULL when --seed was passed (make test-unit seed=<n>). */
extern char *seed;
}

#if defined(__linux__)
#include <linux/magic.h>
#include <sys/statfs.h>
#endif

class UtilTest : public ::testing::Test {};

TEST_F(UtilTest, TestString2ll) {
    char buf[32];
    long long v;

    /* May not start with +. */
    valkey_strlcpy(buf, "+1", sizeof(buf));
    ASSERT_EQ(string2ll(buf, strlen(buf), &v), 0);

    /* Leading space. */
    valkey_strlcpy(buf, " 1", sizeof(buf));
    ASSERT_EQ(string2ll(buf, strlen(buf), &v), 0);

    /* Trailing space. */
    valkey_strlcpy(buf, "1 ", sizeof(buf));
    ASSERT_EQ(string2ll(buf, strlen(buf), &v), 0);

    /* May not start with 0. */
    valkey_strlcpy(buf, "01", sizeof(buf));
    ASSERT_EQ(string2ll(buf, strlen(buf), &v), 0);

    valkey_strlcpy(buf, "-1", sizeof(buf));
    ASSERT_EQ(string2ll(buf, strlen(buf), &v), 1);
    ASSERT_EQ(v, -1);

    valkey_strlcpy(buf, "0", sizeof(buf));
    ASSERT_EQ(string2ll(buf, strlen(buf), &v), 1);
    ASSERT_EQ(v, 0);

    valkey_strlcpy(buf, "1", sizeof(buf));
    ASSERT_EQ(string2ll(buf, strlen(buf), &v), 1);
    ASSERT_EQ(v, 1);

    valkey_strlcpy(buf, "99", sizeof(buf));
    ASSERT_EQ(string2ll(buf, strlen(buf), &v), 1);
    ASSERT_EQ(v, 99);

    valkey_strlcpy(buf, "-99", sizeof(buf));
    ASSERT_EQ(string2ll(buf, strlen(buf), &v), 1);
    ASSERT_EQ(v, -99);

    valkey_strlcpy(buf, "-9223372036854775808", sizeof(buf));
    ASSERT_EQ(string2ll(buf, strlen(buf), &v), 1);
    ASSERT_EQ(v, LLONG_MIN);

    valkey_strlcpy(buf, "-9223372036854775809", sizeof(buf)); /* overflow */
    ASSERT_EQ(string2ll(buf, strlen(buf), &v), 0);

    valkey_strlcpy(buf, "9223372036854775807", sizeof(buf));
    ASSERT_EQ(string2ll(buf, strlen(buf), &v), 1);
    ASSERT_EQ(v, LLONG_MAX);

    valkey_strlcpy(buf, "9223372036854775808", sizeof(buf)); /* overflow */
    ASSERT_EQ(string2ll(buf, strlen(buf), &v), 0);

    valkey_strlcpy(buf, "18446744073709551615", sizeof(buf)); /* overflow */
    ASSERT_EQ(string2ll(buf, strlen(buf), &v), 0);
}

TEST_F(UtilTest, TestString2l) {
    char buf[32];
    long v;

    /* May not start with +. */
    valkey_strlcpy(buf, "+1", sizeof(buf));
    ASSERT_EQ(string2l(buf, strlen(buf), &v), 0);

    /* May not start with 0. */
    valkey_strlcpy(buf, "01", sizeof(buf));
    ASSERT_EQ(string2l(buf, strlen(buf), &v), 0);

    valkey_strlcpy(buf, "-1", sizeof(buf));
    ASSERT_EQ(string2l(buf, strlen(buf), &v), 1);
    ASSERT_EQ(v, -1);

    valkey_strlcpy(buf, "0", sizeof(buf));
    ASSERT_EQ(string2l(buf, strlen(buf), &v), 1);
    ASSERT_EQ(v, 0);

    valkey_strlcpy(buf, "1", sizeof(buf));
    ASSERT_EQ(string2l(buf, strlen(buf), &v), 1);
    ASSERT_EQ(v, 1);

    valkey_strlcpy(buf, "99", sizeof(buf));
    ASSERT_EQ(string2l(buf, strlen(buf), &v), 1);
    ASSERT_EQ(v, 99);

    valkey_strlcpy(buf, "-99", sizeof(buf));
    ASSERT_EQ(string2l(buf, strlen(buf), &v), 1);
    ASSERT_EQ(v, -99);

#if LONG_MAX != LLONG_MAX
    valkey_strlcpy(buf, "-2147483648", sizeof(buf));
    ASSERT_EQ(string2l(buf, strlen(buf), &v), 1);
    ASSERT_EQ(v, LONG_MIN);

    valkey_strlcpy(buf, "-2147483649", sizeof(buf)); /* overflow */
    ASSERT_EQ(string2l(buf, strlen(buf), &v), 0);

    valkey_strlcpy(buf, "2147483647", sizeof(buf));
    ASSERT_EQ(string2l(buf, strlen(buf), &v), 1);
    ASSERT_EQ(v, LONG_MAX);

    valkey_strlcpy(buf, "2147483648", sizeof(buf)); /* overflow */
    ASSERT_EQ(string2l(buf, strlen(buf), &v), 0);
#endif
}

TEST_F(UtilTest, TestLl2string) {
    char buf[32];
    long long v;
    int sz;

    v = 0;
    sz = ll2string(buf, sizeof buf, v);
    ASSERT_EQ(sz, 1);
    ASSERT_TRUE(!strcmp(buf, "0"));

    v = -1;
    sz = ll2string(buf, sizeof buf, v);
    ASSERT_EQ(sz, 2);
    ASSERT_TRUE(!strcmp(buf, "-1"));

    v = 99;
    sz = ll2string(buf, sizeof buf, v);
    ASSERT_EQ(sz, 2);
    ASSERT_TRUE(!strcmp(buf, "99"));

    v = -99;
    sz = ll2string(buf, sizeof buf, v);
    ASSERT_EQ(sz, 3);
    ASSERT_TRUE(!strcmp(buf, "-99"));

    v = -2147483648;
    sz = ll2string(buf, sizeof buf, v);
    ASSERT_EQ(sz, 11);
    ASSERT_TRUE(!strcmp(buf, "-2147483648"));

    v = LLONG_MIN;
    sz = ll2string(buf, sizeof buf, v);
    ASSERT_EQ(sz, 20);
    ASSERT_TRUE(!strcmp(buf, "-9223372036854775808"));

    v = LLONG_MAX;
    sz = ll2string(buf, sizeof buf, v);
    ASSERT_EQ(sz, 19);
    ASSERT_TRUE(!strcmp(buf, "9223372036854775807"));
}

TEST_F(UtilTest, TestLd2string) {
    char buf[32];
    long double v;
    int sz;

    v = 0.0 / 0.0;
    sz = ld2string(buf, sizeof(buf), v, LD_STR_AUTO);
    ASSERT_EQ(sz, 3);
    ASSERT_TRUE(!strcmp(buf, "nan"));
}

TEST_F(UtilTest, TestFixedpointD2string) {
    char buf[32];
    double v;
    int sz;

    v = 0.0;
    sz = fixedpoint_d2string(buf, sizeof buf, v, 4);
    ASSERT_EQ(sz, 6);
    ASSERT_TRUE(!strcmp(buf, "0.0000"));

    sz = fixedpoint_d2string(buf, sizeof buf, v, 1);
    ASSERT_EQ(sz, 3);
    ASSERT_TRUE(!strcmp(buf, "0.0"));

    /* set junk in buffer */
    memset(buf, 'A', 32);
    v = 0.0001;
    sz = fixedpoint_d2string(buf, sizeof buf, v, 4);
    ASSERT_EQ(sz, 6);
    ASSERT_EQ(buf[sz], '\0');
    ASSERT_TRUE(!strcmp(buf, "0.0001"));

    /* set junk in buffer */
    memset(buf, 'A', 32);
    v = 6.0642951598391699e-05;
    sz = fixedpoint_d2string(buf, sizeof buf, v, 4);
    ASSERT_EQ(sz, 6);
    ASSERT_EQ(buf[sz], '\0');
    ASSERT_TRUE(!strcmp(buf, "0.0001"));

    v = 0.01;
    sz = fixedpoint_d2string(buf, sizeof buf, v, 4);
    ASSERT_EQ(sz, 6);
    ASSERT_TRUE(!strcmp(buf, "0.0100"));

    sz = fixedpoint_d2string(buf, sizeof buf, v, 1);
    ASSERT_EQ(sz, 3);
    ASSERT_TRUE(!strcmp(buf, "0.0"));

    v = -0.01;
    sz = fixedpoint_d2string(buf, sizeof buf, v, 4);
    ASSERT_EQ(sz, 7);
    ASSERT_TRUE(!strcmp(buf, "-0.0100"));

    v = -0.1;
    sz = fixedpoint_d2string(buf, sizeof buf, v, 1);
    ASSERT_EQ(sz, 4);
    ASSERT_TRUE(!strcmp(buf, "-0.1"));

    v = 0.1;
    sz = fixedpoint_d2string(buf, sizeof buf, v, 1);
    ASSERT_EQ(sz, 3);
    ASSERT_TRUE(!strcmp(buf, "0.1"));

    v = 0.01;
    sz = fixedpoint_d2string(buf, sizeof buf, v, 17);
    ASSERT_EQ(sz, 19);
    ASSERT_TRUE(!strcmp(buf, "0.01000000000000000"));

    v = 10.01;
    sz = fixedpoint_d2string(buf, sizeof buf, v, 4);
    ASSERT_EQ(sz, 7);
    ASSERT_TRUE(!strcmp(buf, "10.0100"));

    /* negative tests */
    sz = fixedpoint_d2string(buf, sizeof buf, v, 18);
    ASSERT_EQ(sz, 0);

    sz = fixedpoint_d2string(buf, sizeof buf, v, 0);
    ASSERT_EQ(sz, 0);

    sz = fixedpoint_d2string(buf, 1, v, 1);
    ASSERT_EQ(sz, 0);
}

TEST_F(UtilTest, TestVersion2num) {
    ASSERT_EQ(version2num("7.2.5"), 0x070205);
    ASSERT_EQ(version2num("255.255.255"), 0xffffff);
    ASSERT_EQ(version2num("7.2.256"), -1);
    ASSERT_EQ(version2num("7.2"), -1);
    ASSERT_EQ(version2num("7.2.1.0"), -1);
    ASSERT_EQ(version2num("1.-2.-3"), -1);
    ASSERT_EQ(version2num("1.2.3-rc4"), -1);
    ASSERT_EQ(version2num(""), -1);
}

#if defined(__linux__)
/* Since fadvise and mincore is only supported in specific platforms like
 * Linux, we only verify the fadvise mechanism works in Linux */
static int cache_exist(int fd) {
    unsigned char flag;
    void *m = mmap(NULL, 4096, PROT_READ, MAP_SHARED, fd, 0);
    if (m == MAP_FAILED) return -1;
    if (mincore(m, 4096, &flag) != 0) {
        munmap(m, 4096);
        return -1;
    }
    munmap(m, 4096);
    /* the least significant bit of the byte will be set if the corresponding
     * page is currently resident in memory */
    return flag & 1;
}
#endif

TEST_F(UtilTest, TestReclaimFilePageCache) {
    /* The test is incompatible with valgrind, skip it. */
    if (valgrind) GTEST_SKIP() << "Skipping test due to incompatibility with valgrind";

#if defined(__linux__)
    struct statfs stats;

    /* Check if /tmp is memory-backed (e.g., tmpfs) */
    if (statfs("/tmp", &stats) == 0) {
        if (stats.f_type != TMPFS_MAGIC) { // Not tmpfs, use /tmp
            GTEST_SKIP() << "Skipping test because /tmp is not tmpfs";
        }
    }

    const char *tmpfile = "/tmp/redis-reclaim-cache-test";
    int fd = open(tmpfile, O_RDWR | O_CREAT, 0644);
    ASSERT_GE(fd, 0);

    /* test write file */
    char buf[4] = "foo";
    ASSERT_GT(write(fd, buf, sizeof(buf)), 0);
    ASSERT_TRUE(cache_exist(fd));
    ASSERT_EQ(valkey_fsync(fd), 0);
    ASSERT_EQ(reclaimFilePageCache(fd, 0, 0), 0);
    ASSERT_TRUE(!cache_exist(fd));

    /* test read file */
    ASSERT_GT(pread(fd, buf, sizeof(buf), 0), 0);
    ASSERT_TRUE(cache_exist(fd));
    ASSERT_EQ(reclaimFilePageCache(fd, 0, 0), 0);
    ASSERT_TRUE(!cache_exist(fd));

    close(fd);
    unlink(tmpfile);
#else
    GTEST_SKIP() << "Test only supported on Linux";
#endif
}

TEST_F(UtilTest, TestWritePointerWithPadding) {
    unsigned char buf[8];
    static int dummy;
    void *ptr = &dummy;
    size_t ptr_size = sizeof(ptr);

    /* Write the pointer and pad to 8 bytes */
    writePointerWithPadding(buf, ptr);

    /* The first ptr_size bytes must match the raw pointer bytes */
    unsigned char expected[sizeof(ptr)];
    memcpy(expected, &ptr, ptr_size);
    ASSERT_EQ(memcmp(buf, expected, ptr_size), 0);

    /* The remaining bytes (if any) must be zero */
    for (size_t i = ptr_size; i < sizeof(buf); i++) {
        ASSERT_EQ(buf[i], 0u);
    }
}

/* getSecureRandomHexChars() must emit only [0-9a-f] and must not write past
 * len. The charset is the documented contract, so it is an oracle independent
 * of whichever generator backs the call (provider RAND or the bundled one). */
TEST_F(UtilTest, TestSecureRandomHexCharsCharset) {
    /* A caller-supplied reproducible seed is by construction not usable for
     * secret material, so the secure random helpers fail closed in that mode
     * and these tests cannot run. The skip is unconditional, and therefore
     * conservative in BUILD_TLS=yes builds where the provider path ignores the
     * seed entirely, rather than pulling config.h into the unit test. */
    if (seed != nullptr) GTEST_SKIP() << "reproducible --seed mode: a caller-supplied deterministic seed is not usable "
                                         "for secret material, so the secure random helpers fail closed";

    char buf[128];
    const size_t len = sizeof(buf) - 1;

    memset(buf, '#', sizeof(buf)); /* '#' is outside the charset, so any byte
                                    * left as '#' below was never written. */
    ASSERT_EQ(getSecureRandomHexChars(buf, len), 1);

    for (size_t i = 0; i < len; i++) {
        ASSERT_TRUE((buf[i] >= '0' && buf[i] <= '9') || (buf[i] >= 'a' && buf[i] <= 'f'))
            << "byte " << i << " is 0x" << std::hex << (int)(unsigned char)buf[i];
    }
    ASSERT_EQ(buf[len], '#');
}

/* Every byte position must be written, successive draws must differ, and
 * nothing past len may be touched.
 *
 * The oracle is independent of the generator: the buffer is prefilled with a
 * canary and drawn into kDraws times. If some position is never written it
 * holds the canary in every draw; if it is written, it survives as the canary
 * in all kDraws draws only with probability (2^-8)^kDraws (2^-128 here),
 * since the canary is one byte value out of 256 per draw. That catches a stub
 * filling only a prefix, which the previous all-zeros/two-draws memcmp pair
 * did not: memcmp returns at the first differing byte, so a
 * generator writing just a[0] passed both of its assertions. */
TEST_F(UtilTest, TestSecureRandomBytesFillsAndVaries) {
    /* A caller-supplied reproducible seed is by construction not usable for
     * secret material, so the secure random helpers fail closed in that mode
     * and these tests cannot run. The skip is unconditional, and therefore
     * conservative in BUILD_TLS=yes builds where the provider path ignores the
     * seed entirely, rather than pulling config.h into the unit test. */
    if (seed != nullptr) GTEST_SKIP() << "reproducible --seed mode: a caller-supplied deterministic seed is not usable "
                                         "for secret material, so the secure random helpers fail closed";

    constexpr size_t kLen = 64;
    constexpr int kDraws = 16;
    constexpr unsigned char kCanary = 0xA5;

    bool written[kLen];
    memset(written, 0, sizeof(written));

    unsigned char prev[kLen];
    memset(prev, kCanary, sizeof(prev));

    for (int draw = 0; draw < kDraws; draw++) {
        unsigned char buf[kLen + 1];
        memset(buf, kCanary, sizeof(buf));

        ASSERT_EQ(getSecureRandomBytes(buf, kLen), 1) << "draw " << draw << " failed";
        ASSERT_EQ(buf[kLen], kCanary) << "draw " << draw << " wrote past len";

        for (size_t i = 0; i < kLen; i++) {
            if (buf[i] != kCanary) written[i] = true;
        }

        /* Two consecutive identical 64-byte draws is a 2^-512 event. */
        if (draw > 0) {
            ASSERT_NE(memcmp(buf, prev, kLen), 0) << "draw " << draw << " repeats the previous draw";
        }
        memcpy(prev, buf, kLen);
    }

    for (size_t i = 0; i < kLen; i++) {
        ASSERT_TRUE(written[i]) << "byte " << i << " was never written in " << kDraws << " draws";
    }
}

/* A forked child must not continue the parent's generator stream. The RDB child
 * publishes its draw, the diskless replication EOF mark, on the replication
 * link while the parent's stream position does not advance, so the two streams
 * have to be independent. The oracle is that documented property rather than
 * the implementation: parent and child draw the same number of bytes from what
 * would be the same inherited position, and the results must differ. Two equal
 * 40 character hex draws is a 2^-160 event.
 *
 * This exercises the always-bundled non-secure generator, so it is valid in
 * provider and non-provider builds alike. */
TEST_F(UtilTest, TestRandomStreamDivergesAcrossFork) {
    constexpr size_t kLen = 40;

    /* Draw in the parent first: this registers the atfork handlers and fixes
     * the stream position the child would otherwise inherit and reuse. */
    char warmup[kLen];
    getRandomHexChars(warmup, sizeof(warmup));

    int fds[2];
    ASSERT_EQ(pipe(fds), 0);

    pid_t pid = fork();
    ASSERT_NE(pid, -1);
    if (pid == 0) {
        /* No gtest assertions here: they are not valid in the child. */
        char buf[kLen];
        getRandomHexChars(buf, sizeof(buf));
        ssize_t written = write(fds[1], buf, sizeof(buf));
        _exit(written == (ssize_t)sizeof(buf) ? 0 : 1);
    }
    ASSERT_EQ(close(fds[1]), 0);

    char parent_buf[kLen];
    getRandomHexChars(parent_buf, sizeof(parent_buf));

    char child_buf[kLen];
    size_t got = 0;
    while (got < sizeof(child_buf)) {
        ssize_t n = read(fds[0], child_buf + got, sizeof(child_buf) - got);
        if (n <= 0) break;
        got += (size_t)n;
    }
    ASSERT_EQ(close(fds[0]), 0);

    int status = 0;
    ASSERT_EQ(waitpid(pid, &status, 0), pid);
    ASSERT_TRUE(WIFEXITED(status));
    ASSERT_EQ(WEXITSTATUS(status), 0);
    ASSERT_EQ(got, sizeof(child_buf));

    ASSERT_NE(memcmp(parent_buf, child_buf, sizeof(parent_buf)), 0)
        << "the forked child continued the parent's random stream";
}

/* Forking must diversify the generator, never downgrade it. Two properties are
 * asserted, both taken from the documented fork contract rather than from the
 * generator's own output:
 *
 *  1. Every child draws a stream distinct from the parent's and from any
 *     sibling's. Siblings differ only in pid and fork time, so this is the
 *     assertion that those actually reach the derivation.
 *
 *  2. A child keeps the parent's ability to draw secret material even when it
 *     cannot reach /dev/urandom. The second child lowers RLIMIT_NOFILE to zero
 *     before drawing, so every open() it makes fails the way an fd-exhausted
 *     process, a restricted /dev or a per-child seccomp filter would. A child
 *     that has to re-acquire entropy of its own fails or silently degrades
 *     there, which is what makes the diskless replication EOF mark guessable;
 *     a child that carries the parent's keying material forward does not. */
TEST_F(UtilTest, TestRandomRekeysRatherThanReseedsAcrossFork) {
    /* setRandomSeedCString() clears seed_from_urandom by design, so the secure
     * helpers fail closed in reproducible-seed mode and property 2 cannot run
     * there. */
    if (seed != nullptr) GTEST_SKIP() << "reproducible --seed mode: a caller-supplied deterministic seed is not usable "
                                         "for secret material, so the secure random helpers fail closed";

    constexpr size_t kLen = 40;
    /* One status byte for the child's getSecureRandomBytes() result. */
    constexpr size_t kMsgLen = kLen + 1;
    constexpr int kChildren = 2;

    /* Draw in the parent first so the children exercise the post-fork path
     * rather than a first-draw initialization, and probe the parent's own
     * usability: property 2 only says a child keeps what the parent had. */
    unsigned char warmup[kLen];
    getRandomBytes(warmup, sizeof(warmup));
    int parent_usable = getSecureRandomBytes(warmup, sizeof(warmup));

    unsigned char child_msg[kChildren][kMsgLen];
    pid_t pids[kChildren];
    int read_fds[kChildren];

    for (int c = 0; c < kChildren; c++) {
        int fds[2];
        ASSERT_EQ(pipe(fds), 0);

        pids[c] = fork();
        ASSERT_NE(pids[c], -1);
        if (pids[c] == 0) {
            /* No gtest assertions here: they are not valid in the child. */
            unsigned char msg[kMsgLen];
            unsigned char secure[kLen];
            /* Already open descriptors, the pipe included, survive this; only
             * new ones are refused. */
            struct rlimit no_files = {0, 0};
            if (c == 1 && setrlimit(RLIMIT_NOFILE, &no_files) != 0) _exit(2);
            getRandomBytes(msg, kLen);
            msg[kLen] = getSecureRandomBytes(secure, sizeof(secure)) ? 1 : 0;
            ssize_t written = write(fds[1], msg, sizeof(msg));
            _exit(written == (ssize_t)sizeof(msg) ? 0 : 1);
        }
        ASSERT_EQ(close(fds[1]), 0);
        read_fds[c] = fds[0];
    }

    unsigned char parent_buf[kLen];
    getRandomBytes(parent_buf, sizeof(parent_buf));

    for (int c = 0; c < kChildren; c++) {
        size_t got = 0;
        while (got < kMsgLen) {
            ssize_t n = read(read_fds[c], child_msg[c] + got, kMsgLen - got);
            if (n <= 0) break;
            got += (size_t)n;
        }
        ASSERT_EQ(close(read_fds[c]), 0);

        int status = 0;
        ASSERT_EQ(waitpid(pids[c], &status, 0), pids[c]);
        ASSERT_TRUE(WIFEXITED(status));
        ASSERT_EQ(WEXITSTATUS(status), 0) << "child " << c << " failed to report its draw";
        ASSERT_EQ(got, kMsgLen) << "child " << c << " reported a short draw";

        /* Two equal 40 byte draws is a 2^-320 event. */
        ASSERT_NE(memcmp(parent_buf, child_msg[c], kLen), 0)
            << "child " << c << " continued the parent's random stream";

        if (parent_usable) {
            ASSERT_EQ(child_msg[c][kLen], 1)
                << "child " << c << " lost the parent's ability to draw secret material across fork";
        }
    }

    ASSERT_NE(memcmp(child_msg[0], child_msg[1], kLen), 0) << "two children drew the same random stream";
}
