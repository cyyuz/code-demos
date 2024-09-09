#include <iostream>
#include <signal.h>
#include <unistd.h>

void FathEXIT(int sig);   // 父进程的信号处理函数。

void ChldEXIT(int sig);   // 子进程的信号处理函数。

int main() {
    for (int ii = 1; ii <= 64; ii++) signal(ii, SIG_IGN);
    signal(SIGTERM, FathEXIT);   // 15
    signal(SIGINT, FathEXIT);    // 2
    while (true) {
        if (fork() > 0) {
            sleep(5);
            continue;
        }
        else {
            // 子进程需要重新设置信号。
            signal(SIGTERM, ChldEXIT);   // 子进程的退出函数与父进程不一样。
            signal(SIGINT, SIG_IGN);     // 子进程不需要捕获SIGINT信号。
            while (true) {
                std::cout << "子进程" << getpid() << "正在运行中。\n";
                sleep(3);
                continue;
            }
        }
    }
}

void FathEXIT(int sig) {
    // 以下代码是为了防止信号处理函数在执行的过程中再次被信号中断。
    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    std::cout << "父进程退出，sig=" << sig << std::endl;
    kill(0, SIGTERM);   // 向全部的子进程发送15的信号，通知它们退出。
    // 在这里增加释放资源的代码（全局的资源）。
    exit(0);
}

void ChldEXIT(int sig) {
    // 以下代码是为了防止信号处理函数在执行的过程中再次被信号中断。
    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    std::cout << "子进程" << getpid() << "退出，sig=" << sig << std::endl;
    // 在这里增加释放资源的代码（只释放子进程的资源）。
    exit(0);
}