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
  unsigned char *stuffed_frame = (unsigned char *)malloc((size - byte_destuffing_count(info_frame, size)) * sizeof(unsigned char));
  int counter = 0;
  for (int i = 0; i < size; i++)
  {
    if (info_frame[i] == ESCAPE)
    {
      switch (info_frame[++i])
      {
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
    else
    {
      stuffed_frame[counter] = info_frame[i];
    }
    counter++;
  }

  *result_frame = stuffed_frame;
  return (++counter);
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
