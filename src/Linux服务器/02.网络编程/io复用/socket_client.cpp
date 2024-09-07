#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("using ./client 127.0.0.1 5005\n");
        return -1;
    }

    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(-1);
    }

    struct hostent* h;
    if ((h = gethostbyname(argv[1])) == 0) {
        printf("gethostbyname failed.\n");
        close(sockfd);
        return -1;
    }
    struct sockaddr_in seraddr;
    memset(&seraddr, 0, sizeof(seraddr));
    seraddr.sin_family = AF_INET;
    seraddr.sin_port = htons(atoi(argv[2]));
    memcpy(&seraddr.sin_addr, h->h_addr, h->h_length);
    if (connect(sockfd, (struct sockaddr*)&seraddr, sizeof(struct sockaddr)) != 0) {
        perror("connect");
        close(sockfd);
        return -1;
    }

    int  iret;
    char buffer[1024];
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        scanf("%s", buffer);
        if ((iret = send(sockfd, buffer, strlen(buffer), 0)) <= 0) {
            perror("send");
            break;
        }
        printf("send: %s\n", buffer);
        if ((iret = recv(sockfd, buffer, sizeof(buffer), 0)) <= 0) {
            perror("recv");
            break;
        }
        printf("recv: %s\n", buffer);
    }

    return 0;
}
