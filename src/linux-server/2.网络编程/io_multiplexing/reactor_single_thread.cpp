#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

const int BUFFER_LENGTH = 1024;
const int EVENT_LENGTH = 1024;
const int BLOCK_LENGTH = 1024;

typedef int (*CALLBACK)(int fd, int events, void* arg);

struct connect_t
{
    int      fd;
    CALLBACK cb;
    int      count;
    char     rbuffer[BUFFER_LENGTH];
    int      rc;
    char     wbuffer[BUFFER_LENGTH];
    int      wc;   // 读取的字节数
};

struct connblock_t
{
    connect_t*          block;
    struct connblock_t* next;
};

struct reactor_t
{
    int          epfd;
    int          block_count;
    connblock_t* block_header;
};

int new_block(reactor_t* reactor) {
    if (!reactor) return -1;
    connblock_t* block = reactor->block_header;
    while (block->next != NULL) {
        block = block->next;
    }
    connblock_t* new_block = (connblock_t*)malloc(sizeof(connblock_t) + BLOCK_LENGTH * sizeof(connect_t));
    if (new_block == NULL) return -1;
    new_block->block = (connect_t*)(new_block + 1);
    new_block->next = NULL;
    block->next = new_block;

    reactor->block_count++;
    return 0;
}

connect_t* connect_idx(reactor_t* reactor, int fd) {
    if (!reactor) return NULL;

    int block_idx = fd / BLOCK_LENGTH;

    while (block_idx >= reactor->block_count) {
        new_block(reactor);
    }

    int          i = 0;
    connblock_t* block = reactor->block_header;
    while (i++ < block_idx) {
        block = block->next;
    }
    return &block->block[fd % BLOCK_LENGTH];
}

int init_reactor(reactor_t* reactor) {
    if (!reactor) return -1;
#if 0 
    reactor->block_header = (connblock_t*)malloc(sizeof(connblock_t));
    if (reactor->block_header == NULL) return -1;
    reactor->block_header->blobk = (connect_t*)calloc(BLOBK_LENGTH, sizeof(connblock_t));
    if (reactor->block_header->blobk == NULL) return -1;
#else
    reactor->block_header = (connblock_t*)malloc(sizeof(connblock_t) + BLOCK_LENGTH * sizeof(connect_t));
    if (reactor->block_header == NULL) return -1;
    reactor->block_header->block = (connect_t*)(reactor->block_header + 1);
#endif
    reactor->block_count = 1;
    reactor->block_header->next = NULL;
    reactor->epfd = epoll_create(1);
    return 0;
}

void destroy_reactor(reactor_t* reactor) {
    if (!reactor) return;
    if (!reactor->block_header) free(reactor->block_header);
    close(reactor->epfd);
}

int recv_cb(int fd, int event, void* arg);

int init_server(short port) {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    int          opt = 1;
    unsigned int optlen = sizeof(opt);
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, optlen);

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);   // 0.0.0.0
    server_addr.sin_port = htons(port);

    if (-1 == bind(listen_fd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr))) {
        printf("bind failed: %s", strerror(errno));
        return -1;
    }

    listen(listen_fd, 10);
    return listen_fd;
}

int send_cb(int fd, int event, void* arg) {
    reactor_t* reactor = (reactor_t*)arg;
    connect_t* conn = connect_idx(reactor, fd);

    int ret = send(fd, conn->wbuffer, conn->wc, 0);

    conn->cb = recv_cb;
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = fd;
    epoll_ctl(reactor->epfd, EPOLL_CTL_MOD, fd, &ev);

    return 0;
}

int recv_cb(int fd, int event, void* arg) {
    reactor_t* reactor = (reactor_t*)arg;
    connect_t* conn = connect_idx(reactor, fd);

    int ret = recv(fd, conn->rbuffer, conn->count, 0);
    if (ret < 0) {}
    else if (ret == 0) {   // 连接断开
        conn->fd = -1;
        conn->rc = 0;
        conn->wc = 0;
        epoll_ctl(reactor->epfd, EPOLL_CTL_DEL, fd, NULL);
        printf("client(connect_fd=%d) disconnect.\n", fd);
        close(fd);
        return -1;
    }
    conn->rc += ret;
    memcpy(conn->wbuffer, conn->rbuffer, conn->rc);
    conn->wc = conn->rc;
#if 1
    conn->cb = send_cb;

    struct epoll_event ev;
    ev.events = EPOLLOUT;
    ev.data.fd = fd;
    epoll_ctl(reactor->epfd, EPOLL_CTL_MOD, fd, &ev);
#else
    conn->count = 20;
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = fd;
    epoll_ctl(reactor->epfd, EPOLL_CTL_MOD, fd, &ev);
#endif

    return 0;
}

int accept_cb(int fd, int events, void* arg) {
    struct sockaddr_in client_addr;
    socklen_t          len = sizeof(client_addr);
    int                client_fd = accept(fd, (struct sockaddr*)&client_addr, &len);
    if (client_fd <= 0) {
        printf("accept error: %s\n", strerror(errno));
        return -1;
    }
    printf("accept:%d\n", client_fd);

    reactor_t* reactor = (reactor_t*)arg;
    connect_t* conn = connect_idx(reactor, client_fd);
    conn->fd = client_fd;
    conn->cb = recv_cb;
    conn->count = BUFFER_LENGTH;

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = client_fd;
    epoll_ctl(reactor->epfd, EPOLL_CTL_ADD, client_fd, &ev);
    return 0;
}

int set_listener(reactor_t* reactor, int listen_fd, CALLBACK cb) {
    if (!reactor || !reactor->block_header) return -1;
    reactor->block_header->block[listen_fd].fd = listen_fd;
    reactor->block_header->block[listen_fd].cb = cb;

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = listen_fd;
    epoll_ctl(reactor->epfd, EPOLL_CTL_ADD, listen_fd, &ev);

    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Using: ./reactor port\n");
        return -1;
    }

    int port = atoi(argv[1]);
    int listen_fd = init_server(port);
    if (listen_fd < 0) {
        printf("init server failed.\n");
    }

    reactor_t reactor;
    if (init_reactor(&reactor) != 0) {
        printf("reactor init failed.\n");
    }

    if (set_listener(&reactor, listen_fd, accept_cb) < 0) {
        printf("set listener failed.\n");
    }

    struct epoll_event events[EVENT_LENGTH] = {0};
    while (1) {
        int nready = epoll_wait(reactor.epfd, events, 1024, -1);
        for (int i = 0; i < nready; i++) {
            int        connect_fd = events[i].data.fd;
            connect_t* conn = connect_idx(&reactor, connect_fd);
            if (events[i].events & EPOLLIN) {
                conn->cb(connect_fd, events[i].events, &reactor);
            }
            if (events[i].events & EPOLLOUT) {
                conn->cb(connect_fd, events[i].events, &reactor);
            }
        }
    }
}