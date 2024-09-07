/**
 * 一连接一线程
 */
#include "common.h"
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>

const int BUFFER_LEN = 1024;

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

#if 0   // 非阻塞
    int flags = fcntl(listen_fd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(listen_fd, F_SETFL, flags);
#endif
    struct sockaddr_in client_addr;
    socklen_t          len = sizeof(client_addr);


    while (1) {
        int client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &len);
#if 0
        char buffer[BUFFER_LEN] = {0};
        int  ret = recv(client_fd, buffer, BUFFER_LEN, 0);
        printf("ret=%d, buffer=%s\n", ret, buffer);
        send(client_fd, buffer, ret, 0);
#else
        pthread_t threadid;
        pthread_create(&threadid, NULL, client_thread, &client_fd);
#endif
    }
}