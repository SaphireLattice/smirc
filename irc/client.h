#ifndef SMIRC_CLIENT_H
#define SMIRC_CLIENT_H

#include <unitypes.h>

#define MAX_CLIENTS 8

struct client {
  struct irc_server* server;
  int id;
  int max_clients;
  int socket;
  pthread_t cl_thread;
  int state;
  char* nick;
};

void* client_callback(void* arg);
struct client* add_client(struct irc_server* server);
void rem_client(struct client* client);

void server_send_plain(struct client* clt, char* command, char* message);
void server_send_numeric(struct client* clt, int number, char *message);
void server_send(struct client* clt, char* who, char* command, char *message);
void server_send_client(struct client* clt, char* command, char* message);
void server_join_channel(struct client* clt, char *channame);
void server_send_user_channel(struct client* client, char* channel, char* sender, char* message);
void server_send_channel(struct irc_server* server, char* channel, char* sender, char* message);

#endif //SMIRC_CLIENT_H
