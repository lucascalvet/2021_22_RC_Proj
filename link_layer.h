#ifndef LINK_LAYER_H_
#define LINK_LAYER_H_

enum Role {TRANSMITTER, RECEIVER};

void write_sender_stats(int file_size);
void write_receiver_stats(int file_size);

int llopen(char * port, enum Role flag);
int llwrite(int fd, unsigned char * buffer, int length);
int llread(int fd, unsigned char ** buffer);
int llclose(int fd, enum Role flag);

unsigned char * timeout_write(int fd, unsigned char* to_write, int write_size);
int nc_read(int fd, unsigned char ** read_package);

unsigned char make_bcc(unsigned char * byte_list, int size);
int make_info(unsigned char * data, int size, int seq_n, unsigned char ** info_frame);

int byte_stuffing_count(unsigned char * info_frame, int size);
int byte_destuffing_count(unsigned char * info_frame, int size);
int byte_stuffing(unsigned char * info_frame, int size, unsigned char ** result_frame);
int byte_destuffing(unsigned char * info_frame, int size, unsigned char ** result_frame);

#endif  // LINK_LAYER_H_
