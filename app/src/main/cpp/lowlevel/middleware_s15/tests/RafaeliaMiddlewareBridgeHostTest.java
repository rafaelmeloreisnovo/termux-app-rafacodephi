import com.termux.app.rafaelia.RafaeliaMiddlewareBridge;

/* HOST_ONLY: ensures the Java facade fails closed without a native .so.
 * This is not a substitute for an Android/ART/native integration test.
 */
public final class RafaeliaMiddlewareBridgeHostTest {
    private static int checks;
    private static void eq(int want,int got) {
        if (want!=got) throw new AssertionError("S15 negative JNI facade check " + checks + ": " + got);
        checks++;
    }
    public static void main(String[] args) {
        int[] a=new int[16], b=new int[16];
        eq(RafaeliaMiddlewareBridge.INVALID_BUFFER,
           RafaeliaMiddlewareBridge.plan(null,b));
        eq(RafaeliaMiddlewareBridge.INVALID_BUFFER,
           RafaeliaMiddlewareBridge.plan(a,null));
        eq(RafaeliaMiddlewareBridge.ALIASED_BUFFER,
           RafaeliaMiddlewareBridge.plan(a,a));
        eq(RafaeliaMiddlewareBridge.WRONG_FRAME,
           RafaeliaMiddlewareBridge.plan(new int[15],b));
        eq(RafaeliaMiddlewareBridge.WRONG_FRAME,
           RafaeliaMiddlewareBridge.plan(a,new int[17]));
        if (RafaeliaMiddlewareBridge.isAvailable())
            throw new AssertionError("unexpected native library on host");
        eq(RafaeliaMiddlewareBridge.NATIVE_LIBRARY_UNAVAILABLE,
           RafaeliaMiddlewareBridge.plan(a,b));
        if (b[14] != 0) throw new AssertionError("unexpected EXECUTED bit");
        System.out.println("PASS HOST JAVA: " + checks + " negative cases (NOT Android runtime)");
    }
}
