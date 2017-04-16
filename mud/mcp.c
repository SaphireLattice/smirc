//
// Created by saphire on 16.04.17.
//

#include "mcp.h"


struct mcp_data {
  char* key;
  char* value;
};

struct mcp_data_block {
  struct mcp_data* data;
  struct mcp_data_block* next;
};

struct mcp_msg {
  struct mcp_data* data;
};

void mcp_parse(struct minfo* mud) {

}

struct mcp_msg* mcp_new_msg() {
    return 0;
}

int mcp_has_data() {
    return 0;
}

void mcp_add_data(char* key, char* value) {

}

char* mcp_compose(struct mcp_msg* msg) {
    return "";
}