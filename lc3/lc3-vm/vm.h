#ifndef __vm_h__
#define __vm_h__

#include "memory.h"

#include <istream>

enum registers
{
    r0 = 0,
    r1,
    r2,
    r3,
    r4,
    r5,
    r6,
    r7,
    pc,
    cond,
    count
};

class vm
{
public:
    vm();
    ~vm();
    void load(std::istream &stream);
    void run();
    

private:
    uint16_t next_instruction();
    void set_cc(uint16_t reg);
    uint16_t& pc();
    uint16_t regaddr_at(uint16_t loc, uint16_t inst);

private:
    void add(uint16_t inst);
    void do_and(uint16_t inst);
    void do_not(uint16_t inst);
    void br(uint16_t inst);
    void jmp(uint16_t inst);
    void jsr(uint16_t inst);
    void ld(uint16_t inst);
    void ldi(uint16_t inst);
    void ldr(uint16_t inst);
    void lea(uint16_t inst);
    void st(uint16_t inst);
    void sti(uint16_t inst);
    void str(uint16_t inst);
    void trap(uint16_t inst);

private:
    void getc();
    void out();
    void puts();
    void in();
    void putsp();
    void halt();

private:
    void enable_echo(bool enable);

private:
    bool running_;
    memory memory_;
    std::array<uint16_t, registers::count> registers_ = { 0 };

};

#endif // __vm_h__