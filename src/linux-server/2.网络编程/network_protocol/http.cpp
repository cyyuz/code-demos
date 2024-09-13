#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

const int BUFFER_LENGTH = 1024;
const int EVENT_LENGTH = 1024;
const int BLOCK_LENGTH = 1024;
const int KEY_MAX_LENGTH = 128;
const int VALUE_MAX_LENGTH = 512;
const int MAX_KEY_COUNT = 128;

typedef int (*CALLBACK)(int fd, int events, void* arg);

struct connect_t
{
    int        fd;
    CALLBACK   cb;
    int        count;
    char       rbuffer[BUFFER_LENGTH];
    int        rc;
    char       wbuffer[BUFFER_LENGTH];
    int        wc;   // 读取的字节数
    char       resource[BUFFER_LENGTH];
    bool       enable_sendfile;
    kvstore_t* kvheader;
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

struct kvpair_t
{
    char key[KEY_MAX_LENGTH];
    char value[VALUE_MAX_LENGTH];
};

struct kvstore_t
{
    struct kvpair_t* table;
    int              max_pairs;   // 最大键值对数量
    int              num_pairs;   // 当前存储的键值对数量
};

int init_kvpair(kvstore_t* kvstore) {
    if (!kvstore) return -1;
    kvstore->table = (kvpair_t*)calloc(MAX_KEY_COUNT, sizeof(kvpair_t));
    if (!kvstore->table) return -2;

    kvstore->max_pairs = MAX_KEY_COUNT;
    kvstore->num_pairs = 0;
}

int destroy_kvpair(kvstore_t* kvstore) {
    if (!kvstore) return -1;
    if (!kvstore->table) {
        free(kvstore->table);
    }
}

int put_kvpair(kvstore_t* kvstore, const char* key, const char* value) {
    if (!kvstore || !kvstore->table || !key || !value) return -1;

    if (kvstore->num_pairs < kvstore->max_pairs) {
        // lock
        int idx = kvstore->num_pairs++;
        // unlock
        strncpy(kvstore->table[idx].key, key, KEY_MAX_LENGTH);
        strncpy(kvstore->table[idx].value, value, VALUE_MAX_LENGTH);
        // kvstore->num_pairs++;
        return 0;
    }
    else {
        return -1;
    }
}

char* get_kvpair(kvstore_t* kvstore, const char* key) {
    for (int i = 0; i < kvstore->num_pairs; i++) {
        if (strcmp(kvstore->table[i].key, key)) {
            return kvstore->table[i].value;
        }
    }
    return NULL;
}

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

#define HTTP_WEB_ROOT "/home/cyyu/code-demos/src/linux-server/2.网络编程/network_protocol"

int http_response(connect_t* conn) {
#if 0
    int len = sprintf(conn->wbuffer,
                      "HTTP/1.1 200 OK\r\n"
                      "Accept-Ranges: bytes\r\n"
                      "Content-Length: 71\r\n"
                      "Content-Type: text/html\r\n"
                      "Date: Sat, 06 Aug 2022 13:16:46 GMT\r\n\r\n"
                      "<html><head><title>test</title></head><body><h1>CyYu</h1><body/></html>");
#elif 0
    printf("resource=%s\n", conn->resource);

    int filefd = open(conn->resource, O_RDONLY);
    if (filefd == -1) {
        printf("open failed\n");
        return -1;
    }
    struct stat stat_buf;
    fstat(filefd, &stat_buf);
    int len = sprintf(conn->wbuffer,
                      "HTTP/1.1 200 OK\r\n"
                      "Accept-Ranges: bytes\r\n"
                      "Content-Length: %ld\r\n"
                      "Content-Type: text/html\r\n"
                      "Date: Sat, 06 Aug 2022 13:16:46 GMT\r\n\r\n",
                      stat_buf.st_size);
    len += read(filefd, conn->wbuffer + len, BUFFER_LENGTH - len);

    close(filefd);
#elif 0
    int filefd = open(conn->resource, O_RDONLY);
    if (filefd == -1) {
        printf("errno=%d: %s\n", errno, strerror(errno));
        return -1;
    }
    struct stat stat_buf;
    fstat(filefd, &stat_buf);
    close(filefd);
    int len = sprintf(conn->wbuffer,
                      "HTTP/1.1 200 OK\r\n"
                      "Accept-Ranges: bytes\r\n"
                      "Content-Length: %ld\r\n"
                      "Content-Type: text/html\r\n"
                      "Date: Sat, 06 Aug 2022 13:16:46 GMT\r\n\r\n",
                      stat_buf.st_size);
    conn->enable_sendfile = true;
#elif 1
    int filefd = open(conn->resource, O_RDONLY);
    if (filefd == -1) {
        printf("errno=%d: %s\n", errno, strerror(errno));
        return -1;
    }
    struct stat stat_buf;
    fstat(filefd, &stat_buf);
    close(filefd);
    int len = sprintf(conn->wbuffer,
                      "HTTP/1.1 200 OK\r\n"
                      "Accept-Ranges: bytes\r\n"
                      "Content-Length: %ld\r\n"
                      "Content-Type: image/jpg\r\n"
                      "Date: Sat, 06 Aug 2022 13:16:46 GMT\r\n\r\n",
                      stat_buf.st_size);
    conn->enable_sendfile = true;
#endif
    conn->wc = len;
    return 0;
}

int http_request(connect_t* conn) {
    printf("\nhttp request:\n\n%s\n", conn->rbuffer);
    char line_buffer[1024] = {0};
    int  idx = read_line(conn->rbuffer, 0, line_buffer);

    if (strstr(line_buffer, "GET")) {
        int i = 0;
        while (line_buffer[sizeof("GET ") + i] != ' ') i++;
        line_buffer[sizeof("GET ") + i] = '\0';

        sprintf(conn->resource, "%s/%s", HTTP_WEB_ROOT, line_buffer + 4);
        // printf("%s\n",line_buffer + 4);
    }
    else {
        while (idx != -1) {
            idx = read_line(conn->rbuffer, idx, line_buffer);

            char* key = line_buffer;
            int   i = 0;
            while (key[i++] != ':');
            key[i] = '\0';
            char* value = line_buffer + i + 1;
            put_kvpair(conn->kvheader, key, value);
        }
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

#if 1
    if (conn->enable_sendfile) {
        int filefd = open(conn->resource, O_RDONLY);
        if (filefd == -1) {
            printf("errno=%d\n", errno);
            return -1;
        }
        struct stat stat_buf;
        fstat(filefd, &stat_buf);

        int ret = sendfile(fd, filefd, NULL, stat_buf.st_size);   // send body
        if (ret == -1) {
            printf("errno=%d\n", errno);
        }
        close(filefd);
        conn->enable_sendfile = false;
    }
#endif

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
    conn->kvheader = (kvstore_t*)malloc(sizeof(kvstore_t));
    init_kvpair(conn->kvheader);

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