#ifndef ESCROOM_COMMONS_H
#define ESCROOM_COMMONS_H

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


#define SHM_NAME "/arbabyk"
#define SEM_NAME "/semaporli"
#define BUFF_SIZE 1024

#define MAX_STACK_LEVELS 50

// helper-function to print the current stack trace
void print_stacktrace();

#define assertion(expr) {\
int expr_val = expr; \
if (expr_val == 0) {\
    fprintf( stderr,"[" #expr "] assertion failed, ret_val=%d, errno=%d, error_str=%s\n", expr_val, errno, strerror(errno)); \
    fprintf( stderr, "Location: file=%s, line=%d\n", __FILE__, __LINE__); \
    print_stacktrace(); \
    exit(1); \
}\
}


void *shared_mem_get(bool create);

void shared_mem_close();

void shared_mem_unlink();

void *init_sem();


struct Logger {
    int pid;
    char *proc_name;
};

void Logger_init(char *);

void Logger_log(const char *, ...);

void Logger_destruct();

#endif //ESCROOM_COMMONS_H
