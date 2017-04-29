#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/ossl_typ.h>

#include "client.h"
#include "../ssl.h"
#include "telnet.h"
#include "mcp.h"
#ifndef STANDALONE_MUD
# include "../irc/irc_client.h"
# include "../irc/irc_server.h"
#endif

char** get_line_buffer(struct minfo* mud) {
    return mud->irc_buffer;
}

void send_msg_irc(struct minfo* mud, char* who, char* msg) {
    server_send_channel(mud->ircserver, mud->name, who, msg);
}

void send_line_irc(struct minfo* mud) {
    char* irc_line = ansi_to_irc_color(mud->user_buffer);
    mud->irc_length++;
    if (mud->irc_length > (MUD_IRC_BUFFER - 1)) {
        free(mud->irc_buffer[0]);
        memcpy(mud->irc_buffer, mud->irc_buffer + 1, sizeof(char *) * (MUD_IRC_BUFFER - 2));
        bzero(mud->irc_buffer + (MUD_IRC_BUFFER - 3), sizeof(char* ) * 2);
        mud->irc_length--;
    }
    mud->irc_buffer[mud->irc_length] = calloc(sizeof(char), strlen(irc_line) + 1);
    bzero(mud->irc_buffer[mud->irc_length], strlen(irc_line) + 1);
    strcpy(mud->irc_buffer[mud->irc_length], irc_line);
    send_msg_irc(mud, ">", irc_line);
    free(irc_line);
}

void send_line_mud(struct minfo* mud, char* line) {
    fputs("M<", stdout);
    fputs(line, stdout);
    fputs("\n", stdout);
    mud_write(mud, line, (int) strlen(line));
    mud_write(mud, "\r\n", 2);
}

int mud_write(struct minfo* mud, char* buffer, int length) {
    int status;
    if (mud->use_ssl == 1)
        status = SSL_write(mud->ssl, buffer, length);
    else
        status = (int) write(mud->socket, buffer, (size_t) length);
    return status;
}

int mud_read(struct minfo* mud) {
    int status;
    if (mud->use_ssl == 1)
        status = SSL_read(mud->ssl, mud->read_buffer, 1024);
    else
        status = (int) read(mud->socket, mud->read_buffer, 1024);
    return status;
}

void* mud_connect(void* arg) {
    struct minfo* mud = (struct minfo*) arg;
    printf("M> Starting thread ");
    send_msg_irc(mud, "!", "Starting connection thread");

    mud->socket = -2;
    mud->irc_buffer = calloc(sizeof(char *), MUD_IRC_BUFFER);
    memset(mud->irc_buffer, 0, MUD_IRC_BUFFER);
    mud->irc_length = -1;
    ssize_t n = 0;
    mud->line_length = 0;
    mud->user_length = 0;
    mud->read_buffer = calloc(sizeof(char), 1025);
    mud->line_buffer = calloc(sizeof(char), 1025);
    mud->user_buffer = calloc(sizeof(char), 1025);
    memset(mud->read_buffer, 0, 1025);
    memset(mud->line_buffer, 0, 1025);
    memset(mud->user_buffer, 0, 1025);

    mud->mcp_state = 0;

    struct sockaddr_in serv_addr;

    if((mud->socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\nError: Could not create socket \n");
        send_msg_irc(mud, "!", "Error: Could not create socket");
        free_mud(mud);
        return 0;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons((uint16_t) mud->port);

    struct hostent *he;
    if ( (he = gethostbyname( mud->address ) ) == NULL) {
        free_mud(mud);
        return 0;
    }
    serv_addr.sin_addr = *((struct in_addr **) he->h_addr_list)[0];
    printf("%s\n",inet_ntoa(serv_addr.sin_addr));

    if( connect(mud->socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nError: Connect Failed \n");
        send_msg_irc(mud, "!", "Error: Conection failed");
        free_mud(mud);
        return 0;
    }

    if (mud->use_ssl == 1) {
        printf("Initializing SSL...\n");
        InitializeSSL();
        const SSL_METHOD *meth = SSLv23_client_method();
        printf("%i", meth->version);
        SSL_CTX *ctx = SSL_CTX_new(meth);
        if ( ctx == NULL )
        {
            ERR_print_errors_fp(stderr);
            free_mud(mud);
            return 0;
        }
        SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, verify_callback);
        mud->ssl = SSL_new(ctx);

        SSL_set_fd(mud->ssl, mud->socket);

        printf("Connecting with SSL...");
        if (SSL_connect(mud->ssl) == -1) {
            printf(" FAIL\n");
            ERR_print_errors_fp(stderr);
            send_msg_irc(mud, "!", "Error: SSL Connection failed");
            free_mud(mud);
            return 0;
        }
        printf("\nConnected with %s encryption\n", SSL_get_cipher(mud->ssl));
        send_msg_irc(mud, "!", "SSL Connection established");
        ShowCerts(mud->ssl);
    }
    char* buf = calloc(sizeof(char), 3);
    buf[0] = IAC;
    buf[1] = DO;
    char* telnet_protocols = get_protocols();
    for (int p = 0; telnet_protocols[p] != 0; p++) {
        printf("Negotiating %X\n", (unsigned char) telnet_protocols[p]);
        buf[2] = telnet_protocols[p];
        mud_write(mud, buf , 3);
    }
    free(buf);

    printf("Entering read loop...\n");
    while ( (n = mud_read(mud)) > 0) {
        if (mud->ircserver->debug) {
            printf("M> RAW: \"%s\"\nM> HEX: ", mud->read_buffer);
            for (int i = 0; i < n; i++)
                printf("%X ", (unsigned char) mud->read_buffer[i]);
            printf("\n--------\n");
        }
        process_buffer(mud);
    }

    if(n == 0) {
        send_msg_irc(mud, "!", "Connection closed");
        printf("\nConnection closed\n");
    }

    if(n < 0) {
        send_msg_irc(mud, "!", "Read error");
        perror("Read error");
    }

    free_mud(mud);

    return 0;
}

void process_buffer(struct minfo* mud) {
    for (int i = 0; mud->read_buffer[i] != 0; i++) {
        switch(mud->read_buffer[i]) {
            default:
                if (mud->ircserver->debug)
                    printf("M> '%c' - %X\n", mud->read_buffer[i], (unsigned char) mud->read_buffer[i]);
                mud->line_buffer[mud->line_length++] = mud->read_buffer[i];
                if (mud->line_length < 1022)
                    break;
            case '\n':
                for (int j = 0; mud->line_buffer[j] != 0; j++) {
                    if (mud->line_buffer[j] == IAC) {
                        printf("T> IAC ");
                        char next = mud->line_buffer[++j];
                        char *cmd = ttoa((unsigned int) next);
                        printf("%s ", cmd);
                        free(cmd);
                        if (next == WILL || next == WONT || next == DO || next == DONT) {
                            char option = mud->line_buffer[++j];
                            char *opt = ttoa((unsigned int) option);
                            printf("%s", opt);
                            free(opt);
                        } else if (next == SB) {
                            char sub;
                            do {
                                sub = mud->line_buffer[++j];
                                printf("\nSub: %X (%i)", (unsigned char) sub, (unsigned char) sub);
                            } while (sub != IAC && mud->line_buffer[j + 1] != SE);
                            j+=2;
                        }
                        printf("\n");
                    } else {
                        if (mud->ircserver->debug)
                            printf("M> %i (%i): '%c' - %X\n", j, mud->user_length, mud->line_buffer[j], (unsigned char) mud->line_buffer[j]);
                        mud->user_buffer[mud->user_length++] = mud->line_buffer[j];
                    }
                }
                mud->line_length = 0;
                bzero(mud->line_buffer, 1024);

                mud->user_buffer[mud->user_length++] =  '\r';
                mud->user_buffer[mud->user_length] =    '\n';
                if (mud->user_length > 3 && memcmp(mud->user_buffer, "#$#", 3) == 0)
                    if(mud->mcp_state == 0)
                        mcp_first(mud);
                    else
                        mcp_parse(mud);
                else if (mud->user_length > 0) {
                    if (mud->ircserver->debug) {
                        printf("M> RAW (%i): \"%s\"\nM> HEX: ", mud->user_length, mud->user_buffer);
                        for (int i = 0; i < mud->user_length; i++)
                            printf("%X ", (unsigned char) mud->user_buffer[i]);
                        printf("\n--------\n");
                    }

                    printf("M> %s", mud->user_buffer);
                    send_line_irc(mud);
                }
                mud->user_length = 0;
                bzero(mud->user_buffer, 1024);
                break;
            case '\r':
                break;
        }
    }
    bzero(mud->read_buffer, 1024);
}

void free_mud(struct minfo *mud) {
    del_mud(mud);
    free(mud->name);
    free(mud->read_buffer);
    free(mud->line_buffer);
    free(mud->user_buffer);
    for (int i = 0; mud->irc_buffer[i] != 0; i++)
        free(mud->irc_buffer[i]);
    free(mud->irc_buffer);
    if (mud->use_ssl && mud->ssl != 0) {
        SSL_shutdown(mud->ssl);
        free(mud->ssl);
    }
    if (mud->socket != -2)
        shutdown(mud->socket, SHUT_RDWR);
    free(mud);
}

void add_mud(struct minfo* mud) {
    struct irc_mud** next = &(mud->ircserver->mud);
    while (*next != 0) {
        next = &((*next)->next);
    }
    *next = malloc(sizeof(struct irc_mud));
    (*next)->mud = mud;
    (*next)->next = 0;
}

struct minfo* get_mud(struct irc_server* server, char* channel) {
    struct irc_mud** next = &(server->mud);
    while (*next != 0) {
        if (strcmp((*next)->mud->name, channel + (channel[0] == '#' ? 1 : 0)) == 0)
            return (*next)->mud;
        next = &((*next)->next);
    }
    return 0;
}

void del_mud(struct minfo* mud) {
    struct irc_mud** curr = &(mud->ircserver->mud);
    while (*curr != 0) {
        if ((*curr)->mud == mud) {
            (*curr) = (*curr)->next;
            free(*curr);
            shutdown(mud->socket, SHUT_RD);
            break;
        }
        curr = &((*curr)->next);
    }
}