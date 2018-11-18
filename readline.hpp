#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <error.h>
#include <errno.h>

#define ERR_EXIT(m) \
    do { \
        perror(m); \
        exit(EXIT_FAILURE); \
    } while(0)

ssize_t readn(int fd, void *buf, size_t count);
ssize_t recv_peek(int sockfd, void *buf, size_t len);
ssize_t readline(int fd, void* buf, size_t maxlen);

ssize_t writen(int fd, void *buf, size_t count) {
    size_t nleft = count;
    ssize_t nwritten;
    char* bufp = (char*)buf;

    while (nleft > 0) {
        if ((nwritten = write(fd, bufp, nleft)) < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        else if (nwritten == 0) {
            return count - nleft;
        }
        bufp += nwritten;
        nleft -= nwritten;
    }
    return count;
}

ssize_t readn(int fd, void *buf, size_t count) {
    size_t nleft = count;
    ssize_t nread;
    char* bufp = (char*)buf;
    while (nleft > 0) {
        if ( (nread = read(fd, bufp, nleft)) < 0) {
            if (errno == EINTR){
                continue;
            }
            return -1;
        }
        else if (nread == 0) {
            return count - nleft;
        }
        nleft -= nread;
        bufp += nread;
    }
    return count;
}

//recv函数是用来对sock进行读操作的函数，若为阻塞模式:
//返回读到的字节数，错误返回－１，返回０表示对方关闭连接
ssize_t recv_peek(int sockfd, void *buf, size_t len) {
    while(1) {
        int ret = recv(sockfd, buf, len, MSG_PEEK);
        if (ret == -1 && errno == EINTR) {
            continue;
        }
        return ret;
    }
}

ssize_t readline(int fd, void *buf, size_t maxlen) {
    int ret, nread;
    char* bufp = (char*)buf;
    int nleft = maxlen;
    while(1) {
        ret = recv_peek(fd, bufp, nleft);
        if (ret < 0) {
            return ret;
        }
        else if (ret == 0) {
            return 0;
        }
        nread = ret;
        int i;
        for (i = 0; i < nread; i++) {
            if (bufp[i] == '\n') {
                ret = readn(fd, bufp, i + 1);
                if (ret != i + 1) {
                    exit(EXIT_FAILURE);
                }
                return ret;
            }
        }

        if (nread > nleft) {
            exit(EXIT_FAILURE);
        }
        ret = readn(fd, bufp, nread);
        if (ret != nread) {
            exit(EXIT_FAILURE);
        }
        nleft -= nread;
        bufp += nread;
    }
    return -1;
}

//ret : 0代表未超时,-1且errno == ETIMEDOUT代表超时, -1出错
int read_timeout(int fd, unsigned int wait_seconds) {
    int ret = -1;
        if (wait_seconds > 0) {
        fd_set read_fdset;
        struct timeval timeout;

        FD_ZERO(&read_fdset);
        FD_SET(fd, &read_fdset);
        timeout.tv_sec = wait_seconds;
        timeout.tv_usec = 0;
        do {
            ret = select(fd + 1, &read_fdset, NULL, NULL, &timeout);
        } while(ret < 0 && errno == EINTR);

        if (ret == 0) {
            ret = -1;
            errno = ETIMEDOUT;
        }
        else if (ret == 1) {
            ret = 0;
        }
    }
    return ret;
}

int write_timeout(int fd, unsigned int wait_seconds) {
    int ret;
    if (wait_seconds > 0) {
        fd_set write_fdset;
        struct timeval timeout;

        FD_ZERO(&write_fdset);
        FD_SET(fd, &write_fdset);

        timeout.tv_sec = wait_seconds;
        timeout.tv_usec = 0;

        do {
            ret = select(fd + 1, &write_fdset, NULL, NULL, &timeout);
        } while (ret < 0 && errno == EINTR);

        if (ret == 0) {
            ret = -1;
            errno = ETIMEDOUT;
        }
        else if (ret == 1) {
            ret = 0;
        }
    }
    return ret;
}

int accept_timeout(int fd, struct sockaddr_in *addr, unsigned int wait_seconds) {
    int ret;
    socklen_t addrlen = sizeof(*addr);

    if (wait_seconds > 0) {
        fd_set accept_fdset;
        struct timeval timeout;
        FD_ZERO(&accept_fdset);
        FD_SET(fd, &accept_fdset);
        timeout.tv_sec = wait_seconds;
        timeout.tv_usec = 0;
        do {
            ret = select(fd + 1, &accept_fdset, NULL, NULL, &timeout);
        } while (ret < 0 && errno == EINTR);
        if (ret == -1) {
            return -1;
        }
        else if (ret == 0) {
            errno = ETIMEDOUT;
            return -1;
        }
    }
    if (addr != NULL) {
        ret = accept(fd, (struct sockaddr*) addr, &addrlen);
    }
    else {
        ret = accept(fd, NULL, NULL);
    }

    if (ret == -1) {
        ERR_EXIT("accept");
    }
    return ret;
}

void activate_nonblock(int fd) {
    int ret;
    int flags = fcntl(fd, F_GETFL);
    if (flags == -1) {
        ERR_EXIT("fcntl");
    }
    flags |= O_NONBLOCK;
    ret = fcntl(fd, F_SETFL);
    if (ret == -1) {
        ERR_EXIT("fcntl");
    }
}

void deactivate_nonblock(int fd) {
    int ret;
    int flags = fcntl(fd, F_GETFL);
    if (flags == -1) {
        ERR_EXIT("fcntl");
    }
    flags &= ~O_NONBLOCK;
    ret = fcntl(fd, F_SETFL, flags);
    if (ret == -1) {
        ERR_EXIT("fcntl");
    }
}

int connect_timeout(int fd, struct sockaddr_in *addr, unsigned int wait_seconds) {
    int ret;
    socklen_t addrlen = sizeof(*addr);
    if (wait_seconds > 0) {
        activate_nonblock(fd);
    }
    ret = connect(fd, (struct sockaddr*) addr, addrlen);
    if (ret < 0 && errno == EINPROGRESS) {
        fd_set connect_fdset;
        struct timeval timeout;
        FD_ZERO(&connect_fdset);
        FD_SET(fd, &connect_fdset);
        timeout.tv_sec = wait_seconds;
        timeout.tv_usec = 0;
        do {
            ret = select(fd + 1, NULL, &connect_fdset, NULL, &timeout);
        } while(ret < 0 && errno == EINTR);
        if (ret == 0) {
            ret = -1;
            errno = ETIMEDOUT;
        }
        else if (ret < 0) {
            return -1;
        }
        else if (ret == 1) { //两种情况:1.连接建立成功，　２．套接字产生错误,此时信息不会保存在errno中，需要用getsockopt获取
            int err;
            socklen_t socklen = sizeof(*addr);
            int sockoptret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &socklen);
            if (sockoptret == -1) {
                return -1;
            }
            if (err == 0) {
                ret = 0;
            }
            else {
                errno = err;
                ret = -1;
            }
        }
    }
    if (wait_seconds > 0) {
        deactivate_nonblock(fd);
    }
    return ret;
}
