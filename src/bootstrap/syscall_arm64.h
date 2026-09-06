#ifndef SYSCALL_ARM64_H
#define SYSCALL_ARM64_H

/*
 * Compatibility include retained for historical callers.
 *
 * The previous file carried a divergent AArch64 syscall table (including
 * open/fork/wait4/gettid values that do not match the AArch64 Linux ABI) and
 * could not compile for armeabi-v7a.  The canonical implementation is now
 * architecture-neutral across the two supported Android ABIs.
 */
#include "syscall_arm.h"

#endif /* SYSCALL_ARM64_H */
