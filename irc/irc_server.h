#ifndef SMIRC_SERVER_H
#define SMIRC_SERVER_H

struct irc_socket {
  int socket_description;
  int desc_size;
  struct sockaddr_in addr;
};

struct irc_mud {
  struct minfo* mud;
  struct irc_mud* next;
};

struct irc_server {
  char* name;
  struct irc_client** clients;
  struct irc_socket socket;
  struct irc_mud* mud;

  struct cmd* commands;
  struct config* config;

  int debug;
};

#include "irc_client.h"

void server_init(struct irc_server*);
void* server_loop(void*);

#endif //SMIRC_SERVER_H
