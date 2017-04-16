#ifndef SMIRC_MCP_H
#define SMIRC_MCP_H

#include "client.h"

#define MCP_DATA_BLOCK_SIZE 8

struct mcp_state {
  int state;
  char* key;
  char* data_tag;
};

struct mcp_data {
  char* key;
  char* value;
};

struct mcp_data_block {
  struct mcp_data* data;
  struct mcp_data_block* next;
};

struct mcp_msg {
  struct mcp_state* state;
  struct mcp_data_block* data;
  char* who;
  char* key;
};

void mcp_first(struct minfo* mud);
void mcp_parse(struct minfo* mud);

char* mcp_compose(struct mcp_msg* msg);
struct mcp_msg* mcp_decompose(char* str);

struct mcp_msg* mcp_new_msg();
void mcp_new_block(struct mcp_msg* msg);

struct mcp_data* mcp_has_key(struct mcp_msg* msg, char* key);
void mcp_add_key(struct mcp_msg* msg, char* key, char* value);
void mcp_set_key(struct mcp_msg* msg, char* key, char* value);

void mcp_free(struct mcp_msg* msg);
char* mcp_low(char *orig);

#endif //SMIRC_MCP_H
