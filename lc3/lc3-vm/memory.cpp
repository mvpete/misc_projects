#include "memory.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <conio.h>

uint16_t check_key()
{
    HANDLE csl = GetStdHandle(STD_INPUT_HANDLE);
    return WaitForSingleObject(csl, 1000) == WAIT_OBJECT_0 && _kbhit();
}

uint16_t memory::read(uint16_t addr)
{
    if (addr == mmaps::kbsr)
    {
        if (check_key())
        {
            memory_[mmaps::kbsr] = (1 << 15);
            memory_[mmaps::kbdr] = _getch();
        }
        else
        {
            memory_[mmaps::kbsr] = 0;
        }
    }
    return memory_[addr];
}

void memory::write(uint16_t addr, uint16_t value)
{
    memory_[addr] = value;
}

uint16_t *memory::get()
{
    return memory_.data();
}
