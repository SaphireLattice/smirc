#ifndef SMIRC_MUD_CLIENT_H
#define SMIRC_MUD_CLIENT_H

#include <openssl/ossl_typ.h>

struct minfo {
  int id;
  char* name;
  char* address;
  int port;
  int socket;
  int use_ssl;
  SSL* ssl;
  char* read_buffer;
  char* line_buffer;
  int line_length;
};

char** get_line_buffer();
void send_line_mud(char* line);

int mud_write(struct minfo* mud_l, char* buffer, int length);
int mud_read(struct minfo* mud_l);

void* mud_connect(void* arg);


#endif //SMIRC_MUD_CLIENT_H
