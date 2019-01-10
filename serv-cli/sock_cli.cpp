#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <endian.h>
#include <poll.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <iostream>
#include <utility>
#include "readline.hpp"

#define ERR_EXIT(m) \
    do { \
        perror(m); \
        exit(EXIT_FAILURE); \
    } while(0)

void str_cli(FILE* fp, int sockfd) {
    char recvbuf[1024] = {0};
    char sendbuf[1024] = {0};
    fd_set rset;
    int maxfd;
    int n;
    FD_ZERO(&rset);
    for ( ; ; ) {
        FD_SET(fileno(fp), &rset);
        FD_SET(sockfd, &rset);
        maxfd = std::max(sockfd, fileno(fp)) + 1;
        int ret = select(maxfd, &rset, NULL, NULL, NULL);
        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            } else {
                ERR_EXIT("select");
            }
        }

        if (FD_ISSET(sockfd, &rset)) {
            if ( (n = readline(sockfd, recvbuf, sizeof(recvbuf))) == -1 ) {
                ERR_EXIT("readline");
            } else if (n == 0) {
                printf("server close\n");
                break;
            }
            fputs(recvbuf, stdout);
            memset(recvbuf, 0, sizeof(recvbuf));
        }

        if (FD_ISSET(fileno(fp), &rset)) {
            if (fgets(sendbuf, sizeof(sendbuf), stdin) == NULL) {
                break;
            }
            writen(sockfd, sendbuf, strlen(sendbuf));
            memset(sendbuf, 0, sizeof(sendbuf));
        }
    }
    close(sockfd);
}

int main() {
    int sock;
    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        ERR_EXIT("socket");
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htobe16(5100);
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        ERR_EXIT("connect");
    }
    struct sockaddr_in localaddr;
    socklen_t addrlen = sizeof(localaddr);
    if (getsockname(sock, (struct sockaddr*)&localaddr, &addrlen) < 0) {
        ERR_EXIT("getsockname");
    }
    std::cout << "ip=" << inet_ntoa(localaddr.sin_addr) << "port =" << ntohl(localaddr.sin_port) << std::endl;

    str_cli(stdin, sock);
    exit(0);
}
