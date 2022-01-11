#include <stdio.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <regex.h>

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

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "INVALID NUMBER OF ARGUMENTS\nUsage: %s ftp://[<user>:<password>@]<host>/<url-path>\n", argv[0]);
        exit(-1);
    }

    regex_t url_regex;

    if (regcomp(&url_regex, "^ftp://((([[:alnum:]+.-][[:alnum:]+.-]*)(:([[:alnum:]+.-]*)){0,1}){0,1}@){0,1}([[:alnum:]+.-]*)/([[:alnum:]+./-]*)$", REG_EXTENDED) != 0)
    {
        fprintf(stderr, "BAD EXPRESSION\nUsage: %s ftp://[<user>:<password>@]<host>/<url-path>\n", argv[0]);
        exit(-1);
    }

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
        perror("gethostbyname()");
        exit(-1);
    }

    printf("Host name  : %s\n", h->h_name);
    printf("IP Address : %s\n", inet_ntoa(*((struct in_addr *)h->h_addr)));

    int socket_fd = connection(inet_ntoa(*((struct in_addr *)h->h_addr)), port);

    if (socket_fd == -1)
    {
        perror("connection()");
        exit(-1);
    }

    int bytes;
    char buf[4];
    bytes = read(socket_fd, buf, 3);
    buf[bytes] = 0;
    printf("Bytes: %d, Read: %s\n", bytes, buf);

    if (close(socket_fd) < 0)
    {
        perror("close()");
        exit(-1);
    }

    free(user);
    free(password);
    free(host);
    free(path);

    printf("Success!\n");
    return 0;
}



//fscanf(%d%s, &numb, null)