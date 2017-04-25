//
// Created by saphire on 16.04.17.
//

#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include <ctype.h>
#include "mcp.h"
#include "../random.h"

void mcp_first(struct minfo* mud) {
    srand((unsigned int) urandom());
    printf("P> Starting MCP negotiation\n");
    mud->mcp_state = malloc(sizeof(struct mcp_state));

    char* key;
    key = calloc(sizeof(char), 16);
    key[15] = 0;
    rand_str(key, 15);

    mud->mcp_state->mud = mud;
    mud->mcp_state->key = 0;

    struct mcp_msg* msg = mcp_decompose(mud->user_buffer);

    struct mcp_data* data;
    if ((data = mcp_has_data(msg, "version")) != 0) {
        printf("P> Version: %s\n", data->value);
        struct mcp_msg* reply = mcp_new_msg();
        reply->state = mud->mcp_state;
        reply->msg_name = strdup("mcp");
        //authentication-key: 18972163558 version: 1.0 to: 2.1
        mcp_add_data(reply, "authentication-key", key);
        mcp_add_data(reply, "version", "2.1");
        mcp_add_data(reply, "to", "2.1");
        //mud_write(mud, "#$#mcp authentication-key: \"QoMNUckLhytCmvX\" version: \"2.1\" to: \"2.1\"\r\n", 71);
        mcp_send(reply);
        mcp_free(reply);
    } else
        printf("P> Invalid negotiation message\n");

    mud->mcp_state->key = key;

    mcp_free(msg);
}

void mcp_parse(struct minfo* mud) {
    //printf("parse\n");
    struct mcp_msg* msg = mcp_decompose(mud->user_buffer);
    msg->state = mud->mcp_state;
    mcp_free(msg);
}

void mcp_send(struct mcp_msg* mcp_msg) {
    char* msg = mcp_compose(mcp_msg);
    mud_write(mcp_msg->state->mud, msg, (int) strlen(msg));
    free(msg);
}

char* mcp_compose(struct mcp_msg* mcp_msg) {
    char* msg = calloc(sizeof(char), 512);
    char* mptr = msg;

    mptr += mcp_str_copy(mptr, "#$#");

    mptr += mcp_str_copy(mptr, mcp_msg->msg_name);

    if (mcp_msg->state->key != 0) {
        *(mptr++) = ' ';
        mptr += mcp_str_copy(mptr, mcp_msg->key);
    }

    struct mcp_data_block* data_block = mcp_msg->data;
    while (data_block != 0) {
        struct mcp_data_block* next = data_block->next;
        for (int i = 0; (data_block->data[i] != 0) && i < MCP_DATA_BLOCK_SIZE; i++) {
            *(mptr++) = ' ';
            mptr += mcp_str_copy(mptr, data_block->data[i]->key);
            *(mptr++) = ':';
            *(mptr++) = ' ';
            *(mptr++) = '"';
            mptr += mcp_str_copy(mptr, data_block->data[i]->value);
            *(mptr++) = '"';
        }
        data_block = next;
    }
    *(mptr++) = '\r';
    *(mptr++) = '\n';
    *(mptr++) = '\0';
    printf("P< %s", msg);
    return msg;
}

// <message-start> ::= <message-name> <space> <auth-key> <keyvals>
// <message-multiline-continue> ::= '*' <space> <datatag> <space> <simple-key> ':' ' ' <line>
// <message-multiline-end> ::= ':' <space> <datatag>
struct mcp_msg* mcp_decompose(char* str) {
    //printf("decompose\n");
    struct mcp_msg* msg = mcp_new_msg();
    char* ptr = str + 3;

    // Binary: escape quote initial_msg need_name
    enum {
      SKIP      = 256,  //9
      COLON     = 128,  //8
      KEY       = 64,   //7
      VALUE     = 32,   //6
      ESCAPE    = 16,   //5
      QUOTE     = 8,    //4
      INITIAL   = 4,    //3
      AUTH      = 2,    //2
      NEEDNAME  = 1,    //1
    };

    int msg_state = NEEDNAME;

    size_t len = strlen(str);
    char* word = calloc(sizeof(char), len);
    char* key = calloc(sizeof(char), len);
    char* wptr = word;
    msg->msg_name = calloc(sizeof(char), 32);
    memset(word, 0, len);
    memset(key, 0, len);
    memset(msg->msg_name, 0, 32);

    fputs("P>", stdout);
    char* start = ptr;
    while(*ptr != 0) {
        switch (*ptr) {
            case '"':
                if (msg_state & ESCAPE) {
                    *(wptr++) = '"';
                } else
                    msg_state ^= QUOTE;
                ptr++;
                break;
            case '\n':
            case ' ':
                if (msg_state & QUOTE) {
                    *(wptr++) = '"';
                    ptr++;
                    break;
                }

                // Message name
                if (((msg_state & SKIP) == 0) && msg_state == NEEDNAME) {
                    fputs("\x1b[33m", stdout);
                    msg->msg_name = calloc(sizeof(char), ptr - start + 1);
                    memset(msg->msg_name, 0, ptr - start);
                    memcpy(msg->msg_name, word, ptr - start);
                    mcp_low_orig(msg->msg_name);

                    msg_state = AUTH;
                    if (strcmp(msg->msg_name, "mcp") == 0) {
                        msg_state = INITIAL;
                        msg_state|= KEY;
                    }
                    msg_state |= SKIP;
                }

                // Auth key.
                if (((msg_state & SKIP) == 0) && (msg_state == AUTH)) {
                    fputs("\x1b[31m", stdout);
                    char* auth_key = calloc(sizeof(char), 32);
                    memset(auth_key, 0, 32);
                    memcpy(auth_key, word, ptr - start);
                    msg->key = auth_key;
                    msg_state |= KEY;
                    msg_state |= SKIP;
                }

                if (((msg_state & SKIP) == 0) && (msg_state & KEY)) {
                    fputs("\x1b[34m", stdout);
                    // Shouldn't happen, but won't hurt to check, also can tell about other parsing errors
                    if ((msg_state & COLON) == 0) {
                        fputs("\n\x1b[0mP>\x1b[0;1;31m!MCP Error: no colon after key\x1b[0m\n", stdout);
                        return msg;
                    }
                    memset(key, 0, len);
                    strcpy(key, word);
                    msg_state ^= COLON;
                    msg_state ^= KEY;
                    msg_state |= VALUE;
                    msg_state |= SKIP;
                }

                if (((msg_state & SKIP) == 0) && (msg_state & VALUE)) {
                    fputs(":\x1b[32m", stdout);

                    mcp_add_data(msg, key, word);

                    msg_state ^= VALUE;
                    msg_state |= KEY;
                    msg_state |= SKIP;
                }

                // Skipping to next non-space
                {
                    ptr++;
                    while(*ptr == ' ') {
                        ptr++;
                        if (*ptr == 0 || *ptr == '\n') { // Just in case..
                            free(word);
                            return msg;
                        }
                    }
                    start = ptr;
                }

                if (msg_state & SKIP)
                    msg_state ^= SKIP;
                fputs(" ", stdout);
                fputs(word, stdout);
                memset(word, 0, len);
                wptr = word;
                break;
            case ':':
                msg_state |= COLON;
                ptr++;
                break;
            case '\\':
                if (msg_state & ESCAPE) {
                    *(wptr++) = '\\';
                }
                msg_state ^= ESCAPE;
                ptr++;
                break;
            default:
                *(wptr++) = *ptr;
                ptr++;
        }
    }
    fputs("\x1b[0m\n", stdout);
    free(word);
    free(key);
    return msg;
}

struct mcp_msg* mcp_new_msg() {
    struct mcp_msg* msg = malloc(sizeof(struct mcp_msg));
    memset(msg, 0, sizeof(struct mcp_msg));
    return msg;
}

struct mcp_data_block* mcp_new_block(struct mcp_msg* msg) {
    struct mcp_data_block* ptr = msg->data;
    while (ptr != 0) {
        ptr = ptr->next;
    }
    ptr = malloc(sizeof(struct mcp_data_block));
    ptr->next = 0;
    ptr->data = calloc(sizeof(struct mcp_data), MCP_DATA_BLOCK_SIZE);
    memset(ptr->data, 0, sizeof(struct mcp_data) * MCP_DATA_BLOCK_SIZE);
    return ptr;
}

struct mcp_data* mcp_has_data(struct mcp_msg *msg, char *key_orig) {
    struct mcp_data* data = 0;
    char* key = mcp_low(key_orig);
    struct mcp_data_block* data_block = msg->data;
    while (data == 0 && data_block != 0) {
        struct mcp_data_block* next = data_block->next;
        for (int i = 0; (data_block->data[i] != 0) && i < MCP_DATA_BLOCK_SIZE; i++) {
            if (strcmp(data_block->data[i]->key, key) == 0)
                data = data_block->data[i];
        }
        data_block = next;
    }
    free(key);
    //printf("%p\n", (void*) data);
    return data;
}

void mcp_add_data(struct mcp_msg *msg, char *key_orig, char *value) {
    //printf("P> Adding data\n");
    char* key = mcp_low(key_orig);
    int lfree = 0;
    if (mcp_has_data(msg, key) == 0) {
        //printf("P> Entry not found\n");
        struct mcp_data_block** data_block = &msg->data;
        struct mcp_data* data = 0;
        while (*data_block != 0) {
            //printf("P> Looping data blocks (%p:%p)\n", (void*)data_block, (void*)*data_block);
            struct mcp_data_block* next = (*data_block)->next;
            int i = 0;
            for (; ((*data_block)->data[i] != 0) && i < MCP_DATA_BLOCK_SIZE; i++);
            if (i != MCP_DATA_BLOCK_SIZE) {
                (*data_block)->data[i] = malloc(sizeof(struct mcp_data));
                data = (*data_block)->data[i];
                break;
            }
            (*data_block) = next;
        }
        if ((*data_block) == 0) {
            //printf("P> Adding new block\n");
            (*data_block) = mcp_new_block(msg);
            (*data_block)->data[0] = malloc(sizeof(struct mcp_data));
            data = (*data_block)->data[0];
            //printf("P> data=block[0]\n");
        }
        data->key = key;
        data->value = 0;
        msg->last_data = data;
    } else lfree = 1;
    //printf("P> Setting data\n");
    mcp_set_data(msg, key, value);
    //printf("P> add_data: free(key)\n");
    if (lfree == 1) free(key);
}

void mcp_set_data(struct mcp_msg *msg, char *key_orig, char *value_orig) {
    char* key = mcp_low(key_orig);
    char* value = strdup(value_orig);
    struct mcp_data* data = 0;
    if (msg->last_data && strcmp(msg->last_data->key, key) == 0)
        data = msg->last_data;
    else {
        struct mcp_data_block* data_block = msg->data;
        while (data_block != 0) {
            //printf("P> Looping data blocks (%p)\n", (void*)data_block);
            struct mcp_data_block* next = data_block->next;
            int i = 0;
            int exit = 0;
            for (; (data_block->data[i] != 0) && i < MCP_DATA_BLOCK_SIZE; i++) {
                if (strcmp(data_block->data[i]->key, key) == 0) {
                    //printf("P> Key of data %i: %s\n", i, data_block->data[i]->key);
                    data = data_block->data[i];
                    exit = 1;
                    break;
                }
            }
            if (exit)
                break;
            data_block = next;
        }
        if (data_block == 0) {
            //printf("P> SET_KEY ERROR: reached end of data storage\n");
            return;
        }
    }
    msg->last_data = data;
    if (data == 0) {
        //printf("P> SET_DATA ERROR: data is NULL\n");
        return;
    } else {
        if (data->value != 0) {
            //printf("P> add_data: free(value)\n");
            free(data->value);
        }
        data->value = value;
        //printf("P> Data key: %s\n", data->key);
        //printf("P> Data value: %s\n", data->value);
    }
    //printf("P> set_data: free(key)\n");
    free(key);
}

void mcp_free(struct mcp_msg* msg) {
    struct mcp_data_block* data_block = msg->data;
    while (data_block != 0) {
        struct mcp_data_block* next = data_block->next;
        for (int i = 0; (data_block->data[i] != 0) && i < MCP_DATA_BLOCK_SIZE; i++) {
            //printf("P> Freeing %i %s\n", i, data_block->data[i]->key);
            free(data_block->data[i]->value);
            free(data_block->data[i]->key);
            free(data_block->data[i]);
        }
        free(data_block->data);
        free(data_block);
        data_block = next;
    }
    free(msg->key);
    free(msg->msg_name);
    free(msg);
}

char* mcp_low(char *orig) {
    size_t len = strlen(orig);
    char* ret = calloc(sizeof(char), len + 1);
    for (int k = 0; *(orig + k); ++k) {
        *(ret + k) = (char) tolower(*(orig + k));
    }
    ret[len] = 0;
    return ret;
}

void mcp_low_orig(char *orig) {
    for (char* k = orig; *k; ++k) *k = (char) tolower(*k);
}

int mcp_str_copy(char* dest, char* source) {
    char* ptr = source;
    for (; *ptr != 0; ptr++) {
        if (*ptr == '\\' || *ptr == '"' )
            *(dest++) = '\\';
        *(dest++) = *ptr;
    }
    return (int) (ptr - source);
}

void rand_str(char *dest, int length) {
    char charset[] = "0123456789"
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    while (length--) {
        size_t index = (size_t) (((double) rand()) / RAND_MAX * (sizeof charset - 1));
        *dest++ = charset[index];
    }
    *dest = '\0';
}