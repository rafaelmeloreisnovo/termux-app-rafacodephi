#include <jni.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif
#ifndef O_NOFOLLOW
#define O_NOFOLLOW 0
#endif

#define EXTERNAL_BOOTSTRAP_PATH \
    "/data/data/com.termux.rafacodephi/files/bootstrap-inbox/bootstrap-external.zip"
#define MAX_EXTERNAL_BOOTSTRAP_BYTES (128u * 1024u * 1024u)
#define COPY_BUFFER_BYTES 8192

extern jbyte blob[];
extern int blob_size;

static jbyteArray new_empty_array(JNIEnv *env)
{
    return (*env)->NewByteArray(env, 0);
}

static jbyteArray embedded_bootstrap(JNIEnv *env)
{
    if (blob_size <= 0) return new_empty_array(env);
    jbyteArray result = (*env)->NewByteArray(env, blob_size);
    if (result == NULL) return NULL;
    (*env)->SetByteArrayRegion(env, result, 0, blob_size, blob);
    return result;
}

static jbyteArray external_bootstrap(JNIEnv *env, int fd, off_t size)
{
    if (size <= 0 || (uint64_t) size > MAX_EXTERNAL_BOOTSTRAP_BYTES || size > INT_MAX) {
        return new_empty_array(env);
    }

    jbyteArray result = (*env)->NewByteArray(env, (jsize) size);
    if (result == NULL) return NULL;

    jbyte buffer[COPY_BUFFER_BYTES];
    off_t offset = 0;
    while (offset < size) {
        size_t remaining = (size_t) (size - offset);
        size_t requested = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
        ssize_t count = read(fd, buffer, requested);
        if (count <= 0) {
            (*env)->DeleteLocalRef(env, result);
            return new_empty_array(env);
        }
        (*env)->SetByteArrayRegion(env, result, (jsize) offset, (jsize) count, buffer);
        if ((*env)->ExceptionCheck(env)) return result;
        offset += count;
    }

    return result;
}

JNIEXPORT jbyteArray JNICALL
Java_com_termux_app_TermuxInstaller_getZip(JNIEnv *env, jclass clazz)
{
    (void) clazz;

    int fd = open(EXTERNAL_BOOTSTRAP_PATH, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (fd < 0) {
        if (errno == ENOENT) return embedded_bootstrap(env);
        return new_empty_array(env);
    }

    struct stat status;
    if (fstat(fd, &status) != 0 || !S_ISREG(status.st_mode)) {
        close(fd);
        return new_empty_array(env);
    }

    jbyteArray result = external_bootstrap(env, fd, status.st_size);
    close(fd);
    return result;
}
