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

#include "zt.h"

#include <ctype.h>
#include <inttypes.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WIN32
#define ZT_UNUSED __attribute__((unused))
#else
#define ZT_UNUSED
#endif

static inline zt_value_kind zt_value_kind_of(zt_value value)
{
    return value.kind;
}

static inline const char* zt_source_of(zt_value value)
{
    return value.source;
}

zt_value zt_pack_rune(int value, const char* source)
{
    zt_value v;
    /* Rune is meant to be an unsigned value but for practicality the pack
    helper is defined to take an integer. Negative values can arise when
    a non-7-bit byte is encoded in the string and the architecture uses
    signed characters. To counter this treat negative values as if they were
    created by sign-extending a signed character and cast to unsigned
    character. */
    if (value < 0) {
        value &= 0xFF;
    }
    v.as.rune = value;
    v.source = source;
    v.kind = ZT_RUNE;
    return v;
}

/** zt_binary_relation describes one of typical binary relations. */
typedef enum zt_binary_relation {
    ZT_REL_INVALID,
    ZT_REL_EQ,
    ZT_REL_NE,
    ZT_REL_LE,
    ZT_REL_GE,
    ZT_REL_LT,
    ZT_REL_GT
} zt_binary_relation;

/** zt_find_binary_relation finds a binary relation given operator name. */
static zt_binary_relation zt_find_binary_relation(const char* rel)
{
    if (strcmp(rel, "==") == 0) {
        return ZT_REL_EQ;
    }
    if (strcmp(rel, "!=") == 0) {
        return ZT_REL_NE;
    }
    if (strcmp(rel, "<=") == 0) {
        return ZT_REL_LE;
    }
    if (strcmp(rel, ">=") == 0) {
        return ZT_REL_GE;
    }
    if (strcmp(rel, "<") == 0) {
        return ZT_REL_LT;
    }
    if (strcmp(rel, ">") == 0) {
        return ZT_REL_GT;
    }
    return ZT_REL_INVALID;
}

/** zt_invert_binary_relation returns the inverted relation. */
static zt_binary_relation zt_invert_binary_relation(zt_binary_relation rel)
{
    switch (rel) {
    default:
    case ZT_REL_INVALID:
        return ZT_REL_INVALID;
    case ZT_REL_EQ:
        return ZT_REL_NE;
    case ZT_REL_NE:
        return ZT_REL_EQ;
    case ZT_REL_LE:
        return ZT_REL_GT;
    case ZT_REL_GE:
        return ZT_REL_LT;
    case ZT_REL_LT:
        return ZT_REL_GE;
    case ZT_REL_GT:
        return ZT_REL_LE;
    }
}

/** zt_binary_relation_as_text returns text representation of a relation. */
static const char* zt_binary_relation_as_text(zt_binary_relation rel)
{
    switch (rel) {
    default:
    case ZT_REL_INVALID:
        return "invalid";
    case ZT_REL_EQ:
        return "==";
    case ZT_REL_NE:
        return "!=";
    case ZT_REL_LE:
        return "<=";
    case ZT_REL_GE:
        return ">=";
    case ZT_REL_LT:
        return "<";
    case ZT_REL_GT:
        return ">";
    }
}

static bool zt_relation_inconsistent(zt_value rel)
{
    return strcmp(rel.as.string, zt_source_of(rel)) != 0;
}

static void zt_quote_rune_inner(FILE* stream, int c, int quote)
{
    switch (c) {
    case '\'':
        fputs(quote != '\'' ? "'" : "\\'", stream);
        break;
    case '"':
        fputs(quote != '\"' ? "\"" : "\\\"", stream);
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
    case '\b':
        fputs("\\b", stream);
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

static void zt_quote_rune(FILE* stream, int c)
{
    fputc('\'', stream);
    zt_quote_rune_inner(stream, c, '\'');
    fputc('\'', stream);
}

static void zt_quote_string(FILE* stream, const char* str)
{
    int c;
    fputs("\"", stream);
    while ((c = *(str++))) {
        zt_quote_rune_inner(stream, c, '"');
    }
    fputs("\"", stream);
}

/** zt_outcome describes outcome of a single test. */
typedef enum zt_outcome {
    ZT_PENDING,
    ZT_PASSED,
    ZT_FAILED
} zt_outcome;

typedef struct zt_test {
#ifndef _WIN32
    sigjmp_buf jump_buffer;
#else
    jmp_buf jump_buffer;
#endif
    const char* name;
    FILE* stream;
    zt_location location; /** location of the last verified claim. */
    zt_outcome outcome;
} zt_test;

static bool zt_test_failure(zt_test* test, const char* fmt, ...)
#ifndef _WIN32
    __attribute__((format(printf, 2, 3)))
#endif
    ;

typedef struct zt_visitor_vtab {
    void (*visit_case)(void*, zt_test_case_func, const char* name);
    void (*visit_suite)(void*, zt_test_suite_func, const char* name);
} zt_visitor_vtab;

typedef struct zt_test_lister {
    FILE* stream;
    int nesting;
} zt_test_lister;

typedef struct zt_test_runner {
    FILE* stream_out;
    FILE* stream_err;
    int nesting;
    int num_passed;
    int num_failed;
    bool verbose;
} zt_test_runner;

/** zt_verify0_func is a type of verification function with no arguments. */
typedef bool (*zt_verify0_func)(struct zt_test*);

/** zt_verify1_func is a type of verification function with one argument. */
typedef bool (*zt_verify1_func)(struct zt_test*, zt_value);

/** zt_verify2_func is a type of verification function with two arguments. */
typedef bool (*zt_verify2_func)(struct zt_test*, zt_value, zt_value);

/** zt_verify3_func is a type of verification function with three arguments. */
typedef bool (*zt_verify3_func)(struct zt_test*, zt_value, zt_value, zt_value);

/**
 * zt_arg_info describes expected kind of arguments for verification
 * functions.
 *
 * Because verification functions use value variants as arguments there is an
 * additional layer that describes the desired type of each value. If the
 * actual value is of another kind a kind mismatch message contains useful
 * explanation about what is necessary.
 **/
typedef struct zt_arg_info {
    const char* kind_mismatch_msg; /**< failure message on kind mismatch. */
    zt_value_kind kind; /**< expected kind of argument value. */
} zt_arg_info;

/**
 * zt_verifier describes a verification function for a given claim.
 *
 * Verifiers are used to check if some property holds. For example, _not null_,
 * _integer value greater or equal than_, _has string prefix_ are all
 * verifiers.
 *
 * Verifiers are like functions, they don't contain specific values yet, those
 * are provided by the claim type below.
 **/
typedef struct zt_verifier {
    union {
        zt_verify0_func args0; /**< function pointer if nargs==0 */
        zt_verify1_func args1; /**< function pointer if nargs==1 */
        zt_verify2_func args2; /**< function pointer if nargs==2 */
        zt_verify3_func args3; /**< function pointer if nargs==3 */
    } func; /**< union storing typed function pointer. */
    size_t nargs; /**< number of arguments of the referenced function. */
    zt_arg_info arg_infos[3]; /**< arguments for the referenced function. */
} zt_verifier;

void zt_visit_test_suite(zt_visitor v, zt_test_suite_func func,
    const char* name)
{
    v.vtab->visit_suite(v.id, func, name);
}

void zt_visit_test_case(zt_visitor v, zt_test_case_func func,
    const char* name)
{
    v.vtab->visit_case(v.id, func, name);
}

/* Lister visitor */

static zt_visitor zt_visitor_from_test_lister(zt_test_lister* lister);

static void zt_test_lister__visit_suite(void* id, zt_test_suite_func func,
    const char* name)
{
    zt_test_lister* lister = (zt_test_lister*)id;
    fprintf(lister->stream, "%*c %s\n", lister->nesting * 3, '-', name);
    lister->nesting++;
    func(zt_visitor_from_test_lister(lister));
    lister->nesting--;
}

static void zt_test_lister__visit_case(void* id, ZT_UNUSED zt_test_case_func func,
    const char* name)
{
    zt_test_lister* lister = (zt_test_lister*)id;
    fprintf(lister->stream, "%*c %s\n", lister->nesting * 3, '-', name);
}

static const zt_visitor_vtab zt_test_lister__visitor_vtab = {
    /* .visit_case = */ zt_test_lister__visit_case,
    /* .visit_suite = */ zt_test_lister__visit_suite,
};

static zt_visitor zt_visitor_from_test_lister(zt_test_lister* lister)
{
    zt_visitor visitor;
    memset(&visitor, 0, sizeof visitor);
    visitor.id = lister;
    visitor.vtab = &zt_test_lister__visitor_vtab;
    return visitor;
}

/** zt_list_tests_from lists tests from given suite to a given file. */
static void zt_list_tests_from(FILE* stream, zt_test_suite_func tsuite)
{
    zt_test_lister lister;
    memset(&lister, 0, sizeof lister);
    lister.stream = stream;
    tsuite(zt_visitor_from_test_lister(&lister));
}

/* Runner visitor */

static zt_visitor zt_visitor_from_test_runner(zt_test_runner* runner);

static void zt_runner_visitor__visit_suite(void* id, zt_test_suite_func func,
    const char* name)
{
    zt_test_runner* runner = (zt_test_runner*)id;
    if (runner->verbose && runner->stream_out) {
        fprintf(runner->stream_out, "%*c %s\n", runner->nesting * 3, '-', name);
    }

    runner->nesting++;
    func(zt_visitor_from_test_runner(runner));
    runner->nesting--;
}

static void zt_runner_visitor__visit_case(void* id, zt_test_case_func func,
    const char* name)
{
    zt_test_runner* runner = (zt_test_runner*)id;
    zt_test test;
    int jump_result;
    memset(&test, 0, sizeof test);
    test.stream = runner->stream_err;
    test.outcome = ZT_PENDING;
#ifndef _WIN32
    jump_result = sigsetjmp(test.jump_buffer, 1);
#else
    jump_result = setjmp(test.jump_buffer);
#endif
    if (jump_result == 0) {
        if (runner->verbose && runner->stream_out) {
            fprintf(runner->stream_out, "%*c %s\n", runner->nesting * 3, '-', name);
        }
        func(&test);
    }
    switch (test.outcome) {
    case ZT_PENDING:
    case ZT_PASSED:
        runner->num_passed++;
        break;
    case ZT_FAILED:
        runner->num_failed++;
        break;
    default:
        if (runner->stream_err) {
            fprintf(runner->stream_err, "%*c %s - unexpected outcome code %d\n",
                runner->nesting * 3, '-', name, test.outcome);
        }
        runner->num_failed++;
        break;
    }
}

static const zt_visitor_vtab zt_test_runner__visitor_vtab = {
    /* .visit_case = */ zt_runner_visitor__visit_case,
    /* .visit_suite = */ zt_runner_visitor__visit_suite,
};

static zt_visitor zt_visitor_from_test_runner(zt_test_runner* runner)
{
    zt_visitor visitor;
    visitor.id = runner;
    visitor.vtab = &zt_test_runner__visitor_vtab;
    return visitor;
}

/** zt_run_tests_from runs tests from given suite and returns the outcome. */
static zt_outcome zt_run_tests_from(FILE* stream_out, FILE* stream_err, bool verbose,
    void (*test_suite_func)(zt_visitor))
{
    zt_test_runner runner;
    memset(&runner, 0, sizeof runner);
    runner.stream_out = stream_out;
    runner.stream_err = stream_err;
    runner.verbose = verbose;
    test_suite_func(zt_visitor_from_test_runner(&runner));
    if (runner.num_failed > 0) {
        return ZT_FAILED;
    }
    return ZT_PASSED;
}

/* claim verifier and test failure */

static bool zt_verify_claim(zt_test* test, const zt_claim* claim)
{
    zt_verifier verifier = claim->make_verifier();
    test->location = claim->location;
    switch (verifier.nargs) {
    case 0:
        return verifier.func.args0(test);
    case 1:
        if (zt_value_kind_of(claim->args[0]) != verifier.arg_infos[0].kind) {
            return zt_test_failure(test, "%s", verifier.arg_infos[0].kind_mismatch_msg);
        }
        return verifier.func.args1(test, claim->args[0]);
    case 2:
        if (zt_value_kind_of(claim->args[0]) != verifier.arg_infos[0].kind) {
            return zt_test_failure(test, "%s", verifier.arg_infos[0].kind_mismatch_msg);
        }
        if (zt_value_kind_of(claim->args[1]) != verifier.arg_infos[1].kind) {
            return zt_test_failure(test, "%s", verifier.arg_infos[1].kind_mismatch_msg);
        }
        return verifier.func.args2(test, claim->args[0], claim->args[1]);
    case 3:
        if (zt_value_kind_of(claim->args[0]) != verifier.arg_infos[0].kind) {
            return zt_test_failure(test, "%s", verifier.arg_infos[0].kind_mismatch_msg);
        }
        if (zt_value_kind_of(claim->args[1]) != verifier.arg_infos[1].kind) {
            return zt_test_failure(test, "%s", verifier.arg_infos[1].kind_mismatch_msg);
        }
        if (zt_value_kind_of(claim->args[2]) != verifier.arg_infos[2].kind) {
            return zt_test_failure(test, "%s", verifier.arg_infos[2].kind_mismatch_msg);
        }
        return verifier.func.args3(test, claim->args[0], claim->args[1],
            claim->args[2]);
    default:
        return zt_test_failure(test, "unsupported number of arguments: %" PRIuMAX,
            (uintmax_t)verifier.nargs);
    }
}

static bool zt_test_failure(zt_test* test, const char* fmt, ...)
{
    va_list ap;
    FILE* stream = test->stream;

    va_start(ap, fmt);
    if (stream != NULL) {
        const zt_location loc = test->location;
        fprintf(stream, "%s:%d: ", loc.fname, loc.lineno);
        vfprintf(stream, fmt, ap);
        fprintf(stream, "\n");
    }
    va_end(ap);
    return false;
}

/* check and assert */

void zt_check(zt_test* test, zt_claim claim)
{
    if (!zt_verify_claim(test, &claim)) {
        test->outcome = ZT_FAILED;
    }
}

void zt_assert(zt_test* test, zt_claim claim)
{
    if (!zt_verify_claim(test, &claim)) {
        test->outcome = ZT_FAILED;
#ifndef _WIN32
        siglongjmp(test->jump_buffer, 1);
#else
        longjmp(test->jump_buffer, 1);
#endif
        /* TODO: in C++ mode throw an exception. */
    }
}

/* main */

#ifdef ZT_SELF_TEST_BUILD
static FILE* zt_mock_stdout = NULL;
static FILE* zt_mock_stderr = NULL;
#endif

static FILE* zt_stdout(void)
{
#ifdef ZT_SELF_TEST_BUILD
    if (zt_mock_stdout != NULL) {
        return zt_mock_stdout;
    }
#endif
    return stdout;
}

static FILE* zt_stderr(void)
{
#ifdef ZT_SELF_TEST_BUILD
    if (zt_mock_stderr != NULL) {
        return zt_mock_stderr;
    }
#endif
    return stderr;
}

int zt_main(int argc, char** argv, ZT_UNUSED char** envp,
    zt_test_suite_func tsuite)
{
    if (argc == 2 && strcmp(argv[1], "-l") == 0) {
        zt_list_tests_from(zt_stdout(), tsuite);
        return EXIT_SUCCESS;
    } else {
        bool verbose = argc == 2 && strcmp(argv[1], "-v") == 0;
        return zt_run_tests_from(
                   zt_stdout(), zt_stderr(), verbose, tsuite)
                == ZT_PASSED
            ? EXIT_SUCCESS
            : EXIT_FAILURE;
    }
}

/* verifiers and verification functions */

static bool zt_verify_true(zt_test* test, zt_value value)
{
    if (value.as.boolean) {
        return true;
    }
    return zt_test_failure(test, "assertion failed because %s is false", zt_source_of(value));
}

static zt_verifier zt_verifier_for_true(void)
{
    zt_verifier verifier;
    memset(&verifier, 0, sizeof verifier);
    verifier.func.args1 = zt_verify_true;
    verifier.nargs = 1;
    verifier.arg_infos[0].kind = ZT_BOOLEAN;
    verifier.arg_infos[0].kind_mismatch_msg = "value is not a boolean";
    return verifier;
}

zt_claim zt_true(zt_location location, zt_value value)
{
    zt_claim claim;
    memset(&claim, 0, sizeof claim);
    claim.location = location;
    claim.make_verifier = zt_verifier_for_true;
    claim.args[0] = value;
    return claim;
}

static bool zt_verify_false(zt_test* test, zt_value value)
{
    if (!value.as.boolean) {
        return true;
    }
    return zt_test_failure(test, "assertion failed because %s is true", zt_source_of(value));
}

static zt_verifier zt_verifier_for_false(void)
{
    zt_verifier verifier;
    memset(&verifier, 0, sizeof verifier);
    verifier.func.args1 = zt_verify_false;
    verifier.nargs = 1;
    verifier.arg_infos[0].kind = ZT_BOOLEAN;
    verifier.arg_infos[0].kind_mismatch_msg = "value is not a boolean";
    return verifier;
}

zt_claim zt_false(zt_location location, zt_value value)
{
    zt_claim claim;
    memset(&claim, 0, sizeof claim);
    claim.location = location;
    claim.make_verifier = zt_verifier_for_false;
    claim.args[0] = value;
    return claim;
}

static const char* zt_boolean_as_text(bool b)
{
    return b ? "true" : "false";
}

static bool zt_verify_boolean_relation(zt_test* test, zt_value left, zt_value rel,
    zt_value right)
{
    zt_binary_relation bin_rel;

    if (zt_relation_inconsistent(rel)) {
        return zt_test_failure(test, "%s %s %s uses inconsistent relation %s",
            zt_source_of(left), zt_source_of(rel), zt_source_of(right), rel.as.string);
    }
    bin_rel = zt_find_binary_relation(rel.as.string);
    switch (bin_rel) {
    case ZT_REL_EQ:
        if (left.as.boolean == right.as.boolean) {
            return true;
        }
        break;
    case ZT_REL_NE:
        if (left.as.boolean != right.as.boolean) {
            return true;
        }
        break;
    default:
        return zt_test_failure(test, "assertion %s %s %s uses unsupported relation",
            zt_source_of(left), rel.as.string, zt_source_of(right));
    }
    return zt_test_failure(test, "assertion %s %s %s failed because %s %s %s",
        zt_source_of(left), zt_binary_relation_as_text(bin_rel),
        zt_source_of(right),
        zt_boolean_as_text(left.as.boolean),
        zt_binary_relation_as_text(zt_invert_binary_relation(bin_rel)),
        zt_boolean_as_text(right.as.boolean));
}

static zt_verifier zt_verifier_for_boolean_relation(void)
{
    zt_verifier verifier;
    memset(&verifier, 0, sizeof verifier);
    verifier.func.args3 = zt_verify_boolean_relation;
    verifier.nargs = 3;
    verifier.arg_infos[0].kind = ZT_BOOLEAN;
    verifier.arg_infos[0].kind_mismatch_msg = "left hand side is not a boolean";
    verifier.arg_infos[1].kind = ZT_STRING;
    verifier.arg_infos[1].kind_mismatch_msg = "relation is not a string";
    verifier.arg_infos[2].kind = ZT_BOOLEAN;
    verifier.arg_infos[2].kind_mismatch_msg = "right hand side is not a boolean";
    return verifier;
}

zt_claim zt_cmp_bool(zt_location location, zt_value left, zt_value rel, zt_value right)
{
    zt_claim claim;
    memset(&claim, 0, sizeof claim);
    claim.location = location;
    claim.make_verifier = zt_verifier_for_boolean_relation;
    claim.args[0] = left;
    claim.args[1] = rel;
    claim.args[2] = right;
    return claim;
}

/**
 * zt_verify_rune_relation verifies a relation between two runes.
 *
 * Relation is one of: ==, !=, <=, =>, < and >.
 **/
static bool zt_verify_rune_relation(zt_test* test, zt_value left, zt_value rel,
    zt_value right)
{
    zt_binary_relation bin_rel;

    if (zt_relation_inconsistent(rel)) {
        return zt_test_failure(test, "assertion %s %s %s uses inconsistent relation %s",
            zt_source_of(left), zt_source_of(rel), zt_source_of(right), rel.as.string);
    }
    bin_rel = zt_find_binary_relation(rel.as.string);
    switch (bin_rel) {
    case ZT_REL_EQ:
        if (left.as.rune == right.as.rune) {
            return true;
        }
        break;
    case ZT_REL_NE:
        if (left.as.rune != right.as.rune) {
            return true;
        }
        break;
    case ZT_REL_LE:
        if (left.as.rune <= right.as.rune) {
            return true;
        }
        break;
    case ZT_REL_GE:
        if (left.as.rune >= right.as.rune) {
            return true;
        }
        break;
    case ZT_REL_LT:
        if (left.as.rune < right.as.rune) {
            return true;
        }
        break;
    case ZT_REL_GT:
        if (left.as.rune > right.as.rune) {
            return true;
        }
        break;
    case ZT_REL_INVALID:
        return zt_test_failure(test, "assertion %s %s %s uses unsupported relation",
            zt_source_of(left), zt_source_of(rel), zt_source_of(right));
    }
    if (test->stream) {
        const zt_location loc = test->location;
        FILE* stream = test->stream;
        fprintf(stream, "%s:%d: ", loc.fname, loc.lineno);
        fprintf(stream, "assertion %s %s %s failed because ",
            zt_source_of(left), zt_binary_relation_as_text(bin_rel),
            zt_source_of(right));
        zt_quote_rune(stream, left.as.rune);
        fprintf(stream, " %s ", zt_binary_relation_as_text(zt_invert_binary_relation(bin_rel)));
        zt_quote_rune(stream, right.as.rune);
        fprintf(stream, "\n");
    }
    return false;
}

/**
 * zt_verifier_for_rune_relation returns a verifier for rune relations.
 *
 * The returned verifier is used to implement ZT_CMP_RUNE().
 **/
static zt_verifier zt_verifier_for_rune_relation(void)
{
    zt_verifier verifier;
    memset(&verifier, 0, sizeof verifier);
    verifier.func.args3 = zt_verify_rune_relation;
    verifier.nargs = 3;
    verifier.arg_infos[0].kind = ZT_RUNE;
    verifier.arg_infos[0].kind_mismatch_msg = "left hand side is not a rune";
    verifier.arg_infos[1].kind = ZT_STRING;
    verifier.arg_infos[1].kind_mismatch_msg = "relation is not a string";
    verifier.arg_infos[2].kind = ZT_RUNE;
    verifier.arg_infos[2].kind_mismatch_msg = "right hand side is not a rune";
    return verifier;
}

zt_claim zt_cmp_rune(zt_location location, zt_value left, zt_value rel, zt_value right)
{
    zt_claim claim;
    memset(&claim, 0, sizeof claim);
    claim.location = location;
    claim.make_verifier = zt_verifier_for_rune_relation;
    claim.args[0] = left;
    claim.args[1] = rel;
    claim.args[2] = right;
    return claim;
}

/**
 * zt_verify_integer_relation verifies a relation between two signed integers.
 *
 * Relation is one of: ==, !=, <=, =>, < and >.
 **/
static bool zt_verify_integer_relation(zt_test* test, zt_value left, zt_value rel,
    zt_value right)
{
    zt_binary_relation bin_rel;

    if (zt_relation_inconsistent(rel)) {
        return zt_test_failure(test, "assertion %s %s %s uses inconsistent relation %s",
            zt_source_of(left), zt_source_of(rel), zt_source_of(right), rel.as.string);
    }
    bin_rel = zt_find_binary_relation(rel.as.string);
    switch (bin_rel) {
    case ZT_REL_EQ:
        if (left.as.integer == right.as.integer) {
            return true;
        }
        break;
    case ZT_REL_NE:
        if (left.as.integer != right.as.integer) {
            return true;
        }
        break;
    case ZT_REL_LE:
        if (left.as.integer <= right.as.integer) {
            return true;
        }
        break;
    case ZT_REL_GE:
        if (left.as.integer >= right.as.integer) {
            return true;
        }
        break;
    case ZT_REL_LT:
        if (left.as.integer < right.as.integer) {
            return true;
        }
        break;
    case ZT_REL_GT:
        if (left.as.integer > right.as.integer) {
            return true;
        }
        break;
    case ZT_REL_INVALID:
        return zt_test_failure(test, "assertion %s %s %s uses unsupported relation",
            zt_source_of(left), zt_source_of(rel), zt_source_of(right));
    }
    return zt_test_failure(test, "assertion %s %s %s failed because %d %s %d",
        zt_source_of(left), zt_source_of(rel), zt_source_of(right),
        left.as.integer, zt_binary_relation_as_text(zt_invert_binary_relation(bin_rel)),
        right.as.integer);
}

/**
 * zt_verifier_for_integer_relation returns a verifier for integer relations.
 *
 * The returned verifier is used to implement ZT_CMP_INT().
 **/
static zt_verifier zt_verifier_for_integer_relation(void)
{
    zt_verifier verifier;
    memset(&verifier, 0, sizeof verifier);
    verifier.func.args3 = zt_verify_integer_relation;
    verifier.nargs = 3;
    verifier.arg_infos[0].kind = ZT_INTEGER;
    verifier.arg_infos[0].kind_mismatch_msg = "left hand side is not an integer";
    verifier.arg_infos[1].kind = ZT_STRING;
    verifier.arg_infos[1].kind_mismatch_msg = "relation is not a string";
    verifier.arg_infos[2].kind = ZT_INTEGER;
    verifier.arg_infos[2].kind_mismatch_msg = "right hand side is not an integer";
    return verifier;
}

zt_claim zt_cmp_int(zt_location location, zt_value left, zt_value rel, zt_value right)
{
    zt_claim claim;
    memset(&claim, 0, sizeof claim);
    claim.location = location;
    claim.make_verifier = zt_verifier_for_integer_relation;
    claim.args[0] = left;
    claim.args[1] = rel;
    claim.args[2] = right;
    return claim;
}

/**
 * zt_verify_unsigned_relation verifies a relation between two unsigned values.
 *
 * Relation is one of: ==, !=, <=, =>, < and >.
 **/
static bool zt_verify_unsigned_relation(zt_test* test, zt_value left, zt_value rel,
    zt_value right)
{
    zt_binary_relation bin_rel;

    if (zt_relation_inconsistent(rel)) {
        return zt_test_failure(test, "assertion %s %s %s uses inconsistent relation %s",
            zt_source_of(left), zt_source_of(rel), zt_source_of(right), rel.as.string);
    }
    bin_rel = zt_find_binary_relation(rel.as.string);
    switch (bin_rel) {
    case ZT_REL_EQ:
        if (left.as.unsigned_integer == right.as.unsigned_integer) {
            return true;
        }
        break;
    case ZT_REL_NE:
        if (left.as.unsigned_integer != right.as.unsigned_integer) {
            return true;
        }
        break;
    case ZT_REL_LE:
        if (left.as.unsigned_integer <= right.as.unsigned_integer) {
            return true;
        }
        break;
    case ZT_REL_GE:
        if (left.as.unsigned_integer >= right.as.unsigned_integer) {
            return true;
        }
        break;
    case ZT_REL_LT:
        if (left.as.unsigned_integer < right.as.unsigned_integer) {
            return true;
        }
        break;
    case ZT_REL_GT:
        if (left.as.unsigned_integer > right.as.unsigned_integer) {
            return true;
        }
        break;
    case ZT_REL_INVALID:
        return zt_test_failure(test, "assertion %s %s %s uses unsupported relation",
            zt_source_of(left), rel.as.string, zt_source_of(right));
    }
    return zt_test_failure(test, "assertion %s %s %s failed because %u %s %u",
        zt_source_of(left), rel.as.string, zt_source_of(right),
        left.as.unsigned_integer,
        zt_binary_relation_as_text(zt_invert_binary_relation(bin_rel)),
        right.as.unsigned_integer);
}

/**
 * zt_verifier_for_unsigned_relation returns a verifier for unsigned relations.
 *
 * The returned verifier is used to implement ZT_CMP_UINT().
 */
static zt_verifier zt_verifier_for_unsigned_relation(void)
{
    zt_verifier verifier;
    memset(&verifier, 0, sizeof verifier);
    verifier.func.args3 = zt_verify_unsigned_relation;
    verifier.nargs = 3;
    verifier.arg_infos[0].kind = ZT_UNSIGNED;
    verifier.arg_infos[0].kind_mismatch_msg = "left hand side is not an unsigned integer";
    verifier.arg_infos[1].kind = ZT_STRING;
    verifier.arg_infos[1].kind_mismatch_msg = "relation is not a string";
    verifier.arg_infos[2].kind = ZT_UNSIGNED;
    verifier.arg_infos[2].kind_mismatch_msg = "right hand side is not an unsigned integer";
    return verifier;
}

zt_claim zt_cmp_uint(zt_location location, zt_value left, zt_value rel, zt_value right)
{
    zt_claim claim;
    memset(&claim, 0, sizeof claim);
    claim.location = location;
    claim.make_verifier = zt_verifier_for_unsigned_relation;
    claim.args[0] = left;
    claim.args[1] = rel;
    claim.args[2] = right;
    return claim;
}

static bool zt_verify_string_relation(zt_test* test, zt_value left, zt_value rel,
    zt_value right)
{
    zt_binary_relation bin_rel;
    int cmp;

    if (zt_relation_inconsistent(rel)) {
        return zt_test_failure(test, "assertion %s %s %s uses inconsistent relation %s",
            zt_source_of(left), zt_source_of(rel), zt_source_of(right), rel.as.string);
    }
    bin_rel = zt_find_binary_relation(rel.as.string);
    if (left.as.string == NULL) {
        return zt_test_failure(test, "assertion %s %s %s failed because left hand side is NULL", zt_source_of(left),
            rel.as.string, zt_source_of(right));
    }
    if (right.as.string == NULL) {
        return zt_test_failure(test, "assertion %s %s %s failed because right hand side is NULL", zt_source_of(left),
            rel.as.string, zt_source_of(right));
    }
    cmp = strcmp(left.as.string, right.as.string);
    switch (bin_rel) {
    case ZT_REL_EQ:
        if (cmp == 0) {
            return true;
        }
        break;
    case ZT_REL_NE:
        if (cmp != 0) {
            return true;
        }
        break;
    case ZT_REL_LE:
        if (cmp <= 0) {
            return true;
        }
        break;
    case ZT_REL_GE:
        if (cmp >= 0) {
            return true;
        }
        break;
    case ZT_REL_LT:
        if (cmp < 0) {
            return true;
        }
        break;
    case ZT_REL_GT:
        if (cmp > 0) {
            return true;
        }
        break;
    case ZT_REL_INVALID:
        return zt_test_failure(test, "assertion %s %s %s uses unsupported relation",
            zt_source_of(left), zt_source_of(rel), zt_source_of(right));
    }
    if (test->stream) {
        const zt_location loc = test->location;
        FILE* stream = test->stream;
        fprintf(stream, "%s:%d: ", loc.fname, loc.lineno);
        fprintf(stream, "assertion %s %s %s failed because ", zt_source_of(left),
            rel.as.string, zt_source_of(right));
        zt_quote_string(stream, left.as.string);
        fprintf(stream, " %s ", zt_binary_relation_as_text(zt_invert_binary_relation(bin_rel)));
        zt_quote_string(stream, right.as.string);
        fprintf(stream, "\n");
    }
    return false;
}

/**
 * zt_verifier_for_string_relation returns a verifier for string relations.
 *
 * The returned verifier is used to implement ZT_CMP_CSTR().
 */
static zt_verifier zt_verifier_for_string_relation(void)
{
    zt_verifier verifier;
    memset(&verifier, 0, sizeof verifier);
    verifier.func.args3 = zt_verify_string_relation;
    verifier.nargs = 3;
    verifier.arg_infos[0].kind = ZT_STRING;
    verifier.arg_infos[0].kind_mismatch_msg = "left hand side is not a string";
    verifier.arg_infos[1].kind = ZT_STRING;
    verifier.arg_infos[1].kind_mismatch_msg = "relation is not a string";
    verifier.arg_infos[2].kind = ZT_STRING;
    verifier.arg_infos[2].kind_mismatch_msg = "right hand side is not a string";
    return verifier;
}

zt_claim zt_cmp_cstr(zt_location location, zt_value left, zt_value rel, zt_value right)
{
    zt_claim claim;
    memset(&claim, 0, sizeof claim);
    claim.location = location;
    claim.make_verifier = zt_verifier_for_string_relation;
    claim.args[0] = left;
    claim.args[1] = rel;
    claim.args[2] = right;
    return claim;
}

static bool zt_verify_null(zt_test* test, zt_value value)
{
    if (value.as.pointer == NULL) {
        return true;
    }
    return zt_test_failure(test, "assertion %s == NULL failed because %p != NULL",
        zt_source_of(value), value.as.pointer);
}

static zt_verifier zt_verifier_for_null(void)
{
    zt_verifier verifier;
    memset(&verifier, 0, sizeof verifier);
    verifier.func.args1 = zt_verify_null;
    verifier.nargs = 1;
    verifier.arg_infos[0].kind = ZT_POINTER;
    verifier.arg_infos[0].kind_mismatch_msg = "argument is not a pointer";
    return verifier;
}

zt_claim zt_null(zt_location location, zt_value value)
{
    zt_claim claim;
    memset(&claim, 0, sizeof claim);
    claim.location = location;
    claim.make_verifier = zt_verifier_for_null;
    claim.args[0] = value;
    return claim;
}

static bool zt_verify_not_null(zt_test* test, zt_value value)
{
    if (value.as.pointer != NULL) {
        return true;
    }
    return zt_test_failure(test, "assertion %s != NULL failed", zt_source_of(value));
}

static zt_verifier zt_verifier_for_not_null(void)
{
    zt_verifier verifier;
    memset(&verifier, 0, sizeof verifier);
    verifier.func.args1 = zt_verify_not_null;
    verifier.nargs = 1;
    verifier.arg_infos[0].kind = ZT_POINTER;
    verifier.arg_infos[0].kind_mismatch_msg = "argument is not a pointer";
    return verifier;
}

zt_claim zt_not_null(zt_location location, zt_value value)
{
    zt_claim claim;
    memset(&claim, 0, sizeof claim);
    claim.location = location;
    claim.make_verifier = zt_verifier_for_not_null;
    claim.args[0] = value;
    return claim;
}
