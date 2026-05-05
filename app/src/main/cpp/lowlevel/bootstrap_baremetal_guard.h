#ifndef TERMUX_BOOTSTRAP_BAREMETAL_GUARD_H
#define TERMUX_BOOTSTRAP_BAREMETAL_GUARD_H

int raf_bootstrap_guard_selftest(char* out_json, int cap);
int raf_bootstrap_guard_validate_prefix(const char* prefix, char* out_json, int cap);
int raf_bootstrap_guard_check_required_bins(const char* prefix);
int raf_bootstrap_guard_check_exec(const char* path);
int raf_bootstrap_guard_check_page_size(void);
int raf_bootstrap_guard_check_basic_dirs(const char* prefix);

#endif
