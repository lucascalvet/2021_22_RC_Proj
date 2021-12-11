#ifndef APP_H_
#define APP_H_

// Prepares data packages to be sent by the writer with their respective header, returns its size
int make_data_package(int seq_n, unsigned char *data, int size, unsigned char **data_package);
// Verifies if received a data package and reads it, returns read size
int read_data_package(unsigned char *data_package, int *seq_n, unsigned char **data);
// Prepares a control package according to the applications configuration, returns its size
int make_control_package(int start, int size, char *file_name, unsigned char **control_package);
// Verifies if received a control package and reads it, returns read size
int read_control_package(unsigned char *control_package, int package_size, int *file_size, char **file_name);

#endif // APP_H_
