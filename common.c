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

int write_sender_set(int fd)
{
  unsigned char set[5];
  set[0] = FLAG;
  set[1] = A_SENDER;
  set[2] = C_SET;
  unsigned char bcc_set = C_SET ^ A_SENDER;
  set[3] = bcc_set;
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
  unsigned char bcc_ua = C_UA ^ A_RECEIVER;
  ua[3] = bcc_ua;
  ua[4] = FLAG;

  int res = write(fd, ua, 5);
  printf("%d bytes written\n", res);

  return 0;
}
