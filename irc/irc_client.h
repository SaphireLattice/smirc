#ifndef SMIRC_CLIENT_H
#define SMIRC_CLIENT_H

#include <unitypes.h>

#define MAX_CLIENTS 8

struct irc_client {
  struct sockaddr_in* addr;
  struct irc_server* server;
  int id;
  int socket;
  int state;
  char* nick;
};

void client_onsocket(struct irc_client* client);
struct irc_client* get_client(struct irc_server* server, int socket);
struct irc_client* add_client(struct irc_server* server);
void rem_client(struct irc_client* client);

void server_send_plain(struct irc_client* clt, char* command, char* message);
void server_send_numeric(struct irc_client* clt, int number, char *message);
void server_send(struct irc_client* clt, char* who, char* command, char *message);
void server_send_client(struct irc_client* clt, char* command, char* message);
void server_join_channel(struct irc_client* clt, char *channame);
void server_send_user_channel(struct irc_client* client, char* channel, char* sender, char* message);
void server_send_channel(struct irc_server* server, char* channel, char* sender, char* message);

#endif //SMIRC_CLIENT_H
