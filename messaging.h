#ifndef ESCROOM_MESSAGING_H
#define ESCROOM_MESSAGING_H

#include "utils.h"

#define MSG_BUFF_LEN 7000
#define MAX_CLIENTS 1025
#define TOKEN_BUFF_SIZE 40

/******************************* Synchronization objects ***************************
 ***********************************************************************************/

typedef struct msg_synch {
    bool server;
    char *prefix;

    sem_t *server_mutex;

    sem_t *server_empty;
    sem_t *server_busy;

    sem_t **client_empty;
    sem_t **client_busy;
} MsgSynch;
char *append_prefix(char *buff, char *prefix, char *str) {
    sprintf(buff, "%s_%s", prefix, str);
    return buff;
}


#define create_sem(name, val)  force_new ? force_create_sem(name, val) : bind_sem(name)
MsgSynch *synch_create(bool force_new, char *prefix) {
    MsgSynch *s = malloc(sizeof(MsgSynch));
    char name[TOKEN_BUFF_SIZE];
    s->server_mutex = create_sem(append_prefix(name, prefix, "server_mutex"), 1);
    s->server_empty = create_sem(append_prefix(name, prefix, "server_empty"), 0);
    s->server_busy = create_sem(append_prefix(name, prefix, "server_busy"), 0);
    s->server = force_new;
    s->prefix = malloc(strlen(prefix) + 1);
    strcpy(s->prefix, prefix);

    s->client_empty = malloc(sizeof(void *) * MAX_CLIENTS);
    s->client_busy = malloc(sizeof(void *) * MAX_CLIENTS);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        sprintf(name, "%s_clb_%d", prefix, i);
        s->client_busy[i] = create_sem(name, 0);
        sprintf(name, "%s_cle_%d", prefix, i);
        s->client_empty[i] = create_sem(name, 1);
    }
    return s;
}


void synch_destroy(MsgSynch *synch) {
    if (synch->server) {
        char *prefix = synch->prefix;
        char name[TOKEN_BUFF_SIZE];
        sem_close(synch->server_busy);
        sem_close(synch->server_mutex);
        sem_close(synch->server_empty);
        sem_unlink(append_prefix(name, prefix, "server_mutex"));
        sem_unlink(append_prefix(name, prefix, "server_mutex"));
        sem_unlink(append_prefix(name, prefix, "server_mutex"));
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            sprintf(name, "%s_clb_%d", prefix, i);
            sem_close(synch->client_empty[i]);
            sem_unlink(name);
            sprintf(name, "%s_cle_%d", prefix, i);
            sem_close(synch->client_busy[i]);
            sem_unlink(name);
        }
    }
    free(synch->prefix);
    free(synch->client_busy);
    free(synch->client_empty);
    free(synch);
}

/******************************* Messaging ******************************************
 **********************************************************************************/

typedef struct msg_buffer {
    char shmem_name[TOKEN_BUFF_SIZE];
    char server[MSG_BUFF_LEN];
    char clients[MAX_CLIENTS][MSG_BUFF_LEN];
    MsgSynch *synch;
} MsgBuffer;


MsgBuffer *msgb_init(char *name, bool server) {
    MsgBuffer *ptr = shared_mem_get(name, sizeof(MsgBuffer), server);
    strcpy(ptr->shmem_name, name);
    ptr->synch = synch_create(server, name);
    return ptr;
}

void msgb_close(MsgBuffer *buffer) {
    bool server = buffer->synch->server;
    synch_destroy(buffer->synch);
    if (server) {
        shared_mem_close(buffer->shmem_name, buffer, sizeof(MsgBuffer));
    }
}


#endif //ESCROOM_MESSAGING_H
