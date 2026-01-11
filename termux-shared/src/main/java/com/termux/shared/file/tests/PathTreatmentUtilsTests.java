package com.termux.shared.file.tests;

import android.content.Context;

import androidx.annotation.NonNull;

import com.termux.shared.errors.Error;
import com.termux.shared.errors.ISO9001Errno;
import com.termux.shared.file.PathTreatmentUtils;
import com.termux.shared.logger.Logger;

/**
 * Tests for {@link PathTreatmentUtils} and ISO 8000/9001 error handling.
 *
 * Tests path validation, traversal detection, and quality management features.
 */
public class PathTreatmentUtilsTests {

    private static final String LOG_TAG = "PathTreatmentUtilsTests";

    /**
     * Run basic tests for {@link PathTreatmentUtils} class.
     *
     * @param context The {@link Context} for operations.
     * @param testRootDirectoryPath The root directory for testing.
     */
    public static void runTests(@NonNull final Context context, @NonNull final String testRootDirectoryPath) {
        try {
            Logger.logInfo(LOG_TAG, "Running PathTreatmentUtils tests");
            Logger.logInfo(LOG_TAG, "testRootDirectoryPath: \"" + testRootDirectoryPath + "\"");

            runPathValidationTests(testRootDirectoryPath);
            runPathTraversalTests();
            runISO9001ErrnoTests();

            Logger.logInfo(LOG_TAG, "All PathTreatmentUtils tests successful");
        } catch (Exception e) {
            Logger.logErrorExtended(LOG_TAG, e.getMessage());
            Logger.showToast(context, e.getMessage() != null ?
                e.getMessage().replaceAll("(?s)\nFull Error:\n.*", "") : null, true);
        }
    }

    /**
     * Test path validation functionality.
     */
    private static void runPathValidationTests(@NonNull String testRootDirectoryPath) throws Exception {
        Logger.logInfo(LOG_TAG, "Running path validation tests");

        // Test null path
        PathTreatmentUtils.PathValidationResult result = PathTreatmentUtils.validatePath(null);
        assertFalse("Null path should fail validation", result.isValid());
        assertNotNull("Null path should have error", result.getError());

        // Test empty path
        result = PathTreatmentUtils.validatePath("");
        assertFalse("Empty path should fail validation", result.isValid());

        // Test relative path
        result = PathTreatmentUtils.validatePath("relative/path");
        assertFalse("Relative path should fail validation", result.isValid());

        // Test valid absolute path (root)
        result = PathTreatmentUtils.validatePath("/");
        assertTrue("Root path should pass validation", result.isValid());
        assertEqual("Root path canonical should be /", "/", result.getCanonicalPath());

        // Test valid absolute path
        result = PathTreatmentUtils.validatePath("/system");
        assertTrue("/system should pass validation", result.isValid());

        // Test path with null byte (security risk)
        result = PathTreatmentUtils.validatePath("/path\0/with/null");
        assertFalse("Path with null byte should fail validation", result.isValid());

        Logger.logInfo(LOG_TAG, "Path validation tests passed");
    }

    /**
     * Test path traversal detection.
     */
    private static void runPathTraversalTests() throws Exception {
        Logger.logInfo(LOG_TAG, "Running path traversal tests");

        // Test basic traversal patterns
        assertTrue(".. should be detected as traversal",
            PathTreatmentUtils.containsPathTraversal("/path/../file"));

        assertTrue("../ should be detected as traversal",
            PathTreatmentUtils.containsPathTraversal("/path/../../file"));

        assertTrue("..\\/ should be detected as traversal",
            PathTreatmentUtils.containsPathTraversal("/path/..\\file"));

        // Test URL encoded traversal
        assertTrue("%2e%2e should be detected as traversal",
            PathTreatmentUtils.containsPathTraversal("/path/%2e%2e/file"));

        // Test safe paths
        assertFalse("Normal path should not be detected as traversal",
            PathTreatmentUtils.containsPathTraversal("/normal/path/to/file"));

        assertFalse("Path with dots in filename should not be detected as traversal",
            PathTreatmentUtils.containsPathTraversal("/path/file..name"));

        assertFalse("Path with dotfile should not be detected as traversal",
            PathTreatmentUtils.containsPathTraversal("/path/.hidden"));

        // Test path validation with traversal
        PathTreatmentUtils.PathValidationResult result =
            PathTreatmentUtils.validatePath("/etc/../../../etc/passwd");
        assertFalse("Path with traversal should fail validation", result.isValid());
        assertNotNull("Path with traversal should have error", result.getError());
        assertEqual("Error code should be path traversal",
            ISO9001Errno.ERRNO_PATH_TRAVERSAL_DETECTED.getCode(),
            result.getError().getCode());

        Logger.logInfo(LOG_TAG, "Path traversal tests passed");
    }

    /**
     * Test ISO 9001 error codes and categories.
     */
    private static void runISO9001ErrnoTests() throws Exception {
        Logger.logInfo(LOG_TAG, "Running ISO 9001 errno tests");

        // Test error category identification
        assertEqual("Data quality error category",
            "Data Quality (ISO 8000)",
            ISO9001Errno.getErrorCategory(8000));

        assertEqual("Path treatment error category",
            "Path Treatment",
            ISO9001Errno.getErrorCategory(8100));

        assertEqual("Process execution error category",
            "Process Execution",
            ISO9001Errno.getErrorCategory(8200));

        assertEqual("Service lifecycle error category",
            "Service Lifecycle",
            ISO9001Errno.getErrorCategory(8300));

        assertEqual("Dependency management error category",
            "Dependency Management",
            ISO9001Errno.getErrorCategory(8400));

        assertEqual("Permission/access control error category",
            "Permission/Access Control",
            ISO9001Errno.getErrorCategory(8500));

        assertEqual("Resource management error category",
            "Resource Management",
            ISO9001Errno.getErrorCategory(8600));

        assertEqual("Signal handling error category",
            "Signal Handling",
            ISO9001Errno.getErrorCategory(8700));

        assertEqual("Logic/state error category",
            "Logic/State",
            ISO9001Errno.getErrorCategory(8800));

        assertEqual("Package operations error category",
            "Package Operations",
            ISO9001Errno.getErrorCategory(8900));

        assertEqual("Quality compliance error category",
            "Quality Compliance (ISO 9001)",
            ISO9001Errno.getErrorCategory(9000));

        // Test critical error detection
        assertTrue("Path traversal should be critical",
            ISO9001Errno.isCriticalError(8101));

        assertTrue("Permission denied should be critical",
            ISO9001Errno.isCriticalError(8500));

        assertTrue("SELinux denial should be critical",
            ISO9001Errno.isCriticalError(8505));

        assertTrue("Data integrity violation should be critical",
            ISO9001Errno.isCriticalError(8002));

        assertTrue("Resource exhaustion should be critical",
            ISO9001Errno.isCriticalError(8601));

        assertTrue("Bug detection should be critical",
            ISO9001Errno.isCriticalError(8807));

        assertTrue("Non-conformity should be critical",
            ISO9001Errno.isCriticalError(9001));

        assertFalse("Normal error should not be critical",
            ISO9001Errno.isCriticalError(8000));

        // Test recommended action retrieval
        String action = ISO9001Errno.getRecommendedAction(8100);
        assertNotNull("Should have recommended action for 8100", action);
        assertTrue("Action should mention absolute paths",
            action.toLowerCase().contains("absolute"));

        action = ISO9001Errno.getRecommendedAction(8400);
        assertNotNull("Should have recommended action for 8400", action);
        assertTrue("Action should mention pkg install",
            action.toLowerCase().contains("pkg install"));

        // Test error creation
        Error error = ISO9001Errno.ERRNO_PATH_TRAVERSAL_DETECTED.getError("/test/path");
        assertNotNull("Should create error", error);
        assertEqual("Error code should match",
            ISO9001Errno.ERRNO_PATH_TRAVERSAL_DETECTED.getCode(),
            error.getCode());

        Logger.logInfo(LOG_TAG, "ISO 9001 errno tests passed");
    }

    // Test helper methods

    private static void assertTrue(String message, boolean condition) throws Exception {
        if (!condition) {
            throw new Exception("Assertion failed: " + message);
        }
    }

    private static void assertFalse(String message, boolean condition) throws Exception {
        if (condition) {
            throw new Exception("Assertion failed (should be false): " + message);
        }
    }

    private static void assertEqual(String message, String expected, String actual) throws Exception {
        if (expected == null && actual != null) {
            throw new Exception(message + " - expected null but got: " + actual);
        }
        if (expected != null && !expected.equals(actual)) {
            throw new Exception(message + " - expected: \"" + expected + "\" but got: \"" + actual + "\"");
        }
    }

    private static void assertEqual(String message, int expected, int actual) throws Exception {
        if (expected != actual) {
            throw new Exception(message + " - expected: " + expected + " but got: " + actual);
        }
    }

    private static void assertNotNull(String message, Object obj) throws Exception {
        if (obj == null) {
            throw new Exception("Assertion failed (should not be null): " + message);
        }
    }

}
