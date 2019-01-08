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
    sem_t *server_full;

    sem_t **clients_empty;
    sem_t **clients_full;

    sem_t *my_client_empty;
    sem_t *my_client_full;
} MsgSynch;

char *append_prefix(char *buff, char *prefix, char *str) {
    sprintf(buff, "%s_%s", prefix, str);
    return buff;
}


#define create_sem(name, val)  server ? force_create_sem(name, val) : bind_sem(name)

MsgSynch *synch_create_server(char *prefix) {
    MsgSynch *s = malloc(sizeof(MsgSynch));
    char name[TOKEN_BUFF_SIZE];
    s->server_mutex = force_create_sem(append_prefix(name, prefix, "server_mutex"), 1);
    s->server_empty = force_create_sem(append_prefix(name, prefix, "server_empty"), 1);
    s->server_full = force_create_sem(append_prefix(name, prefix, "server_busy"), 0);
    s->server = true;
    s->prefix = malloc(strlen(prefix) + 1);
    strcpy(s->prefix, prefix);

    //TODO Optimize clients by holding only pointer to his/her semaphore
    s->clients_empty = malloc(sizeof(void *) * MAX_CLIENTS);
    s->clients_full = malloc(sizeof(void *) * MAX_CLIENTS);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        sprintf(name, "%s_clb_%d", prefix, i);
        s->clients_full[i] = force_create_sem(name, 0);
        sprintf(name, "%s_cle_%d", prefix, i);
        s->clients_empty[i] = force_create_sem(name, 1);
    }
    s->my_client_empty = NULL;
    s->my_client_full = NULL;
    return s;
}

MsgSynch *synch_create_client(char *prefix, int client_id) {
    assertion(client_id >= 0 && client_id < MAX_CLIENTS);
    MsgSynch *s = malloc(sizeof(MsgSynch));
    char name[TOKEN_BUFF_SIZE];
    s->server_mutex = bind_sem(append_prefix(name, prefix, "server_mutex"));
    s->server_empty = bind_sem(append_prefix(name, prefix, "server_empty"));
    s->server_full = bind_sem(append_prefix(name, prefix, "server_busy"));
    s->server = false;
    s->prefix = malloc(strlen(prefix) + 1);
    strcpy(s->prefix, prefix);

    sprintf(name, "%s_clb_%d", prefix, client_id);
    s->my_client_full = bind_sem(name);
    sprintf(name, "%s_cle_%d", prefix, client_id);
    s->my_client_empty = bind_sem(name);
    s->clients_empty = NULL;
    s->clients_full = NULL;
    return s;
}


void synch_destroy(MsgSynch *synch) {
    if (synch->server) {
        char *prefix = synch->prefix;
        char name[TOKEN_BUFF_SIZE];
        sem_close(synch->server_full);
        sem_close(synch->server_mutex);
        sem_close(synch->server_empty);
        sem_unlink(append_prefix(name, prefix, "server_mutex"));
        sem_unlink(append_prefix(name, prefix, "server_empty"));
        sem_unlink(append_prefix(name, prefix, "server_busy"));
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            sprintf(name, "%s_clb_%d", prefix, i);
            sem_close(synch->clients_empty[i]);
            sem_unlink(name);
            sprintf(name, "%s_cle_%d", prefix, i);
            sem_close(synch->clients_full[i]);
            sem_unlink(name);
        }
        free(synch->clients_full);
        free(synch->clients_empty);
    }
    free(synch->prefix);
    free(synch);
}

/******************************* Messaging ******************************************
 **********************************************************************************/

typedef struct msg_buffer {
    char shmem_name[TOKEN_BUFF_SIZE];
    char server[MSG_BUFF_LEN];
    char clients[MAX_CLIENTS][MSG_BUFF_LEN];
} MsgBuffer;


MsgBuffer *msgb_init(char *name, bool server) {
    MsgBuffer *ptr = shared_mem_get(name, sizeof(MsgBuffer), server);
    strcpy(ptr->shmem_name, name);
    return ptr;
}

void msgb_close(MsgBuffer *buffer) {
    shared_mem_close(buffer->shmem_name, buffer, sizeof(MsgBuffer));
}


/******************************* IPC  *********************************************
 **********************************************************************************/

typedef struct ipc_manage {
    MsgBuffer *buff;
    MsgSynch *synch;
    bool is_server;
} IpcManager;


IpcManager *ipc_create(bool server, char *prefix, int client_id) {
    assert((server && client_id== -1) || (!server && client_id >= 0));
    IpcManager *mngr = malloc(sizeof(IpcManager));
    mngr->is_server = server;
    mngr->buff = msgb_init(prefix, server);
    mngr->synch = server ? synch_create_server(prefix) : synch_create_client(prefix, client_id);
    return mngr;
}

void ipc_close(IpcManager *mngr) {
    if (mngr->is_server) {
        msgb_close(mngr->buff);
    }
    synch_destroy(mngr->synch);
    free(mngr);
}


void ipc_getfrom_server(IpcManager *m, char *buff, int client_id){
    assertion(!m->is_server && "Only client can call this procedure");
    sem_wait(m->synch->my_client_full);
    strcpy(buff, m->buff->clients[client_id]);
    sem_post(m->synch->my_client_empty);
}

void ipc_sendto_client(IpcManager *m, char *msg, int client_id) {
    assertion(m->is_server && "Only server can call this procedure");
    assertion(strlen(msg) < MSG_BUFF_LEN - 1 && "Message is too long");
    sem_wait(m->synch->clients_empty[client_id]);
    strcpy(m->buff->clients[client_id], msg);
    sem_post(m->synch->clients_full[client_id]);
}

void ipc_getfrom_client(IpcManager *m, char *buff){
    assertion(m->is_server && "Only server can call this procedure");
    sem_wait(m->synch->server_full);
    strcpy(buff, m->buff->server);
    sem_post(m->synch->server_empty);
}

void ipc_sendto_server(IpcManager *m, char *msg){
    assertion(!m->is_server && "Only client can call this procedure");
    assertion(strlen(msg) < MSG_BUFF_LEN - 1 && "Message is too long");
    sem_wait(m->synch->server_mutex);
    sem_wait(m->synch->server_empty);
    strcpy(m->buff->server, msg);
    sem_post(m->synch->server_full);
    sem_post(m->synch->server_mutex);
}


#endif //ESCROOM_MESSAGING_H
