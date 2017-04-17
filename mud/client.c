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
# include "../irc/client.h"
# include "../irc/server.h"
#endif

struct minfo* mud;
char** irc_buffer;
int irc_buffer_lines;

char** get_line_buffer() {
    return irc_buffer;
}

#ifndef STANDALONE_MUD
void send_line_irc(char* line) {
    char* irc_line = ansi_to_irc_color(line);
    irc_buffer_lines++;
    if (irc_buffer_lines > 127) {
        free(irc_buffer[0]);
        memcpy(irc_buffer, irc_buffer + 1, sizeof(char *) * 126);
        bzero(irc_buffer + 125, sizeof(char* ) * 2);
        irc_buffer_lines--;
    }
    irc_buffer[irc_buffer_lines] = calloc(sizeof(char), strlen(irc_line) + 1);
    bzero(irc_buffer[irc_buffer_lines], strlen(irc_line) + 1);
    strcpy(irc_buffer[irc_buffer_lines], irc_line);
    struct client** clients = get_clients();
    if (clients != 0)
        for(int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] != 0) {
                printf("Sending to %i", i);
                char* cat = calloc(sizeof(char), strlen(irc_line) + 9);
                strcpy(cat, "#smirc :");
                strcat(cat, irc_line);
                server_send(clients[i], ">", "PRIVMSG", cat);
                free(cat);
            }
        }
    fputs("M>", stdout);
    fputs(irc_line, stdout);
    free(irc_line);
}
#endif

void send_line_mud(char* line) {
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

int mud_read(struct minfo* mud_l) {
    int status;
    if (mud_l->use_ssl == 1)
        status = SSL_read(mud_l->ssl, mud_l->read_buffer, sizeof(mud_l->read_buffer)-1);
    else
        status = (int) read(mud_l->socket, mud_l->read_buffer, sizeof(mud_l->read_buffer)-1);
    return status;
}

void* mud_connect(void* arg) {
    mud = (struct minfo*) arg;

    irc_buffer = calloc(sizeof(char *), 128);
    memset(irc_buffer, 0, 128);
    irc_buffer_lines = -1;
    ssize_t n = 0;
    mud->line_length = 0;
    mud->read_buffer = calloc(sizeof(char), 1024);
    mud->line_buffer = calloc(sizeof(char), 1024);
    memset(mud->read_buffer, 0, 1024);
    memset(mud->line_buffer, 0, 1024);

    struct sockaddr_in serv_addr;

    if((mud->socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\nError: Could not create socket \n");
        return 0;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons((uint16_t) mud->port);

    struct hostent *he;
    if ( (he = gethostbyname( mud->address ) ) == NULL)
        return 0;
    serv_addr.sin_addr = *((struct in_addr **) he->h_addr_list)[0];
    printf("%s\n",inet_ntoa(serv_addr.sin_addr));

    if( connect(mud->socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\n Error : Connect Failed \n");
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
            return 0;
        }
        SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, verify_callback);
        mud->ssl = SSL_new(ctx);

        SSL_set_fd(mud->ssl, mud->socket);

        printf("Connecting with SSL...");
        if (SSL_connect(mud->ssl) == -1) {
            printf(" FAIL\n");
            ERR_print_errors_fp(stderr);
            return 0;
        }
        printf("\nConnected with %s encryption\n", SSL_get_cipher(mud->ssl));
        ShowCerts(mud->ssl);
    }
    char* buf = calloc(sizeof(char), 3);
    buf[0] = IAC;
    buf[1] = WILL;
    char* telnet_protocols = get_protocols();
    for (int p = 0; telnet_protocols[p] != 0; p++) {
        printf("Negotiating %X\n", (unsigned char) telnet_protocols[p]);
        buf[2] = telnet_protocols[p];
        mud_write(mud, buf , 3);
    }


    printf("Entering read loop...\n");
    while ( (n = mud_read(mud)) > 0) {
        mud->read_buffer[n] = 0;

        for (int i = 0; mud->read_buffer[i] != 0; i++) {
            switch(mud->read_buffer[i]) {
                case IAC:
                    printf("T> IAC ");
                    char next = mud->read_buffer[++i];
                    char *cmd = ttoa((unsigned int) next);
                    printf("%s ", cmd);
                    free(cmd);
                    if (next == WILL || next == WONT || next == DO || next == DONT) {
                        char option = mud->read_buffer[++i];
                        char *opt = ttoa((unsigned int) option);
                        printf("%s", opt);
                        free(opt);
                    } else if (next == SB) {
                        char sub;
                        do {
                            sub = mud->read_buffer[++i];
                            printf("\nSub: %X (%i)", (unsigned char) sub, (unsigned char) sub);
                        } while (sub != IAC && mud->read_buffer[i + 1] != SE);
                    }
                    printf("\n");
                    break;
                case '\r':
                    break;
                case '\n':
                    mud->line_buffer[mud->line_length] = mud->read_buffer[i];
                    if (mud->line_length > 3 && memcmp(mud->line_buffer, "#$#", 3) == 0)
                        if(mud->mcp_state == 0)
                            mcp_first(mud);
                        else
                            mcp_parse(mud);
                    else
                        printf(" > %s", mud->line_buffer);
#ifndef STANDALONE_MUD
                        send_line_irc(mud->line_buffer);
#endif
                    mud->line_length = 0;
                    bzero(mud->line_buffer, 1024);
                    break;
                default:
                    mud->line_buffer[mud->line_length++] = mud->read_buffer[i];
            }
        }
        bzero(mud->read_buffer, 1024);
    }

    if(n == 0)
        printf("\nConnection closed\n");

    if(n < 0) {
        perror("Read error");
    }

    return 0;
}

