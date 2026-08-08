package com.termux.shared.termux.shell.command.runner.terminal;

import android.content.Context;
import android.system.OsConstants;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.google.common.base.Joiner;
import com.termux.shared.R;
import com.termux.shared.shell.command.ExecutionCommand;
import com.termux.shared.shell.command.environment.ShellEnvironmentUtils;
import com.termux.shared.shell.command.environment.UnixShellEnvironment;
import com.termux.shared.errors.Error;
import com.termux.shared.errors.Errno;
import com.termux.shared.logger.Logger;
import com.termux.shared.shell.command.environment.IShellEnvironment;
import com.termux.shared.shell.ShellUtils;
import com.termux.shared.termux.TermuxRuntimePaths;
import com.termux.shared.termux.shell.TermuxQualityManager;
import com.termux.terminal.TerminalSession;
import com.termux.terminal.TerminalSessionClient;

import java.io.File;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;

/**
 * A class that maintains info for foreground Termux sessions.
 * It also provides a way to link each {@link TerminalSession} with the {@link ExecutionCommand}
 * that started it.
 */
public class TermuxSession {

    private final TerminalSession mTerminalSession;
    private final ExecutionCommand mExecutionCommand;
    private final TermuxSessionClient mTermuxSessionClient;
    private final boolean mSetStdoutOnExit;

    private static final String LOG_TAG = "TermuxSession";

    private TermuxSession(@NonNull final TerminalSession terminalSession, @NonNull final ExecutionCommand executionCommand,
                          final TermuxSessionClient termuxSessionClient, final boolean setStdoutOnExit) {
        this.mTerminalSession = terminalSession;
        this.mExecutionCommand = executionCommand;
        this.mTermuxSessionClient = termuxSessionClient;
        this.mSetStdoutOnExit = setStdoutOnExit;
    }

    public static TermuxSession execute(@NonNull final Context currentPackageContext, @NonNull ExecutionCommand executionCommand,
                                        @NonNull final TerminalSessionClient terminalSessionClient, final TermuxSessionClient termuxSessionClient,
                                        @NonNull final IShellEnvironment shellEnvironmentClient,
                                        @Nullable HashMap<String, String> additionalEnvironment,
                                        final boolean setStdoutOnExit) {
        TermuxRuntimePaths.init(currentPackageContext);

        if (executionCommand.executable != null && executionCommand.executable.isEmpty())
            executionCommand.executable = null;
        if (executionCommand.workingDirectory == null || executionCommand.workingDirectory.isEmpty())
            executionCommand.workingDirectory = shellEnvironmentClient.getDefaultWorkingDirectoryPath();
        executionCommand.workingDirectory = resolveBootWorkingDirectory(executionCommand.workingDirectory);

        String defaultBinPath = shellEnvironmentClient.getDefaultBinPath();
        Logger.logInfo(LOG_TAG, "defaultBinPath=" + defaultBinPath);
        Logger.logInfo(LOG_TAG, "workingDirectory=" + executionCommand.workingDirectory);
        Logger.logInfo(LOG_TAG, "runtimeLayout=" + TermuxRuntimePaths.layoutState());
        if (defaultBinPath.isEmpty()) defaultBinPath = "/system/bin";

        boolean isLoginShell = false;
        boolean systemShellFallback = false;
        if (executionCommand.executable == null) {
            if (!executionCommand.isFailsafe) {
                Error bootstrapError = TermuxQualityManager.checkBootstrapComplete();
                if (bootstrapError != null) {
                    Logger.logWarn(LOG_TAG, "Bootstrap validation failed, continuing with shell fallback: " + bootstrapError.getMessage());
                }
            }

            // Do not hard-gate terminal startup on bash: scan every supported login shell,
            // then keep /system/bin/sh as the recovery shell when the runtime prefix is incomplete.
            if (!executionCommand.isFailsafe) {
                StringBuilder foundCandidates = new StringBuilder();
                for (String shellBinary : UnixShellEnvironment.LOGIN_SHELL_BINARIES) {
                    File shellFile = new File(defaultBinPath, shellBinary);
                    if (shellFile.canExecute()) {
                        if (foundCandidates.length() > 0) foundCandidates.append(",");
                        foundCandidates.append(shellBinary);
                        if (executionCommand.executable == null) executionCommand.executable = shellFile.getAbsolutePath();
                    }
                }
                Logger.logInfo(LOG_TAG, "shellCandidatesFound=" + (foundCandidates.length() == 0 ? "none" : foundCandidates.toString()));
            }

            if (executionCommand.executable == null) {
                Logger.logWarn(LOG_TAG, "fallback /system/bin/sh selected");
                executionCommand.executable = "/system/bin/sh";
                systemShellFallback = true;
            } else {
                isLoginShell = true;
                Logger.logInfo(LOG_TAG, "selected shell=" + executionCommand.executable);
            }
        }

        String[] commandArgs = shellEnvironmentClient.setupShellCommandArguments(executionCommand.executable, executionCommand.arguments);
        executionCommand.executable = commandArgs[0];
        Logger.logInfo(LOG_TAG, "final executable=" + executionCommand.executable);
        String processName = (isLoginShell ? "-" : "") + ShellUtils.getExecutableBasename(executionCommand.executable);
        Logger.logInfo(LOG_TAG, "processName=" + processName);

        String[] arguments = new String[commandArgs.length];
        arguments[0] = processName;
        if (commandArgs.length > 1) System.arraycopy(commandArgs, 1, arguments, 1, commandArgs.length - 1);
        executionCommand.arguments = arguments;

        if (executionCommand.commandLabel == null) executionCommand.commandLabel = processName;

        HashMap<String, String> environment = shellEnvironmentClient.setupShellCommandEnvironment(currentPackageContext,
            executionCommand);
        if (systemShellFallback) hardenSystemShellFallbackEnvironment(environment, executionCommand.workingDirectory);
        if (additionalEnvironment != null) environment.putAll(additionalEnvironment);
        List<String> environmentList = ShellEnvironmentUtils.convertEnvironmentToEnviron(environment);
        Collections.sort(environmentList);
        String[] environmentArray = environmentList.toArray(new String[0]);

        if (!executionCommand.setState(ExecutionCommand.ExecutionState.EXECUTING)) {
            executionCommand.setStateFailed(Errno.ERRNO_FAILED.getCode(), currentPackageContext.getString(R.string.error_failed_to_execute_termux_session_command, executionCommand.getCommandIdAndLabelLogString()));
            TermuxSession.processTermuxSessionResult(null, executionCommand);
            return null;
        }

        Logger.logDebugExtended(LOG_TAG, executionCommand.toString());
        Logger.logVerboseExtended(LOG_TAG, "\"" + executionCommand.getCommandIdAndLabelLogString() + "\" TermuxSession Environment:\n" +
            Joiner.on("\n").join(environmentArray));

        Logger.logDebug(LOG_TAG, "Running \"" + executionCommand.getCommandIdAndLabelLogString() + "\" TermuxSession");
        TerminalSession terminalSession = new TerminalSession(executionCommand.executable,
            executionCommand.workingDirectory, executionCommand.arguments, environmentArray,
            executionCommand.terminalTranscriptRows, terminalSessionClient);

        if (executionCommand.shellName != null) terminalSession.mSessionName = executionCommand.shellName;
        return new TermuxSession(terminalSession, executionCommand, termuxSessionClient, setStdoutOnExit);
    }

    private static String resolveBootWorkingDirectory(@Nullable String requestedWorkingDirectory) {
        String workingDirectory = requestedWorkingDirectory;
        if (workingDirectory == null || workingDirectory.isEmpty())
            workingDirectory = TermuxRuntimePaths.homeDirPath();

        File candidate = new File(workingDirectory);
        if (!candidate.isDirectory() && isPrivateBootDirectory(candidate)) {
            if (candidate.mkdirs())
                Logger.logInfo(LOG_TAG, "Created terminal working directory=" + candidate.getAbsolutePath());
        }
        if (isUsableDirectory(candidate)) return candidate.getAbsolutePath();

        File filesDir = TermuxRuntimePaths.filesDir();
        if (!filesDir.isDirectory()) filesDir.mkdirs();
        if (isUsableDirectory(filesDir)) {
            Logger.logWarn(LOG_TAG, "workingDirectory fallback selected=" + filesDir.getAbsolutePath());
            return filesDir.getAbsolutePath();
        }

        Logger.logWarn(LOG_TAG, "workingDirectory fallback selected=/");
        return "/";
    }

    private static boolean isPrivateBootDirectory(@NonNull File candidate) {
        String path = candidate.getAbsolutePath();
        return path.equals(TermuxRuntimePaths.homeDirPath()) ||
            path.equals(TermuxRuntimePaths.filesDirPath()) ||
            path.startsWith(TermuxRuntimePaths.filesDirPath() + "/");
    }

    private static boolean isUsableDirectory(@NonNull File candidate) {
        return candidate != null && candidate.isDirectory() && candidate.canRead() && candidate.canExecute();
    }

    private static void hardenSystemShellFallbackEnvironment(@NonNull HashMap<String, String> environment,
                                                            @NonNull String workingDirectory) {
        String existingPath = environment.get(UnixShellEnvironment.ENV_PATH);
        String systemPath = "/system/bin:/system/xbin";
        if (existingPath != null && !existingPath.isEmpty()) systemPath = systemPath + ":" + existingPath;
        environment.put(UnixShellEnvironment.ENV_PATH, systemPath);

        String tmp = environment.get(UnixShellEnvironment.ENV_TMPDIR);
        if (tmp == null || !isUsableDirectory(new File(tmp)))
            environment.put(UnixShellEnvironment.ENV_TMPDIR, workingDirectory);
        String home = environment.get(UnixShellEnvironment.ENV_HOME);
        if (home == null || !isUsableDirectory(new File(home)))
            environment.put(UnixShellEnvironment.ENV_HOME, workingDirectory);

        Logger.logWarn(LOG_TAG, "system shell fallback environment hardened for boot recovery");
    }

    public void finish() {
        if (mTerminalSession.isRunning()) return;
        int exitCode = mTerminalSession.getExitStatus();
        if (exitCode == 0)
            Logger.logDebug(LOG_TAG, "The \"" + mExecutionCommand.getCommandIdAndLabelLogString() + "\" TermuxSession exited normally");
        else
            Logger.logDebug(LOG_TAG, "The \"" + mExecutionCommand.getCommandIdAndLabelLogString() + "\" TermuxSession exited with code: " + exitCode);

        if (mExecutionCommand.isStateFailed()) {
            Logger.logDebug(LOG_TAG, "Ignoring result processing since execution command already failed");
            return;
        }
        mExecutionCommand.resultData.exitCode = exitCode;
        if (this.mSetStdoutOnExit)
            mExecutionCommand.resultData.stdout.append(ShellUtils.getTerminalSessionTranscriptText(mTerminalSession, true, false));
        if (!mExecutionCommand.setState(ExecutionCommand.ExecutionState.EXECUTED)) return;
        TermuxSession.processTermuxSessionResult(this, null);
    }

    public void killIfExecuting(@NonNull final Context context, boolean processResult) {
        if (mExecutionCommand.hasExecuted()) {
            Logger.logDebug(LOG_TAG, "Ignoring SIGKILL for completed command");
            return;
        }

        Logger.logDebug(LOG_TAG, "Send SIGKILL to \"" + mExecutionCommand.getCommandIdAndLabelLogString() + "\"");
        if (mExecutionCommand.setStateFailed(Errno.ERRNO_FAILED.getCode(), context.getString(R.string.error_sending_sigkill_to_process))) {
            if (processResult) {
                mExecutionCommand.resultData.exitCode = 137;
                if (this.mSetStdoutOnExit)
                    mExecutionCommand.resultData.stdout.append(ShellUtils.getTerminalSessionTranscriptText(mTerminalSession, true, false));
                TermuxSession.processTermuxSessionResult(this, null);
            }
        }
        mTerminalSession.finishIfRunning();
    }

    public static void processTermuxSessionResult(@Nullable TermuxSession termuxSession,
                                                  @Nullable ExecutionCommand executionCommand) {
        if (termuxSession != null) executionCommand = termuxSession.mExecutionCommand;
        if (executionCommand == null) return;
        executionCommand.setState(ExecutionCommand.ExecutionState.EXECUTED);
    }

    public TerminalSession getTerminalSession() { return mTerminalSession; }
    public ExecutionCommand getExecutionCommand() { return mExecutionCommand; }
    public TermuxSessionClient getTermuxSessionClient() { return mTermuxSessionClient; }

    public interface TermuxSessionClient {
        void onTermuxSessionExited(TermuxSession termuxSession);
    }
}
