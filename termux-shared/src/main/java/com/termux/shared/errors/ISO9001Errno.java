package com.termux.shared.errors;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

/**
 * ISO 8000/9001 compliant error codes for quality management in Termux service.
 *
 * ISO 8000 (Data Quality) - Defines data quality requirements and processes
 * ISO 9001 (Quality Management System) - Defines quality management requirements
 *
 * Error Code Ranges:
 * - 8000-8099: Data Quality Errors (ISO 8000)
 * - 8100-8199: Path Treatment Errors
 * - 8200-8299: Process Execution Errors
 * - 8300-8399: Service Lifecycle Errors
 * - 8400-8499: Dependency Management Errors
 * - 8500-8599: Permission and Access Control Errors
 * - 8600-8699: Resource Management Errors
 * - 8700-8799: Signal Handling Errors (kill, deny, etc.)
 * - 8800-8899: Logic and State Errors
 * - 8900-8999: Package (pkg) Operation Errors
 * - 9000-9099: Quality Compliance Errors (ISO 9001)
 *
 * This class follows ISO 9001:2015 Section 8.5.2 - Identification and Traceability
 * Each error must be:
 * - Uniquely identifiable
 * - Traceable to its source
 * - Documented with cause and recommended action
 */
public class ISO9001Errno extends Errno {

    public static final String TYPE = "ISO9001 Error";

    /*
     * ISO 8000 - Data Quality Errors (8000-8099)
     * Section 8000: Master Data Quality Requirements
     */

    /** Invalid input data format - ISO 8000 compliance error */
    public static final Errno ERRNO_INVALID_DATA_FORMAT = new Errno(TYPE, 8000,
        "Invalid data format: %1$s. Expected: %2$s");

    /** Null or empty required data - ISO 8000 compliance error */
    public static final Errno ERRNO_REQUIRED_DATA_MISSING = new Errno(TYPE, 8001,
        "Required data missing: %1$s in operation %2$s");

    /** Data integrity violation - ISO 8000 compliance error */
    public static final Errno ERRNO_DATA_INTEGRITY_VIOLATION = new Errno(TYPE, 8002,
        "Data integrity violation detected in %1$s: %2$s");

    /** Data validation failed - ISO 8000 compliance error */
    public static final Errno ERRNO_DATA_VALIDATION_FAILED = new Errno(TYPE, 8003,
        "Data validation failed for %1$s: %2$s");

    /** Inconsistent data state - ISO 8000 compliance error */
    public static final Errno ERRNO_INCONSISTENT_DATA_STATE = new Errno(TYPE, 8004,
        "Inconsistent data state detected in %1$s: expected %2$s, found %3$s");


    /*
     * Path Treatment Errors (8100-8199)
     * Section 8100: Path Validation and Resolution
     */

    /** Invalid path format - Path treatment error */
    public static final Errno ERRNO_INVALID_PATH_FORMAT = new Errno(TYPE, 8100,
        "Invalid path format: \"%1$s\". Path must be absolute and well-formed.");

    /** Path traversal attempt detected - Security error */
    public static final Errno ERRNO_PATH_TRAVERSAL_DETECTED = new Errno(TYPE, 8101,
        "Path traversal attempt detected in: \"%1$s\". Access denied for security reasons.");

    /** Path not within allowed directory - Security error */
    public static final Errno ERRNO_PATH_NOT_IN_ALLOWED_DIR = new Errno(TYPE, 8102,
        "Path \"%1$s\" is not within allowed directory \"%2$s\".");

    /** Path resolution failed - Path treatment error */
    public static final Errno ERRNO_PATH_RESOLUTION_FAILED = new Errno(TYPE, 8103,
        "Failed to resolve path \"%1$s\": %2$s");

    /** Symbolic link resolution limit exceeded - Path treatment error */
    public static final Errno ERRNO_SYMLINK_LOOP_DETECTED = new Errno(TYPE, 8104,
        "Symbolic link resolution limit exceeded for path \"%1$s\". Possible symlink loop.");

    /** Path canonicalization failed - Path treatment error */
    public static final Errno ERRNO_PATH_CANONICALIZATION_FAILED = new Errno(TYPE, 8105,
        "Failed to canonicalize path \"%1$s\": %2$s");

    /** Working directory invalid - Path treatment error */
    public static final Errno ERRNO_INVALID_WORKING_DIRECTORY = new Errno(TYPE, 8106,
        "Invalid working directory \"%1$s\": %2$s");


    /*
     * Process Execution Errors (8200-8299)
     * Section 8200: Shell and Process Execution Quality
     */

    /** Executable not found - Execution error */
    public static final Errno ERRNO_EXECUTABLE_NOT_FOUND = new Errno(TYPE, 8200,
        "Executable not found at path \"%1$s\".");

    /** Executable not executable - Permission error */
    public static final Errno ERRNO_EXECUTABLE_NOT_EXECUTABLE = new Errno(TYPE, 8201,
        "File at path \"%1$s\" is not executable. Check permissions.");

    /** Process creation failed - Execution error */
    public static final Errno ERRNO_PROCESS_CREATION_FAILED = new Errno(TYPE, 8202,
        "Failed to create process for command \"%1$s\": %2$s");

    /** Process execution timeout - Execution error */
    public static final Errno ERRNO_PROCESS_EXECUTION_TIMEOUT = new Errno(TYPE, 8203,
        "Process execution timeout for command \"%1$s\" after %2$d milliseconds.");

    /** Process execution interrupted - Execution error */
    public static final Errno ERRNO_PROCESS_INTERRUPTED = new Errno(TYPE, 8204,
        "Process execution interrupted for command \"%1$s\": %2$s");

    /** Shell environment setup failed - Execution error */
    public static final Errno ERRNO_SHELL_ENVIRONMENT_SETUP_FAILED = new Errno(TYPE, 8205,
        "Failed to setup shell environment for \"%1$s\": %2$s");

    /** Invalid arguments - Execution error */
    public static final Errno ERRNO_INVALID_COMMAND_ARGUMENTS = new Errno(TYPE, 8206,
        "Invalid command arguments for \"%1$s\": %2$s");

    /** Shebang parsing failed - Execution error */
    public static final Errno ERRNO_SHEBANG_PARSING_FAILED = new Errno(TYPE, 8207,
        "Failed to parse shebang for script \"%1$s\": %2$s");


    /*
     * Service Lifecycle Errors (8300-8399)
     * Section 8300: Service Management Quality
     */

    /** Service already running - Lifecycle error */
    public static final Errno ERRNO_SERVICE_ALREADY_RUNNING = new Errno(TYPE, 8300,
        "Service \"%1$s\" is already running. Cannot start multiple instances.");

    /** Service not running - Lifecycle error */
    public static final Errno ERRNO_SERVICE_NOT_RUNNING = new Errno(TYPE, 8301,
        "Service \"%1$s\" is not running. Cannot perform requested operation.");

    /** Service start failed - Lifecycle error */
    public static final Errno ERRNO_SERVICE_START_FAILED = new Errno(TYPE, 8302,
        "Failed to start service \"%1$s\": %2$s");

    /** Service stop failed - Lifecycle error */
    public static final Errno ERRNO_SERVICE_STOP_FAILED = new Errno(TYPE, 8303,
        "Failed to stop service \"%1$s\": %2$s");

    /** Service bind failed - Lifecycle error */
    public static final Errno ERRNO_SERVICE_BIND_FAILED = new Errno(TYPE, 8304,
        "Failed to bind to service \"%1$s\": %2$s");

    /** Service foreground transition failed - Lifecycle error */
    public static final Errno ERRNO_FOREGROUND_TRANSITION_FAILED = new Errno(TYPE, 8305,
        "Failed to transition service to foreground: %1$s");

    /** Invalid service state - Lifecycle error */
    public static final Errno ERRNO_INVALID_SERVICE_STATE = new Errno(TYPE, 8306,
        "Invalid service state transition from \"%1$s\" to \"%2$s\".");


    /*
     * Dependency Management Errors (8400-8499)
     * Section 8400: Dependency Resolution Quality
     */

    /** Dependency not found - Dependency error */
    public static final Errno ERRNO_DEPENDENCY_NOT_FOUND = new Errno(TYPE, 8400,
        "Required dependency \"%1$s\" not found.");

    /** Dependency version mismatch - Dependency error */
    public static final Errno ERRNO_DEPENDENCY_VERSION_MISMATCH = new Errno(TYPE, 8401,
        "Dependency \"%1$s\" version mismatch. Required: %2$s, Found: %3$s");

    /** Dependency initialization failed - Dependency error */
    public static final Errno ERRNO_DEPENDENCY_INIT_FAILED = new Errno(TYPE, 8402,
        "Failed to initialize dependency \"%1$s\": %2$s");

    /** Circular dependency detected - Dependency error */
    public static final Errno ERRNO_CIRCULAR_DEPENDENCY = new Errno(TYPE, 8403,
        "Circular dependency detected: %1$s");

    /** Library load failed - Dependency error */
    public static final Errno ERRNO_LIBRARY_LOAD_FAILED = new Errno(TYPE, 8404,
        "Failed to load native library \"%1$s\": %2$s");

    /** Bootstrap not complete - Dependency error */
    public static final Errno ERRNO_BOOTSTRAP_INCOMPLETE = new Errno(TYPE, 8405,
        "Termux bootstrap is not complete. Missing files: %1$s");


    /*
     * Permission and Access Control Errors (8500-8599)
     * Section 8500: Access Control Quality
     */

    /** Permission denied - Access control error */
    public static final Errno ERRNO_PERMISSION_DENIED = new Errno(TYPE, 8500,
        "Permission denied for operation \"%1$s\" on \"%2$s\".");

    /** Insufficient privileges - Access control error */
    public static final Errno ERRNO_INSUFFICIENT_PRIVILEGES = new Errno(TYPE, 8501,
        "Insufficient privileges for operation \"%1$s\". Required: %2$s");

    /** Access denied by policy - Access control error */
    public static final Errno ERRNO_ACCESS_DENIED_BY_POLICY = new Errno(TYPE, 8502,
        "Access denied by security policy for \"%1$s\": %2$s");

    /** Authentication failed - Access control error */
    public static final Errno ERRNO_AUTHENTICATION_FAILED = new Errno(TYPE, 8503,
        "Authentication failed for \"%1$s\": %2$s");

    /** Runtime permission not granted - Access control error */
    public static final Errno ERRNO_RUNTIME_PERMISSION_NOT_GRANTED = new Errno(TYPE, 8504,
        "Runtime permission \"%1$s\" not granted. Request permission first.");

    /** SELinux denial - Access control error */
    public static final Errno ERRNO_SELINUX_DENIAL = new Errno(TYPE, 8505,
        "SELinux denied access to \"%1$s\": %2$s");


    /*
     * Resource Management Errors (8600-8699)
     * Section 8600: Resource Management Quality
     */

    /** Resource not available - Resource error */
    public static final Errno ERRNO_RESOURCE_NOT_AVAILABLE = new Errno(TYPE, 8600,
        "Resource \"%1$s\" is not available: %2$s");

    /** Resource exhausted - Resource error */
    public static final Errno ERRNO_RESOURCE_EXHAUSTED = new Errno(TYPE, 8601,
        "Resource \"%1$s\" exhausted. Limit: %2$s");

    /** Resource allocation failed - Resource error */
    public static final Errno ERRNO_RESOURCE_ALLOCATION_FAILED = new Errno(TYPE, 8602,
        "Failed to allocate resource \"%1$s\": %2$s");

    /** Resource release failed - Resource error */
    public static final Errno ERRNO_RESOURCE_RELEASE_FAILED = new Errno(TYPE, 8603,
        "Failed to release resource \"%1$s\": %2$s");

    /** Wake lock acquisition failed - Resource error */
    public static final Errno ERRNO_WAKE_LOCK_ACQUISITION_FAILED = new Errno(TYPE, 8604,
        "Failed to acquire wake lock: %1$s");

    /** Wake lock release failed - Resource error */
    public static final Errno ERRNO_WAKE_LOCK_RELEASE_FAILED = new Errno(TYPE, 8605,
        "Failed to release wake lock: %1$s");

    /** Out of memory - Resource error */
    public static final Errno ERRNO_OUT_OF_MEMORY = new Errno(TYPE, 8606,
        "Out of memory while performing \"%1$s\".");


    /*
     * Signal Handling Errors (8700-8799)
     * Section 8700: Signal Management Quality
     */

    /** Process killed by signal - Signal error */
    public static final Errno ERRNO_PROCESS_KILLED_BY_SIGNAL = new Errno(TYPE, 8700,
        "Process \"%1$s\" (PID: %2$d) was killed by signal %3$s.");

    /** SIGKILL sent to process - Signal error */
    public static final Errno ERRNO_SIGKILL_SENT = new Errno(TYPE, 8701,
        "SIGKILL sent to process \"%1$s\" (PID: %2$d): %3$s");

    /** SIGTERM sent to process - Signal error */
    public static final Errno ERRNO_SIGTERM_SENT = new Errno(TYPE, 8702,
        "SIGTERM sent to process \"%1$s\" (PID: %2$d): %3$s");

    /** Failed to send signal - Signal error */
    public static final Errno ERRNO_SIGNAL_SEND_FAILED = new Errno(TYPE, 8703,
        "Failed to send signal %1$s to process (PID: %2$d): %3$s");

    /** Signal handler registration failed - Signal error */
    public static final Errno ERRNO_SIGNAL_HANDLER_REGISTRATION_FAILED = new Errno(TYPE, 8704,
        "Failed to register signal handler for %1$s: %2$s");

    /** Process denied action - Signal error */
    public static final Errno ERRNO_PROCESS_ACTION_DENIED = new Errno(TYPE, 8705,
        "Process action denied for \"%1$s\": %2$s");

    /** Session terminated unexpectedly - Signal error */
    public static final Errno ERRNO_SESSION_TERMINATED_UNEXPECTEDLY = new Errno(TYPE, 8706,
        "Session \"%1$s\" terminated unexpectedly with exit code %2$d.");


    /*
     * Logic and State Errors (8800-8899)
     * Section 8800: Application Logic Quality
     */

    /** Invalid state transition - Logic error */
    public static final Errno ERRNO_INVALID_STATE_TRANSITION = new Errno(TYPE, 8800,
        "Invalid state transition in %1$s from \"%2$s\" to \"%3$s\".");

    /** Unexpected state - Logic error */
    public static final Errno ERRNO_UNEXPECTED_STATE = new Errno(TYPE, 8801,
        "Unexpected state in %1$s: expected \"%2$s\", found \"%3$s\".");

    /** Operation not supported in current state - Logic error */
    public static final Errno ERRNO_OPERATION_NOT_SUPPORTED = new Errno(TYPE, 8802,
        "Operation \"%1$s\" not supported in current state \"%2$s\".");

    /** Precondition failed - Logic error */
    public static final Errno ERRNO_PRECONDITION_FAILED = new Errno(TYPE, 8803,
        "Precondition failed for operation \"%1$s\": %2$s");

    /** Postcondition failed - Logic error */
    public static final Errno ERRNO_POSTCONDITION_FAILED = new Errno(TYPE, 8804,
        "Postcondition failed for operation \"%1$s\": %2$s");

    /** Invariant violation - Logic error */
    public static final Errno ERRNO_INVARIANT_VIOLATION = new Errno(TYPE, 8805,
        "Invariant violation in %1$s: %2$s");

    /** Assertion failed - Logic error (for debugging) */
    public static final Errno ERRNO_ASSERTION_FAILED = new Errno(TYPE, 8806,
        "Assertion failed in %1$s: %2$s");

    /** Bug detected - Logic error */
    public static final Errno ERRNO_BUG_DETECTED = new Errno(TYPE, 8807,
        "Internal bug detected in %1$s: %2$s. Please report this issue.");


    /*
     * Package (pkg) Operation Errors (8900-8999)
     * Section 8900: Package Management Quality
     */

    /** Package not found - Package error */
    public static final Errno ERRNO_PACKAGE_NOT_FOUND = new Errno(TYPE, 8900,
        "Package \"%1$s\" not found in repositories.");

    /** Package installation failed - Package error */
    public static final Errno ERRNO_PACKAGE_INSTALL_FAILED = new Errno(TYPE, 8901,
        "Failed to install package \"%1$s\": %2$s");

    /** Package removal failed - Package error */
    public static final Errno ERRNO_PACKAGE_REMOVE_FAILED = new Errno(TYPE, 8902,
        "Failed to remove package \"%1$s\": %2$s");

    /** Package update failed - Package error */
    public static final Errno ERRNO_PACKAGE_UPDATE_FAILED = new Errno(TYPE, 8903,
        "Failed to update package \"%1$s\": %2$s");

    /** Package conflict detected - Package error */
    public static final Errno ERRNO_PACKAGE_CONFLICT = new Errno(TYPE, 8904,
        "Package conflict detected: \"%1$s\" conflicts with \"%2$s\".");

    /** Repository update failed - Package error */
    public static final Errno ERRNO_REPOSITORY_UPDATE_FAILED = new Errno(TYPE, 8905,
        "Failed to update package repository: %1$s");

    /** Package verification failed - Package error */
    public static final Errno ERRNO_PACKAGE_VERIFICATION_FAILED = new Errno(TYPE, 8906,
        "Package verification failed for \"%1$s\": %2$s");

    /** Insufficient storage for package - Package error */
    public static final Errno ERRNO_INSUFFICIENT_STORAGE_FOR_PACKAGE = new Errno(TYPE, 8907,
        "Insufficient storage to install package \"%1$s\". Required: %2$s, Available: %3$s");


    /*
     * ISO 9001 Quality Compliance Errors (9000-9099)
     * Section 9000: Quality Management System Errors
     */

    /** Quality check failed - QMS error */
    public static final Errno ERRNO_QUALITY_CHECK_FAILED = new Errno(TYPE, 9000,
        "Quality check failed for %1$s: %2$s");

    /** Non-conformity detected - QMS error */
    public static final Errno ERRNO_NON_CONFORMITY_DETECTED = new Errno(TYPE, 9001,
        "Non-conformity detected in %1$s: %2$s");

    /** Corrective action required - QMS error */
    public static final Errno ERRNO_CORRECTIVE_ACTION_REQUIRED = new Errno(TYPE, 9002,
        "Corrective action required for %1$s: %2$s");

    /** Preventive action recommended - QMS warning */
    public static final Errno ERRNO_PREVENTIVE_ACTION_RECOMMENDED = new Errno(TYPE, 9003,
        "Preventive action recommended for %1$s: %2$s");

    /** Audit trail incomplete - QMS error */
    public static final Errno ERRNO_AUDIT_TRAIL_INCOMPLETE = new Errno(TYPE, 9004,
        "Audit trail incomplete for operation %1$s: missing %2$s");

    /** Documentation requirement not met - QMS error */
    public static final Errno ERRNO_DOCUMENTATION_REQUIREMENT_NOT_MET = new Errno(TYPE, 9005,
        "Documentation requirement not met for %1$s: %2$s");

    /** Traceability lost - QMS error */
    public static final Errno ERRNO_TRACEABILITY_LOST = new Errno(TYPE, 9006,
        "Traceability lost for %1$s: cannot determine origin of %2$s");

    /** Configuration management error - QMS error */
    public static final Errno ERRNO_CONFIGURATION_MANAGEMENT_ERROR = new Errno(TYPE, 9007,
        "Configuration management error in %1$s: %2$s");


    ISO9001Errno(final String type, final int code, final String message) {
        super(type, code, message);
    }

    /**
     * Get a human-readable error category name based on error code.
     *
     * @param errorCode The error code to categorize
     * @return The category name
     */
    @NonNull
    public static String getErrorCategory(int errorCode) {
        if (errorCode >= 8000 && errorCode < 8100) return "Data Quality (ISO 8000)";
        if (errorCode >= 8100 && errorCode < 8200) return "Path Treatment";
        if (errorCode >= 8200 && errorCode < 8300) return "Process Execution";
        if (errorCode >= 8300 && errorCode < 8400) return "Service Lifecycle";
        if (errorCode >= 8400 && errorCode < 8500) return "Dependency Management";
        if (errorCode >= 8500 && errorCode < 8600) return "Permission/Access Control";
        if (errorCode >= 8600 && errorCode < 8700) return "Resource Management";
        if (errorCode >= 8700 && errorCode < 8800) return "Signal Handling";
        if (errorCode >= 8800 && errorCode < 8900) return "Logic/State";
        if (errorCode >= 8900 && errorCode < 9000) return "Package Operations";
        if (errorCode >= 9000 && errorCode < 9100) return "Quality Compliance (ISO 9001)";
        return "Unknown";
    }

    /**
     * Check if an error code represents a critical error that requires immediate attention.
     *
     * @param errorCode The error code to check
     * @return true if the error is critical
     */
    public static boolean isCriticalError(int errorCode) {
        // Security-related errors (path traversal, permission denied, SELinux)
        if (errorCode == 8101 || errorCode == 8500 || errorCode == 8502 || errorCode == 8505) return true;
        // Data integrity errors
        if (errorCode == 8002) return true;
        // Resource exhaustion
        if (errorCode == 8601 || errorCode == 8606) return true;
        // Bug detection
        if (errorCode == 8807) return true;
        // Non-conformity (ISO 9001)
        if (errorCode == 9001) return true;
        return false;
    }

    /**
     * Get recommended recovery action for an error.
     *
     * @param errorCode The error code
     * @return Recommended recovery action
     */
    @Nullable
    public static String getRecommendedAction(int errorCode) {
        switch (errorCode) {
            case 8000: return "Verify input data format matches expected schema.";
            case 8001: return "Ensure all required parameters are provided.";
            case 8100: return "Use absolute paths starting with '/'.";
            case 8101: return "Check for and remove path traversal patterns (../).";
            case 8200: return "Verify the executable exists and has correct permissions.";
            case 8400: return "Install missing dependencies using 'pkg install'.";
            case 8405: return "Run 'termux-setup-storage' and restart the app.";
            case 8500: return "Grant required permissions in Android settings.";
            case 8504: return "Request the permission using the appropriate Android API.";
            case 8600: return "Free up system resources or increase limits.";
            case 8606: return "Close unused applications and clear caches.";
            case 8807: return "Please report this issue at the project's issue tracker.";
            case 8900: return "Check internet connection and run 'pkg update'.";
            case 9001: return "Review and correct the non-conformity before proceeding.";
            default: return null;
        }
    }

}
