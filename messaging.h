#ifndef ESCROOM_MESSAGING_H
#define ESCROOM_MESSAGING_H

#include "utils.h"

#define MSG_BUFF_LEN 7000
#define MAX_CLIENTS 1025

typedef struct msg_buffer {
    char shmem_name[16];
    char server[MSG_BUFF_LEN];
    char clients[MAX_CLIENTS][MSG_BUFF_LEN];
} MsgBuffer;


MsgBuffer *msgb_init(char *shmem_name, bool init) {
    MsgBuffer *ptr = shared_mem_get(shmem_name, sizeof(MsgBuffer), init);
    strcpy(ptr->shmem_name, shmem_name);
    return ptr;
}

void msgb_close(MsgBuffer *buffer) {
    shared_mem_close(buffer->shmem_name, buffer, sizeof(MsgBuffer));
}


#endif //ESCROOM_MESSAGING_H
