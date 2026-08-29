/**
 * verbovivo_jni.c — JNI Bridge for Verbovivo Convergence Engine
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Android JNI interface for T^7 toroid convergence and receipt generation.
 * Freestanding: no libc, no malloc, direct syscalls only.
 */
#include <jni.h>
#include <stdint.h>

/* Include Verbovivo headers */
#include "../../../verbovivo_graph.h"

/* Global graph (static, allocated once at JNI load) */
static T7ToroidGraph g_verbovivo_graph;
static int g_graph_initialized = 0;

/**
 * JNI_OnLoad — Initialize Verbovivo graph at library load time
 */
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    (void)vm;
    (void)reserved;

    /* Build T^7 toroid graph */
    if (vv_build_t7_toroid(&g_verbovivo_graph) != 0) {
        return 0;  /* fail to load */
    }

    /* Verify coherence */
    if (vv_verify_t7_coherence(&g_verbovivo_graph) != 0) {
        return 0;  /* fail to load */
    }

    g_graph_initialized = 1;
    return JNI_VERSION_1_6;
}

/**
 * Java: com.termux.rafaelia.VerbativoCore.executeConvergence()
 *
 * Executes convergence walk and returns φ_fst value.
 * Returns -1 if validation fails (fail-closed).
 */
JNIEXPORT jlong JNICALL
Java_com_termux_rafaelia_VerbativoCore_executeConvergence(
    JNIEnv *env,
    jobject thiz) {

    (void)env;
    (void)thiz;

    if (!g_graph_initialized) {
        return -1;  /* graph not ready */
    }

    /* Set starting node to attractor 0 */
    g_verbovivo_graph.current_node = g_verbovivo_graph.attractor_node_ids[0];

    /* Run convergence walk */
    uint64_t phi_fst = 0;
    uint8_t attractor_id = 255;
    vv_graph_converge(&g_verbovivo_graph, 5000, &phi_fst, &attractor_id);

    /* Validate bounds */
    if (phi_fst > 0x10000u) {
        return -1;  /* out of bounds */
    }

    return (jlong)phi_fst;
}

/**
 * Java: com.termux.rafaelia.VerbativoCore.computePhiMetrics()
 *
 * Computes entropy, coherence, and φ for current graph state.
 * Stores results in provided long array [H_norm, C_norm, φ_fst].
 * Returns 0 on success, -1 on failure.
 */
JNIEXPORT jint JNICALL
Java_com_termux_rafaelia_VerbativoCore_computePhiMetrics(
    JNIEnv *env,
    jobject thiz,
    jlongArray out_metrics) {

    (void)thiz;

    if (!g_graph_initialized || !out_metrics) {
        return -1;
    }

    /* Get array pointer */
    jlong *metrics = (*env)->GetLongArrayElements(env, out_metrics, NULL);
    if (!metrics) {
        return -1;
    }

    /* Compute φ components */
    uint64_t h_norm = 0, c_norm = 0, phi_fst = 0;
    vv_graph_compute_phi(&g_verbovivo_graph, &h_norm, &c_norm, &phi_fst);

    /* Return metrics */
    metrics[0] = (jlong)h_norm;
    metrics[1] = (jlong)c_norm;
    metrics[2] = (jlong)phi_fst;

    (*env)->ReleaseLongArrayElements(env, out_metrics, metrics, 0);
    return 0;
}

/**
 * Java: com.termux.rafaelia.VerbativoCore.recallAttractor()
 *
 * Find attractor nearest to query vector via Hamming distance.
 * Query is provided as byte array (128 bytes = 1024 bits in binary).
 * Returns attractor ID (0-41), or 255 if none found.
 */
JNIEXPORT jint JNICALL
Java_com_termux_rafaelia_VerbativoCore_recallAttractor(
    JNIEnv *env,
    jobject thiz,
    jbyteArray query_bytes) {

    (void)thiz;

    if (!g_graph_initialized || !query_bytes) {
        return 255;
    }

    /* Get query array */
    jbyte *query_data = (*env)->GetByteArrayElements(env, query_bytes, NULL);
    if (!query_data) {
        return 255;
    }

    /* Construct HyperVector from byte array */
    HyperVector query;
    for (uint32_t i = 0; i < HV_LANES; i++) {
        uint64_t lane = 0;
        for (int j = 0; j < 8; j++) {
            lane |= ((uint64_t)(query_data[i * 8 + j] & 0xFF)) << (j * 8);
        }
        query.lane[i] = lane;
    }

    (*env)->ReleaseByteArrayElements(env, query_bytes, query_data, JNI_ABORT);

    /* Perform recall */
    uint8_t result = vv_graph_recall(&g_verbovivo_graph, &query, 5000);
    return (jint)result;
}

/**
 * Java: com.termux.rafaelia.VerbativoCore.validateConvergenceReceipt()
 *
 * Validate a convergence receipt structure.
 * Receipt format (binary): H_norm(8) | C_norm(8) | φ_fst(8) | attractor_id(1) | status(1)
 * Returns 0 if valid, -1 if invalid (fail-closed).
 */
JNIEXPORT jint JNICALL
Java_com_termux_rafaelia_VerbativoCore_validateConvergenceReceipt(
    JNIEnv *env,
    jobject thiz,
    jbyteArray receipt_bytes) {

    (void)thiz;

    if (!receipt_bytes) {
        return -1;
    }

    /* Get receipt array */
    jbyte *receipt_data = (*env)->GetByteArrayElements(env, receipt_bytes, NULL);
    if (!receipt_data) {
        return -1;
    }

    jsize len = (*env)->GetArrayLength(env, receipt_bytes);
    (*env)->ReleaseByteArrayElements(env, receipt_bytes, receipt_data, JNI_ABORT);

    /* Minimal validation */
    if (len < 26) {
        return -1;  /* receipt too short */
    }

    /* Parse fields (big-endian) */
    uint64_t h_norm = 0, c_norm = 0, phi_fst = 0;
    for (int i = 0; i < 8; i++) {
        h_norm |= ((uint64_t)(receipt_data[i] & 0xFF)) << (56 - i * 8);
        c_norm |= ((uint64_t)(receipt_data[8 + i] & 0xFF)) << (56 - i * 8);
        phi_fst |= ((uint64_t)(receipt_data[16 + i] & 0xFF)) << (56 - i * 8);
    }
    uint8_t attractor_id = receipt_data[24] & 0xFF;
    uint8_t status = receipt_data[25] & 0xFF;

    /* Validate bounds */
    if (h_norm > 0x10000u || c_norm > 0x10000u || phi_fst > 0x10000u) {
        return -1;  /* out of bounds */
    }

    /* Validate φ = (1-H)·C */
    uint64_t one_minus_h = 0x10000u - h_norm;
    uint64_t expected_phi = (one_minus_h * c_norm) >> 16;
    if (expected_phi != phi_fst) {
        return -1;  /* φ calculation mismatch */
    }

    /* Validate attractor consistency */
    if (attractor_id < 42u && status != 0u) {
        return -1;  /* inconsistent attractor claim */
    }

    return 0;  /* valid */
}

/**
 * Java: com.termux.rafaelia.VerbativoCore.isInitialized()
 *
 * Check if graph is initialized and ready.
 * Returns 1 if ready, 0 if not.
 */
JNIEXPORT jint JNICALL
Java_com_termux_rafaelia_VerbativoCore_isInitialized(
    JNIEnv *env,
    jobject thiz) {

    (void)env;
    (void)thiz;

    return g_graph_initialized ? 1 : 0;
}
