#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static int read_exact(int fd, void *buf, size_t n) {
  uint8_t *p = (uint8_t *)buf;
  size_t off = 0;
  while (off < n) {
    ssize_t r = read(fd, p + off, n - off);
    if (r <= 0) return -1;
    off += (size_t)r;
  }
  return 0;
}

static int has_eocd(int fd, off_t size) {
  static uint8_t buf[65557];
  off_t win = size > (off_t)sizeof(buf) ? (off_t)sizeof(buf) : size;
  if (lseek(fd, size - win, SEEK_SET) < 0) return 0;
  if (read_exact(fd, buf, (size_t)win) != 0) return 0;
  for (off_t i = win - 4; i >= 0; --i) {
    if (buf[i] == 0x50 && buf[i + 1] == 0x4b && buf[i + 2] == 0x05 && buf[i + 3] == 0x06) return 1;
    if (i == 0) break;
  }
  return 0;
}

int main(int argc, char **argv) {
  if (argc != 2) return 2;
  const char *path = argv[1];
  int fd = open(path, O_RDONLY);
  if (fd < 0) return 3;

  struct stat st;
  if (fstat(fd, &st) != 0 || st.st_size < 22) {
    close(fd);
    return 4;
  }

  uint8_t sig[4];
  if (read_exact(fd, sig, 4) != 0) {
    close(fd);
    return 5;
  }
  if (!(sig[0] == 0x50 && sig[1] == 0x4b && sig[2] == 0x03 && sig[3] == 0x04)) {
    close(fd);
    return 6;
  }

  if (!has_eocd(fd, st.st_size)) {
    close(fd);
    return 7;
  }

  close(fd);
  return 0;
}
