#ifndef APP_H
#define APP_H

int make_data_package(int seq_n, unsigned char * data, int size, unsigned char ** data_package);
int read_data_package(unsigned char * data_package, int * seq_n, unsigned char ** data);
int make_control_package(int start, int size, char * file_name, unsigned char ** control_package);
int read_control_package(unsigned char * control_package, int package_size, int * file_size, char ** file_name);

#endif