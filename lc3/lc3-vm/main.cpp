
#include <fstream>
#include "vm.h"

int main(int argc, const char **argv)
{
    std::ifstream obj_s("2048.obj", std::ios::binary);
    vm machine;
    machine.load(obj_s);
    machine.run();

}