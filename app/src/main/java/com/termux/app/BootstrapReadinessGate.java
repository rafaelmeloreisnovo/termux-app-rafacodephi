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
 * Read-only, runtime-resolved beta bootstrap readiness gate.
 *
 * The beta contract is intentionally stronger than "some shell files exist".
 * It requires a materialized real-pkg bootstrap and architecture-matched ELF
 * package/runtime backends. Source-archive installation metadata such as
 * SYMLINKS.txt is preserved as provenance but is not misclassified as a file
 * that must remain in the installed prefix after TermuxInstaller consumed it.
 */
public final class BootstrapReadinessGate {

    public static final String SCHEMA = "rafcodephi.bootstrap-readiness/v1";
    public static final String STATE_PASS = "PASS";
    public static final String STATE_BLOCKED = "BLOCKED";
    public static final String STATE_UNAVAILABLE = "UNAVAILABLE";
    public static final String TOKEN_VAZIO = "TOKEN_VAZIO";

    private static final String PROFILE_SCHEMA = "rafcodephi-bootstrap-profile/v1";
    private static final String PROFILE_FILE = "BOOTSTRAP_PROFILE.json";
    private static final String REQUIRED_BETA_PROFILE = "real-pkg";
    private static final String SOURCE_ONLY_SYMLINK_MANIFEST = "SYMLINKS.txt";
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

    private static final String[] REQUIRED_REAL_ELFS = new String[] {
        "apt",
        "apt-get",
        "dpkg",
        "bash",
        "busybox",
        "proot"
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

        boolean profilePass = profileContract(context, checks, prefix);

        // Expose the real utility files separately so the UI never conflates a
        // safe wrapper with the native backend that the beta contract requires.
        observeRealExecutable(checks, "$PREFIX/bin/busybox", new File(bin, "busybox"));
        observeRealExecutable(checks, "$PREFIX/bin/proot", new File(bin, "proot"));

        boolean pass = targetsPass && profilePass;
        String state = pass ? STATE_PASS : STATE_BLOCKED;
        String reason;
        if (pass) reason = "REAL_BOOTSTRAP_RUNTIME_AND_PROFILE_READY";
        else if (!profilePass) reason = "BOOTSTRAP_REAL_PROFILE_CONTRACT_BLOCKED";
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
            String packageLayer = profile.optString("package_layer", "");
            require(REQUIRED_BETA_PROFILE.equals(profileName), "beta_profile_must_be_real_pkg", violations);
            require(REQUIRED_BETA_PROFILE.equals(packageLayer), "package_layer_must_be_real_pkg", violations);
            require(context.getPackageName().equals(profile.optString("package_name", "")), "package_name", violations);
            require(prefix.getAbsolutePath().equals(profile.optString("prefix", "")), "prefix", violations);
            require(expectedBootstrapArch().equals(profile.optString("arch", "")), "arch", violations);
            require(profile.optBoolean("runtime_materialized", false), "runtime_materialized", violations);
            require(!profile.optBoolean("claim_allowed", true), "claim_allowed_must_be_false", violations);
            require(!profile.optBoolean("release_allowed", true), "release_allowed_must_be_false", violations);
            require(TOKEN_VAZIO.equals(profile.optString("device_validation", "")),
                "device_validation_must_be_TOKEN_VAZIO", violations);

            JSONArray required = profile.optJSONArray("required_entries");
            boolean sourceSymlinkManifestDeclared = false;
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

                    // SYMLINKS.txt is an archive installation instruction. The
                    // installer consumes it to materialize links and deliberately
                    // does not leave the source manifest in $PREFIX. Treating it
                    // as a runtime file caused the observed missing_required_entry_1.
                    if (SOURCE_ONLY_SYMLINK_MANIFEST.equals(relative)) {
                        sourceSymlinkManifestDeclared = true;
                        continue;
                    }

                    File target = new File(prefix, relative);
                    String canonicalTarget = target.getCanonicalPath();
                    if (!canonicalTarget.startsWith(canonicalPrefix)) {
                        violations.add("required_entry_escape_" + i);
                    } else if (!target.exists()) {
                        violations.add("missing_required_entry_" + i + "_" + relative);
                    }
                }
            }

            if (sourceSymlinkManifestDeclared) {
                checks.add(new Check("$SOURCE_BOOTSTRAP/" + SOURCE_ONLY_SYMLINK_MANIFEST,
                    "OBSERVED", "SOURCE_ARCHIVE_ONLY",
                    "declared_install_manifest_consumed_by_TermuxInstaller_not_runtime_file"));
            }

            File bin = new File(prefix, "bin");
            for (String name : REQUIRED_REAL_ELFS) {
                File target = new File(bin, name);
                if (!target.isFile() || !target.canExecute()) {
                    violations.add("real_executable_missing_" + name);
                } else if (!isElf(target)) {
                    violations.add("real_executable_not_elf_" + name);
                }
            }

            File dpkgStatus = new File(prefix, "var/lib/dpkg/status");
            require(dpkgStatus.isFile() && dpkgStatus.canRead() && dpkgStatus.length() > 0L,
                "dpkg_status_missing_or_empty", violations);

            File deb822 = new File(prefix, "etc/apt/sources.list.d/termux.sources");
            File legacySource = new File(prefix, "etc/apt/sources.list");
            require((deb822.isFile() && deb822.canRead()) || (legacySource.isFile() && legacySource.canRead()),
                "apt_source_definition_missing", violations);

            if (!violations.isEmpty()) {
                checks.add(new Check("$PREFIX/" + PROFILE_FILE, STATE_BLOCKED, path,
                    "profile_contract_violation=" + bounded(join(violations), 512)));
                return false;
            }

            checks.add(new Check("$PREFIX/" + PROFILE_FILE, STATE_PASS, path,
                "schema_package_prefix_arch_runtime_real_pkg_elf_claims_required_entries_valid profile=" + profileName));
            return true;
        } catch (Throwable error) {
            checks.add(new Check("$PREFIX/" + PROFILE_FILE, STATE_BLOCKED, path,
                "profile_read_parse_failure=" + error.getClass().getSimpleName() + ":" + bounded(String.valueOf(error.getMessage()), 160)));
            return false;
        }
    }

    private static boolean isElf(File file) {
        try (FileInputStream input = new FileInputStream(file)) {
            byte[] magic = new byte[4];
            int offset = 0;
            while (offset < magic.length) {
                int read = input.read(magic, offset, magic.length - offset);
                if (read < 0) return false;
                if (read == 0) continue;
                offset += read;
            }
            return (magic[0] & 0xff) == 0x7f && magic[1] == 'E' && magic[2] == 'L' && magic[3] == 'F';
        } catch (Throwable ignored) {
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

    private static void observeRealExecutable(List<Check> checks, String name, File file) {
        boolean available = file != null && file.isFile() && file.canExecute();
        boolean elf = available && isElf(file);
        checks.add(new Check(name, elf ? "OBSERVED_REAL_ELF" : (available ? STATE_BLOCKED : STATE_UNAVAILABLE),
            file == null ? "UNAVAILABLE" : file.getAbsolutePath(),
            elf ? "native_elf_executable_present" : (available ? "executable_present_but_not_elf" : "native_executable_absent")));
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
            out.append("bootstrap_profile_requirement=").append(REQUIRED_BETA_PROFILE).append('\n');
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
                out.put("bootstrap_profile_requirement", REQUIRED_BETA_PROFILE);
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
