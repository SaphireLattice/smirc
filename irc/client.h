#ifndef SMIRC_CLIENT_H
#define SMIRC_CLIENT_H
#include <unitypes.h>
#define MAX_CLIENTS 8

struct client {
    int id;
    struct client** clients;
    int max_clients;
    int socket;
    pthread_t cl_thread;
    int state;
    char* nick;
};
void* client_callback(void* arg);
struct client** get_clients();
struct client* add_client(struct client** clients);
void rem_client(struct client* client);

#endif //SMIRC_CLIENT_H
