#include "src/public/m_string.h"
#include <iostream>
#include <unistd.h>



int main(int argc, char* argv[], char* envp[]) {
    int a = 0;
    if (!cyyu::m_string::stoi_safe("!100.1.1@!", a)) {
        std::cout << "error" << std::endl;
    }
    std::cout << a << std::endl;


    return 0;
}


