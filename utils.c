#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

char* pad_left(int needed, char pad, char* str) {
    size_t len = strlen(str);
    size_t max = len ;
    if (needed > max)
        max = (size_t) needed;
    char* ret = calloc(sizeof(char), max + 2);
    memset(ret, pad, max - len);
    memcpy(ret + max - len, str, len + 1);
    return ret;
}

int urandom_fd = -2;
// check that the base if valid
char* itoa (int value, char *result, int base) {
    if (base < 2 || base > 36) { *result = '\0'; return result; }

    char* ptr = result, *ptr1 = result, tmp_char;
    int tmp_value;

    do {
    tmp_value = value;
    value /= base;
    *ptr++ = "ZYXWVUTSRQPONMLKJIHGFEDCBA9876543210123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ" [35 + (tmp_value - value * base)];
    } while ( value );

    // Apply negative sign
    //if (tmp_value < 0) *ptr++ = '-';
    *ptr-- = '\0';
    while (ptr1 < ptr) {
    tmp_char = *ptr;
    *ptr--= *ptr1;
    *ptr1++ = tmp_char;
    }
    return result;
}

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