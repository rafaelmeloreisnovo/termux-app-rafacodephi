/* HOSTED_BOUNDARY ONLY — this file is NOT freestanding L0.
 * Android JNI is confined to this adapter. No native allocation, persistent
 * global state, logging, Java callback, GPU, NUMA or OS syscall is used here.
 * JNI arrays remain caller-owned; no pointer escapes after this call.
 * Transfer success is NOT actual execution; output[RAF_OUT_EXECUTED] == 0.
 */
#include <jni.h>
#include "vendor_pinned/raf_mw.h"
_Static_assert(sizeof(jint) == sizeof(raf_mw_u32), "JNI int/ABI mismatch");

JNIEXPORT jint JNICALL
Java_com_termux_app_rafaelia_RafaeliaMiddlewareBridge_nativePlan(
    JNIEnv *env, jclass unused, jintArray request, jintArray result) {
  raf_mw_u32 input[RAF_MW_WORDS], output[RAF_MW_WORDS];
  (void)unused;
  if (request == (jintArray)0 || result == (jintArray)0) return (jint)-1;
  if ((*env)->IsSameObject(env, request, result) == JNI_TRUE) return (jint)-2;
  if ((*env)->GetArrayLength(env, request) != (jsize)RAF_MW_WORDS ||
      (*env)->GetArrayLength(env, result) != (jsize)RAF_MW_WORDS)
    return (jint)-3;
  /* jint and uint32 are matching signed/unsigned aliases for the exact
     Android 32-bit word. The JNI transport copies; no pointer is retained. */
  (*env)->GetIntArrayRegion(env, request, 0, (jsize)RAF_MW_WORDS,
                            (jint *)(void *)input);
  if ((*env)->ExceptionCheck(env)) return (jint)-4;
  raf_mw_route(input, RAF_MW_WORDS, output, RAF_MW_WORDS);
  if (output[RAF_OUT_EXECUTED] != 0u ||
      output[RAF_OUT_ABI] != RAF_MW_ABI) return (jint)-5;
  (*env)->SetIntArrayRegion(env, result, 0, (jsize)RAF_MW_WORDS,
                            (const jint *)(const void *)output);
  if ((*env)->ExceptionCheck(env)) return (jint)-4;
  return (jint)0; /* JNI transport success, not a workload success. */
}
