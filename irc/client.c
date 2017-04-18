#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "client.h"
#include "server.h"
#include "../mud/client.h"

struct client** g_clients = 0;


char** sanitize(char* str) {
    size_t len = strlen(str);
    char* blob = calloc(sizeof(char), len);
    char** pointers = calloc(sizeof(char*), 8);
    int parts = 0;
    int blob_ptr = 0;
    pointers[++parts] = blob;
    for (size_t str_ptr = 0; str_ptr <= len; str_ptr++) {
        switch(str[str_ptr]) {
            case '\r':
                break;
            case '\n':
                blob[blob_ptr++] = '\0';
                pointers[++parts] = blob + blob_ptr;
                break;
            default:
                blob[blob_ptr++] = str[str_ptr];
        }
    }
    pointers[0] = malloc(sizeof(char));
    pointers[0][0] = (char) (parts - 1);
    return pointers;
}

struct client** get_clients() {
    return g_clients;
}

struct client* add_client(struct client** clients) {
    g_clients = clients;
    int i = 0;
    for (; (i < MAX_CLIENTS && clients[i] != 0); i++);
    if (i == MAX_CLIENTS)
        return 0;
    clients[i] = malloc(sizeof(struct client));
    clients[i]->id = i;
    clients[i]->state = 0;
    clients[i]->clients = clients;
    clients[i]->nick = calloc(sizeof(char), 32);
    for (int j = 0; j < 32; j++) clients[i]->nick[j] = 0;
    return clients[i];
}

void rem_client(struct client* client) {
    int i = 0;
    for (; (client->clients[i] != 0 && client != client->clients[i]); i++);
    if (i == client->max_clients)
        return;
    client->clients[i] = client->clients[client->max_clients];
    free(client->nick);
    free(client);
}

void* client_callback(void* arg) {
    int i = 0;
    struct client* cinfo = (struct client*) arg;
    long read_size;
    char* client_message = calloc(sizeof(char), 2048);
    memset(client_message, 0, sizeof(char) * 2048);

    //Receive a message from client
    while( (read_size = recv(cinfo->socket, client_message, 2048, 0)) > 0 ) {
        char** sanitized = sanitize(client_message);
        for (int msg = 1; msg <= (int) sanitized[0][0]; msg++) {
            printf("%i> %s\n", cinfo->id, sanitized[msg]);

            // Handle the message
            char *first = malloc(sizeof(char) * 256);
            bzero(first, sizeof(char) * 256);
            for (i = 0; (sanitized[msg][i] != ' '); i++) {
                first[i] = sanitized[msg][i];
            }
            first[++i] = '\0';
            printf("First word: %s\n", first);

            if (strcmp(first, "NICK") == 0) {
                printf("Setting nick to: %s\n", sanitized[msg]);
                char* new_nick = calloc(sizeof(char), 32);
                bzero(new_nick, 32 * sizeof(char));
                size_t copy = 31;
                if (strlen(sanitized[msg]) - 5 < copy)
                    copy = strlen(sanitized[msg]) - 5;
                memccpy(new_nick, sanitized[msg] + 5 * sizeof(char), sizeof(char), copy);

                if (strcmp(cinfo->nick, new_nick) != 0) {
                    server_send_client(cinfo, "NICK", new_nick);
                    free(cinfo->nick);
                    cinfo->nick = new_nick;
                } else
                    free(new_nick);

                if (cinfo->state == 0) {
                    server_send_numeric(cinfo, 1, ":sup");
                    server_join_channel(cinfo, "#smirc");
                    cinfo->state = 1;
                    char** mud_buffer = get_line_buffer();
                    for (int j=0; (mud_buffer[j] != 0); j++) {
                        char* cat = calloc(sizeof(char), strlen(mud_buffer[j]) + 9);
                        strcpy(cat, "#smirc :");
                        strcat(cat, mud_buffer[j]);

                        server_send(cinfo, ">","PRIVMSG", cat);
                        free(cat);
                    }
                }
            }
            if (strcmp(first, "PRIVMSG") == 0) {
                char* tmp = calloc(sizeof(char), 7);
                bzero(tmp, 7);
                memcpy(tmp, sanitized[msg] + 8, 6);
                if (strcmp(tmp, "#smirc") == 0) {
                    printf("M<%s\n",sanitized[msg] + 16);
                    send_line_mud(sanitized[msg] + 16);
                }
                free(tmp);
            }
            if (strcmp(first, "PING") == 0) {
                server_send_plain(cinfo, "PONG", sanitized[msg] + 5);
            }
            printf("Nick: %s\n", cinfo->nick);
            free(first);
        }
        free(sanitized[0]);
        free(sanitized[1]);
        free(sanitized);
        memset(client_message, 0, sizeof(char) * 2048);
    }
    free(client_message);

    if(read_size == 0) {
        puts("Client disconnected");
        fflush(stdout);
    }

    else if(read_size == -1) {
        perror("Socket recv failed");
    }

    rem_client(cinfo);
    return 0;
}