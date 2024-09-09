#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void func(int sig)
{
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

int main() {
    signal(SIGCHLD, func);
    if (fork() > 0) {
        while (true) {
            std::cout << "父进程忙着执行任务。\n";
            sleep(1);
        }
    }
    else {
        // sleep(5);
        int *p=0; *p=10;
        exit(1);
    }
}