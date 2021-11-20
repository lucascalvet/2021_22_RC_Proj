#include "link_layer.h"

struct termios oldtio;

int llopen(int port, Role flag) {
    char port_path[20];
    struct termios newtio;
    
    sprintf(port_path, "/dev/ttyS%d", port); /* Dispositivo /dev/ttySx, x = 0, 1*/
    int fd = open(port_path, O_RDWR | O_NOCTTY);
    if (fd <0) {perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 0;   /* non-blocking */

    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    if (flag == TRANSMITTER) {
        
    }
    else if (flag == RECEIVER) {

    }

    printf("New termios structure set\n");

    return fd;
}