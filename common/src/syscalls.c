/* Minimal newlib hooks for a bare-metal image.
 *
 * Firmware I/O goes through platform_debug_* rather than POSIX descriptors,
 * and dynamic allocation is intentionally unavailable.  Providing explicit
 * stubs keeps accidental libc use deterministic and avoids libnosys warnings.
 */

#include <stddef.h>
#include <sys/stat.h>
#include <sys/types.h>

/* newlib calls these hooks by symbol name and does not provide a public
 * declaration header for bare-metal ports.  Keep the contracts beside the
 * implementations so strict prototype checking still covers them. */
int _close(int file);
int _fstat(int file, struct stat *status);
int _isatty(int file);
off_t _lseek(int file, off_t offset, int whence);
int _read(int file, char *buffer, int length);
int _write(int file, const char *buffer, int length);
void *_sbrk(ptrdiff_t increment);
int _getpid(void);
int _kill(int process, int signal);
void _exit(int status);

int _close(int file)
{
    (void)file;
    return -1;
}

int _fstat(int file, struct stat *status)
{
    (void)file;
    if (status != NULL) {
        status->st_mode = S_IFCHR;
    }
    return 0;
}

int _isatty(int file)
{
    (void)file;
    return 1;
}

off_t _lseek(int file, off_t offset, int whence)
{
    (void)file;
    (void)offset;
    (void)whence;
    return (off_t)-1;
}

int _read(int file, char *buffer, int length)
{
    (void)file;
    (void)buffer;
    (void)length;
    return -1;
}

int _write(int file, const char *buffer, int length)
{
    (void)file;
    (void)buffer;
    (void)length;
    return -1;
}

void *_sbrk(ptrdiff_t increment)
{
    (void)increment;
    return (void *)-1;
}

int _getpid(void)
{
    return 1;
}

int _kill(int process, int signal)
{
    (void)process;
    (void)signal;
    return -1;
}

void _exit(int status)
{
    (void)status;
    for (;;) {
    }
}
