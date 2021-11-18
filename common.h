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
#define C_DISC 0x0B
#define C_RR 0x05
#define C_RR_N 0x85
#define C_REJ 0x01
#define C_REJ_N 0x81

#define C_INFO 0x00
#define C_INFO_N 0x40

#define MAX_DATA_SIZE 200

#define ESCAPE 0x7D
#define FLAG_REP 0x5E
#define ESCAPE_REP 0x5D
#define REP 0x20


unsigned char make_bcc(unsigned char * byte_list, int size);
//bool check_bcc(unsigned char * byte_list, unsigned char bcc);

/*
int write_sender_set(int fd);
int write_receiver_ua(int fd);
int write_sender_ua(int fd);
int write_receiver_rr(int fd, int n_seq);
int write_receiver_rej(int fd, int n_seq);
int write_sender_disc(int fd);
int write_receiver_disc(int fd);
*/

int write_information(int fd, unsigned char * data, int size, int seq_n);

int byte_stuffing_count(unsigned char * info_frame, int size);
int byte_stuffing(unsigned char * info_frame, int size, unsigned char ** result_frame);

/*
int make_sender_set(unsigned char * sender_set, int size);
int make_receiver_ua(unsigned char * receiver_ua, int size);
int make_sender_ua(unsigned char * sender_ua, int size);
int make_receiver_rr(unsigned char * receiver_rr, int size, int n_seq);
int make_receiver_rej(unsigned char * receiver_rej, int size, int n_seq);
int make_sender_disc(unsigned char * sender_disc, int size);
int make_receiver_disc(unsigned char * receiver_disc, int size);

*/









