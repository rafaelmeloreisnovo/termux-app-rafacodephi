#include <jni.h>
#include <string.h>

#include "bootstrap_baremetal_guard.h"

JNIEXPORT jint JNICALL
Java_com_termux_app_BootstrapBaremetalGuard_selftestNative(JNIEnv* env, jclass cls, jobject out, jint cap) {
    char* out_ptr;
    (void)cls;
    if (!out || cap <= 0) return -1;
    out_ptr = (char*)(*env)->GetDirectBufferAddress(env, out);
    if (!out_ptr) return -1;
    return raf_bootstrap_guard_selftest(out_ptr, cap);
}

JNIEXPORT jint JNICALL
Java_com_termux_app_BootstrapBaremetalGuard_validatePrefixNative(JNIEnv* env, jclass cls, jstring prefix, jobject out, jint cap) {
    const char* prefix_utf = NULL;
    char* out_ptr;
    int rc;
    (void)cls;
    if (!out || cap <= 0) return -1;
    out_ptr = (char*)(*env)->GetDirectBufferAddress(env, out);
    if (!out_ptr) return -1;
    if (prefix) prefix_utf = (*env)->GetStringUTFChars(env, prefix, NULL);
    rc = raf_bootstrap_guard_validate_prefix(prefix_utf, out_ptr, cap);
    if (prefix_utf) (*env)->ReleaseStringUTFChars(env, prefix, prefix_utf);
    return rc;
}
