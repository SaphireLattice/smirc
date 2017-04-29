#include <stdio.h>
#include <stdlib.h>
#include "../config.h"

int main(int argc, char** args) {
    int* port = malloc(sizeof(int));
    *port = 8989;

    struct config* conf = config_new();
    config_load(conf, "test.conf");
    //config_value_add(conf, "irc.port", TYPE_INT, port);

    struct config_value *val = config_value_get(conf, "irc.port");
    if (val != 0 && config_value_type(val) == TYPE_INT)
        printf("%i\n", *(int*) val->data);
    else
        printf("No value!\n");

    config_save(conf, "test2.conf");

    free(port);


    return 0;
}