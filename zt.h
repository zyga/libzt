/* Copyright 2019-2020 Zygmunt Krynicki.
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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define ZT_MAJOR_VERSION 0
#define ZT_MINOR_VERSION 3

struct zt_test;
typedef struct zt_test* zt_t;

struct zt_visitor_vtab;
typedef struct zt_visitor {
    void* id;
    const struct zt_visitor_vtab* vtab;
} zt_visitor;

typedef void (*zt_test_case_func)(zt_t);
typedef void (*zt_test_suite_func)(zt_visitor);

int zt_main(int argc, char** argv, char** envp, zt_test_suite_func tsuite);

void zt_visit_test_suite(zt_visitor v, zt_test_suite_func func, const char* name);
void zt_visit_test_case(zt_visitor v, zt_test_case_func func, const char* name);

#define ZT_VISIT_TEST_SUITE(v, tsuite) zt_visit_test_suite(v, tsuite, #tsuite)
#define ZT_VISIT_TEST_CASE(v, tcase) zt_visit_test_case(v, tcase, #tcase)

typedef enum zt_value_kind {
    ZT_NOTHING,
    ZT_BOOLEAN,
    ZT_RUNE, /* Rune is a more practical character type. */
    ZT_INTEGER, /* Deprecated. Promoted to ZT_INTMAX */
    ZT_UNSIGNED, /* Deprecated. Promoted to ZT_UINTMAX */
    ZT_STRING,
    ZT_POINTER,
    ZT_INTMAX,
    ZT_UINTMAX
} zt_value_kind;

typedef struct zt_value {
    union {
        bool boolean;
        int rune;
        int integer; /* Deprecated. */
        unsigned unsigned_integer; /* Deprecated. */
        const char* string;
        const void* pointer;
        intmax_t intmax;
        uintmax_t uintmax;
    } as;
    const char* source;
    zt_value_kind kind;
} zt_value;

static inline zt_value zt_pack_nothing(void)
{
    zt_value v;
    zt_value_kind kind = ZT_NOTHING;
    v.source = NULL;
    v.kind = kind;
    return v;
}

static inline zt_value zt_pack_boolean(bool value, const char* source)
{
    zt_value v;
    v.as.boolean = value;
    v.source = source;
    v.kind = ZT_BOOLEAN;
    return v;
}

zt_value zt_pack_rune(int value, const char* source);

static inline zt_value zt_pack_integer(intmax_t value, const char* source)
{
    zt_value v;
    v.as.intmax = value;
    v.source = source;
    v.kind = ZT_INTMAX;
    return v;
}

static inline zt_value zt_pack_unsigned(uintmax_t value, const char* source)
{
    zt_value v;
    v.source = source;
    v.as.uintmax = value;
    v.kind = ZT_UINTMAX;
    return v;
}

static inline zt_value zt_pack_string(const char* value, const char* source)
{
    zt_value v;
    v.as.string = value;
    v.source = source;
    v.kind = ZT_STRING;
    return v;
}

static inline zt_value zt_pack_pointer(const void* value, const char* source)
{
    zt_value v;
    v.as.pointer = value;
    v.source = source;
    v.kind = ZT_POINTER;
    return v;
}

typedef struct zt_location {
    const char* fname;
    int lineno;
} zt_location;

static inline zt_location zt_location_at(const char* fname, int lineno)
{
    zt_location loc;
    loc.fname = fname;
    loc.lineno = lineno;
    return loc;
}

#define ZT_CURRENT_LOCATION() zt_location_at(__FILE__, __LINE__)

struct zt_verifier;
typedef struct zt_claim {
    struct zt_verifier (*make_verifier)(void);
    zt_value args[3];
    zt_location location;
} zt_claim;

void zt_check(zt_t test, zt_claim claim);
void zt_assert(zt_t test, zt_claim claim);

zt_claim zt_true(zt_location location, zt_value value);
zt_claim zt_false(zt_location location, zt_value value);
zt_claim zt_cmp_bool(zt_location location, zt_value left, zt_value rel, zt_value right);
zt_claim zt_cmp_rune(zt_location location, zt_value left, zt_value rel, zt_value right);
zt_claim zt_cmp_int(zt_location location, zt_value left, zt_value rel, zt_value right);
zt_claim zt_cmp_ptr(zt_location location, zt_value left, zt_value rel, zt_value right);
zt_claim zt_cmp_uint(zt_location location, zt_value left, zt_value rel, zt_value right);
zt_claim zt_cmp_cstr(zt_location location, zt_value left, zt_value rel, zt_value right);
zt_claim zt_null(zt_location location, zt_value value);
zt_claim zt_not_null(zt_location location, zt_value value);

#define ZT_TRUE(value)         \
    zt_true(                   \
        ZT_CURRENT_LOCATION(), \
        zt_pack_boolean((value), #value))

#define ZT_FALSE(value)        \
    zt_false(                  \
        ZT_CURRENT_LOCATION(), \
        zt_pack_boolean((value), #value))

#define ZT_CMP_BOOL(left, rel, right)   \
    zt_cmp_bool(                        \
        ZT_CURRENT_LOCATION(),          \
        zt_pack_boolean((left), #left), \
        zt_pack_string(#rel, #rel),     \
        zt_pack_boolean((right), #right))

#define ZT_CMP_RUNE(left, rel, right) \
    zt_cmp_rune(                      \
        ZT_CURRENT_LOCATION(),        \
        zt_pack_rune((left), #left),  \
        zt_pack_string(#rel, #rel),   \
        zt_pack_rune((right), #right))

#define ZT_CMP_INT(left, rel, right)    \
    zt_cmp_int(                         \
        ZT_CURRENT_LOCATION(),          \
        zt_pack_integer((left), #left), \
        zt_pack_string(#rel, #rel),     \
        zt_pack_integer((right), #right))

#define ZT_CMP_UINT(left, rel, right)    \
    zt_cmp_uint(                         \
        ZT_CURRENT_LOCATION(),           \
        zt_pack_unsigned((left), #left), \
        zt_pack_string(#rel, #rel),      \
        zt_pack_unsigned((right), #right))

#define ZT_CMP_CSTR(left, rel, right)  \
    zt_cmp_cstr(                       \
        ZT_CURRENT_LOCATION(),         \
        zt_pack_string((left), #left), \
        zt_pack_string(#rel, #rel),    \
        zt_pack_string((right), #right))

#define ZT_CMP_PTR(left, rel, right)    \
    zt_cmp_ptr(                         \
        ZT_CURRENT_LOCATION(),          \
        zt_pack_pointer((left), #left), \
        zt_pack_string(#rel, #rel),     \
        zt_pack_pointer((right), #right))

#define ZT_NULL(value)         \
    zt_null(                   \
        ZT_CURRENT_LOCATION(), \
        zt_pack_pointer((value), #value))

#define ZT_NOT_NULL(value)     \
    zt_not_null(               \
        ZT_CURRENT_LOCATION(), \
        zt_pack_pointer((value), #value))

#ifdef __cplusplus
}
#endif
