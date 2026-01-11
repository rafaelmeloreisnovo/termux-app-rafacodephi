package com.termux.shared.file;

import android.text.TextUtils;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.termux.shared.errors.Error;
import com.termux.shared.errors.ISO9001Errno;
import com.termux.shared.logger.Logger;

import java.io.File;
import java.io.IOException;
import java.util.HashSet;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * ISO 8000/9001 compliant path treatment utilities.
 *
 * This class provides path validation, sanitization, and normalization
 * following ISO 8000 (Data Quality) and ISO 9001 (Quality Management) standards.
 *
 * Key features:
 * - Path traversal attack prevention
 * - Symlink loop detection
 * - Canonical path resolution
 * - Path validation against allowed directories
 * - Quality-assured error handling with detailed error codes
 */
public class PathTreatmentUtils {

    private static final String LOG_TAG = "PathTreatmentUtils";

    /** Maximum symlink resolution depth to prevent infinite loops */
    private static final int MAX_SYMLINK_DEPTH = 40;

    /** Maximum path length for logging before truncation */
    private static final int MAX_LOG_PATH_LENGTH = 200;

    /** Length of path prefix to keep when truncating */
    private static final int LOG_PATH_PREFIX_LENGTH = 100;

    /** Length of path suffix to keep when truncating */
    private static final int LOG_PATH_SUFFIX_LENGTH = 97;

    /** Pattern to detect path traversal attempts */
    private static final Pattern PATH_TRAVERSAL_PATTERN = Pattern.compile(
        "(^|[/\\\\])\\.\\.([/\\\\]|$)|" +  // Detect "../" or "..\"
        "(%2e%2e)|" +                       // Detect URL-encoded ".."
        "(\\.\\./)|" +                      // Detect "../"
        "(\\.\\.\\\\)"                      // Detect "..\"
    , Pattern.CASE_INSENSITIVE);

    /** Pattern to detect null bytes in path (security risk) */
    private static final Pattern NULL_BYTE_PATTERN = Pattern.compile("%00|\0");

    /** Pattern to validate absolute path format */
    private static final Pattern ABSOLUTE_PATH_PATTERN = Pattern.compile("^/[^\\0]*$");

    /**
     * Validation result containing both validation status and any errors.
     */
    public static class PathValidationResult {
        private final boolean valid;
        private final String canonicalPath;
        private final Error error;

        private PathValidationResult(boolean valid, String canonicalPath, Error error) {
            this.valid = valid;
            this.canonicalPath = canonicalPath;
            this.error = error;
        }

        public boolean isValid() { return valid; }
        public String getCanonicalPath() { return canonicalPath; }
        public Error getError() { return error; }

        public static PathValidationResult success(String canonicalPath) {
            return new PathValidationResult(true, canonicalPath, null);
        }

        public static PathValidationResult failure(Error error) {
            return new PathValidationResult(false, null, error);
        }
    }

    /**
     * Validate a path according to ISO 8000/9001 standards.
     *
     * Performs the following validations:
     * 1. Null/empty check
     * 2. Null byte detection
     * 3. Path traversal attack detection
     * 4. Absolute path validation
     * 5. Canonical path resolution
     * 6. Symlink loop detection
     *
     * @param path The path to validate
     * @return PathValidationResult containing validation status and canonical path or error
     */
    @NonNull
    public static PathValidationResult validatePath(@Nullable String path) {
        // Check for null or empty path
        if (TextUtils.isEmpty(path)) {
            return PathValidationResult.failure(
                ISO9001Errno.ERRNO_REQUIRED_DATA_MISSING.getError("path", "validatePath"));
        }

        // Check for null bytes (security vulnerability)
        if (NULL_BYTE_PATTERN.matcher(path).find()) {
            Logger.logWarn(LOG_TAG, "Null byte detected in path: " + sanitizeForLogging(path));
            return PathValidationResult.failure(
                ISO9001Errno.ERRNO_PATH_TRAVERSAL_DETECTED.getError(sanitizeForLogging(path)));
        }

        // Check for path traversal attempts
        if (containsPathTraversal(path)) {
            Logger.logWarn(LOG_TAG, "Path traversal attempt detected: " + sanitizeForLogging(path));
            return PathValidationResult.failure(
                ISO9001Errno.ERRNO_PATH_TRAVERSAL_DETECTED.getError(sanitizeForLogging(path)));
        }

        // Validate absolute path format
        if (!path.startsWith("/")) {
            return PathValidationResult.failure(
                ISO9001Errno.ERRNO_INVALID_PATH_FORMAT.getError(sanitizeForLogging(path)));
        }

        // Resolve canonical path
        try {
            String canonicalPath = resolveCanonicalPath(path);
            if (canonicalPath == null) {
                return PathValidationResult.failure(
                    ISO9001Errno.ERRNO_PATH_RESOLUTION_FAILED.getError(sanitizeForLogging(path), "canonical path is null"));
            }
            return PathValidationResult.success(canonicalPath);
        } catch (IOException e) {
            return PathValidationResult.failure(
                ISO9001Errno.ERRNO_PATH_CANONICALIZATION_FAILED.getError(sanitizeForLogging(path), e.getMessage()));
        }
    }

    /**
     * Validate that a path is within an allowed directory.
     *
     * @param path The path to validate
     * @param allowedDir The allowed directory (must be absolute)
     * @return PathValidationResult containing validation status
     */
    @NonNull
    public static PathValidationResult validatePathWithinDirectory(
            @Nullable String path,
            @NonNull String allowedDir) {

        // First validate the path itself
        PathValidationResult pathResult = validatePath(path);
        if (!pathResult.isValid()) {
            return pathResult;
        }

        // Validate the allowed directory
        PathValidationResult dirResult = validatePath(allowedDir);
        if (!dirResult.isValid()) {
            return PathValidationResult.failure(
                ISO9001Errno.ERRNO_INVALID_WORKING_DIRECTORY.getError(allowedDir, "failed validation"));
        }

        // Check if path is within allowed directory
        String canonicalPath = pathResult.getCanonicalPath();
        String canonicalDir = dirResult.getCanonicalPath();

        if (!canonicalPath.startsWith(canonicalDir)) {
            Logger.logWarn(LOG_TAG, "Path not within allowed directory: " +
                sanitizeForLogging(canonicalPath) + " not in " + sanitizeForLogging(canonicalDir));
            return PathValidationResult.failure(
                ISO9001Errno.ERRNO_PATH_NOT_IN_ALLOWED_DIR.getError(
                    sanitizeForLogging(canonicalPath), sanitizeForLogging(canonicalDir)));
        }

        // Ensure path is strictly within (handles edge case of "/allowed" vs "/allowed-other")
        if (!canonicalPath.equals(canonicalDir) &&
            !canonicalPath.startsWith(canonicalDir + File.separator)) {
            return PathValidationResult.failure(
                ISO9001Errno.ERRNO_PATH_NOT_IN_ALLOWED_DIR.getError(
                    sanitizeForLogging(canonicalPath), sanitizeForLogging(canonicalDir)));
        }

        return PathValidationResult.success(canonicalPath);
    }

    /**
     * Validate a working directory for command execution.
     *
     * @param workingDirectory The working directory path
     * @param defaultDir Default directory to use if workingDirectory is null/empty
     * @return PathValidationResult containing validation status
     */
    @NonNull
    public static PathValidationResult validateWorkingDirectory(
            @Nullable String workingDirectory,
            @NonNull String defaultDir) {

        String dirToValidate = TextUtils.isEmpty(workingDirectory) ? defaultDir : workingDirectory;

        PathValidationResult result = validatePath(dirToValidate);
        if (!result.isValid()) {
            return PathValidationResult.failure(
                ISO9001Errno.ERRNO_INVALID_WORKING_DIRECTORY.getError(
                    sanitizeForLogging(dirToValidate),
                    result.getError() != null ? result.getError().getMessage() : "validation failed"));
        }

        // Check if directory exists
        File dir = new File(result.getCanonicalPath());
        if (!dir.exists()) {
            return PathValidationResult.failure(
                ISO9001Errno.ERRNO_INVALID_WORKING_DIRECTORY.getError(
                    sanitizeForLogging(result.getCanonicalPath()), "directory does not exist"));
        }

        if (!dir.isDirectory()) {
            return PathValidationResult.failure(
                ISO9001Errno.ERRNO_INVALID_WORKING_DIRECTORY.getError(
                    sanitizeForLogging(result.getCanonicalPath()), "path is not a directory"));
        }

        return result;
    }

    /**
     * Validate an executable path.
     *
     * @param executablePath The executable path
     * @return PathValidationResult containing validation status
     */
    @NonNull
    public static PathValidationResult validateExecutablePath(@Nullable String executablePath) {
        PathValidationResult result = validatePath(executablePath);
        if (!result.isValid()) {
            return result;
        }

        File executable = new File(result.getCanonicalPath());

        // Check if file exists
        if (!executable.exists()) {
            return PathValidationResult.failure(
                ISO9001Errno.ERRNO_EXECUTABLE_NOT_FOUND.getError(sanitizeForLogging(result.getCanonicalPath())));
        }

        // Check if file is executable
        if (!executable.canExecute()) {
            return PathValidationResult.failure(
                ISO9001Errno.ERRNO_EXECUTABLE_NOT_EXECUTABLE.getError(sanitizeForLogging(result.getCanonicalPath())));
        }

        return result;
    }

    /**
     * Check if a path contains path traversal sequences.
     *
     * @param path The path to check
     * @return true if path traversal is detected
     */
    public static boolean containsPathTraversal(@Nullable String path) {
        if (path == null) return false;

        // Check using regex pattern
        if (PATH_TRAVERSAL_PATTERN.matcher(path).find()) {
            return true;
        }

        // Also check normalized form
        try {
            String normalized = FileUtils.normalizePath(path);
            if (normalized != null && PATH_TRAVERSAL_PATTERN.matcher(normalized).find()) {
                return true;
            }
        } catch (Exception e) {
            // If normalization fails, be cautious and treat as potential traversal
            Logger.logWarn(LOG_TAG, "Path normalization failed, treating as potential traversal: " + e.getMessage());
            return true;
        }

        return false;
    }

    /**
     * Resolve a path to its canonical form with symlink loop detection.
     *
     * @param path The path to resolve
     * @return The canonical path
     * @throws IOException If resolution fails or symlink loop detected
     */
    @Nullable
    public static String resolveCanonicalPath(@NonNull String path) throws IOException {
        File file = new File(path);
        Set<String> visitedPaths = new HashSet<>();

        return resolveCanonicalPathRecursive(file, visitedPaths, 0);
    }

    /**
     * Recursive helper for canonical path resolution with loop detection.
     */
    private static String resolveCanonicalPathRecursive(
            @NonNull File file,
            @NonNull Set<String> visitedPaths,
            int depth) throws IOException {

        if (depth > MAX_SYMLINK_DEPTH) {
            throw new IOException(
                String.format("Maximum symlink depth exceeded (%d). Possible symlink loop.", MAX_SYMLINK_DEPTH));
        }

        String absolutePath = file.getAbsolutePath();

        // Check for symlink loops
        if (visitedPaths.contains(absolutePath)) {
            throw new IOException("Symlink loop detected: path already visited in chain");
        }
        visitedPaths.add(absolutePath);

        // Get canonical path
        String canonicalPath = file.getCanonicalPath();

        // If canonical path differs from absolute, we followed a symlink
        if (!canonicalPath.equals(absolutePath)) {
            // Recursively validate the target
            return resolveCanonicalPathRecursive(new File(canonicalPath), visitedPaths, depth + 1);
        }

        return canonicalPath;
    }

    /**
     * Normalize a path by removing redundant separators.
     * Does NOT follow symlinks or resolve . and .. (use canonical path for that).
     *
     * @param path The path to normalize
     * @return The normalized path, or null if path is null
     */
    @Nullable
    public static String normalizePath(@Nullable String path) {
        if (path == null) return null;

        // Remove redundant separators
        path = path.replaceAll("/+", "/");

        // Remove trailing separator (except for root)
        if (path.length() > 1 && path.endsWith("/")) {
            path = path.substring(0, path.length() - 1);
        }

        return path;
    }

    /**
     * Sanitize a path string for safe logging (remove sensitive parts).
     *
     * @param path The path to sanitize
     * @return Sanitized path suitable for logging
     */
    @NonNull
    public static String sanitizeForLogging(@Nullable String path) {
        if (path == null) return "<null>";
        if (path.isEmpty()) return "<empty>";

        // Truncate very long paths
        if (path.length() > MAX_LOG_PATH_LENGTH) {
            path = path.substring(0, LOG_PATH_PREFIX_LENGTH) + "..." +
                   path.substring(path.length() - LOG_PATH_SUFFIX_LENGTH);
        }

        // Replace null bytes with visible representation
        path = path.replace("\0", "\\0");

        return path;
    }

    /**
     * Create a safe, validated file path by joining directory and filename.
     *
     * @param directory The base directory (must be absolute)
     * @param filename The filename to append (must not contain path separators or traversal)
     * @return PathValidationResult with the combined path
     */
    @NonNull
    public static PathValidationResult safeJoin(
            @NonNull String directory,
            @NonNull String filename) {

        // Validate directory
        PathValidationResult dirResult = validatePath(directory);
        if (!dirResult.isValid()) {
            return dirResult;
        }

        // Validate filename doesn't contain path elements
        if (filename.contains("/") || filename.contains("\\") ||
            filename.equals("..") || filename.equals(".")) {
            return PathValidationResult.failure(
                ISO9001Errno.ERRNO_PATH_TRAVERSAL_DETECTED.getError(sanitizeForLogging(filename)));
        }

        // Join and validate result
        String combined = dirResult.getCanonicalPath() + File.separator + filename;
        return validatePath(combined);
    }

    /**
     * Get an Error object for a path validation failure with ISO 9001 compliance.
     *
     * @param path The path that failed validation
     * @param reason The reason for failure
     * @return Error object with appropriate error code
     */
    @NonNull
    public static Error createPathError(@Nullable String path, @NonNull String reason) {
        String safePath = sanitizeForLogging(path);

        if (reason.toLowerCase().contains("traversal")) {
            return ISO9001Errno.ERRNO_PATH_TRAVERSAL_DETECTED.getError(safePath);
        } else if (reason.toLowerCase().contains("not found") ||
                   reason.toLowerCase().contains("does not exist")) {
            return ISO9001Errno.ERRNO_PATH_RESOLUTION_FAILED.getError(safePath, reason);
        } else if (reason.toLowerCase().contains("permission") ||
                   reason.toLowerCase().contains("denied")) {
            return ISO9001Errno.ERRNO_PERMISSION_DENIED.getError("path access", safePath);
        } else if (reason.toLowerCase().contains("symlink") ||
                   reason.toLowerCase().contains("loop")) {
            return ISO9001Errno.ERRNO_SYMLINK_LOOP_DETECTED.getError(safePath);
        } else {
            return ISO9001Errno.ERRNO_INVALID_PATH_FORMAT.getError(safePath);
        }
    }

}
