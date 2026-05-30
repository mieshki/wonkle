/* Minimal syscall stubs — satisfy newlib-nano linker requirements.
 * These are never called at runtime; the firmware uses RTT for all I/O
 * and never allocates heap memory.
 * Without these, `--specs=nosys.specs` emits linker warnings. */

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

/* Heap — not used; return error to prevent accidental allocation */
extern char end; /* defined by linker script */

caddr_t _sbrk(int incr) {
    (void)incr;
    errno = ENOMEM;
    return (caddr_t)-1;
}

/* File I/O — not used */
int _write(int fd, char *buf, int len) { (void)fd; (void)buf; return len; }
int _read(int fd, char *buf, int len)  { (void)fd; (void)buf; return len; }
int _close(int fd)                     { (void)fd; return -1; }
int _lseek(int fd, int ptr, int dir)   { (void)fd; (void)ptr; (void)dir; return -1; }
int _isatty(int fd)                    { (void)fd; return 0; }
int _fstat(int fd, struct stat *st)    { (void)fd; (void)st; errno = EBADF; return -1; }
