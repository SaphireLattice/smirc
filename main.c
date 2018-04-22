#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include <errno.h>

#include "irc/irc_server.h"
#include "config.h"
#include "mud/client.h"

int main(int argc, char** args) {
    struct irc_server* server;

    struct config* config = config_new();
    config_load(config, "smirc.conf");
    server = malloc(sizeof(struct irc_server));
    memset(server, 0, sizeof(struct irc_server));
    server->name = "localhost";
    server->debug = 0;
    server->mud = 0;
    server->config = config;

    if (server_init(server) < 0) {
        return errno;
    };

    int pollerr = 0;
    while(1) {
        int pr = poll(server->socket.fds, server->socket.fd_num, 3*60*1000);
        if (pr == -1) {
            pollerr = errno;
            break;
        }

        for (int i = 0; i < server->socket.fd_num; i++) {
            if (server->quit != 0)
                break;
            struct pollfd pfd = server->socket.fds[i];
            if (pfd.fd != -1 && pfd.revents != 0) {
                int type = server->socket.fd_map[i].fd_type;
                switch(type) {
                    case SOCKET_SERVER_IRC:
                        server_onsocket(server);
                        break;
                    case SOCKET_CLIENT_IRC:
                        client_onsocket(get_client(server, pfd.fd));
                        break;
                    case SOCKET_CLIENT_TELNET:
                        mud_onsocket(get_mud(server, pfd.fd));
                        break;
                    default:
                        printf("Unknown socket: fd %d, type %d\n", pfd.fd, type);
                        socket_unregister(&server->socket, pfd.fd);
                }
            }
        }
        if (server->quit != 0)
            break;
    }

    config_free(config);
    server_free(server);

    if (pollerr != 0) {
        printf("Error: %s\n", strerror(pollerr));
        return pollerr;
    }
    printf("Main: return 0");
    return 0;
}
