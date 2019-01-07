#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/wait.h>
#include "messaging.h"
#include <stdbool.h>
#include <string.h>

#define BUFF_SIZE 1024
#define SLEEP_LEN 1000000

int main() {
    log_init("Server");

    MsgBuffer *buffer = msgb_init("client_server");
    sprintf(buffer->server, "Message from server");
    void *begin = buffer;
    log_debug("Start of buffer: %p\n", buffer);
    int pid;
    int child_count = 7;
    for (int i = 0; i < child_count; ++i) {
        switch (pid = fork()) {
            case -1:
                assertion(pid != -1);
                break;
            case 0:
                log_debug("Starting client");
                char arg_str[50];
                sprintf(arg_str, "%d", i);
                execlp("./client", "./client", arg_str, NULL);
                assertion(false && "Could not start adder");
        }
    }

    sleep(1);
    for (int i = 0; i < child_count; ++i) {
        void *ptr = &(buffer->clients[i]);
        log_debug("Message from %d: %s", i, buffer->clients[i]);
    }

    wait(NULL);
    msgb_close(buffer);
}