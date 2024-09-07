#include "common.h"
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>

const int BUFFER_LEN = 1024;
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

    int maxfd = listen_fd;
    while (1) {
        // fd_set rset = rfds;  // 临时集合，传出事件
        int nready = poll(fds, maxfd+1, -1);
        // 监听事件
        if (fds[listen_fd].revents & POLLIN) {
            struct sockaddr_in client_addr;
            socklen_t          len = sizeof(client_addr);
            int client_fd = accept(listen_fd, (struct sockaddr*)&server_addr, &len);
            printf("accept:%d\n", client_fd);
            fds[client_fd].fd = client_fd;
            fds[client_fd].events = POLLIN;
            if (maxfd < client_fd) maxfd = client_fd;
            if (--nready == 0) continue;
        }
        // 读写事件
        for (int eventfd = listen_fd + 1; eventfd <= maxfd; eventfd++) {
            if (fds[eventfd].revents & POLLIN) {
                char buffer[BUFFER_LEN] = {0};
                int  ret = recv(eventfd, buffer, BUFFER_LEN, 0);
                if (ret == 0) {
                    fds[eventfd].fd = -1;
                    fds[eventfd].events = 0;
                    printf("client(eventfd=%d) disconnect.\n", eventfd);
                    close(eventfd);           // 关闭socket
                    
                    // 如果删除的是最后一个socket，重新计算maxfd的值
                    if (eventfd == maxfd) {
                        for (int i = maxfd; i > 0; i--) {
                            if (fds[i].fd != -1) maxfd = i;
                            break;
                        }
                    }
                }
                printf("eventfd=%d, buffer=%s\n", eventfd, buffer);
                send(eventfd, buffer, ret, 0);
            }
        }
    }
}