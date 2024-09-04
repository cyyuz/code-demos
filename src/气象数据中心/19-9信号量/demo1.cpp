#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include "public.h"

struct person
{
    char name[51];   // 姓名，注意，不能用string。
    int  no;         // 编号。
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "Using:./demo no name\n";
        return -1;
    }
    // 1. shmget：创建共享内存。
    int key = 0x5005;
    int shmid = shmget(key, sizeof(person), 0640 | IPC_CREAT);
    if (shmid == -1) {
        printf("shmget %d failed.\n", key);
        return -1;
    }
    std::cout << "shmid=" << shmid << std::endl;

    // 2. shmat: 把共享内存连接到当前进程的地址空间
    person* ptr = (person*)shmat(shmid, 0, 0);
    if (ptr == (void*)-1) {
        std::cout << "shmat() failed\n";
        return -1;
    }

    //
    idc::csemp mutex;
    if(mutex.init(0x5005) == false){
        std::cout << "mutex.init(0x5005) failed." << std::endl;
    }

    std::cout << "加锁" << std::endl;
    mutex.wait();
    std::cout << "加锁成功" << std::endl;

    // 3. 使用共享内存，对共享内存进行读/写。
    std::cout << "old value: no=" << ptr->no << ",name=" << ptr->name << std::endl;
    ptr->no = atoi(argv[1]);
    strcpy(ptr->name, argv[2]);
    // ptr->name = argv[2];
    std::cout << "new value: no=" << ptr->no << ",name=" << ptr->name << std::endl;

    sleep(10);
    mutex.post();
    std::cout << "解锁" << std::endl;

    // 4. shmdt: 把共享内存从当前进程中分离。
    shmdt(ptr);

    // 5. shmctl: 删除共享内存。
    if (shmctl(shmid, IPC_RMID, 0) == -1) {
        std::cout << "shmctl failed\n";
        return -1;
    }
}