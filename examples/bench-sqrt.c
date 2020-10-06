#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zt.h>

static void bench_nothing(zt_b b)
{
    for (volatile uint64_t i = b->n; i != 0; --i) {
    }
}

static void bench_sqrtf(zt_b b)
{
    volatile float in = 2.0;
    volatile float out;
    for (uint64_t i = b->n; i != 0; --i) {
        out = sqrtf(in);
    }
    (void)out;
}

static void bench_sqrt(zt_b b)
{
    volatile double in = 2.0;
    volatile double out;
    for (uint64_t i = b->n; i != 0; --i) {
        out = sqrt(in);
    }
    (void)out;
}

static void bench_sqrtl(zt_b b)
{
    volatile long double in = 2.0;
    volatile long double out;
    for (uint64_t i = b->n; i != 0; --i) {
        out = sqrtl(in);
    }
    (void)out;
}

static void test_suite(zt_visitor v)
{
    ZT_VISIT_BENCHMARK(v, bench_nothing);
    ZT_VISIT_BENCHMARK(v, bench_sqrtf);
    ZT_VISIT_BENCHMARK(v, bench_sqrt);
    ZT_VISIT_BENCHMARK(v, bench_sqrtl);
}

int main(int argc, char** argv, char** envp)
{
    return zt_main(argc, argv, envp, test_suite);
}
