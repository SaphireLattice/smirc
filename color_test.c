#include <memory.h>
#include <stdlib.h>
#include <stdio.h>

char* colors = "01,04,03,07,02,06,11,15,99,99,";

struct state {
  int fg;
  int bg;
};

char* ansi_to_irc_color(char* str) {
    int sl = (int) strlen(str);
    char* ret = calloc(sizeof(char), strlen(str));
    bzero(ret, strlen(str));
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
            printf("%c", str[sp]);
            if (str[sp] == ';' || str[sp] == 'm') {
                int c = atoi(code);
                cp = 0;
                bzero(code, 3);
                printf("_%i_", c);
                if (c >= 40 && c < 50)
                    state->bg = c - 40;
                if (c >= 30 && c < 40)
                    state->fg = c - 30;
                if (c == 0) {
                    ret[rp++] = '\x0f';
                }
            } else {
                code[cp++] = str[sp];
            }
            if (str[sp] == 'm') {
                if (state->fg != -1 && state->bg != -1) {
                    printf("Colors: ");
                    ret[rp++] = '\x03';
                    if (state->fg >= 0) {
                        printf("%i", state->fg);
                        memcpy(ret + rp, colors + (state->fg * 3), 3);
                        rp += 2;
                        if (state->bg >= 0) {
                            printf(",%i", state->bg);
                            ret[rp++] = ',';
                            memcpy(ret + rp, colors + (state->bg * 3), 3);
                            rp += 2;
                        }
                    } else if (state->bg >= 0) {
                        printf("__,%i", state->bg);
                        ret[rp++] = ',';
                        memcpy(ret + rp, colors + (state->bg * 3), 3);
                        rp += 2;
                    }
                    printf("\n");
                }
                parsing = 0;
            }
        } else {
            if (str[sp] == '\x1b') {
                printf("\nESC");
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

int main(int argc, char** args) {
    char* str = "\x1b[31,45mABC\x1b[35mABC";
    printf("%s\x1b[0m\n", str);
    str = ansi_to_irc_color(str);
    printf("\n%s\n", str);
    free(str);
    return 0;
}
