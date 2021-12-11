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
#include <libgen.h>
#include <errno.h>
#include <error.h>

#include "./link_layer.h"
#include "./app.h"
#include "./macros.h"

static int write_file_size = 0;
int verbose = FALSE;

int get_file_size(FILE *fp)
{
  fseek(fp, 0L, SEEK_END);
  return ftell(fp);
  //fseek(fp, 0L, SEEK_SET)
}

int main(int argc, char **argv)
{
  if (argc < 3 || argc > 4)
  {
    printf("error: number of arguments\n\nUsage:\twnc SerialPort InputFile\n\twnc /dev/ttyS<X> <file_path> [-v]\n\tX is the serial port number\n\tfile_path is the path of the file to read and then send\n\t-v (optional) enables verbose mode\n\tex:./wnc /dev/ttyS1 pinguim.gif\n");
    exit(1);
  }
  char port_path[10];
  strncpy(port_path, argv[1], 9);
  port_path[9] = '\0';
  if (strcmp("/dev/ttyS", port_path) != 0)
  {
    printf("error: invalid port\n\nUsage:\twnc SerialPort InputFile\n\twnc /dev/ttyS<X> <file_path> [-v]\n\tX is the serial port number\n\tfile_path is the path of the file to read and then send\n\t-v (optional) enables verbose mode\n\tex:./wnc /dev/ttyS1 pinguim.gif\n");
    exit(1);
  }

  char *filepath = argv[2];
  struct stat file_info;
  if (stat(filepath, &file_info) != 0)
  {
    error(1, errno, "unable to open the provided file path (\"%s\")", filepath);
  }
  if (!S_ISREG(file_info.st_mode))
  {
    error(1, 0, "the provided path does not correspond to a regular file!");
  }

  int file_fd = open(filepath, O_RDONLY);
  if (file_fd == -1)
  {
    error(1, errno, "unable to open the provided file");
  }

  verbose = FALSE;
  if (argc == 4)
  {
    if (strcmp("-v", argv[3]) != 0)
    {
      printf("error: invalid argument\n\nUsage:\twnc SerialPort InputFile\n\twnc /dev/ttyS<X> <file_path> [-v]\n\tX is the serial port number\n\tfile_path is the path of the file to read and then send\n\t-v (optional) enables verbose mode\n\tex:./wnc /dev/ttyS1 pinguim.gif\n");
      exit(1);
    }
    else
    {
      verbose = TRUE;
    }
  }

  int port_fd = llopen(argv[1], TRANSMITTER);
  if (port_fd == -1)
    error(1, errno, "cannot open the serial port");
  if (port_fd == -2)
    error(1, 0, "no response after %d tries, cannot establish a connection", TRIES);
  if (port_fd == -3)
    error(1, 0, "got unexpected response, cannot establish a connection");
  if (port_fd < 0)
    error(1, 0, "error opening the serial port");
  if (verbose)
    printf("Connection established as the transmitter.\n");

  unsigned char *app_packet;
  int size, res;
  size = make_control_package(TRUE, file_info.st_size, basename(filepath), &app_packet);
  res = llwrite(port_fd, app_packet, size);
  free(app_packet);
  if (res < 0)
  {
    if (res == -1)
      error(1, errno, "cannot write to the serial port");
    if (res == -2)
      error(1, 0, "no response after %d tries while writing", TRIES);
  }

  int STOP = FALSE, read_size;
  unsigned char read_buffer[MAX_DATA_SIZE - 4];
  int seq_n = 0;
  while (!STOP)
  {
    read_size = read(file_fd, read_buffer, MAX_DATA_SIZE - 4);
    if (read_size < 0)
    {
      error(1, errno, "error while reading the input file");
    }
    else if (read_size == 0)
    {
      if (verbose)
        printf("Finished sending the input file\n");
      STOP = TRUE;
    }
    else
    {
      size = make_data_package(seq_n, read_buffer, read_size, &app_packet);
      seq_n++;
      int llw_res = llwrite(port_fd, app_packet, size);

      if (llw_res < 0)
      {
        free(app_packet);
        error(1, 0, "error in llwrite");
      }

      write_file_size += size - 4;

      free(app_packet);
    }
  }

  size = make_control_package(FALSE, file_info.st_size, basename(filepath), &app_packet);
  if (llwrite(port_fd, app_packet, size) < 0)
  {
    free(app_packet);
    error(1, 0, "error in llwrite");
  }
  free(app_packet);

  if (llclose(port_fd, TRANSMITTER) != 0)
  {
    error(1, 0, "error closing serial port");
  }

  if (close(file_fd) != 0)
  {
    error(1, errno, "File didn't close properly");
  }

  if (verbose)
    write_sender_stats(write_file_size);

  printf("Exiting successfully.\n");
  return 0;
}
