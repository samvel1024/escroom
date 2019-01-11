#ifndef ESCROOM_IPC_H
#define ESCROOM_IPC_H

#include <stdbool.h>

typedef struct ipc_manager_t IpcManager;

struct ipc_manager_t *ipc_create(bool server, char *prefix, int client_id);

void ipc_close(struct ipc_manager_t *mngr);

void ipc_getfrom_server(struct ipc_manager_t *m, void *buff, int size, int client_id);

void ipc_sendto_client(struct ipc_manager_t *m, void *msg, int size, int client_id);

void ipc_getfrom_client(struct ipc_manager_t *m, void *buff, int size);

void ipc_sendto_server(struct ipc_manager_t *m, void *msg, int size);

#endif //ESCROOM_MESSAGING_H
