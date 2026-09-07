#include <jni.h>
#include <stdint.h>

#include "rafaelia_gpu_orchestrator.h"

JNIEXPORT jint JNICALL
Java_com_termux_lowlevel_GpuOrchestrator_nativeRefreshCapabilities(
        JNIEnv* env, jclass clazz) {
    (void)env;
    (void)clazz;
    return (jint)rgpu_runtime_caps();
}

JNIEXPORT void JNICALL
Java_com_termux_lowlevel_GpuOrchestrator_nativeSetThermal(
        JNIEnv* env, jclass clazz, jint thermal) {
    (void)env;
    (void)clazz;
    if (thermal < 0) thermal = 0;
    if (thermal > 100) thermal = 100;
    remk_set_thermal((uint32_t)thermal);
}

JNIEXPORT jint JNICALL
Java_com_termux_lowlevel_GpuOrchestrator_nativeRecordTotalCost(
        JNIEnv* env, jclass clazz,
        jint backend, jlong cpu_total_ns, jlong gpu_total_ns,
        jint sample_count, jint work_bytes,
        jboolean correctness_pass, jboolean stable_environment) {
    rgpu_measurement_t m;
    (void)env;
    (void)clazz;

    if (cpu_total_ns <= 0 || gpu_total_ns <= 0 ||
        sample_count < 0 || work_bytes < 0) {
        return (jint)RGO_ERR_ARGS;
    }

    m.cpu_total_ns = (uint64_t)cpu_total_ns;
    m.gpu_total_ns = (uint64_t)gpu_total_ns;
    m.sample_count = (uint32_t)sample_count;
    m.work_bytes = (uint32_t)work_bytes;
    m.correctness_pass = correctness_pass ? 1u : 0u;
    m.stable_environment = stable_environment ? 1u : 0u;
    m.reserved[0] = 0u;
    m.reserved[1] = 0u;

    return (jint)rgpu_record_total_cost((rgpu_backend_t)backend, &m);
}

JNIEXPORT jlongArray JNICALL
Java_com_termux_lowlevel_GpuOrchestrator_nativeSnapshot(
        JNIEnv* env, jclass clazz) {
    rgpu_snapshot_t s;
    jlong raw[26];
    jlongArray out;
    (void)clazz;

    if (rgpu_get_snapshot(&s) != RGO_OK) return NULL;

    raw[0] = 1; /* snapshot schema */
    raw[1] = (jlong)s.state;
    raw[2] = (jlong)s.runtime_caps;
    raw[3] = (jlong)s.qualified_caps;
    raw[4] = (jlong)s.qualified_backend;
    raw[5] = (jlong)s.core_count;
    raw[6] = (jlong)s.thermal_0_100;
    raw[7] = (jlong)s.policy.min_samples;
    raw[8] = (jlong)s.policy.min_work_bytes;
    raw[9] = (jlong)s.policy.max_thermal;
    raw[10] = (jlong)s.policy.min_gain_permille;
    raw[11] = (jlong)s.route_count_cpu;
    raw[12] = (jlong)s.route_count_gpu;
    raw[13] = (jlong)s.last_route_reason;
    raw[14] = (jlong)s.opencl_measurement.cpu_total_ns;
    raw[15] = (jlong)s.opencl_measurement.gpu_total_ns;
    raw[16] = (jlong)s.opencl_measurement.sample_count;
    raw[17] = (jlong)s.opencl_measurement.work_bytes;
    raw[18] = (jlong)s.opencl_measurement.correctness_pass;
    raw[19] = (jlong)s.opencl_measurement.stable_environment;
    raw[20] = (jlong)s.vulkan_measurement.cpu_total_ns;
    raw[21] = (jlong)s.vulkan_measurement.gpu_total_ns;
    raw[22] = (jlong)s.vulkan_measurement.sample_count;
    raw[23] = (jlong)s.vulkan_measurement.work_bytes;
    raw[24] = (jlong)s.vulkan_measurement.correctness_pass;
    raw[25] = (jlong)s.vulkan_measurement.stable_environment;

    out = (*env)->NewLongArray(env, 26);
    if (out == NULL) return NULL;
    (*env)->SetLongArrayRegion(env, out, 0, 26, raw);
    return out;
}
