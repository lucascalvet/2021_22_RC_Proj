/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "common.h"

int main(int argc, char **argv)
{
  int c, res;
  struct termios oldtio, newtio;
  unsigned char buf[255];
  int i, sum = 0, speed = 0;
  
  if (argc < 3)
  {
    printf("Usage:\twnc SerialPort File\n\tex: nserial /dev/ttyS1 pinguim.gif\n");
    exit(1);
  }

  char port_path[10];
  strncpy(port_path, argv[1], 9);
  port_path[9] = '\0';
  if (strcmp("/dev/ttyS", port_path) != 0)
  {
    printf("Usage:\twnc SerialPort File\n\tex: nserial /dev/ttyS1 pinguim.gif\n");
    exit(1);
  }

  char * filename = argv[2];

  return 0;
}
