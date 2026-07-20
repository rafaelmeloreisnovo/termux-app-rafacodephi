#include <jni.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif
#ifndef O_NOFOLLOW
#define O_NOFOLLOW 0
#endif

#define MAX_EXTERNAL_BOOTSTRAP_BYTES (128u * 1024u * 1024u)
#define COPY_BUFFER_BYTES 8192
#define PACKAGE_NAME_BYTES 192

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

static int valid_package_char(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '.' || c == '_';
}

static int build_external_path(char *path, size_t path_size)
{
    char package_name[PACKAGE_NAME_BYTES];
    int fd = open("/proc/self/cmdline", O_RDONLY | O_CLOEXEC);
    if (fd < 0) return -1;
    ssize_t count = read(fd, package_name, sizeof(package_name) - 1u);
    close(fd);
    if (count <= 0) return -1;
    package_name[count] = '\0';

    size_t length = 0;
    while (length < (size_t) count && package_name[length] != '\0') {
        if (!valid_package_char(package_name[length])) return -1;
        length++;
    }
    if (length == 0 || length >= sizeof(package_name)) return -1;
    package_name[length] = '\0';

    int written = snprintf(
        path,
        path_size,
        "/data/data/%s/files/bootstrap-inbox/bootstrap-external.zip",
        package_name);
    if (written < 0 || (size_t) written >= path_size) return -1;
    return 0;
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

    char external_path[PATH_MAX];
    if (build_external_path(external_path, sizeof(external_path)) != 0) {
        return new_empty_array(env);
    }

    int fd = open(external_path, O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
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
