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

volatile int STOP = FALSE;
//int fd;

int alarm_set = FALSE;

/*
int llopen(char *port_name)
{
  int fd = open(port_name, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (fd < 0)
  {
    perror(port_name);
    exit(-1);
  }
  return fd;
}

int llread(int fd, unsigned char flag, int timeout_secs, unsigned char *packet)
{
  int flag_count = 0;
  int res = 0;
  int count = 0;
  unsigned char buf[1];
  while (flag_count < 2)
  {
    while (res < 1 || (timeout_secs > 0))
      res = read(fd, buf, 1);
    packet[count] = buf[0];
    printf("Received %d byte: %02X\n", res, buf[0]);
    if (packet[count] == flag)
      flag_count++;
    count++;
  }
}

int llwrite(int fd, unsigned char *data, int idle_secs, int retries)
{
  int count = 0, read_count = 0, stop = 0, res = 0;
  unsigned char buf[1];
  unsigned char packet[5];
  while (count < retries || stop)
  {
    write_set(fd);
    alarm(idle_secs);
    read_count = 0;
    while (!stop)
    {
      res = read(fd, buf, 1);
      if(res){
        packet[read_count] = buf[0];
        printf("Received %d byte: %02X\n", res, buf[0]);
      }
      read_count += res;
      if (read_count == 5)
      {
        read_count = 0;
        if (packet[3] == packet[1] ^ packet[2])
          stop = 1;
      }
    }
    count++;
  }

  

  return 0;
}
*/



/*int write_data(int fd) {
  unsigned char data[??];
  data[0] = FLAG;
  data[1] = A_SENDER;
  data[2] = C_SET;
  unsigned char bcc_set = C_SET ^ A_SENDER;
  data[3] = bcc_set;
  data[4] = FLAG;

  int res = write(fd, data, ??);
  printf("%d bytes written\n", res);

  return 0;
}*/

void set_alarm() {
  printf("Alarm Sent\n");
  alarm_set = TRUE;
}

void timeout_write(int fd, unsigned char* to_write, int size) {
  (void) signal(SIGALRM, set_alarm);
  
  write(fd, to_write, size);
  alarm(3);

  int count;
  unsigned char packet[5];
  
  int tries = 3;
  alarm_set = FALSE;

  while (tries > 0) {
    count = 0;
    STOP = FALSE;

    while (STOP == FALSE && tries > 0)
    {
      res = read(fd, buf, 1);
      if (res){
        printf("Received %d byte: %02X\n", res, buf[0]);
        packet[count] = buf[0];
        count++;
      }
      if (count >= 5){
        STOP = TRUE;
      }
      if (alarm_set) {
        alarm_set = FALSE;
        tries--;
        if (tries > 0)
        {
          write(fd, to_write, size);
          alarm(3);
          printf("Alarm triggered, trying again. %d tries left\n", tries);
        }
      }
    }

    if (STOP && make_bcc(&packet[1], 2) == packet[3]) {
      printf("Received UA\n");
      break;
    }
  }

  if(tries == 0){
    printf("Didn't receive confirmation\n");
  }
  else {
    printf("Success\n");
  }
}

int main(int argc, char **argv)
{
  int c, res;
  struct termios oldtio, newtio;
  unsigned char buf[255];
  int i, sum = 0, speed = 0;

  if ((argc < 2) ||
      ((strcmp("/dev/ttyS0", argv[1]) != 0) &&
       (strcmp("/dev/ttyS1", argv[1]) != 0) &&
       (strcmp("/dev/ttyS10", argv[1]) != 0) &&
       (strcmp("/dev/ttyS11", argv[1]) != 0)))
  {
    printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
    exit(1);
  }

  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */

  /*
    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }
    */


  int fd = open(argv[1], O_RDWR | O_NOCTTY | O_NONBLOCK);

  if (tcgetattr(fd, &oldtio) == -1)
  { /* save current port settings */
    perror("tcgetattr");
    exit(-1);
  }

  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;

  /* set input mode (non-canonical, no echo,...) */
  newtio.c_lflag = 0;
  newtio.c_cc[VTIME] = 0; /* inter-character timer unused */
  newtio.c_cc[VMIN] = 0;  /* blocking read until 5 chars received */

  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) pr�ximo(s) caracter(es)
  */

  tcflush(fd, TCIOFLUSH);

  //sleep(1);
  if (tcsetattr(fd, TCSANOW, &newtio) == -1)
  {
    perror("tcsetattr");
    exit(-1);
  }

  printf("New termios structure set\n");

  /* 
    O ciclo FOR e as instru��es seguintes devem ser alterados de modo a respeitar 
    o indicado no gui�o 
  */

  timeout_write(fd, /**coisa pra escrever*/, /**tamamho**/);

  sleep(1);
  if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
  {
    perror("tcsetattr");
    exit(-1);
  }

  close(fd);
  return 0;
}
