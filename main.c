#include "mud/client.h"
#include "irc/server.h"

pthread_t tid[2];

void* doSomeThing(void *arg) {
    pthread_t id = pthread_self();

    if(pthread_equal(id,tid[0])) {
        server_loop();
        printf("First thread processing\n");
        sleep(3);
    } else {
        struct minfo *mud = malloc(sizeof(struct minfo));
        mud->address = ((char**)arg)[1];
        mud->port = atoi(((char**)arg)[2]);
        mud->use_ssl = 1;
        mud_connect(mud);
        printf("Second thread processing\n");
    }

    printf("Thread #%lu dead\n", id);
    return NULL;
}

int main(int argc, char** args) {
    int i = 0;
    int err;

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
