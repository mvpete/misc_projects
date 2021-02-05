#ifndef __memory_h__
#define __memory_h__

#include <stdint.h>
#include <array>
#include <limits>

enum mmaps
{
    kbsr = 0xFE00,
    kbdr = 0xFE02
};

class memory
{
public:
    uint16_t read(uint16_t address);
    void write(uint16_t address, uint16_t value);
    uint16_t *get();

private:
    std::array<uint16_t, std::numeric_limits<uint16_t>::max()> memory_ = { 0 };
};

#endif // __memory_h__