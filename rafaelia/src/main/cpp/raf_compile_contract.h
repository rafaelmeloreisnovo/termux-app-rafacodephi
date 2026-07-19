#ifndef RAF_COMPILE_CONTRACT_H
#define RAF_COMPILE_CONTRACT_H

/*
 * RAFCODE-Phi compile-time intent contract.
 *
 * Diagnostics classify source intent; they do not delete code by themselves.
 * Dead symbols are removed by the combination:
 *   static/internal linkage + -ffunction-sections/-fdata-sections
 *   + linker --gc-sections.
 *
 * Use RAF_UNUSED only for intentionally optional symbols. Do not use it as a
 * blanket silencer for unknown dead code.
 */

#if defined(__GNUC__) || defined(__clang__)
#  define RAF_UNUSED            __attribute__((unused))
#  define RAF_USED              __attribute__((used))
#  define RAF_NORETURN          __attribute__((noreturn))
#  define RAF_COLD              __attribute__((cold))
#  define RAF_HOT               __attribute__((hot))
#  define RAF_PURE              __attribute__((pure))
#  define RAF_CONST             __attribute__((const))
#  define RAF_ALWAYS_INLINE     inline __attribute__((always_inline))
#  define RAF_NOINLINE          __attribute__((noinline))
#  define RAF_EXPORT            __attribute__((visibility("default")))
#  define RAF_LIKELY(x)         __builtin_expect(!!(x), 1)
#  define RAF_UNLIKELY(x)       __builtin_expect(!!(x), 0)
#  define RAF_UNREACHABLE()     __builtin_unreachable()
#  define RAF_COMPILER_BARRIER() __asm__ __volatile__("" ::: "memory")
#else
#  define RAF_UNUSED
#  define RAF_USED
#  define RAF_NORETURN
#  define RAF_COLD
#  define RAF_HOT
#  define RAF_PURE
#  define RAF_CONST
#  define RAF_ALWAYS_INLINE     inline
#  define RAF_NOINLINE
#  define RAF_EXPORT
#  define RAF_LIKELY(x)         (x)
#  define RAF_UNLIKELY(x)       (x)
#  define RAF_UNREACHABLE()     ((void)0)
#  define RAF_COMPILER_BARRIER() ((void)0)
#endif

/* Explicitly discard a value without generating runtime work. */
#define RAF_DISCARD(expr) ((void)(expr))

/* Intentional terminal loop. The barrier distinguishes it from dead looping. */
#define RAF_SPIN_FOREVER()        \
    do {                          \
        for (;;) {                \
            RAF_COMPILER_BARRIER(); \
        }                         \
    } while (0)

/* Portable diagnostic scopes for narrow, documented exceptions only. */
#define RAF_PRAGMA_IMPL(x) _Pragma(#x)
#define RAF_PRAGMA(x) RAF_PRAGMA_IMPL(x)

#if defined(__clang__)
#  define RAF_DIAGNOSTIC_PUSH() RAF_PRAGMA(clang diagnostic push)
#  define RAF_DIAGNOSTIC_POP() RAF_PRAGMA(clang diagnostic pop)
#  define RAF_DIAGNOSTIC_IGNORE(name) RAF_PRAGMA(clang diagnostic ignored name)
#elif defined(__GNUC__)
#  define RAF_DIAGNOSTIC_PUSH() RAF_PRAGMA(GCC diagnostic push)
#  define RAF_DIAGNOSTIC_POP() RAF_PRAGMA(GCC diagnostic pop)
#  define RAF_DIAGNOSTIC_IGNORE(name) RAF_PRAGMA(GCC diagnostic ignored name)
#else
#  define RAF_DIAGNOSTIC_PUSH()
#  define RAF_DIAGNOSTIC_POP()
#  define RAF_DIAGNOSTIC_IGNORE(name)
#endif

#endif /* RAF_COMPILE_CONTRACT_H */
