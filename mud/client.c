#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/ossl_typ.h>
#include <valgrind/memcheck.h>

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
    if (mud->use_ssl == 1) {
        status = SSL_write(mud->ssl, buffer, length);
    } else
        status = (int) write(mud->socket, buffer, (size_t) length);
    return status;
}

int mud_read(struct minfo* mud) {
    int status;
    if (mud->use_ssl == 1) {
        status = SSL_read(mud->ssl, mud->read_buffer, 1024);
#ifdef USING_VALGRIND
        if (status > 0) {
            VALGRIND_MAKE_MEM_DEFINED(mud->read_buffer, status);
        }
#endif
    }
    else
        status = (int) read(mud->socket, mud->read_buffer, 1024);
    return status;
}

void mud_init(struct minfo* mud) {
    printf("M> Starting thread\n");
    send_msg_irc(mud, "info", "Starting connection thread.");

    mud->socket = -2;
    mud->irc_buffer = calloc(sizeof(char *), MUD_IRC_BUFFER);
    mud->irc_length = -1;
    mud->line_length = 0;
    mud->user_length = 0;
    mud->read_buffer = calloc(sizeof(char), 1025);
    mud->line_buffer = calloc(sizeof(char), 1025);
    mud->user_buffer = calloc(sizeof(char), 1025);
    mud->ssl = 0;
    mud->mcp_state = 0;

    struct sockaddr_in serv_addr;

    if((mud->socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\nError: Could not create socket\n");
        send_msg_irc(mud, "error", "Error: Could not create socket");
        free_mud(mud);
        return;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons((uint16_t) mud->port);

    struct hostent *he;
    if ( (he = gethostbyname( mud->address ) ) == NULL) {
        printf("\nError: Invalid address (%s)\n", mud->address);
        send_msg_irc(mud, "error", "Error: Invalid address");
        free_mud(mud);
        return;
    }
    serv_addr.sin_addr = *((struct in_addr **) he->h_addr_list)[0];
    printf("%s\n",inet_ntoa(serv_addr.sin_addr));

    if (connect(mud->socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nError: Connect Failed\n");
        send_msg_irc(mud, "error", "Error: Conection failed");
        free_mud(mud);
        return;
    }

    if (mud->use_ssl == 1) {
        printf("Initializing SSL...\n");
        InitializeSSL();
        const SSL_METHOD *meth = SSLv23_client_method();
        SSL_CTX *ctx = SSL_CTX_new(meth);
        if ( ctx == NULL )
        {
            ERR_print_errors_fp(stderr);
            free_mud(mud);
            return;
        }
        SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, verify_callback);
        mud->ssl = SSL_new(ctx);

        SSL_set_fd(mud->ssl, mud->socket);

        printf("Connecting with SSL...");
        if (SSL_connect(mud->ssl) == -1) {
            printf(" FAIL\n");
            ERR_print_errors_fp(stderr);
            send_msg_irc(mud, "error", "SSL Connection failed");
            free_mud(mud);
            return;
        }
        printf("\nConnected with %s encryption\n", SSL_get_cipher(mud->ssl));
        send_msg_irc(mud, "info", "SSL Connection established");
        ShowCerts(mud->ssl);
    } else {
        printf("\nConnected\n");
        send_msg_irc(mud, "info", "Connection established");
    }

    socket_register(&mud->ircserver->socket, mud->socket, SOCKET_CLIENT_TELNET);

    char* buf = calloc(sizeof(char), 3);
    buf[0] = IAC;
    buf[1] = WILL;
    char* telnet_protocols = get_protocols();
    for (int p = 0; telnet_protocols[p] != 0; p++) {
        printf("Negotiating %X\n", (unsigned char) telnet_protocols[p]);
        buf[2] = telnet_protocols[p];
        mud_write(mud, buf , 3);
    }
    free(buf);
}

void mud_onsocket(struct minfo* mud) {
    ssize_t n = 0;

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
        send_msg_irc(mud, "info", "Connection closed.");
        printf("\nConnection closed\n");

        send_msg_irc(mud, "info", "Freeing and stopping.");
        free_mud(mud);
    }

    if(n < 0) {
        if (errno == EAGAIN)
            return; // All ok, we read everything and now we just wait.

        send_msg_irc(mud, "error", "Read error.");
        perror("Read error");

        send_msg_irc(mud, "info", "Freeing and stopping.");
        free_mud(mud);
    }
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
                memset(mud->user_buffer, 0, 1024);
                break;
            case '\r':
                break;
        }
    }
    memset(mud->read_buffer, 0, 1024);
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
    mcp_state_free(mud->mcp_state);
    socket_unregister(&mud->ircserver->socket, mud->socket);
    free(mud);
}

void add_mud(struct minfo* mud) {
    struct irc_mud** next = &(mud->ircserver->mud);
    while (*next != 0) {
        next = &((*next)->next);
    }
    *next = malloc(sizeof(struct irc_mud));
    (*next)->mud = mud;
    (*next)->next = NULL;
}

struct minfo* get_mud(struct irc_server *server, int fd) {
    struct irc_mud** next = &(server->mud);
    while (*next != NULL) {
        if ((*next)->mud->socket == fd)
            return (*next)->mud;
        next = &((*next)->next);
    }
    return NULL;
}

struct minfo* get_mud_by_name(struct irc_server *server, char *channel) {
    struct irc_mud** next = &(server->mud);
    while (*next != NULL) {
        if (strcmp((*next)->mud->name, channel + (channel[0] == '#' ? 1 : 0)) == 0)
            return (*next)->mud;
        next = &((*next)->next);
    }
    return NULL;
}

void del_mud(struct minfo* mud) { // A-B-C, deleting mud B
    struct irc_mud** curr = &(mud->ircserver->mud);
    while (*curr != 0) {
        printf("%p: %p\n", (void*) curr, (void*) *curr);
        if ((*curr)->mud == mud) {
            struct irc_mud* next = (*curr)->next; // Store &C
            // Otherwise we free the C instead of B
            free(*curr); // Free _B_
            (*curr) = next; // A->next = &C
            shutdown(mud->socket, SHUT_RD);
            break;
        }
        curr = &((*curr)->next);
    }
}
