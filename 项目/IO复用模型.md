
```cpp

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




















