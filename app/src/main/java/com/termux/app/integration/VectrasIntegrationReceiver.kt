package com.termux.app.integration

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import java.io.File

/**
 * VectrasIntegrationReceiver — responds to Vectras-VM-Android integration queries.
 *
 * Vectras sends ACTION_QUERY_INTEGRATION (targeted to this package) to discover whether
 * the termux bootstrap is installed and which QEMU binaries are available on this device.
 * The reply is sent back explicitly to the Vectras package only, so the response cannot
 * be intercepted by third-party apps.
 *
 * Closes X5 (cross-repo integration consumer) and T3 (Vectras↔termux bridge).
 */
class VectrasIntegrationReceiver : BroadcastReceiver() {

    override fun onReceive(context: Context, intent: Intent) {
        if (intent.action != ACTION_QUERY) return

        val filesParent = context.filesDir.parentFile?.absolutePath
            ?: "/data/data/${context.packageName}"
        val prefixPath = "$filesParent/files/usr"
        val binDir = File(prefixPath, "bin")

        val qemuBinaryPaths = QEMU_BINARY_NAMES.mapNotNull { name ->
            val f = File(binDir, name)
            if (f.exists() && f.canExecute()) f.absolutePath else null
        }.toTypedArray()

        val bootstrapReady = File(prefixPath).isDirectory && binDir.isDirectory

        val appVersion = runCatching {
            context.packageManager.getPackageInfo(context.packageName, 0).versionName
        }.getOrDefault("unknown")

        val response = Intent(ACTION_RESPONSE).apply {
            setPackage(VECTRAS_PACKAGE)
            putExtra(KEY_BOOTSTRAP_READY, bootstrapReady)
            putExtra(KEY_PREFIX_PATH, prefixPath)
            putExtra(KEY_QEMU_BINARY_PATHS, qemuBinaryPaths)
            putExtra(KEY_TERMUX_VERSION, appVersion)
        }
        context.sendBroadcast(response)
    }

    companion object {
        const val ACTION_QUERY = "com.vectras.vm.ACTION_QUERY_INTEGRATION"
        const val ACTION_RESPONSE = "com.vectras.vm.ACTION_INTEGRATION_RESPONSE"
        const val VECTRAS_PACKAGE = "com.rafacodephi.app"

        const val KEY_BOOTSTRAP_READY = "bootstrap_ready"
        const val KEY_PREFIX_PATH = "prefix_path"
        const val KEY_QEMU_BINARY_PATHS = "qemu_binary_paths"
        const val KEY_TERMUX_VERSION = "termux_version"

        private val QEMU_BINARY_NAMES = listOf(
            "qemu-system-x86_64",
            "qemu-system-x86_64-rafaelia",
            "qemu-system-x86_64-rafacodephi",
            "qemu-system-aarch64",
            "qemu-system-aarch64-rafaelia",
            "qemu-system-i386",
        )
    }
}
