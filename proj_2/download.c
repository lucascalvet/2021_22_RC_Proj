#include <stdio.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <regex.h>

#define BUFFER_SIZE 256

/*
 * Establishes a connection to a certain IP and port, and returns the respective socket.
 *
 * server_address: the server's address
 * server_port: the server's port
 *
 * returns: On success, a file descriptor for the new socket is returned. On error, -1 is returned, and errno is set to indicate the error.
 */
int connection(char *server_address, int server_port)
{
    struct sockaddr_in server_addr;
    int sockfd;

    /*server address handling*/
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_address); /*32 bit Internet address network byte ordered*/
    server_addr.sin_port = htons(server_port);               /*server TCP port must be network byte ordered */

    /*open a TCP socket*/
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket()");
        return -1;
    }

    /*connect to the server*/
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("connect()");
        return -1;
    }

    return sockfd;
}

/*
 * Reads a reply from the FTP server
 *
 * stream: the socket's file stream
 * reply_buffer: a pointer to store the address of the buffer containing the reply
 * reply_buffer_size: a pointer to store the allocated size of the buffer
 *
 * returns: On success, the number of characters read,
 * including the delimiter character, but not including the terminating null byte.
 * On error, -1 is returned, and errno is set to indicate the error.
 */
int read_reply(FILE *stream, char **reply_buffer, int *reply_buffer_size)
{
    regex_t reply_regex;
    regcomp(&reply_regex, "^[:digit:]{3}[:blank:]", REG_EXTENDED);

    ssize_t nread;
    do
    {
        free(*reply_buffer);
        nread = getline(reply_buffer, reply_buffer_size, stream);
    } while (nread >= 0 && regexec(&reply_regex, reply_buffer, 0, NULL, 0) != 0);

    return nread;
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "INVALID NUMBER OF ARGUMENTS\nUsage: %s ftp://[<user>:<password>@]<host>/<url-path>\n", argv[0]);
        exit(-1);
    }

    regex_t url_regex;

    regcomp(&url_regex, "^ftp://((([[:alnum:]+.-][[:alnum:]+.-]*)(:([[:alnum:]+.-]*)){0,1}){0,1}@){0,1}([[:alnum:]+.-]*)/([[:alnum:]+./-]*)$", REG_EXTENDED);

    regmatch_t url_regmatch[8];

    if (regexec(&url_regex, argv[1], 8, url_regmatch, 0) != 0)
    {
        fprintf(stderr, "NO MATCH\nUsage: %s ftp://[<user>:<password>@]<host>/<url-path>\n", argv[0]);
        exit(-1);
    }

    int no_password = 1;
    char *user, *password, *host, *path;
    int len;

    /* Check if the login details were not given */
    if (url_regmatch[1].rm_so == -1)
    {
        user = (char *)malloc(10 * sizeof(char));
        memcpy(user, "anonymous", 10);

        password = (char *)malloc(1 * sizeof(char));
        memcpy(password, "", 1);
    }
    /* Check if the user is not empty  */
    if (url_regmatch[3].rm_so != -1)
    {
        len = url_regmatch[3].rm_eo - url_regmatch[3].rm_so;
        user = (char *)malloc((len + 1) * sizeof(char));
        memcpy(user, argv[1] + url_regmatch[3].rm_so * sizeof(char), len);
        user[len] = 0;
    }

    /* Check if the password is present */
    if (url_regmatch[4].rm_so != -1)
    {
        no_password = 0;

        len = url_regmatch[5].rm_eo - url_regmatch[5].rm_so;
        password = (char *)malloc((len + 1) * sizeof(char));
        memcpy(password, argv[1] + url_regmatch[5].rm_so * sizeof(char), len);
        password[len] = 0;
    }

    len = url_regmatch[6].rm_eo - url_regmatch[6].rm_so;
    host = (char *)malloc((len + 1) * sizeof(char));
    memcpy(host, argv[1] + url_regmatch[6].rm_so * sizeof(char), len);
    host[len] = 0;

    len = url_regmatch[7].rm_eo - url_regmatch[7].rm_so;
    path = (char *)malloc((len + 1) * sizeof(char));
    memcpy(path, argv[1] + url_regmatch[7].rm_so * sizeof(char), len);
    path[len] = 0;

    printf("User: %s\nPassword: %s\nHost: %s\nPath: %s\n", user, password, host, path);

    int port = 21;
    struct hostent *h;

    if ((h = gethostbyname(host)) == NULL)
    {
        error(1, errno, "error getting hostname");
    }

    printf("Host name  : %s\n", h->h_name);
    printf("IP Address : %s\n", inet_ntoa(*((struct in_addr *)h->h_addr)));

    int socket_fd = connection(inet_ntoa(*((struct in_addr *)h->h_addr)), port);

    if (socket_fd == -1)
    {
        perror("connection()");
        exit(-1);
    }

    char *reply = NULL;
    size_t reply_len = 0;
    FILE *fp = fdopen(socket_fd, "r");

    if (read_reply(fp, &reply, &reply_len) == -1) {
        error(1, errno, "error reading the server's reply");
    }

    if (reply[0] == '2')
    {
        /*
        Write username
        */
    }
    else
    {
        error(1, 0, "server not ready for commands. Response: %s", reply);
    }

    /* Get 331, then write password */

    /* Get 227 and parse the returned ip */

    free(reply);
    reply = NULL;

    regex_t ip_regex;
    regcomp(&ip_regex, "\(([:digit:]*),([:digit:]*),([:digit:]*),([:digit:]*),([:digit:]*),([:digit:]*)\)", REG_EXTENDED);

    regmatch_t ip_regmatch[7];

    char *ip_string;

    if (regexec(&ip_regex, ip_string, 7, ip_regmatch, 0) != 0)
    {
        fprintf(stderr, "NO MATCH\n");
        exit(-1);
    }

    len = url_regmatch[3].rm_eo - url_regmatch[3].rm_so;
    user = (char *)malloc((len + 1) * sizeof(char));
    memcpy(user, argv[1] + url_regmatch[3].rm_so * sizeof(char), len);

    int temp_len, total_len = 4;
    char *temp_ip, *ip, *port_1, *port_2;

    for (int i = 0; i < 4; i++)
    {
        total_len += url_regmatch[i + 1].rm_eo - url_regmatch[i + 1].rm_so;
    }

    ip = (char *)malloc(total_len * sizeof(char));

    for (int i = 0; i < 4; i++)
    {
        temp_len = url_regmatch[i + 1].rm_eo - url_regmatch[i + 1].rm_so;
        temp_ip = (char *)malloc((temp_len + 1) * sizeof(char));
        memcpy(temp_ip, ip_string + url_regmatch[i + 1].rm_so * sizeof(char), temp_len);
        strcat(ip, temp_ip);
        if (i < 3)
        {
            strcat(ip, ".");
        }
    }
    free(temp_ip);

    temp_len = url_regmatch[5].rm_eo - url_regmatch[5].rm_so;
    port_1 = (char *)malloc((temp_len + 1) * sizeof(char));
    memcpy(port_1, ip_string + url_regmatch[5].rm_so * sizeof(char), temp_len);
    temp_len = url_regmatch[6].rm_eo - url_regmatch[6].rm_so;
    port_2 = (char *)malloc((temp_len + 1) * sizeof(char));
    memcpy(port_2, ip_string + url_regmatch[6].rm_so * sizeof(char), temp_len);

    int given_port = 256 * atoi(port_1) + atoi(port_2);

    free(port_1);
    free(port_2);

    char readbuf[BUFFER_SIZE];
    char *filename = basename(path);

    int file_socket_fd = connection(ip, given_port);
    if (file_socket_fd == -1)
    {
        error(1, errno, "error connecting to file server");
    }

    int output_fd = creat(filename, 666);
    if (output_fd == -1)
    {
        error(1, errno, "error creating new file");
    }

    ssize_t nread;
    while ((nread = read(file_socket_fd, readbuf, BUFFER_SIZE)) > 0)
    {
        if (write(output_fd, readbuf, BUFFER_SIZE) == -1)
        {
            error(1, errno, "error writing to the new file");
        }
    }

    if (nread == -1)
    {
        error(1, errno, "error reading the file from the server");
    }

    if (close(output_fd) == -1)
    {
        error(1, errno, "error closing the output file");
    }

    if (close(file_socket_fd) == -1)
    {
        error(1, errno, "error closing the connection to the file server");
    }

    if (close(socket_fd) == -1)
    {
        error(1, errno, "error closing the connection to the server");
    }

    free(user);
    free(password);
    free(host);
    free(path);
    free(ip);

    printf("Success!\n");
    return 0;
}

//fscanf(%d%s, &numb, null)