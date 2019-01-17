/**
 * Single item buffer producers and consumers implementation
 * using POSIX semaphores and shared memory
 */

#include "ipc.h"
#include <semaphore.h>
#include <pthread.h>
#include "utils.h"

#define MSG_BUFF_LEN 4200
#define MAX_CLIENTS 1025
#define TOKEN_BUFF_SIZE 40

/******************************* Synchronization  **********************************
 ***********************************************************************************/

/**
 * Contains necessary semaphore instances for server and client
 * Server creates array of semaphores for each client and server,
 * whereas client binds only his and server's semaphore
 */
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

MsgSynch *synch_create_server(char *prefix) {
  MsgSynch *s = malloc(sizeof(MsgSynch));
  char name[TOKEN_BUFF_SIZE];
  s->server_mutex = force_create_sem(append_prefix(name, prefix, "server_mutex"), 1);
  s->server_empty = force_create_sem(append_prefix(name, prefix, "server_empty"), 1);
  s->server_full = force_create_sem(append_prefix(name, prefix, "server_busy"), 0);
  s->server = true;
  s->prefix = malloc(strlen(prefix) + 1);
  strcpy(s->prefix, prefix);

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
  dassert(client_id >= 0 && client_id < MAX_CLIENTS);
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

/******************************* Messaging ****************************************
 **********************************************************************************/
/**
 * Buffers for communication between processes
 * Server can write to all client buffers.
 * Clients can write only to servers buffer
 */
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
typedef struct q_node_t {
  struct q_node_t *next;
  char bytes[MSG_BUFF_LEN];
  int bytes_len;
  int client_id;
} QNode;

/**
 * Brings all above structs together to provide a uniform API for creation and destruction
 */
struct ipc_manager_t {
  MsgBuffer *buff;
  MsgSynch *synch;
  bool is_server;
  QNode *q_head;
  QNode *q_tail;
  pthread_t out_thread;
  pthread_mutex_t lock;
  pthread_cond_t queue_not_empty;
  bool finished;
};

void async_enqueue(IpcManager *ipc, char *bytes, int size, int client) {
  pthread_mutex_lock(&(ipc->lock));

  QNode *new = malloc(sizeof(QNode));
  new->next = NULL;
  new->bytes_len = size;
  new->client_id = client;
  memcpy(new->bytes, bytes, size);

  if (ipc->q_head == NULL) {
    dassert(ipc->q_tail == NULL);
    ipc->q_head = new;
    ipc->q_tail = new;
  } else {
    dassert(ipc->q_tail != NULL && ipc->q_head != NULL);
    ipc->q_tail->next = new;
    ipc->q_tail = ipc->q_tail->next;
  }
  pthread_cond_signal(&(ipc->queue_not_empty));
  pthread_mutex_unlock(&(ipc->lock));
}

QNode *dequeue(IpcManager *ipc) {
  if (ipc->q_head == NULL) {
    dassert(ipc->q_tail == NULL);
    return NULL;
  }
  QNode *ans = ipc->q_head;
  if (ipc->q_head == ipc->q_tail) {
    ipc->q_tail = NULL;
    ipc->q_head = NULL;
  } else {
    ipc->q_head = ipc->q_head->next;
  }
  return ans;
}

void *out_thread_loop(void *ptr) {
  IpcManager *ipc = ptr;
  while (!ipc->finished || ipc->q_head != NULL) {
    pthread_mutex_lock(&(ipc->lock));
    while (ipc->q_head == NULL && !ipc->finished) {
      pthread_cond_wait(&(ipc->queue_not_empty), &(ipc->lock));
    }
    QNode *node = dequeue(ipc);
    if (node != NULL) {
      sem_wait(ipc->synch->clients_empty[node->client_id]);
      memcpy(ipc->buff->clients[node->client_id], node->bytes, node->bytes_len);
      sem_post(ipc->synch->clients_full[node->client_id]);
      free(node);
    }
    pthread_mutex_unlock(&(ipc->lock));
  }

  return NULL;
}

void join_out_thread(IpcManager *ipc) {
  pthread_mutex_lock(&(ipc->lock));
  ipc->finished = true;
  pthread_cond_signal(&(ipc->queue_not_empty));
  pthread_mutex_unlock(&(ipc->lock));
  void *arg;
  pthread_join(ipc->out_thread, &arg);
}

IpcManager *ipc_create(bool server, char *prefix, int client_id) {
  assert((server && client_id == -1) || (!server && client_id >= 0));
  IpcManager *mngr = malloc(sizeof(IpcManager));
  mngr->is_server = server;
  mngr->buff = msgb_init(prefix, server);
  mngr->synch = server ? synch_create_server(prefix) : synch_create_client(prefix, client_id);
  mngr->q_head = NULL;
  mngr->q_tail = NULL;
  mngr->finished = false;
  pthread_create(&mngr->out_thread, NULL, out_thread_loop, mngr);
  return mngr;
}

void ipc_close(IpcManager *mngr) {
  join_out_thread(mngr);
  if (mngr->is_server) {
    msgb_close(mngr->buff);
  }
  synch_destroy(mngr->synch);
  free(mngr);
}

void ipc_getfrom_server(IpcManager *m, void *buff, int size, int client_id) {
  dassert(!m->is_server && "Only client can call this procedure");
  sem_wait(m->synch->my_client_full);
  memcpy(buff, m->buff->clients[client_id], size);
  sem_post(m->synch->my_client_empty);
}

void ipc_sendto_client(IpcManager *m, void *msg, int size, int client_id) {
  dassert(m->is_server && "Only server can call this procedure");
  dassert(size < MSG_BUFF_LEN && "Message is too long");
  async_enqueue(m, msg, size, client_id);
}

void ipc_getfrom_client(IpcManager *m, void *buff, int size) {
  dassert(m->is_server && "Only server can call this procedure");
  sem_wait(m->synch->server_full);
  memcpy(buff, m->buff->server, size);
  sem_post(m->synch->server_empty);
}

void ipc_sendto_server(IpcManager *m, void *msg, int size) {
  dassert(!m->is_server && "Only client can call this procedure");
  dassert(size < MSG_BUFF_LEN && "Message is too long");
  sem_wait(m->synch->server_mutex);
  sem_wait(m->synch->server_empty);
  memcpy(m->buff->server, msg, size);
  sem_post(m->synch->server_full);
  sem_post(m->synch->server_mutex);
}

