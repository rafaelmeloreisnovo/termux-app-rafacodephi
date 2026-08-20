package com.termux.app.integration

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import java.io.File
import java.security.MessageDigest

/**
 * Protocol v2 discovery endpoint for Vectras-VM-Android.
 *
 * It exposes bounded capability names and material identity digests, never
 * private sandbox paths. Actual execution is delegated to RunCommandService
 * and remains subject to the user-controlled allow-external-apps policy and
 * RUN_COMMAND permission.
 */
class VectrasIntegrationReceiver : BroadcastReceiver() {

    override fun onReceive(context: Context, intent: Intent) {
        if (intent.action != ACTION_QUERY) return

        val nonce = intent.getStringExtra(KEY_NONCE)
        if (!isValidNonce(nonce)) return

        val filesParent = context.filesDir.parentFile?.absolutePath
            ?: "/data/data/${context.packageName}"
        val prefixPath = "$filesParent/files/usr"
        val binDir = File(prefixPath, "bin")

        val qemuFiles = QEMU_BINARY_NAMES.mapNotNull { name ->
            val file = File(binDir, name)
            if (file.isFile && file.canExecute()) name to file else null
        }
        val qemuBinaryNames = qemuFiles.map { it.first }.toTypedArray()
        val qemuBinarySha256 = qemuFiles.mapNotNull { (name, file) ->
            sha256File(file)?.let { digest -> "$name=$digest" }
        }.toTypedArray()

        val bootstrapReady = File(prefixPath).isDirectory && binDir.isDirectory
        val appVersion = runCatching {
            context.packageManager.getPackageInfo(context.packageName, 0).versionName
        }.getOrDefault("unknown")
        val providerApkSha256 = context.applicationInfo.sourceDir
            ?.let(::File)
            ?.takeIf { it.isFile }
            ?.let(::sha256File)

        val response = Intent(ACTION_RESPONSE).apply {
            setPackage(VECTRAS_PACKAGE)
            putExtra(KEY_NONCE, nonce)
            putExtra(KEY_PROTOCOL_VERSION, PROTOCOL_VERSION)
            putExtra(KEY_BOOTSTRAP_READY, bootstrapReady)
            putExtra(KEY_QEMU_BINARY_NAMES, qemuBinaryNames)
            putExtra(KEY_QEMU_BINARY_SHA256, qemuBinarySha256)
            putExtra(KEY_TERMUX_VERSION, appVersion)
            putExtra(KEY_PROVIDER_APK_SHA256, providerApkSha256)
            putExtra(KEY_EXECUTION_MODE, EXECUTION_MODE_RUN_COMMAND_SERVICE)
            putExtra(KEY_RUN_COMMAND_PERMISSION, RUN_COMMAND_PERMISSION)
            putExtra(KEY_PRIVATE_PATHS_EXPOSED, false)
        }

        // Only receivers holding the user-granted RUN_COMMAND permission may
        // consume this response. The payload contains digests and capability
        // names only; no private path crosses the app boundary.
        context.sendBroadcast(response, RUN_COMMAND_PERMISSION)
    }

    private fun isValidNonce(value: String?): Boolean =
        value != null && value.length in 16..128 &&
            value.all { it.isLetterOrDigit() || it == '-' || it == '_' }

    private fun sha256File(file: File): String? = runCatching {
        val digest = MessageDigest.getInstance("SHA-256")
        file.inputStream().buffered().use { input ->
            val buffer = ByteArray(DEFAULT_BUFFER_SIZE)
            while (true) {
                val count = input.read(buffer)
                if (count < 0) break
                if (count > 0) digest.update(buffer, 0, count)
            }
        }
        digest.digest().joinToString("") { "%02x".format(it) }
    }.getOrNull()

    companion object {
        const val PROTOCOL_VERSION = 2

        const val ACTION_QUERY = "com.vectras.vm.ACTION_QUERY_INTEGRATION"
        const val ACTION_RESPONSE = "com.vectras.vm.ACTION_INTEGRATION_RESPONSE"
        const val VECTRAS_PACKAGE = "com.rafacodephi.app"

        const val RUN_COMMAND_PERMISSION =
            "com.termux.rafacodephi.permission.RUN_COMMAND"
        const val EXECUTION_MODE_RUN_COMMAND_SERVICE = "run_command_service"

        const val KEY_NONCE = "nonce"
        const val KEY_PROTOCOL_VERSION = "protocol_version"
        const val KEY_BOOTSTRAP_READY = "bootstrap_ready"
        const val KEY_QEMU_BINARY_NAMES = "qemu_binary_names"
        const val KEY_QEMU_BINARY_SHA256 = "qemu_binary_sha256"
        const val KEY_TERMUX_VERSION = "termux_version"
        const val KEY_PROVIDER_APK_SHA256 = "provider_apk_sha256"
        const val KEY_EXECUTION_MODE = "execution_mode"
        const val KEY_RUN_COMMAND_PERMISSION = "run_command_permission"
        const val KEY_PRIVATE_PATHS_EXPOSED = "private_paths_exposed"

        private val QEMU_BINARY_NAMES = listOf(
            "qemu-system-x86_64",
            "qemu-system-x86_64-rafaelia",
            "qemu-system-x86_64-rafacodephi",
            "qemu-system-aarch64",
            "qemu-system-aarch64-rafaelia",
            "qemu-system-i386",
            "qemu-system-arm",
        )
    }
}
