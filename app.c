#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "./macros.h"
#include "./app.h"

int make_data_package(int seq_n, unsigned char *data, int size, unsigned char **data_package)
{
    unsigned char *result_package = (unsigned char *)malloc((size + 4) * sizeof(unsigned char));
    result_package[0] = DP;
    result_package[1] = seq_n % 256;
    result_package[2] = size / 256;
    result_package[3] = size % 256;

    //memcpy(&result_package[4], data, size);
    //memcpy(data_package[4], data, size);
    for (int i = 0; i < size; i++)
    {
        result_package[i + 4] = data[i];
    }

    *data_package = result_package;
    return size + 4;
}

int read_data_package(unsigned char *data_package, int *seq_n, unsigned char **data)
{
    if (data_package[0] != DP)
    {
        printf("Not a data package inside read_data_package!\n");
        return -1;
    }
    *seq_n = data_package[1];
    int data_size = data_package[2] * 256 + data_package[3];

    unsigned char *result_data = (unsigned char *)malloc(data_size * sizeof(unsigned char));

    //memcpy(&result_data, data_package + 4, data_size);
    //memcpy(data, data_package + 4, data_size);
    for (int i = 0; i < data_size; i++)
    {
        result_data[i] = data_package[i + 4];
    }

    *data = result_data;
    return data_size;
}

int make_control_package(int start, int file_size, char *file_name, unsigned char **control_package)
{
    unsigned char *result_package = (unsigned char *)malloc(MAX_PACKET_SIZE * sizeof(unsigned char));
    if (start)
    {
        result_package[0] = CP_START;
    }
    else
    {
        result_package[0] = CP_END;
    }

    result_package[1] = CP_T_FSIZE;

    /*
    char *size_string = (char *) malloc(sizeof(int));
    result_package[2] = strlen(size_string);
    */
    result_package[2] = sizeof(int);

    //memcpy(&result_package[3], size_string, result_package[2]);
    //memcpy(control_package[3], size_string, result_package[2]);
    /*
    for (int i = 0; i < strlen(size_string); i++)
    {
        result_package[i + 3] = size_string[i];
    }
    */
    memcpy(&result_package[3], &file_size, sizeof(int));

    //int index = 3 + strlen(size_string);
    int index = 3 + sizeof(int);

    result_package[index++] = CP_T_FNAME;
    result_package[index++] = strlen(file_name) + 1;

    //memcpy(&result_package[index], file_name, strlen(file_name));
    //memcpy(control_package[++index], file_name, strlen(file_name));
    for (int i = 0; i < strlen(file_name) + 1; i++)
    {
        result_package[index + i] = file_name[i];
    }
    //memcpy(&result_package[index], file_name, strlen(file_name));

    *control_package = result_package;
    return index + strlen(file_name) + 1;
}

int read_control_package(unsigned char *control_package, int package_size, int *file_size, char **file_name)
{
    if (control_package[0] != CP_START && control_package[0] != CP_END)
    {
        printf("Not a control package inside read_control_package!\n");
        return -1;
    }

    int fname_len = 0;
    int fsize_len = 0;
    for (int i = 0; i < package_size; i++)
    {
        if (control_package[i] == CP_T_FSIZE)
        {
            fsize_len = control_package[++i];

            memcpy(file_size, &control_package[++i], fsize_len);
            i += fsize_len;
        }

        if (control_package[i] == CP_T_FNAME)
        {
            fname_len = control_package[++i];
            *file_name = (char *)malloc(fname_len * sizeof(char));

            memcpy(*file_name, &control_package[++i], fname_len);
            i += fname_len;
        }
    }

    return 5 + fsize_len + fname_len;
}
