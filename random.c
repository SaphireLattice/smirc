#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h> /* exit */
#include <stdio.h> /* printf */

int urandom_fd = -2;

void urandom_init(){
    urandom_fd = open("/dev/urandom", O_RDONLY);

    if (urandom_fd == -1)
    {
        int errsv = urandom_fd;
        printf("Error opening [/dev/urandom]: %i\n", errsv);
        exit(1);
    }
}

unsigned long urandom()
{
    unsigned long buf_impl;
    unsigned long *buf = &buf_impl;

    if (urandom_fd == -2){
        urandom_init();
    }

    /* Read 4 bytes, or 32 bits into *buf, which points to buf_impl */
    read(urandom_fd, buf, sizeof(long));
    return buf_impl;
}
