#ifndef SMIRC_CONFIG_H
#define SMIRC_CONFIG_H

struct config_value {
  int type;
  char* name;
  void* data;
};
struct config_block;
struct config_block {
  struct config_value* data;
  struct config_block* next;
};

struct config {
    struct config_block* data;
};

#endif //SMIRC_CONFIG_H
