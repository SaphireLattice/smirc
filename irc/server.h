#ifndef SMIRC_SERVER_H
#define SMIRC_SERVER_H

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "client.h"

void server_loop();
void server_send_plain(struct client* clt, char* command, char* message);
void server_send_numeric(struct client* clt, int number, char *message);
void server_send(struct client* clt, char* who, char* command, char *message);
void server_send_client(struct client* clt, char* command, char* message);
void server_join_channel(struct client* clt, char *channame);

#endif //SMIRC_SERVER_H
