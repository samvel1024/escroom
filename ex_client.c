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
#include <fcntl.h>
#include <assert.h>
#include "messaging.h"
#include <string.h>



int main(int count, char *argv[]) {
    log_init("Adder");
    log_debug("Started adder\n");
    assert(count > 0 && "Count has to be greater than 0");
    int id = atoi(argv[1]);

    IpcManager *ipc = ipc_create(false, "a");
    MsgBuffer *buff = ipc->buff;
    log_debug("CLient %d Server: %s", id,  buff->server);
    sprintf(buff->clients[id], "Hello from client %d", id);
    log_debug("Sent message");
    ipc_close(ipc);
}