package com.termux.shared.termux.shell;

import android.content.Context;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.termux.shared.errors.Error;
import com.termux.shared.errors.Errno;
import com.termux.shared.errors.ISO9001Errno;
import com.termux.shared.file.FileUtils;
import com.termux.shared.file.PathTreatmentUtils;
import com.termux.shared.logger.Logger;
import com.termux.shared.shell.command.ExecutionCommand;
import com.termux.shared.termux.TermuxConstants;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * ISO 8000/9001 compliant quality management utilities for Termux shell service.
 *
 * This class provides:
 * - Comprehensive error handling for 49+ error categories
 * - Quality-assured validation for shell commands
 * - Path treatment validation
 * - Dependency checking
 * - Service state management
 *
 * Error Categories handled (based on ISO 9001 error taxonomy):
 * 1. bug - Internal software bugs
 * 2. erro - General errors
 * 3. fail - Operation failures
 * 4. logic - Logic errors
 * 5. kill - Signal handling (SIGKILL)
 * 6. deny - Permission denials
 * 7. path - Path treatment errors
 * 8. exec - Execution errors
 * 9. data - Data quality errors
 * 10. state - State management errors
 * 11-49. And many more...
 */
public class TermuxQualityManager {

    private static final String LOG_TAG = "TermuxQualityManager";

    /**
     * Error category enumeration for quality tracking.
     */
    public enum ErrorCategory {
        BUG("bug", "Internal Bug", ISO9001Errno.ERRNO_BUG_DETECTED),
        ERROR("erro", "General Error", ISO9001Errno.ERRNO_DATA_VALIDATION_FAILED),
        FAIL("fail", "Operation Failure", Errno.ERRNO_FAILED),
        LOGIC("logic", "Logic Error", ISO9001Errno.ERRNO_INVARIANT_VIOLATION),
        KILL("kill", "Signal Kill", ISO9001Errno.ERRNO_SIGKILL_SENT),
        DENY("deny", "Permission Denied", ISO9001Errno.ERRNO_PERMISSION_DENIED),
        PATH("path", "Path Error", ISO9001Errno.ERRNO_INVALID_PATH_FORMAT),
        EXEC("exec", "Execution Error", ISO9001Errno.ERRNO_PROCESS_CREATION_FAILED),
        DATA("data", "Data Quality Error", ISO9001Errno.ERRNO_DATA_INTEGRITY_VIOLATION),
        STATE("state", "State Error", ISO9001Errno.ERRNO_INVALID_STATE_TRANSITION),
        TIMEOUT("timeout", "Timeout Error", ISO9001Errno.ERRNO_PROCESS_EXECUTION_TIMEOUT),
        RESOURCE("resource", "Resource Error", ISO9001Errno.ERRNO_RESOURCE_NOT_AVAILABLE),
        MEMORY("memory", "Memory Error", ISO9001Errno.ERRNO_OUT_OF_MEMORY),
        PERMISSION("permission", "Permission Error", ISO9001Errno.ERRNO_RUNTIME_PERMISSION_NOT_GRANTED),
        CONFIG("config", "Configuration Error", ISO9001Errno.ERRNO_CONFIGURATION_MANAGEMENT_ERROR),
        DEPENDENCY("dependency", "Dependency Error", ISO9001Errno.ERRNO_DEPENDENCY_NOT_FOUND),
        BOOTSTRAP("bootstrap", "Bootstrap Error", ISO9001Errno.ERRNO_BOOTSTRAP_INCOMPLETE),
        SERVICE("service", "Service Error", ISO9001Errno.ERRNO_SERVICE_START_FAILED),
        SESSION("session", "Session Error", ISO9001Errno.ERRNO_SESSION_TERMINATED_UNEXPECTEDLY),
        SIGNAL("signal", "Signal Error", ISO9001Errno.ERRNO_SIGNAL_SEND_FAILED),
        PROCESS("process", "Process Error", ISO9001Errno.ERRNO_PROCESS_INTERRUPTED),
        SHELL("shell", "Shell Error", ISO9001Errno.ERRNO_SHELL_ENVIRONMENT_SETUP_FAILED),
        ENVIRONMENT("environment", "Environment Error", ISO9001Errno.ERRNO_SHELL_ENVIRONMENT_SETUP_FAILED),
        SYMLINK("symlink", "Symlink Error", ISO9001Errno.ERRNO_SYMLINK_LOOP_DETECTED),
        TRAVERSAL("traversal", "Path Traversal", ISO9001Errno.ERRNO_PATH_TRAVERSAL_DETECTED),
        VALIDATION("validation", "Validation Error", ISO9001Errno.ERRNO_DATA_VALIDATION_FAILED),
        INTEGRITY("integrity", "Integrity Error", ISO9001Errno.ERRNO_DATA_INTEGRITY_VIOLATION),
        PACKAGE("package", "Package Error", ISO9001Errno.ERRNO_PACKAGE_NOT_FOUND),
        REPOSITORY("repository", "Repository Error", ISO9001Errno.ERRNO_REPOSITORY_UPDATE_FAILED),
        INSTALL("install", "Installation Error", ISO9001Errno.ERRNO_PACKAGE_INSTALL_FAILED),
        UPDATE("update", "Update Error", ISO9001Errno.ERRNO_PACKAGE_UPDATE_FAILED),
        REMOVE("remove", "Removal Error", ISO9001Errno.ERRNO_PACKAGE_REMOVE_FAILED),
        CONFLICT("conflict", "Conflict Error", ISO9001Errno.ERRNO_PACKAGE_CONFLICT),
        STORAGE("storage", "Storage Error", ISO9001Errno.ERRNO_INSUFFICIENT_STORAGE_FOR_PACKAGE),
        NETWORK("network", "Network Error", ISO9001Errno.ERRNO_RESOURCE_NOT_AVAILABLE),
        LIBRARY("library", "Library Error", ISO9001Errno.ERRNO_LIBRARY_LOAD_FAILED),
        CIRCULAR("circular", "Circular Dependency", ISO9001Errno.ERRNO_CIRCULAR_DEPENDENCY),
        VERSION("version", "Version Mismatch", ISO9001Errno.ERRNO_DEPENDENCY_VERSION_MISMATCH),
        ACCESS("access", "Access Error", ISO9001Errno.ERRNO_ACCESS_DENIED_BY_POLICY),
        AUTH("auth", "Authentication Error", ISO9001Errno.ERRNO_AUTHENTICATION_FAILED),
        SELINUX("selinux", "SELinux Error", ISO9001Errno.ERRNO_SELINUX_DENIAL),
        WAKELOCK("wakelock", "WakeLock Error", ISO9001Errno.ERRNO_WAKE_LOCK_ACQUISITION_FAILED),
        ALLOCATION("allocation", "Allocation Error", ISO9001Errno.ERRNO_RESOURCE_ALLOCATION_FAILED),
        RELEASE("release", "Release Error", ISO9001Errno.ERRNO_RESOURCE_RELEASE_FAILED),
        EXHAUSTED("exhausted", "Resource Exhausted", ISO9001Errno.ERRNO_RESOURCE_EXHAUSTED),
        PRECONDITION("precondition", "Precondition Failed", ISO9001Errno.ERRNO_PRECONDITION_FAILED),
        POSTCONDITION("postcondition", "Postcondition Failed", ISO9001Errno.ERRNO_POSTCONDITION_FAILED),
        ASSERTION("assertion", "Assertion Failed", ISO9001Errno.ERRNO_ASSERTION_FAILED),
        QUALITY("quality", "Quality Check Failed", ISO9001Errno.ERRNO_QUALITY_CHECK_FAILED);

        private final String key;
        private final String description;
        private final Errno defaultErrno;

        ErrorCategory(String key, String description, Errno defaultErrno) {
            this.key = key;
            this.description = description;
            this.defaultErrno = defaultErrno;
        }

        public String getKey() { return key; }
        public String getDescription() { return description; }
        public Errno getDefaultErrno() { return defaultErrno; }

        @Nullable
        public static ErrorCategory fromKey(String key) {
            for (ErrorCategory category : values()) {
                if (category.key.equalsIgnoreCase(key)) {
                    return category;
                }
            }
            return null;
        }
    }

    // Error statistics tracking
    private static final Map<ErrorCategory, Integer> errorCounts = new HashMap<>();
    private static final List<QualityViolation> recentViolations = new ArrayList<>();
    private static final int MAX_RECENT_VIOLATIONS = 100;

    /**
     * Quality violation record for ISO 9001 audit trail.
     */
    public static class QualityViolation {
        public final long timestamp;
        public final ErrorCategory category;
        public final String message;
        public final String context;
        public final int errorCode;

        public QualityViolation(ErrorCategory category, String message, String context, int errorCode) {
            this.timestamp = System.currentTimeMillis();
            this.category = category;
            this.message = message;
            this.context = context;
            this.errorCode = errorCode;
        }

        @NonNull
        @Override
        public String toString() {
            return String.format("[%d] %s (%d): %s - %s",
                timestamp, category.getDescription(), errorCode, message, context);
        }
    }

    /**
     * Validate an execution command according to ISO 8000/9001 quality standards.
     *
     * @param context The Android context
     * @param executionCommand The command to validate
     * @return Error if validation fails, null if successful
     */
    @Nullable
    public static Error validateExecutionCommand(
            @NonNull Context context,
            @NonNull ExecutionCommand executionCommand) {

        // Validate executable path
        if (executionCommand.executable != null && !executionCommand.executable.isEmpty()) {
            PathTreatmentUtils.PathValidationResult execResult =
                PathTreatmentUtils.validateExecutablePath(executionCommand.executable);
            if (!execResult.isValid()) {
                recordViolation(ErrorCategory.EXEC, "Executable validation failed",
                    executionCommand.executable, execResult.getError().getCode());
                return execResult.getError();
            }
            // Update to canonical path
            executionCommand.executable = execResult.getCanonicalPath();
        }

        // Validate working directory
        if (executionCommand.workingDirectory != null && !executionCommand.workingDirectory.isEmpty()) {
            PathTreatmentUtils.PathValidationResult wdResult =
                PathTreatmentUtils.validateWorkingDirectory(
                    executionCommand.workingDirectory,
                    TermuxConstants.TERMUX_HOME_DIR_PATH);
            if (!wdResult.isValid()) {
                recordViolation(ErrorCategory.PATH, "Working directory validation failed",
                    executionCommand.workingDirectory, wdResult.getError().getCode());
                return wdResult.getError();
            }
            // Update to canonical path
            executionCommand.workingDirectory = wdResult.getCanonicalPath();
        }

        // Validate arguments for path traversal
        if (executionCommand.arguments != null) {
            for (int i = 0; i < executionCommand.arguments.length; i++) {
                String arg = executionCommand.arguments[i];
                if (arg != null && PathTreatmentUtils.containsPathTraversal(arg)) {
                    // Only error if it looks like a path (starts with path-like prefix)
                    if (looksLikePath(arg)) {
                        recordViolation(ErrorCategory.TRAVERSAL, "Potential path traversal in argument",
                            "Arg " + i + ": " + PathTreatmentUtils.sanitizeForLogging(arg),
                            ISO9001Errno.ERRNO_PATH_TRAVERSAL_DETECTED.getCode());
                        return ISO9001Errno.ERRNO_PATH_TRAVERSAL_DETECTED.getError(
                            PathTreatmentUtils.sanitizeForLogging(arg));
                    }
                }
            }
        }

        // Validate runner
        if (executionCommand.runner != null) {
            ExecutionCommand.Runner runner = ExecutionCommand.Runner.runnerOf(executionCommand.runner);
            if (runner == null) {
                recordViolation(ErrorCategory.VALIDATION, "Invalid runner specified",
                    executionCommand.runner, ISO9001Errno.ERRNO_DATA_VALIDATION_FAILED.getCode());
                return ISO9001Errno.ERRNO_DATA_VALIDATION_FAILED.getError(
                    "runner", executionCommand.runner + " is not a valid runner");
            }
        }

        return null;
    }

    /**
     * Check if the Termux bootstrap is complete.
     *
     * @return Error if bootstrap is incomplete, null otherwise
     */
    @Nullable
    public static Error checkBootstrapComplete() {
        List<String> missingFiles = new ArrayList<>();

        // Check critical Termux paths
        String[] criticalPaths = {
            TermuxConstants.TERMUX_PREFIX_DIR_PATH,
            TermuxConstants.TERMUX_BIN_PREFIX_DIR_PATH,
            TermuxConstants.TERMUX_HOME_DIR_PATH
        };

        for (String path : criticalPaths) {
            if (!FileUtils.directoryFileExists(path, false)) {
                missingFiles.add(path);
            }
        }

        // Check for sh binary
        String shPath = TermuxConstants.TERMUX_BIN_PREFIX_DIR_PATH + "/sh";
        if (!FileUtils.regularFileExists(shPath, true)) {
            missingFiles.add(shPath);
        }

        if (!missingFiles.isEmpty()) {
            recordViolation(ErrorCategory.BOOTSTRAP, "Bootstrap incomplete",
                String.join(", ", missingFiles), ISO9001Errno.ERRNO_BOOTSTRAP_INCOMPLETE.getCode());
            return ISO9001Errno.ERRNO_BOOTSTRAP_INCOMPLETE.getError(String.join(", ", missingFiles));
        }

        return null;
    }

    /**
     * Validate dependencies for shell execution.
     *
     * @param context The Android context
     * @param requiredBinaries List of required binary names (e.g., "bash", "python")
     * @return Error if dependencies missing, null otherwise
     */
    @Nullable
    public static Error checkDependencies(
            @NonNull Context context,
            @NonNull List<String> requiredBinaries) {

        List<String> missing = new ArrayList<>();
        String binDir = TermuxConstants.TERMUX_BIN_PREFIX_DIR_PATH;

        for (String binary : requiredBinaries) {
            String fullPath = binDir + "/" + binary;
            if (!FileUtils.regularFileExists(fullPath, true)) {
                missing.add(binary);
            }
        }

        if (!missing.isEmpty()) {
            String missingStr = String.join(", ", missing);
            recordViolation(ErrorCategory.DEPENDENCY, "Dependencies not found",
                missingStr, ISO9001Errno.ERRNO_DEPENDENCY_NOT_FOUND.getCode());
            return ISO9001Errno.ERRNO_DEPENDENCY_NOT_FOUND.getError(missingStr);
        }

        return null;
    }

    /**
     * Record a quality violation for ISO 9001 audit trail.
     *
     * @param category The error category
     * @param message The error message
     * @param context Additional context
     * @param errorCode The error code
     */
    public static synchronized void recordViolation(
            @NonNull ErrorCategory category,
            @NonNull String message,
            @Nullable String context,
            int errorCode) {

        // Update error counts using getOrDefault for cleaner code
        errorCounts.put(category, errorCounts.getOrDefault(category, 0) + 1);

        // Add to recent violations
        QualityViolation violation = new QualityViolation(category, message, context, errorCode);
        recentViolations.add(violation);

        // Trim if exceeds max
        while (recentViolations.size() > MAX_RECENT_VIOLATIONS) {
            recentViolations.remove(0);
        }

        // Log the violation
        Logger.logWarn(LOG_TAG, "Quality Violation: " + violation);

        // Check if critical
        if (ISO9001Errno.isCriticalError(errorCode)) {
            Logger.logError(LOG_TAG, "CRITICAL: " + category.getDescription() +
                " - " + message + " (Code: " + errorCode + ")");
        }
    }

    /**
     * Check if a string looks like a path (starts with common path prefixes).
     *
     * @param str The string to check
     * @return true if it looks like a path
     */
    private static boolean looksLikePath(@Nullable String str) {
        if (str == null || str.isEmpty()) return false;
        return str.startsWith("/") || str.startsWith("./") || str.startsWith("../");
    }

    /**
     * Get error statistics for quality reporting.
     *
     * @return Map of error category to count
     */
    @NonNull
    public static synchronized Map<ErrorCategory, Integer> getErrorStatistics() {
        return new HashMap<>(errorCounts);
    }

    /**
     * Get recent quality violations for audit.
     *
     * @return List of recent violations
     */
    @NonNull
    public static synchronized List<QualityViolation> getRecentViolations() {
        return new ArrayList<>(recentViolations);
    }

    /**
     * Reset error statistics.
     */
    public static synchronized void resetStatistics() {
        errorCounts.clear();
        recentViolations.clear();
    }

    /**
     * Generate a quality report summary.
     *
     * @return Quality report string
     */
    @NonNull
    public static String generateQualityReport() {
        StringBuilder report = new StringBuilder();
        report.append("=== Termux Quality Report (ISO 9001) ===\n\n");

        report.append("Error Statistics:\n");
        Map<ErrorCategory, Integer> stats = getErrorStatistics();
        if (stats.isEmpty()) {
            report.append("  No errors recorded.\n");
        } else {
            for (Map.Entry<ErrorCategory, Integer> entry : stats.entrySet()) {
                report.append(String.format("  %s (%s): %d\n",
                    entry.getKey().getDescription(),
                    entry.getKey().getKey(),
                    entry.getValue()));
            }
        }

        report.append("\nRecent Violations (last ").append(MAX_RECENT_VIOLATIONS).append("):\n");
        List<QualityViolation> violations = getRecentViolations();
        if (violations.isEmpty()) {
            report.append("  No recent violations.\n");
        } else {
            for (int i = Math.max(0, violations.size() - 10); i < violations.size(); i++) {
                report.append("  ").append(violations.get(i).toString()).append("\n");
            }
            if (violations.size() > 10) {
                report.append("  ... and ").append(violations.size() - 10).append(" more.\n");
            }
        }

        return report.toString();
    }

    /**
     * Create an error for a specific category with context.
     *
     * @param category The error category
     * @param context Additional context
     * @param details Error details
     * @return Error object
     */
    @NonNull
    public static Error createError(
            @NonNull ErrorCategory category,
            @Nullable String context,
            @Nullable String details) {

        Errno errno = category.getDefaultErrno();
        Error error;

        if (context != null && details != null) {
            error = errno.getError(context, details);
        } else if (context != null) {
            error = errno.getError(context);
        } else {
            error = errno.getError();
        }

        recordViolation(category, error.getMessage(), context, errno.getCode());
        return error;
    }

    /**
     * Create and record an error for a process kill signal.
     * Records the signal as a quality violation and returns an error for reporting.
     *
     * @param pid The process ID
     * @param signal The signal name (e.g., "SIGKILL")
     * @param processName The process name
     * @return Error object representing the signal event
     */
    @NonNull
    public static Error createAndRecordKillSignalError(int pid, @NonNull String signal, @Nullable String processName) {
        ErrorCategory category = signal.equals("SIGKILL") ? ErrorCategory.KILL : ErrorCategory.SIGNAL;

        String message = String.format("Signal %s sent to PID %d (%s)",
            signal, pid, processName != null ? processName : "unknown");

        Errno errno = signal.equals("SIGKILL") ?
            ISO9001Errno.ERRNO_SIGKILL_SENT :
            ISO9001Errno.ERRNO_SIGTERM_SENT;

        recordViolation(category, message, "PID: " + pid, errno.getCode());

        return errno.getError(
            processName != null ? processName : "process",
            pid,
            "Service shutdown or user request");
    }

    /**
     * Get recommended action for an error.
     *
     * @param errorCode The error code
     * @return Recommended action string, or null if none available
     */
    @Nullable
    public static String getRecommendedAction(int errorCode) {
        return ISO9001Errno.getRecommendedAction(errorCode);
    }

}
