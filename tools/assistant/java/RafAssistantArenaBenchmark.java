package com.termux.rafacodephi.assistant;

/**
 * Hosted experimental benchmark, NOT a backend-speed claim.
 * Both paths perform the SAME write/rollback sequence and end with zeroed data.
 * Baseline copies an entire fixed arena per transaction; alternative snapshots
 * only the first touched page (RafAssistantArenaV1). Single thread, same seed.
 */
public final class RafAssistantArenaBenchmark {
    private static volatile int sink;
    private RafAssistantArenaBenchmark() {}

    private static int[] offsets(int count) {
        int[] result = new int[count];
        for (int i = 0; i < count; ++i) {
            // Distinct pages, same indices for both implementations.
            result[i] = (i * RafAssistantArenaV1.PAGE_SIZE) + ((i * 71 + 13) & 255);
        }
        return result;
    }

    private static int runPageLog(RafAssistantArenaV1 arena, int[] offsets, int transactions) {
        for (int t = 0; t < transactions; ++t) {
            if (arena.begin() != RafAssistantArenaV1.OK) throw new AssertionError("begin");
            for (int i = 0; i < offsets.length; ++i) {
                if (arena.write(offsets[i], (i + t) & 255) != RafAssistantArenaV1.OK)
                    throw new AssertionError("write");
            }
            if (arena.rollback() != RafAssistantArenaV1.OK) throw new AssertionError("rollback");
        }
        int checksum = 0;
        for (int i = 0; i < offsets.length; ++i) checksum |= arena.read(offsets[i]);
        return checksum;
    }

    private static int runFullCopy(byte[] arena, byte[] undo, int[] offsets, int transactions) {
        for (int t = 0; t < transactions; ++t) {
            System.arraycopy(arena, 0, undo, 0, arena.length);
            for (int i = 0; i < offsets.length; ++i) arena[offsets[i]] = (byte) ((i + t) & 255);
            System.arraycopy(undo, 0, arena, 0, arena.length);
        }
        int checksum = 0;
        for (int i = 0; i < offsets.length; ++i) checksum |= arena[offsets[i]] & 255;
        return checksum;
    }

    public static void main(String[] args) {
        if (RafAssistantArenaV1Adversarial.run() != 0) throw new AssertionError("correctness");
        System.out.println("scope=HOSTED_JAVA_PAGE_ROLLBACK_MICROBENCHMARK;claim_allowed=false");
        System.out.println("arch=" + System.getProperty("os.arch") + ";vm="
                + System.getProperty("java.vm.name"));
        System.out.println("count,round,impl,transactions,writes_per_transaction,elapsed_ns,checksum");
        for (int count : new int[]{2, 256}) {
            int transactions = (count == 2 ? 2048 : 64);
            int[] indexes = offsets(count);
            RafAssistantArenaV1 arena = new RafAssistantArenaV1();
            byte[] full = new byte[RafAssistantArenaV1.SIZE];
            byte[] undo = new byte[RafAssistantArenaV1.SIZE];
            for (int warm = 0; warm < 4; ++warm) {
                sink = runPageLog(arena, indexes, transactions);
                sink = runFullCopy(full, undo, indexes, transactions);
            }
            for (int round = 0; round < 9; ++round) {
                for (int pass = 0; pass < 2; ++pass) {
                    boolean page = ((round + pass) & 1) == 0;
                    long started = System.nanoTime();
                    int result = page ? runPageLog(arena, indexes, transactions)
                            : runFullCopy(full, undo, indexes, transactions);
                    long elapsed = System.nanoTime() - started;
                    sink = result;
                    if (result != 0) throw new AssertionError("rollback-mismatch");
                    System.out.println(count + "," + round + "," +
                            (page ? "JAVA_PAGE_LOG" : "JAVA_FULL_SNAPSHOT") + "," +
                            transactions + "," + count + "," + elapsed + "," + result);
                }
            }
        }
    }
}
