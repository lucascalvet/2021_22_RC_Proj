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

volatile int STOP=FALSE;

/*
int nc_read(int fd, unsigned char* to_write, int read_size, int write_size, unsigned char check_byte){
  int count = 0;
  int received = FALSE;
  int res;
  //unsigned char packet[5];
  unsigned char buf[2];
  unsigned char * packet = (unsigned char *) malloc(read_size * sizeof(unsigned char));
  while(!received){
    while (STOP==FALSE && !received) {
        res = read(fd, buf, 1);
        if(res){
          packet[count] = buf[0];
          printf("Received %d byte: %02X\n", res, buf[0]);
          count++;
        }
        if (count == read_size) STOP = TRUE;
    }
    
    if (make_bcc(&packet[1], 2) == packet[3] && packet[2] == check_byte) {
        printf("Feedback Checked Out\n");
        received = TRUE;
        write(fd, to_write, write_size);
        printf("Sent Feedback\n");
    }
    else{
      STOP = FALSE;
      count = 0;
    }
  }

  return 1;
}
*/


int nc_read_flag(int fd){
  int count = 0, flag_state = 0; // 0-> Beg | 1->First Batch | 2->Mid Frame | 3->End
  int received = FALSE;
  int res, address_index;
  //unsigned char packet[5];
  unsigned char buf[2];
  unsigned char * packet = (unsigned char *) malloc(MAX_DATA_SIZE * sizeof(unsigned char));
  unsigned char * read_res = (unsigned char *) malloc(MAX_DATA_SIZE * sizeof(unsigned char));
  while(!received){
    while (STOP==FALSE && !received) {
        res = read(fd, buf, 1);
        if(res){
          
          printf("Received %d byte: %02X\n", res, buf[0]);
          
          if(packet[count] == FLAG){
            switch flag_state{
              case 0:
                flag_state = 1;
                break;
              case 2:
                flag_state = 3;
                break;
              default:
                break;
            }
          } 
          else{
            packet[count] = buf[0];
            count++;
            if(flag_state == 1){
              //address_index = count;
              flag_state = 2;
            }
          }
          //count++;
        }

        if (flag_state == 3){
          STOP = TRUE;
          flag_state = 1;
        }
    }
    
    //if (make_bcc(&packet[address_index], 2) == packet[3])
    if (make_bcc(&packet[0], 2) == packet[2]) { 
      printf("Info Header Checked Out\n");
      received = TRUE;
      read_res = packet;
      unsigned char * to_write;
      int write_size = 5;
      int n_seq = 0;
      switch(packet[1]){
        case C_SET:
          printf("SET Received. Sending UA\n");
          make_receiver_ua(&to_write);
          break;

        case C_DISC:
          printf("DISC Received. Sending DISC\n");
          make_receiver_disc(&to_write);
          break;

        case C_INFO:
          n_seq = 1;
        case C_INFO_N:
          unsigned char * destuffed_info;
          byte_destuffing(packet, count, &destuffed_info);
          read_res = destuffed_info;
          if(make_bcc(&packet[3], count - 4) == packet[count - 1]){
            printf("Info Body Checks Out. Sending RR\n");
            make_receiver_rr(&to_write, n_seq);
          }
          else{
            printf("Info Body Wrong. Sending REJ\n");
            make_receiver_rej(&to_write, n_seq);
          }
          break;

        case C_UA:
          printf("Reading Complete\n");
          break;

        default:
          received = false;
          break;
      }
      
      write(fd, to_write, write_size);
      printf("Sent Feedback\n");
    }
    
    STOP = FALSE;
    count = 0;
    flag_state = 1;
  }

  free(packet);
  return read_res;
}

/*
int nc_read_info(int fd){
  int count = 0, flag_state = 0; // 0-> Beg | 1->First Batch | 2->Mid Frame | 3->End
  int received = FALSE, res;
  //unsigned char packet[5];
  unsigned char buf[2];
  unsigned char * packet = (unsigned char *) malloc(MAX_DATA_SIZE * sizeof(unsigned char));
  while(!received){
    while (STOP==FALSE && !received) {
        res = read(fd, buf, 1);
        if(res){
          
          printf("Received %d byte: %02X\n", res, buf[0]);
          
          if(packet[count] == FLAG){
            switch flag_state{
              case 0:
                flag_state = 1;
                break;
              case 2:
                flag_state = 3;
                break;
              default:
                break;
            }
          } 
          else{
            packet[count] = buf[0];
            count++;
            if(flag_state == 1){
              //address_index = count;
              flag_state = 2;
            }
          }
          //count++;
        }

        if (flag_state == 3){
          STOP = TRUE;
          flag_state = 1;
        } 
    }
    
    //if (make_bcc(&packet[address_index], 2) == packet[3])
    if (make_bcc(&packet[0], 2) == packet[2]) { 
      printf("Info Header Checked Out\n");
      received = TRUE;
      unsigned char * to_write;
      int write_size = 5;
      int n_seq = 0;
      switch(packet[1]){
        case C_INFO:
          n_seq = 1;
        case C_INFO_N:
          unsigned char * destuffed_info;
          byte_destuffing(packet, count, &destuffed_info);
          if(make_bcc(&packet[3], count - 4) == packet[count - 1]){
            printf("Info Body Checks Out. Sending RR\n");
            make_receiver_rr(&to_write, n_seq);
          }
          else{
            printf("Info Body Wrong. Sending REJ\n");
            make_receiver_rej(&to_write, n_seq);
          }
          break;

        default:
          break;
      }
      
      write(fd, to_write, write_size);
      printf("Sent Feedback\n");
    }
  }

  return count;
}
*/


/* 
if(!check_info || make_bcc(&packet[4], write_size - 6) == packet[write_size - 2]) //CHECK INFO BCC2
if(check_info){
  printf("Info Checked Out\n");
  }
*/

/*
int nc_read_checkless(int fd, unsigned char* to_write, int read_size, int write_size){
  return nc_read(fd, to_write, read_size, write_size, 0, FALSE);
}

int nc_read_check(int fd, unsigned char* to_write, int read_size, int write_size, unsigned char check_byte){
  return nc_read(fd, to_write, read_size, write_size, check_byte, TRUE);
}
*/


int main(int argc, char** argv)
{
    int fd, c, res;
    struct termios oldtio,newtio;
    char buf[255];

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
  
    
    fd = open(argv[1], O_RDWR | O_NOCTTY );
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
    newtio.c_cc[VMIN]     = 0;   /* blocking read until 5 chars received */



  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) pr�ximo(s) caracter(es)
  */



    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");

    unsigned char ua[5];
    ua[0] = FLAG;
    ua[1] = A_RECEIVER;
    ua[2] = C_UA;
    ua[3] = ua[1] ^ ua[2];
    ua[4] = FLAG;
    //unsigned char * ua;
    //make_receiver_ua(&ua);

    nc_read(fd, ua, 5, 5, C_SET);

    
    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
