package com.termux.app.audit

enum class EvidenceStatus {
    DOCUMENTED,
    STATIC_CHECKED,
    CI_CHECKED,
    DEVICE_PENDING,
    DEVICE_VALIDATED,
    EXPERIMENTAL,
    NOT_CERTIFIED
}

data class BenchmarkMetric(
    val id: String,
    val name: String,
    val category: String,
    val unit: String,
    val value: String?,
    val status: EvidenceStatus,
    val source: String,
    val notes: String
)

data class TermuxRafcodephiAudit(
    val packageName: String = "com.termux.rafacodephi",
    val versionName: String? = null,
    val versionCode: Int? = null,
    val minSdk: Int? = null,
    val targetSdk: Int? = null,
    val compileSdk: Int? = null,
    val ndkVersion: String? = null,
    val supportedAbis: List<String> = emptyList(),
    val bootstrapStatus: String = "DOCUMENTED",
    val bootstrapRuntimeReady: EvidenceStatus = EvidenceStatus.DOCUMENTED,
    val bootstrapBlake3Status: EvidenceStatus = EvidenceStatus.DOCUMENTED,
    val pageSizeSupport16kb: EvidenceStatus = EvidenceStatus.DOCUMENTED,
    val rmrPureCoreAvailable: EvidenceStatus = EvidenceStatus.DOCUMENTED,
    val rmrNoMallocMode: EvidenceStatus = EvidenceStatus.DOCUMENTED,
    val rmrNoHeapMode: EvidenceStatus = EvidenceStatus.DOCUMENTED,
    val rmrNoLibmMode: EvidenceStatus = EvidenceStatus.DOCUMENTED,
    val jniDirectAvailable: EvidenceStatus = EvidenceStatus.DOCUMENTED,
    val terminalLifecycleGuards: EvidenceStatus = EvidenceStatus.DOCUMENTED,
    val processGroupKillEnabled: EvidenceStatus = EvidenceStatus.STATIC_CHECKED,
    val waitForEintrHandlingEnabled: EvidenceStatus = EvidenceStatus.STATIC_CHECKED,
    val benchmarkStatus: EvidenceStatus = EvidenceStatus.CI_CHECKED,
    val deviceRuntimeStatus: EvidenceStatus = EvidenceStatus.DEVICE_PENDING,
    val isoClaimStatus: String = "NOT_CERTIFIED_INTERNAL_REFERENCE_ONLY",
    val benchmarkSummary: Map<String, String> = emptyMap(),
    val benchmarkMetrics: List<BenchmarkMetric> = emptyList(),
    val runtimeDeviceStatus: String = "DEVICE_PENDING",
    val isoNotice: String = "Internal checklist only. No ISO certification or formal compliance claim is made.",
    val evidenceStatus: Map<String, EvidenceStatus> = emptyMap()
)
