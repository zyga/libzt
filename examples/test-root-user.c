#include <pwd.h>
#include <sys/types.h>

#include <zt.h>

static void test_root_user(zt_t t)
{
    struct passwd* p = getpwnam("root");
    zt_assert(t, ZT_NOT_NULL(p));
    zt_check(t, ZT_CMP_CSTR(p->pw_name, ==, "root"));
    zt_check(t, ZT_CMP_INT(p->pw_uid, ==, 0));
    zt_check(t, ZT_CMP_INT(p->pw_gid, ==, 0));
}

static void test_suite(zt_visitor v) { ZT_VISIT_TEST_CASE(v, test_root_user); }

int main(int argc, char** argv, char** envp)
{
    return zt_main(argc, argv, envp, test_suite);
}
