#ifndef ESCROOM_MESSAGING_H
#define ESCROOM_MESSAGING_H

#include "utils.h"

#define MSG_BUFF_LEN 7000
#define MAX_CLIENTS 1025
#define TOKEN_BUFF_SIZE 40

typedef struct msg_buffer {
    char shmem_name[TOKEN_BUFF_SIZE];
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


typedef struct msg_synch {
    bool owned;
    sem_t *server_mutex;

    sem_t *server_empty;
    sem_t *server_busy;

    sem_t **client_empty;
    sem_t **client_busy;
} MsgSynch;


#define create_sem(name, val)  force_new ? force_create_sem(name, val) : bind_sem(name)

MsgSynch *sycnh_create(bool force_new) {
    MsgSynch *s = malloc(sizeof(MsgSynch));
    s->server_mutex = create_sem("server_mutex", 1);
    s->server_empty = create_sem("server_empty", 0);
    s->server_busy = create_sem("server_busy", 0);
    s->owned = force_new;

    s->client_empty = malloc(sizeof(void *) * MAX_CLIENTS);
    s->client_busy = malloc(sizeof(void *) * MAX_CLIENTS);
    char name[TOKEN_BUFF_SIZE];
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        sprintf(name, "client_busy_%d", i);
        s->client_busy[i] = create_sem(name, 0);
        sprintf(name, "client_empty_%d", i);
        s->client_empty[i] = create_sem(name, 1);
    }
    return s;
}

MsgSynch *synch_create_semaphores() {
    return sycnh_create(true);
}

MsgSynch *synch_bind_semaphores() {
    return sycnh_create(false);
}

void *synch_destroy(MsgSynch *synch) {
    if (synch->owned) {
        sem_close(synch->server_busy);
        sem_close(synch->server_mutex);
        sem_close(synch->server_empty);
        sem_unlink("server_busy");
        sem_unlink("server_mutex");
        sem_unlink("server_empty");
        char name[TOKEN_BUFF_SIZE];
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            sprintf(name, "client_empty_%d", i);
            sem_close(synch->client_empty[i]);
            sem_unlink(name);
            sprintf(name, "client_busy_%d", i);
            sem_close(synch->client_busy[i]);
            sem_unlink(name);
        }
    }
    free(synch->client_busy);
    free(synch->client_empty);
    free(synch);
}


#endif //ESCROOM_MESSAGING_H
