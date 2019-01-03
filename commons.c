#include "commons.h"
#include <zconf.h>
#include <semaphore.h>

typedef struct Logger Logger;

Logger *instance;


void print_stacktrace() {

    void *buffer[MAX_STACK_LEVELS];
    int levels = backtrace(buffer, MAX_STACK_LEVELS);

    // print to stderr (fd = 2), and remove this function from the trace
    backtrace_symbols_fd(buffer + 1, levels - 1, 2);
}


void Logger_init(char *name) {
    instance = malloc(sizeof(Logger));
    instance->proc_name = malloc(strlen(name) + 1);
    strcpy(instance->proc_name, name);
    instance->pid = getpid();
}


void Logger_log(const char *format, ...) {
#ifdef DEBUG
    printf("[PID=%d %s] ", getpid(), instance->proc_name);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    if (format[strlen(format) - 1] != '\n') {
        printf("\n");
    }
    va_end(args);
#endif
}


void Logger_destruct() {
    free(instance->proc_name);
    free(instance);
}

void *shared_mem = NULL;
int file_desc = -1;

void *shared_mem_get(bool create) {
    if (shared_mem != NULL) return shared_mem;

    file_desc = shm_open(SHM_NAME, (create ? O_CREAT : 0) | O_RDWR, S_IRUSR | S_IWUSR);
    assertion(file_desc != 1 && "Shm_open failed");
    if (create) assertion(ftruncate(file_desc, BUFF_SIZE) != -1);

    void *mapped_mem = mmap(NULL, BUFF_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, file_desc, 0);
    assertion(mapped_mem != MAP_FAILED);
    shared_mem = mapped_mem;
    return mapped_mem;
}

void shared_mem_close() {
    assertion(file_desc != -1 && shared_mem != NULL);
    close(file_desc);
    munmap(shared_mem, BUFF_SIZE);
}

void shared_mem_unlink() {
    shm_unlink(SHM_NAME);
}


void *init_sem() {
    sem_t *sem = sem_open(SEM_NAME, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, 0);
    assertion(sem != SEM_FAILED);
//    exit_if_err(sem_close(sem) == 0);
    return sem;
}