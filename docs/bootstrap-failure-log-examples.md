# Bootstrap Failure Log Examples

## Integrity mismatch (BLAKE3)

```
E/TermuxInstaller: Bootstrap Error:
java.lang.RuntimeException: Bootstrap integrity verification failed (BLAKE3 mismatch). expected=8f9b...a1c2, actual=37d9...4ee0
```

## Missing runtime binary

```
E/TermuxInstaller: Bootstrap Error:
java.lang.RuntimeException: Runtime binary verification failed: missing proot at /data/data/com.termux.rafacodephi/files/usr-staging/bin/proot
```

## Non-executable runtime binary

```
E/TermuxInstaller: Bootstrap Error:
java.lang.RuntimeException: Runtime binary verification failed: busybox is not executable by owner. mode=0100644
```

## Permission application failure

```
E/TermuxInstaller: Bootstrap Error:
java.lang.RuntimeException: Failed to chmod executable bootstrap payload "/data/.../usr-staging/bin/sh" to mode 0700
```

## Build-time hash configuration failure

```
* What went wrong:
Execution failed for task ':app:verifyBootstrapZipsPresent'.
> Missing or invalid TERMUX_BOOTSTRAP_BLAKE3_* values for architectures: aarch64, arm, i686, x86_64.
```

Required variables:
- `TERMUX_BOOTSTRAP_BLAKE3_AARCH64`
- `TERMUX_BOOTSTRAP_BLAKE3_ARM`
- `TERMUX_BOOTSTRAP_BLAKE3_I686`
- `TERMUX_BOOTSTRAP_BLAKE3_X86_64`
