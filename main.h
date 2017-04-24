#ifndef SMIRC_MAIN_H
#define SMIRC_MAIN_H

#include "irc/client.h"
#include "mud/client.h"
#include "config.h"

struct smirc {
  struct minfo** servers;
  struct client** clients;
  struct config* config;
};

#endif //SMIRC_MAIN_H
