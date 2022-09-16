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

```cpp
// 初始化服务端的监听端口。
int initserver(int port);

int main(int argc,char *argv[])
{
    if (argc != 2) { printf("usage: ./tcpselect port\n"); return -1; }

    // 初始化服务端用于监听的socket。
    int listensock = initserver(atoi(argv[1]));
    printf("listensock=%d\n",listensock);
    if (listensock < 0) { printf("initserver() failed.\n"); return -1; }

    fd_set readfds; // 读事件集合
    FD_ZERO(&readfds);   // 初始化集合
    FD_SET(listensock,&readfds);  // 监听socket加入集合
    int maxfd = listensock;    // 记录最大值

    while(true)
    {
        // 调用select监视有事件发生的socket
        fd_set tmpfds = readfds;// 副本传入select，传出的是有事件的socket的位图  
        struct timeval timeout; timeout.tv_sec = 10;timeout.tv_usec = 0; // 超时时间
        
        int infds = select(maxfd+1,&tmpfds,NULL,NULL,&timeout);  // 最大值+1，读事件，写事件，，超时时间

        if(infds < 0) { perror("select() failed."); break; }  //返回失败

        if(infds == 0) {printf("select() timeout.\n"); continue; }// 超时

        // infds>0,有事件发生的socket数量
        for(int eventfd=0;eventfd<=maxfd;eventfd++)
        {
            if(FD_ISSET(eventfd,&tmpfds)<=0) continue;  // 判断是否有事件
            // 如果有事件 判断是什么事件
            if(eventfd == listensock) // 判断是不是监听socket
            {
                struct sockaddr_in client;
                socklen_t len = sizeof(client);
                int clientsock = accept(listensock,(struct sockaddr*)&client,&len);
                if(clientsock < 0) {perror("accept"); continue;}
                printf("accept client(socket=%d)\n",clientsock);
                // 把客户端socket加入可读集合
                FD_SET(clientsock,&readfds);
                if(maxfd<clientsock) maxfd = clientsock;  // 更新maxfd
            }
            else
            {
                // 客户端socket有事件，表示有报文发过来或断开连接
                char buffer[1024];
                memset(buffer,0,sizeof(buffer));
                if(recv(eventfd,buffer,sizeof(buffer),0) <= 0) // 客户端已断开
                {
                    printf("client(eventfd=%d) disconnect.\n",eventfd);
                    close(eventfd);   // 关闭socket
                    FD_CLR(eventfd,&readfds); // 从集合删除socket
                    // 如果删除的是最后一个socket，重新计算maxfd的值
                    if(eventfd == maxfd)
                    {
                        for(int i = maxfd;i>0;i--)
                        {
                            if(FD_ISSET(i,&readfds)) maxfd = i; break;
                        }
                    }
                }
                else // recv接收成功，则有报文发过来
                {
                    printf("recv(eventfd=%d):%s\n",eventfd,buffer);
                    // 原封不动发回去
                    send(eventfd,buffer,strlen(buffer),0);
                }
            }
        }
    }
    return 0;
}

// 初始化服务端的监听端口。
int initserver(int port)
{
  int sock = socket(AF_INET,SOCK_STREAM,0);
  if (sock < 0)  {perror("socket() failed"); return -1;}

  int opt = 1; unsigned int len = sizeof(opt);
  setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&opt,len);

  struct sockaddr_in servaddr;
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(port);

  if (bind(sock,(struct sockaddr *)&servaddr,sizeof(servaddr)) < 0 )
  {perror("bind() failed"); close(sock); return -1;}

  if (listen(sock,5) != 0 )  {perror("listen() failed"); close(sock); return -1;}
  return sock;
}  
```

# poll
- poll和select本质上没有区别，弃用了bitmap，采用数组表示法。

```cpp
struct pollfd {
    int   fd;         /* file descriptor */
    short events;     /* requested events */
    short revents;    /* returned events */
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

    struct pollfd fds[1024]; //fds代替了位图
    for(int i=0;i<1024;i++)  fds[i].fd = -1; // 初始化数组

    fds[listensock].fd = listensock;
    fds[listensock].events = POLLIN; // events存放事件 POLLIN / POLLOUT

    int maxfd = listensock;    // 记录最大值
    while(true)
    {
        // 调用poll监视有事件发生的socket
        int infds = poll(fds,maxfd+1,5000);  // 单位微妙
        // 超时事件填0会立即返回，填-1没有事件会一直等待，

        if(infds < 0) { perror("poll() failed."); break; }  //返回失败

        if(infds == 0) {printf("poll() timeout.\n"); continue; }// 超时

        // infds>0,有事件发生的socket数量 
        for(int eventfd=0;eventfd<=maxfd;eventfd++)
        {
            if(fds[eventfd].fd < 0) continue;  // 判断fd

            // 如果有事件 判断是什么事件
            if((fds[eventfd].revents&POLLIN) == 0) continue; // 没事时间continue
            fds[eventfd].revents = 0; // 清空revents

            if(eventfd == listensock) // 判断是不是监听socket
            {
                struct sockaddr_in client;
                socklen_t len = sizeof(client);
                int clientsock = accept(listensock,(struct sockaddr*)&client,&len);
                if(clientsock < 0) {perror("accept"); continue;}
                printf("accept client(socket=%d)\n",clientsock);
                // 把客户端socket加入可读集合
                fds[clientsock].fd = clientsock;
                fds[clientsock].events = POLLIN;
                fds[clientsock].revents = 0;
                if(maxfd<clientsock) maxfd = clientsock;  // 更新maxfd
            }
            else
            {
                // 客户端socket有事件，表示有报文发过来或断开连接
                char buffer[1024];
                memset(buffer,0,sizeof(buffer));
                if(recv(eventfd,buffer,sizeof(buffer),0) <= 0) // 客户端已断开
                {
                    printf("client(eventfd=%d) disconnect.\n",eventfd);
                    close(eventfd);   // 关闭socket
                    fds[eventfd].fd = -1; // 从集合删除socket
                    // 如果删除的是最后一个socket，重新计算maxfd的值
                    if(eventfd == maxfd)
                    {
                        for(int i = maxfd;i>0;i--)
                        {
                            if(fds[i].fd != -1) maxfd = i; break;
                        }
                    }
                }
                else // recv接收成功，则有报文发过来
                {
                    printf("recv(eventfd=%d):%s\n",eventfd,buffer);
                    // 原封不动发回去
                    send(eventfd,buffer,strlen(buffer),0);
                }
            }
        }
    }
    return 0;
}    
```






















