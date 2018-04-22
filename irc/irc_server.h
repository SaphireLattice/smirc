#ifndef SMIRC_SERVER_H
#define SMIRC_SERVER_H

#include <poll.h>
#include "irc_client.h"

enum {
  SOCKET_NULL = 0,
  SOCKET_SERVER_IRC = 1,
  SOCKET_CLIENT_IRC = 2,
  SOCKET_CLIENT_TELNET = 3
};

struct fd_map {
  struct pollfd* fd;
  int fd_type;
};

struct irc_socket {
  int fd_id;
  struct sockaddr_in addr;
  int addr_size;
  struct pollfd* fds;
  nfds_t fd_num;
  nfds_t fd_max;
  struct fd_map* fd_map;
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

  int quit;
  int debug;
};


int socket_register(struct irc_socket* isocket, int fd, int type);
int socket_unregister(struct irc_socket* isocket, int fd);
int socket_init(struct irc_socket* isocket, char* address, int port);
int server_init(struct irc_server* server);
void server_free(struct irc_server* server);
void server_onsocket(struct irc_server* server);

#endif //SMIRC_SERVER_H
