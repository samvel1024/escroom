#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/wait.h>
#include "messaging.h"
#include <stdbool.h>
#include <string.h>


int main() {
    log_init("Server");

    IpcManager *ipc = ipc_create(true, "a", -1);
    int pid;
    int child_count = 10;
    for (int i = 0; i < child_count; ++i) {
        switch (pid = fork()) {
            case -1: {
                assertion(false && "Could not fork");
                break;
            }
            case 0: {
                char str[50];
                sprintf(str, "%d", i);
                execlp("./client", "./client", str, NULL);
                assertion(false && "Could not start adder");
            }
        }
    }

    sleep(1);
    char buff[MSG_BUFF_LEN];
    for (int i = 0; i < child_count; ++i) {
        int client = -1;
        ipc_getfrom_client(ipc, buff);
        sscanf(buff, "Hello server, from client %d", &client);
        log_debug("Message from client %d: %s", client, buff);
        sprintf(buff, "Echo from server to client %d", client);
        ipc_sendto_client(ipc, buff, client);
    }

    wait(NULL);
    ipc_close(ipc);
}