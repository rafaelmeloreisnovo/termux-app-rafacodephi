#include <stdio.h>
#include <string.h>

#include "../app/src/main/cpp/lowlevel/bootstrap_baremetal_guard.h"

int main(void) {
    char out[512];
    int rc;

    rc = raf_bootstrap_guard_validate_prefix(NULL, out, sizeof(out));
    printf("null_prefix_rc=%d\n", rc);

    rc = raf_bootstrap_guard_validate_prefix("", out, sizeof(out));
    printf("empty_prefix_rc=%d\n", rc);

    rc = raf_bootstrap_guard_validate_prefix("/tmp/fake/files/usr", out, sizeof(out));
    printf("fake_prefix_rc=%d json=%s\n", rc, out);

    rc = raf_bootstrap_guard_check_page_size();
    printf("page_size=%d\n", rc);

    rc = raf_bootstrap_guard_validate_prefix("/tmp/fake/files/usr", out, 32);
    printf("small_buffer_rc=%d\n", rc);

    rc = raf_bootstrap_guard_selftest(out, sizeof(out));
    printf("selftest_rc=%d json=%s\n", rc, out);

    return 0;
}
