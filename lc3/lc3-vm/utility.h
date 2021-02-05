#ifndef __utility_h__
#define __utility_h__

#include <stdint.h>
#include <intrin.h>

inline uint16_t sign_extend(uint16_t value, int bit_count)
{
    if ((value >> (bit_count - 1)) & 1)
    {
        value |= (0xFFFF << bit_count);
    }
    return value;
}

inline uint16_t flip16(uint16_t value)
{
    return _byteswap_ushort(value);
}


#endif // __utility_h__