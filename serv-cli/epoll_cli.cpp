#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <endian.h>
#include <poll.h>
#include <sys/epoll.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <iostream>
#include <utility>
#include <vector>
using namespace std;
#include "readline.hpp"

#define MAX_EVENT 1000

void echo_cli(FILE* fp, int sockfd) {
    char recvbuf[1024] = {0};
    char sendbuf[1024] = {0};

    int epollfd = epoll_create1(0);

    struct epoll_event  ev0;
    ev0.events = POLLIN;
    ev0.data.fd = sockfd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev0);

    struct epoll_event ev1;
    ev1.events = POLLIN;
    ev1.data.fd = fileno(fp);
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fileno(fp), &ev1);

    vector<struct epoll_event> revents;
    revents.resize(MAX_EVENT);
    for (;;) {
        int rn = epoll_wait(epollfd, &*revents.begin(), MAX_EVENT, -1);
        if (rn == 0) continue;
        if (rn < 0) ERR_EXIT("epoll wait");
        for (int i = 0; i < rn; i++) {
            if (revents[i].data.fd == fileno(fp)) {
                if (fgets(sendbuf, sizeof sendbuf, stdin) == NULL) {
                    break;
                }
                writen(sockfd, sendbuf, strlen(sendbuf));
                bzero(sendbuf, sizeof sendbuf);
            }
            if (revents[i].data.fd == sockfd) {
                if (revents[i].events & POLLIN) {
                    std::cout << "POLLIN" << std::endl;
                }
                if (revents[i].events & POLLHUP) {
                    std::cout << "POLLHUP" << std::endl;
                }
                int n;
                if ( (n = readline(sockfd, recvbuf, sizeof recvbuf)) == 0) {
                    printf("server close\n");
                    close(sockfd);
                    exit(0);
                }
                fputs(recvbuf, stdout);
                bzero(recvbuf, sizeof recvbuf);
            }
        }
    }
    close(sockfd);
}

int main()
{
    int sock;
    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        ERR_EXIT("SOCKET");
    }

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof servaddr);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htobe16(5100);
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr*)&servaddr, sizeof servaddr) < 0) {
        ERR_EXIT("connect");
    }

    struct sockaddr_in localaddr;
    socklen_t addrlen = sizeof localaddr;
    if (getsockname(sock, (struct sockaddr*)&localaddr, &addrlen) < 0) {
        ERR_EXIT("getsockname");
    }
    std::cout << "ip=" << inet_ntoa(localaddr.sin_addr) << "port=" <<
        be16toh(localaddr.sin_port) << std::endl;
    echo_cli(stdin, sock);
    exit(-1);
}
