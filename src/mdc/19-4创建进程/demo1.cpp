/**
 * fork()函数创建进程
 */

#include <iostream>
#include <string>
#include <unistd.h>

int main() {
    std::string str = "hello world";
    pid_t pid = fork();
    if(pid > 0){
        sleep(1);
        std::cout << "parent process   " << std::endl;
        std::cout << "parent pid:" << pid << std::endl;
        std::cout << "parent str:" << str << std::endl;
    }else if(pid == 0){
        str = "hello child";
        std::cout << "child process    " << std::endl;
        std::cout << "child pid:" << pid << std::endl;
        std::cout << "child str:" << str << std::endl;
    }
}