package com.termux.app;

import android.content.Context;
import android.os.Build;

import com.termux.shared.termux.TermuxRuntimePaths;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * Read-only, runtime-resolved bootstrap readiness gate.
 *
 * This is the single UI/orchestration contract for deciding whether the installed
 * private filesystem is ready to proceed to runtime evidence collection. It does
 * not install, repair, chmod, mkdir or delete anything. Missing required evidence
 * blocks the dependent action and remains visible instead of becoming PASS.
 */
public final class BootstrapReadinessGate {

    public static final String SCHEMA = "rafcodephi.bootstrap-readiness/v1";
    public static final String STATE_PASS = "PASS";
    public static final String STATE_BLOCKED = "BLOCKED";
    public static final String STATE_UNAVAILABLE = "UNAVAILABLE";
    public static final String TOKEN_VAZIO = "TOKEN_VAZIO";

    private static final String PROFILE_SCHEMA = "rafcodephi-bootstrap-profile/v1";
    private static final String PROFILE_FILE = "BOOTSTRAP_PROFILE.json";
    private static final int PROFILE_READ_LIMIT = 64 * 1024;
    private static final int PROFILE_REQUIRED_ENTRIES_LIMIT = 512;

    private static final String[] REQUIRED_EXECUTABLES = new String[] {
        "sh",
        "pkg",
        "apkmanager",
        "shellbash",
        "busybox-safe",
        "proot-safe"
    };

    private BootstrapReadinessGate() {}

    public static Report evaluate(Context context) {
        if (context == null) {
            return new Report(STATE_UNAVAILABLE, "CONTEXT_UNAVAILABLE", "UNAVAILABLE", "UNAVAILABLE",
                false, false, Collections.emptyList());
        }

        TermuxRuntimePaths.init(context);
        List<Check> checks = new ArrayList<>();
        boolean targetsPass = true;

        File prefix = TermuxRuntimePaths.prefixDir();
        targetsPass &= directory(checks, "$PREFIX", prefix);
        File bin = new File(TermuxRuntimePaths.binDirPath());
        targetsPass &= directory(checks, "$PREFIX/bin", bin);
        targetsPass &= directory(checks, "$HOME", TermuxRuntimePaths.homeDir());
        targetsPass &= directory(checks, "$HOME/storage", TermuxRuntimePaths.storageHomeDir());

        for (String executable : REQUIRED_EXECUTABLES) {
            targetsPass &= executable(checks, "$PREFIX/bin/" + executable, new File(bin, executable));
        }

        // Profile validation is deliberately independent from BootstrapBaremetalGuard's
        // mutable install-time directory/chmod logic and from debug/release strict flags.
        boolean profilePass = profileContract(context, checks, prefix);

        // Optional real utility binaries are observations, not readiness requirements.
        observeOptionalExecutable(checks, "$PREFIX/bin/busybox", new File(bin, "busybox"));
        observeOptionalExecutable(checks, "$PREFIX/bin/proot", new File(bin, "proot"));

        boolean pass = targetsPass && profilePass;
        String state = pass ? STATE_PASS : STATE_BLOCKED;
        String reason;
        if (pass) reason = "REQUIRED_BOOTSTRAP_RUNTIME_AND_PROFILE_READY";
        else if (!profilePass) reason = "BOOTSTRAP_PROFILE_CONTRACT_BLOCKED";
        else reason = "REQUIRED_BOOTSTRAP_RUNTIME_TARGET_MISSING";

        return new Report(state, reason, TermuxRuntimePaths.layoutState(), TermuxRuntimePaths.prefixDirPath(),
            TermuxRuntimePaths.isCanonicalLayout(), TermuxRuntimePaths.realPkgRelocationClaimAllowed(), checks);
    }

    private static boolean directory(List<Check> checks, String name, File file) {
        boolean ok = file != null && file.isDirectory() && file.canRead() && file.canWrite() && file.canExecute();
        checks.add(new Check(name, ok ? STATE_PASS : STATE_BLOCKED,
            file == null ? "UNAVAILABLE" : file.getAbsolutePath(),
            ok ? "read_write_execute_directory" : "required_directory_not_ready"));
        return ok;
    }

    private static boolean executable(List<Check> checks, String name, File file) {
        boolean ok = file != null && file.isFile() && file.canExecute();
        checks.add(new Check(name, ok ? STATE_PASS : STATE_BLOCKED,
            file == null ? "UNAVAILABLE" : file.getAbsolutePath(),
            ok ? "owner_visible_executable" : "required_executable_not_ready"));
        return ok;
    }

    private static boolean profileContract(Context context, List<Check> checks, File prefix) {
        File profileFile = prefix == null ? null : new File(prefix, PROFILE_FILE);
        String path = profileFile == null ? "UNAVAILABLE" : profileFile.getAbsolutePath();
        if (profileFile == null || !profileFile.isFile()) {
            checks.add(new Check("$PREFIX/" + PROFILE_FILE, STATE_BLOCKED, path,
                "required_profile_missing"));
            return false;
        }
        if (profileFile.length() <= 0L || profileFile.length() > PROFILE_READ_LIMIT) {
            checks.add(new Check("$PREFIX/" + PROFILE_FILE, STATE_BLOCKED, path,
                "profile_size_out_of_bounds"));
            return false;
        }

        try {
            JSONObject profile = new JSONObject(readBoundedUtf8(profileFile, PROFILE_READ_LIMIT));
            List<String> violations = new ArrayList<>();

            require(PROFILE_SCHEMA.equals(profile.optString("schema", "")), "schema", violations);
            String profileName = profile.optString("profile", "");
            require("bridge".equals(profileName) || "real-pkg".equals(profileName), "profile", violations);
            require(context.getPackageName().equals(profile.optString("package_name", "")), "package_name", violations);
            require(prefix.getAbsolutePath().equals(profile.optString("prefix", "")), "prefix", violations);
            require(expectedBootstrapArch().equals(profile.optString("arch", "")), "arch", violations);
            require(!profile.optBoolean("claim_allowed", true), "claim_allowed_must_be_false", violations);
            require(!profile.optBoolean("release_allowed", true), "release_allowed_must_be_false", violations);
            require(TOKEN_VAZIO.equals(profile.optString("device_validation", "")),
                "device_validation_must_be_TOKEN_VAZIO", violations);

            JSONArray required = profile.optJSONArray("required_entries");
            if (required == null || required.length() == 0 || required.length() > PROFILE_REQUIRED_ENTRIES_LIMIT) {
                violations.add("required_entries_bounds");
            } else {
                String canonicalPrefix = prefix.getCanonicalPath() + File.separator;
                for (int i = 0; i < required.length(); i++) {
                    String relative = required.optString(i, "");
                    if (relative.isEmpty() || relative.startsWith("/") || relative.contains("..") || relative.contains("\\")) {
                        violations.add("unsafe_required_entry_" + i);
                        continue;
                    }
                    File target = new File(prefix, relative);
                    String canonicalTarget = target.getCanonicalPath();
                    if (!canonicalTarget.startsWith(canonicalPrefix)) {
                        violations.add("required_entry_escape_" + i);
                    } else if (!target.exists()) {
                        violations.add("missing_required_entry_" + i);
                    }
                }
            }

            if (!violations.isEmpty()) {
                checks.add(new Check("$PREFIX/" + PROFILE_FILE, STATE_BLOCKED, path,
                    "profile_contract_violation=" + bounded(join(violations), 256)));
                return false;
            }

            checks.add(new Check("$PREFIX/" + PROFILE_FILE, STATE_PASS, path,
                "schema_package_prefix_arch_claims_required_entries_valid profile=" + profileName));
            return true;
        } catch (Throwable error) {
            checks.add(new Check("$PREFIX/" + PROFILE_FILE, STATE_BLOCKED, path,
                "profile_read_parse_failure=" + error.getClass().getSimpleName() + ":" + bounded(String.valueOf(error.getMessage()), 160)));
            return false;
        }
    }

    private static void require(boolean condition, String violation, List<String> violations) {
        if (!condition) violations.add(violation);
    }

    private static String expectedBootstrapArch() {
        String abi = Build.SUPPORTED_ABIS != null && Build.SUPPORTED_ABIS.length > 0 ? Build.SUPPORTED_ABIS[0] : "";
        if ("armeabi-v7a".equals(abi)) return "arm";
        if ("arm64-v8a".equals(abi)) return "aarch64";
        if ("x86".equals(abi)) return "i686";
        if ("x86_64".equals(abi)) return "x86_64";
        return "unknown";
    }

    private static String readBoundedUtf8(File file, int limit) throws Exception {
        try (FileInputStream input = new FileInputStream(file);
             ByteArrayOutputStream output = new ByteArrayOutputStream((int) Math.min(file.length(), 8192L))) {
            byte[] buffer = new byte[4096];
            int total = 0;
            int count;
            while ((count = input.read(buffer)) >= 0) {
                if (count == 0) continue;
                total += count;
                if (total > limit) throw new IllegalStateException("profile_read_limit_exceeded");
                output.write(buffer, 0, count);
            }
            return new String(output.toByteArray(), StandardCharsets.UTF_8);
        }
    }

    private static String join(List<String> values) {
        StringBuilder out = new StringBuilder();
        for (int i = 0; i < values.size(); i++) {
            if (i > 0) out.append(',');
            out.append(values.get(i));
        }
        return out.toString();
    }

    private static String bounded(String value, int limit) {
        if (value == null) return "null";
        return value.length() <= limit ? value : value.substring(0, limit) + "…";
    }

    private static void observeOptionalExecutable(List<Check> checks, String name, File file) {
        boolean available = file != null && file.isFile() && file.canExecute();
        checks.add(new Check(name, available ? "OBSERVED" : STATE_UNAVAILABLE,
            file == null ? "UNAVAILABLE" : file.getAbsolutePath(),
            available ? "optional_real_binary_present" : "optional_real_binary_absent_safe_shim_is_contract"));
    }

    public static final class Report {
        public final String state;
        public final String reason;
        public final String runtimeLayout;
        public final String runtimePrefix;
        public final boolean canonicalLayout;
        public final boolean realPkgRelocationClaimAllowed;
        public final List<Check> checks;

        Report(String state, String reason, String runtimeLayout, String runtimePrefix,
               boolean canonicalLayout, boolean realPkgRelocationClaimAllowed, List<Check> checks) {
            this.state = state;
            this.reason = reason;
            this.runtimeLayout = runtimeLayout;
            this.runtimePrefix = runtimePrefix;
            this.canonicalLayout = canonicalLayout;
            this.realPkgRelocationClaimAllowed = realPkgRelocationClaimAllowed;
            this.checks = Collections.unmodifiableList(new ArrayList<>(checks));
        }

        public boolean isPass() {
            return STATE_PASS.equals(state);
        }

        public String render() {
            StringBuilder out = new StringBuilder();
            out.append("bootstrap_readiness_state=").append(state).append('\n');
            out.append("bootstrap_readiness_reason=").append(reason).append('\n');
            out.append("runtime_layout=").append(runtimeLayout).append('\n');
            out.append("runtime_prefix=").append(runtimePrefix).append('\n');
            out.append("canonical_layout=").append(canonicalLayout).append('\n');
            out.append("real_pkg_relocation_claim_allowed=").append(realPkgRelocationClaimAllowed).append('\n');
            out.append("device_runtime_proof=").append(TOKEN_VAZIO).append('\n');
            out.append("claim_allowed_release=false\n");
            for (Check check : checks) {
                out.append(check.name).append('=').append(check.state)
                    .append(" [").append(check.detail).append("]\n");
            }
            return out.toString().trim();
        }

        public JSONObject toJson() {
            JSONObject out = new JSONObject();
            try {
                out.put("schema", SCHEMA);
                out.put("state", state);
                out.put("reason", reason);
                out.put("runtime_layout", runtimeLayout);
                out.put("runtime_prefix", runtimePrefix);
                out.put("canonical_layout", canonicalLayout);
                out.put("real_pkg_relocation_claim_allowed", realPkgRelocationClaimAllowed);
                out.put("device_runtime_proof", TOKEN_VAZIO);
                out.put("claim_allowed_release", false);
                JSONArray rows = new JSONArray();
                for (Check check : checks) rows.put(check.toJson());
                out.put("checks", rows);
            } catch (JSONException ignored) {
                return new JSONObject();
            }
            return out;
        }
    }

    public static final class Check {
        public final String name;
        public final String state;
        public final String path;
        public final String detail;

        Check(String name, String state, String path, String detail) {
            this.name = name;
            this.state = state;
            this.path = path;
            this.detail = detail;
        }

        JSONObject toJson() throws JSONException {
            JSONObject out = new JSONObject();
            out.put("name", name);
            out.put("state", state);
            out.put("path", path);
            out.put("detail", detail);
            return out;
        }
    }
}
