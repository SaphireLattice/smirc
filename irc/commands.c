#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "irc_client.h"
#include "irc_server.h"
#include "../mud/client.h"
#include "../utils.h"
#include "commands.h"

void commands_output(struct cmd_env* env, char* msg) {
    size_t l = strlen(env->command) + strlen(msg);
    char* cat = calloc(sizeof(char), l + 2);
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
        struct minfo* mud = malloc(sizeof(struct minfo));
        mud->ircserver = env->cinfo->server;
        mud->name = strdup(env->args[0]);

        size_t l = strlen(env->args[0]) + 4;
        char* chan = calloc(sizeof(char), l);
        memset(chan, 0, sizeof(char) * l);
        strcpy(chan + 1, env->args[0]);
        chan[0] = '#';
        server_join_channel(env->cinfo, chan);
        free(chan);

        mud->address = strdup(env->args[2]);
        mud->use_ssl = 1;
        if (env->argc >= 3) {
            mud->port = atoi(strdup(env->args[4]));
            if (env->args[4][0] == '-') {
                mud->port = -mud->port;
                mud->use_ssl = 0;
            }
        } else
            mud->port = 23;
        add_mud(mud);
        pthread_create(&mud->thread, NULL, &mud_connect, mud);
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
}

void command_debug(struct cmd_env* env) {
    if (env->argc > 0 && strcasecmp(env->args[0], "off"))
        env->cinfo->server->debug = 0;
    else
        env->cinfo->server->debug = 1;
}

void command_list(struct cmd_env* env) {
    server_send_user_channel(env->cinfo, "smirc", "smirc", "Connected to:");
    struct irc_mud** curr = &(env->cinfo->server->mud);
    while (*curr != 0) {
        char* port = calloc(sizeof(char), 7);
        itoa((*curr)->mud->port, port, 0);
        size_t namel = strlen((*curr)->mud->name);
        size_t addressl = strlen((*curr)->mud->address);
        size_t len =  namel + addressl + strlen(port) + 6 + 2 + 2 + 5 + 1;
        char* cat = calloc(sizeof(char), len);
        memset(cat, 0, len);
        strcat(cat, ">");
        char* tmp = pad_left(10, ' ', (*curr)->mud->name);
        strcat(cat, tmp);
        free(tmp);
        tmp = pad_left(16, ' ', (*curr)->mud->name);
        strcat(cat, tmp);
        free(tmp);
        tmp = pad_left(6, ' ', port);
        strcat(cat, tmp);
        free(tmp);
        free(port);
        server_send_user_channel(env->cinfo, "smirc", "smirc", cat);
        free(cat);
        curr = &((*curr)->next);
    }
}

#include "commands.h"
