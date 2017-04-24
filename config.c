#include <stdlib.h>
#include <memory.h>
#include "config.h"

struct config* new_config(char* file) {
    struct config* conf = malloc(sizeof(struct config));
    memset(conf, 0, sizeof(struct config));
    return conf;
}