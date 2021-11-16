#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define FLAG 0x7E
#define A_SENDER 0x03
#define A_RECEIVER 0x01
#define C_UA 0x07
#define C_SET 0x03

unsigned char make_bcc(unsigned char * byte_list, int size);
//bool check_bcc(unsigned char * byte_list, unsigned char bcc);
int write_sender_set(int fd);
int write_receiver_ua(int fd);










