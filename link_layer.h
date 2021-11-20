#include <termios.h>
#include "common.h"

enum Role {TRANSMITTER, RECEIVER};

int llopen(int port, Role flag);
int llwrite(int fd, unsigned char * buffer, int length);
int llread(int fd, unsigned char * buffer);
int llclose(int fd);