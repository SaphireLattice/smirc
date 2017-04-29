#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include "../mud/telnet.h"

int main(int argc, char** args) {
    char* str = "\x1b[32;46mABC\x1b[35mABC";
    printf("%s\x1b[0m\n", str);
    str = ansi_to_irc_color(str);
    printf("\n%s\n", str);
    free(str);
    return 0;
}
