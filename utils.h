#ifndef SMIRC_ITOA_H
#define SMIRC_ITOA_H

char* pad_left(int needed, char pad, char* str);
char* itoa (int value, char *result, int base);
char* ltoa (long value, char *result, int base);
unsigned long urandom();

#endif //SMIRC_ITOA_H
