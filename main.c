#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "mud/client.h"
#include "irc/server.h"

pthread_t tid[2];
struct irc_server* server;

void* doSomeThing(void *arg) {
    pthread_t id = pthread_self();

    if(pthread_equal(id,tid[0])) {
        server_loop(server);
    } else {
        /*struct minfo *mud = malloc(sizeof(struct minfo));
        memset(mud, 0, sizeof(struct minfo));
        mud->ircserver = server;
        printf("%s\n", server->name);
        mud->address = ((char**)arg)[1];
        mud->port = atoi(((char**)arg)[2]);
        mud->use_ssl = 0;//1;
        mud_connect(mud);*/
    }

    printf("Thread #%lu dead\n", id);
    return NULL;
}

int main(int argc, char** args) {
    int i = 0;
    int err;

    server = malloc(sizeof(struct irc_server));
    memset(server, 0, sizeof(struct irc_server));
    server->name = "localhost";
    printf("%s\n", server->name);
    printf("%p\n", (void*) server);
    server->clients = calloc(sizeof(struct client*), MAX_CLIENTS + 1);
    memset(server->clients, 0, sizeof(struct client*) * (MAX_CLIENTS + 1));

    while(i < 2) {
        err = pthread_create(&(tid[i]), NULL, &doSomeThing, args);

        if (err != 0)
            printf("Can't create thread :[%s]", strerror(err));
        else
            printf("Thread created successfully\n");

        i++;
    }

    for (;i > 0; i--)
        pthread_join(tid[i - 1], NULL);

    printf("Main: return 0");
    return 0;
}
