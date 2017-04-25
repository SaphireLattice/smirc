#ifndef SMIRC_CONFIG_H
#define SMIRC_CONFIG_H

#define CONFIG_BLOCK_SIZE 8

enum {
  TYPE_INT      = 1,
  TYPE_LONG     = 2,
  TYPE_STRING   = 3,
  TYPE_SECTION  = 4,
};

struct config_value {
  int type;
  char* name;
  void* data;
};

struct config_block;
struct config_block {
  struct config_value** data;
  struct config_block* next;
};

struct config {
    struct config_block* section;
};

struct config_value* config_get_in_section(struct config_block** block_ptr, char* singular);
struct config_value* config_get_in_block(struct config_block* block, char* name);

#endif //SMIRC_CONFIG_H
