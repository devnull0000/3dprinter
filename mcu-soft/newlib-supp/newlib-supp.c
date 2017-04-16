#include "errno.h"
#include <sys/stat.h>
#include <unistd.h>
/*#include <time.h>*/
#include <sys/times.h>
#include "command.h"    //YS: it's ours! for enum visibility...

#undef errno
extern int errno;

void _exit(int status)
{
    while(1);
}

int _close(int file) {
  return -1;
}

char *__env[1] = { 0 };
char **environ = __env;

int _execve(const char *__path, char * const __argv[], char * const __envp[]) {
  errno = ENOMEM;
  return -1;
}

int _fork(void) {
  errno = EAGAIN;
  return -1;
}

int _fstat(int file, struct stat *st) {
  st->st_mode = S_IFCHR;
  return 0;
}

int _getpid(void) {
  return 1;
}

int _isatty(int file) {
  return 1;
}

int _kill(int pid, int sig) {
  errno = EINVAL;
  return -1;
}

int _link(const char *__path1, const char *__path2) {
  errno = EMLINK;
  return -1;
}

off_t _lseek(int __fildes, off_t __offset, int __whence) {
  return 0;
}

int _open(const char *name, int flags, int mode) {
  return -1;
}

int _read(int __fd, void *__buf, size_t __nbyte) {
  return 0;
}

void * _sbrk(ptrdiff_t incr) {
  extern char _end;		/* Defined by the linker */
  static char *heap_end;
  char *prev_heap_end;
 
  if (heap_end == 0) {
    heap_end = &_end;
  }
  prev_heap_end = heap_end;
  if (heap_end + incr >= (char *)&incr) {
    write (1, "Heap and stack collision\n", 25);
    while(1);
  }

  heap_end += incr;
  return (void *) prev_heap_end;
}

int _stat(const char *__restrict __path, struct stat *__restrict st) {
  st->st_mode = S_IFCHR;
  return 0;
}

clock_t _times(struct tms *buf) {
  return -1;
}

int _unlink(char const * name) {
  errno = ENOENT;
  return -1; 
}

int _wait(int *status) {
  errno = ECHILD;
  return -1;
}

//void cmd_send_message_to_host(enum EPrinterEventType type, char const * msg);
int _write(int file, void const *ptr, size_t len) {
    enum EPrinterEventType type = (file == 1 ? ePrinterEventType_Msg : ePrinterEventType_Err);
    cmd_send_message_to_host(type, ptr, (int)len);
    
    return len;
}
