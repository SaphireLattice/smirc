#ifndef SMIRC_MCP_H
#define SMIRC_MCP_H

#include "client.h"

#define MCP_DATA_BLOCK_SIZE 8

struct mcp_state {
  struct minfo* mud;
  int state;
  char* key;
  char* data_tag;
};

struct mcp_data {
  char* key;
  char* value;
};

struct mcp_data_block {
  struct mcp_data** data;
  struct mcp_data_block* next;
};

struct mcp_msg {
  struct mcp_state* state;
  struct mcp_data_block* data;
  struct mcp_data* last_data;
  char* msg_name;
  char* key;
};

void mcp_first(struct minfo* mud);
void mcp_parse(struct minfo* mud);

void mcp_send(struct mcp_msg* mcp_msg);
char* mcp_compose(struct mcp_msg* mcp_msg);
struct mcp_msg* mcp_decompose(char* str);

struct mcp_msg* mcp_new_msg();
struct mcp_data_block* mcp_new_block(struct mcp_msg* msg);

struct mcp_data* mcp_has_data(struct mcp_msg *msg, char *key);
void mcp_add_data(struct mcp_msg *msg, char *key, char *value);
void mcp_set_data(struct mcp_msg *msg, char *key, char *value);

void mcp_free(struct mcp_msg* msg);
char* mcp_low(char *orig);
void mcp_low_orig(char *orig);
int mcp_str_copy(char* dest, char* source);

void rand_str(char *dest, int length);

#endif //SMIRC_MCP_H
