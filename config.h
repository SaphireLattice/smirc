#ifndef SMIRC_CONFIG_H
#define SMIRC_CONFIG_H

#define CONFIG_BLOCK_SIZE 8

enum {
  TYPE_NULL     = 0,
  TYPE_INT      = 1,
  TYPE_LONG     = 2,
  TYPE_STRING   = 3,
  TYPE_SECTION  = 4,
};

struct config_value {
  int type;
  char* name;
  void* data;
  struct config_block* parent;
};

struct config_block;
struct config_block {
  struct config_value** data;
  struct config_block* next;
  struct config_value* parent;
};

struct config {
    struct config_block* data;
};

struct config* config_new();
void config_load(struct config* conf, char* filename);

struct config_value* config_value_add(struct config *config, char *path, int type, void *data);
struct config_value* config_value_set(struct config *config, char *path, int type, void *data);
struct config_value* config_value_get(struct config *config, char *path);
int config_value_type(struct config_value* value);

char** config_explode_path(char *path);
char* config_value_path(struct config_value* value);

struct config_value* config_section_add_value(struct config_block** start, char *name, int type, void *data);

struct config_value* config_section_get_value(struct config_block *block_ptr, char *singular);
struct config_value* config_block_get_value(struct config_block *block, char *name);

#endif //SMIRC_CONFIG_H
