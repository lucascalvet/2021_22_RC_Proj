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
#include <libgen.h>
#include <errno.h>
#include <error.h>

#include "link_layer.h"
#include "app.h"

static int read_file_size = 0;

int main(int argc, char **argv)
{
  int port_fd, file_fd;
  int file_size, end_file_size;
  int package_len, seq_n;
  int end_package_stream = FALSE;
  unsigned char *package;
  char *file_name;
  unsigned char *data;
  char *end_file_name;

  if (argc == 2 || argc == 3)
  {
    if (strncmp(argv[1], "/dev/ttyS", 9) != 0)
    {
      printf("Usage:\twnc SerialPort OutputFile\n\twnc /dev/ttySX <path_file>\n\tex: nserial /dev/ttyS1 pinguim.gif\n");
      exit(1);
    }
  }
  else
  {
    printf("Usage:\twnc SerialPort OutputFile\n\twnc /dev/ttySX <path_file>\n\tex: nserial /dev/ttyS1 pinguim.gif\n");
    exit(1);
  }

  port_fd = llopen(argv[1], RECEIVER);
  if (port_fd == -1)
  {
    error(1, 0, "error opening serial port");
    ;
  }

  int received_start_cp = FALSE;

  while (!received_start_cp)
  {
    package_len = llread(port_fd, &package);
    if (package[0] == CP_START)
    {
      if(verbose) printf("Start Control Package Received\n");
      read_control_package(package, package_len, &file_size, &file_name);
      received_start_cp = TRUE;
    }
    free(package);
  }

  if (argc == 3)
  {
    file_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0666);
  }
  else
  {
    file_fd = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  }

  if (file_fd == -1)
  {
    free(file_name);
    error(1, errno, "unable to open/create the file");
  }

  while (!end_package_stream)
  {
    if ((package_len = llread(port_fd, &package)) < 0)
    {
      free(package);
      error(1, errno, "llread failed");
    }

    switch (package[0])
    {
    case DP:
      package_len = read_data_package(package, &seq_n, &data);
      int written_size = write(file_fd, data, package_len);
      free(data);
      if (package_len <= 0 || written_size != package_len)
      {
        free(package);
        error(1, errno, "write to file failed");
      }
      read_file_size += written_size;
      break;
    case CP_END:
      package_len = read_control_package(package, package_len, &end_file_size, &end_file_name);
      if(verbose) printf("Final Control Package Read\n");
      if (strcmp(end_file_name, file_name) != 0)
      {
        if(verbose) printf("End file name: %s :-: Begin file name: %s\n", end_file_name, file_name);
        free(package);
        free(end_file_name);
        error(1, errno, "File information isn't the same");
      }
      free(end_file_name);

      if (end_file_size != file_size || end_file_size != read_file_size)
      {
        if(verbose) printf("End file size: %d :-: Begin file size: %d :-: Received file size: %d\n", end_file_size, file_size, read_file_size);
        free(package);
        free(end_file_name);
        error(1, errno, "File information isn't the same");
      }
      end_package_stream = TRUE;
      break;
    default:
      if(verbose) printf("Received unexpected package type: %X\n", package[0]);
      break;
    }
    free(package);
  }

  if(verbose){
    printf("Package Stream Ended\n");
    printf("FILENAME: %s\n", file_name);
    printf("FILESIZE: %d\n", file_size);
  }
  
  free(file_name);

  if (llclose(port_fd, RECEIVER) != 0)
  {
    error(1, 0, "error closing serial port");
  }

  if (close(file_fd) != 0)
  {
    error(1, errno, "File didn't close properly");
  }

  write_receiver_stats(read_file_size);

  return 0;
}
