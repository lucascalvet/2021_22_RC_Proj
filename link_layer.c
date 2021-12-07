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
#include <errno.h>

#include "link_layer.h"

extern int verbose;
struct termios oldtio;
static int alarm_set = FALSE;
int n = 0;
static int sender_set_count = 0, receiver_ua_count = 0;
static int sender_inf_count = 0, receiver_rr_count = 0, receiver_rej_count = 0;
static int sender_ua_count = 0, sender_disc_count = 0, receiver_disc_count = 0;

void set_alarm()
{
  //printf("Alarm Sent\n");
  alarm_set = TRUE;
}

void write_sender_stats(int file_size)
{
  printf("Protocol Statistics\n");
  printf("Size Sent: %d\n", file_size);
  printf("Sent %d SET Frames\n", sender_set_count);
  printf("Sent %d INF Frames\n", sender_inf_count);
  printf("Sent %d DISC Frames\n", sender_disc_count);
  printf("Sent %d UA Frames\n", sender_ua_count);
  printf("Received %d UA Frames\n", receiver_ua_count);
  printf("Received %d RR Frames\n", receiver_rr_count);
  printf("Received %d REJ Frames\n", receiver_rej_count);
  printf("Received %d DISC Frames\n", receiver_disc_count);
}

void write_receiver_stats(int file_size)
{
  printf("Protocol Statistics\n");
  printf("Size Received: %d\n", file_size);
  printf("Received %d SET Frames\n", sender_set_count);
  printf("Received %d INF Frames\n", sender_inf_count);
  printf("Received %d DISC Frames\n", sender_disc_count);
  printf("Received %d UA Frames\n", sender_ua_count);
  printf("Sent %d UA Frames\n", receiver_ua_count);
  printf("Sent %d RR Frames\n", receiver_rr_count);
  printf("Sent %d REJ Frames\n", receiver_rej_count);
  printf("Sent %d DISC Frames\n", receiver_disc_count);
}

int llopen(char *port, enum Role flag)
{
  struct termios newtio;

  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */
  int fd = open(port, O_RDWR | O_NOCTTY);
  if (fd < 0)
  {
    return -1;
  }

  /* save current port settings */
  if (tcgetattr(fd, &oldtio) == -1)
  {
    return -1;
  }

  /* generate new port settings */
  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;

  /* set input mode (non-canonical, no echo,...) */
  newtio.c_lflag = 0;
  newtio.c_cc[VTIME] = 0; /* inter-character timer unused */
  newtio.c_cc[VMIN] = 0;  /* non-blocking */

  tcflush(fd, TCIOFLUSH);

  if (tcsetattr(fd, TCSANOW, &newtio) == -1)
  {
    return -1;
  }

  if (flag == TRANSMITTER)
  {
    unsigned char set[5];
    set[0] = FLAG;
    set[1] = A_SENDER;
    set[2] = C_SET;
    set[3] = set[1] ^ set[2];
    set[4] = FLAG;

    unsigned char *response;
    response = timeout_write(fd, set, 5);
    sender_set_count++;
    if (response == NULL) // There was no response after a set number of tries
    {
      free(response);
      return -2;
    }
    if (response[1] != C_UA) // Got unexpected response
    {
      free(response);
      return -3;
    }
    receiver_ua_count++;
    free(response);
  }
  else if (flag == RECEIVER)
  {
    unsigned char *request;
    nc_read(fd, &request);
    if (request == NULL)
    {
      error(1, 0, "nc_read() returned NULL, this should not happen\n");
    }
    if (request[1] != C_SET)
    {
      free(request);
      printf("Got wrong instruction, expected SET.\n");
      return -1;
    }
    free(request);
    printf("Receiver - Connection established.\n");
  }

  return fd;
}

int llclose(int fd, enum Role flag)
{
  printf("Closing connection...\n");

  if (flag == TRANSMITTER)
  {
    unsigned char disc[5];
    disc[0] = FLAG;
    disc[1] = A_SENDER;
    disc[2] = C_DISC;
    disc[3] = disc[1] ^ disc[2];
    disc[4] = FLAG;

    unsigned char *response;
    response = timeout_write(fd, disc, 5);
    sender_disc_count++;
    if (response == NULL)
    {
      free(response);
      printf("No response to DISC after %d tries.\n", TRIES);
      return -1;
    }
    if (response[1] != C_DISC)
    {
      free(response);
      printf("Wrong response, expected DISC.\n");
      return -1;
    }
    receiver_disc_count++;
    free(response);

    unsigned char ua[5];
    ua[0] = FLAG;
    ua[1] = A_RECEIVER;
    ua[2] = C_UA;
    ua[3] = ua[1] ^ ua[2];
    ua[4] = FLAG;
    if (write(fd, ua, 5) != 5)
    {
      error(0, errno, "error writing UA");
      return -1;
    }
    sender_ua_count++;

    printf("Transmitter - Connection closed.\n");
  }
  else if (flag == RECEIVER)
  {
    unsigned char *request;
    nc_read(fd, &request);
    if (request == NULL)
    {
      error(1, 0, "nc_read() returned NULL, this should not happen\n");
    }
    if (request[1] != C_DISC)
    {
      free(request);
      printf("Got wrong instruction, expected DISC.");
      return -1;
    }
    free(request);
    nc_read(fd, &request);
    if (request == NULL)
    {
      error(1, 0, "nc_read() returned NULL, this should not happen\n");
    }
    if (request[1] != C_UA)
    {
      free(request);
      printf("Got wrong instruction, expected UA.");
      return -1;
    }
    free(request);
    printf("Receiver - Connection closed.\n");
  }

  sleep(1);

  if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
  {
    perror("tcsetattr");
    exit(-1);
  }

  if (close(fd) == -1)
  {
    error(0, errno, "error closing the serial port");
    return -1;
  }

  return 0;
}

int llwrite(int fd, unsigned char *buffer, int length)
{
  unsigned char *info_frame;
  unsigned char *response;
  int rej = FALSE;

  int size = make_info(buffer, length, n, &info_frame);
  int try_again;
  do
  {
    sender_inf_count++;
    
    try_again = FALSE;
    response = timeout_write(fd, info_frame, size);
    if (response == NULL) // There was no response after a set number of tries
    {
      free(response);
      return -2;
    }

    if (response[1] == C_REJ_N || response[1] == C_REJ || (response[1] == C_RR_N && n) || (response[1] == C_RR && !n))
    {
      try_again = TRUE;
      if (response[1] == C_REJ_N || response[1] == C_REJ) receiver_rej_count++;
      else receiver_rr_count++;
      
      free(response);
      if (verbose)
        printf("[llwrite] Data frame rejected by receiver, trying again...\n");
    }

  } while (try_again);

  if (response[1] != C_RR_N && response[1] != C_RR) // Got unexpected response
  {
    free(response);
    error(1, 0, "Wrong response.\n");
  }
  else
  {
    if (n)
    {
      n = 0;
    }
    else
    {
      n = 1;
    }
    receiver_rr_count++;
    free(response);
  }

  return size;
}

int llread(int fd, unsigned char **buffer)
{
  //unsigned char * request = (unsigned char *) malloc(MAX_DATA_SIZE * sizeof(unsigned char));
  unsigned char *request;
  int size = nc_read(fd, &request);
  if (request == NULL || size == 0)
  {
    error(1, 0, "nc_read() returned NULL, this should not happen\n");
  }
  if (request[1] != C_INFO && request[1] != C_INFO_N)
  {
    free(request);
    printf("Got wrong instruction, expected INFO.");
    return -1;
  }

  *buffer = (unsigned char *)malloc((size - 4) * sizeof(unsigned char));
  memcpy(*buffer, &request[3], size - 4);
  free(request);
  return size - 4;
}

unsigned char *timeout_write(int fd, unsigned char *to_write, int write_size)
{
  int STOP = FALSE;
  (void)signal(SIGALRM, set_alarm);

  write(fd, to_write, write_size);
  alarm(TIMEOUT);

  int res, count = 0, flag_state = 0;
  unsigned char *packet = (unsigned char *)malloc(MAX_DATA_SIZE * sizeof(unsigned char));
  unsigned char buf;

  int tries = TRIES;
  alarm_set = FALSE;

  while (tries > 0)
  {
    count = 0;
    STOP = FALSE;
    //flag_state = 0;

    while (STOP == FALSE && tries > 0)
    {
      res = read(fd, &buf, 1);
      if (res)
      {
        printf("Received %d byte: %02X\n", res, buf);

        if (buf == FLAG)
        {
          switch (flag_state)
          {
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
        else
        {
          if (flag_state == 1)
          {
            flag_state = 2;
          }
          if (flag_state == 2)
          {
            printf("Packet Received byte number %d: %02X\n", count, buf);
            packet[count] = buf;
            count++;
          }
        }
      }

      if (alarm_set)
      {
        alarm_set = FALSE;
        tries--;
        if (tries > 0)
        {
          write(fd, to_write, write_size);
          printf("Alarm triggered, trying again. %d tries left\n", tries);
          alarm(TIMEOUT);
        }
      }

      if (flag_state == 3)
      {
        STOP = TRUE;
        flag_state = 1;
      }
    }

    if (STOP && make_bcc(&packet[0], 2) == packet[2])
    {
      printf("Received correct Feedback\n");
      break;
    }
  }

  if (tries == 0)
  {
    printf("Didn't receive confirmation\n");
    return NULL;
  }
  else
  {
    printf("Success\n");
    return packet;
  }
}

int nc_read(int fd, unsigned char **read_package)
{
  int STOP = FALSE;
  int count = 0, flag_state = 0; // 0-> Beg | 1->First Batch of Flags | 2->Mid Frame
  int received = FALSE, ua = FALSE;
  int res;
  unsigned char buf;
  unsigned char *packet;
  while (!received)
  {
    packet = (unsigned char *)malloc(MAX_PACKET_SIZE * sizeof(unsigned char));
    STOP = FALSE;
    ua = FALSE;
    count = 0;
    while (STOP == FALSE && !received)
    {
      ua = FALSE;
      res = read(fd, &buf, 1);
      if (res)
      {
        if(verbose) printf("Received %d byte: %02X\n", res, buf);

        if (buf == FLAG)
        {
          switch (flag_state)
          {
          case 0:
            flag_state = 1;
            break;
          case 2:
            STOP = TRUE;
            flag_state = 1;
            break;
          default:
            break;
          }
        }
        else
        {
          if (flag_state == 1)
          {
            flag_state = 2;
          }
          if (flag_state == 2)
          {
            if(verbose) printf("Packet Received byte number %d: %02X\n", count, buf);
            packet[count] = buf;
            count++;
          }
        }
      }
    }

    if (count >= 3 && make_bcc(&packet[0], 2) == packet[2])
    {
      if(verbose) printf("Frame BCC checked out\n");
      received = TRUE;
      unsigned char response[5];
      response[0] = FLAG;
      response[1] = A_SENDER;
      response[4] = FLAG;

      int write_size = 5;
      switch (packet[1])
      {
      case C_SET:
        if(verbose) printf("SET Received. Sending UA\n");
        sender_set_count++;
        receiver_ua_count++;
        response[2] = C_UA;
        break;
      case C_DISC:
        if(verbose) printf("DISC Received. Sending DISC\n");
        sender_disc_count++;
        receiver_disc_count++;
        response[1] = A_RECEIVER;
        response[2] = C_DISC;
        break;
      case C_INFO:
      case C_INFO_N:
        sender_inf_count++;
        if ((n == 0 && packet[1] == C_INFO_N) || (n == 1 && packet[1] == C_INFO))
        {
          if(verbose) printf("Received unexpected sequence number data packet, possible duplicate. Sending RR.\n");

          if (n)
          {
            response[2] = C_RR_N;
          }
          else
          {
            response[2] = C_RR;
          }

          received = FALSE;
          response[3] = response[1] ^ response[2];
          write(fd, response, write_size);
          receiver_rr_count++;
          break;
        }
        unsigned char *destuffed_info;
        if(verbose) printf("Count before BD: %d\n", count);
        count = byte_destuffing(packet, count, &destuffed_info);
        if(verbose) printf("Count after BD: %d\n", count);
        free(packet);
        packet = destuffed_info;
        if (make_bcc(&packet[3], count - 4) == packet[count - 1])
        {
          //n ?= 0 : 1;
          if (n)
          {
            n = 0;
            response[2] = C_RR;
          }
          else
          {
            n = 1;
            response[2] = C_RR_N;
          }
          if(verbose) printf("Info Body Checks Out. Sending RR and changing expected sequence number to %d.\n", n);
          receiver_rr_count++;
          //response[2] = C_RR;
        }
        else
        {
          if(verbose) printf("Info Body Wrong. Sending REJ\n");
          if (n)
          {
            response[2] = C_REJ_N;
          }
          else
          {
            response[2] = C_REJ;
          }
          received = FALSE;
          response[3] = response[1] ^ response[2];
          write(fd, response, write_size);
          receiver_rej_count++;
        }
        break;
      case C_UA:
        ua = TRUE;
        if(verbose) printf("Reading Complete\n");
        sender_ua_count++;
        break;

      default:
        received = FALSE;
        break;
      }

      if (received && !ua)
      {
        response[3] = response[1] ^ response[2];
        write(fd, response, write_size);
      }
      if(verbose) printf("Sent Feedback\n");
    }
    flag_state = 1;
    if (!received) {free(packet);}
  }
  *read_package = packet;
  return count;
}

unsigned char make_bcc(unsigned char *byte_list, int size)
{
  unsigned char bcc = 0;
  for (int i = 0; i < size; i++)
  {
    bcc ^= byte_list[i];
  }

  return bcc;
}

int make_info(unsigned char *data, int size, int seq_n, unsigned char **info_frame)
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

  unsigned char *info_result = (unsigned char *)malloc((size + 6) * sizeof(unsigned char));
  for (int i = 0; i < 4; i++)
  {
    info_result[i] = frame_start[i];
  }

  for (int i = 0; i < size; i++)
  {
    info_result[i + 4] = data[i];
  }

  info_result[size + 4] = frame_end[0];
  info_result[size + 5] = frame_end[1];

  //info_frame = info_result;
  //return (size + 6);
  return byte_stuffing(info_result, size + 6, info_frame);
}

int byte_stuffing_count(unsigned char *info_frame, int size)
{
  int counter = 0;
  for (int i = 1; i < size - 1; i++)
  {
    if (info_frame[i] == FLAG || info_frame[i] == ESCAPE)
    {
      counter++;
    }
  }
  return counter;
}

int byte_destuffing_count(unsigned char *info_frame, int size)
{
  int counter = 0;
  for (int i = 1; i < size - 1; i++)
  {
    if (info_frame[i] == ESCAPE)
    {
      counter++;
    }
  }
  return counter;
}

int byte_stuffing(unsigned char *info_frame, int size, unsigned char **result_frame)
{
  unsigned char *stuffed_frame = (unsigned char *)malloc((size + byte_stuffing_count(info_frame, size)) * sizeof(unsigned char));
  int counter = 1;
  stuffed_frame[0] = info_frame[0];
  for (int i = 1; i < size - 1; i++)
  {
    if (info_frame[i] == FLAG || info_frame[i] == ESCAPE)
    {
      stuffed_frame[counter] = ESCAPE;
      stuffed_frame[++counter] = REP ^ info_frame[i];
    }
    else
    {
      stuffed_frame[counter] = info_frame[i];
    }
    counter++;
  }
  stuffed_frame[counter] = info_frame[size - 1];

  *result_frame = stuffed_frame;
  return (++counter);
}

int byte_destuffing(unsigned char *info_frame, int size, unsigned char **result_frame)
{
  unsigned char *destuffed_frame = (unsigned char *)malloc((size - byte_destuffing_count(info_frame, size)) * sizeof(unsigned char));
  int counter = 0;
  for (int i = 0; i < size; i++)
  {
    if (info_frame[i] == ESCAPE)
    {
      switch (info_frame[++i])
      {
      case FLAG_REP:
        destuffed_frame[counter] = FLAG;
        break;
      case ESCAPE_REP:
        destuffed_frame[counter] = ESCAPE;
        break;
      default:
        break;
      }
    }
    else
    {
      destuffed_frame[counter] = info_frame[i];
    }
    counter++;
  }

  *result_frame = destuffed_frame;
  return counter;
}

int make_sender_set(unsigned char **sender_set)
{
  //unsigned char res[5];
  unsigned char *res = (unsigned char *)malloc(5 * sizeof(unsigned char));
  res[0] = FLAG;
  res[1] = A_SENDER;
  res[2] = C_SET;
  res[3] = res[1] ^ res[2];
  res[4] = FLAG;

  *sender_set = res;

  return 5;
}

int make_receiver_ua(unsigned char **receiver_ua)
{
  //unsigned char res[5];
  unsigned char *res = (unsigned char *)malloc(5 * sizeof(unsigned char));
  res[0] = FLAG;
  res[1] = A_RECEIVER;
  res[2] = C_UA;
  res[3] = res[1] ^ res[2];
  res[4] = FLAG;

  *receiver_ua = res;

  return 5;
}

int make_sender_ua(unsigned char **sender_ua)
{
  //unsigned char res[5];
  unsigned char *res = (unsigned char *)malloc(5 * sizeof(unsigned char));
  res[0] = FLAG;
  res[1] = A_SENDER;
  res[2] = C_UA;
  res[3] = res[1] ^ res[2];
  res[4] = FLAG;

  *sender_ua = res;

  return 5;
}

int make_receiver_rr(unsigned char **receiver_rr, int n_seq)
{
  //unsigned char res[5];
  unsigned char *res = (unsigned char *)malloc(5 * sizeof(unsigned char));
  res[0] = FLAG;
  res[1] = A_RECEIVER;
  if (n_seq)
  {
    res[2] = C_RR_N;
  }
  else
  {
    res[2] = C_RR;
  }
  res[3] = res[1] ^ res[2];
  res[4] = FLAG;

  *receiver_rr = res;

  return 5;
}

int make_receiver_rej(unsigned char **receiver_rej, int n_seq)
{
  //unsigned char res[5];
  unsigned char *res = (unsigned char *)malloc(5 * sizeof(unsigned char));
  res[0] = FLAG;
  res[1] = A_RECEIVER;
  if (n_seq)
  {
    res[2] = C_REJ_N;
  }
  else
  {
    res[2] = C_REJ;
  }
  res[3] = res[1] ^ res[2];
  res[4] = FLAG;

  *receiver_rej = res;

  return 5;
}

int make_sender_disc(unsigned char **sender_disc)
{
  //unsigned char res[5];
  unsigned char *res = (unsigned char *)malloc(5 * sizeof(unsigned char));
  res[0] = FLAG;
  res[1] = A_SENDER;
  res[2] = C_DISC;
  res[3] = res[1] ^ res[2];
  res[4] = FLAG;

  *sender_disc = res;

  return 5;
}

int make_receiver_disc(unsigned char **receiver_disc)
{
  //unsigned char res[5];
  unsigned char *res = (unsigned char *)malloc(5 * sizeof(unsigned char));
  res[0] = FLAG;
  res[1] = A_RECEIVER;
  res[2] = C_DISC;
  res[3] = res[1] ^ res[2];
  res[4] = FLAG;

  *receiver_disc = res;

  return 5;
}
