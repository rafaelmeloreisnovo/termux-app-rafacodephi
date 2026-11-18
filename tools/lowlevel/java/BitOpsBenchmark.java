package com.termux.rafacodephi.lowlevel;

/** Hosted measurement harness; no measurement is implied by its existence. */
public final class BitOpsBenchmark {
    private static volatile int sink;
    private BitOpsBenchmark() {}

    private static int kernel(int[] input, boolean reference) {
        int sum = 0;
        if (reference) {
            for (int r = 0; r < 64; r++)
                for (int v : input) sum += Integer.bitCount(v);
        } else {
            for (int r = 0; r < 64; r++)
                for (int v : input) sum += AuthorialBitOps.count(v);
        }
        return sum;
    }

    public static void main(String[] args) {
        if (AuthorialBitOpsAdversarial.run() != 0)
            throw new AssertionError("correctness gate failed");
        int[] input = new int[4096]; // 16 KiB; allocations outside timed region.
        int x = 0x13579BDF;
        for (int i = 0; i < input.length; i++) {
            x ^= x << 13; x ^= x >>> 17; x ^= x << 5; input[i] = x;
        }
        int expected = kernel(input, true);
        if (kernel(input, false) != expected) throw new AssertionError("checksum");
        for (int w = 0; w < 5; w++) {
            sink = kernel(input, true); sink = kernel(input, false);
        }
        System.out.println("scope=LOCAL_RUNTIME_MICROBENCHMARK;claim_allowed=false");
        System.out.println("arch=" + System.getProperty("os.arch")
                + ";vm=" + System.getProperty("java.vm.name")
                + ";java=" + System.getProperty("java.version"));
        System.out.println("seed=0x13579BDF;working_set_bytes=16384;warmup_pairs=5;rounds=9;clock=System.nanoTime");
        System.out.println("round,backend,words,elapsed_ns,checksum");
        for (int round = 0; round < 9; round++) {
            for (int turn = 0; turn < 2; turn++) {
                boolean reference = ((round + turn) & 1) == 0;
                long start = System.nanoTime();
                int result = kernel(input, reference);
                long elapsed = System.nanoTime() - start;
                sink = result;
                if (result != expected) throw new AssertionError("checksum");
                System.out.println(round + "," + (reference ? "JAVA_INTEGER_BITCOUNT" : "JAVA_PURE_SWAR")
                        + ",262144," + elapsed + "," + result);
            }
        }
    }
}
