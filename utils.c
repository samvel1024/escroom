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
  printf("[PID=%d %s DEBUG] ", getpid(), log_prefix);
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

void log_info(const char *format, ...){
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
}

char *arr_to_str(int *arr, int end, char *buff){
  int len = 1;
  sprintf(buff, "(");
  char num_buff[20];
  for(int i=0; arr[i] != end; ++i){
    sprintf(num_buff, i == 0 ? "%d": ", %d", arr[i]);
    size_t num_len = strlen(num_buff);
    sprintf(buff + len, "%s", num_buff);
    len += num_len;
  }
  sprintf(buff + len , "%c", ')');
  return buff;
}


/******************************* Shared memory ************************************
 **********************************************************************************/




void *shared_mem_get(char *mem_name, unsigned long size, bool init) {
  if (init)
    unlink(mem_name);
  int file_desc = shm_open(mem_name, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
  if (errno == EEXIST) {
    file_desc = shm_open(mem_name, O_RDWR, S_IRUSR | S_IWUSR);
  } else {
    log_debug("Opened new shared memory");
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

sem_t *force_create_sem(char *sem_name, int val) {
  int flag = (O_RDWR | O_CREAT | O_EXCL);
  sem_t *sem = sem_open(sem_name, flag, S_IRUSR | S_IWUSR, val);
  if (sem == SEM_FAILED && errno == EEXIST) {
    log_debug("Semaphore already created, deleting the existing");
    sem_unlink(sem_name);
    sem = sem_open(sem_name, flag, S_IRUSR | S_IWUSR, val);
  }
  assertion(sem != SEM_FAILED);
  return sem;
}

sem_t *bind_sem(char *sem_name) {
  sem_t *sem = sem_open(sem_name, O_RDWR, S_IRUSR | S_IWUSR, 0);
  assertion(sem != SEM_FAILED);
  return sem;
}


