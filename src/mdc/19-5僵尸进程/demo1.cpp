#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
int main() {
    if (fork() > 0) {
        int   sts;
        pid_t pid = wait(&sts);
        std::cout << "已终止的子进程编号是：" << pid << std::endl;
        if (WIFEXITED(sts)) {
            std::cout << "子进程是正常退出的，退出状态是：" << WEXITSTATUS(sts) << std::endl;
        }
        else {
            std::cout << "子进程是异常退出的，终止它的信号是：" << WTERMSIG(sts) << std::endl;
        }
    }
    else { 
        int* p = 0;
        *p = 10;
        exit(1);
    }
}