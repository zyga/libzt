/* This is an open source non-commercial project. Dear PVS-Studio, please check it.
 * PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
 *
 * Copyright 2019-2020 Zygmunt Krynicki.
 *
 * This file is part of libzt.
 *
 * Libzt is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License.
 *
 * Libzt is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Libzt.  If not, see <https://www.gnu.org/licenses/>. */

#ifdef __linux__
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <assert.h>
#include <ctype.h>
#include <limits.h>

#define ZT_SELF_TEST_BUILD
#include "zt.c"

/* internal self-test helpers */

static FILE* selftest_temporary_file(void)
{
    FILE* f;
#if defined(__linux__)
    /* Hardened version for Linux. */
    char tmpl[PATH_MAX];
    const char* tmp_dir_name;
    int fd;

    tmp_dir_name = getenv("TMPDIR");
    if (tmp_dir_name == NULL) {
        tmp_dir_name = "/tmp";
    }
#if defined(O_TMPFILE)
    fd = open(tmp_dir_name, O_TMPFILE | O_RDWR | O_EXCL, 0600);
    if (fd < 0) {
        if (errno != EOPNOTSUPP) {
            perror("cannot open temporary file via O_TMPFILE");
            exit(1);
        }
    }
    tmpl[0] = '\0';
#else
    fd = -1;
#endif
    if (fd < 0) {
        int n = snprintf(tmpl, sizeof tmpl - 1, "%s/libzt-test.XXXXXX", tmp_dir_name);
        if (n < 0 || (size_t)n >= sizeof tmpl - 1) {
            perror("cannot format temporary file name");
            exit(1);
        }
        fd = mkostemp(tmpl, O_CLOEXEC);
    }
    if (fd < 0) {
        perror("cannot open temporary file");
        exit(1);
    }
    if (tmpl[0] != '\0' && unlink(tmpl) < 0) {
        perror("cannot unlink temporary file");
        exit(1);
    }
    f = fdopen(fd, "w+");
#else
    /* Portable version. */
    f = tmpfile();
#endif
    if (f == NULL) {
        perror("cannot open temporary file");
        exit(1);
    }
    return f;
}

static zt_test selftest_make_test(void)
{
    zt_test t;
    memset(&t, 0, sizeof t);
    t.location.fname = "file.c";
    t.location.lineno = 13;
    t.stream = selftest_temporary_file();
    return t;
}

static void selftest_close_test(zt_t t)
{
    fclose(t->stream);
    t->stream = NULL;
}

static void selftest_quote_str(FILE* stream, const char* str)
{
    /* self-test quote function uses single-quotes. Meanwhile the production
     * quote function uses double quotes. This allows one to quote the other
     * without unnecessary escaping. */
    int c;
    fputs("\'", stream);
    while ((c = *(str++))) {
        switch (c) {
        case '\'':
            fputs("\\'", stream);
            break;
        case '\n':
            fputs("\\n", stream);
            break;
        case '\r':
            fputs("\\r", stream);
            break;
        case '\t':
            fputs("\\t", stream);
            break;
        case '\v':
            fputs("\\v", stream);
            break;
        default:
            if (isprint(c)) {
                fputc(c, stream);
            } else {
                fprintf(stream, "\\%#04x", c);
            }
            break;
        }
    }
    fputs("\'", stream);
}

#define selftest_stream_eq(stream, expected) \
    selftest_stream_eq_at(                   \
        stream, __FILE__, __LINE__, "%s", expected)

static void selftest_stream_eq_at(FILE* stream, const char* file,
    int lineno, const char* fmt, ...)
{
    size_t nread;
    int nfmt;
    char buf1[1024], buf2[1024];
    size_t buf1_size = sizeof buf1;
    size_t buf2_size = sizeof buf2;
    va_list ap;

    if (fseek(stream, 0, SEEK_SET) < 0) {
        perror("cannot seek in test stream");
        exit(1);
    }
    nread = fread(buf1, 1, buf1_size - 1, stream);
    if (nread == 0 && !feof(stream) && ferror(stream) != 0) {
        perror("cannot read from test stream");
        exit(1);
    }
    if (nread == buf1_size - 1 && !feof(stream)) {
        fprintf(stderr, "insufficient space to read entire stream\n");
        exit(1);
    }
    buf1[nread] = '\0';

    va_start(ap, fmt);
    nfmt = vsnprintf(buf2, buf2_size, fmt, ap);
    va_end(ap);
    if (nfmt < 0) {
        perror("cannot format expected stream contents");
        exit(1);
    }
    if ((size_t)nfmt >= buf2_size) {
        fprintf(stderr, "insufficient space to format expected stream contents\n");
        exit(1);
    }
    buf2[nfmt] = '\0';

    if (strcmp(buf1, buf2) != 0) {
        fprintf(stderr, "%s:%d: error: stream content mismatch ", file, lineno);
        selftest_quote_str(stderr, buf1);
        fprintf(stderr, " ( <- actual) != ");
        selftest_quote_str(stderr, buf2);
        fprintf(stderr, " ( <- expected)\n");
        exit(1);
    }
}

/* library version */

static void test_MAJOR_MINOR_VERSION(void)
{
    assert(ZT_MAJOR_VERSION == 0);
    assert(ZT_MINOR_VERSION == 1);
}

/* packing arguments */

static void test_pack_boolean(void)
{
    zt_value v;

    v = zt_pack_boolean(true, "true");
    assert(zt_value_kind_of(v) == ZT_BOOLEAN);
    assert(v.as.boolean == true);
    assert(strcmp(zt_source_of(v), "true") == 0);

    v = zt_pack_boolean(false, "false");
    assert(zt_value_kind_of(v) == ZT_BOOLEAN);
    assert(v.as.boolean == false);
    assert(strcmp(zt_source_of(v), "false") == 0);
}

static void test_pack_rune(void)
{
    zt_value v;

    v = zt_pack_rune('a', "'a'");
    assert(zt_value_kind_of(v) == ZT_RUNE);
    assert(v.as.rune == 'a');
    assert(strcmp(zt_source_of(v), "'a'") == 0);

    v = zt_pack_rune('\x80', "'\\x80'");
    assert(zt_value_kind_of(v) == ZT_RUNE);
    assert(v.as.rune == 0x80);
    assert(strcmp(zt_source_of(v), "'\\x80'") == 0);

    v = zt_pack_rune('\x01', "'\\x01'");
    assert(v.as.rune == 0x01);
    v = zt_pack_rune('\x80', "'\\x80'");
    assert(v.as.rune == 0x80);
    v = zt_pack_rune('\xFF', "'\\xFF'");
    assert(v.as.rune == 0xFF);
}

static void test_pack_integer(void)
{
    zt_value v;

    v = zt_pack_integer(42, "42");
    assert(zt_value_kind_of(v) == ZT_INTEGER);
    assert(v.as.integer == 42);
    assert(strcmp(zt_source_of(v), "42") == 0);
}

static void test_pack_unsigned(void)
{
    zt_value v;

    v = zt_pack_unsigned(42U, "42U");
    assert(zt_value_kind_of(v) == ZT_UNSIGNED);
    assert(v.as.unsigned_integer == 42U);
    assert(strcmp(zt_source_of(v), "42U") == 0);
}

static void test_pack_string(void)
{
    zt_value v;

    v = zt_pack_string("foo", "\"foo\"");
    assert(zt_value_kind_of(v) == ZT_STRING);
    assert(strcmp(v.as.string, "foo") == 0);
    assert(strcmp(zt_source_of(v), "\"foo\"") == 0);
}

static void test_pack_pointer(void)
{
    zt_value v;

    v = zt_pack_pointer(NULL, "NULL");
    assert(zt_value_kind_of(v) == ZT_POINTER);
    assert(v.as.pointer == NULL);
    assert(strcmp(zt_source_of(v), "NULL") == 0);
}

/* binary relation */

static void test_find_binary_relation(void)
{
    assert(zt_find_binary_relation("==") == ZT_REL_EQ);
    assert(zt_find_binary_relation("!=") == ZT_REL_NE);
    assert(zt_find_binary_relation("<=") == ZT_REL_LE);
    assert(zt_find_binary_relation(">=") == ZT_REL_GE);
    assert(zt_find_binary_relation("<") == ZT_REL_LT);
    assert(zt_find_binary_relation(">") == ZT_REL_GT);
    assert(zt_find_binary_relation("potato") == ZT_REL_INVALID);
}

static void test_invert_binary_relation(void)
{

    assert(zt_invert_binary_relation(ZT_REL_EQ) == ZT_REL_NE);
    assert(zt_invert_binary_relation(ZT_REL_NE) == ZT_REL_EQ);
    assert(zt_invert_binary_relation(ZT_REL_LE) == ZT_REL_GT);
    assert(zt_invert_binary_relation(ZT_REL_GE) == ZT_REL_LT);
    assert(zt_invert_binary_relation(ZT_REL_LT) == ZT_REL_GE);
    assert(zt_invert_binary_relation(ZT_REL_GT) == ZT_REL_LE);
    assert(zt_invert_binary_relation(ZT_REL_INVALID) == ZT_REL_INVALID);
    assert(zt_invert_binary_relation(1000) == ZT_REL_INVALID);
}

static void test_binary_relation_as_text(void)
{
    assert(strcmp(zt_binary_relation_as_text(ZT_REL_EQ), "==") == 0);
    assert(strcmp(zt_binary_relation_as_text(ZT_REL_NE), "!=") == 0);
    assert(strcmp(zt_binary_relation_as_text(ZT_REL_LE), "<=") == 0);
    assert(strcmp(zt_binary_relation_as_text(ZT_REL_GE), ">=") == 0);
    assert(strcmp(zt_binary_relation_as_text(ZT_REL_LT), "<") == 0);
    assert(strcmp(zt_binary_relation_as_text(ZT_REL_GT), ">") == 0);
    assert(strcmp(zt_binary_relation_as_text(ZT_REL_INVALID), "invalid") == 0);
    assert(strcmp(zt_binary_relation_as_text(1000), "invalid") == 0);
}

/* boolean formatting */

static void test_boolean_as_text(void)
{
    assert(strcmp(zt_boolean_as_text(true), "true") == 0);
    assert(strcmp(zt_boolean_as_text(false), "false") == 0);
}

/* claim verification */
static bool selftest_passing_verify0_called;
static bool selftest_passing_verify0(zt_t t)
{
    assert(t != NULL);
    selftest_passing_verify0_called = true;
    return true;
}

static bool selftest_passing_verify1_called;
static bool selftest_passing_verify1(zt_t t, ZT_UNUSED zt_value arg1)
{
    assert(t != NULL);
    selftest_passing_verify1_called = true;
    return true;
}

static bool selftest_passing_verify2_called;
static bool selftest_passing_verify2(zt_t t, ZT_UNUSED zt_value arg1, ZT_UNUSED zt_value arg2)
{
    assert(t != NULL);
    selftest_passing_verify2_called = true;
    return true;
}

static bool selftest_passing_verify3_called;
static bool selftest_passing_verify3(zt_t t, ZT_UNUSED zt_value arg1, ZT_UNUSED zt_value arg2, ZT_UNUSED zt_value arg3)
{
    assert(t != NULL);
    selftest_passing_verify3_called = true;
    return true;
}

static zt_verifier selftest_passing_verifier0(void)
{
    zt_verifier verifier;
    memset(&verifier, 0, sizeof verifier);
    verifier.func.args0 = selftest_passing_verify0;
    verifier.nargs = 0;
    return verifier;
}

static zt_verifier selftest_passing_verifier1(void)
{
    zt_verifier verifier;
    memset(&verifier, 0, sizeof verifier);
    verifier.func.args1 = selftest_passing_verify1;
    verifier.nargs = 1;
    verifier.arg_infos[0].kind = ZT_INTEGER;
    verifier.arg_infos[0].kind_mismatch_msg = "arg[0] type mismatch";
    return verifier;
}

static zt_verifier selftest_passing_verifier2(void)
{
    zt_verifier verifier;
    memset(&verifier, 0, sizeof verifier);
    verifier.func.args2 = selftest_passing_verify2;
    verifier.nargs = 2;
    verifier.arg_infos[0].kind = ZT_INTEGER;
    verifier.arg_infos[0].kind_mismatch_msg = "arg[0] type mismatch";
    verifier.arg_infos[1].kind = ZT_INTEGER;
    verifier.arg_infos[1].kind_mismatch_msg = "arg[1] type mismatch";
    return verifier;
}

static zt_verifier selftest_passing_verifier3(void)
{
    zt_verifier verifier;
    memset(&verifier, 0, sizeof verifier);
    verifier.func.args3 = selftest_passing_verify3;
    verifier.nargs = 3;
    verifier.arg_infos[0].kind = ZT_INTEGER;
    verifier.arg_infos[0].kind_mismatch_msg = "arg[0] type mismatch";
    verifier.arg_infos[1].kind = ZT_INTEGER;
    verifier.arg_infos[1].kind_mismatch_msg = "arg[1] type mismatch";
    verifier.arg_infos[2].kind = ZT_INTEGER;
    verifier.arg_infos[2].kind_mismatch_msg = "arg[2] type mismatch";
    return verifier;
}

static void test_verify_claim0(void)
{
    zt_claim claim;
    bool result;
    zt_test t = selftest_make_test();
    memset(&claim, 0, sizeof claim);
    claim.location.fname = "file.c";
    claim.location.lineno = 13;
    selftest_passing_verify0_called = true;
    claim.make_verifier = selftest_passing_verifier0;
    result = zt_verify_claim(&t, &claim);
    assert(result == true);
    assert(selftest_passing_verify0_called == true);
    selftest_close_test(&t);
}

static void test_verify_claim1(void)
{
    bool result;
    zt_claim claim;
    zt_test t = selftest_make_test();
    memset(&claim, 0, sizeof claim);
    claim.args[0] = zt_pack_integer(123, "arg0");
    claim.location.fname = "file.c";
    claim.location.lineno = 13;
    selftest_passing_verify0_called = true;
    claim.make_verifier = selftest_passing_verifier1;
    result = zt_verify_claim(&t, &claim);
    assert(result == true);
    assert(selftest_passing_verify1_called == true);
    selftest_close_test(&t);
}

static void test_verify_claim2(void)
{
    bool result;
    zt_claim claim;
    zt_test t = selftest_make_test();
    memset(&claim, 0, sizeof claim);
    claim.args[0] = zt_pack_integer(123, "arg0");
    claim.args[1] = zt_pack_integer(123, "arg1");
    claim.location.fname = "file.c";
    claim.location.lineno = 13;
    selftest_passing_verify2_called = true;
    claim.make_verifier = selftest_passing_verifier2;
    result = zt_verify_claim(&t, &claim);
    assert(result == true);
    assert(selftest_passing_verify2_called == true);
    selftest_close_test(&t);
}

static void test_verify_claim3(void)
{
    bool result;
    zt_claim claim;
    zt_test t = selftest_make_test();
    memset(&claim, 0, sizeof claim);
    claim.args[0] = zt_pack_integer(123, "arg0");
    claim.args[1] = zt_pack_integer(123, "arg1");
    claim.args[2] = zt_pack_integer(123, "arg2");
    claim.location.fname = "file.c";
    claim.location.lineno = 13;
    selftest_passing_verify3_called = true;
    claim.make_verifier = selftest_passing_verifier3;
    result = zt_verify_claim(&t, &claim);
    assert(result == true);
    assert(selftest_passing_verify3_called == true);
    selftest_close_test(&t);
}

static zt_verifier selftest_bogus_verifier4(void)
{
    zt_verifier verifier;
    memset(&verifier, 0, sizeof verifier);
    verifier.nargs = 4;
    return verifier;
}

static void test_verify_bogus_claim4(void)
{
    bool result;
    zt_claim claim;
    zt_test t = selftest_make_test();
    memset(&claim, 0, sizeof claim);
    claim.make_verifier = selftest_bogus_verifier4;
    claim.location.fname = "file.c";
    claim.location.lineno = 13;
    result = zt_verify_claim(&t, &claim);
    assert(result == false);
    selftest_stream_eq(t.stream, "file.c:13: unsupported number of arguments: 4\n");
    selftest_close_test(&t);
}

static void test_verify_mismatch_claim1of1(void)
{
    bool result;
    zt_claim claim;
    zt_test t = selftest_make_test();
    memset(&claim, 0, sizeof claim);
    claim.args[0] = zt_pack_unsigned(123U, "arg0");
    claim.location.fname = "file.c";
    claim.location.lineno = 13;
    selftest_passing_verify1_called = false;
    claim.make_verifier = selftest_passing_verifier1;
    result = zt_verify_claim(&t, &claim);
    assert(result == false);
    assert(selftest_passing_verify1_called == false);
    selftest_stream_eq(t.stream, "file.c:13: arg[0] type mismatch\n");
    selftest_close_test(&t);
}

static void test_verify_mismatch_claim1of2(void)
{
    bool result;
    zt_claim claim;
    zt_test t = selftest_make_test();
    memset(&claim, 0, sizeof claim);
    claim.args[0] = zt_pack_unsigned(123U, "arg0");
    claim.args[1] = zt_pack_integer(123, "arg1");
    claim.location.fname = "file.c";
    claim.location.lineno = 13;
    selftest_passing_verify2_called = false;
    claim.make_verifier = selftest_passing_verifier2;
    result = zt_verify_claim(&t, &claim);
    assert(result == false);
    assert(selftest_passing_verify2_called == false);
    selftest_stream_eq(t.stream, "file.c:13: arg[0] type mismatch\n");
    selftest_close_test(&t);
}

static void test_verify_mismatch_claim2of2(void)
{
    bool result;
    zt_claim claim;
    zt_test t = selftest_make_test();
    memset(&claim, 0, sizeof claim);
    claim.args[0] = zt_pack_integer(123, "arg0");
    claim.args[1] = zt_pack_unsigned(123U, "arg1");
    claim.location.fname = "file.c";
    claim.location.lineno = 13;
    selftest_passing_verify2_called = false;
    claim.make_verifier = selftest_passing_verifier2;
    result = zt_verify_claim(&t, &claim);
    assert(result == false);
    assert(selftest_passing_verify2_called == false);
    selftest_stream_eq(t.stream, "file.c:13: arg[1] type mismatch\n");
    selftest_close_test(&t);
}

static void test_verify_mismatch_claim1of3(void)
{
    bool result;
    zt_claim claim;
    zt_test t = selftest_make_test();
    memset(&claim, 0, sizeof claim);
    claim.args[0] = zt_pack_unsigned(123U, "arg0");
    claim.args[1] = zt_pack_integer(123, "arg1");
    claim.args[2] = zt_pack_integer(123, "arg2");
    claim.location.fname = "file.c";
    claim.location.lineno = 13;
    selftest_passing_verify3_called = false;
    claim.make_verifier = selftest_passing_verifier3;
    result = zt_verify_claim(&t, &claim);
    assert(result == false);
    assert(selftest_passing_verify3_called == false);
    selftest_stream_eq(t.stream, "file.c:13: arg[0] type mismatch\n");
    selftest_close_test(&t);
}

static void test_verify_mismatch_claim2of3(void)
{
    bool result;
    zt_claim claim;
    zt_test t = selftest_make_test();
    memset(&claim, 0, sizeof claim);
    claim.args[0] = zt_pack_integer(123, "arg0");
    claim.args[1] = zt_pack_unsigned(123U, "arg1");
    claim.args[2] = zt_pack_integer(123, "arg2");
    claim.location.fname = "file.c";
    claim.location.lineno = 13;
    selftest_passing_verify3_called = false;
    claim.make_verifier = selftest_passing_verifier3;
    result = zt_verify_claim(&t, &claim);
    assert(result == false);
    assert(selftest_passing_verify3_called == false);
    selftest_stream_eq(t.stream, "file.c:13: arg[1] type mismatch\n");
    selftest_close_test(&t);
}

static void test_verify_mismatch_claim3of3(void)
{
    bool result;
    zt_claim claim;
    zt_test t = selftest_make_test();
    memset(&claim, 0, sizeof claim);
    claim.args[0] = zt_pack_integer(123, "arg0");
    claim.args[1] = zt_pack_integer(123, "arg1");
    claim.args[2] = zt_pack_unsigned(123U, "arg2");
    claim.location.fname = "file.c";
    claim.location.lineno = 13;
    selftest_passing_verify3_called = false;
    claim.make_verifier = selftest_passing_verifier3;
    result = zt_verify_claim(&t, &claim);
    assert(result == false);
    assert(selftest_passing_verify3_called == false);
    selftest_stream_eq(t.stream, "file.c:13: arg[2] type mismatch\n");
    selftest_close_test(&t);
}

/* verifier for true and verify true. */

static void test_verifier_for_true(void)
{
    zt_verifier v = zt_verifier_for_true();
    assert(v.nargs == 1);
    assert(v.func.args1 == zt_verify_true);
    assert(v.arg_infos[0].kind == ZT_BOOLEAN);
    assert(strcmp(v.arg_infos[0].kind_mismatch_msg, "value is not a boolean") == 0);
    assert(v.arg_infos[1].kind == ZT_NOTHING);
    assert(v.arg_infos[1].kind_mismatch_msg == NULL);
    assert(v.arg_infos[2].kind == ZT_NOTHING);
    assert(v.arg_infos[2].kind_mismatch_msg == NULL);
}

static void test_verify_true(void)
{
    zt_test t;

    /* passing */
    t = selftest_make_test();
    assert(zt_verify_true(&t, zt_pack_boolean(true, "errno == EPERM")) == true);
    selftest_stream_eq(t.stream, "");
    selftest_close_test(&t);

    /* failing */
    t = selftest_make_test();
    assert(zt_verify_true(&t, zt_pack_boolean(false, "fd < 0")) == false);
    selftest_stream_eq(t.stream, "file.c:13: assertion failed because fd < 0 is false\n");
    selftest_close_test(&t);
}

/* verifier for false and verify false. */

static void test_verifier_for_false(void)
{
    zt_verifier v = zt_verifier_for_false();
    assert(v.nargs == 1);
    assert(v.func.args1 == zt_verify_false);
    assert(v.arg_infos[0].kind == ZT_BOOLEAN);
    assert(strcmp(v.arg_infos[0].kind_mismatch_msg, "value is not a boolean") == 0);
    assert(v.arg_infos[1].kind == ZT_NOTHING);
    assert(v.arg_infos[1].kind_mismatch_msg == NULL);
    assert(v.arg_infos[2].kind == ZT_NOTHING);
    assert(v.arg_infos[2].kind_mismatch_msg == NULL);
}

static void test_verify_false(void)
{
    zt_test t;

    /* passing */
    t = selftest_make_test();
    assert(zt_verify_false(&t, zt_pack_boolean(false, "errno == 0")) == true);
    selftest_stream_eq(t.stream, "");
    selftest_close_test(&t);

    /* failing */
    t = selftest_make_test();
    assert(zt_verify_false(&t, zt_pack_boolean(true, "errno == 0")) == false);
    selftest_stream_eq(t.stream, "file.c:13: assertion failed because errno == 0 is true\n");
    selftest_close_test(&t);
}

/* verifier for boolean relation. */

static void test_verifier_for_boolean_relation(void)
{
    zt_verifier v = zt_verifier_for_boolean_relation();
    assert(v.nargs == 3);
    assert(v.func.args3 == zt_verify_boolean_relation);
    assert(v.arg_infos[0].kind == ZT_BOOLEAN);
    assert(strcmp(v.arg_infos[0].kind_mismatch_msg,
               "left hand side is not a boolean")
        == 0);
    assert(v.arg_infos[1].kind == ZT_STRING);
    assert(strcmp(v.arg_infos[1].kind_mismatch_msg, "relation is not a string") == 0);
    assert(v.arg_infos[2].kind == ZT_BOOLEAN);
    assert(strcmp(v.arg_infos[2].kind_mismatch_msg,
               "right hand side is not a boolean")
        == 0);
}

static void test_verify_boolean_relation(void)
{
    zt_test t;

    /* passing == */
    t = selftest_make_test();
    assert(zt_verify_boolean_relation(&t, zt_pack_boolean(true, "L"),
        zt_pack_string("==", "=="), zt_pack_boolean(true, "R")));
    selftest_stream_eq(t.stream, "");
    selftest_close_test(&t);

    /* passing != */
    t = selftest_make_test();
    assert(zt_verify_boolean_relation(&t, zt_pack_boolean(true, "L"),
        zt_pack_string("!=", "!="), zt_pack_boolean(false, "R")));
    selftest_stream_eq(t.stream, "");
    selftest_close_test(&t);

    /* failing == */
    t = selftest_make_test();
    assert(!zt_verify_boolean_relation(&t, zt_pack_boolean(true, "L"),
        zt_pack_string("==", "=="), zt_pack_boolean(false, "R")));
    selftest_stream_eq(
        t.stream, "file.c:13: assertion L == R failed because true != false\n");
    selftest_close_test(&t);

    t = selftest_make_test();
    assert(!zt_verify_boolean_relation(&t, zt_pack_boolean(false, "L"),
        zt_pack_string("==", "=="), zt_pack_boolean(true, "R")));
    selftest_stream_eq(
        t.stream, "file.c:13: assertion L == R failed because false != true\n");
    selftest_close_test(&t);

    /* failing != */
    t = selftest_make_test();
    assert(!zt_verify_boolean_relation(&t, zt_pack_boolean(false, "L"),
        zt_pack_string("!=", "!="), zt_pack_boolean(false, "R")));
    selftest_stream_eq(
        t.stream, "file.c:13: assertion L != R failed because false == false\n");
    selftest_close_test(&t);

    /* unsupported relation */
    t = selftest_make_test();
    assert(!zt_verify_boolean_relation(&t, zt_pack_boolean(true, "L"),
        zt_pack_string("~", "~"), zt_pack_boolean(true, "R")));
    selftest_stream_eq(
        t.stream, "file.c:13: assertion L ~ R uses unsupported relation\n");
    selftest_close_test(&t);

    /* inconsistent relation */
    t = selftest_make_test();
    assert(!zt_verify_boolean_relation(&t, zt_pack_boolean(true, "L"),
        zt_pack_string("!=", "=="), zt_pack_boolean(true, "R")));
    selftest_stream_eq(
        t.stream, "file.c:13: L == R uses inconsistent relation !=\n");
    selftest_close_test(&t);
}

/* verifier for rune relation. */

static void test_verifier_for_rune_relation(void)
{
    zt_verifier v = zt_verifier_for_rune_relation();
    assert(v.nargs == 3);
    assert(v.func.args3 == zt_verify_rune_relation);
    assert(v.arg_infos[0].kind == ZT_RUNE);
    assert(strcmp(v.arg_infos[0].kind_mismatch_msg,
               "left hand side is not a rune")
        == 0);
    assert(v.arg_infos[1].kind == ZT_STRING);
    assert(strcmp(v.arg_infos[1].kind_mismatch_msg, "relation is not a string") == 0);
    assert(v.arg_infos[2].kind == ZT_RUNE);
    assert(strcmp(v.arg_infos[2].kind_mismatch_msg,
               "right hand side is not a rune")
        == 0);
}

static void test_verify_rune_relation(void)
{
    zt_test t;

    /* passing == */
    t = selftest_make_test();
    assert(zt_verify_rune_relation(&t, zt_pack_rune('a', "L"),
        zt_pack_string("==", "=="), zt_pack_rune('a', "R")));
    assert(zt_verify_rune_relation(&t, zt_pack_rune('\0', "L"),
        zt_pack_string("==", "=="), zt_pack_rune('\0', "R")));
    assert(zt_verify_rune_relation(&t, zt_pack_rune('\xFF', "L"),
        zt_pack_string("==", "=="), zt_pack_rune('\xFF', "R")));
    selftest_stream_eq(t.stream, "");
    selftest_close_test(&t);

    /* passing != */
    t = selftest_make_test();
    assert(zt_verify_rune_relation(&t, zt_pack_rune('a', "L"),
        zt_pack_string("!=", "!="), zt_pack_rune('b', "R")));
    selftest_stream_eq(t.stream, "");
    selftest_close_test(&t);

    /* passing <= */
    t = selftest_make_test();
    assert(zt_verify_rune_relation(&t, zt_pack_rune('a', "L"),
        zt_pack_string("<=", "<="), zt_pack_rune('a', "R")));
    assert(zt_verify_rune_relation(&t, zt_pack_rune('a', "L"),
        zt_pack_string("<=", "<="), zt_pack_rune('b', "R")));
    assert(zt_verify_rune_relation(&t, zt_pack_rune('\0', "L"),
        zt_pack_string("<=", "<="), zt_pack_rune('\xFF', "R")));
    selftest_stream_eq(t.stream, "");
    selftest_close_test(&t);

    /* passing < */
    t = selftest_make_test();
    assert(zt_verify_rune_relation(&t, zt_pack_rune('a', "L"),
        zt_pack_string("<", "<"), zt_pack_rune('b', "R")));
    assert(zt_verify_rune_relation(&t, zt_pack_rune('\0', "L"),
        zt_pack_string("<", "<"), zt_pack_rune('\xFF', "R")));
    selftest_stream_eq(t.stream, "");
    selftest_close_test(&t);

    /* passing >= */
    t = selftest_make_test();
    assert(zt_verify_rune_relation(&t, zt_pack_rune('\xFF', "L"),
        zt_pack_string(">=", ">="), zt_pack_rune('\xFF', "R")));
    assert(zt_verify_rune_relation(&t, zt_pack_rune('b', "L"),
        zt_pack_string(">=", ">="), zt_pack_rune('a', "R")));
    assert(zt_verify_rune_relation(&t, zt_pack_rune('b', "L"),
        zt_pack_string(">=", ">="), zt_pack_rune('b', "R")));
    selftest_stream_eq(t.stream, "");
    selftest_close_test(&t);

    /* passing > */
    t = selftest_make_test();
    assert(zt_verify_rune_relation(&t, zt_pack_rune('b', "L"),
        zt_pack_string(">", ">"), zt_pack_rune('a', "R")));
    assert(zt_verify_rune_relation(&t, zt_pack_rune('\xFF', "L"),
        zt_pack_string(">", ">"), zt_pack_rune('a', "R")));
    selftest_stream_eq(t.stream, "");
    selftest_close_test(&t);

    /* failing == */
    t = selftest_make_test();
    assert(!zt_verify_rune_relation(&t, zt_pack_rune('a', "L"),
        zt_pack_string("==", "=="), zt_pack_rune('b', "R")));
    selftest_stream_eq(
        t.stream, "file.c:13: assertion L == R failed because 'a' != 'b'\n");
    selftest_close_test(&t);

    /* failing != */
    t = selftest_make_test();
    assert(!zt_verify_rune_relation(&t, zt_pack_rune('a', "L"),
        zt_pack_string("!=", "!="), zt_pack_rune('a', "R")));
    selftest_stream_eq(
        t.stream, "file.c:13: assertion L != R failed because 'a' == 'a'\n");
    selftest_close_test(&t);

    /* failing < */
    t = selftest_make_test();
    assert(!zt_verify_rune_relation(&t, zt_pack_rune('b', "L"),
        zt_pack_string("<", "<"), zt_pack_rune('a', "R")));
    selftest_stream_eq(
        t.stream, "file.c:13: assertion L < R failed because 'b' >= 'a'\n");
    selftest_close_test(&t);

    /* failing <= */
    t = selftest_make_test();
    assert(!zt_verify_rune_relation(&t, zt_pack_rune('c', "L"),
        zt_pack_string("<=", "<="), zt_pack_rune('b', "R")));
    selftest_stream_eq(
        t.stream, "file.c:13: assertion L <= R failed because 'c' > 'b'\n");
    selftest_close_test(&t);

    /* failing > */
    t = selftest_make_test();
    assert(!zt_verify_rune_relation(&t, zt_pack_rune('a', "L"),
        zt_pack_string(">", ">"), zt_pack_rune('a', "R")));
    selftest_stream_eq(
        t.stream, "file.c:13: assertion L > R failed because 'a' <= 'a'\n");
    selftest_close_test(&t);

    /* failing >= */
    t = selftest_make_test();
    assert(!zt_verify_rune_relation(&t, zt_pack_rune('a', "L"),
        zt_pack_string(">=", ">="), zt_pack_rune('b', "R")));
    selftest_stream_eq(
        t.stream, "file.c:13: assertion L >= R failed because 'a' < 'b'\n");
    selftest_close_test(&t);

    /* unsupported relation */
    t = selftest_make_test();
    assert(!zt_verify_rune_relation(&t, zt_pack_rune('a', "L"),
        zt_pack_string("~", "~"), zt_pack_rune('b', "R")));
    selftest_stream_eq(
        t.stream, "file.c:13: assertion L ~ R uses unsupported relation\n");
    selftest_close_test(&t);

    /* inconsistent relation */
    t = selftest_make_test();
    assert(!zt_verify_rune_relation(&t, zt_pack_rune('a', "L"),
        zt_pack_string("==", "!="), zt_pack_rune('b', "R")));
    selftest_stream_eq(
        t.stream, "file.c:13: assertion L != R uses inconsistent relation ==\n");
    selftest_close_test(&t);
}

/* verifier for integer relation. */

static void test_verifier_for_integer_relation(void)
{
    zt_verifier v = zt_verifier_for_integer_relation();
    assert(v.nargs == 3);
    assert(v.func.args3 == zt_verify_integer_relation);
    assert(v.arg_infos[0].kind == ZT_INTEGER);
    assert(strcmp(v.arg_infos[0].kind_mismatch_msg,
               "left hand side is not an integer")
        == 0);
    assert(v.arg_infos[1].kind == ZT_STRING);
    assert(strcmp(v.arg_infos[1].kind_mismatch_msg, "relation is not a string") == 0);
    assert(v.arg_infos[2].kind == ZT_INTEGER);
    assert(strcmp(v.arg_infos[2].kind_mismatch_msg,
               "right hand side is not an integer")
        == 0);
}

static void test_verify_integer_relation(void)
{
    zt_test t;

    /* passing == */
    t = selftest_make_test();
    assert(zt_verify_integer_relation(&t, zt_pack_integer(1, "L"),
        zt_pack_string("==", "=="), zt_pack_integer(1, "R")));
    assert(zt_verify_integer_relation(&t, zt_pack_integer(INT_MAX, "L"),
        zt_pack_string("==", "=="), zt_pack_integer(INT_MAX, "R")));
    assert(zt_verify_integer_relation(&t, zt_pack_integer(INT_MIN, "L"),
        zt_pack_string("==", "=="), zt_pack_integer(INT_MIN, "R")));
    selftest_stream_eq(t.stream, "");
    selftest_close_test(&t);

    /* passing != */
    t = selftest_make_test();
    assert(zt_verify_integer_relation(&t, zt_pack_integer(0, "L"),
        zt_pack_string("!=", "!="), zt_pack_integer(-1, "R")));
    assert(zt_verify_integer_relation(&t, zt_pack_integer(INT_MIN, "L"),
        zt_pack_string("!=", "!="), zt_pack_integer(INT_MAX, "R")));
    selftest_stream_eq(t.stream, "");
    selftest_close_test(&t);

    /* passing <= */
    t = selftest_make_test();
    assert(zt_verify_integer_relation(&t, zt_pack_integer(-1, "L"),
        zt_pack_string("<=", "<="), zt_pack_integer(-1, "R")));
    assert(zt_verify_integer_relation(&t, zt_pack_integer(-1, "L"),
        zt_pack_string("<=", "<="), zt_pack_integer(0, "R")));
    assert(zt_verify_integer_relation(&t, zt_pack_integer(INT_MIN, "L"),
        zt_pack_string("<=", "<="), zt_pack_integer(INT_MAX, "R")));
    selftest_stream_eq(t.stream, "");
    selftest_close_test(&t);

    /* passing < */
    t = selftest_make_test();
    assert(zt_verify_integer_relation(&t, zt_pack_integer(-1, "L"),
        zt_pack_string("<", "<"), zt_pack_integer(0, "R")));
    assert(zt_verify_integer_relation(&t, zt_pack_integer(0, "L"),
        zt_pack_string("<", "<"), zt_pack_integer(1, "R")));
    assert(zt_verify_integer_relation(&t, zt_pack_integer(INT_MIN, "L"),
        zt_pack_string("<", "<"), zt_pack_integer(0, "R")));
    assert(zt_verify_integer_relation(&t, zt_pack_integer(INT_MIN, "L"),
        zt_pack_string("<", "<"), zt_pack_integer(INT_MAX, "R")));
    selftest_stream_eq(t.stream, "");
    selftest_close_test(&t);

    /* passing >= */
    t = selftest_make_test();
    assert(zt_verify_integer_relation(&t, zt_pack_integer(INT_MAX, "L"),
        zt_pack_string(">=", ">="), zt_pack_integer(INT_MAX, "R")));
    assert(zt_verify_integer_relation(&t, zt_pack_integer(INT_MAX, "L"),
        zt_pack_string(">=", ">="), zt_pack_integer(INT_MIN, "R")));
    assert(zt_verify_integer_relation(&t, zt_pack_integer(INT_MAX, "L"),
        zt_pack_string(">=", ">="), zt_pack_integer(INT_MAX, "R")));
    selftest_stream_eq(t.stream, "");
    selftest_close_test(&t);

    /* passing > */
    t = selftest_make_test();
    assert(zt_verify_integer_relation(&t, zt_pack_integer(2, "L"),
        zt_pack_string(">", ">"), zt_pack_integer(1, "R")));
    assert(zt_verify_integer_relation(&t, zt_pack_integer(INT_MAX, "L"),
        zt_pack_string(">", ">"), zt_pack_integer(INT_MIN, "R")));
    assert(zt_verify_integer_relation(&t, zt_pack_integer(INT_MAX, "L"),
        zt_pack_string(">", ">"), zt_pack_integer(0, "R")));
    selftest_stream_eq(t.stream, "");
    selftest_close_test(&t);

    /* failing == */
    t = selftest_make_test();
    assert(!zt_verify_integer_relation(&t, zt_pack_integer(1, "L"),
        zt_pack_string("==", "=="), zt_pack_integer(2, "R")));
    selftest_stream_eq(
        t.stream, "file.c:13: assertion L == R failed because 1 != 2\n");
    selftest_close_test(&t);

    /* failing != */
    t = selftest_make_test();
    assert(!zt_verify_integer_relation(&t, zt_pack_integer(1, "L"),
        zt_pack_string("!=", "!="), zt_pack_integer(1, "R")));
    selftest_stream_eq(
        t.stream, "file.c:13: assertion L != R failed because 1 == 1\n");
    selftest_close_test(&t);

    /* failing < */
    t = selftest_make_test();
    assert(!zt_verify_integer_relation(&t, zt_pack_integer(1, "L"),
        zt_pack_string("<", "<"), zt_pack_integer(1, "R")));
    selftest_stream_eq(
        t.stream, "file.c:13: assertion L < R failed because 1 >= 1\n");
    selftest_close_test(&t);

    /* failing <= */
    t = selftest_make_test();
    assert(!zt_verify_integer_relation(&t, zt_pack_integer(3, "L"),
        zt_pack_string("<=", "<="), zt_pack_integer(2, "R")));
    selftest_stream_eq(
        t.stream, "file.c:13: assertion L <= R failed because 3 > 2\n");
    selftest_close_test(&t);

    /* failing > */
    t = selftest_make_test();
    assert(!zt_verify_integer_relation(&t, zt_pack_integer(1, "L"),
        zt_pack_string(">", ">"), zt_pack_integer(1, "R")));
    selftest_stream_eq(
        t.stream, "file.c:13: assertion L > R failed because 1 <= 1\n");
    selftest_close_test(&t);

    /* failing >= */
    t = selftest_make_test();
    assert(!zt_verify_integer_relation(&t, zt_pack_integer(1, "L"),
        zt_pack_string(">=", ">="), zt_pack_integer(2, "R")));
    selftest_stream_eq(
        t.stream, "file.c:13: assertion L >= R failed because 1 < 2\n");
    selftest_close_test(&t);

    /* unsupported relation */
    t = selftest_make_test();
    assert(!zt_verify_integer_relation(&t, zt_pack_integer(1, "L"),
        zt_pack_string("~", "~"), zt_pack_integer(2, "R")));
    selftest_stream_eq(
        t.stream, "file.c:13: assertion L ~ R uses unsupported relation\n");
    selftest_close_test(&t);

    /* inconsistent relation */
    t = selftest_make_test();
    assert(!zt_verify_integer_relation(&t, zt_pack_integer(1, "L"),
        zt_pack_string("==", "!="), zt_pack_integer(2, "R")));
    selftest_stream_eq(
        t.stream, "file.c:13: assertion L != R uses inconsistent relation ==\n");
    selftest_close_test(&t);
}

/* verifier for unsigned relation. */

static void test_verifier_for_unsigned_relation(void)
{
    zt_verifier v = zt_verifier_for_unsigned_relation();
    assert(v.nargs == 3);
    assert(v.func.args3 == zt_verify_unsigned_relation);
    assert(v.arg_infos[0].kind == ZT_UNSIGNED);
    assert(strcmp(v.arg_infos[0].kind_mismatch_msg,
               "left hand side is not an unsigned integer")
        == 0);
    assert(v.arg_infos[1].kind == ZT_STRING);
    assert(strcmp(v.arg_infos[1].kind_mismatch_msg,
               "relation is not a string")
        == 0);
    assert(v.arg_infos[2].kind == ZT_UNSIGNED);
    assert(strcmp(v.arg_infos[2].kind_mismatch_msg,
               "right hand side is not an unsigned integer")
        == 0);
}

static void test_verify_unsigned_relation(void)
{
    zt_test t;

    /* passing == */
    t = selftest_make_test();
    assert(zt_verify_unsigned_relation(&t, zt_pack_unsigned(1U, "L"),
        zt_pack_string("==", "=="), zt_pack_unsigned(1U, "R")));
    assert(zt_verify_unsigned_relation(&t, zt_pack_unsigned(UINT_MAX, "L"),
        zt_pack_string("==", "=="), zt_pack_unsigned(UINT_MAX, "R")));
    selftest_stream_eq(t.stream, "");
    selftest_close_test(&t);

    /* passing != */
    t = selftest_make_test();
    assert(zt_verify_unsigned_relation(&t, zt_pack_unsigned(0U, "L"),
        zt_pack_string("!=", "!="), zt_pack_unsigned(1U, "R")));
    selftest_stream_eq(t.stream, "");
    selftest_close_test(&t);

    /* passing <= */
    t = selftest_make_test();
    assert(zt_verify_unsigned_relation(&t, zt_pack_unsigned(0U, "L"),
        zt_pack_string("<=", "<="), zt_pack_unsigned(0U, "R")));
    assert(zt_verify_unsigned_relation(&t, zt_pack_unsigned(0U, "L"),
        zt_pack_string("<=", "<="), zt_pack_unsigned(UINT_MAX, "R")));
    selftest_stream_eq(t.stream, "");
    selftest_close_test(&t);

    /* passing < */
    t = selftest_make_test();
    assert(zt_verify_unsigned_relation(&t, zt_pack_unsigned(0U, "L"),
        zt_pack_string("<", "<"), zt_pack_unsigned(1U, "R")));
    assert(zt_verify_unsigned_relation(&t, zt_pack_unsigned(0U, "L"),
        zt_pack_string("<", "<"), zt_pack_unsigned(UINT_MAX, "R")));
    selftest_stream_eq(t.stream, "");
    selftest_close_test(&t);

    /* passing >= */
    t = selftest_make_test();
    assert(zt_verify_unsigned_relation(&t, zt_pack_unsigned(0U, "L"),
        zt_pack_string(">=", ">="), zt_pack_unsigned(0U, "R")));
    assert(zt_verify_unsigned_relation(&t, zt_pack_unsigned(UINT_MAX, "L"),
        zt_pack_string(">=", ">="), zt_pack_unsigned(0U, "R")));
    assert(zt_verify_unsigned_relation(&t, zt_pack_unsigned(UINT_MAX, "L"),
        zt_pack_string(">=", ">="), zt_pack_unsigned(UINT_MAX, "R")));
    selftest_stream_eq(t.stream, "");
    selftest_close_test(&t);

    /* passing > */
    t = selftest_make_test();
    assert(zt_verify_unsigned_relation(&t, zt_pack_unsigned(2U, "L"),
        zt_pack_string(">", ">"), zt_pack_unsigned(1U, "R")));
    assert(zt_verify_unsigned_relation(&t, zt_pack_unsigned(UINT_MAX, "L"),
        zt_pack_string(">", ">"), zt_pack_unsigned(0U, "R")));
    assert(zt_verify_unsigned_relation(&t, zt_pack_unsigned(UINT_MAX, "L"),
        zt_pack_string(">", ">"), zt_pack_unsigned(0U, "R")));
    selftest_stream_eq(t.stream, "");
    selftest_close_test(&t);

    /* failing == */
    t = selftest_make_test();
    assert(!zt_verify_unsigned_relation(&t, zt_pack_unsigned(1U, "L"),
        zt_pack_string("==", "=="), zt_pack_unsigned(2U, "R")));
    selftest_stream_eq(
        t.stream, "file.c:13: assertion L == R failed because 1 != 2\n");
    selftest_close_test(&t);

    /* failing != */
    t = selftest_make_test();
    assert(!zt_verify_unsigned_relation(&t, zt_pack_unsigned(1U, "L"),
        zt_pack_string("!=", "!="), zt_pack_unsigned(1U, "R")));
    selftest_stream_eq(
        t.stream, "file.c:13: assertion L != R failed because 1 == 1\n");
    selftest_close_test(&t);

    /* failing < */
    t = selftest_make_test();
    assert(!zt_verify_unsigned_relation(&t, zt_pack_unsigned(1U, "L"),
        zt_pack_string("<", "<"), zt_pack_unsigned(1U, "R")));
    selftest_stream_eq(
        t.stream, "file.c:13: assertion L < R failed because 1 >= 1\n");
    selftest_close_test(&t);

    /* failing <= */
    t = selftest_make_test();
    assert(!zt_verify_unsigned_relation(&t, zt_pack_unsigned(3U, "L"),
        zt_pack_string("<=", "<="), zt_pack_unsigned(2U, "R")));
    selftest_stream_eq(
        t.stream, "file.c:13: assertion L <= R failed because 3 > 2\n");
    selftest_close_test(&t);

    /* failing > */
    t = selftest_make_test();
    assert(!zt_verify_unsigned_relation(&t, zt_pack_unsigned(1U, "L"),
        zt_pack_string(">", ">"), zt_pack_unsigned(1U, "R")));
    selftest_stream_eq(
        t.stream, "file.c:13: assertion L > R failed because 1 <= 1\n");
    selftest_close_test(&t);

    /* failing >= */
    t = selftest_make_test();
    assert(!zt_verify_unsigned_relation(&t, zt_pack_unsigned(1U, "L"),
        zt_pack_string(">=", ">="), zt_pack_unsigned(2U, "R")));
    selftest_stream_eq(
        t.stream, "file.c:13: assertion L >= R failed because 1 < 2\n");
    selftest_close_test(&t);

    /* unsupported relation */
    t = selftest_make_test();
    assert(!zt_verify_unsigned_relation(&t, zt_pack_unsigned(1U, "L"),
        zt_pack_string("~", "~"), zt_pack_unsigned(2U, "R")));
    selftest_stream_eq(
        t.stream, "file.c:13: assertion L ~ R uses unsupported relation\n");
    selftest_close_test(&t);

    /* inconsistent relation */
    t = selftest_make_test();
    assert(!zt_verify_unsigned_relation(&t, zt_pack_unsigned(1U, "L"),
        zt_pack_string("==", "!="), zt_pack_unsigned(2U, "R")));
    selftest_stream_eq(
        t.stream, "file.c:13: assertion L != R uses inconsistent relation ==\n");
    selftest_close_test(&t);
}

/* verifier for string relation. */

static void test_verifier_for_string_relation(void)
{
    zt_verifier v = zt_verifier_for_string_relation();
    assert(v.nargs == 3);
    assert(v.func.args3 == zt_verify_string_relation);
    assert(v.arg_infos[0].kind == ZT_STRING);
    assert(strcmp(v.arg_infos[0].kind_mismatch_msg,
               "left hand side is not a string")
        == 0);
    assert(v.arg_infos[1].kind == ZT_STRING);
    assert(strcmp(v.arg_infos[1].kind_mismatch_msg, "relation is not a string") == 0);
    assert(v.arg_infos[2].kind == ZT_STRING);
    assert(strcmp(v.arg_infos[2].kind_mismatch_msg,
               "right hand side is not a string")
        == 0);
}

static void test_verify_string_relation(void)
{
    zt_test t;

    /* NULL on the left side */
    t = selftest_make_test();
    assert(zt_verify_string_relation(&t, zt_pack_string(NULL, "left"),
               zt_pack_string("==", "=="),
               zt_pack_string("abc", "right"))
        == false);
    selftest_stream_eq(
        t.stream, "file.c:13: assertion left == right failed because left hand side is NULL\n");
    selftest_close_test(&t);

    /* NULL on the right side */
    t = selftest_make_test();
    assert(zt_verify_string_relation(&t, zt_pack_string("abc", "left"),
               zt_pack_string("==", "=="),
               zt_pack_string(NULL, "right"))
        == false);
    selftest_stream_eq(
        t.stream, "file.c:13: assertion left == right failed because right hand side is NULL\n");
    selftest_close_test(&t);

    /* passing == */
    t = selftest_make_test();
    assert(zt_verify_string_relation(&t, zt_pack_string("abc", "left"),
               zt_pack_string("==", "=="),
               zt_pack_string("abc", "right"))
        == true);
    selftest_stream_eq(t.stream, "");
    selftest_close_test(&t);

    /* failing == */
    t = selftest_make_test();
    assert(zt_verify_string_relation(&t, zt_pack_string("abc", "left"),
               zt_pack_string("==", "=="),
               zt_pack_string("xyz", "right"))
        == false);
    selftest_stream_eq(
        t.stream, "file.c:13: assertion left == right failed because \"abc\" != \"xyz\"\n");
    selftest_close_test(&t);

    /* passing != */
    t = selftest_make_test();
    assert(zt_verify_string_relation(&t, zt_pack_string("abc", "left"),
               zt_pack_string("!=", "!="),
               zt_pack_string("xyz", "right"))
        == true);
    selftest_stream_eq(t.stream, "");
    selftest_close_test(&t);

    /* failing != */
    t = selftest_make_test();
    assert(zt_verify_string_relation(&t, zt_pack_string("abc", "left"),
               zt_pack_string("!=", "!="),
               zt_pack_string("abc", "right"))
        == false);
    selftest_stream_eq(
        t.stream, "file.c:13: assertion left != right failed because \"abc\" == \"abc\"\n");
    selftest_close_test(&t);

    /* passing < */
    t = selftest_make_test();
    assert(zt_verify_string_relation(&t, zt_pack_string("abc", "left"),
               zt_pack_string("<", "<"),
               zt_pack_string("xyz", "right"))
        == true);
    selftest_stream_eq(t.stream, "");
    selftest_close_test(&t);

    /* failing < */
    t = selftest_make_test();
    assert(zt_verify_string_relation(&t, zt_pack_string("abc", "left"),
               zt_pack_string("<", "<"),
               zt_pack_string("abc", "right"))
        == false);
    selftest_stream_eq(
        t.stream, "file.c:13: assertion left < right failed because \"abc\" >= \"abc\"\n");
    selftest_close_test(&t);

    t = selftest_make_test();
    assert(zt_verify_string_relation(&t, zt_pack_string("xyz", "left"),
               zt_pack_string("<", "<"),
               zt_pack_string("abc", "right"))
        == false);
    selftest_stream_eq(
        t.stream, "file.c:13: assertion left < right failed because \"xyz\" >= \"abc\"\n");
    selftest_close_test(&t);

    /* passing <= */
    t = selftest_make_test();
    assert(zt_verify_string_relation(&t, zt_pack_string("abc", "left"),
               zt_pack_string("<=", "<="),
               zt_pack_string("xyz", "right"))
        == true);
    selftest_stream_eq(t.stream, "");
    assert(zt_verify_string_relation(&t, zt_pack_string("abc", "left"),
               zt_pack_string("<=", "<="),
               zt_pack_string("abc", "right"))
        == true);
    selftest_stream_eq(t.stream, "");
    selftest_close_test(&t);

    /* failing <= */
    t = selftest_make_test();
    assert(zt_verify_string_relation(&t, zt_pack_string("xyz", "left"),
               zt_pack_string("<=", "<="),
               zt_pack_string("abc", "right"))
        == false);
    selftest_stream_eq(
        t.stream, "file.c:13: assertion left <= right failed because \"xyz\" > \"abc\"\n");
    selftest_close_test(&t);

    /* passing > */
    t = selftest_make_test();
    assert(zt_verify_string_relation(&t, zt_pack_string("xyz", "left"),
               zt_pack_string(">", ">"),
               zt_pack_string("abc", "right"))
        == true);
    selftest_stream_eq(t.stream, "");
    selftest_close_test(&t);

    /* failing > */
    t = selftest_make_test();
    assert(zt_verify_string_relation(&t, zt_pack_string("abc", "left"),
               zt_pack_string(">", ">"),
               zt_pack_string("xyz", "right"))
        == false);
    selftest_stream_eq(
        t.stream, "file.c:13: assertion left > right failed because \"abc\" <= \"xyz\"\n");
    selftest_close_test(&t);

    /* passing >= */
    t = selftest_make_test();
    assert(zt_verify_string_relation(&t, zt_pack_string("xyz", "left"),
               zt_pack_string(">=", ">="),
               zt_pack_string("abc", "right"))
        == true);
    selftest_stream_eq(t.stream, "");
    assert(zt_verify_string_relation(&t, zt_pack_string("abc", "left"),
               zt_pack_string(">=", ">="),
               zt_pack_string("abc", "right"))
        == true);
    selftest_stream_eq(t.stream, "");
    selftest_close_test(&t);

    /* failing >= */
    t = selftest_make_test();
    assert(zt_verify_string_relation(&t, zt_pack_string("abc", "left"),
               zt_pack_string(">=", ">="),
               zt_pack_string("xyz", "right"))
        == false);
    selftest_stream_eq(
        t.stream, "file.c:13: assertion left >= right failed because \"abc\" < \"xyz\"\n");
    selftest_close_test(&t);

    /* unsupported relation */
    t = selftest_make_test();
    assert(zt_verify_string_relation(&t, zt_pack_string("abc", "left"),
               zt_pack_string("~", "~"),
               zt_pack_string("xyz", "right"))
        == false);
    selftest_stream_eq(
        t.stream, "file.c:13: assertion left ~ right uses unsupported relation\n");
    selftest_close_test(&t);

    /* inconsistent relation */
    t = selftest_make_test();
    assert(zt_verify_string_relation(&t, zt_pack_string("abc", "left"),
               zt_pack_string("==", "!="),
               zt_pack_string("xyz", "right"))
        == false);
    selftest_stream_eq(
        t.stream, "file.c:13: assertion left != right uses inconsistent relation ==\n");
    selftest_close_test(&t);
}

/* verifier for null. */

static void test_verifier_for_null(void)
{
    zt_verifier v = zt_verifier_for_null();
    assert(v.nargs == 1);
    assert(v.func.args1 == zt_verify_null);
    assert(v.arg_infos[0].kind == ZT_POINTER);
    assert(strcmp(v.arg_infos[0].kind_mismatch_msg,
               "argument is not a pointer")
        == 0);
    assert(v.arg_infos[1].kind == ZT_NOTHING);
    assert(v.arg_infos[1].kind_mismatch_msg == NULL);
    assert(v.arg_infos[2].kind == ZT_NOTHING);
    assert(v.arg_infos[2].kind_mismatch_msg == NULL);
}

static void test_verify_null(void)
{
    zt_test t;
    void* p;

    /* passing NULL */
    p = NULL;
    t = selftest_make_test();
    assert(zt_verify_null(&t, zt_pack_pointer(p, "p")) == true);
    selftest_stream_eq(t.stream, "");
    selftest_close_test(&t);

    /* failing non-NULL */
    p = &p;
    t = selftest_make_test();
    assert(zt_verify_null(&t, zt_pack_pointer(p, "p")) == false);
    selftest_stream_eq_at(
        t.stream, __FILE__, __LINE__, "file.c:13: assertion p == NULL failed because %p != NULL\n", p);
    selftest_close_test(&t);
}

/* verifier for not-null. */

static void test_verifier_for_not_null(void)
{
    zt_verifier v = zt_verifier_for_not_null();
    assert(v.nargs == 1);
    assert(v.func.args1 == zt_verify_not_null);
    assert(v.arg_infos[0].kind == ZT_POINTER);
    assert(strcmp(v.arg_infos[0].kind_mismatch_msg,
               "argument is not a pointer")
        == 0);
    assert(v.arg_infos[1].kind == ZT_NOTHING);
    assert(v.arg_infos[1].kind_mismatch_msg == NULL);
    assert(v.arg_infos[2].kind == ZT_NOTHING);
    assert(v.arg_infos[2].kind_mismatch_msg == NULL);
}

static void test_verify_not_null(void)
{
    zt_test t;
    void* p;

    /* passing non-NULL */
    p = &p;
    t = selftest_make_test();
    assert(zt_verify_not_null(&t, zt_pack_pointer(p, "p")) == true);
    selftest_stream_eq(t.stream, "");
    selftest_close_test(&t);

    /* failing NULL */
    p = NULL;
    t = selftest_make_test();
    assert(zt_verify_not_null(&t, zt_pack_pointer(p, "p")) == false);
    selftest_stream_eq(t.stream, "file.c:13: assertion p != NULL failed\n");
    selftest_close_test(&t);
}

/* all the claim macros */

static void test_ZT_TRUE(void)
{
    zt_claim claim = ZT_TRUE(1 > 0);
    assert(strcmp(claim.location.fname, __FILE__) == 0);
    assert(claim.location.lineno == __LINE__ - 2);
    assert(claim.make_verifier == zt_verifier_for_true);
    assert(zt_value_kind_of(claim.args[0]) == ZT_BOOLEAN);
    assert(claim.args[0].as.boolean == true);
    assert(strcmp(zt_source_of(claim.args[0]), "1 > 0") == 0);
}

static void test_ZT_FALSE(void)
{
    zt_claim claim = ZT_FALSE(0 < 1);
    assert(strcmp(claim.location.fname, __FILE__) == 0);
    assert(claim.location.lineno == __LINE__ - 2);
    assert(claim.make_verifier == zt_verifier_for_false);
    assert(zt_value_kind_of(claim.args[0]) == ZT_BOOLEAN);
    assert(claim.args[0].as.boolean == true);
    assert(strcmp(zt_source_of(claim.args[0]), "0 < 1") == 0);
}

static void test_ZT_CMP_BOOL(void)
{
    zt_claim claim = ZT_CMP_BOOL(true, ==, 1 == 1);
    assert(strcmp(claim.location.fname, __FILE__) == 0);
    assert(claim.location.lineno == __LINE__ - 2);
    assert(claim.make_verifier == zt_verifier_for_boolean_relation);

    assert(zt_value_kind_of(claim.args[0]) == ZT_BOOLEAN);
    assert(claim.args[0].as.boolean == true);
    assert(strcmp(zt_source_of(claim.args[0]), "true") == 0);

    assert(zt_value_kind_of(claim.args[1]) == ZT_STRING);
    assert(strcmp(claim.args[1].as.string, "==") == 0);
    assert(strcmp(zt_source_of(claim.args[1]), "==") == 0);

    assert(zt_value_kind_of(claim.args[2]) == ZT_BOOLEAN);
    assert(claim.args[2].as.boolean == true);
    assert(strcmp(zt_source_of(claim.args[2]), "1 == 1") == 0);
}

static void test_ZT_CMP_RUNE(void)
{
    char a, b;
    zt_claim claim;

    a = 'a';
    b = 'b';
    claim = ZT_CMP_RUNE(a, ==, b);
    assert(strcmp(claim.location.fname, __FILE__) == 0);
    assert(claim.location.lineno == __LINE__ - 2);
    assert(claim.make_verifier == zt_verifier_for_rune_relation);

    assert(zt_value_kind_of(claim.args[0]) == ZT_RUNE);
    assert(claim.args[0].as.integer == 'a');
    assert(strcmp(zt_source_of(claim.args[0]), "a") == 0);

    assert(zt_value_kind_of(claim.args[1]) == ZT_STRING);
    assert(strcmp(claim.args[1].as.string, "==") == 0);
    assert(strcmp(zt_source_of(claim.args[1]), "==") == 0);

    assert(zt_value_kind_of(claim.args[2]) == ZT_RUNE);
    assert(claim.args[2].as.integer == 'b');
    assert(strcmp(zt_source_of(claim.args[2]), "b") == 0);
}

static void test_ZT_CMP_INT(void)
{
    int a, b;
    zt_claim claim;

    a = 1;
    b = -2;
    claim = ZT_CMP_INT(a, ==, b);
    assert(strcmp(claim.location.fname, __FILE__) == 0);
    assert(claim.location.lineno == __LINE__ - 2);
    assert(claim.make_verifier == zt_verifier_for_integer_relation);

    assert(zt_value_kind_of(claim.args[0]) == ZT_INTEGER);
    assert(claim.args[0].as.integer == 1);
    assert(strcmp(zt_source_of(claim.args[0]), "a") == 0);

    assert(zt_value_kind_of(claim.args[1]) == ZT_STRING);
    assert(strcmp(claim.args[1].as.string, "==") == 0);
    assert(strcmp(zt_source_of(claim.args[1]), "==") == 0);

    assert(zt_value_kind_of(claim.args[2]) == ZT_INTEGER);
    assert(claim.args[2].as.integer == -2);
    assert(strcmp(zt_source_of(claim.args[2]), "b") == 0);
}

static void test_ZT_CMP_UINT(void)
{
    unsigned int a, b;
    zt_claim claim;

    a = 1;
    b = 2;
    claim = ZT_CMP_UINT(a, ==, b);
    assert(strcmp(claim.location.fname, __FILE__) == 0);
    assert(claim.location.lineno == __LINE__ - 2);
    assert(claim.make_verifier == zt_verifier_for_unsigned_relation);

    assert(zt_value_kind_of(claim.args[0]) == ZT_UNSIGNED);
    assert(claim.args[0].as.unsigned_integer == 1);
    assert(strcmp(zt_source_of(claim.args[0]), "a") == 0);

    assert(zt_value_kind_of(claim.args[1]) == ZT_STRING);
    assert(strcmp(claim.args[1].as.string, "==") == 0);
    assert(strcmp(zt_source_of(claim.args[1]), "==") == 0);

    assert(zt_value_kind_of(claim.args[2]) == ZT_UNSIGNED);
    assert(claim.args[2].as.unsigned_integer == 2);
    assert(strcmp(zt_source_of(claim.args[2]), "b") == 0);
}

static void test_ZT_CMP_CSTR(void)
{
    const char *a, *b;
    zt_claim claim;

    a = "foo";
    b = "bar";
    claim = ZT_CMP_CSTR(a, ==, b);
    assert(strcmp(claim.location.fname, __FILE__) == 0);
    assert(claim.location.lineno == __LINE__ - 2);
    assert(claim.make_verifier == zt_verifier_for_string_relation);

    assert(zt_value_kind_of(claim.args[0]) == ZT_STRING);
    assert(strcmp(claim.args[0].as.string, "foo") == 0);
    assert(strcmp(zt_source_of(claim.args[0]), "a") == 0);

    assert(zt_value_kind_of(claim.args[1]) == ZT_STRING);
    assert(strcmp(claim.args[1].as.string, "==") == 0);
    assert(strcmp(zt_source_of(claim.args[1]), "==") == 0);

    assert(zt_value_kind_of(claim.args[2]) == ZT_STRING);
    assert(strcmp(claim.args[2].as.string, "bar") == 0);
    assert(strcmp(zt_source_of(claim.args[2]), "b") == 0);
}

static void test_ZT_NULL(void)
{
    void* p = NULL;
    zt_claim claim = ZT_NULL(p);
    assert(strcmp(claim.location.fname, __FILE__) == 0);
    assert(claim.location.lineno == __LINE__ - 2);
    assert(claim.make_verifier == zt_verifier_for_null);

    assert(zt_value_kind_of(claim.args[0]) == ZT_POINTER);
    assert(claim.args[0].as.pointer == NULL);
    assert(strcmp(zt_source_of(claim.args[0]), "p") == 0);

    assert(zt_value_kind_of(claim.args[1]) == ZT_NOTHING);
    assert(zt_source_of(claim.args[1]) == NULL);

    assert(zt_value_kind_of(claim.args[2]) == ZT_NOTHING);
    assert(zt_source_of(claim.args[2]) == NULL);
}

static void test_ZT_NOT_NULL(void)
{
    void* p = &p;
    zt_claim claim = ZT_NOT_NULL(p);
    assert(strcmp(claim.location.fname, __FILE__) == 0);
    assert(claim.location.lineno == __LINE__ - 2);
    assert(claim.make_verifier == zt_verifier_for_not_null);

    assert(zt_value_kind_of(claim.args[0]) == ZT_POINTER);
    assert(claim.args[0].as.pointer == &p);
    assert(strcmp(zt_source_of(claim.args[0]), "p") == 0);

    assert(zt_value_kind_of(claim.args[1]) == ZT_NOTHING);
    assert(zt_source_of(claim.args[1]) == NULL);

    assert(zt_value_kind_of(claim.args[2]) == ZT_NOTHING);
    assert(zt_source_of(claim.args[2]) == NULL);
}

static void test_quote_string(void)
{
    FILE* f = selftest_temporary_file();
    zt_quote_string(f, "abc \n\r\t\v\b double:\" single:' hex:\x07 123");
    selftest_stream_eq(f, "\"abc \\n\\r\\t\\v\\b double:\\\" single:' hex:\\0x07 123\"");
    fclose(f);
}

static void test_quote_rune(void)
{
    FILE* f = selftest_temporary_file();
    zt_quote_rune(f, '"');
    selftest_stream_eq(f, "'\"'");
    fclose(f);

    f = selftest_temporary_file();
    zt_quote_rune(f, '\'');
    selftest_stream_eq(f, "'\\\''");
    fclose(f);
}

static bool selftest_stub_nested_test_case_visited;
static void selftest_stub_nested_test_case(zt_t t)
{
    assert(t != NULL);
    selftest_stub_nested_test_case_visited = true;
}

static bool selftest_stub_nested_test_suite_visited;
static void selftest_stub_nested_test_suite(zt_visitor visitor)
{
    assert(visitor.id != NULL);
    assert(visitor.vtab != NULL);
    selftest_stub_nested_test_suite_visited = true;

    ZT_VISIT_TEST_CASE(visitor, selftest_stub_nested_test_case);
}

static bool selftest_stub_test_case_visited;
static void selftest_stub_test_case(zt_t t)
{
    assert(t != NULL);
    selftest_stub_test_case_visited = true;
}

static bool selftest_stub_test_suite_visited;
static void selftest_stub_test_suite(zt_visitor visitor)
{
    assert(visitor.id != NULL);
    assert(visitor.vtab != NULL);
    selftest_stub_test_suite_visited = true;

    ZT_VISIT_TEST_CASE(visitor, selftest_stub_test_case);
    ZT_VISIT_TEST_SUITE(visitor, selftest_stub_nested_test_suite);
}

static void test_lister_visitor_visit_suite(void)
{
    FILE* f = selftest_temporary_file();
    zt_test_lister lister;
    zt_visitor visitor;
    memset(&lister, 0, sizeof lister);
    lister.stream = f;
    visitor = zt_visitor_from_test_lister(&lister);
    selftest_stub_nested_test_suite_visited = false;
    selftest_stub_nested_test_case_visited = false;
    selftest_stub_test_suite_visited = false;
    selftest_stub_test_case_visited = false;
    ZT_VISIT_TEST_SUITE(visitor, selftest_stub_test_suite);
    /* test suites are visited, test cases are not. */
    assert(selftest_stub_test_suite_visited == true);
    assert(selftest_stub_nested_test_suite_visited == true);
    assert(selftest_stub_test_case_visited == false);
    assert(selftest_stub_nested_test_case_visited == false);

    selftest_stream_eq(
        f, ""
           "- selftest_stub_test_suite\n"
           "  - selftest_stub_test_case\n"
           "  - selftest_stub_nested_test_suite\n"
           "     - selftest_stub_nested_test_case\n");
    fclose(f);
}

static void test_runner_visitor_visit_suite(void)
{
    FILE* stream_out = selftest_temporary_file();
    FILE* stream_err = selftest_temporary_file();
    zt_test_runner runner;
    zt_visitor visitor;
    memset(&runner, 0, sizeof runner);
    runner.stream_out = stream_out;
    runner.stream_err = stream_err;
    visitor = zt_visitor_from_test_runner(&runner);
    selftest_stub_nested_test_suite_visited = false;
    selftest_stub_nested_test_case_visited = false;
    selftest_stub_test_suite_visited = false;
    selftest_stub_test_case_visited = false;
    ZT_VISIT_TEST_SUITE(visitor, selftest_stub_test_suite);
    /* test suites and test cases are all visited. */
    assert(selftest_stub_test_suite_visited == true);
    assert(selftest_stub_nested_test_suite_visited == true);
    assert(selftest_stub_test_case_visited == true);
    assert(selftest_stub_nested_test_case_visited == true);

    selftest_stream_eq(stream_out, "");
    selftest_stream_eq(stream_err, "");
    fclose(stream_out);
    fclose(stream_err);
}

static bool selftest_case_pending_visited;
static void selftest_case_pending(zt_t t)
{
    selftest_case_pending_visited = true;
    assert(t->outcome == ZT_PENDING);
}

static void test_runner_visitor_visit_case_outcome_pending(void)
{
    FILE* stream_out = selftest_temporary_file();
    FILE* stream_err = selftest_temporary_file();
    zt_test_runner runner;
    zt_visitor visitor;
    memset(&runner, 0, sizeof runner);
    runner.stream_out = stream_out;
    runner.stream_err = stream_err;
    visitor = zt_visitor_from_test_runner(&runner);
    selftest_case_pending_visited = false;
    ZT_VISIT_TEST_CASE(visitor, selftest_case_pending);
    assert(selftest_case_pending_visited == true);

    selftest_stream_eq(stream_out, "");
    selftest_stream_eq(stream_err, "");
    assert(runner.num_passed == 1);
    assert(runner.num_failed == 0);
    fclose(stream_out);
    fclose(stream_err);
}

static bool selftest_case_passed_visited;
static void selftest_case_passed(zt_t t)
{
    selftest_case_passed_visited = true;
    t->outcome = ZT_PASSED;
}

static void test_runner_visitor_visit_case_outcome_passed(void)
{
    FILE* stream_out = selftest_temporary_file();
    FILE* stream_err = selftest_temporary_file();
    zt_test_runner runner;
    zt_visitor visitor;
    memset(&runner, 0, sizeof runner);
    runner.stream_out = stream_out;
    runner.stream_err = stream_err;
    visitor = zt_visitor_from_test_runner(&runner);
    selftest_case_passed_visited = false;
    ZT_VISIT_TEST_CASE(visitor, selftest_case_passed);
    assert(selftest_case_passed_visited == true);

    selftest_stream_eq(stream_out, "");
    selftest_stream_eq(stream_err, "");
    assert(runner.num_passed == 1);
    assert(runner.num_failed == 0);
    fclose(stream_out);
    fclose(stream_err);
}

static bool selftest_case_failed_visited;
static void selftest_case_failed(zt_t t)
{
    selftest_case_failed_visited = true;
    t->outcome = ZT_FAILED;
}

static void test_runner_visitor_visit_case_outcome_failed(void)
{
    FILE* stream_out = selftest_temporary_file();
    FILE* stream_err = selftest_temporary_file();
    zt_test_runner runner;
    zt_visitor visitor;
    memset(&runner, 0, sizeof runner);
    runner.stream_out = stream_out;
    runner.stream_err = stream_err;
    visitor = zt_visitor_from_test_runner(&runner);
    selftest_case_failed_visited = false;
    ZT_VISIT_TEST_CASE(visitor, selftest_case_failed);
    assert(selftest_case_failed_visited == true);

    selftest_stream_eq(stream_out, "");
    selftest_stream_eq(stream_err, "");
    assert(runner.num_passed == 0);
    assert(runner.num_failed == 1);
    fclose(stream_out);
    fclose(stream_err);
}

static bool selftest_case_bogus_outcome_visited;
static void selftest_case_bogus_outcome(zt_t t)
{
    selftest_case_bogus_outcome_visited = true;
    t->outcome = 42;
}

static void test_runner_visitor_visit_case_outcome_bogus(void)
{
    FILE* stream_out = selftest_temporary_file();
    FILE* stream_err = selftest_temporary_file();
    zt_test_runner runner;
    zt_visitor visitor;

    memset(&runner, 0, sizeof runner);
    runner.stream_out = stream_out;
    runner.stream_err = stream_err;
    visitor = zt_visitor_from_test_runner(&runner);
    selftest_case_bogus_outcome_visited = false;
    ZT_VISIT_TEST_CASE(visitor, selftest_case_bogus_outcome);
    assert(selftest_case_bogus_outcome_visited == true);

    selftest_stream_eq(stream_out, "");
    selftest_stream_eq(stream_err, "- selftest_case_bogus_outcome - unexpected outcome code 42\n");
    assert(runner.num_passed == 0);
    assert(runner.num_failed == 1);
    fclose(stream_out);
    fclose(stream_err);
}

static void test_main_listing_tests(void)
{
    char* test_argv[] = { "a.out", "-l" };
    int exit_code;

    zt_mock_stdout = selftest_temporary_file();
    zt_mock_stderr = selftest_temporary_file();

    exit_code = zt_main(2, test_argv, NULL, selftest_stub_test_suite);
    assert(exit_code == 0);
    selftest_stream_eq(
        zt_mock_stdout, ""
                        "- selftest_stub_test_case\n"
                        "- selftest_stub_nested_test_suite\n"
                        "  - selftest_stub_nested_test_case\n");
    selftest_stream_eq(
        zt_mock_stderr, "");
    fclose(zt_mock_stdout);
    fclose(zt_mock_stderr);
    zt_mock_stdout = NULL;
    zt_mock_stderr = NULL;
}

static void selftest_passing_check(zt_t t)
{
    zt_check(t, ZT_TRUE(1));
    zt_check(t, ZT_CMP_INT(1, ==, 1));
}

static void selftest_failing_check(zt_t t)
{
    zt_check(t, ZT_TRUE(0));
    zt_check(t, ZT_CMP_INT(1, !=, 1));
}

static void selftest_passing_assert(zt_t t)
{
    zt_assert(t, ZT_TRUE(1));
}

static void selftest_failing_assert(zt_t t)
{
    zt_assert(t, ZT_TRUE(0));
}

static void selftest_empty_suite(ZT_UNUSED zt_visitor v)
{
}

static void selftest_passing_suite(zt_visitor v)
{
    ZT_VISIT_TEST_CASE(v, selftest_passing_check);
    ZT_VISIT_TEST_CASE(v, selftest_passing_assert);
    ZT_VISIT_TEST_SUITE(v, selftest_empty_suite);
}

static void selftest_failing_suite(zt_visitor v)
{
    ZT_VISIT_TEST_CASE(v, selftest_failing_check);
    ZT_VISIT_TEST_CASE(v, selftest_failing_assert);
    ZT_VISIT_TEST_SUITE(v, selftest_empty_suite);
}

static void test_main_running_passing_tests(void)
{
    char* test_argv[] = { "a.out" };
    int exit_code;

    zt_mock_stdout = selftest_temporary_file();
    zt_mock_stderr = selftest_temporary_file();

    exit_code = zt_main(1, test_argv, NULL, selftest_passing_suite);
    assert(exit_code == EXIT_SUCCESS);
    selftest_stream_eq(zt_mock_stdout, "");
    selftest_stream_eq(zt_mock_stderr, "");
    fclose(zt_mock_stdout);
    fclose(zt_mock_stderr);
    zt_mock_stdout = NULL;
    zt_mock_stderr = NULL;
}

static void test_main_verbosely_running_passing_tests(void)
{
    char* test_argv[] = { "a.out", "-v" };
    int exit_code;

    zt_mock_stdout = selftest_temporary_file();
    zt_mock_stderr = selftest_temporary_file();

    exit_code = zt_main(2, test_argv, NULL, selftest_passing_suite);
    assert(exit_code == EXIT_SUCCESS);
    selftest_stream_eq(zt_mock_stdout, ""
                                       "- selftest_passing_check\n"
                                       "- selftest_passing_assert\n"
                                       "- selftest_empty_suite\n");
    selftest_stream_eq(zt_mock_stderr, "");
    fclose(zt_mock_stdout);
    fclose(zt_mock_stderr);
    zt_mock_stdout = NULL;
    zt_mock_stderr = NULL;
}

static void test_main_running_failing_tests(void)
{
    char* test_argv[] = { "a.out" };
    int exit_code;

    zt_mock_stdout = selftest_temporary_file();
    zt_mock_stderr = selftest_temporary_file();

    exit_code = zt_main(1, test_argv, NULL, selftest_failing_suite);
    assert(exit_code == EXIT_FAILURE);
    selftest_stream_eq(zt_mock_stdout, "");
    selftest_stream_eq_at(
        zt_mock_stderr, __FILE__, __LINE__,
        "%s:%d: assertion failed because 0 is false\n"
        "%s:%d: assertion 1 != 1 failed because 1 == 1\n"
        "%s:%d: assertion failed because 0 is false\n",
        __FILE__, __LINE__ - 84 - 3, __FILE__, __LINE__ - 83 - 3, __FILE__, __LINE__ - 73 - 3);
    fclose(zt_mock_stdout);
    fclose(zt_mock_stderr);
    zt_mock_stdout = NULL;
    zt_mock_stderr = NULL;
}

static void test_stdout_stderr(void)
{
    assert(zt_stdout() == stdout);
    assert(zt_stderr() == stderr);
}

int main(ZT_UNUSED int argc, ZT_UNUSED char** argv, ZT_UNUSED char** envp)
{
    test_MAJOR_MINOR_VERSION();

    test_pack_boolean();
    test_pack_rune();
    test_pack_integer();
    test_pack_unsigned();
    test_pack_string();
    test_pack_pointer();

    test_find_binary_relation();
    test_invert_binary_relation();
    test_binary_relation_as_text();

    test_boolean_as_text();

    test_verify_claim0();
    test_verify_claim1();
    test_verify_claim2();
    test_verify_claim3();
    test_verify_bogus_claim4();
    test_verify_mismatch_claim1of1();
    test_verify_mismatch_claim1of2();
    test_verify_mismatch_claim2of2();
    test_verify_mismatch_claim1of3();
    test_verify_mismatch_claim2of3();
    test_verify_mismatch_claim3of3();

    test_verifier_for_true();
    test_verify_true();

    test_verifier_for_false();
    test_verify_false();

    test_verifier_for_boolean_relation();
    test_verify_boolean_relation();

    test_verifier_for_rune_relation();
    test_verify_rune_relation();

    test_verifier_for_integer_relation();
    test_verify_integer_relation();

    test_verifier_for_unsigned_relation();
    test_verify_unsigned_relation();

    test_verifier_for_string_relation();
    test_verify_string_relation();

    test_verifier_for_null();
    test_verify_null();

    test_verifier_for_not_null();
    test_verify_not_null();

    test_ZT_TRUE();
    test_ZT_FALSE();
    test_ZT_CMP_BOOL();
    test_ZT_CMP_RUNE();
    test_ZT_CMP_INT();
    test_ZT_CMP_UINT();
    test_ZT_CMP_CSTR();
    test_ZT_NULL();
    test_ZT_NOT_NULL();

    test_quote_string();
    test_quote_rune();

    test_lister_visitor_visit_suite();
    test_runner_visitor_visit_suite();
    test_runner_visitor_visit_case_outcome_pending();
    test_runner_visitor_visit_case_outcome_passed();
    test_runner_visitor_visit_case_outcome_failed();
    test_runner_visitor_visit_case_outcome_bogus();

    test_main_listing_tests();
    test_main_running_passing_tests();
    test_main_verbosely_running_passing_tests();
    test_main_running_failing_tests();

    test_stdout_stderr();

    printf("libzt self-test successful\n");
    return 0;
}
