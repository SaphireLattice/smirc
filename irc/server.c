#include <error.h>
#include "server.h"
#include "../itoa.h"

void server_loop() {
    int socket_desc, c;
    struct sockaddr_in server, client;
    int iSetOption = 1;
    int* client_sock = malloc(sizeof(int));
    struct client** clients = calloc(sizeof(struct client*), MAX_CLIENTS);
    for (int i = 0; i < MAX_CLIENTS; i++)
        clients[i] = 0;

    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , IPPROTO_TCP);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
    setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, (char*)&iSetOption,
               sizeof(iSetOption));

    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 8989 );

    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0) {
        //print the error message
        perror("Bind failed. Error");
        pthread_exit(0);
    }
    puts("Bind done.");

    //Listen
    listen(socket_desc , 3);

    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);

    //accept connection from an incoming client
    while (1) {
        printf("S_\n");
        *client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
        if (*client_sock < 0) {
            perror("Accept failed");
            break;
        }
        puts("Connection accepted");
        struct client* cinfo = add_client(clients);
        int sock = *client_sock;
        cinfo->socket = sock;
        printf("SC\n");
        int err = pthread_create(&(cinfo->cl_thread), NULL, &client_callback, cinfo);
        printf("SP\n");

        if (err != 0)
            printf("Can't create thread :[%s]", strerror(err));
        else
            printf("Thread %lu created successfully\n", cinfo->cl_thread);
        printf("SE\n");
    }
    pthread_exit(0);
}

const char* server_name = "localhost\0";

void server_send_plain(struct client* clt, char* command, char* message) {
    ssize_t written = 0;
    written += write(clt->socket, command, strlen(command) * sizeof(char));
    written += write(clt->socket, " ", 1 * sizeof(char));
    written += write(clt->socket, message, strlen(message) * sizeof(char));

    written += write(clt->socket, "\r\n", 2 * sizeof(char));
    if (written == 0)
        error(-1, 1, "well fuck");
}

void server_send_numeric(struct client* clt, int number, char *message) {
    ssize_t written = 0;
    written += write(clt->socket, ":", 1 * sizeof(char));
    written += write(clt->socket, server_name, strlen(server_name) * sizeof(char));
    written += write(clt->socket, " ", 1 * sizeof(char));

    if (number < 100) {
        if (number < 10)
            written += write(clt->socket, "0", 1 * sizeof(char));
        written += write(clt->socket, "0", 1 * sizeof(char));
    }
    char* number_s = malloc(sizeof(char) * 3);
    itoa(number, number_s, 10);
    written += write(clt->socket, number_s, strlen(number_s) * sizeof(char));
    free(number_s);

    written += write(clt->socket, " ", 1 * sizeof(char));
    written += write(clt->socket, clt->nick, strlen(clt->nick) * sizeof(char));

    written += write(clt->socket, " ", 1 * sizeof(char));
    written += write(clt->socket, message, strlen(message) * sizeof(char));

    written += write(clt->socket, "\r\n", 2 * sizeof(char));
    if (written == 0)
        error(-1, 1, "well fuck");
}

void server_send(struct client* clt, char* who, char* command, char *message) {
    ssize_t written = 0;
    written += write(clt->socket, ":", 1 * sizeof(char));
    written += write(clt->socket, who, strlen(who) * sizeof(char));
    written += write(clt->socket, " ", 1 * sizeof(char));
    written += write(clt->socket, command, strlen(command) * sizeof(char));
    written += write(clt->socket, " ", 1 * sizeof(char));
    written += write(clt->socket, message, strlen(message) * sizeof(char));

    written += write(clt->socket, "\r\n", 2 * sizeof(char));
    if (written == 0)
        error(-1, 1, "well fuck");
}

void server_send_client(struct client* clt, char* command, char* message) {
    size_t len = 0;
    len += strlen(clt->nick);
    len += strlen(clt->nick);
    len += strlen(server_name);
    len += 3;
    char* who = calloc(sizeof(char), len);
    strcat(who, clt->nick);
    strcat(who, "!");
    strcat(who, clt->nick);
    strcat(who, "@");
    strcat(who, server_name);

    server_send(clt, who, command, message);

    free(who);
}

void server_join_channel(struct client* clt, char *channame) {
    server_send_client(clt, "JOIN", channame);
}