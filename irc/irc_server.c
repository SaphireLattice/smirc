#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <utils.h>
#include "irc_server.h"
#include "commands.h"
#include "../config.h"
#include "../mud/client.h"

int on = 1;
int socket_register(struct irc_socket* isocket, int fd, int type) {
    int empty;
    for (empty = 0; empty < isocket->fd_max && isocket->fds[empty].fd != -1; empty++);
    if (empty == isocket->fd_max) {
        puts("Registering socket failed, out of space.");
        return -1;
    }
    printf("Registering socket #%d (fd%d) of type %d\n", empty, fd, type);

    isocket->fds[empty].fd = fd;
    isocket->fds[empty].events = POLLIN;
    isocket->fd_num = (empty + 1 > isocket->fd_num) ? empty + 1 : isocket->fd_num;
    isocket->fd_map[empty] = (struct fd_map) {&isocket->fds[empty], type};

    ioctl(fd, FIONBIO, (char*) &on);

    return 0;
}

int socket_unregister(struct irc_socket* isocket, int fd) {
    int found;
    for (found = 0; found < isocket->fd_max && isocket->fds[found].fd != fd; found++);
    if (found == isocket->fd_max) {
        return 0; // No such socket, all ok! Kinda.
    }

    isocket->fds[found].fd = -1;
    isocket->fds[found].events = 0;
    isocket->fd_map[found].fd = NULL;
    isocket->fd_map[found].fd_type = SOCKET_NULL;
    close(fd);

    return 0;
}

int socket_init(struct irc_socket* isocket, char* address, int port) {
    //Create socket
    isocket->fd_id = socket(AF_INET , SOCK_STREAM , IPPROTO_TCP);
    if (isocket->fd_id == -1) {
        perror("Socket creation failed");
        return -1;
    }
    puts("Socket created");

    setsockopt(isocket->fd_id, SOL_SOCKET, SO_REUSEADDR,
               (char*) &on, sizeof(on));
    ioctl(isocket->fd_id, FIONBIO, (char*) &on);

    isocket->addr.sin_family = AF_INET;
    isocket->addr.sin_addr.s_addr = inet_addr(address);
    isocket->addr.sin_port = htons((uint16_t) port);
    isocket->addr_size = sizeof(struct sockaddr_in);

    if (bind(isocket->fd_id, (struct sockaddr *) &isocket->addr, (socklen_t) isocket->addr_size) < 0) {
        perror("Bind failed");
        return -1;
    }
    puts("Bind done.");

    if (listen(isocket->fd_id, 3) < 0) {
        perror("Listen failed");
        return -1;
    }
    puts("Listening on socket...");

    isocket->fd_max = 128;
    isocket->fds = calloc(isocket->fd_max, sizeof(struct pollfd));
    isocket->fd_map = calloc(isocket->fd_max, sizeof(struct fd_map));
    memset(isocket->fds, -1, sizeof(struct pollfd) * isocket->fd_max);

    socket_register(isocket, isocket->fd_id, SOCKET_SERVER_IRC);

    return 0;
}

int server_init(struct irc_server* server) {
    commands_init(server);
    server->clients = calloc(sizeof(struct irc_client*), MAX_CLIENTS + 1);
    memset(server->clients, 0, sizeof(struct irc_client*) * (MAX_CLIENTS + 1));


    int *port = malloc(sizeof(int));
    *port = 8989;

    char* confaddress = config_value_get_soft(server->config, "irc.address", TYPE_STRING,
                          (union config_data) {"127.0.0.1"})->data.as_string;
    int* confport = config_value_get_soft(server->config, "irc.port", TYPE_INT,
                              (union config_data) {port})->data.as_int;
    if (confport != port)
        free(port);

    socket_init(&server->socket, confaddress, *confport);

    printf("Addr: %s\n", inet_ntoa(server->socket.addr.sin_addr));
    printf("Port: %i\n", ntohs(server->socket.addr.sin_port));

    struct config_value* servers = config_value_get(server->config, "mud");
    if (servers != 0) {
        struct config_block* block = servers->data.as_section;
        while (block != 0) {
            for (int i = 0; i < CONFIG_BLOCK_SIZE; i++) {
                if (block->data[i] != 0 &&
                    config_section_get_value(block->data[i]->data.as_section, "autoconnect") != NULL &&
                    *(config_section_get_value(block->data[i]->data.as_section, "autoconnect")->data.as_int) == 1)
                {
                    struct minfo* mud = malloc(sizeof(struct minfo));
                    mud->ircserver = server;
                    mud->name = strdup(block->data[i]->name);

                    mud->address = config_section_get_value(block->data[i]->data.as_section, "address")->data.as_string;
                    mud->port = *(config_section_get_value(block->data[i]->data.as_section, "port")->data.as_int);
                    mud->use_ssl = *(config_section_get_value(block->data[i]->data.as_section, "ssl")->data.as_int);

                    add_mud(mud);
                    mud_init(mud);
                }
            };
            block = block->next;
        }
    }
    return 0;
}

void server_free(struct irc_server* server) {
    while (server->mud != NULL) {
        free_mud(server->mud->mud);
    }
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server->clients[i] != 0)
            rem_client(server->clients[i]);
    }
    commands_free(server);
    free(server->socket.fds);
    free(server->socket.fd_map);
    free(server->clients);
    free(server);
}

void server_onsocket(struct irc_server* server) {
    struct sockaddr_in* client_addr = malloc(sizeof(struct sockaddr_in));
    int client_sock = accept(server->socket.fd_id, (struct sockaddr *) client_addr, (socklen_t*)&server->socket.addr_size);
    if (client_sock < 0) {
        perror("Connection accept failed");
        return;
    }
    puts("Connection accepted.");
    struct irc_client* cinfo = add_client(server);
    if (cinfo != 0) {
        cinfo->addr = client_addr;
        cinfo->socket = client_sock;
        socket_register(&server->socket, cinfo->socket, SOCKET_CLIENT_IRC);
    } else {
        printf("Error: out of client slots!\n");
        close(client_sock);
    }
}