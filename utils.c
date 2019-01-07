#include "utils.h"
#include <semaphore.h>
#include <stdarg.h>

/******************************* Logging ******************************************
 **********************************************************************************/
#define LOG_NAME_SIZE 50

char log_prefix[LOG_NAME_SIZE + 1];

void print_stacktrace() {
    void *buffer[50];
    int levels = backtrace(buffer, 50);
    backtrace_symbols_fd(buffer + 1, levels - 1, 2);
}

void log_init(char *name) {
    assertion(strlen(name) < LOG_NAME_SIZE)
    strcpy(log_prefix, name);
}

#ifdef DEBUG
void log_debug(const char *format, ...) {
    printf("[PID=%d %s] ", getpid(), log_prefix);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    if (format[strlen(format) - 1] != '\n') {
        printf("\n");
    }
    va_end(args);
}
#else
void log_debug(const char *format, ...) {}
#endif


/******************************* Shared memory ************************************
 **********************************************************************************/




void *shared_mem_get(char *mem_name, unsigned long size) {
    int file_desc = shm_open(mem_name, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
    if (errno == EEXIST) {
        log_debug("Shared mem already created opening the existing one");
        file_desc = shm_open(mem_name, O_RDWR, S_IRUSR | S_IWUSR);
    } else {
        log_debug("Opened shared memory");
        assertion(ftruncate(file_desc, size) != -1);
    }
    assertion(file_desc != 1 && "Shm_open failed");

    char *shared_mem = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, file_desc, 0);
    close(file_desc);
    assertion(shared_mem != MAP_FAILED);
    return shared_mem;
}

void shared_mem_close(char *mem_name, void *shared_mem, size_t size) {
    shm_unlink(mem_name);
    munmap(shared_mem, size);
}


void *init_sem(bool init) {
    int flag = init ? (O_RDWR | O_CREAT | O_EXCL) : O_RDWR;
    sem_t *sem = sem_open(SEM_NAME, flag, S_IRUSR | S_IWUSR, 1);
    if (sem == SEM_FAILED && errno == EEXIST && init) {
        log_debug("Semaphore already created, deleting the existing");
        sem_unlink(SEM_NAME);
        sem = sem_open(SEM_NAME, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR, 1);
    } else {
        log_debug("Created a new semaphore");
    }
    assertion(sem != SEM_FAILED);
    return sem;
}


