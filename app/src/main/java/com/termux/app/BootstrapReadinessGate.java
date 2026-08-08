package com.termux.app;

import android.content.Context;

import com.termux.shared.termux.TermuxRuntimePaths;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * Read-only, runtime-resolved bootstrap readiness gate.
 *
 * This is the single UI/orchestration contract for deciding whether the installed
 * private filesystem is ready to proceed to runtime evidence collection. It does
 * not install, repair or delete anything. Missing required evidence blocks the
 * dependent action and remains visible instead of being converted to PASS.
 */
public final class BootstrapReadinessGate {

    public static final String SCHEMA = "rafcodephi.bootstrap-readiness/v1";
    public static final String STATE_PASS = "PASS";
    public static final String STATE_BLOCKED = "BLOCKED";
    public static final String STATE_UNAVAILABLE = "UNAVAILABLE";
    public static final String TOKEN_VAZIO = "TOKEN_VAZIO";

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
        boolean pass = true;

        pass &= directory(checks, "$PREFIX", TermuxRuntimePaths.prefixDir());
        File bin = new File(TermuxRuntimePaths.binDirPath());
        pass &= directory(checks, "$PREFIX/bin", bin);
        pass &= directory(checks, "$HOME", TermuxRuntimePaths.homeDir());
        pass &= directory(checks, "$HOME/storage", TermuxRuntimePaths.storageHomeDir());

        for (String executable : REQUIRED_EXECUTABLES) {
            pass &= executable(checks, "$PREFIX/bin/" + executable, new File(bin, executable));
        }

        // Optional real utility binaries are observations, not readiness requirements.
        observeOptionalExecutable(checks, "$PREFIX/bin/busybox", new File(bin, "busybox"));
        observeOptionalExecutable(checks, "$PREFIX/bin/proot", new File(bin, "proot"));

        String state = pass ? STATE_PASS : STATE_BLOCKED;
        String reason = pass ? "REQUIRED_BOOTSTRAP_RUNTIME_TARGETS_READY" : "REQUIRED_BOOTSTRAP_RUNTIME_TARGET_MISSING";
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
                // JSONObject values above are bounded primitives/strings; fail closed if platform JSON disagrees.
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
