#ifndef SMIRC_COMMANDS_H
#define SMIRC_COMMANDS_H

struct cmd_env {
  struct irc_client* cinfo;
  int argc;
  char** args;
  char* command;
};

struct cmd {
  char* name;
  void (*function)(struct cmd_env* env);
  struct cmd* next;
};

void commands_init(struct irc_server* server);
void commands_add(struct irc_server* server, char *name, void (*function)(struct cmd_env* env));
void commands_run(struct irc_client *cinfo, char *command, int words, char **word);

void command_connect(struct cmd_env* env);
void command_disconnect(struct cmd_env* env);
void command_quit(struct cmd_env* env);
void command_debug(struct cmd_env* env);
void command_list(struct cmd_env* env);
void command_save(struct cmd_env* env);
void command_load(struct cmd_env* env);

#endif //SMIRC_COMMANDS_H
