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

    reactor->block_header = (connblock_t*)malloc(sizeof(connblock_t) + BLOCK_LENGTH * sizeof(connect_t));
    if (reactor->block_header == NULL) return -1;
    reactor->block_header->block = (connect_t*)(reactor->block_header + 1);

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

int read_line(char* allbuf, int idx, char* linebuff) {
    int len = strlen(allbuf);
    for (; idx < len; idx++) {
        if (allbuf[idx] == '\r' && allbuf[idx + 1] == '\n') {
            return idx + 2;
        }
        else {
            *(linebuff++) = allbuf[idx];
        }
    }
    return -1;
}

int http_response(connect_t* conn) {
    sprintf(conn->wbuffer, "HTTP/1.1 200 OK ");
    int len = sprintf(conn->wbuffer,
                      "HTTP/1.1 200 OK\r\n"
                      "Accept-Ranges: bytes\r\n"
                      "Content-Length: 71\r\n"
                      "Content-Type: text/html\r\n"
                      "Date: Sat, 06 Aug 2022 13:16:46 GMT\r\n\r\n"
                      "<html><head><title>test</title></head><body><h1>CyYu</h1><body/></html>");

    conn->wc = len;

    return 0;
}

int http_request(connect_t* conn){
    printf("\nhttp request:\n\n%s\n", conn->rbuffer);
    char line_buffer[1024] = {0};
    int idx = read_line(conn->rbuffer, 0, line_buffer);

    if(strstr(line_buffer, "GET")){
        int i=0;
        while(line_buffer[sizeof("GET ") + i] != ' ') i++;
        line_buffer[sizeof("GET ") + i] = '\0';
        printf("%s\n",line_buffer + 4);
    }   

// GET / HTTP/1.1
// Host: 47.92.88.51:5005
// User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/128.0.0.0 Safari/537.36
// Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7
// Accept-Encoding: gzip, deflate
// Accept-Language: zh-CN,zh;q=0.9,zh-TW;q=0.8
// Cache-Control: max-age=0
// Upgrade-Insecure-Requests: 1
    return 0;
}

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
    printf("listen port: %d\n", port);
    return listen_fd;
}

int recv_cb(int fd, int event, void* arg);

int send_cb(int fd, int event, void* arg) {
    reactor_t* reactor = (reactor_t*)arg;
    connect_t* conn = connect_idx(reactor, fd);

    http_response(conn);

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

    int ret = recv(fd, conn->rbuffer, BUFFER_LENGTH, 0);
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
    else {
        conn->rc += ret;

        /* echo
        memcpy(conn->wbuffer, conn->rbuffer, ret);  // conn->rc
        conn->wc = conn->rc;
        */
        http_request(conn);

        conn->cb = send_cb;

        struct epoll_event ev;
        ev.events = EPOLLOUT;
        ev.data.fd = fd;
        epoll_ctl(reactor->epfd, EPOLL_CTL_MOD, fd, &ev);
    }
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

    reactor_t reactor;
    if (init_reactor(&reactor) != 0) {
        printf("reactor init failed.\n");
    }

    int port = atoi(argv[1]);

    for (int i = 0; i < 1; i++) {
        int listen_fd = init_server(port + i);
        if (listen_fd < 0) {
            printf("init server failed.\n");
        }

        if (set_listener(&reactor, listen_fd, accept_cb) < 0) {
            printf("set listener failed.\n");
        }
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