#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "irc_client.h"
#include "irc_server.h"
#include "../mud/client.h"
#include "../utils.h"
#include "commands.h"
#include "../config.h"

void commands_output(struct cmd_env* env, char* msg) {
    size_t l = strlen(env->command) + strlen(msg);
    char* cat = calloc(sizeof(char), l + 3);
    memset(cat, 0, l +2);
    strcat(cat, env->command);
    strcat(cat, ": ");
    strcat(cat, msg);
    server_send_user_channel(env->cinfo, "smirc", "smirc", cat);
    free(cat);
}

void commands_init(struct irc_server* server) {
    commands_add(server, "connect", &command_connect);
    commands_add(server, "disconnect", &command_disconnect);
    commands_add(server, "quit", &command_quit);
    commands_add(server, "debug", &command_debug);
    commands_add(server, "list", &command_list);
    commands_add(server, "save", &command_save);
    commands_add(server, "load", &command_load);
}

void commands_add(struct irc_server* server, char *name, void (*function)(struct cmd_env* env)) {
    struct cmd** next = &(server->commands);
    while (*next != 0)
        next = &((*next)->next);
    *next = malloc(sizeof(struct cmd));
    (*next)->next = 0;
    (*next)->name = name;
    (*next)->function = function;
}

void commands_run(struct irc_client *cinfo, char *command, int words, char **word) {
    struct cmd_env* env = malloc(sizeof(struct cmd_env));
    env->cinfo = cinfo;
    env->command = command;
    env->argc = words;
    env->args = word;

    void (*cmd)(struct cmd_env*) = NULL;

    struct cmd* next = cinfo->server->commands;
    while (next != 0) {
        if (strcasecmp(next->name,command) == 0)
            cmd = next->function;
        next = next->next;
    }

    if (cmd != NULL)
        cmd(env);
    else {
        size_t l = strlen(env->command) + 17;
        char* cat = calloc(sizeof(char), l + 2);
        memset(cat, 0, l +2);
        strcat(cat, "No such command: ");
        strcat(cat, env->command);
        server_send_user_channel(env->cinfo, "smirc", "smirc", cat);
        free(cat);
    }

    free(env);
}

void command_connect(struct cmd_env* env) {
    if (env->argc < 1)
        commands_output(env, "Invalid connect command!");
    else {
        if (get_mud(env->cinfo->server, env->args[0]) != 0) {
            commands_output(env, "Error, already connected.");
            return;
        }

        size_t namel = strlen(env->args[0]);
        if (namel == 0) {
            commands_output(env, "Error, name is empty! Somehow. Please report to developer RIGHT NOW!");
            return;
        }

        if (namel >= 32) {
            commands_output(env, "Error, name is too long!");
            return;
        }

        if (strchr(env->args[0], '.') != 0) {
            commands_output(env, "Error, invalid name. Name must NOT cointain a dot.");
            return;
        }
        
        size_t l = namel + 4;
        char* path = calloc(sizeof(char), l + 36);
        memset(path, 0, sizeof(char) * (l + 36));
        memcpy(path, "mud.", 4);
        strcat(path, env->args[0]);
        char* start = path + 5 + namel;


        struct minfo* mud = malloc(sizeof(struct minfo));
        mud->ircserver = env->cinfo->server;
        mud->name = strdup(env->args[0]);

        if (env->argc == 1) {
            struct config_value* mud_conf = config_value_get(mud->ircserver->config, path);
            if (mud_conf == 0) {
                commands_output(env, "Error, no such server stored in config.");
                free(mud);
                free(path);
                return;
            }
            strcat(path, ".");
            strcpy(start, "address");
            mud->address = (char*) config_value_get(mud->ircserver->config, path)->data;
            strcpy(start, "port");
            mud->port = *((int*) config_value_get(mud->ircserver->config, path)->data);
            strcpy(start, "ssl");
            mud->use_ssl = *((int*) config_value_get(mud->ircserver->config, path)->data);
        } else {
            mud->address = strdup(env->args[2]);
            strcat(path, ".");
            strcpy(start, "address");
            config_value_set(mud->ircserver->config, path, TYPE_STRING, strdup(mud->address));

            mud->use_ssl = 0;
            if (env->argc >= 3) {
                mud->port = atoi(strdup(env->args[4]));
                if (env->args[4][0] == '+')
                    mud->use_ssl = 1;
            } else
                mud->port = 23;

            strcpy(start, "port");
            int *port = malloc(sizeof(int));
            *port = mud->port;
            config_value_set(mud->ircserver->config, path, TYPE_INT, port);
            strcpy(start, "ssl");
            int *ssl = malloc(sizeof(int));
            *ssl = mud->use_ssl;
            config_value_set(mud->ircserver->config, path, TYPE_INT, ssl);
        }

        char* chan = calloc(sizeof(char), l);
        memset(chan, 0, sizeof(char) * l);
        strcpy(chan + 1, env->args[0]);
        chan[0] = '#';
        server_join_channel(env->cinfo, chan);
        free(chan);

        add_mud(mud);
        pthread_create(&mud->thread, NULL, &mud_connect, mud);
        free(path);
    }
}

void command_disconnect(struct cmd_env* env) {
    if (env->argc < 1)
        commands_output(env, "Invalid connect command!");
    else {
        struct minfo *mud = get_mud(env->cinfo->server, env->args[0]);
        if (mud != 0) {
            shutdown(mud->socket, SHUT_RD);
            del_mud(mud);
        } else
            commands_output(env, "No such active MUD connection!");
    }
}

void command_quit(struct cmd_env* env) {
    shutdown(env->cinfo->server->socket.socket_description, SHUT_RDWR);
    //TODO: Redo irc_server.c loop to use select to allow for immediate shutdown instead of "on client connect"
}

void command_debug(struct cmd_env* env) {
    if (env->argc > 0 && strcasecmp(env->args[0], "off"))
        env->cinfo->server->debug = 0;
    else
        env->cinfo->server->debug = 1;
}

void command_list(struct cmd_env* env) {
    server_send_user_channel(env->cinfo, "smirc", "smirc", "====== Server list ======");
    server_send_user_channel(env->cinfo, "smirc", "smirc", "Connected:");
    struct irc_mud* curr = env->cinfo->server->mud;
    while (curr != 0) {
        char* port = calloc(sizeof(char), 7);
        itoa(curr->mud->port, port, 10);
        size_t namel = strlen(curr->mud->name);
        size_t addressl = strlen(curr->mud->address);
        size_t len =  namel + addressl + strlen(port) + 6 + 2 + 2 + 5 + 1;
        char* cat = calloc(sizeof(char), len);
        memset(cat, 0, len);
        strcat(cat, ">");
        char* tmp = pad_left(15, ' ', curr->mud->name);
        strcat(cat, tmp);
        free(tmp);
        tmp = pad_left(24, ' ', curr->mud->address);
        strcat(cat, tmp);
        free(tmp);
        tmp = pad_left(6, ' ', port);
        strcat(cat, tmp);
        strcat(cat, curr->mud->use_ssl ? "" : " ssl");
        free(tmp);
        free(port);
        server_send_user_channel(env->cinfo, "smirc", "smirc", cat);
        free(cat);
        curr = curr->next;
    }
    server_send_user_channel(env->cinfo, "smirc", "smirc", "Not connected: ");
    struct config_value* servers = config_value_get(env->cinfo->server->config, "mud");
    if (servers != 0) {
        struct config_block* block = (struct config_block*) servers->data;
        while (block != 0) {
            for (int i = 0; i < CONFIG_BLOCK_SIZE; i++) {
                if (block->data[i] != 0 && get_mud(env->cinfo->server, block->data[i]->name) == 0) {
                    char* port = calloc(sizeof(char), 7);
                    itoa(*((int *) config_section_get_value(block->data[i]->data, "port")->data), port, 10);
                    char* name = block->data[i]->name;
                    char* address = (char *) config_section_get_value(block->data[i]->data, "address")->data;

                    size_t namel = strlen(name);
                    size_t addressl = strlen(address);
                    size_t len =  namel + addressl + strlen(port) + 6 + 2 + 2 + 5 + 1;
                    char* cat = calloc(sizeof(char), len);
                    memset(cat, 0, len);
                    strcat(cat, " ");
                    char* tmp = pad_left(15, ' ', name);
                    strcat(cat, tmp);
                    free(tmp);
                    tmp = pad_left(24, ' ', address);
                    strcat(cat, tmp);
                    free(tmp);
                    tmp = pad_left(6, ' ', port);
                    strcat(cat, tmp);
                    strcat(cat, *((int *) config_section_get_value(block->data[i]->data, "ssl")->data) ? "" : " ssl");
                    free(tmp);
                    free(port);
                    server_send_user_channel(env->cinfo, "smirc", "smirc", cat);
                    free(cat);
                }
            };
            block = block->next;
        }
    }
    server_send_user_channel(env->cinfo, "smirc", "smirc", "=========================");
}

void command_save(struct cmd_env* env) {
    if (env->argc == 0)
        config_save(env->cinfo->server->config, "smirc.conf");
    else
        config_save(env->cinfo->server->config, env->args[0]);
    commands_output(env, "Saved!");
}

void command_load(struct cmd_env* env) {
    if (env->argc == 0)
        config_load(env->cinfo->server->config, "smirc.conf");
    else
        config_load(env->cinfo->server->config, env->args[0]);
    commands_output(env, "Loaded!");
}
