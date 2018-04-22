#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include <errno.h>
#include "config.h"
#include "utils.h"

struct config* config_new() {
    struct config* conf = malloc(sizeof(struct config));
    return conf;
}

void config_load(struct config* conf, char* filename) {
    FILE* file = fopen(filename, "r");
    if(!file && errno == ENOENT)
        file = fopen(filename, "w+");
    if(!file) {
        perror("Config file opening failed");
        return;
    }

    char* buf = calloc(sizeof(char *), 1024);
    while (fgets(buf, 1024, file) != NULL) {
        if (strlen(buf) > 0) {
            int state = 0;
            char* ptr = buf - 1;
            char* conf_path = calloc(sizeof(char), 768);
            char* path_ptr = conf_path;
            char* conf_value = calloc(sizeof(char), 256);
            char* val_ptr = conf_value;
            int conf_type = TYPE_STRING;
            while (*(++ptr) != 0) {
                if (state == 0 && *ptr != ' ')
                    state = 1;

                if (state == 1 && *ptr == ' ')
                    state = 2;

                if ((state == 1 || state == 2) && *ptr == '=') {
                    state = 3;
                    ptr++;
                }
                if (state == 1)
                    *(path_ptr++) = *(ptr);

                if (state == 3 && *ptr != ' ')
                    state = 4;

                if (state == 5 && (*ptr == '\r' || *ptr == '\n'))
                    break;

                // It's a number!
                if (state == 4 && ('0' <= *ptr) && (*ptr <= '9')) {
                    conf_type = TYPE_INT;
                    state = 5;
                }
                if (state == 4 && *ptr == 'l') {
                    conf_type = TYPE_LONG;
                    state = 5;
                    ptr++;
                }
                if (state == 5 && (conf_type == TYPE_INT || conf_type == TYPE_LONG)) {
                    if (('0' <= *ptr) && (*ptr <= '9'))
                        *(val_ptr++) = *ptr;
                    else {
                        state = -1; // Non-integer character when reading integer type
                        break;      // No sense in parsing further if there's already error
                    }
                }

                // It's a string!
                if (state == 4 && *ptr == '"') {
                    state = 5;
                    ptr++;
                }
                if (state == 5
                    && conf_type == TYPE_STRING
                    && ((*(ptr + 1) == '\0')
                        || (*(ptr + 1) == '\r')
                        || (*(ptr + 1) == '\n')
                    )) {
                    if (*ptr != '"')
                        state = -2; // String is not terminated with double-quote
                    break;
                }
                if (state == 5 && conf_type == TYPE_STRING)
                    *(val_ptr++) = *ptr;

                if (state == 4 && *ptr != ' ') {
                    state = -3; // Unable to determine type
                    break;
                }
            }
            if (state == 5) {
                union config_data data;
                data.as_ptr = NULL;
                if (conf_type == TYPE_INT) {
                    data.as_int = malloc(sizeof(int));
                    *(data.as_int) = atoi(conf_value);
                }
                if (conf_type == TYPE_LONG) {
                    data.as_long = malloc(sizeof(long));
                    *(data.as_long) = atol(conf_value);
                }
                if (conf_type == TYPE_STRING) {
                    size_t len = strlen(conf_value);
                    data.as_string = calloc(sizeof(char), len + 1);
                    strcpy(data.as_string, conf_value);
                }
                config_value_set(conf, conf_path, conf_type, data);
            } else
                printf("C> Error %i parsing config entry: %s\n", -state, buf);
            free(conf_value);
            free(conf_path);
        }
    }

    fclose(file);

    free(buf);
}

int config_save(struct config* config, char* filename) {
    FILE* file = fopen(filename, "w");
    if (file == 0) {
        perror("Config file opening failed");
        return EOF;
    }
    int ret = config_save_section(config->data, file);
    fclose(file);
    return ret;
}

int config_save_section(struct config_block *block, FILE* file) {
    while (block != NULL) {
        char* section = config_value_path(block->parent);
        for (int i = 0; i < CONFIG_BLOCK_SIZE; i++) {
            struct config_value* val = block->data[i];
            if (val != NULL) {
                if (val->type != TYPE_SECTION) {
                    fputs(section, file);
                    if (block->parent != NULL)
                        fputc('.', file);
                    fputs(val->name, file);
                    fputs(" = ", file);

                    char* buf = calloc(sizeof(char), 32);
                    switch(val->type) {
                        case TYPE_INT:
                            fputs(itoa(*(val->data.as_int), buf, 10), file);
                            break;
                        case TYPE_LONG:
                            fputc('l', file);
                            fputs(ltoa(*(val->data.as_long), buf, 10), file);
                            break;
                        case TYPE_STRING:
                            fputc('"', file);
                            fputs(val->data.as_string, file);
                            fputc('"', file);
                        default:
                            break;
                    }
                    free(buf);
                    fputc('\n', file);
                    fflush(file);
                } else
                    config_save_section(val->data.as_section, file);
            }
        }
        free(section);
        block = block->next;
    }
    return 0;
}

void config_free(struct config *config) {
    config_section_free(config->data);
    free(config);
}

void config_section_data_free(struct config_block* block) {
    for (int i = 0; i < CONFIG_BLOCK_SIZE; i++) {
        if (block->data[i] != NULL) {
            if (block->data[i]->type != TYPE_SECTION) {
                free(block->data[i]->data.as_ptr);
            } else {
                config_section_free(block->data[i]->data.as_section);
            }
            free(block->data[i]->name);
            free(block->data[i]);
        }
    }
    free(block->data);
}

void config_section_free(struct config_block* block) {
    struct config_block* next = block;
    while (next != NULL) {
        struct config_block* freeing = next;
        next = freeing->next;
        config_section_data_free(freeing);
        free(freeing);
    }
}

struct config_value* config_value_add(struct config *config, char *path, int type, union config_data data) {
    struct config_value* ret = NULL;
    struct config_block** block = &(config->data);

    char **word = config_explode_path(path);

    int error = 0;
    for(int i = 0; (i < 32) && (word[i] != 0); i++) {
        if (error == 0) {
            struct config_value *val = config_section_get_value(*block, word[i]);
            if (val != NULL)
                if (val->type != TYPE_SECTION) {
                    printf("C> Attempt to add children value to a non-section value\n");
                    ret = NULL; // WE HAVE SOME PROBLEMS PLEASE STAND BY?!
                    error = 1;
                } else
                    block = (struct config_block **) &(val->data);
            else
                if (word[i + 1] == NULL)
                    ret = config_section_add_value(block, word[i], type, data);
                else {
                    struct config_value *v = config_section_add_value(block, word[i], TYPE_SECTION, (union config_data) {NULL});
                    block = &(v->data.as_section);
                }
        }
        free(word[i]);
    }
    free(word);

    return ret;
}

struct config_value* config_value_set(struct config *config, char* path, int type, union config_data data) {
    struct config_value* val = config_value_get(config, path);
    if (val != 0) {
        free(val->data.as_ptr);
        if (type != TYPE_NULL)
            val->type = type;
        val->data = data;
    } else
        val = config_value_add(config, path, type, data);
    if (val == 0) //Things went wrong.
        return 0;
    return val;
}

struct config_value* config_value_get(struct config *config, char *path) {
    struct config_value* ret = 0;
    struct config_block* block = config->data;

    char **word = config_explode_path(path);

    for(int i = 0; (i < 32) && (word[i] != 0); i++) {
        struct config_value* val = config_section_get_value(block, word[i]);
        if (val != NULL) {
            ret = val;
            if (val->type == TYPE_SECTION)
                block = val->data.as_section;
        } else
            ret = NULL;
        free(word[i]);
    }
    free(word);

    char *valpath = config_value_path(ret);
    if (strcmp(valpath, path) != 0)
        ret = NULL;
    free(valpath);

    return ret;
}

struct config_value* config_value_get_soft(struct config *config, char *path, int type, union config_data failsafe) {
    struct config_value* ret = config_value_get(config, path);
    if (ret == 0)
        ret = config_value_add(config, path, type, failsafe);
    else {
        if (ret->type != type) {
            config_value_set(config, path, type, failsafe);
        }
    }
    return ret;
}

int config_value_type(struct config_value* value) {
    return value->type;
}

char** config_explode_path(char *path) {
    char** word = calloc(sizeof(char *), 32);
    char* ptr = path;
    memset(word, 0, sizeof(char *) * 32);
    word[0] = calloc(sizeof(char), 32);
    memset(word[0], 0, sizeof(char) * 32);
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

    return word;
}

char* config_value_path(struct config_value* value) {
    char **path = calloc(sizeof(char *), 32);
    int i = 0;
    size_t l = 2;

    while (value != 0) {
        size_t len = strlen(value->name);
        path[i] = calloc(sizeof(char *), len + 2);
        memset(path[i], 0, len);
        memcpy(path[i], value->name, len);
        l += len + 1;
        i += 1;
        if (value->parent != 0)
            value = value->parent->parent;
        else
            value = 0;
    }

    char *ret = calloc(sizeof(char), l);
    memset(ret, 0, l);

    for (i--; i >= 0; i--) {
        strcat(ret, path[i]);
        if (i != 0)
            strcat(ret, ".");
        free(path[i]);
    }

    free(path);

    return ret;
}


struct config_value* config_section_add_value(struct config_block** start, char *name, int type, union config_data data) {
    struct config_value *ret = 0;
    struct config_block **next = start;

    while (ret == 0) {
        if (*next != 0) {
            for (int i = 0; i < CONFIG_BLOCK_SIZE; i++) {
                if ((*next)->data[i] == NULL) {
                    ret = malloc(sizeof(struct config_value));
                    ret->type = type;
                    ret->name = strdup(name);
                    ret->data = data;
                    ret->parent = (*next);

                    if (type == TYPE_SECTION && data.as_ptr == NULL) {
                        struct config_block* block = malloc(sizeof(struct config_block));
                        block->parent = ret;
                        block->next = NULL;
                        block->data = calloc(sizeof(struct config_value*), CONFIG_BLOCK_SIZE);
                        ret->data.as_section = block;
                    }

                    (*next)->data[i] = ret;
                    break;
                }
            }
            next = &((*next)->next);
        }

        if (ret == NULL && *next == NULL) {
            struct config_block* block = malloc(sizeof(struct config_block));
            if (*start != NULL)
                block->parent = (*start)->parent;
            else //Root of config
                block->parent = NULL;
            block->next = NULL;
            block->data = calloc(sizeof(struct config_value*), CONFIG_BLOCK_SIZE);
            *next = block;
        }
    };

    return ret;
}

struct config_value* config_section_get_value(struct config_block *block_ptr, char *singular) {
    struct config_block* curr = block_ptr;
    struct config_value* value = NULL;
    while (curr != NULL) {
        if ((value = config_block_get_value(curr, singular)) != NULL) {
            return value;
        }
        curr = curr->next;
    }
    return value;
}

struct config_value* config_block_get_value(struct config_block *block, char *name) {
    for (int i = 0; i < CONFIG_BLOCK_SIZE; i++) {
        if (block->data[i] != NULL && strcmp(block->data[i]->name, name) == 0)
            return block->data[i];
    }
    return 0;
}