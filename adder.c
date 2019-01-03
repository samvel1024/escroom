#include <time.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <assert.h>
#include "commons.h"
#include <string.h>

int *shm;


int main(int count, char *argv[]) {
    Logger_init("Adder");
    shm = shared_mem_get(false);
    Logger_log("Started adder\n");
    assert(count > 0 && "Count has to be greater than 0");
    int times = atoi(argv[1]);
    assert(times > 0);

    sem_t *s = init_sem();

    sleep(1000);

    for(int i=0; i<times; ++i){
        sem_wait(s);
        shm[0]++;
        sem_post(s);
    }

    shared_mem_close();
    Logger_destruct();
}