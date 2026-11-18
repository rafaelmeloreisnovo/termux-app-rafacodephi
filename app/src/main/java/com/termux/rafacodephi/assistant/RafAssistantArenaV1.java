package com.termux.rafacodephi.assistant;

/**
 * RAFAELIA assistant: fixed-size, single-writer, Java-only transactional arena.
 *
 * HOSTED_JAVA_ONLY: runs on a JVM or Android ART, never bare metal.
 * All storage is allocated exactly once per instance, before the hot path.
 * No imports, JNI, native library, filesystem, threads or OS calls.
 * A watchdog heartbeat is cooperative: the host must call heartbeat() and
 * separately supervise hangs/crashes. No CPU registers or GPU are probed here.
 *
 * The undo log records whole 256-byte pages on their first write in a transaction;
 * thus rollback is bounded by the number of changed pages. A successful commit
 * makes the modified bytes the new baseline. No durable disk persistence implied.
 */
public final class RafAssistantArenaV1 {
    public static final int SIZE = 65536;
    public static final int PAGE_SIZE = 256;
    public static final int PAGE_COUNT = SIZE / PAGE_SIZE;

    public static final int OK = 0;
    public static final int ERR_STATE = -1;
    public static final int ERR_BOUNDS = -2;
    public static final int ERR_VALUE = -3;
    public static final int ERR_CLOCK = -4;
    public static final int ERR_TIMEOUT = -5;

    private final byte[] data = new byte[SIZE];
    private final byte[] undo = new byte[SIZE];
    private final byte[] marked = new byte[PAGE_COUNT];
    private final int[] dirtyPages = new int[PAGE_COUNT];
    private int dirtyCount;
    private boolean transaction;
    private boolean safeMode;
    private boolean watchdogArmed;
    private long lastBeat;
    private long maxGap;

    public int begin() {
        if (transaction || safeMode) return ERR_STATE;
        transaction = true;
        return OK;
    }

    public int read(int offset) {
        if (offset < 0 || offset >= SIZE) return ERR_BOUNDS;
        return data[offset] & 255;
    }

    public int write(int offset, int value) {
        if (!transaction || safeMode) return ERR_STATE;
        if (offset < 0 || offset >= SIZE) return ERR_BOUNDS;
        if (value < 0 || value > 255) return ERR_VALUE;
        final int page = offset >>> 8;
        if (marked[page] == 0) {
            final int base = page << 8;
            for (int i = 0; i < PAGE_SIZE; ++i) undo[base + i] = data[base + i];
            marked[page] = 1;
            dirtyPages[dirtyCount++] = page; // bounded by unique page count
        }
        data[offset] = (byte) value;
        return OK;
    }

    public int commit() {
        if (!transaction || safeMode) return ERR_STATE;
        clearMarks();
        transaction = false;
        return OK;
    }

    public int rollback() {
        if (!transaction) return ERR_STATE;
        undoChanges();
        return OK;
    }

    private void clearMarks() {
        for (int i = 0; i < dirtyCount; ++i) marked[dirtyPages[i]] = 0;
        dirtyCount = 0;
    }

    private void undoChanges() {
        for (int p = 0; p < dirtyCount; ++p) {
            int page = dirtyPages[p];
            int base = page << 8;
            for (int i = 0; i < PAGE_SIZE; ++i) data[base + i] = undo[base + i];
        }
        clearMarks();
        transaction = false;
    }

    /** Host supplies an already verified monotonic timestamp and duration. */
    public int armWatchdog(long nowNanos, long timeoutNanos) {
        if (safeMode || timeoutNanos <= 0) return ERR_STATE;
        lastBeat = nowNanos;
        maxGap = timeoutNanos;
        watchdogArmed = true;
        return OK;
    }

    /** Cooperative deadline check. On violation, undo pending writes and fail closed. */
    public int heartbeat(long nowNanos) {
        if (!watchdogArmed || safeMode) return ERR_STATE;
        // System.nanoTime() has an arbitrary (possibly negative) origin.
        // Elapsed subtraction also handles a wrap while intervals stay < 2^63 ns.
        final long elapsed = nowNanos - lastBeat;
        if (elapsed < 0) {
            tripSafeMode();
            return ERR_CLOCK;
        }
        if (elapsed > maxGap) {
            tripSafeMode();
            return ERR_TIMEOUT;
        }
        lastBeat = nowNanos;
        return OK;
    }

    public void tripSafeMode() {
        if (transaction) undoChanges();
        safeMode = true;
        watchdogArmed = false;
    }

    public boolean isSafeMode() { return safeMode; }
    public boolean isInTransaction() { return transaction; }
    public int changedPages() { return dirtyCount; }
    // No automatic exit from safe mode: host must instantiate/recover under
    // independent authorization. This object is not a persistent checkpoint.
}
