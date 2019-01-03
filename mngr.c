#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/wait.h>
#include "commons.h"
#include <assert.h>
#include <stdbool.h>

#define BUFF_SIZE 1024

int main() {
    Logger_init("Manager");
    int *shm = shared_mem_get(true);
    shm[0] = 0;

    Logger_log("value before %d", shm[0]);
    int pid;
    int child_count = 10;
    init_sem();
    for (int i = 0; i < child_count; ++i) {
        switch (pid = fork()) {
            case -1:
                assert(pid != -1);
                break;
            case 0:
                Logger_log("Starting adder");
                execlp("./adder", "./adder", "10000", NULL);
                assert(false && "Could not start adder");
        }

    }
    for (int i = 0; i < child_count; ++i) {
        int w_pid = wait(0);
        assert(w_pid != -1 && "Ellrror in wait");
        Logger_log("Value of shm %d", shm[0]);
    }

    shared_mem_close();
    shared_mem_unlink();
    Logger_destruct();
}