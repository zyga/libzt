#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zt.h>

static void passing_test(zt_test* t)
{
    zt_check(t, ZT_TRUE(2 + 2 == 4));
    zt_check(t, ZT_FALSE(2 + 2 == 5));
    zt_check(t, ZT_CMP_INT(2 + 2, ==, 4));
    zt_check(t, ZT_CMP_BOOL(false, ==, false));
}

static void failing_test(zt_test* t)
{
    zt_check(t, ZT_TRUE(2 + 2 != 4));
    zt_check(t, ZT_FALSE(2 + 2 != 5));
    zt_check(t, ZT_CMP_INT(2 + 2, ==, 5));
    zt_check(t, ZT_CMP_BOOL(false, ==, true));
}

static void badly_failing_test(zt_test* t)
{
    zt_assert(t, ZT_CMP_INT(0, ==, -1));
    abort();
}

static void passing_suite(zt_visitor v) { ZT_VISIT_TEST_CASE(v, passing_test); }

static void test_suite(zt_visitor v)
{
    ZT_VISIT_TEST_SUITE(v, passing_suite);
    ZT_VISIT_TEST_CASE(v, failing_test);
    ZT_VISIT_TEST_CASE(v, badly_failing_test);
}

int main(int argc, char** argv, char** envp)
{
    return zt_main(argc, argv, envp, test_suite);
}
