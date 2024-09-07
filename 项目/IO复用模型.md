# 1 概念
- select模型：只能管理1024个客户端连接。
- poll模型：可以管理更多的客户端连接，但是连接越多，性能线性下降。
- epoll：只要内存够，管理连接数没有上限，性能不会下降。


- 水平触发

> 如果事件和数据已经在缓冲区里，程序调用select()时会报告事件，数据也不会丢失；
> 
> 如果select()已经报告了事件，但是程序没有处理它，下次调用select()的时候会重新报告。


# 2 select
## 2.1 事件

- 可读：1)新客户端的连接请求accept；2)客户端有报文到达recv，可以读；3)客户端连接已断开；

> TCP有缓冲区，如果缓冲区已满，send函数会阻塞
>
> 如果发送端关闭了socket，缓冲区的数据会继续发送给接收端。

- 可写：4)可以向客户端发送报文send，可以写。

> 如果tcp缓冲区没有满，那么socket连接是可写的。
>
> tcp发送缓冲区2.5M，接收缓冲区1M。
>
> 在高并发和流媒体传输场景中，缓冲区有填满的可能。

select：等待事件的发生（监视哪些socket发生了事件）。

select用位图（bitmap）表示socket的集合，

- 缺点

> 支持连接数太小(1024)，调整的意义不大，因为连接越多性能越差。
> 
> select()是内核函数，每次调用都要把fdset从用户态拷贝到内核态，调用select()之后，把fdset从内核态拷贝到用户态。
> 
> select()返回后，需要遍历bitmap，效率比较低。虽然bitmap比较小，但拷贝次数多。

## 2.2 select模型 

```cpp
FD_ZERO();  // 位图清0
FD_SET();   // 置1
FD_CLR();   // 置0
FD_ISSET(); // 判断是否存在，存在返回1
```

- 超时时间

```cpp
struct timeval
{
    long tv_sec; // 秒
    long tv_usec; // 微妙 
}
```




# 2 poll
- poll和select本质上没有区别，弃用了bitmap，采用数组表示法。

```cpp
struct pollfd {
    int   fd;         /* file descriptor */
    short events;     /* requested events */
    short revents;    /* returned events */
};
```


# 3 epoll

epoll没有内存拷贝，没有轮询，没有遍历。

```cpp
// 创建句柄
int epoll_create(int size);   

// 注册事件
int epoll_ctl(int epfd,int op,int fd,struct epoll_event *event);

// 等待事件
int epoll_wait(int epfd,struct epoll_event *events,int maxevents,int timeout);

struct epoll_event{
    uint32_t events;
    epoll_data_t data;
};
```

```cpp
// 初始化服务端的监听端口。
int initserver(int port);

int main(int argc,char *argv[])
{
    if (argc != 2) { printf("usage: ./tcppoll port\n"); return -1; }
    // 初始化服务端用于监听的socket。
    int listensock = initserver(atoi(argv[1]));
    printf("listensock=%d\n",listensock);
    if (listensock < 0) { printf("initserver() failed.\n"); return -1; }

    int epollfd = epoll_create(1); // 创建epoll句柄

    struct epoll_event ev;   // 声名事件的数据结构
    ev.events = EPOLLIN;     // 读事件
    ev.data.fd = listensock; // 指定事件的自定义数据，会随着epoll一并返回

    epoll_ctl(epollfd,EPOLL_CTL_ADD,listensock,&ev);   // EPOLL_CTL_ADD/DEL/MOD 事件加入epollfd
    struct epoll_event evs[10];
    while(true)
    {
        // 调用epoll_wait监视有事件发生的socket
        int infds = epoll_wait(epollfd,evs,10,3000);    // 超时事件填0会立即返回，填-1没有事件会一直等待，

        if(infds < 0) { perror("epoll() failed."); break; }  //返回失败

        if(infds == 0) {printf("epoll() timeout.\n"); continue; }// 超时

        // infds>0,有事件发生的socket数量 
        // 遍历epoll返回的发生事件的数组evs
        for(int i=0;i<infds;i++)
        {
            printf("infds=%d",infds);
            printf("事件信息：events=%d,data.fd=%d\n",evs[i].events,evs[i].data.fd);
            if(evs[i].data.fd == listensock)  // 判断是不是监听socket
            {
                struct sockaddr_in client;
                socklen_t len = sizeof(client);
                int clientsock = accept(listensock,(struct sockaddr*)&client,&len);
                if(clientsock < 0) {perror("accept"); continue;}
                printf("accept client(socket=%d)\n",clientsock);
                // 把客户端socket加入可读集合
                ev.data.fd = clientsock;
                ev.events = EPOLLIN;
                epoll_ctl(epollfd,EPOLL_CTL_ADD,clientsock,&ev);
            }
            else
            {
                // 客户端socket有事件，表示有报文发过来或断开连接
                char buffer[1024];
                memset(buffer,0,sizeof(buffer));
                if(recv(evs[i].data.fd,buffer,sizeof(buffer),0) <= 0) // 客户端已断开
                {
                    printf("client(eventfd=%d) disconnect.\n",evs[i].data.fd);
                    close(evs[i].data.fd);   // 关闭socket
                    // 不需要删除事件，系统自己处理
                }
                else // recv接收成功，则有报文发过来
                {
                    printf("recv(eventfd=%d):%s\n",evs[i].data.fd,buffer);
                    // 原封不动发回去
                    send(evs[i].data.fd,buffer,strlen(buffer),0);
                }
            }
        }
    }
    return 0;
}            
```




















