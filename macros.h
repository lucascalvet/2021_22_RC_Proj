#ifndef MACROS_H_
#define MACROS_H_

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
#define MAX_PACKET_SIZE 500
#define TIMEOUT 3
#define TRIES 5


#define ESCAPE 0x7D
#define FLAG_REP 0x5E
#define ESCAPE_REP 0x5D
#define REP 0x20

#define CP_START 0x02
#define CP_END 0x03
#define DP  0x01
#define CP_T_FSIZE 0x00
#define CP_T_FNAME 0x01

#endif  // MACROS_H_
