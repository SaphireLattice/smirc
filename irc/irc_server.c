#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <utils.h>
#include "irc_server.h"
#include "commands.h"
#include "../config.h"

void server_init(struct irc_server* server) {
    commands_init(server);
    int iSetOption = 1;
    server->clients = calloc(sizeof(struct irc_client*), MAX_CLIENTS + 1);
    memset(server->clients, 0, sizeof(struct irc_client*) * (MAX_CLIENTS + 1));

    //Create socket
    server->socket.socket_description = socket(AF_INET , SOCK_STREAM , IPPROTO_TCP);
    if (server->socket.socket_description == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");

    setsockopt(server->socket.socket_description, SOL_SOCKET, SO_REUSEADDR, (char*)&iSetOption,
               sizeof(iSetOption));

    int *port = malloc(sizeof(int));
    *port = 8989;
    //Prepare the sockaddr_in structure
    server->socket.addr.sin_family = AF_INET;
    server->socket.addr.sin_addr.s_addr = inet_addr((char*) config_value_get_soft(server->config, "irc.address", TYPE_STRING, "127.0.0.1")->data);
    server->socket.addr.sin_port = htons( (uint16_t) *((int*) config_value_get_soft(server->config, "irc.port", TYPE_INT, port)->data));

    printf("Addr: %s\n", inet_ntoa(server->socket.addr.sin_addr));
    printf("Port: %i\n", ntohs(server->socket.addr.sin_port));
    if( bind(server->socket.socket_description,(struct sockaddr *)&server->socket.addr , sizeof(server->socket.addr)) < 0) {
        //print the error message
        perror("Bind failed. Error");
        pthread_exit(0);
    }
    puts("Bind done.");

    //Listen
    listen(server->socket.socket_description, 3);

    server->socket.desc_size = sizeof(struct sockaddr_in);
}

void* server_loop(void* arg) {
    struct irc_server* server = (struct irc_server*) arg;

    //Init loop vars
    int* client_sock = malloc(sizeof(int));
    struct sockaddr_in client;

    puts("Waiting for incoming connections...");
    //accept connection from an incoming client
    while (1) {
        *client_sock = accept(server->socket.socket_description, (struct sockaddr *)&client, (socklen_t*)&server->socket.desc_size);
        if (*client_sock < 0) {
            perror("Accept failed");
            break;
        }
        puts("Connection accepted");
        struct irc_client* cinfo = add_client(server);
        if (cinfo != 0) {
            int sock = *client_sock;
            cinfo->socket = sock;
            printf("Creating client thread..\n");
            int err = pthread_create(&(cinfo->cl_thread), NULL, &client_callback, cinfo);
            if (err != 0)
                printf("Can't create thread: [%s]", strerror(err));
            else {
                pthread_join(cinfo->cl_thread, 0);
                printf("Thread %lu created successfully\n", cinfo->cl_thread);
            }
        } else {
            printf("Error: out of client slots!\n");
            close(*client_sock);
        }
    }
    free(client_sock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server->clients[i] != 0)
            rem_client(server->clients[i]);
    }
    free(server->clients);
    free(server);

    return 0;
}