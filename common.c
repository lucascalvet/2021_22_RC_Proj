#include "common.h"

int check_bcc(char * byte_list) {
    char xor = 0;
    for (int i = 0; byte_list[i] != NULL; i++) {
        xor ^= byte_list[i];
    }
}
