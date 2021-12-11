#ifndef LINK_LAYER_H_
#define LINK_LAYER_H_

enum Role
{
    TRANSMITTER,
    RECEIVER
};

// Print statistics for the sender
void write_sender_stats(int file_size);
// Print statistics for the receiver
void write_receiver_stats(int file_size);

// Configure the serial port and
int llopen(char *port, enum Role flag);
// Write buffer of size "length" to fd, calls timeout_write and checks for a response
int llwrite(int fd, unsigned char *buffer, int length);
// Read from fd, call nc_read
int llread(int fd, unsigned char **buffer);
// Closes connection, transmitter sends disc which is read by receiver and confirmed by transmitter, transmitter writes ua and both close
int llclose(int fd, enum Role flag);

// Writes using timeout, will attempt to send to_write again until it fails an amount of tries defined in macros.h, uses alarm
unsigned char *timeout_write(int fd, unsigned char *to_write, int write_size);
// Handles reading of packages, has state machine to ensure correct read, checks header for type of package to respond accordingly
int nc_read(int fd, unsigned char **read_package);

// Makes bcc from byte_list by exclusive or'ing all its bytes
unsigned char make_bcc(unsigned char *byte_list, int size);
// Makes an info frame with the specified configuration
int make_info(unsigned char *data, int size, int seq_n, unsigned char **info_frame);

// Counts the extra bytes to be added in byte stuffing
int byte_stuffing_count(unsigned char *info_frame, int size);
// Counts the extra bytes that were caused from byte stuffing
int byte_destuffing_count(unsigned char *info_frame, int size);
// Applies byte stuffing in an information frame
int byte_stuffing(unsigned char *info_frame, int size, unsigned char **result_frame);
// Applies byte destuffing in an information frame
int byte_destuffing(unsigned char *info_frame, int size, unsigned char **result_frame);

#endif // LINK_LAYER_H_
