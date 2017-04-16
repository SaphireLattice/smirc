//
// Created by saphire on 16.04.17.
//

#include <stdlib.h>
#include <memory.h>
#include <ctype.h>
#include <stdio.h>
#include "mcp.h"

void mcp_first(struct minfo* mud) {
    mud->mcp_state = malloc(sizeof(struct mcp_state));
}

void mcp_parse(struct minfo* mud) {
    //printf("parse\n");
    struct mcp_msg* msg = mcp_decompose(mud->line_buffer);
    msg->state = mud->mcp_state;
    struct mcp_data* data;

    if ((data = mcp_has_key(msg, "version")) != 0)
        printf("Version: %s\n", data->value);
    mcp_free(msg);
}


char* mcp_compose(struct mcp_msg* msg) {
    return "";
}

struct mcp_msg* mcp_decompose(char* str) {
    //printf("decompose\n");
    struct mcp_msg* msg = mcp_new_msg();
    char* ptr = str + 3;
    printf("M> ");
    while(*ptr != 0) {
        printf("%c", *ptr);
        ptr++;
    }
    return msg;
}

struct mcp_msg* mcp_new_msg() {
    //printf("new_msg\n");
    struct mcp_msg* msg = malloc(sizeof(struct mcp_msg));
    memset(msg, 0, sizeof(struct mcp_msg));
    mcp_new_block(msg);
    return msg;
}

void mcp_new_block(struct mcp_msg* msg) {
    //printf("new_block\n");
    struct mcp_data_block* ptr = msg->data;
    while (ptr != 0) {
        ptr = ptr->next;
    }
    ptr = malloc(sizeof(struct mcp_data_block));
    ptr->data = calloc(sizeof(struct mcp_data), MCP_DATA_BLOCK_SIZE);
}

struct mcp_data* mcp_has_key(struct mcp_msg* msg, char* key_orig) {
    //printf("has_key\n");
    struct mcp_data* data = 0;
    char* key = mcp_low(key_orig);
    struct mcp_data_block* data_block = msg->data;
    while (data == 0 && data_block != 0) {
        struct mcp_data_block* next = data_block->next;
        for (int i = 0; i < MCP_DATA_BLOCK_SIZE; i++) {
            if (strcmp(data_block->data[i].key, key))
                data = &data_block->data[i];
        }
        data_block = next;
    }
    free(key);
    //printf("%p\n", (void*) data);
    return data;
}

void mcp_add_key(struct mcp_msg* msg, char* key_orig, char* value) {
    char* key = mcp_low(key_orig);
    if (mcp_has_key(msg, key) == 0) {

    }
    mcp_set_key(msg, key, value);
    free(key);
}

void mcp_set_key(struct mcp_msg* msg, char* key_orig, char* value) {
    char* key = mcp_low(key_orig);
    free(key);
}

void mcp_free(struct mcp_msg* msg) {
    //printf("free\n");
    struct mcp_data_block* data_block = msg->data;
    while (data_block != 0) {
        struct mcp_data_block* next = data_block->next;
        for (int i = 0; i < MCP_DATA_BLOCK_SIZE; i++) {
            free(data_block->data[i].value);
            free(data_block->data[i].key);
        }
        free(data_block->data);
        free(data_block);
        data_block = next;
    }
    free(msg);
}

char* mcp_low(char *orig) {
    size_t len = strlen(orig);
    char* ret = calloc(sizeof(char), len + 1);
    memcpy(ret, orig, len + 1);
    for (char* k = ret; *k; ++k) *k = (char) tolower(*k);
    return ret;
}