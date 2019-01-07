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

// helper-function to print the current stack trace
void print_stacktrace();

#define assertion(__expr) {\
int __expr_val = (__expr); \
if (__expr_val == 0) {\
    fprintf( stderr,"[" #__expr "] assertion failed, ret_val=%d, errno=%d, error_str=%s\n", __expr_val, errno, strerror(errno)); \
    fprintf( stderr, "Location: file=%s, line=%d\n", __FILE__, __LINE__); \
    print_stacktrace(); \
    exit(1); \
}\
}

void *shared_mem_get(char *name, unsigned long size, bool init);

void shared_mem_close(char *mem_name, void *shared_mem, size_t size);

void *force_create_sem(char *sem_name, int val);

void *bind_sem(char *sem_name);

void log_init(char *);

void log_debug(const char *format, ...);


#endif // ESCROOM_UTILS_H
