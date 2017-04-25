#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include "config.h"

struct config* config_new(char* file) {
    struct config* conf = malloc(sizeof(struct config));
    memset(conf, 0, sizeof(struct config));
    return conf;
}

struct config_value* add_config_value() {
    return 0;
}

struct config_value* get_config_value(struct config* config, char* name) {
    char** word = calloc(sizeof(char *), 32);
    char* ptr = name;
    memset(word, 0, sizeof(char *) * 32);
    word[0] = calloc(sizeof(char), 32);
    memset(word[0], 0, sizeof(char) * 32);
    word[1] = ptr;
    int words = 0;
    int w = 0;

    while (*ptr != '\0') {
        if (*ptr == '.') {
            words++;
            word[words] = calloc(sizeof(char), 32);
            memset(word[words], 0, sizeof(char) * 32);
            w = 0;
        } else
            word[words][w++] = *ptr;
        ptr++;
    }



    for(int i = 0; (i < 256) && (word[i] != 0); i+=2) {
        printf("%i: %s\n", i/2, word[i]);
        free(word[i]);
    }
    free(word);

    return 0;
}

struct config_value* config_get_in_section(struct config_block** block_ptr, char* singular) {
    struct config_block** curr = block_ptr;
    struct config_value* value = 0;
    while (*curr != 0) {
        if ((value = config_get_in_block(*curr, singular)) != 0) {
        }
        curr = &((*curr)->next);
    }
    return value;
}

struct config_value* config_get_in_block(struct config_block* block, char* name) {
    for (int i = 0; i < CONFIG_BLOCK_SIZE; i++) {
        if (block->data[i] != 0 && strcmp(block->data[i]->name, name) == 0)
            return block->data[i];
    }
    return 0;
}