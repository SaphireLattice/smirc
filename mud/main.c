#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <libnet.h>
#include "client.h"
#include "../itoa.h"

int main(int argc, char** args) {
    int err;
    pthread_t thread;

    if(argc < 2) {
        printf("Not enough arguments!\n");
        return 1;
    }

    struct minfo *mud = malloc(sizeof(struct minfo));
    mud->address = args[1];
    mud->port = atoi(args[2]);
    mud->use_ssl = 0;

    printf("%s, %i\n", mud->address, mud->port);
    err = pthread_create(&thread, NULL, &mud_connect, mud);

    if (err != 0)
        printf("Can't create thread :[%s]", strerror(err));
    else {
        printf("Thread created successfully\n");
        pthread_join(thread, NULL);
    }

    free(mud);
    return 0;
}