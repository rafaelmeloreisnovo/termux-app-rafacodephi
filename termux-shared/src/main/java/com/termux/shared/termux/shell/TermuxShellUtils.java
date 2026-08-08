package com.termux.shared.termux.shell;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.termux.shared.errors.Error;
import com.termux.shared.file.FileUtils;
import com.termux.shared.file.filesystem.FileTypes;
import com.termux.shared.logger.Logger;
import com.termux.shared.termux.TermuxRuntimePaths;
import com.termux.shared.termux.settings.properties.TermuxAppSharedProperties;

import org.apache.commons.io.filefilter.TrueFileFilter;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class TermuxShellUtils {

    private static final String LOG_TAG = "TermuxShellUtils";

    /** Setup shell command arguments using the runtime-resolved prefix. */
    @NonNull
    public static String[] setupShellCommandArguments(@NonNull String executable, @Nullable String[] arguments) {
        String interpreter = null;
        try {
            File file = new File(executable);
            try (FileInputStream in = new FileInputStream(file)) {
                byte[] buffer = new byte[256];
                int bytesRead = in.read(buffer);
                if (bytesRead > 4) {
                    if (buffer[0] == 0x7F && buffer[1] == 'E' && buffer[2] == 'L' && buffer[3] == 'F') {
                        // ELF: execute directly.
                    } else if (buffer[0] == '#' && buffer[1] == '!') {
                        StringBuilder builder = new StringBuilder();
                        for (int i = 2; i < bytesRead; i++) {
                            char c = (char) buffer[i];
                            if (c == ' ' || c == '\n') {
                                if (builder.length() != 0) {
                                    String shebangExecutable = builder.toString();
                                    if (shebangExecutable.startsWith("/usr") || shebangExecutable.startsWith("/bin")) {
                                        String[] parts = shebangExecutable.split("/");
                                        String binary = parts[parts.length - 1];
                                        interpreter = TermuxRuntimePaths.binDirPath() + "/" + binary;
                                    }
                                    break;
                                }
                            } else {
                                builder.append(c);
                            }
                        }
                    } else {
                        interpreter = TermuxRuntimePaths.binDirPath() + "/sh";
                    }
                }
            }
        } catch (IOException ignored) {
        }

        List<String> result = new ArrayList<>();
        if (interpreter != null) result.add(interpreter);
        result.add(executable);
        if (arguments != null) Collections.addAll(result, arguments);
        return result.toArray(new String[0]);
    }

    /** Clear files under the runtime-resolved TMPDIR. */
    public static void clearTermuxTMPDIR(boolean onlyIfExists) {
        String tmpPath = TermuxRuntimePaths.tmpDirPath();
        if (onlyIfExists && !FileUtils.directoryFileExists(tmpPath, false)) return;

        Error error;
        TermuxAppSharedProperties properties = TermuxAppSharedProperties.getProperties();
        int days = properties.getDeleteTMPDIRFilesOlderThanXDaysOnExit();
        if (days > 0) days = 0;

        if (days < 0) {
            Logger.logInfo(LOG_TAG, "Not clearing runtime $TMPDIR");
        } else if (days == 0) {
            error = FileUtils.clearDirectory("$TMPDIR", FileUtils.getCanonicalPath(tmpPath, null));
            if (error != null) Logger.logErrorExtended(LOG_TAG, "Failed to clear runtime $TMPDIR\n" + error);
        } else {
            error = FileUtils.deleteFilesOlderThanXDays("$TMPDIR",
                FileUtils.getCanonicalPath(tmpPath, null),
                TrueFileFilter.INSTANCE, days, true, FileTypes.FILE_TYPE_ANY_FLAGS);
            if (error != null)
                Logger.logErrorExtended(LOG_TAG, "Failed to delete runtime $TMPDIR files older than " + days + " days\n" + error);
        }
    }
}
