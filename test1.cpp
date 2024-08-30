#include <iostream>
#include <unistd.h>
int main(int argc, char* argv[]) {
#if 1
    if (execl("/bin/ls", "ls", "-l", NULL) == -1) {
        perror("execl");
    }
#else
    char* args[5];
    args[0] = (char*)"ls";
    args[1] = (char*)"-l";
    args[2] = NULL;
    if (execv("/bin/ls", args) == -1) {
        perror("execl");
    }
#endif
}