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
#include <error.h>

#include "common.h"

unsigned char make_bcc(unsigned char * byte_list, int size) {
    unsigned char bcc = 0;
    for (int i = 0; i < size; i++) {
        bcc ^= byte_list[i];
    }

    return bcc;
}

/*bool check_bcc(unsigned char * byte_list, unsigned char bcc) {
    return (make_bcc(byte_list) == bcc);
}*/

/*int write_timeout() {
  (void) signal(SIGALRM, set_alarm);
  // write_sender_set(fd);
  alarm(3);
  int count;

}*/

/*
int write_sender_set(int fd)
{
  unsigned char set[5];
  set[0] = FLAG;
  set[1] = A_SENDER;
  set[2] = C_SET;
  set[3] = set[1] ^ set[2];
  set[4] = FLAG;

  int res = write(fd, set, 5);
  printf("%d bytes written\n", res);

  return 0;
}

int write_receiver_ua(int fd)
{
  unsigned char ua[5];
  ua[0] = FLAG;
  ua[1] = A_RECEIVER;
  ua[2] = C_UA;
  ua[3] = ua[1] ^ ua[2];
  ua[4] = FLAG;

  int res = write(fd, ua, 5);
  printf("%d bytes written\n", res);

  return 0;
}

int write_sender_ua(int fd)
{
  unsigned char ua[5];
  ua[0] = FLAG;
  ua[1] = A_SENDER;
  ua[2] = C_UA;
  ua[3] = ua[1] ^ ua[2]
  ua[4] = FLAG;

  int res = write(fd, ua, 5);
  printf("%d bytes written\n", res);

  return 0;
}

int write_receiver_rr(int fd, int n_seq)
{
  unsigned char rr[5];
  rr[0] = FLAG;
  rr[1] = A_RECEIVER;
  if(n_seq) {
    rr[2] = C_RR_N;
  }
  else{
    rr[2] = C_RR;
  }
  rr[3] = rr[1] ^ rr[2];
  rr[4] = FLAG;

  int res = write(fd, rr, 5);
  printf("%d bytes written\n", res);

  return 0;
}

int write_receiver_rej(int fd, int n_seq)
{
  unsigned char rej[5];
  rej[0] = FLAG;
  rej[1] = A_RECEIVER;
  if(n_seq) {
    rej[2] = C_REJ_N;
  }
  else{
    rej[2] = C_REJ;
  }
  rej[3] = rej[1] ^ rej[2];
  rej[4] = FLAG;

  int res = write(fd, rej, 5);
  printf("%d bytes written\n", res);

  return 0;
}

int write_sender_disc(int fd)
{
  unsigned char disc[5];
  disc[0] = FLAG;
  disc[1] = A_SENDER;
  disc[2] = C_DISC;
  disc[3] = disc[1] ^ disc[2];
  disc[4] = FLAG;

  int res = write(fd, disc, 5);
  printf("%d bytes written\n", res);

  return 0;
}

int write_receiver_disc(int fd)
{
  unsigned char disc[5];
  disc[0] = FLAG;
  disc[1] = A_RECEIVER;
  disc[2] = C_DISC;
  disc[3] = disc[1] ^ disc[2];
  disc[4] = FLAG;

  int res = write(fd, disc, 5);
  printf("%d bytes written\n", res);

  return 0;
}
*/

int make_info(unsigned char * data, int size, int seq_n, unsigned char ** info_frame){
  if (size > MAX_DATA_SIZE)
  {
    error(0, 0, "Requested data write size (%d) exceeds maximum allowed size (%d)!", size, MAX_DATA_SIZE);
    return 0;
  }
  if (seq_n != 0 && seq_n != 1)
  {
    error(0, 0, "Invalid seq_n!");
    return 0;
  }

  unsigned char frame_start[4];
  frame_start[0] = FLAG;
  frame_start[1] = A_SENDER;
  frame_start[2] = C_INFO | seq_n << 6;
  frame_start[3] = frame_start[1] ^ frame_start[2];

  unsigned char frame_end[2];
  frame_end[0] = make_bcc(data, size);
  frame_end[1] = FLAG;

  unsigned char * info_result = (unsigned char *) malloc((size + 6) * sizeof(unsigned char));
  for(int i = 0; i < 4; i++){
    info_result[i] = frame_start[i];
  }

  for(int i = 0; i < size; i++){
    info_result[i+4] = data[i];
  }

  info_result[size + 4] = frame_end[0];
  info_result[size + 5] = frame_end[1];

  *info_frame = info_result;
  return (size + 6);
}

int write_information(int fd, unsigned char * data, int size, int seq_n)
{
  if (size > MAX_DATA_SIZE)
  {
    error(0, 0, "Requested data write size (%d) exceeds maximum allowed size (%d)!", size, MAX_DATA_SIZE);
    return 0;
  }
  if (seq_n != 0 && seq_n != 1)
  {
    error(0, 0, "Invalid seq_n!");
    return 0;
  }

  unsigned char frame_start[4];
  frame_start[0] = FLAG;
  frame_start[1] = A_SENDER;
  frame_start[2] = C_INFO | seq_n << 6;
  frame_start[3] = frame_start[1] ^ frame_start[2];

  unsigned char frame_end[2];
  frame_end[0] = make_bcc(data, size);
  frame_end[1] = FLAG;

  int res;
  res = write(fd, frame_start, 4);
  printf("%d bytes written (frame start)\n", res);
  res = write(fd, data, size);
  printf("%d bytes written (data)\n", res);
  res = write(fd, frame_end, 2);
  printf("%d bytes written (frame end)\n", res);
}

int byte_stuffing_count(unsigned char * info_frame, int size){
  int counter = 0;
  for(int i = 1; i < size - 1; i++){
    if(info_frame[i] == FLAG || info_frame[i] == ESCAPE){
      counter++;
    }
  }
  return counter;
}

int byte_destuffing_count(unsigned char * info_frame, int size){
  int counter = 0;
  for(int i = 1; i < size - 1; i++){
    if(info_frame[i] == ESCAPE){
      counter++;
    }
  }
  return counter;
}

int byte_stuffing(unsigned char * info_frame, int size, unsigned char ** result_frame){
  unsigned char * stuffed_frame = (unsigned char *) malloc((size + byte_stuffing_count(info_frame, size)) * sizeof(unsigned char));
  int counter = 1;
  stuffed_frame[0] = info_frame[0];
  for(int i = 1; i < size - 1; i++){
    if(info_frame[i] == FLAG || info_frame[i] == ESCAPE){
      stuffed_frame[counter] = ESCAPE;
      stuffed_frame[++counter] = REP ^ info_frame[i];
    }
    else{
      stuffed_frame[counter] = info_frame[i];
    }
    counter++;
  }
  stuffed_frame[counter] = info_frame[size - 1];

  *result_frame = stuffed_frame;
  return (++counter);
}

int byte_destuffing(unsigned char * info_frame, int size, unsigned char ** result_frame){
  unsigned char * stuffed_frame = (unsigned char *) malloc((size - byte_destuffing_count(info_frame, size)) * sizeof(unsigned char));
  int counter = 0;
  for(int i = 0; i < size; i++){
    if(info_frame[i] == ESCAPE){
      switch (info_frame[++i]){
        case FLAG_REP:
          stuffed_frame[counter] = FLAG;
          break;
        case ESCAPE_REP:
          stuffed_frame[counter] = ESCAPE;
          break;
        default:
          break;
      }
    }
    else{
      stuffed_frame[counter] = info_frame[i];
    }
    counter++;
  }
  
  *result_frame = stuffed_frame;
  return (++counter);
}


int make_sender_set(unsigned char ** sender_set)
{
  unsigned char res[5];
  res[0] = FLAG;
  res[1] = A_SENDER;
  res[2] = C_SET;
  res[3] = res[1] ^ res[2];
  res[4] = FLAG;

  *sender_set = res;

  return 5;
}

int make_receiver_ua(unsigned char ** receiver_ua)
{
  unsigned char res[5];
  res[0] = FLAG;
  res[1] = A_RECEIVER;
  res[2] = C_UA;
  res[3] = res[1] ^ res[2];
  res[4] = FLAG;

  *receiver_ua = res;

  return 5;
}

int make_sender_ua(unsigned char ** sender_ua)
{
  unsigned char res[5];
  res[0] = FLAG;
  res[1] = A_SENDER;
  res[2] = C_UA;
  res[3] = res[1] ^ res[2];
  res[4] = FLAG;

  *sender_ua = res;

  return 5;
}

int make_receiver_rr(unsigned char ** receiver_rr, int n_seq)
{
  unsigned char res[5];
  res[0] = FLAG;
  res[1] = A_RECEIVER;
  if(n_seq) {
    res[2] = C_RR_N;
  }
  else{
    res[2] = C_RR;
  }
  res[3] = res[1] ^ res[2];
  res[4] = FLAG;

  *receiver_rr = res;

  return 5;
}

int make_receiver_rej(unsigned char ** receiver_rej, int n_seq)
{
  unsigned char res[5];
  res[0] = FLAG;
  res[1] = A_RECEIVER;
  if(n_seq) {
    res[2] = C_REJ_N;
  }
  else{
    res[2] = C_REJ;
  }
  res[3] = res[1] ^ res[2];
  res[4] = FLAG;

  *receiver_rej = res;

  return 5;
}

int make_sender_disc(unsigned char ** sender_disc)
{
  unsigned char res[5];
  res[0] = FLAG;
  res[1] = A_SENDER;
  res[2] = C_DISC;
  res[3] = res[1] ^ res[2];
  res[4] = FLAG;
  
  *sender_disc = res;

  return 5;
}

int make_receiver_disc(unsigned char ** receiver_disc)
{
  unsigned char res[5];
  res[0] = FLAG;
  res[1] = A_RECEIVER;
  res[2] = C_DISC;
  res[3] = res[1] ^ res[2];
  res[4] = FLAG;
  
  *receiver_disc = res;

  return 5;
}






