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
    assert(count > 0 && "Count has to be greater than 0");
    int id = atoi(argv[1]);
    char buff[MSG_BUFF_LEN];
    sprintf(buff, "Adder-%d", id);

    log_init(buff);
    IpcManager *ipc = ipc_create(false, "a", id);

    sprintf(buff, "Hello server, from client %d", id);
    ipc_sendto_server(ipc, buff);
    ipc_getfrom_server(ipc, buff, id);
    log_debug("Message from server: %s", buff);

    ipc_close(ipc);
}