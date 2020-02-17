![C CI](https://github.com/zyga/libzt/workflows/C%20CI/badge.svg)

# libzt is an unit test library for C

libzt is a simple and robust unit test library for C.

## Features

 - Robust, allowing you to focus on your code.
 - Simple and small, making it quick to learn and use.
 - Doesn't use dynamic memory allocation, reducing error handling.
 - Equipped with useful helpers for writing test cases.
 - Portable and supported on Linux, MacOS and Windows.
 - Documented and fully coverage and integration tested.

## Example

```
#include <stdio.h>
#include <zt.h>

static const char *greeting(void) {
  return "hello there";
}

static void test_smoke(zt_test *t) {
  zt_check(t, ZT_TRUE(2 + 2 == 4));
  zt_check(t, ZT_CMP_INT(2 + 2, ==, 4));
  zt_check(t, ZT_CMP_STR(greeting(), ==, "hello there"));
}

static void test_writing_to_tmpfile(zt_test *t) {
  FILE *f = tmpfile();
  zt_assert(t, ZT_NOT_NULL(f)); // stops test on failure
  zt_check(t, ZT_CMP_INT(fprintf(f, "%s", greeting()), >, 0);
  zt_check(t, ZT_CMP_INT(ftell(f), ==, strlen(greeting()));
  zt_check(t, ZT_CMP_INT(fclose(f), ==, 0);
}

static void test_suite(zt_visitor v) {
  ZT_VISIT_TEST(v, test_smoke);
  ZT_VISIT_TEST(v, test_writing_to_tmpfile);
}

int main(int argc, char **argv, char **envp) {
  return zt_main(argc, argv, envp, test_suite);
}
```
