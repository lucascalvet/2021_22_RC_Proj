#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
//#include <signal.h>

#include "common.h"
#include "macros.h"

int make_data_package(int seq_n, unsigned char * data, int size, unsigned char ** data_package){
    unsigned char * result_package = (unsigned char *) malloc((size + 4) * sizeof(unsigned char));
    result_package[0] = DP;
    result_package[1] = seq_n % 256;
    result_package[2] = size / 256;
    result_package[3] = size % 256;

    //memcpy(&result_package[4], data, size);
    //memcpy(data_package[4], data, size);
    for(int i = 0; i < size; i++){
        result_package[i + 4] = data[i];
    }

    *data_package = result_package;
    return size + 4;
}

int read_data_package(unsigned char * data_package, int * seq_n, unsigned char ** data){
    if(data_package[0] != DP){
        return 0;
    }
    *seq_n = data_package[1];
    int data_size = data_package[2] * 256 + data_package[3];

    unsigned char * result_data = (unsigned char *) malloc(data_size * sizeof(unsigned char));


    //memcpy(&result_data, data_package + 4, data_size);
    //memcpy(data, data_package + 4, data_size);
    for(int i = 0; i < data_size; i++){
        result_data[i] = data_package[i + 4];
    }

    *data = result_data;
    return data_size;
}

int make_control_package(int start, int file_size, unsigned char * file_name, unsigned char ** control_package){
    unsigned char * result_package = (unsigned char *) malloc(MAX_DATA_SIZE * sizeof(unsigned char)); //MAX_DATA_SIZE?????
    if(start){
        result_package[0] = CP_START;
    }
    else{
        result_package[0] = CP_END;
    }

    result_package[1] = CP_T_FSIZE;
    char * size_string = (char*)malloc(sizeof(int)); 
    sprintf(size_string, "%d", file_size);
    result_package[2] = strlen(size_string);
    
    //memcpy(&result_package[3], size_string, result_package[2]);
    //memcpy(control_package[3], size_string, result_package[2]);
    for(int i = 0; i < strlen(size_string); i++){
        result_package[i + 3] = size_string[i];
    }

    int index = 3 + strlen(size_string);

    result_package[index++] = CP_T_FNAME;
    result_package[index++] = strlen(file_name);

    //memcpy(&result_package[++index], file_name, strlen(file_name));
    //memcpy(control_package[++index], file_name, strlen(file_name));
    for(int i = 0; i < strlen(file_name); i++){
        result_package[index + i] = size_string[i];
    }

    *control_package = result_package;
    return index + strlen(file_name);
}

int read_control_package(unsigned char * control_package, int package_size, int * file_size, unsigned char ** file_name){
    int fname_len = -1;
    for(int i = 0; i < package_size; i++){
        if(control_package[i] == CP_T_FSIZE){
            int fsize_len = control_package[++i];
            unsigned char * fsize_str = (unsigned char *) malloc(fsize_len * sizeof(unsigned char));

            //memcpy(fsize_str, &control_package[++i], fsize_len);
            //i += fsize_len;
            for(int j = 0; j < fsize_len; j++){
                fsize_str[j] = control_package[++i];
            }
            free(fsize_str);
        }

        if(control_package[i] == CP_T_FNAME){
            fname_len = control_package[++i];
            unsigned char * fname_str = (unsigned char *) malloc(fname_len * sizeof(unsigned char));

            //memcpy(fname_str, &control_package[++i], fname_len);
            //i += fname_len;
            for(int j = 0; j < fname_len; j++){
                fname_str[j] = control_package[++i];
            }
            free(fname_str);
        }
    }

    return fname_len;
}

