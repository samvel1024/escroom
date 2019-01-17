#ifndef ESCROOM_UTILS_H
#define ESCROOM_UTILS_H

#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <stdbool.h>
#include <sys/errno.h>
#include <execinfo.h>
#include <semaphore.h>

void print_stacktrace();

#define assertion(__expr) {\
int __expr_val = (__expr); \
if (__expr_val == 0) {\
    fprintf( stderr,"[" #__expr "] assertion failed, ret_val=%d, errno=%d, error_str=%s pid=%d\n", __expr_val, errno, strerror(errno), getpid()); \
    fprintf( stderr, "Location: file=%s, line=%d\n", __FILE__, __LINE__); \
    print_stacktrace(); \
    exit(1); \
}\
}

#ifdef ENABLE_ASSERT
#define dassert(__expr) assertion(__expr) // debugging assertion
#else
#define dassert(expr)
#endif

void *shared_mem_get(char *name, unsigned long size, bool init);

void shared_mem_close(char *mem_name, void *shared_mem, size_t size);

sem_t *force_create_sem(char *sem_name, int val);

sem_t *bind_sem(char *sem_name);

void log_init(char *name, FILE *f);

void log_info(const char *format, ...);

void log_debug(const char *format, ...);

char *arr_to_str(short *arr, short end, char *buff);

#endif // ESCROOM_UTILS_H
