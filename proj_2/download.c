#include <stdio.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <regex.h>
#include <error.h>
#include <errno.h>
#include <libgen.h>
#include <fcntl.h>

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
        return -1;
    }

    /*connect to the server*/
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        return -1;
    }

    return sockfd;
}

/*
 * Reads a reply from the FTP server
 *
 * reply_buffer: a pointer to store the address of the buffer containing the reply
 * reply_buffer_size: a pointer to store the allocated size of the buffer
 * stream: the socket's file stream
 * expected_code: the reply code that is expected
 * error_msg: the message to show in case the reply code is not the expected one
 *
 * returns: On success, the number of characters read,
 * including the delimiter character, but not including the terminating null byte.
 * On error, -1 is returned, and errno is set to indicate the error.
 */
int read_reply(char **reply_buffer, size_t *reply_buffer_size, FILE *stream, char *expected_code, char *error_msg)
{
    regex_t reply_regex;
    regcomp(&reply_regex, "^[0-9]{3} ", REG_EXTENDED | REG_NOSUB);

    ssize_t nread;
    nread = getline(reply_buffer, reply_buffer_size, stream);
    while (nread >= 0 && regexec(&reply_regex, *reply_buffer, 0, NULL, 0) != 0)
    {
        free(*reply_buffer);
        *reply_buffer = NULL;
        nread = getline(reply_buffer, reply_buffer_size, stream);
    }

    if (nread == -1)
    {
        error(1, errno, "error reading the server's reply");
    }

    if (strncmp(*reply_buffer, expected_code, 3) != 0)
    {
        error(1, 0, "%s. Response: %s", error_msg, *reply_buffer);
    }

    return nread;
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "Invalid number of arguments.\n\nUsage: %s ftp://[<user>:<password>@]<host>/<url-path>\n", argv[0]);
        exit(-1);
    }

    regex_t regex;

    regcomp(&regex, "^ftp://((([[:alnum:]+._-][[:alnum:]+._-]*)(:([[:alnum:]+._-]*)){0,1}){0,1}@){0,1}([[:alnum:]+._-]*)/([[:alnum:]+./_-][[:alnum:]+./_-]*)$", REG_EXTENDED);

    regmatch_t url_regmatch[8];

    if (regexec(&regex, argv[1], 8, url_regmatch, 0) != 0)
    {
        fprintf(stderr, "Invalid ftp URL.\n\nUsage: %s ftp://[<user>:<password>@]<host>/<url-path>\n", argv[0]);
        exit(-1);
    }

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
        len = url_regmatch[5].rm_eo - url_regmatch[5].rm_so;
        password = (char *)malloc((len + 1) * sizeof(char));
        memcpy(password, argv[1] + url_regmatch[5].rm_so * sizeof(char), len);
        password[len] = 0;
    }
    else
    {
        password = (char *)malloc(1 * sizeof(char));
        memcpy(password, "", 1);
    }

    len = url_regmatch[6].rm_eo - url_regmatch[6].rm_so;
    host = (char *)malloc((len + 1) * sizeof(char));
    memcpy(host, argv[1] + url_regmatch[6].rm_so * sizeof(char), len);
    host[len] = 0;

    len = url_regmatch[7].rm_eo - url_regmatch[7].rm_so;
    path = (char *)malloc((len + 1) * sizeof(char));
    memcpy(path, argv[1] + url_regmatch[7].rm_so * sizeof(char), len);
    path[len] = 0;

#ifdef DEBUG
    printf("Parsed input:\n\tUser: %s\n\tPass: %s\n\tHost: %s\n\tPath: %s\n\n", user, password, host, path);
#endif

    struct hostent *h;

    if ((h = gethostbyname(host)) == NULL)
    {
        error(1, errno, "error getting hostname");
    }
    free(host);

#ifdef DEBUG
    printf("Resolved host:\n\tHost name: %s\n\tIP Address: %s\n", h->h_name, inet_ntoa(*((struct in_addr *)h->h_addr)));
#endif

    int socket_fd = connection(inet_ntoa(*((struct in_addr *)h->h_addr)), 21);

    if (socket_fd == -1)
    {
        error(1, errno, "connection()");
    }

    char *reply = NULL;
    size_t reply_len = 0;
    FILE *fp = fdopen(socket_fd, "r");

    read_reply(&reply, &reply_len, fp, "220", "server not ready for commands");
    free(reply);
    reply = NULL;

    dprintf(socket_fd, "user %s\r\n", user);
    read_reply(&reply, &reply_len, fp, "331", "login was unsuccessful");
    free(reply);
    reply = NULL;
    free(user);

    dprintf(socket_fd, "pass %s\r\n", password);
    read_reply(&reply, &reply_len, fp, "230", "login was unsuccessful");
    free(reply);
    reply = NULL;
    free(password);

    dprintf(socket_fd, "pasv\r\n");
    read_reply(&reply, &reply_len, fp, "227", "error entering passive mode");

    regcomp(&regex, "\\(([0-9]*),([0-9]*),([0-9]*),([0-9]*),([0-9]*),([0-9]*)\\)", REG_EXTENDED);

    regmatch_t ip_regmatch[7];

    if (regexec(&regex, reply, 7, ip_regmatch, 0) != 0)
    {
        error(1, 0, "there was no match for an ip and port in 227 response. Response: %s", reply);
    }

    len = ip_regmatch[4].rm_eo - ip_regmatch[1].rm_so;
    char *ip, *port_1, *port_2;

    ip = (char *)malloc((len + 1) * sizeof(char));

    memcpy(ip, reply + ip_regmatch[1].rm_so * sizeof(char), len);
    ip[ip_regmatch[1].rm_eo - ip_regmatch[1].rm_so] = '.';
    ip[ip_regmatch[2].rm_eo - ip_regmatch[1].rm_so] = '.';
    ip[ip_regmatch[3].rm_eo - ip_regmatch[1].rm_so] = '.';
    ip[len] = 0;

    len = ip_regmatch[5].rm_eo - ip_regmatch[5].rm_so;
    port_1 = (char *)malloc((len + 1) * sizeof(char));
    memcpy(port_1, reply + ip_regmatch[5].rm_so * sizeof(char), len);
    port_1[len] = 0;
    len = ip_regmatch[6].rm_eo - ip_regmatch[6].rm_so;
    port_2 = (char *)malloc((len + 1) * sizeof(char));
    memcpy(port_2, reply + ip_regmatch[6].rm_so * sizeof(char), len);
    port_2[len] = 0;

    int port = 256 * atoi(port_1) + atoi(port_2);

    free(port_1);
    free(port_2);
    free(reply);
    reply = NULL;

#ifdef DEBUG
    printf("\tFile address: %s:%d\n\n", ip, port);
#endif

    char *filename = basename(path);

    int file_socket_fd = connection(ip, port);
    if (file_socket_fd == -1)
    {
        error(1, errno, "error connecting to file server");
    }
    free(ip);

    dprintf(socket_fd, "retr %s\r\n", path);
    read_reply(&reply, &reply_len, fp, "150", "error retrieving requested file");
    free(reply);
    reply = NULL;

    printf("Downloading to %s...\n", filename);
    int output_fd = creat(filename, 666);
    if (output_fd == -1)
    {
        error(1, errno, "error creating new file");
    }
    free(path);

    char readbuf[BUFFER_SIZE];
    size_t nread;
    while ((nread = read(file_socket_fd, readbuf, BUFFER_SIZE)) > 0)
    {
        if (write(output_fd, readbuf, nread) == -1)
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

    read_reply(&reply, &reply_len, fp, "226", "error completing transfer");
    free(reply);
    reply = NULL;
    dprintf(socket_fd, "quit\r\n");

    if (close(socket_fd) == -1)
    {
        error(1, errno, "error closing the connection to the server");
    }

    printf("Transfer completed! Exiting.\n");
    return 0;
}
