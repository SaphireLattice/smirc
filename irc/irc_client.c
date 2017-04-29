#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <error.h>

#include "irc_client.h"
#include "irc_server.h"
#include "../mud/client.h"
#include "../utils.h"
#include "commands.h"

char** sanitize(char* str) {
    size_t len = strlen(str);
    char* blob = calloc(sizeof(char), len + 2);
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

struct irc_client* add_client(struct irc_server* server) {
    struct irc_client** clients = server->clients;
    int i = 0;
    for (; (i < MAX_CLIENTS && clients[i] != 0); i++);
    if (i == MAX_CLIENTS)
        return 0;
    clients[i] = malloc(sizeof(struct irc_client));
    clients[i]->id = i;
    clients[i]->state = 0;
    clients[i]->server = server;
    clients[i]->nick = calloc(sizeof(char), 32);
    memset(clients[i]->nick, 0, 32);
    return clients[i];
}

void rem_client(struct irc_client* client) {
    int i = 0;
    for (; (client->server->clients[i] != 0 && client != client->server->clients[i]); i++);
    if (i == MAX_CLIENTS)
        return;
    client->server->clients[i] = 0;
    free(client->nick);
    free(client);
}

void* client_callback(void* arg) {
    int quit = 0;
    struct irc_client* cinfo = (struct irc_client*) arg;
    long read_size;
    char* client_message = calloc(sizeof(char), 2048);
    memset(client_message, 0, sizeof(char) * 2048);
    char **word = calloc(sizeof(char *), 256);

    //Receive a message from client
    while( (quit != 1) && (read_size = recv(cinfo->socket, client_message, 2048, 0)) > 0 ) {
        char** sanitized = sanitize(client_message);
        for (int msg = 1; msg <= (int) sanitized[0][0]; msg++) {
            printf("%i> %s\n", cinfo->id, sanitized[msg]);

            char *ptr = sanitized[msg];
            memset(word, 0, sizeof(char *) * 256);
            word[0] = calloc(sizeof(char), 64);
            memset(word[0], 0, sizeof(char) * 64);
            word[1] = ptr;
            int words = 0;
            int w = 0;

            while (*ptr != '\0') {
                if (*ptr == ' ') {
                    words+=2;
                    word[words] = calloc(sizeof(char), 64);
                    memset(word[words], 0, sizeof(char) * 64);
                    word[words + 1] = ptr + 1;
                    w = 0;
                } else
                    word[words][w++] = *ptr;
                ptr++;
            }
            words /= 2;

            printf("First word: %s\n", word[0]);

            if (strcmp(word[0], "NICK") == 0) {
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
                    server_join_channel(cinfo, "#mcp");
                    cinfo->state = 1;

                    struct irc_mud* next = cinfo->server->mud;
                    while (next != 0) {
                        size_t l = strlen(next->mud->name) + 3;
                        char* chan = calloc(sizeof(char), l);
                        memset(chan, 0, sizeof(char) * l);
                        strcpy(chan + 1, next->mud->name);
                        chan[0] = '#';
                        server_join_channel(cinfo, chan);

                        char** mud_buffer = get_line_buffer(next->mud);
                        for (int j=0; (mud_buffer[j] != 0); j++) {
                            server_send_user_channel(cinfo, chan + 1, ">", mud_buffer[j]);
                        }
                        free(chan);
                        next = next->next;
                    }
                }
            }
            if (strcmp(word[0], "PRIVMSG") == 0) {
                if (strcmp(word[2], "#smirc") == 0) {
                    commands_run(cinfo, word[4] + 1, words - 2, &(word[6]));
                } else {
                    struct minfo* mud = get_mud(cinfo->server, word[2]);
                    if (mud != 0) {
                        send_line_mud(mud, word[5] + 1);
                    }
                }
            }
            if (strcmp(word[0], "PING") == 0) {
                server_send_plain(cinfo, "PONG", word[2]);
            }
            if (cinfo->server->debug)
                printf("I> Nick: %s\n", cinfo->nick);

            for(int i = 0; (i < 256) && (word[i] != 0); i+=2) {
                if (cinfo->server->debug)
                    printf("%i: %s\n", i/2, word[i]);
                free(word[i]);
            }
        }
        free(sanitized[0]);
        free(sanitized[1]);
        free(sanitized);
        memset(client_message, 0, sizeof(char) * 2048);
    }
    free(client_message);
    free(word);

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


void server_send_plain(struct irc_client* clt, char* command, char* message) {
    ssize_t written = 0;
    written += write(clt->socket, command, strlen(command) * sizeof(char));
    written += write(clt->socket, " ", 1 * sizeof(char));
    written += write(clt->socket, message, strlen(message) * sizeof(char));

    written += write(clt->socket, "\r\n", 2 * sizeof(char));
    if (written == 0)
        error(-1, 1, "well fuck");
}

void server_send_numeric(struct irc_client* clt, int number, char *message) {
    ssize_t written = 0;
    written += write(clt->socket, ":", 1 * sizeof(char));
    written += write(clt->socket, clt->server->name, strlen(clt->server->name) * sizeof(char));
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

void server_send(struct irc_client* clt, char* who, char* command, char *message) {
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

void server_send_client(struct irc_client* clt, char* command, char* message) {
    size_t len = 0;
    len += strlen(clt->nick);
    len += strlen(clt->nick);
    len += strlen(clt->server->name);
    len += 3;
    char* who = calloc(sizeof(char), len);
    strcat(who, clt->nick);
    strcat(who, "!");
    strcat(who, clt->nick);
    strcat(who, "@");
    strcat(who, clt->server->name);

    server_send(clt, who, command, message);

    free(who);
}

void server_join_channel(struct irc_client* clt, char *channame) {
    server_send_client(clt, "JOIN", channame);
}

void server_send_user_channel(struct irc_client* client, char* channel, char* sender, char* message) {
    char* cat = calloc(sizeof(char), strlen(message) + strlen(channel) + strlen(sender) + 4);
    strcpy(cat, "#");
    strcat(cat, channel);
    strcat(cat, " :");
    strcat(cat, message);

    server_send(client, sender, "PRIVMSG", cat);

    printf(" %s\n", cat);
    free(cat);
}

void server_send_channel(struct irc_server* server, char* channel, char* sender, char* message) {
    char* cat = calloc(sizeof(char), strlen(message) + strlen(channel) + strlen(sender) + 4);
    strcpy(cat, "#");
    strcat(cat, channel);
    strcat(cat, " :");
    strcat(cat, message);

    struct irc_client** clients = server->clients;
    if (clients != 0)
        for(int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] != 0) {
                printf("%i<", i);
                server_send(clients[i], sender, "PRIVMSG", cat);
            }
        }
    printf(" %s\n", cat);
    free(cat);
}