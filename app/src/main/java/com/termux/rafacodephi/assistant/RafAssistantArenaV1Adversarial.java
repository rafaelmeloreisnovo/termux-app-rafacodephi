package com.termux.rafacodephi.assistant;

/** Dependency-free adversarial tests; host/JVM output is test-only. */
public final class RafAssistantArenaV1Adversarial {
    private RafAssistantArenaV1Adversarial() {}
    private static void check(boolean condition, int caseId) {
        if (!condition) throw new AssertionError("RAFAELIA_ARENA_FAIL_" + caseId);
    }
    public static int run() {
        RafAssistantArenaV1 a = new RafAssistantArenaV1();
        check(a.read(-1) == RafAssistantArenaV1.ERR_BOUNDS, 1);
        check(a.read(RafAssistantArenaV1.SIZE) == RafAssistantArenaV1.ERR_BOUNDS, 2);
        check(a.write(0, 1) == RafAssistantArenaV1.ERR_STATE, 3);
        check(a.commit() == RafAssistantArenaV1.ERR_STATE, 4);
        check(a.begin() == RafAssistantArenaV1.OK, 5);
        check(a.begin() == RafAssistantArenaV1.ERR_STATE, 6);
        check(a.write(0, -1) == RafAssistantArenaV1.ERR_VALUE, 7);
        check(a.write(0, 256) == RafAssistantArenaV1.ERR_VALUE, 8);
        check(a.write(-1, 1) == RafAssistantArenaV1.ERR_BOUNDS, 9);
        check(a.write(RafAssistantArenaV1.SIZE, 1) == RafAssistantArenaV1.ERR_BOUNDS, 10);
        check(a.changedPages() == 0, 11);
        check(a.write(0, 4) == RafAssistantArenaV1.OK, 12);
        check(a.write(255, 7) == RafAssistantArenaV1.OK, 13);
        check(a.changedPages() == 1, 14);
        check(a.write(256, 99) == RafAssistantArenaV1.OK, 15);
        check(a.changedPages() == 2, 16);
        check(a.rollback() == RafAssistantArenaV1.OK, 17);
        check(a.read(0) == 0 && a.read(255) == 0 && a.read(256) == 0, 18);
        check(a.changedPages() == 0 && !a.isInTransaction(), 19);
        check(a.begin() == RafAssistantArenaV1.OK, 20);
        check(a.write(0, 42) == RafAssistantArenaV1.OK, 21);
        check(a.write(RafAssistantArenaV1.SIZE-1, 255) == RafAssistantArenaV1.OK, 22);
        check(a.commit() == RafAssistantArenaV1.OK, 23);
        check(a.read(0) == 42 && a.read(RafAssistantArenaV1.SIZE-1) == 255, 24);
        check(a.begin() == RafAssistantArenaV1.OK, 25);
        // Falsifier: modify every page, then restore committed state.
        for (int page = 0; page < RafAssistantArenaV1.PAGE_COUNT; ++page) {
            check(a.write(page * RafAssistantArenaV1.PAGE_SIZE, page) == RafAssistantArenaV1.OK, 26);
        }
        check(a.changedPages() == RafAssistantArenaV1.PAGE_COUNT, 27);
        check(a.rollback() == RafAssistantArenaV1.OK, 28);
        check(a.read(0) == 42 && a.read(256) == 0, 29);
        check(a.read(RafAssistantArenaV1.SIZE-1) == 255, 30);
        check(a.armWatchdog(100, 50) == RafAssistantArenaV1.OK, 31);
        check(a.heartbeat(150) == RafAssistantArenaV1.OK, 32);
        check(a.begin() == RafAssistantArenaV1.OK, 33);
        check(a.write(0, 9) == RafAssistantArenaV1.OK, 34);
        check(a.heartbeat(201) == RafAssistantArenaV1.ERR_TIMEOUT, 35);
        check(a.isSafeMode() && !a.isInTransaction(), 36);
        check(a.read(0) == 42, 37);
        check(a.write(0, 1) == RafAssistantArenaV1.ERR_STATE, 38);
        check(a.begin() == RafAssistantArenaV1.ERR_STATE, 39);
        RafAssistantArenaV1 b = new RafAssistantArenaV1();
        check(b.armWatchdog(1000, 10) == RafAssistantArenaV1.OK, 40);
        check(b.begin() == RafAssistantArenaV1.OK, 41);
        check(b.write(0, 5) == RafAssistantArenaV1.OK, 42);
        check(b.heartbeat(999) == RafAssistantArenaV1.ERR_CLOCK, 43);
        check(b.isSafeMode() && b.read(0) == 0, 44);
        check(b.armWatchdog(0, 10) == RafAssistantArenaV1.ERR_STATE, 45);
        return 0;
    }
    public static void main(String[] args) {
        int result = run();
        if (result != 0) throw new AssertionError("run=" + result);
        System.out.println("PASS_SCOPED: RafAssistantArenaV1Adversarial (JVM host)");
    }
}
