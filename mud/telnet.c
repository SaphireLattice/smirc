#include "telnet.h"
#include "stdlib.h"
#include "memory.h"
#include "../itoa.h"

const char* colors = "01,04,03,07,02,06,11,15,99,99,";

const char telnet_protocols[] = {
    COMPRESS2,
    ATCP,
    GMCP,
    TTYPE,
    MSDP,
    MSSP,
    MSP,
    0
};

char* get_protocols() {
    return (char *) telnet_protocols;
}

char* ttoa(unsigned int code) {
    char* result = calloc(sizeof(char), 5);
    memset(result, 0, 5);
    switch(code) {
        case 0xFF:
            memcpy(result, "IAC", 4);
            break;
        case WONT:
            memcpy(result, "WILL", 5);
            break;
        case WILL:
            memcpy(result, "WONT", 5);
            break;
        case DO:
            memcpy(result, "DO", 3);
            break;
        case DONT:
            memcpy(result, "DONT", 5);
            break;
        case SE:
            memcpy(result, "SE", 3);
            break;
        case SB:
            memcpy(result, "SB", 3);
            break;
        case GMCP:
            memcpy(result, "GMCP", 5);
            break;
        default: {
            int i = 0;
            result[i++] = '0';
            result[i++] = 'x';
            if (code < 0x10) {
                result[i++] = '0';
            }
            itoa(code, result + i, 16);
        }
    }
    return result;
}

char* ansi_to_irc_color(char* str) {
    int sl = (int) strlen(str);
    char* ret = calloc(sizeof(char), strlen(str) + 1);
    bzero(ret, strlen(str) + 1);
    int sp, rp;
    sp = 0;
    rp = 0;

    struct state* state;
    state = malloc(sizeof(state));
    state->fg = -1;
    state->bg = -1;

    int parsing = 0;
    char* code = calloc(sizeof(char), 3);
    int cp = 0;
    for (sp = 0; sp < sl; sp++ ) {
        if (parsing == 1) {
            if (str[sp] == ';' || str[sp] == 'm') {
                int c = atoi(code);
                cp = 0;
                bzero(code, 3);
                if (c == 40)
                    state->bg = -1;
                if (c >= 41 && c < 50)
                    state->bg = c - 40;
                if (c >= 30 && c < 40) {
                    if (c == 37)
                        state->fg = -1;
                    else
                        state->fg = c - 30;
                }
                if (c == 0) {
                    ret[rp++] = '\x0f';
                }
            } else {
                code[cp++] = str[sp];
            }
            if (str[sp] == 'm') {
                if (state->fg != -1 && state->bg != -1) {
                    ret[rp++] = '\x03';
                    if (state->fg >= 0) {
                        memcpy(ret + rp, colors + (state->fg * 3), 2);
                        rp += 2;
                        if (state->bg >= 0) {
                            ret[rp++] = ',';
                            memcpy(ret + rp, colors + (state->bg * 3), 2);
                            rp += 2;
                        }
                    } else if (state->bg >= 0) {
                        ret[rp++] = ',';
                        memcpy(ret + rp, colors + (state->bg * 3), 3);
                        rp += 2;
                    }
                }
                parsing = 0;
            }
        } else {
            if (str[sp] == '\x1b') {
                sp++;
                if (str[sp] == '[')
                    parsing = 1;
                else {
                    sp--;
                    ret[rp++] = str[sp];
                }
            } else
                ret[rp++] = str[sp];
        }
    }
    free(state);
    free(code);
    return ret;
}
