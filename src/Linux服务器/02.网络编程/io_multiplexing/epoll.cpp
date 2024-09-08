#include "common.h"
#include <fcntl.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <unistd.h>

const int BUFFER_LEN = 5;
const int POLL_SIZE = 1024;

void* client_thread(void* arg) {
    int client_fd = *(int*)arg;
    while (1) {
        char buffer[BUFFER_LEN] = {0};
        int  ret = recv(client_fd, buffer, BUFFER_LEN, 0);
        if (ret == 0) {
            close(client_fd);
            break;
        }
        printf("ret=%d, buffer=%s\n", ret, buffer);
        send(client_fd, buffer, ret, 0);
    }
    return NULL;
}

int main() {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    int          opt = 1;
    unsigned int optlen = sizeof(opt);
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, optlen);

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);   // 0.0.0.0
    server_addr.sin_port = htons(5005);

    if (-1 == bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr))) {
        printf("bind failed: %s", strerror(errno));
        return -1;
    }

    listen(listen_fd, 10);

    // 事件集合
    struct pollfd fds[POLL_SIZE] = {0};
    fds[listen_fd].fd = listen_fd;
    fds[listen_fd].events = POLLIN;

    int                epfd = epoll_create(1);
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = listen_fd;

    epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev);

    struct epoll_event events[1024] = {0};

    while (1) {
        int nready = epoll_wait(epfd, events, 1024, -1);

        if (nready < 0) continue;
        for (int i = 0; i < nready; i++) {
            int connect_fd = events[i].data.fd;
            // 监听事件
            if (listen_fd == connect_fd) {
                struct sockaddr_in client_addr;
                socklen_t          len = sizeof(client_addr);
                int                client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &len);
                if (client_fd <= 0) continue;
                printf("accept:%d\n", client_fd);
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = client_fd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev);
            }
            else if (events[i].events & EPOLLIN) {
                char buffer[BUFFER_LEN] = {0};
                int  ret = recv(connect_fd, buffer, BUFFER_LEN, 0);
                if (ret == 0) {   // 连接断开
                    epoll_ctl(epfd, EPOLL_CTL_DEL, connect_fd, NULL);
                    printf("client(connect_fd=%d) disconnect.\n", connect_fd);
                    close(connect_fd);   // 关闭socket
                }
                printf("connect_fd=%d, buffer=%s\n", connect_fd, buffer);
                send(connect_fd, buffer, ret, 0);
            }
        }
    }
}