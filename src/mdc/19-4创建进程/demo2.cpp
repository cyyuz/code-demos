#include <fstream>
#include <iostream>
#include <unistd.h>

int main() {
    std::ofstream fout;
    fout.open("/tmp/tmp.txt");
    fork();
    for (int i = 0; i < 10000; i++)  
    {
        fout << "进程" << getpid() << "==" << i << "\n";
    }

    fout.close();
}