#ifndef LINKLAYER_H
#define LINKLAYER_H

#include <termios.h>
#include "macros.h"

enum Role {TRANSMITTER, RECEIVER};

int llopen(int port, enum Role flag);
int llwrite(int fd, unsigned char * buffer, int length);
int llread(int fd, unsigned char * buffer);
int llclose(int fd, enum Role flag);

unsigned char * timeout_write(int fd, unsigned char* to_write, int write_size);
int nc_read(int fd, unsigned char ** read_package);

unsigned char make_bcc(unsigned char * byte_list, int size);
int make_info(unsigned char * data, int size, int seq_n, unsigned char ** info_frame);

int write_information(int fd, unsigned char * data, int size, int seq_n);

int byte_stuffing_count(unsigned char * info_frame, int size);
int byte_destuffing_count(unsigned char * info_frame, int size);
int byte_stuffing(unsigned char * info_frame, int size, unsigned char ** result_frame);
int byte_destuffing(unsigned char * info_frame, int size, unsigned char ** result_frame);


int make_sender_set(unsigned char ** sender_set);
int make_receiver_ua(unsigned char ** receiver_ua);
int make_sender_ua(unsigned char ** sender_ua);
int make_receiver_rr(unsigned char ** receiver_rr, int n_seq);
int make_receiver_rej(unsigned char ** receiver_rej, int n_seq);
int make_sender_disc(unsigned char ** sender_disc);
int make_receiver_disc(unsigned char ** receiver_disc);

#endif